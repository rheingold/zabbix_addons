/*
 * collector_shared.cpp - CGO-Compatible Shared Library Export
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 3, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Creates a Windows DLL that exports the collector API for use with Go's CGO.
 *   This minimal file forces the linker to include collector.cpp functions and
 *   decorates them with __declspec(dllexport) for Windows DLL visibility.
 *
 * RELATION TO OTHER CODE:
 *   - Uses: collector.hpp (includes extern "C" function declarations)
 *         : collector.cpp (actual implementations, linked into this DLL)
 *   - Used by: main.go via CGO (#cgo LDFLAGS: -L../build -laggcollector)
 *
 * BUILD PROCESS:
 *   1. Compiler: g++ (MinGW-w64) compiles this file and collector.cpp
 *   2. Linker: Creates libaggcollector.dll with exported symbols
 *   3. Output: build/libaggcollector.dll
 *   4. CGO: Go linker links against this DLL at build time
 *   5. Runtime: Go executable loads DLL and calls exported C functions
 *
 * DLL EXPORTS:
 *   - collector_init(): Initialize sampling thread
 *   - collector_stop(): Stop sampling thread
 *   - collector_register_metric(): Register metric for sampling
 *   - collector_set_max_samples(): Set auto-reset threshold
 *   - collector_fetch_and_reset_json(): Fetch stats and reset
 *
 * CGO INTEGRATION:
 *   Go code in main.go uses these directives:
 *     #cgo LDFLAGS: -L../build -laggcollector
 *     import "C"
 *     C.collector_init(C.double(1.0))
 *
 * TECHNICAL NOTES:
 *   - DLL_EXPORT is Windows-specific (__declspec(dllexport))
 *   - On Linux, would use __attribute__((visibility("default")))
 *   - extern "C" prevents C++ name mangling for CGO compatibility
 *   - Actual implementations in collector.cpp are linked automatically
 *
 * WHY THIS FILE EXISTS:
 *   collector.cpp already has extern "C" declarations, but:
 *   1. DLL export decoration (__declspec) must be in exactly one compilation unit
 *   2. Separating export declarations from implementation is cleaner
 *   3. Allows building both static and shared libraries from same collector.cpp
 */

// DLL export macro for Windows
#define DLL_EXPORT __declspec(dllexport)

// INCLUDE DOCUMENTATION:
#include "collector.hpp"  // collector API: extern "C" function declarations
                          // Includes: collector_init(), collector_stop(),
                          //           collector_register_metric(), collector_set_max_samples(),
                          //           collector_fetch_and_reset_json()

/*
 * DLL EXPORT DECLARATIONS:
 * 
 * These forward declarations with DLL_EXPORT ensure the functions are visible
 * in the Windows DLL export table. The actual implementations are in collector.cpp
 * and are linked into this shared library by the build system.
 *
 * CALLED BY: main.go via CGO (C.collector_*)
 *
 * NOTE: collector_set_max_samples is defined in collector.hpp but not re-exported here.
 *       It's included automatically via collector.hpp and the linker will export it.
 */
extern "C" {
    DLL_EXPORT void collector_init(double base_interval_seconds);
    DLL_EXPORT void collector_stop(void);
    DLL_EXPORT int collector_register_metric(const char *name, double multiplicator);
    DLL_EXPORT int collector_set_max_samples(const char *name, unsigned max_samples);
    DLL_EXPORT int collector_set_output_format(int format);
    DLL_EXPORT int collector_fetch_and_reset_json(const char *name, char *result, unsigned result_len, const char *filter);
}

// No implementation code here - all logic is in collector.cpp
// This file just forces the linker to include collector.cpp and export symbols