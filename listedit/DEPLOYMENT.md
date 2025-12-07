# ListEdit Module - Deployment Guide

## Quick Start

The module is deployed to a TrueNAS Scale instance running Zabbix 7.4.3 in a Docker container.

### Quick Deployment Command (PowerShell)

```powershell
cd C:\Users\plachy\Documents\Dev\Cpp\zabbix\listedit
tar -czf listedit.tar.gz -C src .
scp -i ai_priv/id_aibot listedit.tar.gz aibot@192.168.254.16:/tmp/listedit.tar.gz
ssh -i ai_priv/id_aibot aibot@192.168.254.16 `
"sudo rm -rf /root/customzabbix/usr/share/zabbix/modules/listedit/*; `
 sudo tar -xzf /tmp/listedit.tar.gz -C /root/customzabbix/usr/share/zabbix/modules/listedit/; `
 sudo docker build -q -t zabbix-web-custom:latest /root/customzabbix/; `
 midclt call app.stop zabbix-web >/dev/null; sleep 2; `
 midclt call app.start zabbix-web >/dev/null && echo 'Deployed'"
```

## Deployment Environment

| Item | Value |
|------|-------|
| **Host** | truenas (192.168.254.16) |
| **SSH User** | aibot |
| **SSH Key** | ai_priv/id_aibot |
| **Docker Container** | ix-zabbix-web-zabbix-web-1 |
| **Custom Image** | zabbix-web-custom:latest |
| **Build Source** | /root/customzabbix/Dockerfile |
| **Module Path (Build)** | /root/customzabbix/usr/share/zabbix/modules/listedit/ |
| **Module Path (Container)** | /usr/share/zabbix/modules/listedit/ |
| **Zabbix Version** | 7.4.3 |
| **Zabbix Public URL** | https://zabbix.plachy.eu |
| **Zabbix Internal URL** | http://192.168.254.16:8080 |

## Step-by-Step Deployment

### 1. Prepare Source Files

```bash
cd c:\Users\plachy\Documents\Dev\Cpp\zabbix\listedit
```

Make changes to files in `src/` directory:
- `src/manifest.json` - Module registration
- `src/Module.php` - Module initialization & menu
- `src/actions/` - AJAX endpoint controllers
- `src/views/` - HTML and JavaScript views
- `src/assets/` - CSS and JavaScript assets

### 2. Create Archive

```bash
tar -czf listedit.tar.gz -C src .
```

**Note:** This includes all files in `src/` directory. Verify:
- manifest.json ✓
- Module.php ✓
- actions/ ✓
- views/ ✓
- assets/ ✓
- README.md ✓

### 3. Upload to TrueNAS

```bash
scp -i ai_priv/id_aibot listedit.tar.gz aibot@192.168.254.16:/tmp/listedit.tar.gz
```

### 4. Extract to Build Directory

```bash
ssh -i ai_priv/id_aibot aibot@192.168.254.16
sudo rm -rf /root/customzabbix/usr/share/zabbix/modules/listedit/*
sudo tar -xzf /tmp/listedit.tar.gz -C /root/customzabbix/usr/share/zabbix/modules/listedit/
```

**Verify extraction:**
```bash
ls -la /root/customzabbix/usr/share/zabbix/modules/listedit/
```

Should contain: manifest.json, Module.php, actions/, views/, assets/

### 5. Rebuild Docker Image

```bash
sudo docker build -q -t zabbix-web-custom:latest /root/customzabbix/
```

**Monitor build:**
```bash
sudo docker build -t zabbix-web-custom:latest /root/customzabbix/
```

Watch for successful layer completion.

### 6. Restart Zabbix App

```bash
midclt call app.stop zabbix-web
sleep 2
midclt call app.start zabbix-web
sleep 3
```

### 7. Verify Deployment

Open browser:
```
http://192.168.254.16:8080/zabbix.php?action=listedit.editor
```

