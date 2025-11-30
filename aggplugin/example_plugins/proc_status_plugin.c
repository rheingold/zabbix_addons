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
 *   1. Copy proc_status_plugin.dll to C:\Zabbix\plugins\measurements\
 *   2. Add to aggplugin.conf:
 *      [plugin.proc_status]
 *      enabled=1
 *      # Optional performance filters (comma-separated, limits what plugin collects):
 *      process_filter=notepad.exe,chrome.exe,explorer.exe,zabbix_agent2.exe
 *      service_filter=Zabbix Agent 2,Windows Update,Print Spooler
 *
 * METRICS EXPOSED:
 *   - proc.running[source_filter,aggregation,period] - Process running status
 *   - service.status[source_filter,aggregation,period] - Service status
 *
 * ZABBIX ITEM KEY EXAMPLES:
 *   aggplugin.proc.running[notepad,last,0]              # Single process
 *   aggplugin.proc.running[notepad;chrome;explorer,last,0]  # Multiple (semicolon-separated)
 *   aggplugin.proc.running[*,avg,60]                    # All processes
 *   aggplugin.service.status[zabbix;agent,last,0]       # Services containing "zabbix" OR "agent"
 *   aggplugin.service.status[Zabbix Agent 2,last,0]     # Exact service display name match
 *
 * SOURCE_ID FORMAT:
 *   "friendly_name|full_path"
 *   Examples:
 *   - Process: "notepad.exe|C:\Windows\System32\notepad.exe"
 *   - Service: "Zabbix Agent 2|C:\zabbix\bin\zabbix_agent2.exe"
 *
 * FILTERING:
 *   - Config filter (optional): Limits what plugin collects (performance optimization)
 *   - Query filter: Semicolon-separated terms, case-insensitive substring match
 *   - Matches either friendly name OR full path
 *   - "*" or "all" = return all collected items
 *
 * RETURN VALUES:
 *   - 1 = Running/Active
 *   - 0 = Stopped/Not running
 *   - -1 = Error state (service pending, etc.)
 *
 * NOTES:
 *   - Uses Windows Toolhelp32 API for process enumeration
 *   - Uses Windows Service Control Manager API for service status
 *   - Config filter is WHITELIST: only specified processes/services are collected
 *   - If no config filters specified, collects ALL processes/services (can be many!)
 *   - Query-time filtering via Zabbix item key (semicolon-separated)
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
                
                // Format: "process_name.exe|C:\full\path\to\process.exe"
                char* source_id = (char*)malloc(MAX_PATH_LEN + MAX_NAME_LEN);
                snprintf(source_id, MAX_PATH_LEN + MAX_NAME_LEN, "%s|%s", 
                        pe32.szExeFile, path);
                
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
            
            // Format: "Display Name|C:\path\to\service.exe"
            char* source_id = (char*)malloc(MAX_PATH_LEN + MAX_NAME_LEN);
            snprintf(source_id, MAX_PATH_LEN + MAX_NAME_LEN, "%s|%s", 
                    services[i].lpDisplayName, path);
            
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
