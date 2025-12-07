# Zabbix API and Environment Initialization Patterns

Based on analysis of the multigraph project, here are the key patterns for working with Zabbix APIs and environment initialization.

## 1. Environment Initialization Patterns

### Pattern A: Standalone PHP Script (External)
**File:** `multigraph/deprecated/tmptest/scan_modules.php`

```php
#!/usr/bin/php
<?php
define('ZBX_SESSION_NAME', '');
require_once '/usr/share/zabbix/include/config.inc.php';
require_once '/usr/share/zabbix/include/classes/core/APP.php';

APP::getInstance()->run(APP::EXEC_MODE_DEFAULT);

// Now you can use API::* classes
$db_modules = API::Module()->get([
    'output' => ['id', 'relative_path'],
    'sortfield' => 'relative_path',
    'preservekeys' => true
]);
```

**Key Points:**
- Used for scripts that run outside of Zabbix request cycle
- Must initialize APP instance with `run(APP::EXEC_MODE_DEFAULT)`
- Requires session name definition
- Full access to `API::*` classes after initialization

### Pattern B: Web-Accessible Helper Script (Minimal)
**File:** `multigraph/docker/modules_rescan.php`

```php
<?php
// Simple authentication
define('RESCAN_TOKEN', 'd52aecac75ee7af2dd2a31b1725b423ce68027db0567303f859a78b750cf0d83');

if (!isset($_GET['token']) || $_GET['token'] !== RESCAN_TOKEN) {
    http_response_code(403);
    header('Content-Type: application/json');
    die(json_encode(['error' => 'Unauthorized']));
}

header('Content-Type: application/json');

// Direct file system operations - NO Zabbix API
$manifest_file = '/usr/share/zabbix/modules/module_name/manifest.json';
if (file_exists($manifest_file)) {
    $manifest = json_decode(file_get_contents($manifest_file), true);
    // Process manifest...
}
```

**Key Points:**
- Minimal PHP without Zabbix framework
- Token-based authentication
- Used for simple filesystem operations
- NO access to API classes

### Pattern C: Widget Action Controller (Integrated)
**File:** `multigraph/src/actions/ItemList.php`

```php
<?php declare(strict_types = 0);
namespace Widgets\Multigraph\Actions;

use CController;
use CControllerResponseData;
use API;

class ItemList extends CController {
    protected function init(): void {
        $this->disableCsrfValidation(); // For AJAX endpoints
    }

    protected function checkPermissions(): bool {
        return $this->getUserType() >= USER_TYPE_ZABBIX_USER;
    }

    protected function checkInput(): bool {
        $fields = [
            'hostid' => 'db hosts.hostid',
            'templateid' => 'db hosts.hostid'
        ];
        return $this->validateInput($fields);
    }

    protected function doAction(): void {
        // API is already initialized by Zabbix framework
        $items = API::Item()->get([
            'output' => ['itemid', 'name', 'key_'],
            'hostids' => $this->getInput('hostid'),
            'sortfield' => 'name',
            'limit' => 1000
        ]);
        
        header('Content-Type: application/json');
        echo json_encode(['items' => $items]);
        exit;
    }
}
```

**Key Points:**
- Extends `CController` - part of Zabbix request cycle
- Framework handles API initialization
- Must implement: `init()`, `checkPermissions()`, `checkInput()`, `doAction()`
- Direct access to `API::*` classes
- Built-in input validation and permission checking

### Pattern D: Module with Menu Integration
**File:** `multigraph/deprecated/oldmodule/Module.php`

```php
<?php declare(strict_types = 1);
namespace Modules\ModuleName;

use APP;
use CController as CAction;
use CMenu;
use CMenuItem;
use Zabbix\Core\CModule;

class Module extends CModule {
    public function init(): void {
        // Add menu entry
        /** @var CMenu $menu */
        $menu = APP::Component()->get('menu.main');
        $menu
            ->findOrAdd(_('Configuration'))  // Top-level menu
            ->getSubMenu()
            ->add(
                (new CMenuItem(_('Macro List Editor')))
                    ->setAction('listedit.editor')  // Links to action
            );
    }

    public function onBeforeAction(CAction $action): void {
        // Called before any action in this module
    }

    public function onTerminate(CAction $action): void {
        // Called after action completes
    }
}
```

**Key Points:**
- Extends `CModule` - module lifecycle integration
- `init()` - called when module loads (add menu items here)
- Menu path: `findOrAdd(_('Configuration'))` for top-level section
- Action naming: `modulename.actionname` (e.g., `listedit.editor`)
- Module must be registered via UI: Administration → Modules → "Scan directory"

## 2. JSON RPC API Patterns

### Pattern E: External cURL Request
**File:** `multigraph/deprecated/tmptest/api_test.php`

```php
<?php
$ch = curl_init("http://localhost:8080/api_jsonrpc.php");
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_POST, true);
curl_setopt($ch, CURLOPT_HTTPHEADER, ["Content-Type: application/json-rpc"]);
curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode([
    "jsonrpc" => "2.0",
    "method" => "apiinfo.version",
    "params" => [],
    "id" => 1
]));
$result = curl_exec($ch);
curl_close($ch);
echo $result . "\n";
```

**Key Points:**
- Used for external API access (outside Zabbix PHP)
- Content-Type: `application/json-rpc`
- Standard JSON-RPC 2.0 format
- Requires authentication for most methods

