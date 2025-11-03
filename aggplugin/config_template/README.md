# Aggplugin Configuration Template

This directory contains the configuration file template for the Aggplugin.

## File: aggplugin.conf

**Purpose:** Configuration template for Zabbix Agent2 external plugin

### Deployment Location

The configuration file must be deployed to your Zabbix Agent2 plugin configuration directory:

**Common locations:**

**Windows:**
- `C:\Program Files\Zabbix Agent 2\conf\zabbix_agent2.d\plugins.d\aggplugin.conf`
- `C:\Zabbix\conf\zabbix_agent2.d\plugins.d\aggplugin.conf`

**Linux:**
- `/etc/zabbix/zabbix_agent2.d/plugins.d/aggplugin.conf`
- `/usr/local/etc/zabbix/zabbix_agent2.d/plugins.d/aggplugin.conf`

### Deployment Steps

1. **Copy the template:**
   ```
   Copy aggplugin.conf to [ZABBIX_HOME]/conf/zabbix_agent2.d/plugins.d/
   ```

2. **Edit the configuration:**
   - Update `Plugins.Aggplugin.System.Path` to point to your deployed plugin executable
   - Adjust other parameters as needed (DebugLevel, MaxSamples, etc.)

3. **Restart Zabbix Agent2:**
   ```
   Windows: Restart-Service "Zabbix Agent 2"
   Linux:   systemctl restart zabbix-agent2
   ```

### Required Parameters

**Plugins.Aggplugin.System.Path** (MANDATORY)
- Path to the aggplugin-agent2 executable (or aggplugin-agent2.exe on Windows)
- Must be absolute path
- Example Windows: `C:\Zabbix\plugins\aggplugin-agent2.exe`
- Example Linux: `/usr/local/zabbix/plugins/aggplugin-agent2`

### Optional Parameters

**Plugins.Aggplugin.DebugLevel** (0-5)
- Default: 0 (Fatal errors only)
- Recommended for troubleshooting: 3 (Info level)
- Logs written to: `[ZABBIX_HOME]/log/aggplugin_debug.log`

**Plugins.Aggplugin.MaxSamples** (integer)
- Default: 1000
- Maximum samples per metric before auto-reset
- At 1-second sampling: 1000 ≈ 16.7 minutes

**Plugins.Aggplugin.PreloadMetrics** (comma-separated)
- Default: "cpu_load,mem_free"
- Metrics to preload on startup (avoids cold-start nulls)

**Plugins.Aggplugin.PreloadDelay** (seconds)
- Default: 0 (immediate)
- Wait time for baseline establishment before accepting queries

### Verification

After deployment, verify the plugin is loaded:

```bash
# Test connectivity
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.test"

# Expected output:
# Aggplugin minimal test - plugin loaded successfully!

# Test metrics
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.cpu_load"
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.memory_usage"
```

### Troubleshooting

**Plugin not loading:**
1. Check Agent2 log: `[ZABBIX_HOME]/log/zabbix_agent2.log`
2. Verify executable exists at path specified in `System.Path`
3. Check file permissions (executable must be readable/executable)
4. Verify dependent DLL exists: `libaggcollector.dll` (same directory as .exe)

**Metrics returning null:**
1. Wait 2-3 seconds after service start (baseline establishment)
2. Check plugin debug log: `[ZABBIX_HOME]/log/aggplugin_debug.log`
3. Increase `DebugLevel` to 4 or 5 for detailed diagnostics

**Configuration not taking effect:**
1. Verify config file location matches Agent2's plugin config directory
2. Check config syntax (no typos in parameter names)
3. Restart Agent2 service after config changes
4. Review debug log for "Config: DebugLevel=X" messages

### Related Documentation

- **README.md**: Complete project documentation
- **USAGE.md**: Detailed usage guide with examples
- **ai.txt**: Development context and technical details

### Configuration Priority

The plugin reads configuration in this order:
1. Plugin-specific config file (this file)
2. Main Agent2 config (`zabbix_agent2.conf`)
3. Hardcoded defaults in source code

Parameters in plugin-specific config override those in main agent config.
