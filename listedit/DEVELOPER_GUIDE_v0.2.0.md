# Zabbix ListEdit Module v0.2.0 - Developer Quick Reference

## File Overview

### CSS: `src/assets/css/listedit.css` (450 lines)
```css
/* Light/Dark theme variables */
--mle-bg-primary, --mle-text-primary, --mle-button-primary, etc.

/* Main components */
.macro-list-editor { ... }
.selection-panel { ... }
.macro-selector { ... }
.macro-editor { ... }

/* Table & editing */
.list-table { ... }
.cell-editable { ... }
.row-enter, .row-exit { animation } /* smooth add/remove */

/* Dialogs */
.host-search-dialog, .host-search-overlay { ... }
.column-editor-dialog { ... }
.search-results, .host-search-item { ... }

/* Bulk editor */
.bulk-editor-panel { ... }
.bulk-editor-textarea { ... }

/* Animations */
@keyframes rowEnter { fade + slide }
@keyframes rowExit { fade + slide }
@keyframes fadeIn { dialog appearance }
```

### HTML: `src/views/editor.php` (98 lines)
```html
<div id="macro-list-editor">
  <!-- 1. Host selection -->
  <button id="open-host-search">Search & Select</button>
  
  <!-- 2. Macro dropdown -->
  <select id="macro-selector">...</select>
  
  <!-- 3. Editor (auto-populated) -->
  <div id="macro-editor-container"></div>
</div>

<!-- Dialogs (templates) -->
<div id="host-search-dialog">
  <input id="host-search-input" oninput="...handleHostSearch()">
  <div id="host-search-results"></div>
</div>

<div id="column-editor-dialog">
  <div id="column-editor-columns"></div>
</div>
```

### JavaScript: `src/views/editor.js.php` (917 lines)

#### Module Structure
```javascript
var MacroListEditor = (function() {
    var config = { userType, canEdit }
    var state = { currentHostId, currentMacroId, allMacros, ... }
    
    // 20 functions + public API
    return {
        init, loadHostList, loadMacros, selectMacro, handleHostSearch
    }
})();
```

#### Core Functions

**1. Host Selection**
```javascript
MacroListEditor.loadHostList()
  → AJAX: zabbix.php?action=listedit.hostlist
  → populateHostSearch(items)

openHostSearchDialog()
  → Show modal overlay + dialog

handleHostSearch()
  → Filter items by input value
  → populateHostSearch(filtered)

confirmHostSearch()
  → Save selection
  → MacroListEditor.loadMacros()
```

**2. Macro Selection**
```javascript
MacroListEditor.loadMacros()
  → AJAX: zabbix.php?action=listedit.macrolist
  → populateMacroSelector(macros)

MacroListEditor.selectMacro(macroId)
  → displayMacroEditor(macro)
  → renderMacroTableEditor()
  → renderBulkTextEditor()
  → attachMacroEditorEvents()
```

**3. Table Editing**
```javascript
editCell(cell)
  → Convert <td> to <input>
  → Save on blur or Enter
  → syncTableToText()

addTableRow()
  → Append <tr> to <tbody>
  → Reattach event handlers
  → syncTableToText()

removeTableRow(row)
  → Add .row-exit animation
  → setTimeout 300ms
  → row.remove()
  → syncTableToText()

openColumnEditor()
  → Show column editor dialog
  → Allow rename/remove columns

confirmColumnEditor()
  → Update table <thead>
  → Adjust <tbody> cells
  → syncTableToText()
```

**4. Format Detection & Sync**
```javascript
detectFormat(value)
  → 'pipe-separated' | 'json-array' | 'empty' | 'unknown'

parsePipeSeparated(value)
  → { headers: [...], rows: [[...], ...] }

syncTableToText(editor)
  → Read table structure
  → Generate text value
  → Update textarea

syncTextToTable(textarea, editor)
  → Parse text value
  → Rebuild table
  → Rebuild tbody
  → Reattach handlers

saveMacro(macroId, editor)
  → AJAX: zabbix.php?action=listedit.macroupdate
  → POST hostmacroid + value
  → showSuccess() or showError()
```

## Usage Examples

