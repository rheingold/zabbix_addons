# Multigraph - Zabbix Widget

```
╔═══════════════════════════════════════════════════════════════════════════╗
║                                                                           ║
║                       MULTIGRAPH WIDGET                                    ║
║                  Enhanced Zabbix Graph Visualization                      ║
║                                                                           ║
║  Version:          0.1.8-draft (Template Dashboard Support Complete)     ║
║  Created:          November 5, 2025                                       ║
║  Last Updated:     November 14, 2025                                      ║
║                                                                           ║
║  Lead & Architecture:  lukas@plachy.eu                                    ║
║  Development:          Claude Sonnet 4 (AI Assistant, Anthropic)          ║
║                                                                           ║
║  License:          MIT License (see LICENSE file)                         ║
║  Repository:       https://github.com/rheingold/zabbix_addons            ║
║                                                                           ║
╚═══════════════════════════════════════════════════════════════════════════╝
```

**⚠️ IMPORTANT NOTICE**

**This version (0.1.8-draft) is:**
- ✅ Tested and working with **Zabbix Dashboard (New UI)**
- ❌ **NOT tested** with **Classic Zabbix Frontend**
- ⚠️ **Draft release** - template dashboard support completed but needs wider testing
- 🔧 Under active development

**Production Use:** Not recommended until v1.0.0 with full test coverage.

---

**AI-Human Collaborative Development**  
*This project represents collaborative engineering between human architecture/design and AI-assisted implementation.*

---

## Table of Contents

