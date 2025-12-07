# Zabbix Macro Value Editor Type Extension Research

**Date:** December 2, 2025  
**Project:** ListEdit Widget  
**Goal:** Register custom template macro value editor type  

---

## 1. Current Zabbix Macro System Architecture

### 1.1 Built-in Macro Types

Zabbix 7.4.3 has three built-in macro value types defined in `/usr/share/zabbix/include/defines.inc.php`:

```php
define('ZBX_MACRO_TYPE_TEXT', 0);    // Display macro value as text
define('ZBX_MACRO_TYPE_SECRET', 1);  // Display masked macro value
define('ZBX_MACRO_TYPE_VAULT', 2);   // Display macro value as text (path to secret in Vault)
```

### 1.2 CMacroValue Class

Location: `/usr/share/zabbix/include/classes/html/CMacroValue.php`

**Key Features:**
- Extends `CDiv` (HTML div container)
- Handles rendering of macro input fields
- Supports dropdown type selector with icons
- Implements JavaScript initialization via `jQuery().macroValue()`

**Constructor:**
```php
public function __construct(int $type, string $name, ?string $value = null, bool $add_post_js = true)
```

**Rendering Logic:**
- `ZBX_MACRO_TYPE_TEXT` → `CTextAreaFlexible` with text icon
- `ZBX_MACRO_TYPE_SECRET` → `CInputSecret` with eye-off icon
- `ZBX_MACRO_TYPE_VAULT` → `CTextAreaFlexible` with lock icon

**Dropdown Configuration:**
```php
(new CButtonDropdown($this->name.'[type]',  $this->type, [
    ['label' => _('Text'), 'value' => ZBX_MACRO_TYPE_TEXT, 'class' => ZBX_ICON_TEXT],
    ['label' => _('Secret text'), 'value' => ZBX_MACRO_TYPE_SECRET, 'class' => ZBX_ICON_EYE_OFF],
    ['label' => _('Vault secret'), 'value' => ZBX_MACRO_TYPE_VAULT, 'class' => ZBX_ICON_LOCK]
]))
```

---

## 2. Extension Options Analysis

### Option 1: Widget-Based Custom Macro Editor (RECOMMENDED)

**Approach:** Create a dashboard widget that provides a custom interface for editing specific macro types.

**Pros:**
- ✅ Fully supported extension mechanism in Zabbix
- ✅ No core file modifications required
- ✅ Can be deployed via standard widget deployment
- ✅ Isolated from Zabbix updates
- ✅ Can use AJAX endpoints for data fetching
- ✅ Full control over UI/UX

**Cons:**
- ❌ Not integrated into native macro editing screens
- ❌ Users must add widget to dashboard
- ❌ Separate from Configuration → Templates → Macros interface

**Implementation:**
1. Create widget in `/usr/share/zabbix/widgets/listedit/`
2. Widget fetches host/template macros via Zabbix API
3. Custom UI for editing macro values (e.g., list editor, JSON editor, etc.)
4. Save changes back via `API::UserMacro()->update()`

**Best For:**
- Dashboard-based macro management
- Custom macro value formats (lists, JSON, structured data)
- Workflow-specific macro editing

---

### Option 2: PHP Module Extension (NOT DIRECTLY SUPPORTED)

**Approach:** Create a Zabbix module that extends CMacroValue class or hooks into macro rendering.

**Pros:**
- ✅ Could potentially integrate into native UI
- ✅ Module system exists for frontend extensions

**Cons:**
- ❌ **No documented hook system for CMacroValue extension**
- ❌ Zabbix modules don't have formal extension points for HTML classes
- ❌ Would require core class modification
- ❌ High risk of breaking on Zabbix updates
- ❌ Not officially supported

**Current Module Capabilities:**
- Menu items (Administration → General → Modules)
- Custom pages/views
- Helper functions
- **NOT:** Core HTML class extensions

**Conclusion:** Not feasible without core modifications.

---

### Option 3: Core File Modification (NOT RECOMMENDED)

**Approach:** Directly modify `/usr/share/zabbix/include/classes/html/CMacroValue.php` and related files.

**Pros:**
- ✅ Complete integration into native macro UI
- ✅ Direct control over rendering

**Cons:**
- ❌ **LOST ON EVERY ZABBIX UPDATE**
- ❌ Violates best practices
- ❌ No upgrade path
- ❌ Maintenance nightmare
- ❌ Security/support issues

**What Would Be Required:**
1. Add new `ZBX_MACRO_TYPE_LIST` constant to `defines.inc.php`
2. Modify `CMacroValue::toString()` to handle new type
3. Add dropdown option for new type
4. Create custom input component (e.g., `CListEditor`)
5. Update database schema to support new type value
6. Modify validation logic in controllers

**Conclusion:** Technically possible but **strongly discouraged**.

---

### Option 4: JavaScript UI Override (HACKY)

**Approach:** Inject JavaScript to modify macro editor UI after page load.

**Pros:**
- ✅ No PHP modifications
- ✅ Can be deployed via widget or userscript

**Cons:**
- ❌ Fragile (breaks if Zabbix changes UI)
- ❌ Race conditions with page load
- ❌ Server-side validation issues
- ❌ Difficult to maintain

**Implementation Example:**
```javascript
// Inject custom macro type into dropdown
jQuery(document).ready(function() {
    jQuery('.macro-input-group').each(function() {
        // Find dropdown, add custom option
        // Replace input with custom editor
    });
});
```

**Conclusion:** Unreliable, not recommended for production.

---

### Option 5: Hybrid Widget + API Approach (PRACTICAL ALTERNATIVE)

**Approach:** Widget that links to native macro screen with pre-filled values.