### Select and Edit a Macro
```javascript
// User flow:
// 1. Page loads → MacroListEditor.init() called
// 2. User clicks "Search & Select Host" → openHostSearchDialog()
// 3. User types in search box → handleHostSearch() filters results
// 4. User clicks host → confirmHostSearch() loads macros
// 5. User selects macro from dropdown → selectMacro(macroId)
// 6. User edits cells → editCell() saves on blur
// 7. User clicks Save → saveMacro() via AJAX
```

### Add a Table Row
```javascript
<button class="btn-row-add" data-macroid="123">+ Add Row</button>

// Click handler:
// → addTableRow(editor, button)
// → Table structure read from table
// → New <tr> appended with correct column count
// → Cell click handlers attached
// → syncTableToText() rebuilds textarea value
```

### Edit Bulk Text
```html
<textarea class="bulk-editor-textarea">
{#COL1}|{#COL2},value1|value2
</textarea>
<!-- on change event → syncTextToTable(textarea, editor) -->
```

## Key Design Decisions

1. **One Macro at a Time**
   - Simplifies state management
   - Prevents accidental cross-macro edits
   - Clear visual context

2. **Inline Editing**
   - Faster workflow than modal dialogs
   - Real-time table ↔ textarea sync
   - Users see changes immediately

3. **Two-Way Sync**
   - Edit table OR text editor, both stay in sync
   - Users choose preferred editing method
   - No data loss on format conflicts

4. **Dialog System**
   - Overlay + modal dialog pattern
   - Consistent positioning (fixed, centered)
   - Click-outside to close

5. **Format Auto-Detection**
   - No user selection needed
   - Supports pipe-separated and JSON
   - Unknown formats show initialize buttons

## Common Customizations

### Change Theme Colors
Edit `src/assets/css/listedit.css`:
```css
:root {
    --mle-bg-primary: #ffffff;
    --mle-button-primary: #0275d8;
    /* ... etc */
}

@media (prefers-color-scheme: dark) {
    :root {
        --mle-bg-primary: #1e1e1e;
        /* ... etc */
    }
}
```

### Add Column Validation
In `syncTextToTable()` or `syncTableToText()`:
```javascript
// Example: Maximum 5 columns
var numCols = headers.length;
if (numCols > 5) {
    showError('Maximum 5 columns allowed');
    return false;
}
```

### Change Animation Speed
Edit `src/assets/css/listedit.css`:
```css
.row-enter {
    animation: rowEnter 300ms ease-in-out; /* Change 300ms */
}
```

### Add Keyboard Shortcuts
In `attachMacroEditorEvents()`:
```javascript
jQuery(document).on('keydown', function(e) {
    if (e.ctrlKey && e.key === 'n') {
        addTableRow(editor); // Ctrl+N to add row
    }
});
```

## Troubleshooting

### Macro dropdown doesn't populate
- Check browser console for AJAX errors
- Verify `zabbix.php?action=listedit.macrolist` returns JSON
- Check host ID is passed correctly

### Bulk editor doesn't sync to table
- Check `detectFormat()` returns correct format
- Verify textarea `onchange` handler attached
- Check browser console for parse errors

### Animations don't work
- Verify CSS file loaded (Network tab)
- Check `.row-enter` and `.row-exit` classes in CSS
- Verify jQuery version supports `.animate()` or CSS animations

### Dialog doesn't close
- Check overlay click handler: `closeHostSearchDialog()`
- Verify `#host-search-overlay` element exists
- Check z-index layering in CSS

## Performance Notes

- Suitable for:
  - < 50 macros per host
  - < 100 hosts/templates in list
  - < 20 columns per macro
  - < 100 rows of data

- Bottlenecks:
  - Large AJAX responses (>100 hosts) - consider pagination
  - Many rows (100+) - consider virtual scrolling
  - Complex regex in sync functions - pre-compile patterns

## Browser Support

- Chrome/Edge: Full support (CSS variables, ES5 JS)
- Firefox: Full support
- Safari: Full support (check CSS variable syntax)
- IE11: Partial (no CSS variables, need polyfill)

---

**For detailed documentation, see CHANGELOG_v0.2.0.md**
