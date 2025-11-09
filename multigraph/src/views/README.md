# Views Directory

**Purpose:** HTML/PHP templates for widget presentation

**Files:**
- `widget.view.php` - Widget display template (dashboard view)
- `widget.edit.php` - Widget configuration form template
- `widget.edit.js.php` - Configuration form JavaScript (PHP-generated for i18n)
- `js/` - Additional view-specific JavaScript (if any)

**Template Pattern:**
These are pure presentation templates. They receive data from controllers via `$data` array
and use Zabbix's template helpers (CWidgetView, CDiv, CTag, etc.) to generate HTML.

**PHP-Generated JavaScript:**
Files ending in `.js.php` are processed by PHP before being served as JavaScript.
This allows using PHP variables, translations `_()`, and server-side configuration in client code.

**Data Flow:**
1. Controller prepares data
2. Controller calls template with `$data`
3. Template uses `setVar()` to pass data to JavaScript
4. JavaScript in `assets/js/` receives and processes data

**Dependencies:**
- Receives data from `actions/` controllers
- Uses Zabbix template classes (CWidgetView, CScriptTag, etc.)
- Works with `assets/js/class.widget.js` for client-side rendering
