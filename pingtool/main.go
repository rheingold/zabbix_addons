/*
 * main.go - Zabbix Agent2 Plugin for ICMP Ping with Statistics
 * Zabbix Pingtool v2.0 (Agent2 Go Plugin) | December 2, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Go-based external plugin for Zabbix Agent2 that provides on-demand ICMP ping
 *   functionality via CGO integration with Windows ICMP API. Returns JSON array
 *   format compatible with Zabbix preprocessing.
 *
 * ARCHITECTURE:
 *   Data Flow: Zabbix Agent2 → Named Pipes → Go Plugin → CGO → C Ping Functions → Windows ICMP API
 *
 *   Components:
 *   1. Go Plugin (this file): Implements Zabbix SDK plugin interface
 *   2. CGO Bridge: Marshals calls between Go and C ping functions
 *   3. C Ping Library (pingtool_lib.c): ICMP ping implementation (IPv4/IPv6)
 *   4. Windows ICMP API: IcmpSendEcho, Icmp6SendEcho2 (no admin required)
 *
 * KEY DIFFERENCE FROM AGGPLUGIN:
 *   - NO background sampling thread (on-demand execution only)
 *   - Each metric call performs ping operation synchronously
 *   - Metrics require parameters (target host, count, timeout)
 *   - Execution time: 1-10+ seconds depending on count and timeout
 *
 * DEPLOYMENT:
 *   1. Build: build.ps1 → pingtool.exe (Go executable with embedded C library)
 *   2. Install: Copy to C:\Zabbix\plugins\
 *   3. Config: zabbix_agent2.d\plugins.d\pingtool.conf:
 *      Plugins.Pingtool.System.Path=C:\Zabbix\plugins\pingtool.exe
 *   4. Restart: Agent2 service
 *
 * METRICS PROVIDED:
 *   ping.icmp[target,count,timeout] - Full ping results (array format)
 *     Parameters:
 *       target: hostname or IP address (required)
 *       count: number of pings (default: 3, max: 100)
 *       timeout: timeout per ping in ms (default: 1000)
 *     Returns: JSON array with ping results + summary
 *
 * OUTPUT FORMAT (Zabbix-Compatible Array):
 *   [
 *     {"target":"192.168.1.1","seq":1,"ms":1.0,"ok":true},
 *     {"target":"192.168.1.1","seq":2,"ms":0.0,"ok":true},
 *     {"target":"192.168.1.1","seq":3,"ms":15.0,"ok":true},
 *     {"target":"192.168.1.1","type":"summary","success_ratio":1.0,"avg_ms":5.333}
 *   ]
 *
 * CONFIGURATION FILE:
 *   Location: C:\zabbix\conf\zabbix_agent2.d\plugins.d\pingtool.conf
 *   Parameters:
 *   - Plugins.Pingtool.System.Path (required): Path to this executable
 *   - Plugins.Pingtool.DebugLevel (optional): 0-5, default 0
 *   - Plugins.Pingtool.DefaultCount (optional): Default ping count, default 3
 *   - Plugins.Pingtool.DefaultTimeout (optional): Default timeout in ms, default 1000
 *
 * ZABBIX ITEM EXAMPLES:
 *   1. Success Ratio:
 *      Key: ping.icmp[192.168.1.1,3,1000]
 *      Preprocessing: JSONPath `$[?(@.type=="summary")].success_ratio.first()`
 *
 *   2. Average Response Time:
 *      Key: ping.icmp[192.168.1.1,3,1000]
 *      Preprocessing: JSONPath `$[?(@.type=="summary")].avg_ms.first()`
 *
 *   3. Last Ping Time:
 *      Key: ping.icmp[192.168.1.1,3,1000]
 *      Preprocessing: JSONPath `$[-2].ms`
 *
 * CRITICAL SDK DEPENDENCY:
 *   MUST use golang.zabbix.com/sdk v1.2.2-0.20251007063238-42702926b56d or newer
 *   (Named pipe connection bug fix)
 *
 * BUILD DEPENDENCIES:
 *   - Go 1.21+ (with CGO enabled)
 *   - MinGW-w64 GCC (for CGO C compilation)
 *   - Windows SDK headers (winsock2.h, iphlpapi.h, icmpapi.h)
 *   - Zabbix Agent2 SDK (go.mod)
 */

package main