### Pattern F: Internal API Usage (Widget/Module)
```php
// Inside a CController action or after APP initialization
$hosts = API::Host()->get([
    'output' => ['hostid', 'host', 'name'],
    'filter' => ['status' => HOST_STATUS_MONITORED],
    'sortfield' => 'name'
]);

$macros = API::UserMacro()->get([
    'output' => ['hostmacroid', 'macro', 'value', 'type'],
    'hostids' => $hostid,
    'filter' => ['macro' => '{$MYLIST_LIST}']  // Specific macro search
]);
```

**Key Points:**
- No curl/HTTP needed - direct PHP calls
- `API::<Class>()->get()` / `create()` / `update()` / `delete()`
- Array-based query DSL
- Common parameters: `output`, `filter`, `sortfield`, `limit`

## 3. Common API Classes for Macro Editor

```php
// Fetch hosts
API::Host()->get([
    'output' => ['hostid', 'host', 'name'],
    'filter' => ['status' => HOST_STATUS_MONITORED]
]);

// Fetch templates
API::Template()->get([
    'output' => ['templateid', 'host', 'name'],
    'sortfield' => 'name'
]);

// Fetch user macros from host/template
API::UserMacro()->get([
    'output' => ['hostmacroid', 'macro', 'value', 'type', 'description'],
    'hostids' => $hostid,  // or 'templateids' => $templateid
    'sortfield' => 'macro'
]);

// Fetch global macros
API::UserMacro()->get([
    'output' => ['globalmacroid', 'macro', 'value', 'type', 'description'],
    'globalmacro' => true
]);

// Update macro value
API::UserMacro()->update([
    'hostmacroid' => $macroid,
    'value' => $new_value
]);
```

## 4. Project Structure for Macro List Editor Module

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
│   └── editor.js.php     # Editor JavaScript
└── assets/
    ├── css/
    │   └── listedit.css
    └── js/
        └── listedit.js
```

## 5. Macro Type Constants

From `include/defines.inc.php`:
```php
define('ZBX_MACRO_TYPE_TEXT', 0);    // Plain text
define('ZBX_MACRO_TYPE_SECRET', 1);  // Secret text (hidden)
define('ZBX_MACRO_TYPE_VAULT', 2);   // Vault secret
```

## 6. Manifest.json Example

```json
{
    "manifest_version": "2.0",
    "id": "listedit",
    "name": "Macro List Editor",
    "version": "0.1.0",
    "namespace": "Modules\\ListEdit",
    "author": "Your Name",
    "description": "Edit list-type template macros ending with _LIST}",
    "actions": [
        {"name": "listedit.editor", "class": "Editor"},
        {"name": "listedit.macrolist", "class": "MacroList"},
        {"name": "listedit.macroupdate", "class": "MacroUpdate"},
        {"name": "listedit.hostlist", "class": "HostList"}
    ]
}
```

## 7. Recommended Implementation Strategy

### Phase 1: Basic Module with Menu
1. Create manifest.json
2. Create Module.php with menu integration
3. Create Editor.php action (basic page)
4. Deploy and verify menu appears

### Phase 2: Host/Template Selection
1. Create HostList.php AJAX action
2. Add host/template dropdown to editor view
3. Test host selection

### Phase 3: Macro Fetching
1. Create MacroList.php AJAX action
2. Filter macros ending with `_LIST}`
3. Display macro list in UI

### Phase 4: List Editor UI
1. Parse pipe-separated format: `{#H1}|{#H2},v1|v2,v3|v4`
2. Build editable table UI
3. Add/remove/reorder rows

### Phase 5: Macro Update
1. Create MacroUpdate.php AJAX action
2. Serialize table back to pipe-separated format
3. Call `API::UserMacro()->update()`

## 8. Authentication & Permissions

All module actions inherit Zabbix session authentication:
- No separate login needed
- Use `checkPermissions()` to enforce role requirements
- `USER_TYPE_ZABBIX_USER` - read-only access
- `USER_TYPE_ZABBIX_ADMIN` - admin access
- `USER_TYPE_SUPER_ADMIN` - super admin

Example:
```php
protected function checkPermissions(): bool {
    // Require admin to edit macros
    return $this->getUserType() >= USER_TYPE_ZABBIX_ADMIN;
}
```

## 9. Deployment Workflow

```bash
# 1. Local development
cd c:\Users\plachy\Documents\Dev\Cpp\zabbix\listedit

# 2. Copy to TrueNAS host
scp -i ai_priv/id_aibot -r src/* aibot@192.168.254.16:/tmp/listedit_deploy/

# 3. SSH to TrueNAS and copy into container
ssh -i ai_priv/id_aibot aibot@192.168.254.16
sudo docker cp /tmp/listedit_deploy/. ix-zabbix-web-zabbix-web-1:/usr/share/zabbix/modules/listedit/
sudo docker exec ix-zabbix-web-zabbix-web-1 chown -R www-data:www-data /usr/share/zabbix/modules/listedit
sudo docker exec ix-zabbix-web-zabbix-web-1 chmod -R 755 /usr/share/zabbix/modules/listedit

# 4. Register in Zabbix UI
# Navigate to: Administration → Modules → "Scan directory"
```

## 10. Key Differences: Widget vs Module

| Aspect | Widget | Module |
|--------|--------|--------|
| Location | `/usr/share/zabbix/widgets/` | `/usr/share/zabbix/modules/` |
| Purpose | Dashboard visualization | Menu items, actions, pages |
| Base Class | `CWidget` | `CModule` |
| Namespace | `Widgets\WidgetName` | `Modules\ModuleName` |
| Menu Access | No | Yes (via `init()`) |
| Actions | Widget-specific | Custom pages |
| Use Case | Charts, graphs, monitoring | Configuration, tools, editors |

For macro list editor: **USE MODULE** (not widget)
