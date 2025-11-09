# Source Directory Structure

**Last Updated:** November 9, 2025

This directory follows MVC (Model-View-Controller) pattern with functional taxonomy for clear separation of concerns.

## Directory Layout

```
src/
├── manifest.json              # Widget manifest (Zabbix widget configuration)
├── Widget.php                 # Widget entry point (extends CWidget)
│
├── controllers/               # MVC Layer: Request handlers
│   ├── WidgetView.php        # Handles widget display requests
│   └── WidgetEdit.php        # Handles widget configuration requests
│
├── models/                    # MVC Layer: Data/Business logic
│   ├── GraphData.php         # Graph data preparation and rendering
│   └── MatchedItemsData.php  # Item pattern matching and metadata
│
├── forms/                     # Configuration Layer
│   └── WidgetForm.php        # Widget form field definitions
│
├── views/                     # MVC Layer: Presentation templates
│   ├── widget.view.php       # Widget display template
│   ├── widget.edit.php       # Widget configuration template
│   └── widget.edit.js.php    # Edit form JavaScript (PHP-generated)
│
└── assets/                    # Frontend Resources
    ├── js/
    │   ├── class.widget.js   # Main widget JavaScript (extends CWidget)
    │   └── widget.edit.js    # Edit form JavaScript
    └── css/
        └── multigraph.css    # Widget styles
```

## Naming Conventions

### PHP Classes
- **Controllers:** `Widget[Action].php` (e.g., WidgetView, WidgetEdit)
- **Models:** `[Concept]Data.php` (e.g., GraphData, MatchedItemsData)
- **Forms:** `WidgetForm.php`

### Namespaces
- Controllers: `Widgets\Multigraph\Actions` (kept for Zabbix compatibility)
- Models: `Widgets\Multigraph\Models`
- Forms: `Widgets\Multigraph\Forms`

## Key Architectural Principles

1. **MVC Separation**
   - Controllers handle HTTP requests and orchestrate logic
   - Models contain business logic and data access
   - Views are pure presentation templates

2. **Relative Paths Only**
   - No hardcoded URLs or absolute paths
   - Portable across different Zabbix installations

3. **Namespace Organization**
   - Clear namespace hierarchy matching directory structure
   - Easy to locate files by namespace

4. **Functional Taxonomy**
   - Files grouped by their primary function
   - "Data" suffix in models clarifies purpose
   - Forms separated from generic includes

## File Responsibilities

### Controllers (`controllers/`)
- **WidgetView.php**: Fetches data, prepares graph, renders widget
- **WidgetEdit.php**: Prepares and validates configuration form

### Models (`models/`)
- **GraphData.php**: Fetches history from Zabbix API, prepares data structure for JavaScript
- **MatchedItemsData.php**: Finds items matching patterns (wildcard/regex), returns metadata

### Forms (`forms/`)
- **WidgetForm.php**: Defines all widget configuration fields, validation, defaults

### Views (`views/`)
- **widget.view.php**: HTML template for widget display
- **widget.edit.php**: HTML template for configuration form
- **widget.edit.js.php**: JavaScript for configuration form (PHP-generated for i18n)

### Assets (`assets/`)
- **js/class.widget.js**: Client-side rendering, graph drawing, event handling
- **js/widget.edit.js**: Configuration form interactivity
- **css/multigraph.css**: Widget styling, layout, theme integration

## Development Workflow

1. **Adding Configuration Field**
   - Add field definition in `forms/WidgetForm.php`
   - Update `views/widget.edit.php` if custom UI needed
   - Handle value in `controllers/WidgetView.php`
   - Use value in `models/GraphData.php` or JavaScript

2. **Adding Data Source**
   - Create method in `models/MatchedItemsData.php` or `models/GraphData.php`
   - Call from `controllers/WidgetView.php`
   - Pass to view via `setVar()` in `controllers/WidgetView.php`

3. **Adding Visual Feature**
   - Update `assets/js/class.widget.js` for rendering
   - Update `assets/css/multigraph.css` for styling
   - Pass config from PHP via `views/widget.view.php`

## Notes

- **PHP-generated JavaScript:** Files ending in `.js.php` are processed by PHP before serving, allowing use of PHP variables and i18n functions
- **Controller Namespace:** Controllers use `Actions` namespace (not `Controllers`) for Zabbix routing compatibility
- **Static Methods:** Models use static methods as they're stateless utility classes
