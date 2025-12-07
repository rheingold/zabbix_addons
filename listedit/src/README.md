# Macro List Editor Module

Zabbix 7.4.3+ module for editing template macros ending with `_LIST}` suffix.

## Status

✅ **WORKING** - Module loads successfully, UI displays, JavaScript initializes

Module Location: `Monitoring → Macro List Editor`

## Purpose

Provides a user-friendly interface for editing list-type macros that use special formats:
1. **Pipe-separated with headers**: `{#HOST}|{#PORT},192.168.3.11|80,192.168.4.1|443`
2. **JSON array (single column)**: `["C:","D:","S:"]`

## Completed Features (v0.1.0)

- Module UI loads and displays in Zabbix menu
- JavaScript initializes with correct user context
- CSRF validation disabled for module access
- Permission checks allow authenticated users
- Main editor page renders without PHP errors
- HTML form structure with host/template selector

## Planned Features (v0.2.0+)

- AJAX host/template loading (HostList.php)
- AJAX macro loading (MacroList.php)
- AJAX macro saving (MacroUpdate.php)
- Pipe-separated format editor
- JSON array format editor
- Format conversion utilities
- Error handling and user feedback

## Installation

### 1. Deploy Module to TrueNAS

```bash
# Local machine (PowerShell)
cd C:\Users\plachy\Documents\Dev\Cpp\zabbix\listedit

# Create archive
tar -czf listedit.tar.gz -C src .

# Upload to TrueNAS
scp -i ai_priv/id_aibot listedit.tar.gz aibot@192.168.254.16:/tmp/listedit.tar.gz

# SSH to TrueNAS and extract to build directory
ssh -i ai_priv/id_aibot aibot@192.168.254.16
sudo rm -rf /root/customzabbix/usr/share/zabbix/modules/listedit/*
sudo tar -xzf /tmp/listedit.tar.gz -C /root/customzabbix/usr/share/zabbix/modules/listedit/

# Rebuild Docker image
sudo docker build -q -t zabbix-web-custom:latest /root/customzabbix/

# Restart Zabbix Web app via TrueNAS
midclt call app.stop zabbix-web
sleep 2
midclt call app.start zabbix-web
```

### 2. Verify Installation

Open browser and navigate to:
```
http://192.168.254.16:8080/zabbix.php?action=listedit.editor
```

You should see:
- HTML form with "Select Host or Template" dropdown
- JavaScript console shows: `MacroListEditor.init({userType: 3, canEdit: true})`
- Module appears in Zabbix menu under **Monitoring → Macro List Editor**

## Module Structure

```
src/
├── manifest.json           Module registration
├── Module.php              Module initialization & menu registration
├── actions/
│   ├── Editor.php          Main editor page controller
│   ├── HostList.php        AJAX endpoint - load hosts/templates (TODO)
│   ├── MacroList.php       AJAX endpoint - load macros (TODO)
│   └── MacroUpdate.php     AJAX endpoint - update macro values (TODO)
├── views/
│   ├── editor.php          Main HTML page
│   └── js/
│       └── editor.js.php   JavaScript initialization (inline PHP)
├── assets/
│   ├── js/listedit.js      Main JavaScript class (MacroListEditor)
│   └── css/listedit.css    Styling
└── README.md               This file
```

## Configuration

### Permissions

- **Access**: Any authenticated Zabbix user (USER_TYPE_ZABBIX_USER and above)
- **Edit**: Requires USER_TYPE_ZABBIX_ADMIN or higher
- **Menu**: Located in **Monitoring** (not Administration) to allow standard user access

### CSRF Protection

- CSRF validation disabled for module actions (GET requests)
- Necessary for module initialization

## Troubleshooting

### "Page not found" error
→ Module files not deployed to `/root/customzabbix/usr/share/zabbix/modules/listedit/`
→ Rebuild Docker image and restart app

### "Access denied" error
→ Check CSRF validation is disabled in Editor.php
→ Verify checkPermissions() returns true
→ Check menu location (should be under Monitoring, not Administration)

### JavaScript not initializing
→ Open browser DevTools (F12) and check Console tab for errors
→ Verify `/usr/share/zabbix/modules/listedit/views/js/editor.js.php` exists
→ Check Network tab to confirm listedit.js loads

### PHP errors
→ SSH to TrueNAS and check logs:
  ```bash
  sudo docker logs ix-zabbix-web-zabbix-web-1 | tail -50
  ```
→ Fix error in source files
→ Redeploy using process above

## Development

### Quick Deployment (After Local Changes)

