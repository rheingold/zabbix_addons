/*
 * proc_status_plugin.c - Process and Service Status Monitoring Plugin
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 30, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Monitors Windows processes and services availability status.
 *   Returns 1 (running), 0 (stopped/not running), -1 (error/not found).
 *
 * COMPILATION (MinGW64):
 *   gcc -shared -o proc_status_plugin.dll proc_status_plugin.c -ladvapi32 -Wall -O2
 *
 * DEPLOYMENT:
 *   1. Copy proc_status_plugin.dll to C:\Zabbix\bin\plugins\
 *   2. Restart Zabbix Agent 2 service
 *
 * METRICS EXPOSED:
 *   - proc.running[<source>,<agg>,<window>]   - Process running status
 *   - service.status[<source>,<agg>,<window>] - Service status
 *
 * PARAMETERS:
 *   All metrics accept three parameters (all optional, can use empty brackets []):
 *   
 *   [<source>,<agg>,<window>]
 *   
 *   1. <source> - Source filter (string, semicolon-separated for multiple)
 *      - Process name substring (e.g., "notepad", "chrome", "explorer")
 *      - Service name substring (e.g., "zabbix", "agent", "Print Spooler")
 *      - Multiple filters: Semicolon-separated (e.g., "notepad;chrome;firefox")
 *      - Special values: "*" or "all" or empty = return ALL processes/services
 *      - Matching: Case-insensitive substring match on name OR full path
 *      - Examples: "notepad", "zabbix;agent", "*", ""
 *   
 *   2. <agg> - Aggregation function (string)
 *      - "last"   - Most recent value (default) - best for status checks
 *      - "avg"    - Average of all values in time window
 *      - "min"    - Minimum value (0 if stopped at any point)
 *      - "max"    - Maximum value (1 if running at any point)
 *      - "sum"    - Sum of all values
 *      - "count"  - Number of samples collected
 *      - Examples: "last", "avg", "max"
 *   
 *   3. <window> - Time window in seconds (number)
 *      - "0" or empty: Use default collection window (typically 60-300 seconds)
 *      - Positive number: Use specific time window (e.g., "60", "300", "3600")
 *      - Examples: "0", "60", "300"
 *   
 *   USAGE EXAMPLES:
 *   - aggplugin.proc.running[]                       - All processes, last value, default window
 *   - aggplugin.proc.running[notepad,last,0]         - Process "notepad", last value
 *   - aggplugin.proc.running[notepad;chrome,last,0]  - Multiple processes (OR logic)
 *   - aggplugin.proc.running[zabbix,avg,300]         - Processes matching "zabbix", 5-min avg
 *   - aggplugin.service.status[Zabbix Agent 2,last,0] - Specific service by display name
 *   - aggplugin.service.status[*,last,0]             - All services, last value
 *
 * SOURCE_ID FORMAT:
 *   "friendly_name|full_path"
 *   Examples:
 *   - Process: "notepad.exe|C:\Windows\System32\notepad.exe"
 *   - Service: "Zabbix Agent 2|C:\zabbix\bin\zabbix_agent2.exe"
 *
 * FILTERING LOGIC:
 *   - Query filter: Semicolon-separated terms, case-insensitive substring match
 *   - Matches either friendly name OR full path
 *   - Multiple terms use OR logic (any match returns the source)
 *   - "*" or "all" or empty = return all collected items
 *   - Example: "zabbix;agent" matches "Zabbix Agent 2" OR any path containing "agent"
 *
 * RETURN VALUES:
 *   - 1 = Running/Active
 *   - 0 = Stopped/Not running
 *   - -1 = Error state (service pending, paused, etc.)
 *
 * NOTES:
 *   - Uses Windows Toolhelp32 API for process enumeration
 *   - Uses Windows Service Control Manager API for service status
 *   - Collects ALL processes/services by default (can enumerate many items!)
 *   - Query-time filtering via Zabbix item key parameter 1 (semicolon-separated)
 *   - Filtering is case-insensitive substring match on name OR path
 */

#include "../cpp_common/measurement_plugin_api.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * CONFIGURATION & STATE
 * ======================================================================== */

#define MAX_FILTERS 100
#define MAX_PATH_LEN 512
#define MAX_NAME_LEN 256

