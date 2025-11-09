# Actions Directory

**Purpose:** Controllers that handle HTTP requests for widget operations

**Namespace:** `Widgets\Multigraph\Actions`

**Files:**
- `WidgetView.php` - Handles widget display/rendering requests
- `WidgetEdit.php` - Handles widget configuration form requests

**Zabbix Integration:** 
This directory name must be `actions/` to match Zabbix's PSR-4 autoloading convention. 
The namespace `Widgets\Multigraph\Actions` maps directly to this directory path.

**Dependencies:**
- Uses `includes/` classes for business logic (data fetching, form management)
- Passes data to `views/` templates for presentation
- Extends Zabbix base controllers (CControllerDashboardWidgetView, CControllerDashboardWidgetEdit)
