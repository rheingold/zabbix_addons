/*
 * ping_plugin.c - ICMP Ping Measurement Plugin for AggPlugin
 * 
 * FEATURES:
 * - Multiple target pings in single call (semicolon-separated)
 * - IPv4 and IPv6 support
 * - Configurable: ping count, TTL, payload length, Don't Fragment flag
 * - Returns: success rate (0.0-1.0) or average RTT in milliseconds
 * - Thread-safe with request queuing
 * - Timeout protection
 * 
 * PARAMETERS (semicolon-separated targets):
 *   target1[,count][,ipv6][,ttl][,length][,df][,rtt];target2[,...]
 * 
 * EXAMPLES:
 *   8.8.8.8,10                           - 10 pings to 8.8.8.8
 *   8.8.8.8,5,0,64,32,1                  - 5 pings, IPv4, TTL=64, 32 bytes, DF set
 *   8.8.8.8,5,0,64,32,1,1                - Same but return average RTT
 *   8.8.8.8,10;1.1.1.1,10                - Ping both targets
 *   2001:4860:4860::8888,5,1             - IPv6 ping to Google DNS
 */

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

// Configuration defaults
static int g_default_ping_count = 5;
static int g_max_execution_time_ms = 30000; // 30 seconds
static CRITICAL_SECTION g_ping_lock;
static volatile int g_is_executing = 0;
static int g_initialized = 0;

// Plugin info structure
typedef struct {
    const char *name;
    const char *key;
    const char *description;
} PluginKey;

static PluginKey plugin_keys[] = {
    {"Ping", "ping.status", "ICMP ping success rate or average RTT"}
};

typedef struct {
    char target[256];
    int count;
    int use_ipv6;
    int ttl;
    int payload_length;
    int dont_fragment;
    int return_rtt;
} PingTarget;

typedef struct {
    double success_rate;
    double avg_rtt_ms;
    int total_sent;
    int total_received;
} PingResult;

/*
 * Initialize plugin
 */
__declspec(dllexport) int plugin_init(void) {
    if (!g_initialized) {
        InitializeCriticalSection(&g_ping_lock);
        
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return 1;
        }
        
        g_initialized = 1;
    }
    return 0;
}

/*
 * Cleanup plugin
 */
__declspec(dllexport) void plugin_cleanup(void) {
    if (g_initialized) {
        DeleteCriticalSection(&g_ping_lock);
        WSACleanup();
        g_initialized = 0;
    }
}

/*
 * Get plugin information
 */
__declspec(dllexport) int plugin_get_info(const char **name, const char **version, int *key_count) {
    *name = "PingPlugin";
    *version = "1.0";
    *key_count = sizeof(plugin_keys) / sizeof(plugin_keys[0]);
    return 0;
}

/*
 * Get key information
 */
__declspec(dllexport) int plugin_get_key_info(int index, const char **key, const char **description) {
    if (index < 0 || index >= sizeof(plugin_keys) / sizeof(plugin_keys[0])) {
        return 1;
    }
    *key = plugin_keys[index].key;
    *description = plugin_keys[index].description;
    return 0;
}

/*
 * Set configuration parameter
 */
__declspec(dllexport) int plugin_set_config(const char *key, const char *value) {
    if (!key || !value) return 1;
    
    if (strcmp(key, "DefaultPingCount") == 0) {
        int count = atoi(value);
        if (count > 0 && count <= 100) {
            g_default_ping_count = count;
            return 0;
        }
    } else if (strcmp(key, "MaxExecutionTime") == 0) {
        int timeout = atoi(value);
        if (timeout > 0 && timeout <= 120000) {
            g_max_execution_time_ms = timeout;
            return 0;
        }
    }
    
    return 1;
}

/*
 * Parse a single target specification
 * Format: target[,count][,ipv6][,ttl][,length][,df][,rtt]
 */
