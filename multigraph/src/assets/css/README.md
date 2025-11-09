# Assets - CSS

**Purpose:** Widget styling and layout

**Files:**
- `multigraph.css` - Widget styles, canvas layout, theme integration

**Styling Approach:**
- Uses Zabbix CSS variables for theme compatibility (`var(--font-color)`, etc.)
- Canvas layering with z-index for proper event handling
- Responsive layout that adapts to widget resize

**Key CSS Classes:**
- `.multigraph-container` - Main widget container
- `.multigraph-error` - Error message styling
- Canvas elements - Proper stacking and pointer-events control

**Z-Index Strategy:**
- Main canvas (graph): `z-index: 1`, `pointer-events: none`
- Overlay canvas (hover): `z-index: 2`, `pointer-events: auto`
- This ensures hover events work while graph is non-interactive

**Theme Integration:**
Uses Zabbix's CSS custom properties to match the selected theme automatically.
No hardcoded colors (except fallbacks).

**Dependencies:**
- Works with: `assets/js/class.widget.js`
- Requires: HTML structure from `views/widget.view.php`
- Integrates: Zabbix theme CSS variables
