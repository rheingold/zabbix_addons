// pingtool_lib.c - ICMP ping library for Zabbix Agent2 Go plugin
// Zabbix Pingtool v2.0 (Agent2 Go Plugin) | December 2, 2025
// Refactored from pingtool.c standalone tool

#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "pingtool_lib.h"

typedef struct {
    int seq;
    double ms;
    bool ok;
    const char *err;
} ping_sample_t;

static int resolve_host(const char *host, bool prefer_ipv6, struct addrinfo **out_ai) {
    struct addrinfo hints; 
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = prefer_ipv6 ? AF_INET6 : AF_INET;
    
    int rc = getaddrinfo(host, NULL, &hints, out_ai);
    if (rc == 0) return (*out_ai)->ai_family;
    
    hints.ai_family = AF_UNSPEC;
    rc = getaddrinfo(host, NULL, &hints, out_ai);
    if (rc != 0) return AF_UNSPEC;
    return (*out_ai)->ai_family;
}

static DWORD now_ms(void) {
    return GetTickCount();
}

static double clamp_zero(double v) { 
    return v < 0 ? 0 : v; 
}

static int ping_ipv4(const struct sockaddr_in *sin, int count, int timeout_ms, int ttl, 
                     int payload_len, bool df, ping_sample_t *samples) {
    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) return -1;

    if (payload_len < 0) payload_len = 32;
    if (payload_len > 1400) payload_len = 1400;
    
    char *sendbuf = (char*)malloc((size_t)payload_len);
    if (!sendbuf) { IcmpCloseHandle(hIcmp); return -1; }
    memset(sendbuf, 'A', (size_t)payload_len);

    IP_OPTION_INFORMATION opts; 
    memset(&opts, 0, sizeof(opts));
    if (ttl > 0 && ttl <= 255) opts.Ttl = (UCHAR)ttl;
    if (df) opts.Flags = 0x02;

    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + (DWORD)payload_len + 64;
    char *replyBuf = (char*)malloc(replySize);
    if (!replyBuf) { free(sendbuf); IcmpCloseHandle(hIcmp); return -1; }

    for (int i = 0; i < count; i++) {
        DWORD t0 = now_ms();
        DWORD ret = IcmpSendEcho(hIcmp, sin->sin_addr.S_un.S_addr,
                                 sendbuf, (WORD)payload_len,
                                 &opts, replyBuf, replySize, timeout_ms);
        DWORD t1 = now_ms();
        
        samples[i].seq = i + 1;
        samples[i].err = NULL;
        
        if (ret == 0) {
            DWORD gle = GetLastError();
            samples[i].ok = false;
            samples[i].ms = -1.0;
            if (gle == IP_REQ_TIMED_OUT) samples[i].err = "timeout";
            else if (gle == IP_BAD_DESTINATION) samples[i].err = "bad_destination";
            else samples[i].err = "error";
        } else {
            PICMP_ECHO_REPLY rep = (PICMP_ECHO_REPLY)replyBuf;
            double rtt = rep->RoundTripTime ? (double)rep->RoundTripTime : (double)(t1 - t0);
            samples[i].ok = (rep->Status == IP_SUCCESS);
            samples[i].ms = samples[i].ok ? clamp_zero(rtt) : -1.0;
            if (!samples[i].ok) samples[i].err = "icmp_error";
        }
    }

    free(replyBuf);
    free(sendbuf);
    IcmpCloseHandle(hIcmp);
    return 0;
}

static int ping_ipv6(const struct sockaddr_in6 *sin6, int count, int timeout_ms, int ttl,
                     int payload_len, ping_sample_t *samples) {
#if _WIN32_WINNT >= 0x0600
    HANDLE hIcmp6 = Icmp6CreateFile();
    if (hIcmp6 == INVALID_HANDLE_VALUE) return -1;
    
    if (payload_len < 0) payload_len = 32;
    if (payload_len > 1400) payload_len = 1400;
    
    char *sendbuf = (char*)malloc((size_t)payload_len);
    if (!sendbuf) { IcmpCloseHandle(hIcmp6); return -1; }
    memset(sendbuf, 'A', (size_t)payload_len);

    DWORD replySize = sizeof(ICMPV6_ECHO_REPLY) + (DWORD)payload_len + 64;
    char *replyBuf = (char*)malloc(replySize);
    if (!replyBuf) { free(sendbuf); IcmpCloseHandle(hIcmp6); return -1; }

    struct sockaddr_in6 src; 
    memset(&src, 0, sizeof(src)); 
    src.sin6_family = AF_INET6;

    for (int i = 0; i < count; i++) {
        DWORD t0 = now_ms();
        DWORD ret = Icmp6SendEcho2(hIcmp6, NULL, NULL, NULL,
                                   (struct sockaddr_in6*)&src, (struct sockaddr_in6*)sin6,
                                   sendbuf, (WORD)payload_len,
                                   NULL, replyBuf, replySize, timeout_ms);
        DWORD t1 = now_ms();
        
        samples[i].seq = i + 1;
        samples[i].err = NULL;
        
        if (ret == 0) {
            DWORD gle = GetLastError();
            samples[i].ok = false;
            samples[i].ms = -1.0;
            if (gle == IP_REQ_TIMED_OUT) samples[i].err = "timeout";
            else samples[i].err = "error";
        } else {
            PICMPV6_ECHO_REPLY rep6 = (PICMPV6_ECHO_REPLY)replyBuf;
            double rtt = (double)(t1 - t0);
            samples[i].ok = (rep6->Status == IP_SUCCESS);
            samples[i].ms = samples[i].ok ? clamp_zero(rtt) : -1.0;
            if (!samples[i].ok) samples[i].err = "icmp_error";
        }
    }

    free(replyBuf);
    free(sendbuf);
    IcmpCloseHandle(hIcmp6);
    return 0;
#else
    (void)sin6; (void)count; (void)timeout_ms; (void)ttl; (void)payload_len; (void)samples;
    return -1;
#endif
}