Should display:
- "Select host or template..." dropdown
- "Load Macros" button
- Module menu appears in Zabbix UI (Monitoring → Macro List Editor)

## Troubleshooting

### "Page not found" Error

**Cause:** Module not properly deployed or manifest invalid

**Fix:**
1. Check file extraction: `ls -la /root/customzabbix/usr/share/zabbix/modules/listedit/`
2. Verify manifest.json is valid JSON: `cat /root/customzabbix/usr/share/zabbix/modules/listedit/manifest.json`
3. Check Docker logs: `sudo docker logs ix-zabbix-web-zabbix-web-1 | grep -i error | tail -10`
4. Rebuild image and restart

### PHP Errors

**View logs:**
```bash
sudo docker logs ix-zabbix-web-zabbix-web-1 | tail -50
```

**Common errors:**
- `Undefined constant` - Class constant doesn't exist in Zabbix 7.4.3
- `Cannot read file` - File path mismatch (check views/js/ subdirectory)
- `Fatal error` - Syntax error in PHP file

**Fix:**
1. Correct the error in `src/` files
2. Redeploy using steps above

### AJAX Endpoints Not Responding

**Test endpoint:**
```bash
sudo docker exec ix-zabbix-web-zabbix-web-1 \
curl -s -X POST http://localhost/zabbix.php?action=listedit.hostlist
```

**If empty response:**
- Check PHP error logs
- Verify CSRF validation is disabled in action init()
- Check checkPermissions() returns true

### Module Not Visible in Menu

**Cause:** App not restarted or module has syntax errors

**Fix:**
1. Restart app: `midclt call app.stop zabbix-web; sleep 2; midclt call app.start zabbix-web`
2. Check PHP errors in logs
3. Verify Module.php menu registration code

## Development Notes

### Adding New Endpoints

1. Create new action file: `src/actions/YourAction.php`
   - Extend CController
   - Implement: init(), checkPermissions(), checkInput(), doAction()
   - Disable CSRF: `$this->disableCsrfValidation()`

2. Register in manifest.json:
   ```json
   "actions": {
     "yourmodule.youraction": {
       "class": "YourAction",
       "layout": "json",
       "view": "null"
     }
   }
   ```

3. Call from JavaScript:
   ```javascript
   jQuery.ajax({
       url: 'zabbix.php?action=listedit.youraction',
       method: 'POST',
       data: { /* params */ },
       dataType: 'json'
   });
   ```

### File Paths in Module

**From browser perspective:**
- CSS: `modules/listedit/assets/css/listedit.css`
- JS: `modules/listedit/assets/js/listedit.js`
- Actions: `zabbix.php?action=listedit.editor`

**In Docker container:**
- Module root: `/usr/share/zabbix/modules/listedit/`
- Same file structure applies

## Rollback

If deployment fails:

```bash
ssh -i ai_priv/id_aibot aibot@192.168.254.16

# Remove failed module
sudo rm -rf /root/customzabbix/usr/share/zabbix/modules/listedit/*

# Rebuild image (will restore from git or previous version)
sudo docker build -q -t zabbix-web-custom:latest /root/customzabbix/

# Restart app
midclt call app.stop zabbix-web; sleep 2; midclt call app.start zabbix-web
```

## Performance Notes

- Module loads all hosts/templates on startup (limit: 5000)
- Macro loading is per-host (filtered AJAX call)
- Format detection is client-side (JavaScript)
- Serialization happens before AJAX POST

## Security Considerations

- ✅ CSRF validation disabled only for AJAX endpoints
- ✅ Permission checks require authenticated users
- ✅ User type determines edit capabilities (ADMIN+)
- ✅ API calls go through Zabbix authentication
- ⚠️ Input validation on server (userids filtered by API)

## Version History

| Version | Date | Status | Notes |
|---------|------|--------|-------|
| 0.1.0 | 2025-12-04 | Release | All features complete, module fully functional |
| 0.0.1 | 2025-12-02 | Initial | Project structure created |
