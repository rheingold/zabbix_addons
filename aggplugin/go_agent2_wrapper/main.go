/*
 * main.go - Zabbix Agent2 External Plugin with CGO Integration
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 3, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Go-based external plugin for Zabbix Agent2 that integrates C++ metric aggregation
 *   engine via CGO. Provides continuous background sampling of system metrics (CPU, memory)
 *   with comprehensive statistical analysis.
 *
 * ARCHITECTURE:
 *   Data Flow: Zabbix Agent2 ←→ Named Pipes ←→ Go Plugin ←→ CGO ←→ C++ Collector ←→ Windows APIs
 *
 *   Components:
 *   1. Go Plugin (this file): Implements Zabbix SDK plugin interface
 *   2. CGO Bridge: Marshals calls between Go and C++
 *   3. C++ Collector (collector.cpp): Background sampling thread with statistics
 *   4. Platform Layer (plugin_common.cpp): Windows API calls (GetSystemTimes, GlobalMemoryStatusEx)
 *
 * DEPLOYMENT:
 *   1. Build: build_cpp.ps1 -Variant agent2 → aggplugin-agent2.exe + libaggcollector.dll
 *   2. Install: Copy to C:\Zabbix\plugins\
 *   3. Config: zabbix_agent2.d\plugins.d\aggplugin.conf:
 *      Plugins.Aggplugin.System.Path=C:\Zabbix\plugins\aggplugin-agent2.exe
 *   4. Restart: Agent2 service
 *
 * FEATURES:
 *   - Continuous background sampling (1-second interval default)
 *   - 8 aggregation statistics: avg, min, max, median, mode, stddev, variance, count
 *   - Auto-reset when max samples reached (prevents unbounded memory growth)
 *   - Configurable debug logging (0-5 levels)
 *   - Preload delay for baseline establishment
 *   - JSON output compatible with Zabbix preprocessing
 *
 * METRICS PROVIDED:
 *   - Dynamically loaded from DLL plugins via Plugins.Aggplugin.PluginPath
 *   - Example plugins: cpu_load, mem_free, disk.io.read[*], disk.io.write[*], disk.queue.length[*]
 *   - Process/Service monitoring: proc.running[name|path], service.status[name|path]
 *
 * CONFIGURATION FILE:
 *   Location: C:\zabbix\conf\zabbix_agent2.d\plugins.d\aggplugin.conf
 *   Parameters:
 *   - Plugins.Aggplugin.System.Path (required): Path to this executable
 *   - Plugins.Aggplugin.DebugLevel (optional): 0-5, default 0
 *   - Plugins.Aggplugin.MaxSamples (optional): Auto-reset threshold, default 1000
 *   - Plugins.Aggplugin.PreloadMetrics (optional): Metrics to preload, default "cpu_load,mem_free"
 *   - Plugins.Aggplugin.PreloadDelay (optional): Baseline delay in seconds, default 0
 *
 * CRITICAL SDK DEPENDENCY:
 *   MUST use golang.zabbix.com/sdk v1.2.2-0.20251007063238-42702926b56d or newer
 *   Older v1.2.1 has CRITICAL BUG in named pipe connection retry (infinite 3-second timeout loop)
 *
 *   Bug: npipe.Dial retry counter never incremented → infinite retry loop
 *   Fix: Committed to master branch 2025-10-07
 *   Symptom: Plugin hangs at Execute() with 3-second timeout loop, never connects to agent
 *
 * RELATION TO OTHER CODE:
 *   - Uses (Go): golang.zabbix.com/sdk/plugin (Zabbix Agent2 SDK)
 *              : golang.zabbix.com/sdk/plugin/container (plugin lifecycle)
 *   - Uses (CGO): collector.cpp via collector_shared.cpp DLL exports
 *               : libaggcollector.dll (must be in same directory or PATH)
 *   - Called by: Zabbix Agent2 via named pipe protocol
 *
 * BUILD DEPENDENCIES:
 *   - Go 1.21+ (with CGO enabled)
 *   - MinGW-w64 GCC (for CGO C++ compilation)
 *   - libaggcollector.dll (C++ collector engine)
 *   - Zabbix Agent2 SDK (go.mod)
 */

package main

/*
 * CGO INTEGRATION DOCUMENTATION:
 *
 * The comment block below contains CGO directives (must be pure C code, no narrative).
 * CGO processes this to configure C/C++ compilation and linking.
 *
 * Configuration:
 *   #cgo CFLAGS: C compiler flags for header search paths
 *     -IC:/msys64/mingw64/include: MinGW-w64 headers (windows.h, etc.)
 *     -I../cpp_common: Project headers (collector.hpp, plugin_common.hpp)
 *
 *   #cgo LDFLAGS: Linker flags for library linking
 *     -L../build: Library search path
 *     -laggcollector: Link libaggcollector.dll (collector_shared.cpp + collector.cpp)
 *
 *   #include <stdlib.h>: Standard C library (malloc, free for C.CString cleanup)
 *
 * Collector API (from collector.hpp):
 *   collector_init(double): Initialize sampling thread
 *   collector_stop(void): Stop sampling thread
 *   collector_register_metric(name, mult): Register metric (0=success, 1=error)
 *   collector_set_max_samples(name, max): Set auto-reset threshold
 *   collector_fetch_and_reset_json(name, result, len): Fetch stats (0=success, 1=error)
 */

