# pingtool (Windows)

Standalone Windows ping helper using the ICMP API (no admin rights) that prints JSON suitable for Zabbix ingestion.

## Features
- IPv4 (IcmpSendEcho) and IPv6 (Icmp6SendEcho2, when available)
- Configurable count, timeout, TTL, payload length, and DF flag (IPv4)
- Outputs detailed per-probe results and a summary

## Build
Requires MinGW-w64 gcc available on PATH.

```
# From repo root or pingtool directory
powershell -ExecutionPolicy Bypass -File pingtool/build.ps1
```

Result: `build/pingtool.exe`

## Usage
```
pingtool.exe -target <host> [-count N] [-timeout MS] [-ttl N] [-len N] [-df 0|1] [-ipv6 0|1]
```

- target: hostname or IP
- count: number of pings (default 3)
- timeout: per-ping timeout in ms (default 1000)
- ttl: IPv4 TTL / IPv6 hop limit (default 64)
- len: payload size in bytes (default 32)
- df: IPv4 Don't-Fragment flag (0/1, default 0)
- ipv6: prefer IPv6 resolution (0/1, default 0). If IPv6 fails and not forced, falls back to IPv4.

## Output JSON
Example:
```
{
  "target": "www.example.com",
  "count": 3,
  "timeout_ms": 1000,
  "ttl": 64,
  "len": 32,
  "data": [
    { "seq": 1, "ms": 32.112, "ok": true },
    { "seq": 2, "ms": null,  "ok": false, "err": "timeout" },
    { "seq": 3, "ms": 30.804, "ok": true }
  ],
  "summary": { "success_ratio": 0.6667, "avg_ms": 31.458 }
}
```

- `data` is an array of objects (compatible with Zabbix discovery-style array-of-objects)
- `success_ratio` is a decimal in range 0..1
- `avg_ms` averages only successful pings

## Notes
- Running does not require Administrator privileges on Windows (uses ICMP API). Raw sockets are not used.
- IPv6 support requires Windows Vista/Server 2008+.
- The tool pings a single target per invocation; invoke it multiple times for multiple targets.
