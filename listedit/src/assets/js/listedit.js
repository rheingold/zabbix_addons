/**
 * ============================================================================
 * File: listedit.js
 * Created: 2025-12-02
 *
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 *
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 *
 * PURPOSE:
 * Main JavaScript for Macro List Editor.
 * Handles UI interactions, AJAX calls, and list editing functionality.
 *
 * SUPPORTED FORMATS:
 * 1. Pipe-separated with headers: {#COL1}|{#COL2},val1|val2,val3|val4
 * 2. JSON array (single column): ["val1","val2","val3"]
 * 3. Empty value: ""
 */

var MacroListEditor = (function() {
    'use strict';
    
    var config = {
        userType: 0,
        canEdit: false
    };
    
    var state = {
        currentHostId: null,
        currentHostType: null,
        currentMacro: null
    };
    
    /**
     * Initialize editor
     */
    function init(options) {
        config = jQuery.extend(config, options);
        console.log('MacroListEditor: initialized with config', config);
    }
    
    /**
     * Load host/template list
     */
    function loadHostList() {
        console.log('MacroListEditor: loadHostList() called');
        jQuery.ajax({
            url: 'zabbix.php?action=listedit.hostlist',
            method: 'POST',
            dataType: 'json',
            success: function(response) {
                console.log('MacroListEditor: hostlist response', response);
                if (response.error) {
                    showError('Failed to load hosts/templates: ' + response.error);
                    return;
                }
                
                populateHostSelect(response.items);
            },
            error: function(xhr, status, error) {
                console.error('MacroListEditor: hostlist error', xhr, status, error);
                showError('Failed to load hosts/templates: ' + error);
            }
        });
    }
    
    /**
     * Populate host/template select dropdown
     */
    function populateHostSelect(items) {
        var select = jQuery('#hostid');
        select.empty();
        select.append(jQuery('<option>', {
            value: '',
            text: 'Select host or template...'
        }));
        
        jQuery.each(items, function(i, item) {
            var label = item.name + ' (' + item.type + ')';
            select.append(jQuery('<option>', {
                value: item.id,
                text: label,
                'data-type': item.type
            }));
        });
    }
    
    /**
     * Load macros for selected host/template
     */
    function loadMacros() {
        var hostId = jQuery('#hostid').val();
        var hostType = jQuery('#hostid option:selected').data('type');
        
        if (!hostId) {
            showError('Please select a host or template');
            return;
        }
        
        state.currentHostId = hostId;
        state.currentHostType = hostType;
        
        jQuery.ajax({
            url: 'zabbix.php?action=listedit.macrolist',
            method: 'POST',
            data: {
                hostid: hostId,
                type: hostType
            },
            dataType: 'json',
            success: function(response) {
                if (response.error) {
                    showError('Failed to load macros: ' + response.error);
                    return;
                }
                
                displayMacros(response.macros);
            },
            error: function(xhr, status, error) {
                showError('Failed to load macros: ' + error);
            }
        });
    }
    
    /**
     * Display macros in UI
     */
    function displayMacros(macros) {
        var container = jQuery('#macro-list-container');
        container.empty();
        
        if (macros.length === 0) {
            container.html('<div class="msg-info">No _LIST} macros found for this host/template.</div>');
            return;
        }
        
        var html = '<div class="macro-list-wrapper">';
        
        jQuery.each(macros, function(i, macro) {
            html += '<div class="macro-item" style="margin-bottom: 30px; border: 1px solid #ccc; padding: 15px;">';
            html += '<h3>' + escapeHtml(macro.macro) + '</h3>';
            
            if (macro.description) {
                html += '<p style="color: #666; margin: 5px 0 10px 0;"><strong>Description:</strong> ' + escapeHtml(macro.description) + '</p>';
            }
            
            html += '<div class="macro-editor-container" data-macroid="' + macro.hostmacroid + '" data-value="' + escapeHtml(macro.value) + '">';
            html += renderMacroEditor(macro);
            html += '</div>';
            
            html += '</div>';
        });
        
        html += '</div>';
        container.html(html);
        
        // Attach event handlers
        attachEditorEvents();
    }
    
    /**
     * Render macro editor based on format
     */
    function renderMacroEditor(macro) {
        var value = macro.value || '';
        var format = detectFormat(value);
        
        var html = '<div class="format-info" style="margin-bottom: 10px; color: #666; font-size: 11px;">';
        html += 'Detected format: <strong>' + format + '</strong>';
        html += '</div>';
        
        if (format === 'pipe-separated') {
            html += renderPipeEditor(value, macro.hostmacroid);
        } else if (format === 'json-array') {
            html += renderJsonArrayEditor(value, macro.hostmacroid);
        } else {
            html += renderEmptyEditor(macro.hostmacroid);
        }
        
        return html;
    }
    
    /**
     * Detect macro value format
     */
    function detectFormat(value) {
        if (!value || value.trim() === '') {
            return 'empty';
        }
        
        // Try JSON array
        if (value.trim().startsWith('[') && value.trim().endsWith(']')) {
            try {
                JSON.parse(value);
                return 'json-array';
            } catch(e) {
                // Not valid JSON, fall through
            }
        }
        
        // Check for pipe-separated format
        if (value.includes('|') || value.includes(',')) {
            return 'pipe-separated';
        }
        
        return 'unknown';
    }
    
    /**
     * Render pipe-separated format editor
     */
    function renderPipeEditor(value, macroId) {
        var parsed = parsePipeSeparated(value);
        
        var html = '<div class="pipe-editor" data-format="pipe-separated">';
        html += '<table class="list-table" style="width: 100%; border-collapse: collapse; margin-bottom: 10px;">';
        
        // Header row
        html += '<thead><tr style="background: #f0f0f0;">';
        jQuery.each(parsed.headers, function(i, header) {
            html += '<th style="border: 1px solid #ccc; padding: 8px;">' + escapeHtml(header) + '</th>';
        });
        html += '<th style="border: 1px solid #ccc; padding: 8px; width: 80px;">Actions</th>';
        html += '</tr></thead>';
        
        // Data rows
        html += '<tbody>';
        jQuery.each(parsed.rows, function(i, row) {
            html += '<tr data-row-index="' + i + '">';
            jQuery.each(row, function(j, cell) {
                html += '<td style="border: 1px solid #ccc; padding: 4px;">';
                html += '<input type="text" class="cell-input" data-col="' + j + '" value="' + escapeHtml(cell) + '" style="width: 100%; border: none; padding: 4px;" ' + (config.canEdit ? '' : 'readonly') + '>';
                html += '</td>';
            });
            html += '<td style="border: 1px solid #ccc; padding: 4px; text-align: center;">';
            if (config.canEdit) {
                html += '<button class="btn-link btn-remove-row" title="Remove row">❌</button>';
            }
            html += '</td>';
            html += '</tr>';
        });
        html += '</tbody>';
        
        html += '</table>';
        
        if (config.canEdit) {
            html += '<button class="btn-add-row" data-macroid="' + macroId + '">Add Row</button> ';
            html += '<button class="btn-save" data-macroid="' + macroId + '">Save Changes</button>';
        }
        
        html += '</div>';
        
        return html;
    }
    
    /**
     * Render JSON array editor
     */
    function renderJsonArrayEditor(value, macroId) {
        var items = [];
        try {
            items = JSON.parse(value);
        } catch(e) {
            items = [];
        }
        
        var html = '<div class="json-array-editor" data-format="json-array">';
        html += '<table class="list-table" style="width: 100%; border-collapse: collapse; margin-bottom: 10px;">';
        html += '<thead><tr style="background: #f0f0f0;">';
        html += '<th style="border: 1px solid #ccc; padding: 8px;">Value</th>';
        html += '<th style="border: 1px solid #ccc; padding: 8px; width: 80px;">Actions</th>';
        html += '</tr></thead>';
        html += '<tbody>';
        
        jQuery.each(items, function(i, item) {
            html += '<tr data-row-index="' + i + '">';
            html += '<td style="border: 1px solid #ccc; padding: 4px;">';
            html += '<input type="text" class="cell-input" data-col="0" value="' + escapeHtml(item) + '" style="width: 100%; border: none; padding: 4px;" ' + (config.canEdit ? '' : 'readonly') + '>';
            html += '</td>';
            html += '<td style="border: 1px solid #ccc; padding: 4px; text-align: center;">';
            if (config.canEdit) {
                html += '<button class="btn-link btn-remove-row" title="Remove row">❌</button>';
            }
            html += '</td>';
            html += '</tr>';
        });
        
        html += '</tbody>';
        html += '</table>';
        
        if (config.canEdit) {
            html += '<button class="btn-add-row" data-macroid="' + macroId + '">Add Value</button> ';
            html += '<button class="btn-save" data-macroid="' + macroId + '">Save Changes</button>';
        }
        
        html += '</div>';
        
        return html;
    }
    
    /**
     * Render empty editor
     */
    function renderEmptyEditor(macroId) {
        var html = '<div class="empty-editor">';
        html += '<p style="color: #666;">This macro is empty or in an unsupported format.</p>';
        
        if (config.canEdit) {
            html += '<button class="btn-init-pipe" data-macroid="' + macroId + '">Initialize as Pipe-Separated</button> ';
            html += '<button class="btn-init-json" data-macroid="' + macroId + '">Initialize as JSON Array</button>';
        }
        
        html += '</div>';
        
        return html;
    }
    
    /**
     * Parse pipe-separated format
     */
    function parsePipeSeparated(value) {
        var parts = value.split(',');
        var headers = [];
        var rows = [];
        
        if (parts.length === 0) {
            return {headers: [], rows: []};
        }
        
        // First part is headers
        headers = parts[0].split('|');
        var numCols = headers.length;
        
        // Remaining parts are data rows
        for (var i = 1; i < parts.length; i++) {
            var cells = parts[i].split('|');
            // Pad or truncate to match header count
            while (cells.length < numCols) {
                cells.push('');
            }
            if (cells.length > numCols) {
                cells = cells.slice(0, numCols);
            }
            rows.push(cells);
        }
        
        return {headers: headers, rows: rows};
    }
    
    /**
     * Serialize pipe-separated format
     */
    function serializePipeSeparated(container) {
        var rows = [];
        var headers = [];
        
        // Extract headers
        container.find('thead th').not(':last').each(function() {
            headers.push(jQuery(this).text());
        });
        
        rows.push(headers.join('|'));
        
        // Extract data rows
        container.find('tbody tr').each(function() {
            var cells = [];
            jQuery(this).find('.cell-input').each(function() {
                cells.push(jQuery(this).val());
            });
            if (cells.length > 0) {
                rows.push(cells.join('|'));
            }
        });
        
        return rows.join(',');
    }
    
    /**
     * Serialize JSON array format
     */
    function serializeJsonArray(container) {
        var items = [];
        
        container.find('tbody tr').each(function() {
            var value = jQuery(this).find('.cell-input').val();
            if (value) {
                items.push(value);
            }
        });
        
        return JSON.stringify(items);
    }
    
    /**
     * Attach event handlers
     */
    function attachEditorEvents() {
        // Add row button
        jQuery('.btn-add-row').on('click', function() {
            var editor = jQuery(this).closest('.pipe-editor, .json-array-editor');
            var tbody = editor.find('tbody');
            var numCols = editor.find('thead th').not(':last').length;
            
            var html = '<tr data-row-index="' + tbody.find('tr').length + '">';
            for (var i = 0; i < numCols; i++) {
                html += '<td style="border: 1px solid #ccc; padding: 4px;">';
                html += '<input type="text" class="cell-input" data-col="' + i + '" value="" style="width: 100%; border: none; padding: 4px;">';
                html += '</td>';
            }
            html += '<td style="border: 1px solid #ccc; padding: 4px; text-align: center;">';
            html += '<button class="btn-link btn-remove-row" title="Remove row">❌</button>';
            html += '</td>';
            html += '</tr>';
            
            tbody.append(html);
            attachRemoveRowEvents();
        });
        
        // Remove row button
        attachRemoveRowEvents();
        
        // Save button
        jQuery('.btn-save').on('click', function() {
            var macroId = jQuery(this).data('macroid');
            saveMacro(macroId);
        });
        
        // Initialize buttons
        jQuery('.btn-init-pipe').on('click', function() {
            var macroId = jQuery(this).data('macroid');
            initializePipeFormat(macroId);
        });
        
        jQuery('.btn-init-json').on('click', function() {
            var macroId = jQuery(this).data('macroid');
            initializeJsonFormat(macroId);
        });
    }
    
    /**
     * Attach remove row event handlers
     */
    function attachRemoveRowEvents() {
        jQuery('.btn-remove-row').off('click').on('click', function() {
            if (confirm('Remove this row?')) {
                jQuery(this).closest('tr').remove();
            }
        });
    }
    
    /**
     * Save macro changes
     */
    function saveMacro(macroId) {
        var container = jQuery('[data-macroid="' + macroId + '"]');
        var editor = container.find('.pipe-editor, .json-array-editor');
        var format = editor.data('format');
        var newValue;
        
        if (format === 'pipe-separated') {
            newValue = serializePipeSeparated(editor);
        } else if (format === 'json-array') {
            newValue = serializeJsonArray(editor);
        } else {
            showError('Unknown format');
            return;
        }
        
        jQuery.ajax({
            url: 'zabbix.php?action=listedit.macroupdate',
            method: 'POST',
            data: {
                hostmacroid: macroId,
                value: newValue
            },
            dataType: 'json',
            success: function(response) {
                if (response.error) {
                    showError('Failed to save: ' + response.error);
                    return;
                }
                
                showSuccess('Macro saved successfully');
            },
            error: function(xhr, status, error) {
                showError('Failed to save macro: ' + error);
            }
        });
    }
    
    /**
     * Initialize pipe format
     */
    function initializePipeFormat(macroId) {
        var defaultValue = '{#COL1}|{#COL2},value1|value2';
        updateMacroValue(macroId, defaultValue);
    }
    
    /**
     * Initialize JSON format
     */
    function initializeJsonFormat(macroId) {
        var defaultValue = '["value1","value2"]';
        updateMacroValue(macroId, defaultValue);
    }
    
    /**
     * Update macro value and reload
     */
    function updateMacroValue(macroId, value) {
        jQuery.ajax({
            url: 'zabbix.php?action=listedit.macroupdate',
            method: 'POST',
            data: {
                hostmacroid: macroId,
                value: value
            },
            dataType: 'json',
            success: function(response) {
                if (response.error) {
                    showError('Failed to initialize: ' + response.error);
                    return;
                }
                
                // Reload macros
                loadMacros();
            },
            error: function(xhr, status, error) {
                showError('Failed to initialize macro: ' + error);
            }
        });
    }
    
    /**
     * Show error message
     */
    function showError(message) {
        alert('Error: ' + message);
    }
    
    /**
     * Show success message
     */
    function showSuccess(message) {
        alert(message);
    }
    
    /**
     * Escape HTML
     */
    function escapeHtml(text) {
        if (!text) return '';
        return text
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#039;');
    }
    
    // Public API
    return {
        init: init,
        loadHostList: loadHostList,
        loadMacros: loadMacros
    };
})();
