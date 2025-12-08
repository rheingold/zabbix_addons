<?php
/**
 * ============================================================================
 * File: editor.php (v0.2.0)
 * Created: 2025-12-02
 * Updated: 2025-12-08
 *
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 *
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 *
 * PURPOSE:
 * Main editor page HTML view (v0.2.0).
 * Enhanced UI with:
 * - Host/template search dialog (replaces dropdown)
 * - Macro selector dropdown
 * - Inline row/column editing
 * - Bulk text editor panel
 * - Column editor dialog
 * - Smooth animations and theme-aware styling
 *
 * @var CView $this
 * @var array $data
 */

$this->addCssFile('modules/listedit/assets/css/listedit.css');
?>

<div id="macro-list-editor" class="macro-list-editor">
    
    <!-- Host/Template Selection Panel -->
    <div class="selection-panel">
        <h3>1. Select Host or Template</h3>
        <div id="current-host-display" style="margin-bottom: 10px; font-weight: bold;">
            Select a host or template...
        </div>
        <button type="button" id="open-host-search" class="btn-primary">🔍 Search & Select Host/Template</button>
    </div>
    
    <!-- Macro Selector Panel -->
    <div class="selection-panel" style="margin-top: 20px;">
        <h3>2. Select Macro to Edit</h3>
        <select id="macro-selector" class="macro-selector">
            <option value="">Select a macro...</option>
        </select>
    </div>
    
    <!-- Macro Editor Container -->
    <div id="macro-editor-container" style="margin-top: 30px;"></div>
    
</div>

<!-- Host/Template Search Dialog -->
<div id="host-search-overlay" class="host-search-overlay"></div>
<div id="host-search-dialog" class="host-search-dialog">
    <div class="dialog-header">
        <h3>Search & Select Host or Template</h3>
        <button type="button" id="close-host-search" class="dialog-close">✕</button>
    </div>
    <div class="dialog-body">
        <input type="text" id="host-search-input" class="search-input" 
               placeholder="Search by name or host..." 
               oninput="MacroListEditor.handleHostSearch ? MacroListEditor.handleHostSearch() : null">
        <div id="host-search-results" class="search-results"></div>
    </div>
    <div class="dialog-footer">
        <button type="button" class="btn-secondary" id="close-host-search">Cancel</button>
        <button type="button" class="btn-primary" id="host-search-confirm">Confirm</button>
    </div>
</div>

<!-- Column Editor Dialog -->
<div id="column-editor-overlay" class="host-search-overlay"></div>
<div id="column-editor-dialog" class="host-search-dialog">
    <div class="dialog-header">
        <h3>Edit Column Headers</h3>
        <button type="button" id="close-column-editor" class="dialog-close">✕</button>
    </div>
    <div class="dialog-body">
        <div id="column-editor-columns"></div>
        <button type="button" style="margin-top: 10px;">+ Add Column</button>
    </div>
    <div class="dialog-footer">
        <button type="button" class="btn-secondary" id="close-column-editor">Cancel</button>
        <button type="button" class="btn-primary" id="column-editor-confirm">Save</button>
    </div>
</div>

<?php
// Render JavaScript inline by executing editor.js.php
ob_start();
include dirname(__FILE__) . '/editor.js.php';
$js_output = ob_get_clean();
echo $js_output;
?>