- [Project Overview](#project-overview)
- [Quick Start](#quick-start)
- [Target Platform](#target-platform)
- [Architecture](#architecture)
- [Functional Requirements](#functional-requirements)
- [Technical Requirements](#technical-requirements)
- [Development Roadmap](#development-roadmap)
- [Current Issues](#current-issues)
- [Building from Source](#building-from-source)
- [Source Structure](#source-structure)
- [Documentation](#documentation)

---

## Project Overview

### B.2. Project Name and Main Goal

**Multigraph** is a custom Zabbix dashboard widget that provides **pattern-based multi-item graph visualization**.

**Main Goal:** Enable dynamic graph creation using item name patterns (wildcards/regex) instead of manual item selection, allowing flexible visualization of multiple related metrics in a single widget.

### B.2.1. Usage Examples

**Example 1: CPU Usage Across All Cores**
```
Host: myserver
Pattern: CPU * usage
Mode: Wildcard
Result: Matches "CPU 0 usage", "CPU 1 usage", "CPU 2 usage", etc.
```

**Example 2: Network Traffic Pattern**
```
Host: router
Pattern: Interface .* traffic in
Mode: Regex
Result: Matches "Interface eth0 traffic in", "Interface eth1 traffic in", etc.
```

**Example 3: Memory Metrics**
```
Host: webserver
Pattern: Memory*
Mode: Wildcard
Result: Matches "Memory total", "Memory used", "Memory available", etc.
```

### B.2.2. Quick Install Manual

**Prerequisites:** Zabbix 7.4.3+ with dashboard enabled

**Installation Steps:**
1. Download widget from GitHub: https://github.com/rheingold/zabbix_addons/tree/tmp0.1/multigraph
2. Copy `src/` folder contents to `/usr/share/zabbix/widgets/multigraph/` on Zabbix server
3. Set permissions: `chmod -R 755 /usr/share/zabbix/widgets/multigraph`
4. In Zabbix web UI: Navigate to **Administration → General → Modules**
5. Click **Scan directory** button
6. Enable **Multigraph** widget

**For detailed installation and configuration, see [USAGE.md](USAGE.md) - Administrator's Manual (C2.2)**

---

## Target Platform

### B.3. Platform Specification (E.3.1.A)

**Server-Side:**
- **Zabbix Version:** 7.4.3 or higher
- **PHP Version:** 8.4+ (Zabbix 7.x requirement)
- **Web Server:** Apache 2.4+ or Nginx 1.18+
- **Operating System:** Linux (tested on Debian/Ubuntu)
- **Optional:** Docker 20.10+ (for containerized deployment)

**Client-Side:**
- **Browser:** Modern browser with HTML5 Canvas support
  - Chrome 90+
  - Firefox 88+
  - Edge 90+
  - Safari 14+
- **JavaScript:** ES6+ support required
- **Interface:** Zabbix Dashboard (New UI) - **Classic UI not tested**

**Development Environment:**
- **PowerShell:** 5.1+ (for deployment scripts)
- **Git:** 2.30+
- **SSH Client:** OpenSSH or compatible

---

## Architecture

### B.4. General Architecture Schema

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        ZABBIX DASHBOARD                                  │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │                     MULTIGRAPH WIDGET                              │  │
│  │  ┌─────────────────┐        ┌──────────────────┐                  │  │
│  │  │  Configuration  │        │  Display View    │                  │  │
│  │  │  (Edit Mode)    │        │  (View Mode)     │                  │  │
│  │  └────────┬────────┘        └────────┬─────────┘                  │  │
│  └───────────┼──────────────────────────┼─────────────────────────────┘  │
└──────────────┼──────────────────────────┼────────────────────────────────┘
               │                          │
               │ HTTP POST                │ HTTP GET
               │ (Configuration)          │ (with timeframe, hostids, pattern)
               │                          │
┌──────────────▼──────────────────────────▼────────────────────────────────┐
│                     ZABBIX PHP BACKEND                                    │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │                   Widget.php (Entry Point)                        │    │
│  │  ┌──────────────────┐              ┌──────────────────────────┐  │    │
│  │  │ WidgetEdit.php   │              │ WidgetView.php           │  │    │
│  │  │ (Controller)     │              │ (Controller)             │  │    │
│  │  │                  │              │                          │  │    │
│  │  │ - Prepare form   │              │ - Parse timeframe        │  │    │
│  │  │ - Validate input │              │ - Match items by pattern │  │    │
│  │  └──────────────────┘              │ - Fetch history data     │  │    │
│  │                                     │ - Prepare graph data     │  │    │
│  │  ┌──────────────────────────────┐  └─────────┬────────────────┘  │    │
│  │  │ WidgetForm.php (Model)       │            │                   │    │
│  │  │ - Field definitions          │◄───────────┘                   │    │
│  │  │ - Validation rules           │                                │    │
│  │  │ - Constants                  │                                │    │
│  │  └──────────────────────────────┘                                │    │
│  │                                                                   │    │
│  │  ┌──────────────────────────────┐  ┌──────────────────────────┐ │    │
│  │  │ MatchedItemsData.php (Model) │  │ GraphData.php (Model)    │ │    │
│  │  │ - findItems()                │  │ - fetchHistoryData()     │ │    │
│  │  │ - Wildcard/Regex matching    │  │ - prepareGraphData()     │ │    │
│  │  └──────────┬───────────────────┘  └──────────┬───────────────┘ │    │
│  └─────────────┼──────────────────────────────────┼─────────────────┘    │
└────────────────┼──────────────────────────────────┼──────────────────────┘
                 │                                  │
                 │ API::Item()::get()               │ API::History()::get()
                 │                                  │
┌────────────────▼──────────────────────────────────▼──────────────────────┐
│                        ZABBIX API                                         │
│  ┌─────────────────────────────────┐  ┌─────────────────────────────┐   │
│  │     Item API                    │  │     History API             │   │
│  │  - Query items by host          │  │  - Fetch time series data   │   │
│  │  - Filter by pattern            │  │  - Return [[ts, val], ...]  │   │
│  └─────────────────────────────────┘  └─────────────────────────────┘   │
└───────────────────────────────────────────────────────────────────────────┘
                 │                                  │
┌────────────────▼──────────────────────────────────▼──────────────────────┐
│                       ZABBIX DATABASE                                     │
│  - Items table                                                            │
│  - History/Trends tables                                                  │
└───────────────────────────────────────────────────────────────────────────┘

     ┌─────────────────── HTTP RESPONSE ──────────────────────┐
     │                                                         │
     ▼                                                         ▼
┌─────────────────────────────┐      ┌──────────────────────────────────────┐
│  widget.edit.php (View)     │      │  widget.view.php (View)              │
│  - Render form HTML         │      │  - Render canvas elements            │
│  - Load widget.edit.js      │      │  - Pass graph_data to JavaScript     │
└─────────────┬───────────────┘      └────────────┬─────────────────────────┘
              │                                   │
              │                                   │
┌─────────────▼───────────────┐      ┌───────────▼──────────────────────────┐
│  CLIENT-SIDE JAVASCRIPT     │      │  CLIENT-SIDE JAVASCRIPT              │
│                             │      │                                      │
│  widget.edit.js             │      │  class.widget.js                     │
│  - Form interactions        │      │  - renderGraph()                     │
│  - Pattern builder          │      │  - HTML5 Canvas rendering            │
│  - Validation               │      │  - Hover tooltips                    │
│                             │      │  - Responsive resize                 │
└─────────────────────────────┘      └──────────────────────────────────────┘
```

### Key Components

**1. Entry Point**
- `Widget.php` - Widget registration and routing

**2. MVC Controllers (actions/)**
- `WidgetView.php` - Display logic, data orchestration
- `WidgetEdit.php` - Configuration form logic

**3. MVC Models (includes/)**
- `MatchedItemsData.php` - Item pattern matching (wildcard/regex)
- `GraphData.php` - History data fetching and preparation
- `WidgetForm.php` - Form field definitions and validation

**4. MVC Views (views/)**
- `widget.view.php` - Graph display template (HTML5 Canvas)
- `widget.edit.php` - Configuration form template
- `widget.edit.js.php` - Form JavaScript initialization

**5. Frontend Assets (assets/)**
- `class.widget.js` - Main widget rendering (extends CWidget)
- `widget.edit.js` - Form interactions (extends CWidgetForm)
- `multigraph.css` - Widget styling

**6. Data Flow**
```
User Config → WidgetEdit → WidgetForm → Save
Dashboard Load → WidgetView → MatchedItemsData → API::Item
                          → GraphData → API::History
                          → widget.view.php → class.widget.js → Canvas Render
```

---

## Functional Requirements

### B.5. Main Functional Requirements

**FR-1: Pattern-Based Item Selection**
- User shall be able to match items using wildcard patterns (`*`, `?`)
- User shall be able to match items using regular expressions
- System shall display all matched items in a single graph

**FR-2: Multi-Host Support**
- User shall be able to select multiple hosts
- Items shall be matched across all selected hosts

**FR-3: Graph Rendering**
- System shall render time series data using HTML5 Canvas
- Graph shall display multiple series with different colors
- Graph shall show X-axis (time) and Y-axis (values) with labels

**FR-4: Interactive Features**
- User shall be able to hover over data points to see precise values
- Graph shall highlight hovered data points
- Tooltip shall show series name, value, and timestamp

**FR-5: Customization**
- User shall be able to configure graph colors
- User shall be able to configure legend position (top-left, right, bottom)
- User shall be able to configure Y-axis range (auto or fixed)
- User shall be able to enable/disable grid
- User shall be able to configure text colors (legend, axes)

**FR-6: Dashboard Integration**
- Widget shall integrate with Zabbix dashboard timeframe selector
- Widget shall automatically refresh when timeframe changes
- Widget shall support responsive resize

### B.5.1. Detailed Requirements and Constraints

**FR-1 Details: Pattern Matching**
- **Wildcard Mode:**
  - `*` matches zero or more characters
  - `?` matches exactly one character
  - Examples: `CPU*`, `Memory ?`, `Interface * traffic`
- **Regex Mode:**
  - Full PCRE regex support
  - Examples: `CPU \d+ usage`, `Memory (total|used|free)`
- **Constraints:**
  - Pattern matching is case-sensitive
  - Maximum 100 items matched per query
  - Items must belong to selected hosts

**FR-3 Details: Graph Rendering**
- **Canvas Layering:**
  - Base canvas (z-index: 1) for graph rendering
  - Overlay canvas (z-index: 2) for hover effects
- **Rendering Strategy:**
  - Stacked area fills (bottom-to-top)
  - Line series on top
  - Adaptive time formatting based on range
- **Constraints:**
  - Minimum widget size: 200x150 pixels
  - Maximum data points per series: 1000 (recommended)

**FR-4 Details: Interactivity**
- **Hover Behavior:**
  - Find closest data point by timestamp
  - Draw 5px radius circle at point
  - Show tooltip with series name, value (2 decimals), timestamp
- **Constraints:**
  - Overlay canvas must capture pointer events
  - Tooltip positioned to avoid edge overflow

**FR-5 Details: Customization**
- **Color Configuration:**
  - Comma-separated hex colors (e.g., `#1f77b4,#ff7f0e`)
  - Automatic color cycling for excess series
  - Fill opacity: 0-1 (0 = transparent, 1 = opaque)
- **Y-Axis Range:**
  - `auto` = calculate from data
  - Fixed: specify min/max values
  - Constraint: min < max
- **Grid Density:**
  - `auto` = 5 horizontal lines
  - Fixed: specify number of lines (1-20)

**FR-6 Details: Dashboard Integration**
- **Timeframe Parsing:**
  - Absolute: Unix timestamp, "YYYY-MM-DD HH:MM:SS"
  - Relative: `now`, `now-6h`, `now-1d`, `now-1w`, `now-1M`, `now-1y`
  - Period rounding: `now/d` (today), `now/w` (this week), `now/M` (this month)
- **Constraints:**
  - Timeframe parsing failures default to last 1 hour
  - Week starts on Monday (ISO 8601)

---

## Technical Requirements

### B.6. Architectural and Usability Constraints

**Architecture Constraints:**

1. **Platform:** Zabbix 7.4.3+ widget framework
2. **Language:** PHP 8.4+ (backend), JavaScript ES6+ (frontend)
3. **MVC Pattern:** Actions (controllers), Includes (models), Views (templates)
4. **Namespace:** `Widgets\Multigraph\{Actions|Includes}`
5. **Autoloading:** PSR-4 compliant (directory names must match namespace)
6. **API:** Zabbix API for data retrieval (no direct database access)

**Performance Constraints:**

1. **Response Time:** Widget load < 2 seconds (typical network/dataset)
2. **Data Points:** Recommend max 1000 points per series for smooth rendering
3. **Items Matched:** Max 100 items per pattern query (configurable in code)
4. **Canvas Size:** Efficient rendering up to 4K resolution

**Usability Constraints:**

1. **Form Validation:** All fields validated before save
2. **Error Messages:** Clear, actionable error messages shown in widget
3. **No Data Handling:** Graceful display when no data available
4. **Responsive:** Widget adapts to all dashboard cell sizes

**Browser Compatibility:**

1. **Required:** HTML5 Canvas, ES6 JavaScript
2. **Tested:** Chrome 90+, Firefox 88+, Edge 90+
3. **Not Tested:** IE11, Classic Zabbix UI

**Security Constraints:**

1. **Authentication:** Uses Zabbix session authentication
2. **Authorization:** Respects Zabbix user permissions for hosts/items
3. **Input Validation:** All user input sanitized and validated
4. **XSS Prevention:** Output escaped in templates

---

## Development Roadmap

### B.7. Work Packages and Development Phases

#### **Phase A: Planning (Completed)**
- [x] A.1. Requirements gathering
- [x] A.2. Architecture design
- [x] A.3. Technology selection

#### **Phase B: Core Development**

**B.1. Widget Foundation (Completed)**
- [x] B.1.1. Widget manifest and registration
- [x] B.1.2. MVC structure setup
- [x] B.1.3. Basic form definition

**B.2. Graph Rendering (Completed)**
- [x] B.2.1. Canvas setup and rendering pipeline
- [x] B.2.2. Data fetching and preparation
- [x] B.2.3. Line and area chart implementation
- [x] B.2.4. Axes and grid rendering
- [x] B.2.5. Legend rendering

**B.3. Item Pattern Matching (Completed)**
- [x] B.3.1. Wildcard pattern matching
- [x] B.3.2. Regex pattern matching
- [x] B.3.3. Multi-host support

**B.4. Interactivity (Completed)**
- [x] B.4.1. Hover tooltips with interpolation
- [x] B.4.2. Overlay canvas event handling
- [x] B.4.3. Responsive resize

**B.5. Configuration (Completed)**
- [x] B.5.1. Form fields (hosts, pattern, timeframe)
- [x] B.5.2. Color configuration
- [x] B.5.3. Y-axis configuration
- [x] B.5.4. Grid and legend configuration
- [x] B.5.5. Text color configuration

**B.6. Dashboard Integration (Completed)**
- [x] B.6.1. Timeframe selector integration
- [x] B.6.2. Timeframe parsing (absolute/relative)
- [x] B.6.3. Auto-refresh on timeframe change
- [x] B.6.4. Fixed: Period rounding (`now/d`, `now/w`, `now/M`)

**B.7. Documentation (Completed - This Phase)**
- [x] B.7.1. Code documentation (PHPDoc, JSDoc)
- [x] B.7.2. File headers and inline comments
- [x] B.7.3. Directory READMEs
- [x] B.7.4. Main README.md (this file)

#### **Phase C: Testing and Quality (Planned)**

**C.1. Unit Testing (TODO)**
- [ ] C.1.1. PHP unit tests (PHPUnit)
  - [ ] MatchedItemsData tests
  - [ ] GraphData tests
  - [ ] WidgetForm validation tests
- [ ] C.1.2. JavaScript unit tests (Jest)
  - [ ] renderGraph() tests
  - [ ] parseZabbixTime() tests
  - [ ] Interpolation tests

**C.2. Integration Testing (TODO)**
- [ ] C.2.1. Widget lifecycle tests
- [ ] C.2.2. API interaction tests
- [ ] C.2.3. Dashboard integration tests

**C.3. Browser Compatibility Testing (TODO)**
- [ ] C.3.1. Chrome testing
- [ ] C.3.2. Firefox testing
- [ ] C.3.3. Edge testing
- [ ] C.3.4. Safari testing

**C.4. Classic UI Testing (TODO)**
- [ ] C.4.1. Test widget in Classic Zabbix UI
- [ ] C.4.2. Fix compatibility issues if found

**C.5. Performance Testing (TODO)**
- [ ] C.5.1. Large dataset rendering (1000+ points)
- [ ] C.5.2. Multiple widget instances
- [ ] C.5.3. Memory leak testing

#### **Phase D: Enhancement (Planned)**

**D.1. Advanced Features (TODO)**
- [ ] D.1.1. Export graph as PNG/SVG
- [ ] D.1.2. Data point annotations
- [ ] D.1.3. Multiple Y-axes support
- [ ] D.1.4. Threshold lines/bands

**D.2. Usability Improvements (TODO)**
- [ ] D.2.1. Item pattern builder UI
- [ ] D.2.2. Color picker UI
- [ ] D.2.3. Preview mode in configuration

**D.3. Additional Chart Types (TODO)**
- [ ] D.3.1. Bar charts
- [ ] D.3.2. Scatter plots
- [ ] D.3.3. Heatmaps

#### **Phase E: Release (Planned)**

**E.1. Release 1.0.0 (TODO)**
- [ ] E.1.1. All Phase C testing complete
- [ ] E.1.2. Documentation complete (README, USAGE, REFERENCE)
- [ ] E.1.3. Deployment automation
- [ ] E.1.4. Release notes and changelog

### Version History

**v0.1.8-draft (2025-11-14) - Template Dashboard Support (DRAFT RELEASE)**
- ✅ Pattern builder now works on template dashboards
- ✅ Detects template context from URL action parameter
- ✅ Extracts templateid from breadcrumb links
- ✅ Proper dialog closing and widget reactivation
- ✅ Z-index management for dialog layering
- ✅ Cleaned production code (removed debug logging)
- ⚠️ **Draft status:** Needs broader testing before stable release

**v0.1.7 (2025-11-14) - Pattern Builder UI**
- Added interactive Pattern Builder button next to item_pattern field
- Created ItemList.php AJAX endpoint for fetching items and item prototypes
- Built modal UI showing items with {#MACROS} for easy selection
- Auto-converts {#MACROS} to wildcards (*) or regex (.+) based on pattern mode
- Supports both regular items and LLD item prototypes
- Validates host selection before showing item list
- Click any item to auto-fill pattern field with macro conversion

**v0.1.6 (2025-11-13) - Named Color Sets**
- Added predefined color palette dropdown (Material, Pastel, Vibrant, Earth, etc.)
- Integer-based color set keys for efficient storage
- Auto-population of graph_colors field on selection
- 8 professional color schemes included

**v0.1.5 (2025-11-12) - Module Rescan Action**
- Added rescan action for clearing Zabbix module cache
- Enables hot-reload without restarting web server

**v0.1.0 (2025-11-10) - Initial Release**
- Initial implementation
- Core functionality complete
- Documentation complete
- Testing framework not yet implemented
- **Status:** Functional but untested for production

---

## Current Issues

### B.8. Known Issues and Active Work

#### **Issue #1: Classic UI Compatibility - UNTESTED**
- **Status:** Unknown
- **Priority:** High
- **Description:** Widget has not been tested with Zabbix Classic UI, only Dashboard (New UI)
- **Impact:** May not work or may have rendering issues in Classic UI
- **Planned Fix:** Phase C.4 - Classic UI Testing

#### **Issue #2: Large Dataset Performance - NOT OPTIMIZED**
- **Status:** Known limitation
- **Priority:** Medium
- **Description:** No optimization for datasets with >1000 points per series
- **Impact:** May cause slow rendering or browser lag with large datasets
- **Workaround:** Limit matched items or use shorter timeframes
- **Planned Fix:** Phase C.5 - Performance testing and optimization

#### **Issue #3: No Automated Testing**
- **Status:** Critical gap
- **Priority:** High
- **Description:** No unit tests, integration tests, or automated testing framework
- **Impact:** Regression risk, difficult to validate changes
- **Planned Fix:** Phase C.1-C.2 - Implement test framework

#### **Issue #4: Item Limit Not Configurable via UI**
- **Status:** Enhancement needed
- **Priority:** Low
- **Description:** Max items limit (100) hardcoded in MatchedItemsData.php
- **Impact:** Cannot increase limit without code modification
- **Planned Fix:** Phase D.2 - Add configuration option

#### **Resolved Issues:**

**✅ Issue #0: Timeframe Parsing - FIXED (2025-11-10)**
- **Description:** Dashboard time selector expressions like "now/w", "now/M" not parsed correctly
- **Fix:** Implemented `parseZabbixTime()` method with full support for period rounding
- **Commit:** 89dada2

---

## Building from Source

### B.9. Build Documentation

**Important:** This is a **PHP/JavaScript widget** for Zabbix. There is **no compilation step** - the source code is deployed directly.

### Prerequisites

**Required:**
1. **Zabbix 7.4.3+** - Target platform
   - Download: https://www.zabbix.com/download
   - Installation guide: https://www.zabbix.com/documentation/current/manual/installation
   
2. **PHP 8.4+** - Backend runtime
   - Download: https://www.php.net/downloads
   - Zabbix requires PHP with extensions: gd, mbstring, bcmath, sockets, etc.
   - Note: PHP version must match Zabbix requirements

3. **Web Server** - Apache or Nginx
   - Apache 2.4+: https://httpd.apache.org/
   - Nginx 1.18+: https://nginx.org/

**Development Tools:**
1. **Git** - Version control
   - Download: https://git-scm.com/downloads
   - Required for cloning repository

2. **Text Editor / IDE**
   - Recommended: VS Code with PHP and JavaScript extensions
   - VS Code: https://code.visualstudio.com/

3. **PowerShell 5.1+** (Windows) or Bash (Linux)
   - For deployment scripts

**Optional:**
1. **Docker 20.10+** - For containerized Zabbix
   - Download: https://www.docker.com/get-started
   - Useful for testing without installing Zabbix natively

2. **PHPUnit** - For unit testing (Phase C)
   - Installation: https://phpunit.de/getting-started.html

3. **Node.js + Jest** - For JavaScript testing (Phase C)
   - Node.js: https://nodejs.org/
   - Jest: https://jestjs.io/

### Development Environment Setup

**1. Clone Repository**
```bash
git clone https://github.com/rheingold/zabbix_addons.git
cd zabbix_addons/multigraph
```

**2. Review Source Structure**
```bash
# All source code is in src/ directory
ls -la src/

# No build step needed - files are deployed as-is
```

**3. Development Workflow**
```bash
# Edit files in src/ directory
# Test by deploying to Zabbix test instance
# See "Deployment" section below
```

### Deployment

**Method 1: Manual Deployment (Recommended for Development)**

```bash
# On Zabbix server:
sudo mkdir -p /usr/share/zabbix/widgets/multigraph
sudo cp -r src/* /usr/share/zabbix/widgets/multigraph/
sudo chmod -R 755 /usr/share/zabbix/widgets/multigraph
sudo chown -R www-data:www-data /usr/share/zabbix/widgets/multigraph

# In Zabbix UI:
# Administration → General → Modules → Scan directory
```

**Method 2: Docker Deployment**

**For containerized Zabbix installations:**

**Prerequisites:**
- Docker 20.10+
- Docker Compose (optional)
- SSH access to Docker host
- Custom Zabbix web image build directory

**Deployment Process:**

**Step 1: Prepare Build Context**

Create a custom Zabbix build directory with Dockerfile:

```bash
# On Docker host, create build directory
mkdir -p /root/customzabbix/usr/share/zabbix/widgets/multigraph
cd /root/customzabbix
```

**Step 2: Create Dockerfile**

Create `/root/customzabbix/Dockerfile`:

```dockerfile
# Dockerfile for custom Zabbix web with Multigraph widget
# Base image - use official Zabbix web-apache image matching your version
FROM zabbix/zabbix-web-apache-mysql:7.4.3-alpine

# Maintainer
LABEL maintainer="lukas@plachy.eu"
LABEL description="Zabbix Web with Multigraph custom widget"

# Switch to root for file operations
USER root

# Copy Multigraph widget to Zabbix widgets directory
COPY usr/share/zabbix/widgets/multigraph /usr/share/zabbix/widgets/multigraph

# Set correct permissions
RUN chmod -R 755 /usr/share/zabbix/widgets/multigraph && \
    chown -R www-data:www-data /usr/share/zabbix/widgets/multigraph

# Optional: Copy additional customizations
# COPY custom-zabbix.conf.php /etc/zabbix/web/zabbix.conf.php

# Optional: Install additional PHP extensions if needed
# RUN apk add --no-cache php83-gd php83-mbstring

# Switch back to default user
USER www-data

# Default command (inherited from base image)
CMD ["apache2-foreground"]
```

**Alternative Dockerfile for Debian/Ubuntu base:**

```dockerfile
# For Debian/Ubuntu-based Zabbix images
FROM zabbix/zabbix-web-apache-mysql:7.4.3-ubuntu

LABEL maintainer="lukas@plachy.eu"
LABEL description="Zabbix Web with Multigraph custom widget"

USER root

# Copy widget
COPY usr/share/zabbix/widgets/multigraph /usr/share/zabbix/widgets/multigraph

# Set permissions
RUN chmod -R 755 /usr/share/zabbix/widgets/multigraph && \
    chown -R www-data:www-data /usr/share/zabbix/widgets/multigraph

# Optional: Install additional dependencies
# RUN apt-get update && apt-get install -y \
#     php8.4-gd \
#     php8.4-mbstring \
#     && rm -rf /var/lib/apt/lists/*

USER www-data
```

**Step 3: Copy Widget Files to Build Context**

From your development machine:

```bash
# Copy widget files from dev machine to Docker host
scp -r src/* user@dockerhost:/root/customzabbix/usr/share/zabbix/widgets/multigraph/

# Or using SSH key
scp -i ~/.ssh/id_rsa -r src/* user@dockerhost:/root/customzabbix/usr/share/zabbix/widgets/multigraph/
```

From Docker host:

```bash
# Verify files are in build context
ls -la /root/customzabbix/usr/share/zabbix/widgets/multigraph/

# Expected structure:
# multigraph/
# ├── manifest.json
# ├── Widget.php
# ├── actions/
# ├── includes/
# ├── views/
# └── assets/
```

**Step 4: Build Custom Image**

```bash
# Navigate to build directory
cd /root/customzabbix

# Build image with tag
sudo docker build -t zabbix-web-custom:latest .

# Build with no cache (for clean rebuild)
sudo docker build --no-cache -t zabbix-web-custom:latest .

# Build with specific tag (version)
sudo docker build -t zabbix-web-custom:0.1.0 .
```

**Step 5: Update Docker Compose (if using)**

Edit your `docker-compose.yml`:

```yaml
services:
  zabbix-web:
    # Replace official image with custom image
    # image: zabbix/zabbix-web-apache-mysql:7.4.3-alpine
    image: zabbix-web-custom:latest
    
    # Rest of configuration remains the same
    container_name: zabbix-web
    environment:
      - DB_SERVER_HOST=zabbix-db
      - MYSQL_USER=zabbix
      - MYSQL_PASSWORD=${MYSQL_PASSWORD}
      - ZBX_SERVER_HOST=zabbix-server
    ports:
      - "8080:8080"
    volumes:
      - /etc/localtime:/etc/localtime:ro
    depends_on:
      - zabbix-db
      - zabbix-server
    restart: unless-stopped
```

**Step 6: Deploy and Restart**

**With Docker Compose:**

```bash
# Stop current container
sudo docker-compose stop zabbix-web

# Remove old container (preserves volumes)
sudo docker-compose rm -f zabbix-web

# Start with new image
sudo docker-compose up -d zabbix-web

# Verify container is running
sudo docker-compose ps

# Check logs
sudo docker-compose logs -f zabbix-web
```

**Without Docker Compose:**

```bash
# Stop and remove old container
sudo docker stop zabbix-web
sudo docker rm zabbix-web

# Run new container (adjust parameters to match your setup)
sudo docker run -d \
  --name zabbix-web \
  --network zabbix-net \
  -e DB_SERVER_HOST=zabbix-db \
  -e MYSQL_USER=zabbix \
  -e MYSQL_PASSWORD=secret \
  -e ZBX_SERVER_HOST=zabbix-server \
  -p 8080:8080 \
  --restart unless-stopped \
  zabbix-web-custom:latest

# Verify container is running
sudo docker ps | grep zabbix-web

# Check logs
sudo docker logs -f zabbix-web
```

**Step 7: Enable Widget in Zabbix**

1. Open Zabbix web interface
2. Navigate to **Administration → General → Modules**
3. Click **Scan directory**
4. Enable **Multigraph** widget
5. Click **Update**

**Complete Deployment Script Example:**

```bash
#!/bin/bash
# deploy-multigraph.sh - Complete deployment automation

set -e  # Exit on error

# Configuration
DOCKER_HOST="192.168.254.16"
SSH_USER="aibot"
SSH_KEY="~/.ssh/id_aibot"
BUILD_DIR="/root/customzabbix"
WIDGET_DIR="$BUILD_DIR/usr/share/zabbix/widgets/multigraph"
IMAGE_NAME="zabbix-web-custom"
IMAGE_TAG="latest"
CONTAINER_NAME="zabbix-web"

echo "=== Multigraph Widget Deployment ==="
echo "Target: $SSH_USER@$DOCKER_HOST"
echo "Image: $IMAGE_NAME:$IMAGE_TAG"
echo ""

# Step 1: Copy files to Docker host
echo "Step 1: Copying widget files to Docker host..."
ssh -i "$SSH_KEY" "$SSH_USER@$DOCKER_HOST" "mkdir -p $WIDGET_DIR"
scp -i "$SSH_KEY" -r src/* "$SSH_USER@$DOCKER_HOST:$WIDGET_DIR/"
echo "✓ Files copied"

# Step 2: Build Docker image
echo "Step 2: Building Docker image..."
ssh -i "$SSH_KEY" "$SSH_USER@$DOCKER_HOST" \
  "cd $BUILD_DIR && sudo docker build -t $IMAGE_NAME:$IMAGE_TAG ."
echo "✓ Image built"

# Step 3: Restart container
echo "Step 3: Restarting container..."
ssh -i "$SSH_KEY" "$SSH_USER@$DOCKER_HOST" \
  "sudo docker-compose -f /path/to/docker-compose.yml restart $CONTAINER_NAME"
echo "✓ Container restarted"

# Step 4: Verify deployment
echo "Step 4: Verifying deployment..."
ssh -i "$SSH_KEY" "$SSH_USER@$DOCKER_HOST" \
  "sudo docker exec $CONTAINER_NAME ls -la /usr/share/zabbix/widgets/multigraph/"
echo "✓ Deployment verified"

echo ""
echo "=== Deployment Complete ==="
echo "Next steps:"
echo "1. Open Zabbix UI: http://$DOCKER_HOST:8080"
echo "2. Navigate to Administration → General → Modules"
echo "3. Click 'Scan directory'"
echo "4. Enable 'Multigraph' widget"
```

**Dockerfile Best Practices:**

1. **Base Image Selection:**
   - Match your Zabbix version exactly: `7.4.3-alpine` or `7.4.3-ubuntu`
   - Use official images from Docker Hub: `zabbix/zabbix-web-apache-mysql`

2. **COPY Directives:**
   - Copy entire widget directory: `COPY usr/share/zabbix/widgets/multigraph /usr/share/zabbix/widgets/multigraph`
   - Preserve directory structure in build context
   - Use relative paths from build context root

3. **Permissions:**
   - Always set permissions after COPY: `chmod -R 755`
   - Set correct ownership: `chown -R www-data:www-data`
   - Switch back to non-root user after file operations

4. **Layer Optimization:**
   - Combine RUN commands with `&&` to reduce layers
   - Clean up package manager cache in same layer
   - Use `.dockerignore` to exclude unnecessary files

5. **Build Context:**
   - Keep widget files in predictable location: `usr/share/zabbix/widgets/multigraph/`
   - Match container paths for clarity
   - Include only necessary files in build context

**Troubleshooting Docker Deployment:**

**Issue: "Widget not found after deployment"**
```bash
# Verify files inside container
docker exec zabbix-web ls -la /usr/share/zabbix/widgets/multigraph/

# Check permissions
docker exec zabbix-web stat /usr/share/zabbix/widgets/multigraph/manifest.json

# Verify manifest.json is valid
docker exec zabbix-web cat /usr/share/zabbix/widgets/multigraph/manifest.json
```

**Issue: "Permission denied errors"**
```bash
# Fix permissions inside running container
docker exec -u root zabbix-web chmod -R 755 /usr/share/zabbix/widgets/multigraph
docker exec -u root zabbix-web chown -R www-data:www-data /usr/share/zabbix/widgets/multigraph

# Or rebuild image with correct permissions in Dockerfile
```

**Issue: "Changes not appearing after rebuild"**
```bash
# Remove old images and rebuild
docker rmi zabbix-web-custom:latest
docker build --no-cache -t zabbix-web-custom:latest .
docker-compose up -d --force-recreate zabbix-web

# Clear browser cache
# Ctrl+Shift+Delete or hard refresh (Ctrl+F5)
```

**Method 3: Automated PowerShell Deployment**

For Windows development environments with SSH access to Docker host:

**Prerequisites:**
- Windows PowerShell 5.1+ or PowerShell Core 7+
- OpenSSH client (built-in on Windows 10+)
- SSH key pair for authentication (optional but recommended)
- plink.exe from PuTTY (alternative to OpenSSH)

**PowerShell Deployment Script:**

```powershell
# deploy-multigraph.ps1 - PowerShell deployment automation

# Configuration
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
Write-Host "Step 1: Copying widget files to Docker host..." -ForegroundColor Yellow

# Create remote directory
ssh -i $sshKeyPath "$sshUser@$dockerHost" "mkdir -p $widgetDir"

# Copy all source files
scp -i $sshKeyPath -r .\src\* "$sshUser@$dockerHost`:$widgetDir/"

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Files copied successfully" -ForegroundColor Green
} else {
    Write-Host "✗ File copy failed" -ForegroundColor Red
    exit 1
}

# Step 2: Build Docker image
Write-Host "`nStep 2: Building Docker image..." -ForegroundColor Yellow

$buildCommand = "cd $buildDir && sudo docker build -t $imageName`:$imageTag ."
ssh -i $sshKeyPath "$sshUser@$dockerHost" $buildCommand

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Image built successfully" -ForegroundColor Green
} else {
    Write-Host "✗ Image build failed" -ForegroundColor Red
    exit 1
}

# Step 3: Restart container
Write-Host "`nStep 3: Restarting container..." -ForegroundColor Yellow

$restartCommand = "sudo docker-compose -f $composeFile restart $containerName"
ssh -i $sshKeyPath "$sshUser@$dockerHost" $restartCommand

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Container restarted successfully" -ForegroundColor Green
} else {
    Write-Host "✗ Container restart failed" -ForegroundColor Red
    exit 1
}

# Step 4: Verify deployment
Write-Host "`nStep 4: Verifying deployment..." -ForegroundColor Yellow

$verifyCommand = "sudo docker exec $containerName ls -la /usr/share/zabbix/widgets/multigraph/"
ssh -i $sshKeyPath "$sshUser@$dockerHost" $verifyCommand

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Deployment verified" -ForegroundColor Green
} else {
    Write-Host "✗ Verification failed" -ForegroundColor Red
}

# Display next steps
Write-Host "`n=== Deployment Complete ===" -ForegroundColor Green
Write-Host "Next steps:"
Write-Host "1. Open Zabbix UI: http://$dockerHost`:8080"
Write-Host "2. Navigate to Administration → General → Modules"
Write-Host "3. Click 'Scan directory'"
Write-Host "4. Enable 'Multigraph' widget"
```

**Usage:**

```powershell
# Make script executable (PowerShell Core on Linux)
chmod +x deploy-multigraph.ps1

# Run script (Windows PowerShell)
.\deploy-multigraph.ps1

# Run with execution policy bypass (if needed)
powershell -ExecutionPolicy Bypass -File .\deploy-multigraph.ps1
```

**Alternative: PowerShell Functions (Manual Steps):**

```powershell
# Function to copy files to Docker host
function Copy-WidgetToDockerHost {
    param(
        [string]$Host = "192.168.254.16",
        [string]$User = "aibot",
        [string]$KeyPath = ".\ai_priv\id_aibot",
        [string]$DestPath = "/root/customzabbix/usr/share/zabbix/widgets/multigraph"
    )
    
    Write-Host "Copying widget files..." -ForegroundColor Yellow
    
    # Create remote directory
    $createDirCmd = "mkdir -p $DestPath"
    ssh -i $KeyPath "$User@$Host" $createDirCmd
    
    # Copy files
    scp -i $KeyPath -r .\src\* "$User@$Host`:$DestPath/"
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Files copied successfully" -ForegroundColor Green
        return $true
    } else {
        Write-Host "✗ Copy failed" -ForegroundColor Red
        return $false
    }
}

# Function to build Docker image
function Build-ZabbixImage {
    param(
        [string]$Host = "192.168.254.16",
        [string]$User = "aibot",
        [string]$KeyPath = ".\ai_priv\id_aibot",
        [string]$BuildDir = "/root/customzabbix",
        [string]$ImageName = "zabbix-web-custom",
        [string]$Tag = "latest"
    )
    
    Write-Host "Building Docker image..." -ForegroundColor Yellow
    
    $buildCmd = "cd $BuildDir && sudo docker build -t $ImageName`:$Tag ."
    ssh -i $KeyPath "$User@$Host" $buildCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Build successful" -ForegroundColor Green
        return $true
    } else {
        Write-Host "✗ Build failed" -ForegroundColor Red
        return $false
    }
}

# Function to restart container
function Restart-ZabbixContainer {
    param(
        [string]$Host = "192.168.254.16",
        [string]$User = "aibot",
        [string]$KeyPath = ".\ai_priv\id_aibot",
        [string]$ContainerName = "zabbix-web",
        [string]$ComposeFile = "/root/docker-compose.yml"
    )
    
    Write-Host "Restarting container..." -ForegroundColor Yellow
    
    $restartCmd = "sudo docker-compose -f $ComposeFile restart $ContainerName"
    ssh -i $KeyPath "$User@$Host" $restartCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Container restarted" -ForegroundColor Green
        return $true
    } else {
        Write-Host "✗ Restart failed" -ForegroundColor Red
        return $false
    }
}

# Function to verify deployment
function Test-WidgetDeployment {
    param(
        [string]$Host = "192.168.254.16",
        [string]$User = "aibot",
        [string]$KeyPath = ".\ai_priv\id_aibot",
        [string]$ContainerName = "zabbix-web"
    )
    
    Write-Host "Verifying deployment..." -ForegroundColor Yellow
    
    $verifyCmd = "sudo docker exec $ContainerName ls -la /usr/share/zabbix/widgets/multigraph/"
    ssh -i $KeyPath "$User@$Host" $verifyCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Verification successful" -ForegroundColor Green
        return $true
    } else {
        Write-Host "✗ Verification failed" -ForegroundColor Red
        return $false
    }
}

# Complete deployment workflow using functions
function Deploy-MultigraphWidget {
    param(
        [string]$Host = "192.168.254.16",
        [string]$User = "aibot",
        [string]$KeyPath = ".\ai_priv\id_aibot"
    )
    
    Write-Host "=== Starting Deployment ===" -ForegroundColor Green
    
    # Step 1: Copy files
    if (-not (Copy-WidgetToDockerHost -Host $Host -User $User -KeyPath $KeyPath)) {
        Write-Host "Deployment aborted: File copy failed" -ForegroundColor Red
        return $false
    }
    
    # Step 2: Build image
    if (-not (Build-ZabbixImage -Host $Host -User $User -KeyPath $KeyPath)) {
        Write-Host "Deployment aborted: Build failed" -ForegroundColor Red
        return $false
    }
    
    # Step 3: Restart container
    if (-not (Restart-ZabbixContainer -Host $Host -User $User -KeyPath $KeyPath)) {
        Write-Host "Deployment aborted: Restart failed" -ForegroundColor Red
        return $false
    }
    
    # Step 4: Verify
    Test-WidgetDeployment -Host $Host -User $User -KeyPath $KeyPath
    
    Write-Host "`n=== Deployment Complete ===" -ForegroundColor Green
    Write-Host "Remember to scan modules in Zabbix UI!"
    return $true
}

# Usage: Run individual functions or complete deployment
# Deploy-MultigraphWidget
# Or step by step:
# Copy-WidgetToDockerHost
# Build-ZabbixImage
# Restart-ZabbixContainer
# Test-WidgetDeployment
```

**SSH Key Setup (if not already configured):**

```powershell
# Generate SSH key pair (one-time setup)
ssh-keygen -t rsa -b 4096 -f .\ai_priv\id_aibot -N '""'

# Copy public key to Docker host
type .\ai_priv\id_aibot.pub | ssh user@dockerhost "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"

# Test connection
ssh -i .\ai_priv\id_aibot aibot@192.168.254.16 "echo 'Connection successful'"
```

**Troubleshooting PowerShell Deployment:**

**Issue: "ssh/scp not recognized"**
```powershell
# Install OpenSSH client (Windows 10+)
Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0

# Or use full path
& "C:\Windows\System32\OpenSSH\ssh.exe" -i $keyPath "$user@$host" "command"

# Or install Git for Windows (includes SSH)
# Download from: https://git-scm.com/download/win
```

**Issue: "Permission denied (publickey)"**
```powershell
# Verify key permissions
icacls .\ai_priv\id_aibot

# Set correct permissions (owner only)
icacls .\ai_priv\id_aibot /inheritance:r
icacls .\ai_priv\id_aibot /grant:r "$env:USERNAME`:F"

# Test connection
ssh -i .\ai_priv\id_aibot -v aibot@192.168.254.16
```

**Issue: "scp: ambiguous target"**
```powershell
# Use backtick to escape colon in PowerShell
scp -i $keyPath -r .\src\* "$user@$host`:$destPath/"

# Or use quotes differently
scp -i $keyPath -r .\src\* "${user}@${host}:${destPath}/"
```

### Important Notes

**No Compilation Required:**
- PHP is interpreted, not compiled
- JavaScript is interpreted by browser
- CSS is interpreted by browser
- Deploy source files directly

**Version Control:**
- All changes tracked in Git
- Use feature branches for development
- Main branch: `tmp0.1` (development)
- Production branch: `main` (planned for v1.0.0)

**Testing:**
- No automated tests yet (Phase C)
- Manual testing required
- Test on Zabbix test instance before production

---

## Source Structure

### B.10. Source Code Organization

#### Directory Structure

```
multigraph/
├── README.md                      # This file - project documentation
├── USAGE.md                       # User and administrator manual (to be created)
├── REFERENCE.md                   # API reference (to be created)
├── LICENSE                        # MIT License
├── .gitignore                     # Git exclusions
├── ai.txt                         # AI context (generic, for AI assistants)
├── ai_priv/                       # Private context (git-ignored)
│   ├── ai_priv.txt               # Local environment, credentials, paths
│   └── id_aibot                  # SSH private key for deployment
│
├── docker/                        # Deployment helpers
│   └── modules_rescan.php        # HTTP-accessible module rescan helper
│
├── zabbix-helpers.ps1            # PowerShell deployment functions
│
├── deprecated/                    # Old code (git-ignored, for reference)
│   ├── tmptest/                  # API rescan experiments
│   │   ├── api_login.json
│   │   ├── api_modules.json
│   │   ├── api_request.json
│   │   ├── api_test.php
│   │   ├── query_modules.php
│   │   └── scan_modules.php
│   └── oldmodule/                # Old module-based approach
│       ├── src/Module.php
│       ├── src/actions/Test.php
│       ├── src/views/multigraph.test.php
│       └── src/manifest.json     # Old module manifest
│
└── src/                           # Widget source (Zabbix-compatible structure)
    ├── manifest.json              # Widget manifest - defines widget metadata
    ├── Widget.php                 # Entry point - widget registration
    ├── README.md                  # Source structure documentation
    │
    ├── actions/                   # MVC Controllers (Zabbix convention)
    │   ├── README.md              # Controllers documentation
    │   ├── WidgetView.php         # Display controller
    │   └── WidgetEdit.php         # Configuration controller
    │
    ├── includes/                  # MVC Models + Forms (Zabbix convention)
    │   ├── README.md              # Models documentation
    │   ├── GraphData.php          # Graph data preparation model
    │   ├── MatchedItemsData.php   # Item pattern matching model
    │   └── WidgetForm.php         # Configuration form definition
    │
    ├── views/                     # MVC Views / Templates (Zabbix convention)
    │   ├── README.md              # Views documentation
    │   ├── widget.view.php        # Display template (graph canvas)
    │   ├── widget.edit.php        # Configuration form template
    │   └── widget.edit.js.php     # Configuration JavaScript template
    │
    └── assets/                    # Frontend resources
        ├── js/
        │   ├── README.md          # JavaScript documentation
        │   ├── class.widget.js    # Main widget JavaScript class
        │   └── widget.edit.js     # Configuration form JavaScript
        └── css/
            ├── README.md          # CSS documentation
            └── multigraph.css     # Widget styles
```

#### File Purposes and Relationships

**Entry Point and Registration:**

- **`src/manifest.json`**
  - Defines widget metadata (name, namespace, version)
  - Declares required JS/CSS files
  - Loaded by Zabbix module scanner
  
- **`src/Widget.php`**
  - Extends `CWidget` (Zabbix base class)
  - Returns widget default name
  - Entry point for widget lifecycle
  - **Relationships:** Used by Zabbix framework, references actions

**Controllers (actions/):**

- **`src/actions/WidgetView.php`**
  - **Purpose:** Handle widget display requests
  - **Key Methods:**
    - `init()` - Set validation rules for input
    - `doAction()` - Main controller logic:
      1. Parse timeframe (`parseZabbixTime()`)
      2. Find matching items (`MatchedItemsData::findItems()`)
      3. Fetch history data (`GraphData::fetchHistoryData()`)
      4. Prepare graph data (`GraphData::prepareGraphData()`)
      5. Pass to view (`views/widget.view.php`)
  - **Dependencies:** 
    - Calls: `MatchedItemsData`, `GraphData`
    - Renders: `views/widget.view.php`
    - Called by: Zabbix MVC framework

- **`src/actions/WidgetEdit.php`**
  - **Purpose:** Handle widget configuration form requests
  - **Key Methods:**
    - `doAction()` - Create form, prepare fields, render view
  - **Dependencies:**
    - Uses: `WidgetForm`
    - Renders: `views/widget.edit.php`
    - Called by: Zabbix MVC framework

**Models (includes/):**

- **`src/includes/MatchedItemsData.php`**
  - **Purpose:** Match items by pattern (wildcard or regex)
  - **Key Methods:**
    - `findItems($hostids, $pattern, $mode)` - Find items matching pattern
  - **Algorithm:**
    - Convert wildcard to regex if needed
    - Call Zabbix `API::Item()::get()`
    - Filter by name pattern
    - Return matched items
  - **Dependencies:**
    - Uses: Zabbix API
    - Used by: `WidgetView.php`

- **`src/includes/GraphData.php`**
  - **Purpose:** Fetch and prepare graph data
  - **Key Methods:**
    - `fetchHistoryData($items, $time_from, $time_till)` - Fetch history
    - `prepareGraphData($items, $history, $config)` - Prepare for JS
  - **Algorithm:**
    - Determine value type per item
    - Call Zabbix `API::History()::get()`
    - Format as `[[timestamp_ms, value], ...]`
    - Apply color configuration
  - **Dependencies:**
    - Uses: Zabbix API
    - Used by: `WidgetView.php`

- **`src/includes/WidgetForm.php`**
  - **Purpose:** Define configuration form fields
  - **Key Fields:**
    - `hostids` - Multi-select host picker
    - `item_pattern` - Text pattern
    - `pattern_mode` - Wildcard (0) or Regex (1)
    - `time_period` - Time range selector
    - `show_legend`, `legend_position`, `graph_colors`, etc.
  - **Validation:**
    - Required fields checked
    - Pattern mode validated
    - Color format validated
  - **Dependencies:**
    - Extends: `CWidgetForm` (Zabbix)
    - Used by: `WidgetEdit.php`

**Views (views/):**

- **`src/views/widget.view.php`**
  - **Purpose:** Render graph display (HTML5 Canvas)
  - **Output:**
    - Creates two canvas elements (base + overlay)
    - Passes `graph_data` to JavaScript
  - **Error Handling:**
    - Shows error message if data fetch failed
    - Shows "No data" if no series available
  - **Dependencies:**
    - Receives data from: `WidgetView.php`
    - Loads: `class.widget.js`

- **`src/views/widget.edit.php`**
  - **Purpose:** Render configuration form
  - **Output:**
    - Form fields using Zabbix field view classes
    - Conditionally includes optional fields
  - **Dependencies:**
    - Receives data from: `WidgetEdit.php`
    - Loads: `widget.edit.js`

- **`src/views/widget.edit.js.php`**
  - **Purpose:** JavaScript initialization for config form
  - **Output:**
    - Minimal `window.widget_form` class
    - Actual logic in `widget.edit.js`
  - **Dependencies:**
    - Loaded by: Zabbix framework
    - Extended by: `widget.edit.js`

**JavaScript (assets/js/):**

- **`src/assets/js/class.widget.js`**
  - **Purpose:** Main widget client-side logic
  - **Key Class:** `CWidgetMultigraph extends CWidget`
  - **Key Methods:**
    - `onInitialize()` - Set up instance variables
    - `setContents(response)` - Receive data from server
    - `renderGraph()` - Main rendering pipeline (13 phases):
      1. DOM setup
      2. Layout calculation
      3. Data range discovery
      4. Y-axis overrides
      5. Clear canvas
      6. Draw grid
      7. Draw axes
      8. Y-axis labels
      9. X-axis labels (adaptive formatting)
      10. Draw stacked fills
      11. Draw line series
      12. Draw legend
      13. Setup hover tooltips
    - `onResize()` - Redraw on widget resize
    - `onFeedback()` - Handle dashboard updates
    - `_getValueAtTime()` - Interpolate values for hover
  - **Dependencies:**
    - Extends: `CWidget` (Zabbix)
    - Receives: `graph_data` from `widget.view.php`
    - Uses: HTML5 Canvas API

- **`src/assets/js/widget.edit.js`**
  - **Purpose:** Configuration form interactions
  - **Key Class:** `window.widget_form extends CWidgetForm`
  - **Key Methods:**
    - `init()` - Bind form fields
    - `openItemSelector()` - Show item picker dialog
    - `showItemList(hostid)` - Fetch items via AJAX
    - `displayItemSelector(items)` - Render item list
    - `itemNameToPattern(name, is_regex)` - Generate pattern
  - **Dependencies:**
    - Extends: `CWidgetForm` (Zabbix)
    - Uses: Zabbix `overlayDialogue`, `Curl`, jQuery

**CSS (assets/css/):**

- **`src/assets/css/multigraph.css`**
  - **Purpose:** Widget styling
  - **Key Classes:**
    - `.multigraph-widget` - Main container
    - `.multigraph-container` - Canvas container
    - `.multigraph-tooltip` - Hover tooltip
  - **Strategy:**
    - CSS custom properties for theme integration
    - Layered canvas (base + overlay)
    - Absolute positioning for tooltip

#### Component Relationships

```
Zabbix Framework
    │
    ├──[routes to]──► Widget.php
    │                     │
    │                     ├──[view request]──► WidgetView.php
    │                     │                         │
    │                     │                         ├──[calls]──► MatchedItemsData::findItems()
    │                     │                         │                   │
    │                     │                         │                   └──[uses]──► Zabbix API::Item
    │                     │                         │
    │                     │                         ├──[calls]──► GraphData::fetchHistoryData()
    │                     │                         │                   │
    │                     │                         │                   └──[uses]──► Zabbix API::History
    │                     │                         │
    │                     │                         ├──[calls]──► GraphData::prepareGraphData()
    │                     │                         │
    │                     │                         └──[renders]──► widget.view.php
    │                     │                                             │
    │                     │                                             └──[loads]──► class.widget.js
    │                     │                                                               │
    │                     │                                                               └──[renders]──► Canvas
    │                     │
    │                     └──[edit request]──► WidgetEdit.php
    │                                               │
    │                                               ├──[uses]──► WidgetForm
    │                                               │
    │                                               └──[renders]──► widget.edit.php
    │                                                                   │
    │                                                                   └──[loads]──► widget.edit.js
```

---

## Documentation

**Main Documentation:**
- **README.md** (this file) - Project overview, architecture, requirements
- **USAGE.md** (to be created) - User and administrator manual
- **REFERENCE.md** (to be created) - API reference, configuration reference

**Source Documentation:**
- **`src/README.md`** - Source structure overview
- **`src/actions/README.md`** - Controllers documentation
- **`src/includes/README.md`** - Models documentation
- **`src/views/README.md`** - Views documentation
- **`src/assets/js/README.md`** - JavaScript documentation
- **`src/assets/css/README.md`** - CSS documentation

**Code Documentation:**
- All PHP files have comprehensive PHPDoc headers and method documentation
- All JavaScript files have comprehensive JSDoc headers and method documentation
- All CSS files have inline comments explaining strategies

**AI Context:**
- **`ai.txt`** - Generic AI context (architecture, requirements, constraints)
- **`ai_priv/ai_priv.txt`** - Local environment context (paths, credentials)

---

## Contributing

This project is currently in active development by lukas@plachy.eu with AI assistance. Contributions will be accepted after v1.0.0 release.

---

## License

MIT License - See LICENSE file for details

---

**Last Updated:** November 10, 2025  
**Current Version:** 0.1.0 (Development)  
**Next Milestone:** Phase C - Testing and Quality