/*
#cgo CFLAGS: -IC:/msys64/mingw64/include -I../cpp_common
#cgo LDFLAGS: -L../build/win -laggcollector
#include <stdlib.h>
#include <windows.h>

// C++ collector functions from collector.hpp
extern void collector_init(double base_interval_seconds);
extern void collector_stop(void);
extern int collector_register_metric(const char *name, double multiplicator);
extern int collector_set_max_samples(const char *name, unsigned max_samples);
extern int collector_fetch_and_reset_json(const char *name, char *result, unsigned result_len);

// Plugin registry functions from plugin_loader.hpp
extern int register_measurement_plugin(const char* metric_key, void* dll_handle, void* collect_func, size_t key_index);
extern void unregister_all_plugins();
*/
import "C"

// STANDARD LIBRARY IMPORTS:
import (
	"bufio" // Scanner: line-by-line config file reading
	"fmt"
	"os"
	"os/signal"
	"path/filepath"
	"strconv"
	"strings"
	"syscall"
	"time"
	"unsafe"

	"golang.zabbix.com/sdk/plugin"
	"golang.zabbix.com/sdk/plugin/container"
	// Sprintf, Fprintf: string formatting, logging
	// File I/O: config reading, log writing
	// Signal handling: graceful shutdown on SIGINT/SIGTERM
	// Atoi, ParseFloat: config parameter parsing
	// TrimSpace, SplitN, HasPrefix: config file parsing
	// SIGINT, SIGTERM: signal constants
	// Sleep, Now: preload delay, timestamp formatting
	// Pointer: CGO C string conversion
)

// ZABBIX AGENT2 SDK IMPORTS:
// CRITICAL: Must use v1.2.2-0.20251007063238-42702926b56d or newer (named pipe bug fix)

// Base plugin interface: Export(), RegisterMetrics()
// Plugin lifecycle: NewHandler(), Execute()

// Plugin name
const pluginName = "Aggplugin"

// Debug level constants
const (
	DBG_FATAL    = 0 // Fatal errors only
	DBG_ERROR    = 1 // Errors
	DBG_WARNING  = 2 // Warnings
	DBG_INFO     = 3 // Debug info
	DBG_VERBOSE  = 4 // Verbose debug
	DBG_VVERBOSE = 5 // Very verbose debug
)

// Global configuration variables (loaded from config files in order: plugin config > agent config > defaults)
// Defaults: DebugLevel=0 (errors only), MaxSamples=1000, PreloadMetrics="cpu_load,mem_free", PreloadDelay=0
var cfgDebugLevel = 0                       // 0=None/Fatal, 1=Error, 2=Warning, 3=Info, 4=Verbose, 5=VeryVerbose
var cfgMaxSamples = 1000                    // Maximum samples per metric before auto-reset
var cfgPreloadMetrics = "cpu_load,mem_free" // Comma-separated list of metrics to preload on startup
var cfgPreloadDelay = 0.0                   // Seconds to wait for baseline (0 = no delay, immediate availability)
var cfgPluginPath = ""                      // Path pattern for measurement plugin DLLs (e.g., C:\Zabbix\plugins\*.dll)
var cfgDLLMetrics = make(map[string]string) // Map of [dllname.dll] → comma-separated metric keys

// ========================================================================
// MEASUREMENT PLUGIN LOADER
// ========================================================================

// Measurement plugin structures (matching measurement_plugin_api.h)
type MetricKeyInfo struct {
	Key                string
	Description        string
	Type               int32
	Parameters         string
	HasMultipleSources int32
}

type PluginInfo struct {
	Name     string
	Version  string
	Author   string
	KeyCount int
	Keys     []MetricKeyInfo
}

type MeasurementValue struct {
	SourceID *byte // C string pointer
	Value    float64
	StrValue *byte // C string pointer
}

type CollectionResult struct {
	Key        *byte // C string pointer
	Status     int32
	ValueCount uint64
	Values     *MeasurementValue
}

// Loaded plugin instance
type LoadedPlugin struct {
	DLLPath     string
	Handle      syscall.Handle
	Info        PluginInfo
	GetInfoFunc uintptr
	InitFunc    uintptr
	CollectFunc uintptr
	DeinitFunc  uintptr
}

// Global plugin registry
var loadedPlugins []LoadedPlugin

// Plugin implementation
type Plugin struct {
	plugin.Base
}

// impl is the singleton plugin implementation
var impl Plugin

