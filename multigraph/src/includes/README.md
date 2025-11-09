# Includes Directory

**Purpose:** Business logic, data models, and form definitions

**Namespace:** `Widgets\Multigraph\Includes`

**Files:**
- `GraphData.php` - Graph data preparation, history fetching, rendering logic
- `MatchedItemsData.php` - Item pattern matching (wildcard/regex), metadata retrieval
- `WidgetForm.php` - Widget configuration form field definitions

**Zabbix Integration:**
This directory name must be `includes/` to match Zabbix's PSR-4 autoloading convention.
The namespace `Widgets\Multigraph\Includes` maps directly to this directory path.

**Responsibilities:**
- **GraphData:** Fetches history from Zabbix API, prepares data structures for JavaScript rendering
- **MatchedItemsData:** Finds items by name patterns, returns item metadata
- **WidgetForm:** Defines all widget configuration fields, validation rules, defaults

**Dependencies:**
- Used by `actions/` controllers
- Interacts with Zabbix API (CApi, API::History(), API::Item())
- No direct dependencies on views or assets