```powershell
cd C:\Users\plachy\Documents\Dev\Cpp\zabbix\listedit
tar -czf listedit.tar.gz -C src .; `
scp -i ai_priv/id_aibot listedit.tar.gz aibot@192.168.254.16:/tmp/listedit.tar.gz; `
ssh -i ai_priv/id_aibot aibot@192.168.254.16 `
"sudo rm -rf /root/customzabbix/usr/share/zabbix/modules/listedit/*; \
 sudo tar -xzf /tmp/listedit.tar.gz -C /root/customzabbix/usr/share/zabbix/modules/listedit/; \
 sudo docker build -q -t zabbix-web-custom:latest /root/customzabbix/; \
 midclt call app.stop zabbix-web >/dev/null; sleep 2; \
 midclt call app.start zabbix-web >/dev/null && echo 'Deployed'"
```

### Debugging

Browser DevTools (F12):
- **Console**: Check for JavaScript errors
- **Network**: Verify listedit.js loads successfully
- **Sources**: Debug JavaScript execution

Docker Logs:
```bash
ssh -i ai_priv/id_aibot aibot@192.168.254.16 \
sudo docker logs -f ix-zabbix-web-zabbix-web-1
```

## Supported Formats
| D: | ❌ |
| S: | ❌ |
| W: | ❌ |

### Pipe-Separated Format

Format: `{#HEADER1}|{#HEADER2},value1|value2,value3|value4`

Example:
```
{#HOST}|{#PORT},192.168.3.11|80,192.168.3.11|22,192.168.4.1|444
```

Editor displays as table:

| {#HOST} | {#PORT} | Actions |
|---------|---------|---------|
| 192.168.3.11 | 80 | ❌ |
| 192.168.3.11 | 22 | ❌ |
| 192.168.4.1 | 444 | ❌ |

### JSON Array Format

Format: `["value1","value2","value3"]`

Example:
```
["C:","D:","S:","W:"]
```

Editor displays as table:

| Value | Actions |
|-------|---------|
| C: | ❌ |
| D: | ❌ |
| S: | ❌ |
| W: | ❌ |

## API Endpoints (Planned v0.2.0+)

### GET /zabbix.php?action=listedit.editor
Main editor page (displays UI)

### POST /zabbix.php?action=listedit.hostlist
Load hosts and templates for dropdown
```json
{
  "search": "",
  "limit": 50
}
```

### POST /zabbix.php?action=listedit.macrolist
Load macros from selected host/template
```json
{
  "hostid": "123456",
  "host_type": "host"
}
```

### POST /zabbix.php?action=listedit.macroupdate
Save macro value changes
```json
{
  "hostid": "123456",
  "macro_name": "{$LIST_MACRO}",
  "format": "pipe-separated",
  "data": "[...]"
}
```

## License

MIT License - See LICENSE file for details

## Author

Lukas Plachy <lukas@plachy.eu>

## Changelog

### v0.1.0 (2025-12-04)
- Initial module creation
- Module UI loads and displays in Zabbix menu
- JavaScript initialization working
- All PHP errors fixed
- Proper Docker deployment process documented
- Ready for feature development

- `listedit.hostlist` - Fetch hosts and templates
- `listedit.macrolist` - Fetch _LIST} macros for selected host/template
- `listedit.macroupdate` - Update macro value
- `listedit.editor` - Main editor page

## File Structure

```
/usr/share/zabbix/modules/listedit/
├── manifest.json          # Module metadata
├── Module.php            # Main module class (menu integration)
├── actions/
│   ├── Editor.php        # Main editor page action
│   ├── MacroList.php     # AJAX: fetch macros
│   ├── MacroUpdate.php   # AJAX: update macro value
│   └── HostList.php      # AJAX: fetch hosts/templates
├── views/
│   ├── editor.php        # Main editor HTML
│   └── editor.js.php     # Editor JavaScript initialization
└── assets/
    ├── css/
    │   └── listedit.css  # Editor styles
    └── js/
        └── listedit.js   # Main editor logic
```

## Development

See `API_PATTERNS.md` for API usage patterns and development guidelines.

## Troubleshooting

### Module not appearing in list
- Verify files are in `/usr/share/zabbix/modules/listedit/`
- Check permissions: `755` for directories, readable for files
- Click "Scan directory" button in Administration → Modules

### Menu item not visible
- Ensure module status is "Enabled" in Administration → Modules
- Refresh browser page
- Check browser console for JavaScript errors

### Cannot save macros
- Verify user has Admin or Super Admin role
- Check Zabbix frontend logs: `/var/log/zabbix/zabbix_server.log`
- Inspect browser Network tab for API errors

### Macro format not detected
- Supported formats:
  - Pipe-separated: Must contain `|` or `,` characters
  - JSON array: Must start with `[` and end with `]`
- Empty macros: Use initialization buttons to create default format

## License

MIT (unless specified otherwise in project root)

## Authors

- Lead & Architecture: lukas@plachy.eu
- Development: Claude Sonnet 4 (AI Assistant, Anthropic)