// ========================================================================
// PLUGIN LOADING FUNCTIONS
// ========================================================================

// loadMeasurementPlugins scans for DLLs matching PluginPath pattern and loads them
func loadMeasurementPlugins() error {
	if cfgPluginPath == "" {
		debugLog(DBG_INFO, "No PluginPath configured, skipping measurement plugin loading")
		return nil
	}

	debugLog(DBG_INFO, fmt.Sprintf("Scanning for plugins: %s", cfgPluginPath))

	// Find all DLL files matching the pattern
	matches, err := filepath.Glob(cfgPluginPath)
	if err != nil {
		return fmt.Errorf("failed to scan plugin path: %v", err)
	}

	debugLog(DBG_INFO, fmt.Sprintf("Found %d plugin DLL(s)", len(matches)))

	for _, dllPath := range matches {
		if err := loadSinglePlugin(dllPath); err != nil {
			debugLog(DBG_ERROR, fmt.Sprintf("Failed to load plugin %s: %v", dllPath, err))
			// Continue loading other plugins
		}
	}

	debugLog(DBG_INFO, fmt.Sprintf("Loaded %d measurement plugin(s)", len(loadedPlugins)))
	return nil
}

// loadSinglePlugin loads a single measurement plugin DLL
func loadSinglePlugin(dllPath string) error {
	debugLog(DBG_VERBOSE, fmt.Sprintf("Loading plugin: %s", dllPath))

	// Load the DLL
	handle, err := syscall.LoadLibrary(string(dllPath))
	if err != nil {
		return fmt.Errorf("LoadLibrary failed: %v", err)
	}

	// Get plugin_get_info function
	getInfoProc, err := syscall.GetProcAddress(handle, "plugin_get_info")
	if err != nil {
		syscall.FreeLibrary(handle)
		return fmt.Errorf("plugin_get_info not found: %v", err)
	}

	// Call plugin_get_info to get metadata
	ret, _, _ := syscall.SyscallN(getInfoProc)
	if ret == 0 {
		syscall.FreeLibrary(handle)
		return fmt.Errorf("plugin_get_info returned NULL")
	}

	// Parse plugin info (C struct pointer)
	type CPluginInfo struct {
		Name     *byte
		Version  *byte
		Author   *byte
		KeyCount uint64
		Keys     uintptr
	}
	cInfo := (*CPluginInfo)(unsafe.Pointer(ret))

	pluginName := cStringToGo(cInfo.Name)
	pluginVersion := cStringToGo(cInfo.Version)
	pluginAuthor := cStringToGo(cInfo.Author)

	debugLog(DBG_INFO, fmt.Sprintf("Plugin: %s v%s by %s (%d keys)",
		pluginName, pluginVersion, pluginAuthor, cInfo.KeyCount))

	// Parse metric keys
	type CMetricKeyInfo struct {
		Key                *byte
		Description        *byte
		Type               int32
		Parameters         *byte
		HasMultipleSources int32
	}

	keys := make([]MetricKeyInfo, cInfo.KeyCount)
	for i := uint64(0); i < cInfo.KeyCount; i++ {
		cKey := (*CMetricKeyInfo)(unsafe.Pointer(cInfo.Keys + uintptr(i)*unsafe.Sizeof(CMetricKeyInfo{})))
		keys[i] = MetricKeyInfo{
			Key:                cStringToGo(cKey.Key),
			Description:        cStringToGo(cKey.Description),
			Type:               cKey.Type,
			Parameters:         cStringToGo(cKey.Parameters),
			HasMultipleSources: cKey.HasMultipleSources,
		}
		debugLog(DBG_VERBOSE, fmt.Sprintf("  - %s: %s", keys[i].Key, keys[i].Description))
	}

	// Get other required functions
	initProc, err := syscall.GetProcAddress(handle, "plugin_init")
	if err != nil {
		syscall.FreeLibrary(handle)
		return fmt.Errorf("plugin_init not found: %v", err)
	}

	collectProc, err := syscall.GetProcAddress(handle, "plugin_collect")
	if err != nil {
		syscall.FreeLibrary(handle)
		return fmt.Errorf("plugin_collect not found: %v", err)
	}

	deinitProc, err := syscall.GetProcAddress(handle, "plugin_deinit")
	if err != nil {
		// Optional function
		deinitProc = 0
	}

	// Call plugin_init (pass empty config for now)
	configStr := C.CString("")
	defer C.free(unsafe.Pointer(configStr))
	initRet, _, _ := syscall.SyscallN(initProc, uintptr(unsafe.Pointer(configStr)))
	if initRet != 0 {
		syscall.FreeLibrary(handle)
		return fmt.Errorf("plugin_init failed: %d", initRet)
	}

	// Prime the plugin by calling collect once (for plugins that need baseline)
	// This is needed for delta-based metrics like CPU load
	debugLog(DBG_VERBOSE, fmt.Sprintf("Priming plugin %s (initial collect call for baseline)...", pluginName))
	primeResults := make([]CollectionResult, int(cInfo.KeyCount))
	syscall.SyscallN(collectProc, uintptr(unsafe.Pointer(&primeResults[0])))
	// Ignore result - this is just to establish baseline

	// Register each plugin key with the C++ collector
	for i, keyInfo := range keys {
		keyName := C.CString(keyInfo.Key)
		defer C.free(unsafe.Pointer(keyName))

		// Register with plugin registry (this also registers with collector internally)
		regRet := C.register_measurement_plugin(
			keyName,
			unsafe.Pointer(uintptr(handle)),
			unsafe.Pointer(&collectProc),
			C.size_t(i),
		)
		if regRet != 0 {
			debugLog(DBG_ERROR, fmt.Sprintf("Failed to register plugin key '%s'", keyInfo.Key))
			continue
		}
		debugLog(DBG_INFO, fmt.Sprintf("Registered plugin key '%s' with collector", keyInfo.Key))

		// Set max samples limit
		C.collector_set_max_samples(keyName, C.uint(cfgMaxSamples))
	}

	// Store loaded plugin
	loaded := LoadedPlugin{
		DLLPath: dllPath,
		Handle:  handle,
		Info: PluginInfo{
			Name:     pluginName,
			Version:  pluginVersion,
			Author:   pluginAuthor,
			KeyCount: int(cInfo.KeyCount),
			Keys:     keys,
		},
		GetInfoFunc: getInfoProc,
		InitFunc:    initProc,
		CollectFunc: collectProc,
		DeinitFunc:  deinitProc,
	}

	loadedPlugins = append(loadedPlugins, loaded)
	debugLog(DBG_INFO, fmt.Sprintf("Successfully loaded plugin: %s", pluginName))

	return nil
}

