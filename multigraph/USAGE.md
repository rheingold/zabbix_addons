# Multigraph Widget - Usage Manual

**Version:** 0.1.0  
**Date:** November 10, 2025  
**Target:** Zabbix 7.4.3+

---

## Table of Contents

**Part I: User Manual**
- [C.1.1 Overview](#c11-overview)
- [C.1.2 Operations](#c12-operations)
  - [Adding Widget to Dashboard](#adding-widget-to-dashboard)
  - [Basic Configuration](#basic-configuration)
  - [Pattern Matching](#pattern-matching)
  - [Customizing Appearance](#customizing-appearance)
  - [Using Time Selectors](#using-time-selectors)
  - [Interactive Features](#interactive-features)
- [C.1.3 Usage Examples](#c13-usage-examples)

**Part II: Administrator's Manual**
- [C.2.1 Architecture Reference](#c21-architecture-reference)
- [C.2.2 Setup and Deployment](#c22-setup-and-deployment)
  - [C.2.2.1 Key Assets and Configuration](#c221-key-assets-and-configuration)
- [C.2.3 Configuration Reference](#c23-configuration-reference)
- [C.2.4 Troubleshooting Guide](#c24-troubleshooting-guide)

---

# Part I: User Manual

## C.1.1 Overview

The **Multigraph Widget** allows you to create dynamic graphs in Zabbix dashboards using **pattern-based item selection**. Instead of manually selecting individual metrics, you specify a pattern (wildcard or regular expression) to match multiple items automatically.

### Key Benefits

- **Dynamic Graphs:** Automatically includes new items that match your pattern
- **Reduced Maintenance:** No need to update widget when new metrics are added
- **Multi-Host Support:** Match items across multiple hosts simultaneously
- **Flexible Patterns:** Use simple wildcards or powerful regular expressions

### When to Use Multigraph

**✅ Ideal Use Cases:**
- Monitoring all CPU cores: `CPU * usage`
- All network interfaces: `Interface * traffic in`
- Multiple disk partitions: `Disk * used space`
- Temperature sensors: `Temperature sensor *`
- Similar metrics across multiple hosts

**❌ Not Ideal For:**
- Single specific metric (use standard Graph widget)
- Completely unrelated metrics
- Items requiring different Y-axis scales

---

## C.1.2 Operations

### Adding Widget to Dashboard

**Step 1: Open Dashboard in Edit Mode**

1. Navigate to **Monitoring → Dashboards**
2. Select your dashboard or create a new one
3. Click **Edit dashboard** button (top right)

**Step 2: Add Multigraph Widget**

1. Click **Add** → **Add widget**
2. In the **Type** dropdown, select **Multigraph**
3. Widget configuration form appears

**Step 3: Configure Widget** (see sections below)

**Step 4: Save**

1. Click **Add** button in widget form
2. Click **Save changes** in dashboard toolbar

---

### Basic Configuration

The following fields are **required** to create a graph:

#### 1. Host Selection

**Field:** `Hosts`  
**Type:** Multi-select picker

**Steps:**
1. Click **Select** button in Hosts field
2. Choose one or more hosts from the picker
3. Selected hosts appear in the field

**Notes:**
- Widget will search for matching items on all selected hosts
- You can mix hosts from different groups
- Requires read permission on selected hosts

#### 2. Item Pattern

**Field:** `Item name pattern`  
**Type:** Text input

**Purpose:** Specify the pattern to match item names

**Examples:**
- `CPU*` - Matches all items starting with "CPU"
- `*traffic*` - Matches all items containing "traffic"
- `Memory ?` - Matches "Memory A", "Memory B", etc. (single character)

**Pattern Syntax:**
- `*` - Matches zero or more characters
- `?` - Matches exactly one character
- Case-sensitive matching

**Best Practices:**
- Start broad, then refine: Try `CPU*` first, then narrow to `CPU * usage`
- Test your pattern: Check Zabbix host items to verify matches
- Avoid overly broad patterns that match too many items

#### 3. Pattern Mode

**Field:** `Pattern mode`  
**Type:** Radio buttons  
**Options:** `Wildcard` (default) or `Regular expression`

**Wildcard Mode:**
- Simple pattern matching with `*` and `?`
- Easiest for most use cases
- Example: `CPU * usage`

**Regular Expression Mode:**
- Full PCRE regex support
- More powerful but complex
- Example: `CPU \d+ usage` (matches "CPU 0 usage", "CPU 1 usage", etc.)

**When to Use Regex:**
- Need more precise matching
- Complex patterns with alternation: `(CPU|Memory) usage`
- Character classes: `Temperature [0-9]+`
- Anchoring: `^Interface eth[0-9]+ traffic in$`

**Regex Tips:**
- Use online regex testers for validation
- Remember to escape special characters: `\.` for literal dot
- Avoid overly complex patterns (performance impact)

---

### Pattern Matching

#### Wildcard Examples

```
Pattern: CPU*
Matches: CPU usage, CPU load, CPU temperature
Does NOT match: Processor usage

Pattern: CPU * usage
Matches: CPU 0 usage, CPU 1 usage, CPU average usage
Does NOT match: CPU usage (missing middle word)

Pattern: *temperature*
Matches: CPU temperature, Disk temperature, Room temperature
Does NOT match: CPU temp (exact word "temperature" required)

Pattern: Memory ?
Matches: Memory A, Memory B, Memory 1
Does NOT match: Memory AB (? = single character only)
```

#### Regular Expression Examples

```
Pattern: CPU \d+ usage
Matches: CPU 0 usage, CPU 12 usage
Does NOT match: CPU usage, CPU a usage

Pattern: Interface eth[0-9]+ traffic (in|out)
Matches: Interface eth0 traffic in, Interface eth1 traffic out
Does NOT match: Interface wlan0 traffic in

Pattern: ^Disk /dev/sd[a-z] used$
Matches: Disk /dev/sda used, Disk /dev/sdb used
Does NOT match: Disk /dev/sda1 used ($ = end anchor)

Pattern: (CPU|Memory|Disk) .* usage
Matches: CPU 0 usage, Memory total usage, Disk sda usage
Does NOT match: Network usage
```

#### Pattern Building Strategy

1. **Start Simple:**
   ```
   CPU*
   ```

2. **Add Specificity:**
   ```
   CPU * usage
   ```

3. **Test in Zabbix:**
   - Go to **Configuration → Hosts → [Your Host] → Items**
   - Use browser search (Ctrl+F) to test pattern manually

4. **Refine Pattern:**
   ```
   CPU [0-9]+ usage
   ```

5. **Apply to Widget**

---

### Customizing Appearance

#### Graph Colors

**Field:** `Graph colors`  
**Type:** Text input (comma-separated hex colors)  
**Default:** `#1f77b4,#ff7f0e,#2ca02c,#d62728,#9467bd,#8c564b`

**Format:**
```
#1f77b4,#ff7f0e,#2ca02c
```

**Behavior:**
- Colors assigned to series in order
- If more series than colors, colors cycle automatically
- Use 6-digit hex format (`#RRGGBB`)

**Tips:**
- Use color palette generators: coolors.co, colorhunt.co
- Ensure sufficient contrast for readability
- Consider colorblind-friendly palettes

**Examples:**
```
# Traffic light colors
#00ff00,#ffff00,#ff0000

# Blue gradient
#e6f2ff,#99ccff,#3399ff,#0066cc

# Grayscale
#333333,#666666,#999999,#cccccc
```

#### Legend Configuration

**Field:** `Show legend`  
**Type:** Checkbox  
**Default:** Enabled

**Field:** `Legend position`  
**Type:** Radio buttons  
**Options:** `Top-left`, `Right`, `Bottom`

**Layout Impact:**

- **Top-left:** Legend overlays graph in upper-left corner (semi-transparent)
- **Right:** Legend beside graph, reduces graph width
- **Bottom:** Legend below graph, reduces graph height

**Best Practices:**
- Few series (1-3): Top-left or Right
- Many series (4+): Right or Bottom
- Small widgets: Hide legend, use tooltips
- Large widgets: Right position for best readability

#### Fill Configuration

**Field:** `Fill graphs`  
**Type:** Checkbox  
**Default:** Enabled

**Field:** `Fill opacity`  
**Type:** Number (0-1)  
**Default:** 0.3

**Effect:**
- **Enabled:** Area below line is filled with color
- **Disabled:** Only line is drawn (no fill)

**Opacity Guide:**
- `0.1` - Very transparent, subtle highlight
- `0.3` - Default, good balance
- `0.5` - Semi-transparent, moderate fill
- `0.8` - Mostly opaque, strong emphasis
- `1.0` - Fully opaque, solid fill

**Visual Comparison:**
```
Opacity 0.1:  Barely visible fill, line stands out
Opacity 0.3:  Subtle fill, both line and area visible
Opacity 0.7:  Strong fill, line less prominent
Opacity 1.0:  Solid fill, line may be hidden
```

**Best Practices:**
- Multiple overlapping series: Use lower opacity (0.2-0.4)
- Single series: Can use higher opacity (0.5-0.7)
- Performance: Lower opacity = less rendering work

#### Grid Configuration

**Field:** `Show grid`  
**Type:** Checkbox  
**Default:** Enabled

**Effect:**
- **Enabled:** Horizontal lines across graph
- **Disabled:** Clean graph with only axes

**Best Practices:**
- Enable for precise value reading
- Disable for cleaner, minimalist look
- Always enable for multi-series comparison

#### Y-Axis Configuration

**Field:** `Y-axis min`  
**Type:** Number (optional)  
**Default:** Auto-calculated from data

**Field:** `Y-axis max`  
**Type:** Number (optional)  
**Default:** Auto-calculated from data

**Behavior:**
- **Auto (both empty):** Range calculated from data min/max
- **Fixed:** Specify exact min and/or max values
- **Mixed:** Fix one, auto-calculate the other

**Use Cases:**

**Auto Range (Default):**
```
Best for: General monitoring, varying metrics
Example: CPU usage (0-100%), Network traffic (unpredictable)
```

**Fixed Range:**
```
Best for: Comparing across widgets, percentage metrics
Example: Y-axis min = 0, max = 100 (for percentage data)
```

**Partial Fixed:**
```
Best for: Zero-baseline with auto-scaling
Example: Y-axis min = 0, max = empty (starts at zero, scales up)
```

#### Text Color Configuration

**Field:** `Legend text color`  
**Type:** Text input (hex color)  
**Default:** `#333333` (dark gray)

**Field:** `Axes text color`  
**Type:** Text input (hex color)  
**Default:** `#666666` (medium gray)

**Purpose:** Customize text colors for theme integration

**Tips:**
- Match Zabbix dashboard theme
- Ensure readability against background
- Use darker colors for light themes, lighter for dark themes

**Theme Examples:**
```
# Light theme (default)
Legend: #333333
Axes: #666666

# Dark theme
Legend: #cccccc
Axes: #999999

# High contrast
Legend: #000000
Axes: #000000
```

---

### Using Time Selectors

The Multigraph widget integrates with **Zabbix Dashboard Time Selector**.

#### Dashboard Time Selector

**Location:** Top-right corner of dashboard

**Behavior:**
- Change timeframe in selector
- All widgets (including Multigraph) update automatically
- No widget configuration needed

**Supported Formats:**

**1. Absolute Time:**
```
2025-11-10 00:00:00  to  2025-11-10 23:59:59
```

**2. Relative Time:**
```
Last 1 hour
Last 6 hours
Last 24 hours
Last 7 days
Last 30 days
```

**3. Period Rounding:**
```
Today           (now/d)    # 00:00:00 today to now
This week       (now/w)    # Monday 00:00:00 to now
This month      (now/M)    # 1st of month 00:00:00 to now
This year       (now/y)    # Jan 1 00:00:00 to now
```

**4. Relative Expressions:**
```
now-6h          # 6 hours ago to now
now-1d          # 1 day ago to now
now-1w          # 1 week ago to now
now-1M          # 1 month ago to now
now-1y          # 1 year ago to now
```

**Examples:**

**View Today's CPU Usage:**
1. Set dashboard time to "Today" (now/d)
2. Multigraph shows data from midnight to now

**Compare Last Week vs This Week:**
1. Create two Multigraph widgets with same pattern
2. Widget 1: Time selector = "This week"
3. Widget 2: Time selector = "Last week" (now-1w to now-1w+7d)

**Monitor Last 6 Hours:**
1. Set dashboard time to "Last 6 hours" or "now-6h"
2. All widgets show last 6 hours

---

### Interactive Features

#### Hover Tooltips

**Behavior:**
- Move mouse over graph
- Tooltip appears showing:
  - Series name
  - Precise value (2 decimal places)
  - Timestamp
- Data point highlighted with circle

**Tooltip Format:**
```
┌────────────────────────┐
│ CPU 0 usage            │
│ Value: 23.45           │
│ Time: 2025-11-10 14:30 │
└────────────────────────┘
```

**Notes:**
- If multiple series at same time, shows closest by Y-value
- Interpolates between data points for smooth display
- Works on both desktop and touch devices

#### Widget Resize

**Behavior:**
- Drag widget corner to resize
- Graph automatically redraws to fit
- Legend position adjusts

**Best Practices:**
- Minimum size: 2x2 cells (approx 400x300px)
- Recommended: 3x2 or 4x3 cells for readability
- Many series: Use wider widgets (4+ cells)
- Legend position adjusts automatically

---

## C.1.3 Usage Examples

### Example 1: CPU Usage Monitoring

**Scenario:** Monitor all CPU cores on a server

**Configuration:**
```
Hosts:          myserver.example.com
Pattern:        CPU * usage
Pattern mode:   Wildcard
Legend:         Right
Fill graphs:    Yes
Fill opacity:   0.3
```

**Result:**
- Displays all CPU cores (CPU 0 usage, CPU 1 usage, etc.)
- Each core in different color
- Legend on right shows core names
- Filled areas for visual comparison

**Use Case:** Quickly identify CPU core imbalances or high usage spikes

---

### Example 2: Network Traffic Comparison

**Scenario:** Compare inbound and outbound traffic on network interfaces

**Configuration:**
```
Hosts:          router.example.com
Pattern:        Interface eth[0-9]+ traffic (in|out)
Pattern mode:   Regular expression
Graph colors:   #00ff00,#ff0000,#0000ff,#ffff00
Legend:         Bottom
Fill graphs:    No
Y-axis min:     0
```

**Result:**
- Shows traffic for eth0 in, eth0 out, eth1 in, eth1 out, etc.
- Green/red/blue/yellow colors
- Line graph (no fill) for clarity
- Y-axis starts at zero

**Use Case:** Monitor network utilization and identify bottlenecks

---

### Example 3: Memory Metrics Dashboard

**Scenario:** Display memory statistics (total, used, available, cached)

**Configuration:**
```
Hosts:          webserver1, webserver2, webserver3
Pattern:        Memory*
Pattern mode:   Wildcard
Legend:         Top-left
Fill graphs:    Yes
Fill opacity:   0.5
Show grid:      Yes
```

**Result:**
- Shows memory metrics for all three servers
- Items like "Memory total", "Memory used", "Memory available"
- Legend in top-left (semi-transparent overlay)
- Grid helps read values

**Use Case:** Compare memory usage across multiple servers

---

### Example 4: Temperature Monitoring

**Scenario:** Monitor all temperature sensors in a data center

**Configuration:**
```
Hosts:          datacenter-rack1, datacenter-rack2
Pattern:        Temperature sensor *
Pattern mode:   Wildcard
Graph colors:   #0000ff,#00ffff,#ffff00,#ff0000
Legend:         Right
Y-axis min:     15
Y-axis max:     40
```

**Result:**
- All temperature sensors displayed
- Color gradient: blue (cool) → red (hot)
- Fixed Y-axis: 15°C to 40°C for easy comparison
- Legend on right

**Use Case:** Quickly spot overheating equipment

---

### Example 5: Disk Space Monitoring

**Scenario:** Monitor used space percentage on all disk partitions

**Configuration:**
```
Hosts:          fileserver
Pattern:        Disk * used space %
Pattern mode:   Wildcard
Legend:         Bottom
Fill graphs:    Yes
Fill opacity:   0.7
Y-axis min:     0
Y-axis max:     100
Grid:           Yes
```

**Result:**
- All disk partitions shown (/, /home, /var, etc.)
- Percentage view (0-100%)
- Strong fill opacity emphasizes usage
- Grid helps identify 80% threshold

**Use Case:** Proactive disk space management

---

### Example 6: Multi-Host Performance Comparison

**Scenario:** Compare same metric across multiple web servers

**Configuration:**
```
Hosts:          web1, web2, web3, web4
Pattern:        Apache: Requests per second
Pattern mode:   Wildcard
Graph colors:   #1f77b4,#ff7f0e,#2ca02c,#d62728
Legend:         Right
Fill graphs:    No
```

**Result:**
- Single metric (Apache requests/sec) for 4 servers
- Each server different color
- Line graph (no fill) for clarity
- Easy to compare server load

**Use Case:** Load balancer validation, performance comparison

---

### Tips for Creating Effective Graphs

**1. Group Related Metrics:**
- ✅ CPU cores together
- ✅ Network interfaces together
- ❌ CPU + memory + disk in one widget (use separate widgets)

**2. Use Consistent Patterns:**
- Define naming conventions in Zabbix items
- Example: "CPU * usage", "CPU * load", "CPU * temperature"

**3. Color Coding:**
- Use color to convey meaning: green=good, yellow=warning, red=critical
- Keep colors consistent across related dashboards

**4. Optimize Widget Size:**
- 1-3 series: 2x2 or 3x2 cells
- 4-8 series: 3x3 or 4x3 cells
- 9+ series: 4x4+ cells with legend on right

**5. Time Range Selection:**
- Real-time monitoring: Last 1-6 hours
- Daily patterns: Last 24 hours
- Weekly trends: Last 7 days
- Capacity planning: Last 30 days

---

# Part II: Administrator's Manual

## C.2.1 Architecture Reference

For detailed architecture documentation, see **README.md Section B.4 (Architecture)**.

### Key Components

**Backend (PHP):**
- `Widget.php` - Entry point
- `WidgetView.php` - Display controller (data orchestration)
- `WidgetEdit.php` - Configuration controller
- `MatchedItemsData.php` - Item pattern matching
- `GraphData.php` - History data fetching
- `WidgetForm.php` - Form validation

**Frontend (JavaScript):**
- `class.widget.js` - Graph rendering (HTML5 Canvas)
- `widget.edit.js` - Form interactions

**Views (PHP Templates):**
- `widget.view.php` - Graph display template
- `widget.edit.php` - Configuration form template

### Data Flow

```
User Config → WidgetEdit → WidgetForm → Validate → Save to Dashboard

Dashboard Load → WidgetView:
    1. Parse timeframe (parseZabbixTime)
    2. Match items (MatchedItemsData::findItems)
    3. Fetch history (GraphData::fetchHistoryData)
    4. Prepare data (GraphData::prepareGraphData)
    5. Render view (widget.view.php)
    6. Client-side render (class.widget.js → Canvas)
```

---

## C.2.2 Setup and Deployment

### Prerequisites

**Server Requirements:**
- Zabbix 7.4.3 or higher
- PHP 8.4+ with extensions: gd, mbstring, bcmath, sockets
- Apache 2.4+ or Nginx 1.18+
- Linux OS (tested on Debian/Ubuntu)

**Client Requirements:**
- Modern browser: Chrome 90+, Firefox 88+, Edge 90+, Safari 14+
- JavaScript enabled
- HTML5 Canvas support

**Administrator Requirements:**
- SSH access to Zabbix server
- Root or sudo privileges
- Basic Linux command-line knowledge

### Installation Steps

#### Method 1: Manual Installation (Production)

**Step 1: Download Widget**

```bash
# Option A: Clone from GitHub
cd /tmp
git clone https://github.com/rheingold/zabbix_addons.git
cd zabbix_addons/multigraph

# Option B: Download ZIP
wget https://github.com/rheingold/zabbix_addons/archive/refs/heads/tmp0.1.zip
unzip tmp0.1.zip
cd zabbix_addons-tmp0.1/multigraph
```

**Step 2: Deploy Files**

```bash
# Create widget directory
sudo mkdir -p /usr/share/zabbix/widgets/multigraph

# Copy source files
sudo cp -r src/* /usr/share/zabbix/widgets/multigraph/

# Set permissions
sudo chmod -R 755 /usr/share/zabbix/widgets/multigraph
sudo chown -R www-data:www-data /usr/share/zabbix/widgets/multigraph
```

**Step 3: Enable Widget**

1. Open Zabbix web interface
2. Navigate to **Administration → General**
3. Select **Modules** from dropdown
4. Click **Scan directory** button
5. Find **Multigraph** in list
6. Click **Enable** or check **Enabled** checkbox
7. Click **Update** button

**Step 4: Verify Installation**

1. Go to **Monitoring → Dashboards**
2. Edit any dashboard
3. Click **Add widget**
4. In **Type** dropdown, you should see **Multigraph**

#### Method 2: Docker Deployment

**For Docker-based Zabbix installations:**

```bash
# Determine container name
docker ps | grep zabbix-web

# Copy files to container
docker cp src/. <container-name>:/usr/share/zabbix/widgets/multigraph/

# Set permissions inside container
docker exec -it <container-name> bash
chmod -R 755 /usr/share/zabbix/widgets/multigraph
chown -R www-data:www-data /usr/share/zabbix/widgets/multigraph
exit

# Restart container
docker restart <container-name>
```

**Then follow Step 3 from Method 1 (Enable Widget).**

#### Method 3: Automated PowerShell Deployment (Development)

**For Windows developers with SSH access:**

**Complete PowerShell deployment script:**

```powershell
# deploy-multigraph.ps1 - Automated deployment for development

# Configuration - adjust these values for your environment
$dockerHost = "192.168.254.16"
$sshUser = "aibot"
$sshKeyPath = ".\ai_priv\id_aibot"
$buildDir = "/root/customzabbix"
$widgetDir = "$buildDir/usr/share/zabbix/widgets/multigraph"
$imageName = "zabbix-web-custom"
$imageTag = "latest"
$containerName = "zabbix-web"
$composeFile = "/root/docker-compose.yml"

Write-Host "=== Multigraph Widget Deployment ===" -ForegroundColor Green
Write-Host "Target: $sshUser@$dockerHost"
Write-Host "Image: $imageName`:$imageTag`n"

# Step 1: Copy files to Docker host
Write-Host "Step 1: Copying widget files..." -ForegroundColor Yellow
ssh -i $sshKeyPath "$sshUser@$dockerHost" "mkdir -p $widgetDir"
scp -i $sshKeyPath -r .\src\* "$sshUser@$dockerHost`:$widgetDir/"

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Files copied" -ForegroundColor Green
} else {
    Write-Host "✗ Copy failed" -ForegroundColor Red
    exit 1
}

# Step 2: Build Docker image
Write-Host "`nStep 2: Building Docker image..." -ForegroundColor Yellow
$buildCmd = "cd $buildDir && sudo docker build -t $imageName`:$imageTag ."
ssh -i $sshKeyPath "$sshUser@$dockerHost" $buildCmd

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Image built" -ForegroundColor Green
} else {
    Write-Host "✗ Build failed" -ForegroundColor Red
    exit 1
}

# Step 3: Restart container
Write-Host "`nStep 3: Restarting container..." -ForegroundColor Yellow
$restartCmd = "sudo docker-compose -f $composeFile restart $containerName"
ssh -i $sshKeyPath "$sshUser@$dockerHost" $restartCmd

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Container restarted" -ForegroundColor Green
} else {
    Write-Host "✗ Restart failed" -ForegroundColor Red
    exit 1
}

# Step 4: Verify deployment
Write-Host "`nStep 4: Verifying..." -ForegroundColor Yellow
$verifyCmd = "sudo docker exec $containerName ls -la /usr/share/zabbix/widgets/multigraph/"
ssh -i $sshKeyPath "$sshUser@$dockerHost" $verifyCmd

Write-Host "`n=== Deployment Complete ===" -ForegroundColor Green
Write-Host "Next: Enable widget in Zabbix UI (Administration → Modules)"
```

**Usage:**

```powershell
# Save script as deploy-multigraph.ps1
# Edit configuration section with your values
# Run deployment
.\deploy-multigraph.ps1
```

### Updating Widget

**To update to a new version:**

```bash
# Backup current version
sudo cp -r /usr/share/zabbix/widgets/multigraph /usr/share/zabbix/widgets/multigraph.backup

# Download new version
cd /tmp
git clone https://github.com/rheingold/zabbix_addons.git
cd zabbix_addons/multigraph

# Replace files
sudo cp -r src/* /usr/share/zabbix/widgets/multigraph/

# Set permissions
sudo chmod -R 755 /usr/share/zabbix/widgets/multigraph
sudo chown -R www-data:www-data /usr/share/zabbix/widgets/multigraph

# Rescan modules in Zabbix UI
# Administration → General → Modules → Scan directory
```

### Uninstalling Widget

```bash
# Remove widget directory
sudo rm -rf /usr/share/zabbix/widgets/multigraph

# Rescan modules in Zabbix UI
# Administration → General → Modules → Scan directory

# Multigraph will disappear from list
```

**Note:** Existing widgets on dashboards will show error message after uninstall.

---

## C.2.2.1 Key Assets and Configuration

### File Locations

**Widget Root:**
```
/usr/share/zabbix/widgets/multigraph/
```

**Key Files:**
```
manifest.json               # Widget metadata and registration
Widget.php                  # Entry point
actions/WidgetView.php      # Display logic
actions/WidgetEdit.php      # Configuration logic
includes/WidgetForm.php     # Form definition
includes/MatchedItemsData.php  # Pattern matching
includes/GraphData.php      # Data fetching
views/widget.view.php       # Display template
views/widget.edit.php       # Config template
assets/js/class.widget.js   # Main JavaScript
assets/js/widget.edit.js    # Config JavaScript
assets/css/multigraph.css   # Styles
```

### Configuration Files

#### manifest.json

**Location:** `/usr/share/zabbix/widgets/multigraph/manifest.json`

**Purpose:** Widget registration, metadata, asset declarations

**Key Fields:**
- `name` - Widget name displayed in UI
- `namespace` - PHP namespace for autoloading
- `version` - Widget version
- `js_files` - JavaScript files to load
- `css_files` - CSS files to load

**Example:**
```json
{
  "name": "Multigraph",
  "namespace": "Widgets\\Multigraph",
  "version": "0.1.0",
  "js_files": [
    "assets/js/class.widget.js",
    "assets/js/widget.edit.js"
  ],
  "css_files": [
    "assets/css/multigraph.css"
  ]
}
```

**Modifications:**
- Update `version` when deploying new versions
- Add files to `js_files`/`css_files` if extending widget
- Do NOT change `namespace` (breaks autoloading)

#### WidgetForm.php Configuration Constants

**Location:** `/usr/share/zabbix/widgets/multigraph/includes/WidgetForm.php`

**Configurable Limits:**

```php
// Line 15-20 (approximate)
const MAX_ITEMS_MATCHED = 100;  // Maximum items to match per query
```

**To Modify:**
1. Edit `WidgetForm.php`
2. Change constant value
3. No restart needed (PHP interpreted)
4. Test with large pattern matches

**Warning:** Increasing limits may impact performance.

#### MatchedItemsData.php Pattern Limits

**Location:** `/usr/share/zabbix/widgets/multigraph/includes/MatchedItemsData.php`

**Configurable:**

```php
// Line 30 (approximate)
'limit' => 100,  // Maximum items returned by API
```

**To Modify:**
1. Edit `MatchedItemsData.php`
2. Change `limit` value in `API::Item()::get()` call
3. Must match `WidgetForm::MAX_ITEMS_MATCHED`

### Logs

**Zabbix Frontend Logs:**
```
/var/log/apache2/error.log        # Apache
/var/log/nginx/error.log          # Nginx
/var/log/php8.4-fpm.log           # PHP-FPM
```

**Widget Errors:**
- PHP errors appear in web server error log
- JavaScript errors appear in browser console (F12)
- Check logs for pattern matching issues, API errors

**Enable Debug Logging:**

Edit `/etc/zabbix/web/zabbix.conf.php`:

```php
$DB['DB_DEBUG'] = 1;  // Enable database query logging
```

**View Logs:**
```bash
sudo tail -f /var/log/apache2/error.log
```

#### Docker Container Logging

**For Docker/containerized Zabbix installations:**

**1. Check Container Status:**
```bash
# List all Zabbix-related containers
docker ps -a | grep zabbix

# Check specific container status
docker ps -a --filter "name=zabbix-web"

# Example output:
# CONTAINER ID   IMAGE                    STATUS         NAMES
# a1b2c3d4e5f6   zabbix-web-custom:latest Up 2 hours     zabbix-web
```

**2. View Container Logs:**

```bash
# View all logs (stdout/stderr)
docker logs zabbix-web

# Follow logs in real-time (like tail -f)
docker logs -f zabbix-web

# View last 100 lines
docker logs --tail 100 zabbix-web

# View logs since specific time
docker logs --since 30m zabbix-web           # Last 30 minutes
docker logs --since 2025-11-10T14:00:00 zabbix-web  # Since specific time

# View logs with timestamps
docker logs -t zabbix-web

# Search for specific errors
docker logs zabbix-web 2>&1 | grep -i "multigraph"
docker logs zabbix-web 2>&1 | grep -i "error"
docker logs zabbix-web 2>&1 | grep -i "warning"
```

**3. Detect Log Redirection:**

Many Docker images redirect logs to stdout/stderr for container log visibility.

**Check if logs are redirected:**

```bash
# Enter container
docker exec -it zabbix-web bash

# Check if log files are symlinks to /dev/stdout or /proc/self/fd
ls -la /var/log/apache2/error.log
ls -la /var/log/nginx/error.log
ls -la /var/log/php8.4-fpm.log

# Example output if redirected:
# lrwxrwxrwx 1 root root 11 Nov 10 10:00 /var/log/apache2/error.log -> /dev/stderr
# lrwxrwxrwx 1 root root 11 Nov 10 10:00 /var/log/apache2/access.log -> /dev/stdout

# If symlinks exist, logs are captured by docker logs command
```

**4. Access Logs Inside Container:**

If logs are NOT redirected (traditional files):

```bash
# Enter container
docker exec -it zabbix-web bash

# View logs inside container
tail -f /var/log/apache2/error.log
tail -f /var/log/nginx/error.log
tail -f /var/log/php8.4-fpm.log

# Search logs
grep "multigraph" /var/log/apache2/error.log
grep "Fatal error" /var/log/apache2/error.log

# Exit container
exit
```

**5. Copy Logs from Container:**

```bash
# Copy log file from container to host
docker cp zabbix-web:/var/log/apache2/error.log ./error.log

# Analyze on host
grep "multigraph" error.log
less error.log
```

**6. Enable PHP Error Logging in Container:**

```bash
# Enter container
docker exec -it zabbix-web bash

# Edit PHP configuration
vi /etc/zabbix/web/zabbix.conf.php

# Add/modify:
$DB['DB_DEBUG'] = 1;  // Enable database query logging

# Or edit PHP-FPM configuration
vi /etc/php/8.4/fpm/php.ini

# Set:
error_reporting = E_ALL
display_errors = On
log_errors = On
error_log = /var/log/php8.4-fpm.log

# Restart PHP-FPM inside container
service php8.4-fpm restart

# Or restart entire container from host
docker restart zabbix-web
```

**7. Docker Compose Logging:**

If using docker-compose:

```bash
# View logs for all services
docker-compose logs

# View logs for specific service
docker-compose logs zabbix-web

# Follow logs in real-time
docker-compose logs -f zabbix-web

# View last 50 lines
docker-compose logs --tail 50 zabbix-web
```

**8. Persistent Log Storage (Docker Volumes):**

Check if logs are stored in Docker volumes:

```bash
# List volumes
docker volume ls | grep zabbix

# Inspect volume
docker volume inspect zabbix_logs

# Example output shows Mountpoint:
# "Mountpoint": "/var/lib/docker/volumes/zabbix_logs/_data"

# Access volume data (requires root)
sudo ls -la /var/lib/docker/volumes/zabbix_logs/_data/
sudo tail -f /var/lib/docker/volumes/zabbix_logs/_data/error.log
```

**9. Common Docker Container Issues:**

**Issue: "docker logs" shows nothing**
- **Cause:** Logs written to files, not stdout/stderr
- **Solution:** Use `docker exec` to access logs inside container

**Issue: Permission denied accessing logs**
- **Cause:** Log files owned by root or www-data
- **Solution:** Use `sudo` or `docker exec -u root`

**Issue: Logs not updating**
- **Cause:** Log rotation or container restart
- **Solution:** Check container uptime with `docker ps`, verify log file with `ls -la`

**10. Complete Docker Debugging Example:**

```bash
# Step 1: Identify container
docker ps | grep zabbix-web
# Result: zabbix-web container ID = a1b2c3d4e5f6

# Step 2: Check recent logs
docker logs --tail 50 --timestamps zabbix-web

# Step 3: Search for widget errors
docker logs zabbix-web 2>&1 | grep -i "multigraph\|fatal\|error"

# Step 4: If no logs, check inside container
docker exec -it zabbix-web bash
ls -la /var/log/apache2/error.log
# If symlink: exit, use docker logs
# If file: tail -f /var/log/apache2/error.log

# Step 5: Check PHP errors
docker exec -it zabbix-web tail -f /var/log/php8.4-fpm.log

# Step 6: Check Zabbix debug log
docker exec -it zabbix-web tail -f /var/log/zabbix/zabbix_server.log

# Step 7: Monitor logs in real-time during widget test
docker logs -f zabbix-web &
# Now test widget in browser, watch logs scroll
# Ctrl+C to stop
```

**11. Log Aggregation Tips:**

For production environments with multiple containers:

```bash
# Aggregate logs from all Zabbix containers
docker-compose logs -f | tee zabbix-all-logs.txt

# Filter aggregated logs
docker-compose logs | grep "multigraph" | tee multigraph-logs.txt

# Monitor specific container while testing
watch -n 1 'docker logs --tail 20 zabbix-web'
```

### Resources and Performance

**Memory Usage:**
- PHP: ~10-20 MB per widget render (typical)
- JavaScript: ~5-10 MB per widget instance
- Large datasets (1000+ points): up to 50 MB

**CPU Usage:**
- Graph rendering: Client-side (browser)
- Pattern matching: Server-side (Zabbix API)
- Typical render time: <200ms

**Network:**
- Initial load: ~50-100 KB (JS + CSS)
- Data fetch: Varies by timeframe and items (10 KB - 5 MB)
- Refresh rate: Follows dashboard refresh interval

**Database Impact:**
- Pattern matching: 1 API::Item() call per widget load
- Data fetching: 1 API::History() call per widget load
- Cached by Zabbix API layer

---

## C.2.3 Configuration Reference

### Widget Configuration Fields

#### Required Fields

| Field | Type | Description | Validation |
|-------|------|-------------|------------|
| `hostids` | Array | Selected host IDs | At least 1 host required |
| `item_pattern` | String | Item name pattern | Non-empty string |
| `pattern_mode` | Integer | 0=Wildcard, 1=Regex | Must be 0 or 1 |

#### Optional Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `show_legend` | Boolean | `true` | Display legend |
| `legend_position` | String | `top-left` | `top-left`, `right`, `bottom` |
| `graph_colors` | String | `#1f77b4,...` | Comma-separated hex colors |
| `fill_graphs` | Boolean | `true` | Enable area fill |
| `fill_opacity` | Float | `0.3` | Fill opacity (0.0-1.0) |
| `show_grid` | Boolean | `true` | Display grid lines |
| `yaxis_min` | Float | Auto | Y-axis minimum value |
| `yaxis_max` | Float | Auto | Y-axis maximum value |
| `legend_color` | String | `#333333` | Legend text color |
| `axes_color` | String | `#666666` | Axes text color |

### Pattern Syntax Reference

#### Wildcard Mode (pattern_mode = 0)

| Symbol | Meaning | Example | Matches |
|--------|---------|---------|---------|
| `*` | Zero or more chars | `CPU*` | CPU, CPUs, CPU usage |
| `?` | Exactly one char | `CPU ?` | CPU 0, CPU A |
| `*...*` | Contains | `*traffic*` | Network traffic in |

**Rules:**
- Case-sensitive
- `*` can match empty string
- `?` must match exactly one character

#### Regex Mode (pattern_mode = 1)

**Full PCRE support:**

| Pattern | Description | Example |
|---------|-------------|---------|
| `\d+` | One or more digits | `CPU \d+ usage` |
| `[a-z]` | Character class | `Disk /dev/sd[a-z]` |
| `(a\|b)` | Alternation | `Interface (eth\|wlan)[0-9]+` |
| `^...$` | Anchors | `^CPU 0$` (exact match) |
| `.*` | Any characters | `CPU .* usage` |
| `{n,m}` | Quantifier | `Temperature [0-9]{1,2}` |

**Escaping:**
- Literal dot: `\.`
- Literal star: `\*`
- Literal bracket: `\[`

### API Limits and Constraints

**Hard Limits:**

```php
MAX_ITEMS_MATCHED = 100          // Max items per pattern
API_TIMEOUT = 30                 // API call timeout (seconds)
MAX_HISTORY_POINTS = 100000      // Max data points returned
```

**Soft Limits (Performance):**

```
Recommended max items: 20
Recommended max data points: 1000 per series
Recommended timeframe: < 30 days
```

**Exceeding Limits:**
- 100+ items: Pattern too broad, widget shows error
- Large datasets: Slow rendering, consider shorter timeframe
- Long timeframes: Zabbix returns averaged trends, not raw history

---

## C.2.4 Troubleshooting Guide

### Common Issues and Solutions

#### Issue: Widget Shows "No data"

**Symptoms:**
- Widget displays "No matching items" or "No data available"
- Graph area is blank

**Possible Causes:**

**1. Pattern Doesn't Match Any Items**

**Diagnosis:**
- Check item names in Zabbix: **Configuration → Hosts → [Host] → Items**
- Compare pattern to actual item names
- Check case sensitivity

**Solution:**
- Adjust pattern to match existing items
- Example: Change `CPU*usage` to `CPU * usage` (with spaces)

**2. No Data in Timeframe**

**Diagnosis:**
- Items exist but have no history data
- Timeframe predates item monitoring

**Solution:**
- Check item history: **Monitoring → Latest data**
- Select shorter, more recent timeframe
- Verify items are enabled and monitored

**3. Regex Syntax Error**

**Diagnosis:**
- Pattern mode set to "Regular expression"
- Invalid regex pattern

**Solution:**
- Validate regex using online tester (regex101.com)
- Check for unescaped special characters
- Example: `CPU \d+ usage` not `CPU d+ usage`

**4. Host Permission Issue**

**Diagnosis:**
- User doesn't have read permission on selected hosts

**Solution:**
- Check user group permissions: **Administration → User groups**
- Assign read permission to host groups
- Verify host is in accessible group

---

#### Issue: Graph Not Displaying Correctly

**Symptoms:**
- Graph appears but looks wrong
- Colors incorrect, legend missing, axes off

**Possible Causes:**

**1. JavaScript Error**

**Diagnosis:**
- Open browser console (F12 → Console)
- Look for red error messages

**Solution:**
- Clear browser cache (Ctrl+Shift+Delete)
- Disable browser extensions
- Try different browser
- Check for JavaScript console errors and report as bug

**2. Canvas Not Supported**

**Diagnosis:**
- Very old browser (IE11 or earlier)
- Canvas API disabled

**Solution:**
- Update browser to modern version
- Enable JavaScript and Canvas in browser settings
- Use Chrome 90+, Firefox 88+, or Edge 90+

**3. Widget Too Small**

**Diagnosis:**
- Widget size < 2x2 cells (< 400x300px)
- Graph elements overlapping

**Solution:**
- Resize widget to at least 2x2 cells
- For many series, use 3x3 or larger

**4. CSS Not Loaded**

**Diagnosis:**
- Widget has no styling
- Elements visible but unstyled

**Solution:**
- Check browser console for CSS load errors
- Verify `/usr/share/zabbix/widgets/multigraph/assets/css/multigraph.css` exists
- Check file permissions: `chmod 644 multigraph.css`
- Clear browser cache

---

#### Issue: Pattern Matches Too Many Items

**Symptoms:**
- Widget shows error "Too many items matched"
- Graph has too many series to be readable

**Possible Causes:**

**1. Pattern Too Broad**

**Diagnosis:**
- Pattern like `*` matches everything
- Wildcard matches unintended items

**Solution:**
- Make pattern more specific
- Example: `*` → `CPU *` → `CPU * usage`
- Use regex for precise matching

**2. Item Limit Reached**

**Diagnosis:**
- Pattern matches > 100 items (hard limit)

**Solution:**
- Refine pattern to match fewer items
- Create multiple widgets with specific patterns
- Example: Split `CPU*` into `CPU [0-7] usage` and `CPU [8-15] usage`

**3. Multi-Host Pattern**

**Diagnosis:**
- Pattern matches 10 items per host × 20 hosts = 200 items

**Solution:**
- Reduce number of selected hosts
- Create separate widgets per host or host group
- Use more specific pattern

---

#### Issue: Timeframe Not Working

**Symptoms:**
- Dashboard time selector changes but widget doesn't update
- Widget shows wrong time range

**Possible Causes:**

**1. Dashboard Refresh Disabled**

**Diagnosis:**
- Dashboard not auto-refreshing

**Solution:**
- Click dashboard refresh button manually
- Enable auto-refresh: Dashboard settings → Refresh interval

**2. Timeframe Parsing Error**

**Diagnosis:**
- Using unsupported timeframe format
- Check browser console for errors

**Solution:**
- Use standard Zabbix time formats:
  - "Last X hours/days"
  - "now-6h"
  - "now/d" (today), "now/w" (this week), "now/M" (this month)
- Avoid custom date strings

**3. Widget Cache Issue**

**Diagnosis:**
- Widget cached with old timeframe

**Solution:**
- Hard refresh dashboard (Ctrl+F5)
- Clear browser cache
- Edit and re-save widget

---

#### Issue: Performance Problems

**Symptoms:**
- Slow widget loading (> 5 seconds)
- Browser lag when interacting with graph
- High CPU usage

**Possible Causes:**

**1. Too Many Data Points**

**Diagnosis:**
- Long timeframe (30+ days) × many items = 10,000+ points
- Browser struggles to render

**Solution:**
- Reduce timeframe (use "Last 7 days" instead of "Last 30 days")
- Reduce number of matched items
- Zabbix automatically uses trends for long timeframes

**2. Too Many Widget Instances**

**Diagnosis:**
- Dashboard has 10+ Multigraph widgets
- Each widget independently fetches data

**Solution:**
- Consolidate widgets where possible
- Split dashboards by purpose
- Increase dashboard refresh interval

**3. Heavy Browser**

**Diagnosis:**
- Browser has many tabs/extensions
- Low-spec client machine

**Solution:**
- Close unnecessary tabs
- Disable browser extensions
- Use lighter browser (Chrome vs Firefox)

---

#### Issue: Widget Not Appearing After Installation

**Symptoms:**
- Installed widget but can't find it in widget type list
- "Scan directory" doesn't detect widget

**Possible Causes:**

**1. Wrong Directory**

**Diagnosis:**
- Files copied to wrong location
- Directory named incorrectly

**Solution:**
- Verify location: `/usr/share/zabbix/widgets/multigraph/`
- Check directory name matches `namespace` in `manifest.json`
- Ensure `manifest.json` is in widget root

**2. Permission Issues**

**Diagnosis:**
- Web server can't read files
- Wrong file ownership

**Solution:**
```bash
sudo chmod -R 755 /usr/share/zabbix/widgets/multigraph
sudo chown -R www-data:www-data /usr/share/zabbix/widgets/multigraph
```

**3. Manifest Syntax Error**

**Diagnosis:**
- `manifest.json` has JSON syntax error
- Check Zabbix frontend log for errors

**Solution:**
- Validate JSON: `cat manifest.json | python -m json.tool`
- Fix syntax errors (missing comma, quote, etc.)
- Verify all required fields present

**4. PHP Autoload Issue**

**Diagnosis:**
- Namespace doesn't match directory structure
- Class files missing

**Solution:**
- Verify namespace: `Widgets\Multigraph\Actions` matches `actions/` directory
- Check all PHP files have correct namespace declaration
- Ensure capitalization matches exactly

---

### Error Messages and Meanings

| Error Message | Meaning | Solution |
|---------------|---------|----------|
| "No matching items" | Pattern matched zero items | Refine pattern, check item names |
| "Too many items matched" | Pattern matched > 100 items | Make pattern more specific |
| "Invalid pattern" | Regex syntax error | Validate regex, check escaping |
| "No data available" | Items found but no history | Check timeframe, item monitoring |
| "Failed to fetch data" | API error or timeout | Check Zabbix API logs, network |
| "Widget error" | General rendering error | Check browser console for details |
| "Permission denied" | No read permission on hosts | Adjust user group permissions |

---

### Diagnostic Checklist

When troubleshooting, check in this order:

**1. Verify Installation:**
- [ ] Files in `/usr/share/zabbix/widgets/multigraph/`
- [ ] Permissions: `755` directories, `644` files
- [ ] Ownership: `www-data:www-data`
- [ ] Widget enabled in Zabbix UI

**2. Check Configuration:**
- [ ] At least one host selected
- [ ] Non-empty pattern
- [ ] Pattern mode correct (wildcard vs regex)
- [ ] Valid regex syntax (if regex mode)

**3. Verify Data:**
- [ ] Items exist in Zabbix
- [ ] Items have history data
- [ ] Timeframe valid
- [ ] User has host permissions

**4. Test Pattern:**
- [ ] Go to **Configuration → Hosts → [Host] → Items**
- [ ] Use browser search (Ctrl+F) to test pattern
- [ ] Verify items match expected names

**5. Check Browser:**
- [ ] Modern browser (Chrome 90+, Firefox 88+, Edge 90+)
- [ ] JavaScript enabled
- [ ] No console errors (F12)
- [ ] Clear cache tried

**6. Review Logs:**
- [ ] Zabbix frontend log: `/var/log/apache2/error.log`
- [ ] PHP log: `/var/log/php8.4-fpm.log`
- [ ] Browser console: F12 → Console

---

### Getting Help

**1. Check Documentation:**
- **README.md** - Architecture, requirements, installation
- **USAGE.md** (this file) - User and admin manuals
- **ai.txt** - AI context for developers

**2. Check Zabbix Logs:**
```bash
sudo tail -f /var/log/apache2/error.log
```

**3. Enable Debug Mode:**

Edit `src/actions/WidgetView.php`:

```php
// Add at top of doAction() method
error_log("Multigraph Debug: hostids=" . json_encode($hostids));
error_log("Multigraph Debug: pattern=" . $item_pattern);
```

Check logs for debug output.

**4. Report Issue:**

Include in bug report:
- Zabbix version
- Widget version
- Error message (exact text)
- Browser and version
- Steps to reproduce
- Screenshots
- Browser console errors (F12)
- Zabbix frontend log excerpt

**5. GitHub Issues:**

https://github.com/rheingold/zabbix_addons/issues

Provide:
- Widget version (0.1.0)
- Zabbix version
- Detailed description
- Configuration details (sanitized)
- Expected vs actual behavior

---

## Appendix: Quick Reference

### Installation Command Summary

```bash
# Download
git clone https://github.com/rheingold/zabbix_addons.git
cd zabbix_addons/multigraph

# Install
sudo mkdir -p /usr/share/zabbix/widgets/multigraph
sudo cp -r src/* /usr/share/zabbix/widgets/multigraph/
sudo chmod -R 755 /usr/share/zabbix/widgets/multigraph
sudo chown -R www-data:www-data /usr/share/zabbix/widgets/multigraph

# Enable in UI: Administration → General → Modules → Scan directory
```

### Pattern Examples Quick Reference

```
# Wildcards
CPU*                      # All CPU metrics
CPU * usage               # CPU 0 usage, CPU 1 usage, etc.
*traffic in               # All inbound traffic metrics
Memory ?                  # Memory A, Memory B, etc.

# Regex
CPU \d+ usage             # CPU cores with numbers
Interface eth[0-9]+       # Network interfaces eth0, eth1, etc.
(CPU|Memory) .* usage     # CPU or Memory usage metrics
^Disk /dev/sd[a-z]$       # Specific disk naming pattern
```

### Time Selector Reference

```
Last 1 hour               # now-1h
Last 6 hours              # now-6h
Last 24 hours             # now-1d
Last 7 days               # now-7d
Today                     # now/d
This week                 # now/w
This month                # now/M
```

---

**Document Version:** 1.0  
**Last Updated:** November 10, 2025  
**Widget Version:** 0.1.0  
**For:** Zabbix 7.4.3+
