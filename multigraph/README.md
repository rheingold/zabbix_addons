# Multigraph - Zabbix Widget

```
╔═══════════════════════════════════════════════════════════════════════════╗
║                                                                           ║
║                       MULTIGRAPH WIDGET                                    ║
║                  Enhanced Zabbix Graph Visualization                      ║
║                                                                           ║
║  Version:          0.1.0 (Development)                                    ║
║  Created:          November 5, 2025                                       ║
║  Last Updated:     November 9, 2025                                       ║
║                                                                           ║
║  Lead & Architecture:  lukas@plachy.eu                                    ║
║  Development:          Claude Sonnet 4 (AI Assistant, Anthropic)          ║
║                                                                           ║
║  License:          MIT License (see LICENSE file)                         ║
║  Repository:       https://github.com/rheingold/zabbix_addons            ║
║                                                                           ║
╚═══════════════════════════════════════════════════════════════════════════╝
```

**AI-Human Collaborative Development**  
*This project represents collaborative engineering between human architecture/design and AI-assisted implementation.*

---

## Overview

**Multigraph** is a custom Zabbix widget that provides enhanced graph visualization with advanced item pattern matching capabilities. Unlike standard Zabbix graphs that require manual item selection, Multigraph allows you to use wildcard and regex patterns to dynamically match items by name.

### Key Features

- ✨ **Pattern-Based Item Selection**: Use wildcards (`*`, `?`) or regex to match items
- 📊 **Multi-Item Graphs**: Display multiple metrics in a single graph
- 🎨 **Customizable Appearance**: Control colors, legend, fills, grid, and text styling
- ⚡ **Client-Side Rendering**: Fast HTML5 Canvas-based rendering
- 🔄 **Dashboard Integration**: Full integration with Zabbix dashboard timeframe selector
- 🎯 **Interactive**: Hover to see precise values
- 📱 **Responsive**: Adapts to widget resize

---

## Directory Structure

```
multigraph/
├── README.md                      # This file - project overview
├── .gitignore                     # Git exclusions
├── ai.txt                         # AI context (generic)
├── ai_priv/                       # Private context (git-ignored)
│   ├── ai_priv.txt               # Local paths, credentials
│   └── id_aibot                  # SSH key
│
├── docker/                        # Deployment helpers
│   └── modules_rescan.php        # HTTP-accessible rescan helper
│
├── zabbix-helpers.ps1            # PowerShell deployment functions
│
├── deprecated/                    # Git-ignored old code
│   ├── tmptest/                  # API rescan experiments
│   └── oldmodule/                # Old module-based approach
│
└── src/                           # Widget source (Zabbix-compatible structure)
    ├── manifest.json              # Widget manifest
    ├── Widget.php                 # Entry point
    ├── README.md                  # Source structure documentation
    │
    ├── actions/                   # MVC Controllers
    │   ├── README.md
    │   ├── WidgetView.php         # Display controller
    │   └── WidgetEdit.php         # Configuration controller
    │
    ├── includes/                  # MVC Models + Forms
    │   ├── README.md
    │   ├── GraphData.php          # Graph data preparation
    │   ├── MatchedItemsData.php   # Item pattern matching
    │   └── WidgetForm.php         # Configuration form
    │
    ├── views/                     # MVC Views (Templates)
    │   ├── README.md
    │   ├── widget.view.php        # Display template
    │   ├── widget.edit.php        # Configuration template
    │   └── widget.edit.js.php     # Configuration JavaScript
    │
    └── assets/                    # Frontend resources
        ├── js/
        │   ├── README.md
        │   ├── class.widget.js    # Main widget JS
        │   └── widget.edit.js     # Configuration JS
        └── css/
            ├── README.md
            └── multigraph.css     # Widget styles
```

---

## Development Setup

### Prerequisites

- Zabbix 7.4.3+ with web interface
- PHP 8.4+ (Zabbix requirement)
- SSH access to Zabbix server
- Docker (if using containerized Zabbix)

### Deployment

**Manual Installation:**
1. Copy `src/` contents to `/usr/share/zabbix/widgets/multigraph/`
2. Rename `manifest-widget.json` to `manifest.json` (or use `manifest.json` directly)
3. Set permissions: `chmod -R 755 /usr/share/zabbix/widgets/multigraph`
4. In Zabbix UI: Administration → Modules → Scan directory
5. Enable the widget

**Docker Installation:**
See `ai_priv/ai_priv.txt` for automated deployment scripts.

---

## Documentation

- **`src/README.md`** - Source code structure and architecture
- **`src/*/README.md`** - Directory-specific documentation
- **Inline code comments** - Comprehensive PHPDoc and JSDoc

---

## Status

**Current Phase:** B2.1 - Widget Development (Completed)  
**Functional:** ✅ Widget rendering, item matching, configuration, dashboard integration  
**Known Issues:** Timeframe parsing edge cases with relative dates

---

**For detailed development notes, see `ai.txt` and `ai_priv/ai_priv.txt`**

### Deployment

Deployment to Docker container via SSH:
```powershell
# Copy files to Docker host
scp -r ./src/* user@dockerhost:/tmp/multigraph/

# SSH to Docker host and deploy
ssh user@dockerhost
docker cp /tmp/multigraph zabbix-web:/usr/share/zabbix/modules/
```

## Documentation

- [Zabbix Module Development](https://www.zabbix.com/documentation/current/manual/web_interface/frontend_sections/administration/general/modules)
- Plugin manifest structure
- Action routing and views

---

**Status:** Initial setup - Under development