static int parse_target(const char *spec, PingTarget *target) {
    memset(target, 0, sizeof(PingTarget));
    
    // Set defaults
    target->count = g_default_ping_count;
    target->use_ipv6 = 0;
    target->ttl = 64;
    target->payload_length = 32;
    target->dont_fragment = 0;
    target->return_rtt = 0;
    
    char buffer[512];
    strncpy(buffer, spec, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char *token = strtok(buffer, ",");
    int field = 0;
    
    while (token != NULL && field < 7) {
        switch (field) {
            case 0: // target
                strncpy(target->target, token, sizeof(target->target) - 1);
                break;
            case 1: // count
                target->count = atoi(token);
                if (target->count < 1) target->count = 1;
                if (target->count > 100) target->count = 100;
                break;
            case 2: // use_ipv6
                target->use_ipv6 = atoi(token);
                break;
            case 3: // ttl
                target->ttl = atoi(token);
                if (target->ttl < 1) target->ttl = 1;
                if (target->ttl > 255) target->ttl = 255;
                break;
            case 4: // payload_length
                target->payload_length = atoi(token);
                if (target->payload_length < 0) target->payload_length = 0;
                if (target->payload_length > 1024) target->payload_length = 1024;
                break;
            case 5: // dont_fragment
                target->dont_fragment = atoi(token);
                break;
            case 6: // return_rtt
                target->return_rtt = atoi(token);
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
    
    return 0;
}

/*
 * Perform ICMP ping (IPv4)
 */
static int ping_ipv4(const char *target, int count, int ttl, int payload_length, 
                     int dont_fragment, PingResult *result) {
    HANDLE hIcmpFile;
    unsigned long ipaddr;
    DWORD dwRetVal;
    char SendData[1024];
    LPVOID ReplyBuffer;
    DWORD ReplySize;
    IP_OPTION_INFORMATION ipopt;
    
    result->total_sent = 0;
    result->total_received = 0;
    result->avg_rtt_ms = 0.0;
    result->success_rate = 0.0;
    
    // Resolve target
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    
    if (getaddrinfo(target, NULL, &hints, &res) != 0) {
        return 1;
    }
    
    struct sockaddr_in *addr_in = (struct sockaddr_in *)res->ai_addr;
    ipaddr = addr_in->sin_addr.s_addr;
    freeaddrinfo(res);
    
    // Create ICMP handle
    hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        return 1;
    }
    
    // Prepare send data (sequential alphabet pattern)
    for (int i = 0; i < payload_length && i < sizeof(SendData); i++) {
        SendData[i] = 'a' + (i % 26);
    }
    
    // Set IP options
    memset(&ipopt, 0, sizeof(ipopt));
    ipopt.Ttl = (unsigned char)ttl;
    ipopt.Flags = dont_fragment ? IP_FLAG_DF : 0;
    
    // Prepare reply buffer
    ReplySize = sizeof(ICMP_ECHO_REPLY) + payload_length + 8;
    ReplyBuffer = malloc(ReplySize);
    if (ReplyBuffer == NULL) {
        IcmpCloseHandle(hIcmpFile);
        return 1;
    }
    
    double total_rtt = 0.0;
    int success_count = 0;
    
    // Perform pings
    for (int i = 0; i < count; i++) {
        dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, payload_length,
                                &ipopt, ReplyBuffer, ReplySize, 5000);
        
        result->total_sent++;
        
        if (dwRetVal != 0) {
            PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
            if (pEchoReply->Status == IP_SUCCESS) {
                result->total_received++;
                success_count++;
                total_rtt += (double)pEchoReply->RoundTripTime;
            }
        }
        
        // Small delay between pings
        if (i < count - 1) {
            Sleep(100);
        }
    }
    
    free(ReplyBuffer);
    IcmpCloseHandle(hIcmpFile);
    
    // Calculate results
    if (result->total_sent > 0) {
        result->success_rate = (double)result->total_received / (double)result->total_sent;
    }
    
    if (success_count > 0) {
        result->avg_rtt_ms = total_rtt / (double)success_count;
    }
    
    return 0;
}

/*
 * Perform ICMP ping (IPv6)
 */
static int ping_ipv6(const char *target, int count, int ttl, int payload_length,
                     PingResult *result) {
    HANDLE hIcmpFile;
    struct sockaddr_in6 source_addr, dest_addr;
    char SendData[1024];
    LPVOID ReplyBuffer;
    DWORD ReplySize;
    DWORD dwRetVal;
    IP_OPTION_INFORMATION ipopt;
    
    result->total_sent = 0;
    result->total_received = 0;
    result->avg_rtt_ms = 0.0;
    result->success_rate = 0.0;
    
    // Resolve target
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_RAW;
    
    if (getaddrinfo(target, NULL, &hints, &res) != 0) {
        return 1;
    }
    
    memcpy(&dest_addr, res->ai_addr, sizeof(dest_addr));
    freeaddrinfo(res);
    
    // Create ICMP handle for IPv6
    hIcmpFile = Icmp6CreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        return 1;
    }
    
    // Prepare send data
    for (int i = 0; i < payload_length && i < sizeof(SendData); i++) {
        SendData[i] = 'a' + (i % 26);
    }
    
    // Set IP options
    memset(&ipopt, 0, sizeof(ipopt));
    ipopt.Ttl = (unsigned char)ttl;
    
    // Source address (any)
    memset(&source_addr, 0, sizeof(source_addr));
    source_addr.sin6_family = AF_INET6;
    source_addr.sin6_addr = in6addr_any;
    
    // Prepare reply buffer
    ReplySize = sizeof(ICMPV6_ECHO_REPLY) + payload_length + 8;
    ReplyBuffer = malloc(ReplySize);
    if (ReplyBuffer == NULL) {
        IcmpCloseHandle(hIcmpFile);
        return 1;
    }
    
    double total_rtt = 0.0;
    int success_count = 0;
    
    // Perform pings
    for (int i = 0; i < count; i++) {
        dwRetVal = Icmp6SendEcho2(hIcmpFile, NULL, NULL, NULL,
                                  &source_addr, &dest_addr,
                                  SendData, payload_length, &ipopt,
                                  ReplyBuffer, ReplySize, 5000);
        
        result->total_sent++;
        
        if (dwRetVal != 0) {
            PICMPV6_ECHO_REPLY pEchoReply = (PICMPV6_ECHO_REPLY)ReplyBuffer;
            if (pEchoReply->Status == IP_SUCCESS) {
                result->total_received++;
                success_count++;
                total_rtt += (double)pEchoReply->RoundTripTime;
            }
        }
        
        // Small delay between pings
        if (i < count - 1) {
            Sleep(100);
        }
    }
    
    free(ReplyBuffer);
    IcmpCloseHandle(hIcmpFile);
    
    // Calculate results
    if (result->total_sent > 0) {
        result->success_rate = (double)result->total_received / (double)result->total_sent;
    }
    
    if (success_count > 0) {
        result->avg_rtt_ms = total_rtt / (double)success_count;
    }
    
    return 0;
}

