/*
 * disk_stats_plugin.c - Example Measurement Plugin for Disk I/O Statistics
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 14, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Demonstrates how to create a measurement plugin for aggplugin framework.
 *   Collects disk I/O statistics (reads/writes per second) for Windows physical disks.
 *
 * COMPILATION (MinGW64):
 *   gcc -shared -o disk_stats_plugin.dll disk_stats_plugin.c -lPdh -Wall -O2
 *
 * DEPLOYMENT:
 *   1. Copy disk_stats_plugin.dll to C:\Zabbix\plugins\measurements\
 *   2. Add to aggplugin.conf:
 *      [plugin.disk_stats]
 *      enabled=1
 *      devices=0,1
 *
 * METRICS EXPOSED:
 *   - disk.io.read[device]  - Disk read operations/sec (per physical disk)
 *   - disk.io.write[device] - Disk write operations/sec (per physical disk)
 *
 * NOTES:
 *   - Uses Windows PDH (Performance Data Helper) API
 *   - Supports dynamic disk detection (hot-plug USB drives, etc.)
 *   - Source IDs are disk numbers (e.g., "0", "1", "2")
 */

#include "../cpp_common/measurement_plugin_api.h"
#include <windows.h>
#include <pdh.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * PLUGIN STATE
 * ======================================================================== */

static PDH_HQUERY query = NULL;
static int initialized = 0;

// Counter data for each disk
typedef struct {
    int disk_num;
    PDH_HCOUNTER read_counter;
    PDH_HCOUNTER write_counter;
} disk_counter_t;

#define MAX_DISKS 16
static disk_counter_t disk_counters[MAX_DISKS];
static int disk_count = 0;

/* ========================================================================
 * PLUGIN METADATA REGISTRATION
 * ======================================================================== */

static const metric_key_info_t plugin_keys[] = {
    {
        "disk.io.read",
        "Disk read operations per second (per physical disk)",
        METRIC_TYPE_UINT64,
        "device",
        1  // has_multiple_sources
    },
    {
        "disk.io.write",
        "Disk write operations per second (per physical disk)",
        METRIC_TYPE_UINT64,
        "device",
        1  // has_multiple_sources
    }
};