// cStringToGo converts a C string pointer to a Go string
func cStringToGo(cstr *byte) string {
	if cstr == nil {
		return ""
	}
	return C.GoString((*C.char)(unsafe.Pointer(cstr)))
}

// unloadAllPlugins unloads all loaded measurement plugins
func unloadAllPlugins() {
	// Unregister from C++ plugin registry
	C.unregister_all_plugins()

	for _, plugin := range loadedPlugins {
		if plugin.DeinitFunc != 0 {
			syscall.SyscallN(plugin.DeinitFunc)
		}
		syscall.FreeLibrary(plugin.Handle)
		debugLog(DBG_INFO, fmt.Sprintf("Unloaded plugin: %s", plugin.Info.Name))
	}
	loadedPlugins = nil
}

// Export implements the Exporter interface for metric collection
func (p *Plugin) Export(key string, params []string, context plugin.ContextProvider) (interface{}, error) {
	defer func() {
		if r := recover(); r != nil {
			debugLog(DBG_FATAL, fmt.Sprintf("EXPORT PANIC: %v", r))
		}
	}()

	debugLog(DBG_INFO, fmt.Sprintf("EXPORT CALLED: key=%s, params=%v", key, params))

	// Handle built-in _internal metrics
	if key == "aggplugin._internal.test" {
		debugLog(DBG_VERBOSE, "Test metric - returning success")
		return "Aggplugin connectivity test - OK", nil
	}

	// Strip "aggplugin." prefix to get collector metric name
	metricName := strings.TrimPrefix(key, "aggplugin.")

	// Fetch aggregated data from collector
	debugLog(DBG_VERBOSE, fmt.Sprintf("Fetching aggregated data for metric: %s", metricName))

	cMetricName := C.CString(metricName)
	defer C.free(unsafe.Pointer(cMetricName))

	buffer := make([]byte, 4096)
	ret := C.collector_fetch_and_reset_json(cMetricName, (*C.char)(unsafe.Pointer(&buffer[0])), C.uint(len(buffer)))

	if ret != 0 {
		debugLog(DBG_ERROR, fmt.Sprintf("collector_fetch_and_reset_json failed for %s: %d", metricName, ret))
		return nil, fmt.Errorf("failed to fetch %s data: %d", metricName, ret)
	}

	result := string(buffer[:clen(buffer)])
	debugLog(DBG_INFO, fmt.Sprintf("EXPORT: Returning aggregated data for %s: %d bytes", metricName, len(result)))
	return result, nil
}