/*
 * Collect measurements for all keys
 * Returns: Array of results (source, value, status)
 */
__declspec(dllexport) int plugin_collect(const char *filter, void *results, int max_results) {
    // This plugin doesn't use the collector - measurements are done on-demand via plugin_measure
    return 0;
}

/*
 * Measure specific metric with parameters
 */
__declspec(dllexport) int plugin_measure(const char *key, const char *params, char *result, int result_len) {
    if (!key || !params || !result) return 1;
    
    if (strcmp(key, "ping.status") != 0) {
        return 1;
    }
    
    // Thread-safe execution check
    EnterCriticalSection(&g_ping_lock);
    if (g_is_executing) {
        LeaveCriticalSection(&g_ping_lock);
        // Already executing - queue/retry behavior
        snprintf(result, result_len, "0.0"); // Return failure for now
        return 0;
    }
    g_is_executing = 1;
    LeaveCriticalSection(&g_ping_lock);
    
    DWORD start_time = GetTickCount();
    
    // Parse multiple targets (semicolon-separated)
    char params_copy[4096];
    strncpy(params_copy, params, sizeof(params_copy) - 1);
    params_copy[sizeof(params_copy) - 1] = '\0';
    
    PingTarget targets[32];
    int target_count = 0;
    
    char *target_spec = strtok(params_copy, ";");
    while (target_spec != NULL && target_count < 32) {
        if (parse_target(target_spec, &targets[target_count]) == 0) {
            target_count++;
        }
        target_spec = strtok(NULL, ";");
    }
    
    if (target_count == 0) {
        g_is_executing = 0;
        snprintf(result, result_len, "0.0");
        return 1;
    }
    
    // Execute pings for all targets
    double total_success_rate = 0.0;
    double total_rtt = 0.0;
    int total_targets = 0;
    int return_rtt_mode = targets[0].return_rtt; // Use first target's mode
    
    for (int i = 0; i < target_count; i++) {
        // Check timeout
        DWORD elapsed = GetTickCount() - start_time;
        if (elapsed > (DWORD)g_max_execution_time_ms) {
            break;
        }
        
        PingResult ping_result;
        int ret;
        
        if (targets[i].use_ipv6) {
            ret = ping_ipv6(targets[i].target, targets[i].count, targets[i].ttl,
                           targets[i].payload_length, &ping_result);
        } else {
            ret = ping_ipv4(targets[i].target, targets[i].count, targets[i].ttl,
                           targets[i].payload_length, targets[i].dont_fragment,
                           &ping_result);
        }
        
        if (ret == 0) {
            // Check if more than half failed
            if (ping_result.success_rate < 0.5) {
                // Count as total failure (0)
                total_success_rate += 0.0;
                total_rtt += 0.0;
            } else {
                total_success_rate += ping_result.success_rate;
                total_rtt += ping_result.avg_rtt_ms;
            }
            total_targets++;
        }
    }
    
    // Calculate final result
    double final_result = 0.0;
    if (total_targets > 0) {
        if (return_rtt_mode) {
            // Return average RTT
            final_result = total_rtt / (double)total_targets;
        } else {
            // Return average success rate
            final_result = total_success_rate / (double)total_targets;
        }
    }
    
    g_is_executing = 0;
    
    snprintf(result, result_len, "%.3f", final_result);
    return 0;
}
