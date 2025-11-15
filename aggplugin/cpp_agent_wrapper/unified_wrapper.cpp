/*
 * unified_wrapper.cpp - Unified Agent2/Classic Plugin Wrapper
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 3, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * Uses build macros to adapt for different agent types:
 * - ZABBIX_CLASSIC_AGENT: Builds for Classic Zabbix Agent
 * - ZABBIX_AGENT2: Builds for Zabbix Agent2  
 * Both use the same Zabbix module interface (zbx_module_*).
 */

#include "plugin_common.hpp"
#include "collector.hpp"
#include <iostream>
#include <cstring>

// Conditional includes based on build target
#if defined(ZABBIX_CLASSIC_AGENT) || defined(ZABBIX_AGENT2)
#  if defined(__has_include)
#    if __has_include("../../../zabbixlib/include/module.h")
#      include "../../../zabbixlib/include/sysinc.h"
#      include "../../../zabbixlib/include/module.h"
#      define HAVE_ZABBIX_HEADERS 1
#    elif __has_include("../../../../zabbixlib/include/module.h")
#      include "../../../../zabbixlib/include/sysinc.h"
#      include "../../../../zabbixlib/include/module.h"
#      define HAVE_ZABBIX_HEADERS 1
#    endif
#  endif
#endif

using namespace std;

#ifdef HAVE_ZABBIX_HEADERS
// Using Zabbix module interface (works for both classic and agent2 with loadable modules)

// Forward declarations from plugin_common
extern "C" {
    extern const char* plugin_get_cpu_load();
    extern const char* plugin_get_memory_usage();
}

// Module API functions - using C++ linkage as declared in module.h
int zbx_module_api_version(void) {
    return ZBX_MODULE_API_VERSION;
}

// Plugin item functions
static int aggplugin_cpu_load(AGENT_REQUEST *request, AGENT_RESULT *result) {
    const char* value = plugin_get_cpu_load();
    char* allocated_value = (char*)malloc(strlen(value) + 1);
    strcpy(allocated_value, value);
    SET_STR_RESULT(result, allocated_value);
    return SYSINFO_RET_OK;
}

static int aggplugin_memory_usage(AGENT_REQUEST *request, AGENT_RESULT *result) {
    const char* value = plugin_get_memory_usage();
    char* allocated_value = (char*)malloc(strlen(value) + 1);
    strcpy(allocated_value, value);
    SET_STR_RESULT(result, allocated_value);
    return SYSINFO_RET_OK;
}

// Module metrics list
static ZBX_METRIC keys[] = {
    {(char*)"aggplugin.cpu_load", 0, aggplugin_cpu_load, NULL},
    {(char*)"aggplugin.memory_usage", 0, aggplugin_memory_usage, NULL},
    {NULL}  // Sentinel
};

ZBX_METRIC* zbx_module_item_list(void) {
    return keys;
}

int zbx_module_init(void) {
    // DEBUGGING: Sub-task 1a - Only simple log output, no threads
#ifdef ZABBIX_CLASSIC_AGENT
    cout << "DEBUG: Zabbix Classic Agent Plugin zbx_module_init() called - BASIC TEST" << endl;
#elif defined(ZABBIX_AGENT2)  
    cout << "DEBUG: Zabbix Agent2 Plugin zbx_module_init() called - BASIC TEST" << endl;
#else
    cout << "DEBUG: Generic Zabbix Plugin zbx_module_init() called - BASIC TEST" << endl;
#endif
    
    // TODO: Add collector_init() after basic loading is verified
    // collector_init(1.0);
    // collector_register_metric("aggplugin.cpu_load", 1.0);
    // collector_register_metric("aggplugin.memory_usage", 1.0);
    
    cout << "DEBUG: zbx_module_init() returning ZBX_MODULE_OK" << endl;
    return ZBX_MODULE_OK;
}

int zbx_module_uninit(void) {
    // DEBUGGING: Sub-task 1a - Only simple log output  
#ifdef ZABBIX_CLASSIC_AGENT
    cout << "DEBUG: Zabbix Classic Agent Plugin zbx_module_uninit() called" << endl;
#elif defined(ZABBIX_AGENT2)
    cout << "DEBUG: Zabbix Agent2 Plugin zbx_module_uninit() called" << endl;
#else
    cout << "DEBUG: Generic Zabbix Plugin zbx_module_uninit() called" << endl;
#endif
    
    // TODO: Add collector_stop() when we start using threads
    // collector_stop();
    
    return ZBX_MODULE_OK;
}

void zbx_module_item_timeout(int timeout) {
    // Handle item timeout if needed
}

#else
// Fallback: when neither ZABBIX_CLASSIC_AGENT nor ZABBIX_AGENT2 is defined, or headers are missing
extern "C" {
    int zbx_module_api_version(void) { return 0; }
    void* zbx_module_item_list(void) { return nullptr; }
    int zbx_module_init(void) { 
        cerr << "Zabbix headers not available or build macro not set" << endl;
        return -1; 
    }
    int zbx_module_uninit(void) { return -1; }
    void zbx_module_item_timeout(int timeout) {}
}
#endif