// handleInternalMetric processes built-in internal metrics (no DLL required)
// callPluginCollect calls a measurement plugin's collect function and formats the result
func callPluginCollect(plugin *LoadedPlugin, key string, params []string) (interface{}, error) {
	debugLog(DBG_VERBOSE, fmt.Sprintf("Calling plugin_collect for plugin: %s", plugin.Info.Name))

	// Allocate results array (one per key)
	results := make([]CollectionResult, plugin.Info.KeyCount)
	for i := range results {
		results[i].Key = nil
		results[i].Status = 0
		results[i].ValueCount = 0
		results[i].Values = nil
	}

	// Call plugin_collect
	ret, _, _ := syscall.SyscallN(plugin.CollectFunc, uintptr(unsafe.Pointer(&results[0])))
	if ret == 0 {
		return nil, fmt.Errorf("plugin_collect returned 0 results")
	}

	// Find the requested key in results
	for i := uint64(0); i < uint64(plugin.Info.KeyCount); i++ {
		resultKey := cStringToGo(results[i].Key)
		if resultKey == key {
			if results[i].Status != 0 { // COLLECT_OK = 0
				return nil, fmt.Errorf("collection failed with status: %d", results[i].Status)
			}

			// Format result based on value count
			if results[i].ValueCount == 0 {
				return nil, fmt.Errorf("no values returned")
			}

			if results[i].ValueCount == 1 {
				// Single value - return as float
				value := (*MeasurementValue)(unsafe.Pointer(results[i].Values))
				debugLog(DBG_INFO, fmt.Sprintf("Plugin returned single value: %.2f", value.Value))
				return value.Value, nil
			} else {
				// Multiple values - return as JSON
				jsonParts := []string{fmt.Sprintf(`{"metric":"%s","values":{`, key)}

				values := (*[1024]MeasurementValue)(unsafe.Pointer(results[i].Values))[:results[i].ValueCount:results[i].ValueCount]
				for j, val := range values {
					sourceID := cStringToGo(val.SourceID)
					if sourceID == "" {
						sourceID = "all"
					}
					if j > 0 {
						jsonParts = append(jsonParts, ",")
					}
					jsonParts = append(jsonParts, fmt.Sprintf(`"%s":%.2f`, sourceID, val.Value))
				}
				jsonParts = append(jsonParts, "}}")

				result := strings.Join(jsonParts, "")
				debugLog(DBG_INFO, fmt.Sprintf("Plugin returned %d values: %s", results[i].ValueCount, result))
				return result, nil
			}
		}
	}

	return nil, fmt.Errorf("key not found in plugin results")
}

func init() {
	// Load configuration to populate cfgDLLMetrics map
	loadConfig()
}

// collectDLLMetrics scans DLL plugins and collects their metric keys and descriptions
func collectDLLMetrics() []string {
	if cfgPluginPath == "" {
		debugLog(DBG_WARNING, "PluginPath not configured - no DLL metrics will be registered")
		return nil
	}

	matches, err := filepath.Glob(cfgPluginPath)
	if err != nil {
		debugLog(DBG_ERROR, fmt.Sprintf("Failed to glob plugin path '%s': %v", cfgPluginPath, err))
		return nil
	}

	if len(matches) == 0 {
		debugLog(DBG_WARNING, fmt.Sprintf("No DLL plugins found matching: %s", cfgPluginPath))
		return nil
	}

	var metrics []string
	debugLog(DBG_INFO, fmt.Sprintf("Scanning %d DLL plugin(s) for metrics...", len(matches)))

	for _, dllPath := range matches {
		// Temporarily load DLL to get plugin info
		handle, err := syscall.LoadLibrary(dllPath)
		if err != nil {
			debugLog(DBG_WARNING, fmt.Sprintf("Failed to load DLL '%s': %v", dllPath, err))
			continue
		}

		getInfoProc, err := syscall.GetProcAddress(handle, "plugin_get_info")
		if err != nil {
			debugLog(DBG_WARNING, fmt.Sprintf("DLL '%s' missing plugin_get_info: %v", dllPath, err))
			syscall.FreeLibrary(handle)
			continue
		}

		ret, _, _ := syscall.SyscallN(getInfoProc)
		if ret == 0 {
			debugLog(DBG_WARNING, fmt.Sprintf("DLL '%s' plugin_get_info returned NULL", dllPath))
			syscall.FreeLibrary(handle)
			continue
		}

		// Parse C struct (matching measurement_plugin_api.h CPluginInfo)
		type CPluginInfo struct {
			Name     *byte
			Version  *byte
			Author   *byte
			KeyCount uint64
			Keys     uintptr
		}
		cInfo := (*CPluginInfo)(unsafe.Pointer(ret))

		// Parse metric keys array
		type CMetricKeyInfo struct {
			Key                *byte
			Description        *byte
			Type               int32
			Parameters         *byte
			HasMultipleSources int32
		}

		if cInfo.KeyCount > 0 && cInfo.Keys != 0 {
			keysArray := unsafe.Slice((*CMetricKeyInfo)(unsafe.Pointer(cInfo.Keys)), cInfo.KeyCount)

			for i := uint64(0); i < cInfo.KeyCount; i++ {
				keyName := cStringToGo(keysArray[i].Key)
				description := cStringToGo(keysArray[i].Description)

				// Ensure description ends with period (SDK requirement)
				if description != "" && !strings.HasSuffix(description, ".") {
					description += "."
				}

				// Build full metric key with prefix
				fullKey := "aggplugin." + keyName

				metrics = append(metrics, fullKey, description)
				debugLog(DBG_VERBOSE, fmt.Sprintf("Found metric: %s - %s", fullKey, description))
			}
		}

		syscall.FreeLibrary(handle)
	}

	debugLog(DBG_INFO, fmt.Sprintf("Collected %d metric(s) from DLL plugins", len(metrics)/2))
	return metrics
}

