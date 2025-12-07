<?php
/**
 * ============================================================================
 * File: editor.php
 * Created: 2025-12-02
 *
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 *
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 *
 * PURPOSE:
 * Main editor page HTML view.
 * Provides UI for selecting host/template and editing _LIST} macros.
 *
 * @var CView $this
 * @var array $data
 */

$this->addCssFile('modules/listedit/assets/css/listedit.css');
?>

<form id="macro-list-editor-form" class="list-form" method="post">
    <div class="section">
        <div class="list-table">
            <div class="list-row">
                <div class="list-col list-col-left">Host/Template:</div>
                <div class="list-col">
                    <select id="hostid" name="hostid" style="min-width: 400px;">
                        <option value="">Select host or template...</option>
                    </select>
                </div>
            </div>
            <div class="list-row">
                <div class="list-col"></div>
                <div class="list-col">
                    <button type="button" class="btn-primary" onclick="MacroListEditor.loadMacros()">Load Macros</button>
                </div>
            </div>
        </div>
    </div>
    <div id="macro-list-container" style="margin-top: 20px;"></div>
</form>

<?php
// Render JavaScript inline by executing editor.js.php
ob_start();
include dirname(__FILE__) . '/editor.js.php';
$js_output = ob_get_clean();
echo $js_output;
?>