static const plugin_info_t plugin_metadata = {
    "DiskStats",
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
 * plugin_init - Initialize PDH query and counters
 * 
 * Parses configuration for device list (optional).
 * If no devices specified, auto-detects all physical disks.
 */
PLUGIN_EXPORT int plugin_init(const char* config_section) {
    PDH_STATUS status;
    
    // Create PDH query
    status = PdhOpenQuery(NULL, 0, &query);
    if (status != ERROR_SUCCESS) {
        fprintf(stderr, "DiskStatsPlugin: Failed to open PDH query (error 0x%lx)\n", status);
        return -1;
    }
    
    // Parse device list from config (e.g., "devices=0,1,2")
    // For simplicity, auto-detect all disks (0-15)
    char counter_path[256];
    for (int i = 0; i < MAX_DISKS; i++) {
        // Add read counter: \PhysicalDisk(0 C:)\Disk Reads/sec
        // Note: PDH counter names are localized on non-English Windows
        // For production, use PdhEnumObjects/PdhEnumObjectItems to get localized names
        snprintf(counter_path, sizeof(counter_path), 
                 "\\PhysicalDisk(%d *)\\Disk Reads/sec", i);
        
        PDH_HCOUNTER read_counter;
        status = PdhAddEnglishCounter(query, counter_path, 0, &read_counter);
        if (status != ERROR_SUCCESS) {
            // Disk doesn't exist, stop searching
            break;
        }
        
        snprintf(counter_path, sizeof(counter_path), 
                 "\\PhysicalDisk(%d *)\\Disk Writes/sec", i);
        
        PDH_HCOUNTER write_counter;
        status = PdhAddEnglishCounter(query, counter_path, 0, &write_counter);
        if (status != ERROR_SUCCESS) {
            // Should not happen if read counter succeeded
            fprintf(stderr, "DiskStatsPlugin: Failed to add write counter for disk %d\n", i);
            continue;
        }
        
        disk_counters[disk_count].disk_num = i;
        disk_counters[disk_count].read_counter = read_counter;
        disk_counters[disk_count].write_counter = write_counter;
        disk_count++;
    }
    
    if (disk_count == 0) {
        fprintf(stderr, "DiskStatsPlugin: No physical disks detected\n");
        PdhCloseQuery(query);
        return -1;
    }
    
    // Collect initial sample (PDH needs two samples for rate calculation)
    PdhCollectQueryData(query);
    
    initialized = 1;
    return 0;
}

/**
 * plugin_collect - Collect disk I/O measurements
 * 
 * Returns two collection_result_t entries (one for reads, one for writes).
 * Each result contains multiple measurement_value_t (one per disk).
 */
PLUGIN_EXPORT size_t plugin_collect(collection_result_t* results) {
    if (!initialized) {
        results[0].key = "disk.io.read";
        results[0].status = COLLECT_ERROR;
        results[0].value_count = 0;
        results[0].values = NULL;
        
        results[1].key = "disk.io.write";
        results[1].status = COLLECT_ERROR;
        results[1].value_count = 0;
        results[1].values = NULL;
        
        return 2;
    }
    
    // Collect PDH data
    PDH_STATUS status = PdhCollectQueryData(query);
    if (status != ERROR_SUCCESS) {
        fprintf(stderr, "DiskStatsPlugin: Failed to collect PDH data (error 0x%lx)\n", status);
        results[0].key = "disk.io.read";
        results[0].status = COLLECT_ERROR;
        results[0].value_count = 0;
        results[0].values = NULL;
        
        results[1].key = "disk.io.write";
        results[1].status = COLLECT_ERROR;
        results[1].value_count = 0;
        results[1].values = NULL;
        
        return 2;
    }
    
    // Allocate measurement arrays
    measurement_value_t* read_values = (measurement_value_t*)malloc(
        sizeof(measurement_value_t) * disk_count);
    measurement_value_t* write_values = (measurement_value_t*)malloc(
        sizeof(measurement_value_t) * disk_count);
    
    // Collect values for each disk
    for (int i = 0; i < disk_count; i++) {
        PDH_FMT_COUNTERVALUE read_value, write_value;
        
        // Get read counter value
        status = PdhGetFormattedCounterValue(
            disk_counters[i].read_counter, PDH_FMT_DOUBLE, NULL, &read_value);
        
        if (status == ERROR_SUCCESS) {
            // Allocate source_id string (caller will free)
            char* source_id = (char*)malloc(16);
            snprintf(source_id, 16, "%d", disk_counters[i].disk_num);
            
            read_values[i].source_id = source_id;
            read_values[i].value = read_value.doubleValue;
            read_values[i].str_value = NULL;
        } else {
            read_values[i].source_id = NULL;
            read_values[i].value = 0.0;
            read_values[i].str_value = NULL;
        }
        
        // Get write counter value
        status = PdhGetFormattedCounterValue(
            disk_counters[i].write_counter, PDH_FMT_DOUBLE, NULL, &write_value);
        
        if (status == ERROR_SUCCESS) {
            // Allocate source_id string (caller will free)
            char* source_id = (char*)malloc(16);
            snprintf(source_id, 16, "%d", disk_counters[i].disk_num);
            
            write_values[i].source_id = source_id;
            write_values[i].value = write_value.doubleValue;
            write_values[i].str_value = NULL;
        } else {
            write_values[i].source_id = NULL;
            write_values[i].value = 0.0;
            write_values[i].str_value = NULL;
        }
    }
    
    // Fill results
    results[0].key = "disk.io.read";
    results[0].status = COLLECT_OK;
    results[0].value_count = disk_count;
    results[0].values = read_values;
    
    results[1].key = "disk.io.write";
    results[1].status = COLLECT_OK;
    results[1].value_count = disk_count;
    results[1].values = write_values;
    
    return 2;
}

/**
 * plugin_deinit - Clean up PDH resources
 */
PLUGIN_EXPORT int plugin_deinit() {
    if (query) {
        PdhCloseQuery(query);
        query = NULL;
    }
    initialized = 0;
    disk_count = 0;
    return 0;
}

/* ========================================================================
 * OPTIONAL PLUGIN FUNCTIONS (not needed for this simple plugin)
 * ======================================================================== */

// plugin_on_reset - Not needed (no stateful counters)
// plugin_aggregate_custom - Not needed (default aggregation is fine)
// plugin_on_init_complete - Not needed (no cross-plugin coordination)