// debugLog writes to debug log file if message level <= configured level
func debugLog(dbgLvl int, msg string) {
	if dbgLvl > cfgDebugLevel {
		return // Skip messages above configured level
	}

	levelNames := []string{"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"}
	levelName := "UNKNOWN"
	if dbgLvl >= 0 && dbgLvl < len(levelNames) {
		levelName = levelNames[dbgLvl]
	}

	// Try common Zabbix log locations
	logPaths := []string{
		"C:\\Zabbix\\log\\aggplugin_debug.log",
		"C:\\Program Files\\Zabbix Agent 2\\log\\aggplugin_debug.log",
		".\\aggplugin_debug.log", // Fallback: current directory
	}

	var f *os.File
	var err error
	for _, logPath := range logPaths {
		f, err = os.OpenFile(logPath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
		if err == nil {
			break
		}
	}

	if f != nil {
		defer f.Close()
		timestamp := time.Now().Format("2006-01-02 15:04:05.000")
		f.WriteString(fmt.Sprintf("[%s] [%s] %s\n", timestamp, levelName, msg))
	}
}

// clen returns the length of a C string (finds null terminator)
func clen(b []byte) int {
	for i, v := range b {
		if v == 0 {
			return i
		}
	}
	return len(b)
}

// loadConfig reads configuration from agent config files
// Priority: plugin config > main agent config > defaults
func loadConfig() {
	// Try common Zabbix Agent2 configuration locations
	configPaths := []string{
		"C:\\zabbix\\conf\\zabbix_agent2.d\\plugins.d\\aggplugin.conf",
		"C:\\Program Files\\Zabbix Agent 2\\zabbix_agent2.d\\plugins.d\\aggplugin.conf",
		"C:\\zabbix\\conf\\zabbix_agent2.conf",
		"C:\\Program Files\\Zabbix Agent 2\\zabbix_agent2.conf",
	}

	for _, configPath := range configPaths {
		file, err := os.Open(configPath)
		if err != nil {
			continue // Try next config file
		}

		scanner := bufio.NewScanner(file)
		currentSection := "" // Track [section] context

		for scanner.Scan() {
			line := strings.TrimSpace(scanner.Text())

			// Skip comments and empty lines
			if line == "" || strings.HasPrefix(line, "#") {
				continue
			}

			// Check for [section] headers (e.g., [cpu_load_plugin.dll])
			if strings.HasPrefix(line, "[") && strings.HasSuffix(line, "]") {
				currentSection = line[1 : len(line)-1]
				continue
			}

			// Parse key=value
			parts := strings.SplitN(line, "=", 2)
			if len(parts) != 2 {
				continue
			}

			key := strings.TrimSpace(parts[0])
			value := strings.TrimSpace(parts[1])

			// Handle MetricKeys within [dllname.dll] sections
			if currentSection != "" && key == "MetricKeys" {
				cfgDLLMetrics[currentSection] = value
				debugLog(DBG_INFO, fmt.Sprintf("Config: [%s] MetricKeys=%s (from %s)", currentSection, value, configPath))
				continue
			}

			// Match global config parameters
			switch key {
			case "Plugins.Aggplugin.DebugLevel", "DebugLevel":
				if level, err := strconv.Atoi(value); err == nil && level >= 0 && level <= 5 {
					cfgDebugLevel = level
					debugLog(DBG_INFO, fmt.Sprintf("Config: DebugLevel=%d (from %s)", level, configPath))
				}

			case "Plugins.Aggplugin.MaxSamples":
				if maxSamples, err := strconv.Atoi(value); err == nil && maxSamples > 0 {
					cfgMaxSamples = maxSamples
					debugLog(DBG_INFO, fmt.Sprintf("Config: MaxSamples=%d (from %s)", maxSamples, configPath))
				}

			case "Plugins.Aggplugin.PreloadMetrics":
				cfgPreloadMetrics = value
				debugLog(DBG_INFO, fmt.Sprintf("Config: PreloadMetrics=%s (from %s)", value, configPath))

			case "Plugins.Aggplugin.PreloadDelay":
				if delay, err := strconv.ParseFloat(value, 64); err == nil && delay >= 0 {
					cfgPreloadDelay = delay
					debugLog(DBG_INFO, fmt.Sprintf("Config: PreloadDelay=%.1f (from %s)", delay, configPath))
				}

			case "Plugins.Aggplugin.PluginPath":
				cfgPluginPath = value
				debugLog(DBG_INFO, fmt.Sprintf("Config: PluginPath=%s (from %s)", value, configPath))
			}
		}

		file.Close()
	}
}

// checkNamedPipes lists all named pipes
func checkNamedPipes() {
	pipesPath := "\\\\.\\pipe\\"
	entries, err := os.ReadDir(pipesPath)
	if err != nil {
		debugLog(DBG_ERROR, fmt.Sprintf("Cannot list named pipes: %v", err))
		return
	}

	debugLog(DBG_VVERBOSE, fmt.Sprintf("Found %d named pipes:", len(entries)))
	for i, entry := range entries {
		if i < 20 { // List first 20 to avoid spam
			debugLog(DBG_VVERBOSE, fmt.Sprintf("  Pipe %d: %s", i+1, entry.Name()))
		}
	}
}

func main() {
	debugLog(DBG_INFO, "main() started")

	// Ensure plugins are unloaded when main exits
	defer unloadAllPlugins()

	// Load configuration from config files
	loadConfig()
	debugLog(DBG_INFO, fmt.Sprintf("Configuration loaded: DebugLevel=%d, MaxSamples=%d, PreloadMetrics=%s, PreloadDelay=%.1f",
		cfgDebugLevel, cfgMaxSamples, cfgPreloadMetrics, cfgPreloadDelay))

	// Initialize C++ collector with 1-second sampling interval
	debugLog(DBG_INFO, "Initializing C++ collector (1 second interval)...")
	C.collector_init(C.double(1.0))
	debugLog(DBG_INFO, "Collector initialized")

	// Load measurement plugins from DLLs
	if err := loadMeasurementPlugins(); err != nil {
		debugLog(DBG_ERROR, fmt.Sprintf("Failed to load measurement plugins: %v", err))
	}

	// Register all metrics with collector
	debugLog(DBG_INFO, fmt.Sprintf("Registering %d plugin(s) with collector...", len(loadedPlugins)))
	for _, loadedPlugin := range loadedPlugins {
		for _, keyInfo := range loadedPlugin.Info.Keys {
			cName := C.CString(keyInfo.Key)
			ret := C.collector_register_metric(cName, C.double(1.0))
			C.free(unsafe.Pointer(cName))
			if ret != 0 {
				debugLog(DBG_ERROR, fmt.Sprintf("Failed to register metric %s with collector", keyInfo.Key))
			} else {
				debugLog(DBG_VERBOSE, fmt.Sprintf("Registered metric with collector: %s", keyInfo.Key))
			}
		}
	}

	debugLog(DBG_INFO, "Metrics registered, collector sampling started")

	// PRELOAD: Wait for baseline establishment before accepting queries
	// Windows sampling requires TWO calls: first establishes baseline, second returns data
	// Collector samples every 1 second, so wait for initial baseline + configured preload delay
	if cfgPreloadMetrics != "" && cfgPreloadDelay > 0 {
		debugLog(DBG_INFO, fmt.Sprintf("Preloading: waiting %.1fs for baseline + initial samples on: %s", cfgPreloadDelay, cfgPreloadMetrics))
		time.Sleep(time.Duration(cfgPreloadDelay * float64(time.Second)))
		debugLog(DBG_INFO, fmt.Sprintf("Preloading complete: metrics ready with ~%.0f samples", cfgPreloadDelay))
	}

	// INITIAL RESET: Clear any baseline/stale data from collector initialization
	// This ensures fresh state and that first query gets clean data from time zero
	// Only reset metrics specified in PreloadMetrics configuration (or all if empty)
	if cfgPreloadMetrics != "" {
		debugLog(DBG_VERBOSE, fmt.Sprintf("Performing initial collector reset for configured metrics: %s", cfgPreloadMetrics))
		buffer := make([]byte, 4096)
		metricsToReset := strings.Split(cfgPreloadMetrics, ",")
		for _, metricName := range metricsToReset {
			metricName = strings.TrimSpace(metricName)
			if metricName == "" {
				continue
			}
			cMetric := C.CString(metricName)
			C.collector_fetch_and_reset_json(cMetric, (*C.char)(unsafe.Pointer(&buffer[0])), C.uint(len(buffer)))
			C.free(unsafe.Pointer(cMetric))
			debugLog(DBG_VVERBOSE, fmt.Sprintf("Reset metric: %s", metricName))
		}
		debugLog(DBG_VERBOSE, "Initial collector reset complete - configured metrics zeroed")
	} else {
		debugLog(DBG_VERBOSE, "Skipping initial reset - PreloadMetrics is empty")
	}

	// Setup cleanup handler for collector_stop()
	defer func() {
		debugLog(DBG_INFO, "Stopping C++ collector...")
		C.collector_stop()
		debugLog(DBG_INFO, "Collector stopped")
	}()

	// Build complete metric list: built-in _internal metrics + dynamically scanned DLL metrics
	allMetrics := []string{
		"aggplugin._internal.test", "Connectivity test - returns constant success string.",
	}

	// Scan DLL plugins to collect their metric keys
	dllMetrics := collectDLLMetrics()
	if dllMetrics != nil && len(dllMetrics) > 0 {
		allMetrics = append(allMetrics, dllMetrics...)
	}

	// Register all metrics with Zabbix SDK in main()
	// Note: Descriptions MUST end with period, keys must be valid format
	metricCount := len(allMetrics) / 2
	debugLog(DBG_INFO, fmt.Sprintf("Registering %d metric(s) with Zabbix SDK in main()", metricCount))

	if metricCount == 0 {
		debugLog(DBG_ERROR, "No metrics to register!")
		panic("No metrics to register")
	}

	err := plugin.RegisterMetrics(&impl, pluginName, allMetrics...)

	if err != nil {
		debugLog(DBG_ERROR, fmt.Sprintf("RegisterMetrics FAILED: %v", err))
		debugLog(DBG_ERROR, "Attempted to register metrics:")
		for i := 0; i < len(allMetrics); i += 2 {
			debugLog(DBG_ERROR, fmt.Sprintf("  - %s: %s", allMetrics[i], allMetrics[i+1]))
		}
		panic(err)
	}

	debugLog(DBG_INFO, fmt.Sprintf("Successfully registered %d metric(s)", metricCount))

	// Check if plugin was registered
	p, err2 := plugin.GetByName(pluginName)
	if err2 != nil {
		debugLog(DBG_ERROR, fmt.Sprintf("GetByName FAILED: %v", err2))
	} else {
		debugLog(DBG_VERBOSE, fmt.Sprintf("GetByName OK: %v", p))
	}

	h, err := container.NewHandler(pluginName)
	if err != nil {
		debugLog(DBG_FATAL, fmt.Sprintf("NewHandler FAILED: %v", err))
		panic(err)
	}
	debugLog(DBG_INFO, "NewHandler OK")

	// Don't call Init() - it was already called during RegisterMetrics
	// TEMPORARY: Skip logger assignment due to API change in new SDK
	// impl.Logger = &h
	debugLog(DBG_WARNING, "impl.Logger NOT assigned (SDK API changed)")

	impl.SetExternal(true)
	debugLog(DBG_VERBOSE, "impl.SetExternal OK")

	// Check environment and named pipes before Execute()
	debugLog(DBG_VERBOSE, "Checking environment before Execute()...")
	debugLog(DBG_VVERBOSE, fmt.Sprintf("Command line args: %v", os.Args))
	debugLog(DBG_VVERBOSE, fmt.Sprintf("Working directory: %s", func() string { wd, _ := os.Getwd(); return wd }()))
	debugLog(DBG_VVERBOSE, fmt.Sprintf("Environment: ZABBIX_PLUGIN_SOCKET=%s", os.Getenv("ZABBIX_PLUGIN_SOCKET")))

	// List named pipes
	debugLog(DBG_VVERBOSE, "Checking named pipes:")
	checkNamedPipes()

	// Try Execute() with NEW SDK - should have fixed npipe connection retry bug
	debugLog(DBG_INFO, "Starting h.Execute() (using NEW SDK v1.2.2-master)...")

	executeDone := make(chan error, 1)
	go func() {
		debugLog(DBG_VERBOSE, "Goroutine: About to call h.Execute()...")
		debugLog(DBG_VERBOSE, "Goroutine: This will block waiting for Agent2 protocol communication...")

		// Start a monitoring goroutine to prove we're alive
		stopMonitor := make(chan bool)
		go func() {
			for i := 1; i <= 20; i++ {
				select {
				case <-stopMonitor:
					debugLog(DBG_VVERBOSE, "Monitor: Execute() returned, stopping monitor")
					return
				case <-time.After(1 * time.Second):
					debugLog(DBG_VVERBOSE, fmt.Sprintf("Monitor: Still waiting for Execute() to return... (%ds elapsed)", i))
				}
			}
		}()

		err := h.Execute()
		close(stopMonitor)

		debugLog(DBG_INFO, fmt.Sprintf("h.Execute() RETURNED with: %v", err))
		executeDone <- err
	}()

	// Give Execute() time to start
	time.Sleep(500 * time.Millisecond)
	debugLog(DBG_INFO, "Execute() goroutine started, main() is now blocking...")
	debugLog(DBG_VERBOSE, "Waiting for Execute() or interrupt signal...")

	// Setup signal handler for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	// Block here to keep the process alive while Execute goroutine runs
	select {
	case err := <-executeDone:
		if err != nil {
			debugLog(DBG_FATAL, fmt.Sprintf("Execute FAILED: %v", err))
			panic(err)
		}
		debugLog(DBG_INFO, "Execute completed successfully")
	case sig := <-sigChan:
		debugLog(DBG_INFO, fmt.Sprintf("Received signal: %v, exiting", sig))
	}

	debugLog(DBG_INFO, "Main function exiting")
}