/*
#cgo CFLAGS: -IC:/msys64/mingw64/include -I.
#cgo LDFLAGS: -lIphlpapi -lws2_32
#include <stdlib.h>

// C ping functions from pingtool_lib.h
extern int ping_execute(const char *target, int count, int timeout_ms, int ttl, int payload_len,
                       int df, int ipv6, char *output, int output_size);
*/
import "C"

import (
	"fmt"
	"os"
	"strconv"
	"unsafe"

	"golang.zabbix.com/sdk/plugin"
	"golang.zabbix.com/sdk/plugin/container"
)

// Plugin name
const pluginName = "Pingtool"

// Debug levels
const (
	DBG_FATAL   = 0
	DBG_ERROR   = 1
	DBG_WARNING = 2
	DBG_INFO    = 3
	DBG_VERBOSE = 4
)

// Configuration variables
var cfgDebugLevel = 0
var cfgDefaultCount = 3
var cfgDefaultTimeout = 1000

// Plugin structure implements plugin.Exporter interface
type Plugin struct {
	plugin.Base
}

var impl Plugin

// Export implements plugin.Exporter interface
// Called by Zabbix Agent2 for each metric request
func (p *Plugin) Export(key string, params []string, ctx plugin.ContextProvider) (result interface{}, err error) {
	logDebug(DBG_INFO, fmt.Sprintf("Export called: key=%s params=%v", key, params))

	switch key {
	case "ping.icmp":
		return p.pingICMP(params)
	default:
		return nil, plugin.UnsupportedMetricError
	}
}

// pingICMP performs ICMP ping and returns JSON array
// Parameters: [target, count, timeout]
func (p *Plugin) pingICMP(params []string) (interface{}, error) {
	if len(params) < 1 {
		return nil, fmt.Errorf("missing required parameter: target")
	}

	target := params[0]
	count := cfgDefaultCount
	timeout := cfgDefaultTimeout

	// Parse optional parameters
	if len(params) >= 2 && params[1] != "" {
		if c, err := strconv.Atoi(params[1]); err == nil && c > 0 && c <= 100 {
			count = c
		}
	}
	if len(params) >= 3 && params[2] != "" {
		if t, err := strconv.Atoi(params[2]); err == nil && t > 0 {
			timeout = t
		}
	}

	logDebug(DBG_INFO, fmt.Sprintf("ping.icmp: target=%s count=%d timeout=%d", target, count, timeout))

	// Call C ping function via CGO
	cTarget := C.CString(target)
	defer C.free(unsafe.Pointer(cTarget))

	const bufSize = 65536
	output := make([]byte, bufSize)

	rc := C.ping_execute(
		cTarget,
		C.int(count),
		C.int(timeout),
		C.int(64), // TTL
		C.int(32), // payload length
		C.int(0),  // DF flag
		C.int(0),  // IPv6 preference
		(*C.char)(unsafe.Pointer(&output[0])),
		C.int(bufSize),
	)

	if rc != 0 {
		return nil, fmt.Errorf("ping failed: error code %d", rc)
	}

	// Convert C string to Go string
	result := C.GoString((*C.char)(unsafe.Pointer(&output[0])))

	logDebug(DBG_VERBOSE, fmt.Sprintf("ping.icmp result: %s", result))

	return result, nil
}

// logDebug logs debug messages if level is enabled
func logDebug(level int, msg string) {
	if level <= cfgDebugLevel {
		levelNames := []string{"FATAL", "ERROR", "WARNING", "INFO", "VERBOSE"}
		levelName := "UNKNOWN"
		if level >= 0 && level < len(levelNames) {
			levelName = levelNames[level]
		}
		fmt.Fprintf(os.Stderr, "[%s] %s: %s\n", pluginName, levelName, msg)
	}
}

// init registers the plugin with Zabbix Agent2
func init() {
	// Register metrics
	plugin.RegisterMetrics(&impl, pluginName,
		"ping.icmp", "ICMP ping with statistics (JSON array format). Parameters: target, count (default 3), timeout (default 1000ms)",
	)

	logDebug(DBG_INFO, "Pingtool plugin initialized")
}

// main is the entry point for the plugin executable
func main() {
	logDebug(DBG_INFO, "Pingtool plugin starting...")

	// Create plugin container handler
	h, err := container.NewHandler(pluginName)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create plugin handler: %s\n", err.Error())
		return
	}

	logDebug(DBG_INFO, "Plugin handler created, executing...")

	// Execute plugin (blocks until Agent2 terminates connection)
	err = h.Execute()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Plugin execution failed: %s\n", err.Error())
	}

	logDebug(DBG_INFO, "Plugin execution completed")
}