**Pros:**
- ✅ Widget provides custom editor UI
- ✅ Users can still access native macro screen
- ✅ Bidirectional sync via API
- ✅ No core modifications

**Cons:**
- ❌ Dual interface (widget + native screen)
- ❌ Need to keep both in sync

**Implementation:**
1. Widget provides custom list/JSON/structured editor
2. Widget reads macros via `API::UserMacro()->get()`
3. Widget saves as `ZBX_MACRO_TYPE_TEXT` with structured format
4. Optional: Add "Edit in Configuration" link to native screen
5. Validate structured format on widget save

**Best For:**
- Structured macro values (JSON, lists, key-value pairs)
- Complex macro editing workflows
- When native text field is insufficient

---

## 3. Recommended Solution

### **Option 1 + Option 5 Hybrid: Dashboard Widget with API Integration**

**Implementation Plan:**

#### 3.1 Widget Structure
```
/usr/share/zabbix/widgets/listedit/
├── manifest.json          # Register widget + actions
├── Widget.php             # Entry point
├── actions/
│   ├── MacroListView.php  # Fetch macros
│   └── MacroListSave.php  # Save macro changes
├── includes/
│   └── WidgetForm.php     # Widget configuration
├── views/
│   ├── widget.edit.php    # Config form
│   └── widget.view.php    # Main editor UI
└── assets/
    ├── js/
    │   └── class.widget.js    # List editor logic
    └── css/
        └── listedit.css       # Styling
```

#### 3.2 Macro Value Format
Store structured data as JSON in `ZBX_MACRO_TYPE_TEXT`:
```
{MYMACRO} = ["value1", "value2", "value3"]
```

Widget parses/validates/edits this format with a proper UI.

#### 3.3 Key Features
- ✅ Host/Template selector dropdown
- ✅ Macro selector (filter for specific patterns)
- ✅ List editor UI (add/remove/reorder items)
- ✅ JSON validation
- ✅ Save button → `API::UserMacro()->update()`
- ✅ Auto-refresh on dashboard

#### 3.4 API Calls Required
```php
// Fetch macros
API::UserMacro()->get([
    'hostids' => $hostid,
    'templateids' => $templateid,
    'output' => ['hostmacroid', 'macro', 'value', 'type'],
    'filter' => ['macro' => '{$PATTERN*}']
]);

// Update macro
API::UserMacro()->update([
    'hostmacroid' => $macroid,
    'value' => json_encode($list_values),
    'type' => ZBX_MACRO_TYPE_TEXT
]);
```

---

## 4. Proof of Concept: List Editor Widget

### 4.1 Use Case
Edit a macro containing a JSON array of values with a user-friendly list interface:

**Before (native):**
```
{$MONITORED_IPS} = ["192.168.1.1","192.168.1.2","192.168.1.3"]
```
User must manually edit JSON string.

**After (widget):**
```
┌─────────────────────────────────────┐
│ Macro: {$MONITORED_IPS}            │
├─────────────────────────────────────┤
│ ☰ 192.168.1.1          [×] [↑] [↓] │
│ ☰ 192.168.1.2          [×] [↑] [↓] │
│ ☰ 192.168.1.3          [×] [↑] [↓] │
│ [+ Add Item]                        │
└─────────────────────────────────────┘
│ [Save Changes] [Revert]             │
└─────────────────────────────────────┘
```

### 4.2 Widget Configuration
- **Host/Template:** Dropdown selector
- **Macro Pattern:** Text input (e.g., `{$MONITORED_*}`)
- **Display Options:** Show as list, JSON, or raw

### 4.3 Technical Implementation
See `Option 1` architecture above.

---

## 5. Alternative: Custom Macro Type via Fork (Future Consideration)

If Zabbix officially supports plugin-based macro type extensions in the future, the approach would be:

### Theoretical Hook System
```php
// hypothetical future API
register_macro_type([
    'id' => 4, // ZBX_MACRO_TYPE_LIST
    'name' => 'List',
    'icon' => 'ZBX_ICON_LIST',
    'renderer' => 'CListMacroValue', // custom class
    'validator' => 'validate_list_macro'
]);
```

**Note:** This does NOT exist in Zabbix 7.4.3. Would require:
1. Zabbix core support for hook system
2. Official plugin API
3. Database schema changes

---

## 6. Conclusion

**RECOMMENDED APPROACH:**  
**Option 1 (Dashboard Widget) with Option 5 enhancements**

### Why:
1. ✅ Fully supported by Zabbix extension system
2. ✅ No core modifications
3. ✅ Survives Zabbix updates
4. ✅ Can be deployed to TrueNAS Docker container
5. ✅ Provides superior UX for structured macro values
6. ✅ Integrates with existing multigraph deployment workflow

### Next Steps:
1. Define specific macro value format (JSON array, key-value, etc.)
2. Design widget UI mockup
3. Implement widget structure (manifest, Widget.php, actions)
4. Create list editor frontend (JavaScript + HTML)
5. Implement API integration (fetch/save macros)
6. Test deployment to `ix-zabbix-web-zabbix-web-1` container

---

## 7. References

- **CMacroValue Class:** `/usr/share/zabbix/include/classes/html/CMacroValue.php`
- **Macro Constants:** `/usr/share/zabbix/include/defines.inc.php`
- **API Documentation:** https://www.zabbix.com/documentation/current/en/manual/api
- **Widget Development:** Based on multigraph widget structure
- **Existing Module Example:** `/usr/share/zabbix/modules/zabbix-module-sqlexplorer/`

---

**Author:** Claude Sonnet 4 (Anthropic)  
**For:** Lukas Plachy <lukas@plachy.eu>  
**Project:** Zabbix ListEdit Widget v0.1.0