static int build_json_array(const char *target, int count, const ping_sample_t *samples,
                           char *output, int output_size) {
    // Compute summary
    int okc = 0; 
    double sum = 0.0; 
    int okcnt = 0;
    
    for (int i = 0; i < count; i++) {
        if (samples[i].ok && samples[i].ms >= 0) { 
            sum += samples[i].ms; 
            okc++; 
        }
        if (samples[i].ok) okcnt++;
    }
    
    double success_ratio = count > 0 ? ((double)okcnt) / (double)count : 0.0;
    double avg_ms = okc > 0 ? (sum / (double)okc) : 0.0;

    // Build JSON array string
    int written = 0;
    int rem = output_size;
    
    written += snprintf(output + written, rem - written, "[\n");
    
    for (int i = 0; i < count && written < rem; i++) {
        written += snprintf(output + written, rem - written,
                          "  {\"target\":\"%s\",\"seq\":%d,",
                          target, samples[i].seq);
        
        if (samples[i].ms >= 0) {
            written += snprintf(output + written, rem - written, "\"ms\":%.3f,", samples[i].ms);
        } else {
            written += snprintf(output + written, rem - written, "\"ms\":null,");
        }
        
        written += snprintf(output + written, rem - written, "\"ok\":%s",
                          samples[i].ok ? "true" : "false");
        
        if (samples[i].err) {
            written += snprintf(output + written, rem - written, ",\"err\":\"%s\"", samples[i].err);
        }
        
        written += snprintf(output + written, rem - written, "},\n");
    }
    
    // Summary as final array element
    written += snprintf(output + written, rem - written,
                      "  {\"target\":\"%s\",\"type\":\"summary\",\"success_ratio\":%.4f,\"avg_ms\":%.3f}\n",
                      target, success_ratio, avg_ms);
    
    written += snprintf(output + written, rem - written, "]\n");
    
    if (written >= rem) return -1; // Buffer too small
    
    return 0;
}

// Public API function
int ping_execute(const char *target, int count, int timeout_ms, int ttl, int payload_len,
                int df, int ipv6, char *output, int output_size) {
    if (!target || !output || output_size <= 0) return -1;
    if (count <= 0 || count > 100) return -1;

    // Init Winsock
    WSADATA wsa; 
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return -1;

    struct addrinfo *ai = NULL;
    int fam = resolve_host(target, (bool)ipv6, &ai);
    if (fam == AF_UNSPEC || !ai) {
        WSACleanup();
        return -1;
    }

    ping_sample_t *samples = (ping_sample_t*)calloc((size_t)count, sizeof(ping_sample_t));
    if (!samples) {
        freeaddrinfo(ai);
        WSACleanup();
        return -1;
    }

    int rc = -1;
    if (ai->ai_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in*)ai->ai_addr;
        rc = ping_ipv4(sin, count, timeout_ms, ttl, payload_len, (bool)df, samples);
    } else if (ai->ai_family == AF_INET6) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)ai->ai_addr;
        rc = ping_ipv6(sin6, count, timeout_ms, ttl, payload_len, samples);
    }

    // Fallback: if IPv6 failed and not forced, try IPv4
    if (rc != 0 && ai->ai_family == AF_INET6 && !ipv6) {
        freeaddrinfo(ai);
        ai = NULL;
        fam = resolve_host(target, false, &ai);
        if (ai && ai->ai_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in*)ai->ai_addr;
            rc = ping_ipv4(sin, count, timeout_ms, ttl, payload_len, (bool)df, samples);
        }
    }

    if (rc == 0) {
        rc = build_json_array(target, count, samples, output, output_size);
    }

    free(samples);
    if (ai) freeaddrinfo(ai);
    WSACleanup();
    
    return rc;
}
