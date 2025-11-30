/*
 * disk_stats_plugin.c - Example Measurement Plugin for Disk I/O Statistics
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 30, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Demonstrates how to create a measurement plugin for aggplugin framework.
 *   Collects disk I/O statistics (reads/writes per second) for Windows physical disks
 *   and logical drives (partitions).
 *
 * COMPILATION (MinGW64):
 *   gcc -shared -o disk_stats_plugin.dll disk_stats_plugin.c -lPdh -Wall -O2
 *
 * DEPLOYMENT:
 *   1. Copy disk_stats_plugin.dll to C:\Zabbix\bin\plugins\
 *   2. Restart Zabbix Agent 2 service
 *
 * METRICS EXPOSED:
 *   - disk.io.read[<source>,<agg>,<window>]           - Disk read operations/sec (physical)
 *   - disk.io.write[<source>,<agg>,<window>]          - Disk write operations/sec (physical)
 *   - storage.queue.length[<source>,<agg>,<window>]   - Queue length (physical disks)
 *   - vfs.queue.length[<source>,<agg>,<window>]       - Queue length (logical drives)
 *
 * PARAMETERS:
 *   All metrics accept three parameters (all optional, can use empty brackets []):
 *   
 *   [<source>,<agg>,<window>]
 *   
 *   1. <source> - Source identifier (string or number)
 *      - For disk.io.* and storage.queue.length: Physical disk number (e.g., "0", "1", "2")
 *      - For vfs.queue.length: Drive letter (e.g., "C:", "D:", "E:")
 *      - Special value "0" or empty: Returns aggregated data for all sources
 *      - Examples: "0", "1", "C:", "D:"
 *   
 *   2. <agg> - Aggregation function (string)
 *      - "last"   - Most recent value (default)
 *      - "avg"    - Average of all values in time window
 *      - "min"    - Minimum value
 *      - "max"    - Maximum value
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
 *   - aggplugin.storage.queue.length[]               - All sources, last value, default window
 *   - aggplugin.storage.queue.length[0,last,0]       - Disk 0, last value, default window
 *   - aggplugin.storage.queue.length[0,avg,300]      - Disk 0, average, 5-minute window
 *   - aggplugin.vfs.queue.length[C:,max,60]          - Drive C:, maximum, 1-minute window
 *   - aggplugin.disk.io.read[1,avg,0]                - Disk 1, average reads, default window
 *
 * SOURCE IDENTIFIERS:
 *   Physical Disk (disk.io.*, storage.queue.length):
 *   - Format: "0 *" means PhysicalDisk 0 (wildcard matches all partitions on that disk)
 *   - The asterisk (*) represents all drive letters/partitions on the physical disk
 *   - Example: "0 *" could include C:, D:, etc. on Physical Disk 0
 *   - Use: Query specific disk by number (0, 1, 2...) to identify physical disks
 *   
 *   Logical Disk (vfs.queue.length):
 *   - Format: Drive letter with colon (e.g., "C:", "D:", "E:")
 *   - Represents individual partitions/volumes
 *   - Easy to identify which specific drive/partition is monitored
 *   
 *   To see which drive letters are on which physical disk: Check Disk Management (diskmgmt.msc)
 *
 * IMPORTANT NOTES ABOUT DISK QUEUE:
 *   1. Queue Length is for the ENTIRE physical disk (not per partition)
 *   2. Single queue handles ALL I/O operations (both reads AND writes combined)
 *   3. Value represents average number of I/O requests waiting to be processed
 *   4. Good performance: < 2 per physical disk spindle (< 0.5 for SSDs)
 *   5. High values (> 2) indicate disk bottleneck
 *
 * NOTES:
 *   - Uses Windows PDH (Performance Data Helper) API
 *   - Supports dynamic disk detection (hot-plug USB drives, etc.)
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

// Counter data for each physical disk
typedef struct {
    int disk_num;
    char disk_name[64];  // e.g., "0 *" for PhysicalDisk 0
    PDH_HCOUNTER read_counter;
    PDH_HCOUNTER write_counter;
    PDH_HCOUNTER queue_counter;
} disk_counter_t;

// Counter data for each logical disk (drive letter)
typedef struct {
    char drive_letter[8];  // e.g., "C:", "D:", "E:"
    PDH_HCOUNTER queue_counter;
} logical_disk_counter_t;

#define MAX_DISKS 16
#define MAX_LOGICAL_DISKS 26  // A: through Z:
static disk_counter_t disk_counters[MAX_DISKS];
static int disk_count = 0;
static logical_disk_counter_t logical_disk_counters[MAX_LOGICAL_DISKS];
static int logical_disk_count = 0;

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
    },
    {
        "storage.queue.length",
        "Average disk queue length (per physical disk - hardware level)",
        METRIC_TYPE_FLOAT,
        "device",
        1  // has_multiple_sources
    },
    {
        "vfs.queue.length",
        "Average disk queue length (per logical disk - drive letter)",
        METRIC_TYPE_FLOAT,
        "drive",
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
        // Try to expand wildcard to get actual instance name with drive letters
        snprintf(counter_path, sizeof(counter_path), 
                 "\\PhysicalDisk(%d *)\\Disk Reads/sec", i);
        
        // Expand the wildcard path to get actual instances
        DWORD expanded_size = 0;
        status = PdhExpandWildCardPathA(NULL, counter_path, NULL, &expanded_size, 0);
        
        char actual_counter[512] = {0};
        if (expanded_size > 0 && expanded_size < sizeof(actual_counter)) {
            status = PdhExpandWildCardPathA(NULL, counter_path, actual_counter, &expanded_size, 0);
            if (status == ERROR_SUCCESS && actual_counter[0] != '\0') {
                // PdhExpandWildCardPath returns a multi-string (NULL-separated list)
                // Use only the FIRST expanded path (up to first NULL)
                strncpy(counter_path, actual_counter, sizeof(counter_path) - 1);
                counter_path[sizeof(counter_path) - 1] = '\0';
                
                // Debug: print what we got
                fprintf(stderr, "DiskStatsPlugin: Expanded counter path for disk %d: '%s'\n", i, counter_path);
            }
        }
        
        PDH_HCOUNTER read_counter;
        status = PdhAddEnglishCounterA(query, counter_path, 0, &read_counter);
        if (status != ERROR_SUCCESS) {
            // Disk doesn't exist, stop searching
            break;
        }
        
        // Build write counter path (same instance)
        char write_path[512];
        snprintf(write_path, sizeof(write_path), 
                 "\\PhysicalDisk(%d *)\\Disk Writes/sec", i);
        expanded_size = 0;
        status = PdhExpandWildCardPathA(NULL, write_path, NULL, &expanded_size, 0);
        if (expanded_size > 0 && expanded_size < sizeof(actual_counter)) {
            status = PdhExpandWildCardPathA(NULL, write_path, actual_counter, &expanded_size, 0);
            if (status == ERROR_SUCCESS && actual_counter[0] != '\0') {
                strncpy(write_path, actual_counter, sizeof(write_path) - 1);
                write_path[sizeof(write_path) - 1] = '\0';
            }
        }
        
        PDH_HCOUNTER write_counter;
        status = PdhAddEnglishCounterA(query, write_path, 0, &write_counter);
        if (status != ERROR_SUCCESS) {
            // Should not happen if read counter succeeded
            fprintf(stderr, "DiskStatsPlugin: Failed to add write counter for disk %d\n", i);
            continue;
        }
        
        // Build queue length counter path (same instance)
        char queue_path[512];
        snprintf(queue_path, sizeof(queue_path), 
                 "\\PhysicalDisk(%d *)\\Avg. Disk Queue Length", i);
        expanded_size = 0;
        status = PdhExpandWildCardPathA(NULL, queue_path, NULL, &expanded_size, 0);
        if (expanded_size > 0 && expanded_size < sizeof(actual_counter)) {
            status = PdhExpandWildCardPathA(NULL, queue_path, actual_counter, &expanded_size, 0);
            if (status == ERROR_SUCCESS && actual_counter[0] != '\0') {
                strncpy(queue_path, actual_counter, sizeof(queue_path) - 1);
                queue_path[sizeof(queue_path) - 1] = '\0';
            }
        }
        
        PDH_HCOUNTER queue_counter;
        status = PdhAddEnglishCounterA(query, queue_path, 0, &queue_counter);
        if (status != ERROR_SUCCESS) {
            // Should not happen if read counter succeeded
            fprintf(stderr, "DiskStatsPlugin: Failed to add queue counter for disk %d\n", i);
            continue;
        }
        
        // Initialize disk_name to just the number for now
        // We'll update it after first data collection when the wildcard expands
        snprintf(disk_counters[disk_count].disk_name, 
                sizeof(disk_counters[disk_count].disk_name), 
                "%d", i);
        
        disk_counters[disk_count].disk_num = i;
        disk_counters[disk_count].read_counter = read_counter;
        disk_counters[disk_count].write_counter = write_counter;
        disk_counters[disk_count].queue_counter = queue_counter;
        disk_count++;
    }
    
    if (disk_count == 0) {
        fprintf(stderr, "DiskStatsPlugin: No physical disks detected\n");
        PdhCloseQuery(query);
        return -1;
    }
    
    // Now add logical disk (drive letter) queue length counters
    // Try all drive letters A: through Z:
    const char* drive_letters[] = {"C:", "D:", "E:", "F:", "G:", "H:", "I:", "J:", "K:", "L:", "M:", 
                                     "N:", "O:", "P:", "Q:", "R:", "S:", "T:", "U:", "V:", "W:", "X:", 
                                     "Y:", "Z:", "A:", "B:"};
    
    for (int i = 0; i < sizeof(drive_letters) / sizeof(drive_letters[0]); i++) {
        char queue_path[256];
        snprintf(queue_path, sizeof(queue_path), 
                 "\\LogicalDisk(%s)\\Avg. Disk Queue Length", drive_letters[i]);
        
        PDH_HCOUNTER queue_counter;
        status = PdhAddEnglishCounterA(query, queue_path, 0, &queue_counter);
        if (status == ERROR_SUCCESS) {
            strncpy(logical_disk_counters[logical_disk_count].drive_letter, 
                    drive_letters[i], 
                    sizeof(logical_disk_counters[logical_disk_count].drive_letter) - 1);
            logical_disk_counters[logical_disk_count].queue_counter = queue_counter;
            logical_disk_count++;
            
            if (logical_disk_count >= MAX_LOGICAL_DISKS) {
                break;
            }
        }
        // If status != ERROR_SUCCESS, drive doesn't exist, just skip it
    }
    
    fprintf(stderr, "DiskStatsPlugin: Initialized %d physical disks and %d logical drives\n", 
            disk_count, logical_disk_count);
    
    // Collect initial sample (PDH needs two samples for rate calculation)
    PdhCollectQueryData(query);
    
    // Sleep briefly to allow PDH to fully initialize counters
    Sleep(100);
    
    // Now get the actual counter paths after PDH has expanded wildcards
    for (int i = 0; i < disk_count; i++) {
        DWORD path_size = 0;
        // Get counter info which should have the expanded path
        PdhGetCounterInfoA(disk_counters[i].read_counter, FALSE, &path_size, NULL);
        if (path_size > 0) {
            PDH_COUNTER_INFO_A* counter_info = (PDH_COUNTER_INFO_A*)malloc(path_size);
            if (counter_info) {
                PDH_STATUS status = PdhGetCounterInfoA(disk_counters[i].read_counter, FALSE, &path_size, counter_info);
                if (status == ERROR_SUCCESS) {
                    // szFullPath should contain the full expanded path
                    // Format: \\PhysicalDisk(0 C: D:)\\Disk Reads/sec or localized equivalent
                    if (counter_info->szFullPath) {
                        char* paren_start = strchr(counter_info->szFullPath, '(');
                        if (paren_start) {
                            paren_start++; // Move past '('
                            char* paren_end = strchr(paren_start, ')');
                            if (paren_end) {
                                size_t len = paren_end - paren_start;
                                if (len > 0 && len < sizeof(disk_counters[i].disk_name)) {
                                    // Copy instance name like "0 C: D:"
                                    strncpy(disk_counters[i].disk_name, paren_start, len);
                                    disk_counters[i].disk_name[len] = '\0';
                                }
                            }
                        }
                    }
                }
                free(counter_info);
            }
        }
    }
    
    initialized = 1;
    return 0;
}

/**
 * plugin_collect - Collect disk I/O measurements
 * 
 * Returns four collection_result_t entries (reads, writes, physical queue, logical queue).
 * Each result contains multiple measurement_value_t (one per disk/drive).
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
        
        results[2].key = "storage.queue.length";
        results[2].status = COLLECT_ERROR;
        results[2].value_count = 0;
        results[2].values = NULL;
        
        results[3].key = "vfs.queue.length";
        results[3].status = COLLECT_ERROR;
        results[3].value_count = 0;
        results[3].values = NULL;
        
        return 4;
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
        
        results[2].key = "storage.queue.length";
        results[2].status = COLLECT_ERROR;
        results[2].value_count = 0;
        results[2].values = NULL;
        
        results[3].key = "vfs.queue.length";
        results[3].status = COLLECT_ERROR;
        results[3].value_count = 0;
        results[3].values = NULL;
        
        return 4;
    }
    
    // Allocate measurement arrays for physical disks
    measurement_value_t* read_values = (measurement_value_t*)malloc(
        sizeof(measurement_value_t) * disk_count);
    measurement_value_t* write_values = (measurement_value_t*)malloc(
        sizeof(measurement_value_t) * disk_count);
    measurement_value_t* physical_queue_values = (measurement_value_t*)malloc(
        sizeof(measurement_value_t) * disk_count);
    
    // Allocate measurement arrays for logical disks
    measurement_value_t* logical_queue_values = (measurement_value_t*)malloc(
        sizeof(measurement_value_t) * logical_disk_count);
    
    // Collect values for each disk
    for (int i = 0; i < disk_count; i++) {
        PDH_FMT_COUNTERVALUE read_value, write_value, queue_value;
        
        // Get read counter value
        status = PdhGetFormattedCounterValue(
            disk_counters[i].read_counter, PDH_FMT_DOUBLE, NULL, &read_value);
        
        if (status == ERROR_SUCCESS) {
            // Allocate source_id string (caller will free) - use disk_name (e.g., "0 C: D:")
            char* source_id = (char*)malloc(64);
            snprintf(source_id, 64, "%s", disk_counters[i].disk_name);
            
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
            // Allocate source_id string (caller will free) - use disk_name (e.g., "0 C: D:")
            char* source_id = (char*)malloc(64);
            snprintf(source_id, 64, "%s", disk_counters[i].disk_name);
            
            write_values[i].source_id = source_id;
            write_values[i].value = write_value.doubleValue;
            write_values[i].str_value = NULL;
        } else {
            write_values[i].source_id = NULL;
            write_values[i].value = 0.0;
            write_values[i].str_value = NULL;
        }
        
        // Get queue length counter value
        status = PdhGetFormattedCounterValue(
            disk_counters[i].queue_counter, PDH_FMT_DOUBLE, NULL, &queue_value);
        
        if (status == ERROR_SUCCESS) {
            // Allocate source_id string (caller will free) - use disk_name (e.g., "0 *")
            char* source_id = (char*)malloc(64);
            snprintf(source_id, 64, "%s", disk_counters[i].disk_name);
            
            physical_queue_values[i].source_id = source_id;
            physical_queue_values[i].value = queue_value.doubleValue;
            physical_queue_values[i].str_value = NULL;
        } else {
            physical_queue_values[i].source_id = NULL;
            physical_queue_values[i].value = 0.0;
            physical_queue_values[i].str_value = NULL;
        }
    }
    
    // Collect values for each logical disk (drive letter)
    for (int i = 0; i < logical_disk_count; i++) {
        PDH_FMT_COUNTERVALUE queue_value;
        
        // Get queue length counter value
        status = PdhGetFormattedCounterValue(
            logical_disk_counters[i].queue_counter, PDH_FMT_DOUBLE, NULL, &queue_value);
        
        if (status == ERROR_SUCCESS) {
            // Allocate source_id string (caller will free) - use drive letter (e.g., "C:")
            char* source_id = (char*)malloc(8);
            strncpy(source_id, logical_disk_counters[i].drive_letter, 7);
            source_id[7] = '\0';
            
            logical_queue_values[i].source_id = source_id;
            logical_queue_values[i].value = queue_value.doubleValue;
            logical_queue_values[i].str_value = NULL;
        } else {
            logical_queue_values[i].source_id = NULL;
            logical_queue_values[i].value = 0.0;
            logical_queue_values[i].str_value = NULL;
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
    
    results[2].key = "storage.queue.length";
    results[2].status = COLLECT_OK;
    results[2].value_count = disk_count;
    results[2].values = physical_queue_values;
    
    results[3].key = "vfs.queue.length";
    results[3].status = COLLECT_OK;
    results[3].value_count = logical_disk_count;
    results[3].values = logical_queue_values;
    
    return 4;
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
    logical_disk_count = 0;
    return 0;
}

/* ========================================================================
 * OPTIONAL PLUGIN FUNCTIONS (not needed for this simple plugin)
 * ======================================================================== */

// plugin_on_reset - Not needed (no stateful counters)
// plugin_aggregate_custom - Not needed (default aggregation is fine)
// plugin_on_init_complete - Not needed (no cross-plugin coordination)
