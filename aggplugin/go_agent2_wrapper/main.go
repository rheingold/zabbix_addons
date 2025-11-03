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
 *   - aggplugin.test: Connectivity test (returns "success" string)
 *   - aggplugin.cpu_load: CPU load aggregation (JSON with 8 statistics)
 *   - aggplugin.memory_usage: Memory usage aggregation (JSON with 8 statistics)
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
#cgo LDFLAGS: -L../build -laggcollector
#include <stdlib.h>

// C++ collector functions from collector.hpp
extern void collector_init(double base_interval_seconds);
extern void collector_stop(void);
extern int collector_register_metric(const char *name, double multiplicator);
extern int collector_set_max_samples(const char *name, unsigned max_samples);
extern int collector_fetch_and_reset_json(const char *name, char *result, unsigned result_len);
*/
import "C"

// STANDARD LIBRARY IMPORTS:
import (
	"bufio" // Scanner: line-by-line config file reading
	"fmt"
	"os"
	"os/signal"
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

// Plugin implementation
type Plugin struct {
	plugin.Base
}

// impl is the singleton plugin implementation
var impl Plugin

// Export implements the Exporter interface for metric collection
func (p *Plugin) Export(key string, params []string, context plugin.ContextProvider) (interface{}, error) {
	defer func() {
		if r := recover(); r != nil {
			debugLog(DBG_FATAL, fmt.Sprintf("EXPORT PANIC: %v", r))
		}
	}()

	debugLog(DBG_INFO, fmt.Sprintf("EXPORT CALLED: key=%s, params=%v", key, params))
	switch key {
	case "aggplugin.test":
		debugLog(DBG_VERBOSE, "EXPORT: Handling aggplugin.test")
		result := "Aggplugin minimal test - plugin loaded successfully!"
		debugLog(DBG_INFO, fmt.Sprintf("EXPORT: Returning: %s", result))
		return result, nil

	case "aggplugin.cpu_load":
		debugLog(DBG_VERBOSE, "EXPORT: Handling aggplugin.cpu_load")
		// Call C++ collector to fetch and reset aggregated data
		// Use internal metric name that matches collector registration
		metricName := C.CString("cpu_load")
		defer C.free(unsafe.Pointer(metricName))

		buffer := make([]byte, 4096) // Buffer for JSON result
		ret := C.collector_fetch_and_reset_json(metricName, (*C.char)(unsafe.Pointer(&buffer[0])), C.uint(len(buffer)))

		if ret != 0 {
			debugLog(DBG_ERROR, fmt.Sprintf("collector_fetch_and_reset_json failed for cpu_load: %d", ret))
			return nil, fmt.Errorf("failed to fetch cpu_load data: %d", ret)
		}

		// Find the null terminator and convert to string
		result := string(buffer[:clen(buffer)])
		debugLog(DBG_INFO, fmt.Sprintf("EXPORT: Returning CPU load aggregation: %d bytes", len(result)))
		return result, nil

	case "aggplugin.memory_usage":
		debugLog(DBG_VERBOSE, "EXPORT: Handling aggplugin.memory_usage")
		// Call C++ collector to fetch and reset aggregated data
		// Use internal metric name that matches collector registration
		metricName := C.CString("mem_free")
		defer C.free(unsafe.Pointer(metricName))

		buffer := make([]byte, 4096) // Buffer for JSON result
		ret := C.collector_fetch_and_reset_json(metricName, (*C.char)(unsafe.Pointer(&buffer[0])), C.uint(len(buffer)))

		if ret != 0 {
			debugLog(DBG_ERROR, fmt.Sprintf("collector_fetch_and_reset_json failed for mem_free: %d", ret))
			return nil, fmt.Errorf("failed to fetch mem_free data: %d", ret)
		}

		// Find the null terminator and convert to string
		result := string(buffer[:clen(buffer)])
		debugLog(DBG_INFO, fmt.Sprintf("EXPORT: Returning memory usage aggregation: %d bytes", len(result)))
		return result, nil

	default:
		debugLog(DBG_ERROR, fmt.Sprintf("EXPORT: Unknown key: %s", key))
		return nil, fmt.Errorf("unsupported item key: %s", key)
	}
}

func init() {
	// Register the plugin with Zabbix Agent2
	// Each metric returns JSON with all aggregation statistics: {"avg":X,"min":X,"max":X,"med":X,"mod":X,"dev":X,"var":X,"cnt":N}
	plugin.RegisterMetrics(&impl, pluginName,
		"aggplugin.test", "Minimal test metric - returns success message.",
		"aggplugin.cpu_load", "CPU load aggregation - returns JSON with avg/min/max/med/mod/dev/var/cnt.",
		"aggplugin.memory_usage", "Memory usage aggregation - returns JSON with avg/min/max/med/mod/dev/var/cnt (MB).",
	)
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
		for scanner.Scan() {
			line := strings.TrimSpace(scanner.Text())

			// Skip comments and empty lines
			if line == "" || strings.HasPrefix(line, "#") {
				continue
			}

			// Parse key=value
			parts := strings.SplitN(line, "=", 2)
			if len(parts) != 2 {
				continue
			}

			key := strings.TrimSpace(parts[0])
			value := strings.TrimSpace(parts[1])

			// Match our config parameters
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

	// Load configuration from config files
	loadConfig()
	debugLog(DBG_INFO, fmt.Sprintf("Configuration loaded: DebugLevel=%d, MaxSamples=%d, PreloadMetrics=%s, PreloadDelay=%.1f",
		cfgDebugLevel, cfgMaxSamples, cfgPreloadMetrics, cfgPreloadDelay))

	// Initialize C++ collector with 1-second sampling interval
	debugLog(DBG_INFO, "Initializing C++ collector (1 second interval)...")
	C.collector_init(C.double(1.0))
	debugLog(DBG_INFO, "Collector initialized")

	// Register metrics with collector
	// NOTE: Use simple names that match plugin_sample_numeric() in plugin_common.cpp
	debugLog(DBG_INFO, "Registering metrics with collector...")
	cpuName := C.CString("cpu_load")
	defer C.free(unsafe.Pointer(cpuName))
	memName := C.CString("mem_free")
	defer C.free(unsafe.Pointer(memName))

	retCPU := C.collector_register_metric(cpuName, C.double(1.0))
	if retCPU != 0 {
		debugLog(DBG_ERROR, fmt.Sprintf("Failed to register cpu_load metric: %d", retCPU))
	} else {
		debugLog(DBG_INFO, "Registered cpu_load with collector (multiplier 1.0)")
	}

	retMem := C.collector_register_metric(memName, C.double(1.0))
	if retMem != 0 {
		debugLog(DBG_ERROR, fmt.Sprintf("Failed to register mem_free metric: %d", retMem))
	} else {
		debugLog(DBG_INFO, "Registered mem_free with collector (multiplier 1.0)")
	}

	// Set max samples limit for both metrics
	debugLog(DBG_INFO, fmt.Sprintf("Setting max samples limit: %d", cfgMaxSamples))
	C.collector_set_max_samples(cpuName, C.uint(cfgMaxSamples))
	C.collector_set_max_samples(memName, C.uint(cfgMaxSamples))

	// PRELOAD: Wait for baseline establishment before accepting queries
	// Windows sampling requires TWO calls: first establishes baseline, second returns data
	// Collector samples every 1 second, so wait for initial baseline + configured preload delay
	if cfgPreloadMetrics != "" && cfgPreloadDelay > 0 {
		debugLog(DBG_INFO, fmt.Sprintf("Preloading: waiting %.1fs for baseline + initial samples on: %s", cfgPreloadDelay, cfgPreloadMetrics))
		time.Sleep(time.Duration(cfgPreloadDelay * float64(time.Second)))
		debugLog(DBG_INFO, fmt.Sprintf("Preloading complete: metrics ready with ~%.0f samples", cfgPreloadDelay))
	}

	// Setup cleanup handler for collector_stop()
	defer func() {
		debugLog(DBG_INFO, "Stopping C++ collector...")
		C.collector_stop()
		debugLog(DBG_INFO, "Collector stopped")
	}()

	// Check if plugin was registered
	p, err := plugin.GetByName(pluginName)
	if err != nil {
		debugLog(DBG_ERROR, fmt.Sprintf("GetByName FAILED: %v", err))
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