typedef struct {
    char process_filters[MAX_FILTERS][MAX_NAME_LEN];
    int process_filter_count;
    char service_filters[MAX_FILTERS][MAX_NAME_LEN];
    int service_filter_count;
} plugin_config_t;

static plugin_config_t config = {0};
static int initialized = 0;

/* ========================================================================
 * HELPER FUNCTIONS
 * ======================================================================== */

/**
 * case_insensitive_strstr - Case-insensitive substring search
 */
static const char* case_insensitive_strstr(const char* haystack, const char* needle) {
    if (!haystack || !needle) return NULL;
    
    size_t needle_len = strlen(needle);
    if (needle_len == 0) return haystack;
    
    for (const char* p = haystack; *p; p++) {
        if (_strnicmp(p, needle, needle_len) == 0) {
            return p;
        }
    }
    return NULL;
}

/**
 * matches_filter - Check if name matches any filter (case-insensitive substring)
 * Returns 1 if matches, 0 if no match
 */
static int matches_filter(const char* name, char filters[][MAX_NAME_LEN], int filter_count) {
    if (filter_count == 0) {
        return 1;  // No filters = match all
    }
    
    for (int i = 0; i < filter_count; i++) {
        if (case_insensitive_strstr(name, filters[i])) {
            return 1;
        }
    }
    return 0;
}

/**
 * get_process_path - Get full path for a process
 */
static int get_process_path(DWORD pid, char* path, size_t path_size) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) {
        return 0;
    }
    
    DWORD size = (DWORD)path_size;
    if (!QueryFullProcessImageName(hProcess, 0, path, &size)) {
        CloseHandle(hProcess);
        return 0;
    }
    
    CloseHandle(hProcess);
    return 1;
}

/**
 * parse_config - Parse configuration string for filters
 * Format: "process_filter=notepad.exe,chrome.exe|service_filter=Zabbix Agent 2,Spooler"
 */
