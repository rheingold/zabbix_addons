// pingtool_lib.h - C library interface for ICMP ping functionality
// Zabbix Pingtool v2.0 (Agent2 Go Plugin) | December 2, 2025

#ifndef PINGTOOL_LIB_H
#define PINGTOOL_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Execute ICMP ping and return JSON array result
 *
 * @param target Hostname or IP address
 * @param count Number of pings (1-100)
 * @param timeout_ms Timeout per ping in milliseconds
 * @param ttl IP TTL/hop limit (1-255)
 * @param payload_len Payload size in bytes (1-1400)
 * @param df Don't Fragment flag for IPv4 (0 or 1)
 * @param ipv6 Prefer IPv6 resolution (0 or 1)
 * @param output Buffer to receive JSON result
 * @param output_size Size of output buffer
 * @return 0 on success, non-zero on error
 */
int ping_execute(const char *target, int count, int timeout_ms, int ttl, int payload_len,
                 int df, int ipv6, char *output, int output_size);

#ifdef __cplusplus
}
#endif

#endif // PINGTOOL_LIB_H
