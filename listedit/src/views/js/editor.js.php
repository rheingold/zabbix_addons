<?php
/**
 * ============================================================================
 * File: editor.js.php
 * Created: 2025-12-02
 *
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 *
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 *
 * PURPOSE:
 * JavaScript initialization for macro list editor.
 * Loads host/template list on page load and handles AJAX interactions.
 *
 * @var CView $this
 * @var array $data
 */
?>
<script>
jQuery(document).ready(function($) {
    // Initialize editor
    MacroListEditor.init({
        userType: <?= $data['user_type'] ?>,
        canEdit: <?= ($data['user_type'] >= USER_TYPE_ZABBIX_ADMIN) ? 'true' : 'false' ?>
    });
    
    // Load host/template list
    MacroListEditor.loadHostList();
});
</script>