static void parse_config(const char* config_section) {
    if (!config_section) return;
    
    // Simple parser for key=value pairs
    char buffer[4096];
    strncpy(buffer, config_section, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* line = strtok(buffer, "\n\r");
    while (line) {
        // Trim leading whitespace
        while (*line == ' ' || *line == '\t') line++;
        
        if (strncmp(line, "process_filter=", 15) == 0) {
            char* value = line + 15;
            char* token = strtok(value, ",");
            while (token && config.process_filter_count < MAX_FILTERS) {
                // Trim whitespace
                while (*token == ' ') token++;
                char* end = token + strlen(token) - 1;
                while (end > token && (*end == ' ' || *end == '\r' || *end == '\n')) {
                    *end = '\0';
                    end--;
                }
                
                if (strlen(token) > 0) {
                    strncpy(config.process_filters[config.process_filter_count], 
                           token, MAX_NAME_LEN - 1);
                    config.process_filters[config.process_filter_count][MAX_NAME_LEN - 1] = '\0';
                    config.process_filter_count++;
                }
                token = strtok(NULL, ",");
            }
        }
        else if (strncmp(line, "service_filter=", 15) == 0) {
            char* value = line + 15;
            char* token = strtok(value, ",");
            while (token && config.service_filter_count < MAX_FILTERS) {
                // Trim whitespace
                while (*token == ' ') token++;
                char* end = token + strlen(token) - 1;
                while (end > token && (*end == ' ' || *end == '\r' || *end == '\n')) {
                    *end = '\0';
                    end--;
                }
                
                if (strlen(token) > 0) {
                    strncpy(config.service_filters[config.service_filter_count], 
                           token, MAX_NAME_LEN - 1);
                    config.service_filters[config.service_filter_count][MAX_NAME_LEN - 1] = '\0';
                    config.service_filter_count++;
                }
                token = strtok(NULL, ",");
            }
        }
        
        line = strtok(NULL, "\n\r");
    }
}

/* ========================================================================
 * PLUGIN METADATA REGISTRATION
 * ======================================================================== */

static const metric_key_info_t plugin_keys[] = {
    {
        "proc.running",
        "Process running status (1=running, 0=not running)",
        METRIC_TYPE_UINT64,
        "name|path",
        1  // has_multiple_sources
    },
    {
        "service.status",
        "Windows service status (1=running, 0=stopped, -1=not found)",
        METRIC_TYPE_UINT64,
        "name|path",
        1  // has_multiple_sources
    }
};

static const plugin_info_t plugin_metadata = {
    "ProcStatus",
    "1.0",
    "Claude Sonnet 4.5 (AI) + Lukas Plachy (lukas@plachy.eu)",
    sizeof(plugin_keys) / sizeof(metric_key_info_t),
    plugin_keys
};

/* ========================================================================
 * REQUIRED PLUGIN FUNCTIONS
 * ======================================================================== */

/**
 * plugin_get_info - Returns plugin metadata
 */
PLUGIN_EXPORT const plugin_info_t* plugin_get_info() {
    return &plugin_metadata;
}

/**
 * plugin_init - Initialize plugin and parse configuration
 */
PLUGIN_EXPORT int plugin_init(const char* config_section) {
    memset(&config, 0, sizeof(config));
    
    if (config_section) {
        parse_config(config_section);
    }
    
    fprintf(stderr, "ProcStatusPlugin: Initialized\n");
    fprintf(stderr, "ProcStatusPlugin: Process filters: %d\n", config.process_filter_count);
    for (int i = 0; i < config.process_filter_count; i++) {
        fprintf(stderr, "  - %s\n", config.process_filters[i]);
    }
    fprintf(stderr, "ProcStatusPlugin: Service filters: %d\n", config.service_filter_count);
    for (int i = 0; i < config.service_filter_count; i++) {
        fprintf(stderr, "  - %s\n", config.service_filters[i]);
    }
    
    initialized = 1;
    return 0;
}

/**
 * collect_processes - Enumerate running processes
 */
static int collect_processes(measurement_value_t** out_values) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "ProcStatusPlugin: Failed to create process snapshot\n");
        return 0;
    }
    
    // First pass: count matching processes
    int count = 0;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (matches_filter(pe32.szExeFile, config.process_filters, config.process_filter_count)) {
                count++;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    if (count == 0) {
        CloseHandle(hSnapshot);
        return 0;
    }
    
    // Allocate result array
    measurement_value_t* values = (measurement_value_t*)malloc(sizeof(measurement_value_t) * count);
    if (!values) {
        CloseHandle(hSnapshot);
        return 0;
    }
    
    // Second pass: collect data
    int idx = 0;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (matches_filter(pe32.szExeFile, config.process_filters, config.process_filter_count)) {
                char path[MAX_PATH_LEN] = "";
                
                // Try to get full path
                if (!get_process_path(pe32.th32ProcessID, path, sizeof(path))) {
                    strncpy(path, "(unavailable)", sizeof(path) - 1);
                }
                
                // Use only process name (without path) to keep source_id stable
                // This prevents memory explosion from unique PIDs/paths
                char* source_id = (char*)malloc(MAX_NAME_LEN);
                snprintf(source_id, MAX_NAME_LEN, "%s", pe32.szExeFile);
                
                values[idx].source_id = source_id;
                values[idx].value = 1.0;  // Running
                values[idx].str_value = NULL;
                idx++;
                
                if (idx >= count) break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    *out_values = values;
    return idx;
}

/**
 * collect_services - Enumerate Windows services
 */
static int collect_services(measurement_value_t** out_values) {
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (!hSCM) {
        fprintf(stderr, "ProcStatusPlugin: Failed to open Service Control Manager\n");
        return 0;
    }
    
    DWORD bytesNeeded = 0;
    DWORD servicesReturned = 0;
    DWORD resumeHandle = 0;
    
    // First call to get required buffer size
    EnumServicesStatusEx(hSCM, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                        NULL, 0, &bytesNeeded, &servicesReturned, &resumeHandle, NULL);
    
    if (bytesNeeded == 0) {
        CloseServiceHandle(hSCM);
        return 0;
    }
    
    BYTE* buffer = (BYTE*)malloc(bytesNeeded);
    if (!buffer) {
        CloseServiceHandle(hSCM);
        return 0;
    }
    
    if (!EnumServicesStatusEx(hSCM, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                             buffer, bytesNeeded, &bytesNeeded, &servicesReturned, 
                             &resumeHandle, NULL)) {
        free(buffer);
        CloseServiceHandle(hSCM);
        return 0;
    }
    
    ENUM_SERVICE_STATUS_PROCESS* services = (ENUM_SERVICE_STATUS_PROCESS*)buffer;
    
    // Count matching services
    int count = 0;
    for (DWORD i = 0; i < servicesReturned; i++) {
        if (matches_filter(services[i].lpServiceName, config.service_filters, config.service_filter_count) ||
            matches_filter(services[i].lpDisplayName, config.service_filters, config.service_filter_count)) {
            count++;
        }
    }
    
    if (count == 0) {
        free(buffer);
        CloseServiceHandle(hSCM);
        return 0;
    }
    
    // Allocate result array
    measurement_value_t* values = (measurement_value_t*)malloc(sizeof(measurement_value_t) * count);
    if (!values) {
        free(buffer);
        CloseServiceHandle(hSCM);
        return 0;
    }
    
    // Collect matching services
    int idx = 0;
    for (DWORD i = 0; i < servicesReturned && idx < count; i++) {
        if (matches_filter(services[i].lpServiceName, config.service_filters, config.service_filter_count) ||
            matches_filter(services[i].lpDisplayName, config.service_filters, config.service_filter_count)) {
            
            // Get service binary path
            char path[MAX_PATH_LEN] = "(unavailable)";
            SC_HANDLE hService = OpenService(hSCM, services[i].lpServiceName, SERVICE_QUERY_CONFIG);
            if (hService) {
                DWORD bytesNeeded2 = 0;
                QueryServiceConfig(hService, NULL, 0, &bytesNeeded2);
                if (bytesNeeded2 > 0) {
                    QUERY_SERVICE_CONFIG* pConfig = (QUERY_SERVICE_CONFIG*)malloc(bytesNeeded2);
                    if (pConfig) {
                        if (QueryServiceConfig(hService, pConfig, bytesNeeded2, &bytesNeeded2)) {
                            strncpy(path, pConfig->lpBinaryPathName, sizeof(path) - 1);
                            path[sizeof(path) - 1] = '\0';
                        }
                        free(pConfig);
                    }
                }
                CloseServiceHandle(hService);
            }
            
            // Use only service display name (without path) to keep source_id stable
            // This prevents memory explosion from varying service PIDs/paths
            char* source_id = (char*)malloc(MAX_NAME_LEN);
            snprintf(source_id, MAX_NAME_LEN, "%s", services[i].lpDisplayName);
            
            values[idx].source_id = source_id;
            
            // Determine status: 1=running, 0=stopped, -1=error
            if (services[i].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING) {
                values[idx].value = 1.0;
            } else if (services[i].ServiceStatusProcess.dwCurrentState == SERVICE_STOPPED ||
                      services[i].ServiceStatusProcess.dwCurrentState == SERVICE_PAUSED) {
                values[idx].value = 0.0;
            } else {
                values[idx].value = -1.0;  // Other states (pending, etc.)
            }
            
            values[idx].str_value = NULL;
            idx++;
        }
    }
    
    free(buffer);
    CloseServiceHandle(hSCM);
    *out_values = values;
    return idx;
}

/**
 * plugin_collect - Collect process and service status measurements
 */
PLUGIN_EXPORT size_t plugin_collect(collection_result_t* results) {
    if (!initialized) {
        results[0].key = "proc.running";
        results[0].status = COLLECT_ERROR;
        results[0].value_count = 0;
        results[0].values = NULL;
        
        results[1].key = "service.status";
        results[1].status = COLLECT_ERROR;
        results[1].value_count = 0;
        results[1].values = NULL;
        
        return 2;
    }
    
    // Collect processes
    measurement_value_t* proc_values = NULL;
    int proc_count = collect_processes(&proc_values);
    
    results[0].key = "proc.running";
    results[0].status = COLLECT_OK;
    results[0].value_count = proc_count;
    results[0].values = proc_values;
    
    // Collect services
    measurement_value_t* svc_values = NULL;
    int svc_count = collect_services(&svc_values);
    
    results[1].key = "service.status";
    results[1].status = COLLECT_OK;
    results[1].value_count = svc_count;
    results[1].values = svc_values;
    
    return 2;
}

/**
 * plugin_deinit - Clean up plugin resources
 */
PLUGIN_EXPORT int plugin_deinit() {
    initialized = 0;
    memset(&config, 0, sizeof(config));
    return 0;
}
