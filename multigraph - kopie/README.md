# Multigraph - Zabbix UI Plugin

**Version:** 0.1 (development) | **Date:** November 5, 2025

---

## Credits

```
╔════════════════════════════════════════════════════════════════════╗
║  Lead & Architecture:  lukas@plachy.eu                             ║
║  Development:          Claude Sonnet 4.5 (AI Assistant, Anthropic) ║
║  Date:                 November 5, 2025                            ║
║  Version:              0.1 (development)                           ║
╚════════════════════════════════════════════════════════════════════╝
```

*This project was developed through AI-assisted collaborative engineering.*

---

## Overview

Zabbix UI plugin for enhanced graph visualization and multi-metric display capabilities.

## Directory Structure

```
multigraph/
├── src/                    # PHP source files
│   ├── Module.php         # Main module entry point
│   ├── actions/           # Action handlers
│   ├── views/             # View templates
│   └── assets/            # CSS, JavaScript
│
├── manifest.json          # Zabbix plugin manifest
├── docker/                # Docker build files
│   ├── Dockerfile         # Custom Zabbix web image
│   └── deploy.sh          # Deployment script
│
└── README.md              # This file
```

## Development Setup

### Prerequisites

- Zabbix server with web interface
- SSH access to Docker host
- PHP 7.4+ knowledge

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
