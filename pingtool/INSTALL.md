# Pingtool - Zabbix Agent Plugin Installation

Standalone Windows ping utility for Zabbix with JSON array output.

## Quick Install

### 1. Copy Executable

```powershell
# Copy to Zabbix bin directory or custom location
Copy-Item pingtool.exe C:\Zabbix\bin\
```

### 2. Create Zabbix UserParameter

Add to `C:\Zabbix\conf\zabbix_agent2.d\pingtool.conf` (or `zabbix_agentd.conf` for Agent 1):

```ini
# Ping with default settings (3 pings, 1000ms timeout)
UserParameter=ping.check[*],C:\Zabbix\bin\pingtool.exe -target "$1" -count 3 -timeout 1000

# Ping with custom count
UserParameter=ping.check.count[*],C:\Zabbix\bin\pingtool.exe -target "$1" -count $2 -timeout 1000

# Ping with full parameters (target, count, timeout)
UserParameter=ping.check.full[*],C:\Zabbix\bin\pingtool.exe -target "$1" -count $2 -timeout $3

# Discovery format (LLD)
UserParameter=ping.check.lld[*],C:\Zabbix\bin\pingtool.exe -target "$1" -count $2 -timeout 1000 -output lld
```

### 3. Restart Zabbix Agent

```powershell
# For Zabbix Agent 2
Restart-Service "Zabbix Agent 2"

# For Zabbix Agent 1
Restart-Service "Zabbix Agent"
```

### 4. Test from Zabbix Server

```bash
# Test with zabbix_get
zabbix_get -s <windows-host-ip> -k "ping.check[192.168.254.1]"
```

## Output Format

Default output is Zabbix-compatible JSON array:

```json
[
  {"target":"192.168.254.1","seq":1,"ms":1.000,"ok":true},
  {"target":"192.168.254.1","seq":2,"ms":0.000,"ok":true},
  {"target":"192.168.254.1","seq":3,"ms":15.000,"ok":true},
  {"target":"192.168.254.1","type":"summary","success_ratio":1.0000,"avg_ms":5.333}
]
```

## Zabbix Item Configuration

### Create Items

1. **Success Ratio**
   - Key: `ping.check[192.168.254.1]`
   - Type: Zabbix agent (active or passive)
   - Type of information: Text
   - Preprocessing:
     - JSONPath: `$[?(@.type=="summary")].success_ratio.first()`
     - Type of information: Numeric (float)

2. **Average Response Time**
   - Key: `ping.check[192.168.254.1]`
   - Preprocessing:
     - JSONPath: `$[?(@.type=="summary")].avg_ms.first()`
     - Type of information: Numeric (float)

3. **Last Ping Time**
   - Key: `ping.check[192.168.254.1]`
   - Preprocessing:
     - JSONPath: `$[2].ms` (gets 3rd ping, or use `$[-2].ms` for second-to-last)
     - Type of information: Numeric (float)

4. **Failed Pings Count**
   - Key: `ping.check[192.168.254.1]`
   - Preprocessing:
     - JSONPath: `$[?(@.ok==false)]` → count the array length
     - JavaScript: `return value.length;`

## Parameters

- **-target** `<host>` - Hostname or IP (required)
- **-count** `N` - Number of pings (default: 3)
- **-timeout** `MS` - Timeout per ping in milliseconds (default: 1000)
- **-ttl** `N` - IP TTL / hop limit (default: 64)
- **-len** `N` - Payload size in bytes (default: 32)
- **-df** `0|1` - IPv4 Don't Fragment flag (default: 0)
- **-ipv6** `0|1` - Prefer IPv6 resolution (default: 0)
- **-output** `data|lld|full` - Output format (default: data)

## Examples

```powershell
# Basic ping
pingtool.exe -target 192.168.1.1

# 5 pings with 2-second timeout
pingtool.exe -target www.example.com -count 5 -timeout 2000

# IPv6 with large payload
pingtool.exe -target 2001:4860:4860::8888 -ipv6 1 -len 1200

# Discovery format for LLD
pingtool.exe -target 192.168.1.1 -count 3 -output lld
```

## Triggers

Example trigger expressions:

```
# Packet loss > 20%
last(/Host/ping.check[192.168.254.1],#1:now-5m)<0.8

# Average response time > 100ms
last(/Host/ping.check[192.168.254.1],#1:now-5m)>100

# Host unreachable (0% success)
last(/Host/ping.check[192.168.254.1],#1:now-1m)=0
```

## Notes

- No administrator privileges required (uses Windows ICMP API)
- IPv6 support requires Windows Vista/Server 2008 or newer
- Firewall must allow outbound ICMP
- Single target per invocation (call multiple times for multiple targets)
- No configuration file needed

## Troubleshooting

### All pings timeout
- Check Windows Firewall allows outbound ICMP
- Verify target is reachable with standard `ping` command
- Try increasing `-timeout` value

### "Unsupported item key"
- Verify UserParameter syntax in config
- Check pingtool.exe path is correct
- Restart Zabbix Agent after config changes

### IPv6 not working
- Ensure `-ipv6 1` flag is set
- Verify Windows version (Vista+ required)
- Check IPv6 connectivity with `ping -6`
