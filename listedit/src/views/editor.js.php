<?php
/**
 * ============================================================================
 * File: editor.js.php (v0.2.0)
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
 * Inline JavaScript initialization and MacroListEditor definition (v0.2.0).
 * Features:
 * - Macro dropdown selector (only one macro editable at a time)
 * - Host/template search dialog (searchable, scalable to 100+ items)
 * - Inline row/column editing with real-time table updates
 * - Bulk text editor panel that syncs with table
 * - Smooth transitions and theme-aware styling
 * - Column editor dialog for adding/removing columns
 *
 * All code here is inline to ensure proper execution order.
 *
 * @var CView $this
 * @var array $data
 */
?>
<script>
// Inline entire MacroListEditor to avoid async loading issues
var MacroListEditor = (function() {
    'use strict';
    
    var config = {
        userType: 0,
        canEdit: false
    };
    
    var state = {
        currentHostId: null,
        currentHostType: null,
        currentMacroId: null,
        currentMacroName: null,
        allMacros: [],
        hostList: [],
        bulkEditorExpanded: false
    };
    
    /**
     * Initialize editor
     */
    function init(options) {
        config = jQuery.extend(config, options);
        console.log('MacroListEditor: initialized with config', config);
        
        // Setup host search button
        jQuery('#open-host-search').on('click', openHostSearchDialog);
        jQuery('#host-search-confirm').on('click', confirmHostSearch);
        jQuery('#close-host-search').on('click', closeHostSearchDialog);
        jQuery('#host-search-overlay').on('click', closeHostSearchDialog);
        
        // Setup macro selector
        jQuery('#macro-selector').on('change', function() {
            selectMacro(jQuery(this).val());
        });
        
        // Setup bulk editor toggle
        jQuery('#bulk-editor-toggle').on('click', toggleBulkEditor);
        
        // Setup column editor
        jQuery('#open-column-editor').on('click', openColumnEditor);
        jQuery('#column-editor-confirm').on('click', confirmColumnEditor);
        jQuery('#close-column-editor-x').on('click', closeColumnEditor);
        jQuery('#close-column-editor-cancel').on('click', closeColumnEditor);
        jQuery('#column-editor-overlay').on('click', closeColumnEditor);
        jQuery('#add-column-btn').on('click', addColumnToEditor);
    }
    
    /**
     * Load host/template list for search dialog
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
                
                state.hostList = response.items;
                populateHostSearch(response.items);
            },
            error: function(xhr, status, error) {
                console.error('MacroListEditor: hostlist error', xhr, status, error);
                showError('Failed to load hosts/templates: ' + error);
            }
        });
    }
    
    /**
     * Open host/template search dialog
     */
    function openHostSearchDialog() {
        jQuery('#host-search-overlay, #host-search-dialog').addClass('active');
        loadHostList();
        jQuery('#host-search-input').focus().val('');
    }
    
    /**
     * Close host/template search dialog
     */
    function closeHostSearchDialog() {
        jQuery('#host-search-overlay, #host-search-dialog').removeClass('active');
    }
    
    /**
     * Populate host search results
     */
    function populateHostSearch(items) {
        var results = jQuery('#host-search-results');
        results.empty();
        
        var searchTerm = jQuery('#host-search-input').val().toLowerCase();
        var filtered = items.filter(function(item) {
            return item.name.toLowerCase().includes(searchTerm) || 
                   item.host.toLowerCase().includes(searchTerm);
        });
        
        if (filtered.length === 0) {
            results.html('<div style="padding: 20px; text-align: center; color: #999;">No items found</div>');
            return;
        }
        
        jQuery.each(filtered, function(i, item) {
            var elem = jQuery('<div class="host-search-item"></div>')
                .data('id', item.id)
                .data('type', item.type)
                .text(item.name + ' (' + item.type + ')')
                .on('click', function() {
                    state.currentHostId = jQuery(this).data('id');
                    state.currentHostType = jQuery(this).data('type');
                    jQuery('#host-search-results .host-search-item').removeClass('selected');
                    jQuery(this).addClass('selected');
                });
            results.append(elem);
        });
    }
    
    /**
     * Search host/template by input
     */
    function handleHostSearch() {
        populateHostSearch(state.hostList);
    }
    
    /**
     * Confirm host selection
     */
    function confirmHostSearch() {
        if (!state.currentHostId) {
            showError('Please select a host or template');
            return;
        }
        
        closeHostSearchDialog();
        
        // Update current display and load macros
        var hostItem = state.hostList.find(function(h) { return h.id === state.currentHostId; });
        jQuery('#current-host-display').text('Selected: ' + hostItem.name + ' (' + hostItem.type + ')');
        
        loadMacros();
    }
    
    /**
     * Load macros for selected host/template
     */
    function loadMacros() {
        if (!state.currentHostId) {
            showError('Please select a host or template first');
            return;
        }
        
        jQuery.ajax({
            url: 'zabbix.php?action=listedit.macrolist',
            method: 'POST',
            data: {
                hostid: state.currentHostId,
                type: state.currentHostType
            },
            dataType: 'json',
            success: function(response) {
                if (response.error) {
                    showError('Failed to load macros: ' + response.error);
                    return;
                }
                
                state.allMacros = response.macros || [];
                populateMacroSelector(state.allMacros);
                
                // Clear editor
                jQuery('#macro-editor-container').empty();
                state.currentMacroId = null;
                state.currentMacroName = null;
            },
            error: function(xhr, status, error) {
                showError('Failed to load macros: ' + error);
            }
        });
    }
    
    /**
     * Populate macro selector dropdown
     */
    function populateMacroSelector(macros) {
        var select = jQuery('#macro-selector');
        select.empty();
        select.append(jQuery('<option>', {
            value: '',
            text: 'Select a macro to edit...'
        }));
        
        jQuery.each(macros, function(i, macro) {
            select.append(jQuery('<option>', {
                value: macro.hostmacroid,
                text: macro.macro,
                'data-macro-name': macro.macro
            }));
        });
    }
    
    /**
     * Select and edit a specific macro
     */
    function selectMacro(macroId) {
        if (!macroId) {
            jQuery('#macro-editor-container').empty();
            state.currentMacroId = null;
            state.currentMacroName = null;
            return;
        }
        
        var macro = state.allMacros.find(function(m) { return m.hostmacroid === macroId; });
        if (!macro) return;
        
        state.currentMacroId = macroId;
        state.currentMacroName = macro.macro;
        
        displayMacroEditor(macro);
    }
    
    /**
     * Display macro editor for selected macro
     */
    function displayMacroEditor(macro) {
        var container = jQuery('#macro-editor-container');
        container.empty();
        
        var value = macro.value || '';
        var format = detectFormat(value);
        
        var html = '<div class="macro-editor" data-macroid="' + macro.hostmacroid + '" data-format="' + format + '">';
        html += '<div class="macro-header">';
        html += '<div style="display: flex; justify-content: space-between; align-items: center;">';
        html += '<div>';
        html += '<h4 style="margin: 0 0 5px 0;">' + escapeHtml(macro.macro) + '</h4>';
        if (macro.description) {
            html += '<p style="margin: 0; color: var(--mle-text-secondary); font-size: 12px;">' + escapeHtml(macro.description) + '</p>';
        }
        html += '<div class="format-info" style="margin-top: 5px;">Format: <strong>' + format + '</strong></div>';
        html += '</div>';
        html += '<div style="text-align: right;">';
        html += '<label style="display: flex; align-items: center; gap: 8px; font-size: 13px;">';
        html += '<input type="checkbox" class="toggle-header-format" data-macroid="' + macro.hostmacroid + '" checked>';
        html += '<span>Show text names only</span>';
        html += '</label>';
        html += '</div>';
        html += '</div>';
        html += '</div>';
        
        // Table editor
        html += renderMacroTableEditor(macro, format);
        
        // Bulk text editor
        html += renderBulkTextEditor(value, format);
        
        html += '</div>';
        
        container.html(html);
        
        // Attach event handlers
        attachMacroEditorEvents(container, macro.hostmacroid);
    }
    
    /**
     * Render macro table editor
     */
    function renderMacroTableEditor(macro, format) {
        var value = macro.value || '';
        var html = '<div class="macro-table-editor" style="margin-top: 15px;">';
        
        if (format === 'pipe-separated') {
            html += renderPipeTableEditor(value, macro.hostmacroid);
        } else if (format === 'json-array') {
            html += renderJsonTableEditor(value, macro.hostmacroid);
        } else {
            html += '<div class="empty-editor"><p>This macro is empty or in an unsupported format.</p>';
            if (config.canEdit) {
                html += '<button type="button" class="btn-init-pipe" data-macroid="' + macro.hostmacroid + '">Initialize as Pipe-Separated</button> ';
                html += '<button type="button" class="btn-init-json" data-macroid="' + macro.hostmacroid + '">Initialize as JSON Array</button>';
            }
            html += '</div>';
        }
        
        html += '</div>';
        return html;
    }
    
    /**
     * Render pipe-separated table editor with inline editing
     */
    function renderPipeTableEditor(value, macroId) {
        var parsed = parsePipeSeparated(value);
        var html = '<table class="list-table list-table-bordered" data-macroid="' + macroId + '">';
        
        // Header row with edit button
        html += '<thead><tr>';
        jQuery.each(parsed.headers, function(i, header) {
            // Display header in "text name only" format (strip brackets and hash)
            var displayHeader = header;
            if (displayHeader.startsWith('{#') && displayHeader.endsWith('}')) {
                displayHeader = displayHeader.substring(2, displayHeader.length - 1);
            }
            html += '<th data-full-header="' + escapeHtml(header) + '" class="table-header" data-macroid="' + macroId + '">' + escapeHtml(displayHeader) + '</th>';
        });
        html += '<th style="width: 100px;">Actions</th>';
        html += '</tr></thead>';
        
        // Data rows
        html += '<tbody>';
        jQuery.each(parsed.rows, function(i, row) {
            html += '<tr data-row-index="' + i + '" class="row-enter">';
            jQuery.each(row, function(j, cell) {
                html += '<td class="cell-editable" data-col="' + j + '">' + escapeHtml(cell) + '</td>';
            });
            html += '<td><button type="button" class="btn-link btn-remove-row" title="Remove row" data-macroid="' + macroId + '">✕</button></td>';
            html += '</tr>';
        });
        html += '</tbody>';
        
        html += '</table>';
        
        if (config.canEdit) {
            html += '<div style="margin-top: 10px; display: flex; align-items: center; gap: 8px;">';
            html += '<button type="button" class="btn-row-add" data-macroid="' + macroId + '">+ Add Row</button> ';
            html += '<button type="button" class="btn-col-edit" data-macroid="' + macroId + '">Edit Columns</button> ';
            html += '<button type="button" class="btn-save" data-macroid="' + macroId + '">Save Changes</button>';
            html += '</div>';
        }
        
        return html;
    }
    
    /**
     * Render JSON array table editor with inline editing
     */
    function renderJsonTableEditor(value, macroId) {
        var items = [];
        try {
            items = JSON.parse(value);
        } catch(e) {
            items = [];
        }
        
        var html = '<table class="list-table">';
        html += '<thead><tr><th>Value</th><th style="width: 100px;">Actions</th></tr></thead>';
        html += '<tbody>';
        
        jQuery.each(items, function(i, item) {
            html += '<tr data-row-index="' + i + '" class="row-enter">';
            html += '<td class="cell-editable" data-col="0">' + escapeHtml(item) + '</td>';
            html += '<td><button type="button" class="btn-link btn-remove-row" title="Remove" data-macroid="' + macroId + '">✕</button></td>';
            html += '</tr>';
        });
        
        html += '</tbody>';
        html += '</table>';
        
        if (config.canEdit) {
            html += '<div style="margin-top: 10px;">';
            html += '<button type="button" class="btn-row-add" data-macroid="' + macroId + '">+ Add Value</button> ';
            html += '<button type="button" class="btn-save" data-macroid="' + macroId + '">Save Changes</button>';
            html += '</div>';
        }
        
        return html;
    }
    
    /**
     * Render bulk text editor panel
     */
    function renderBulkTextEditor(value, format) {
        var html = '<div class="bulk-editor-panel">';
        html += '<div class="bulk-editor-toggle">📝 Raw Text Editor (click to expand)</div>';
        html += '<textarea class="bulk-editor-textarea" placeholder="Edit the raw ' + format + ' value here. Changes sync with table.">' + escapeHtml(value) + '</textarea>';
        html += '<div style="margin-top: 10px; color: #999; font-size: 12px;">Edit this text directly or edit the table above. Both stay in sync.</div>';
        html += '</div>';
        return html;
    }
    
    /**
     * Attach macro editor event handlers
     */
    function attachMacroEditorEvents(container, macroId) {
        var editor = container.find('.macro-editor');
        
        // Toggle header format (text-only vs full)
        editor.find('.toggle-header-format').on('change', function() {
            var table = editor.find('.list-table');
            var checked = jQuery(this).is(':checked');
            table.find('th.table-header').each(function() {
                var th = jQuery(this);
                var fullHeader = th.data('full-header');
                if (checked) {
                    // Show text only (strip {# and })
                    var displayHeader = fullHeader;
                    if (displayHeader.startsWith('{#') && displayHeader.endsWith('}')) {
                        displayHeader = displayHeader.substring(2, displayHeader.length - 1);
                    }
                    th.text(displayHeader);
                } else {
                    // Show full format
                    th.text(fullHeader);
                }
            });
            
            // If column editor is open, refresh it to match toggle
            var dialog = jQuery('#column-editor-dialog');
            if (dialog.hasClass('active')) {
                // Update the stored toggle state
                dialog.data('display-toggle', checked);
                
                var cols = dialog.find('#column-editor-columns');
                cols.find('input').each(function() {
                    var input = jQuery(this);
                    var fullHeader = input.data('full-header');
                    var displayValue = fullHeader;
                    
                    if (checked) {
                        // Show text-only
                        if (fullHeader.startsWith('{#') && fullHeader.endsWith('}')) {
                            displayValue = fullHeader.substring(2, fullHeader.length - 1);
                        }
                    } else {
                        // Show full format
                        displayValue = fullHeader;
                    }
                    input.val(displayValue);
                });
            }
        });
        
        // Inline cell editing
        editor.find('.cell-editable').on('click', function() {
            if (!config.canEdit) return;
            editCell(jQuery(this));
        });
        
        // Add row
        editor.find('.btn-row-add').on('click', function() {
            addTableRow(editor, jQuery(this));
        });
        
        // Remove row
        editor.find('.btn-remove-row').on('click', function() {
            removeTableRow(jQuery(this).closest('tr'), editor);
        });
        
        // Edit columns
        editor.find('.btn-col-edit').on('click', function() {
            openColumnEditor(editor);
        });
        
        // Save
        editor.find('.btn-save').on('click', function() {
            saveMacro(macroId, editor);
        });
        
        // Initialize empty
        editor.find('.btn-init-pipe').on('click', function() {
            initializeMacro(macroId, 'pipe');
        });
        editor.find('.btn-init-json').on('click', function() {
            initializeMacro(macroId, 'json');
        });
        
        // Bulk editor
        editor.find('.bulk-editor-toggle').on('click', function() {
            jQuery(this).closest('.bulk-editor-panel').toggleClass('collapsed');
        });
        
        editor.find('.bulk-editor-textarea').on('change', function() {
            syncTextToTable(jQuery(this), editor);
        });
    }
    
    /**
     * Edit cell inline
     */
    function editCell(cell) {
        if (cell.find('input').length > 0) return; // Already editing
        
        var value = cell.text();
        var input = jQuery('<input type="text" class="cell-input" value="' + escapeHtml(value) + '">');
        
        var originalContent = cell.html();
        cell.html(input);
        input.focus().select();
        
        function saveEdit() {
            var newValue = input.val();
            cell.text(newValue);
            syncTableToText(cell.closest('.macro-editor'));
        }
        
        function cancelEdit() {
            cell.html(originalContent);
        }
        
        input.on('blur', saveEdit);
        input.on('keypress', function(e) {
            if (e.which === 13) { // Enter
                saveEdit();
            } else if (e.which === 27) { // Escape
                cancelEdit();
            }
        });
    }
    
    /**
     * Add table row
     */
    function addTableRow(editor, button) {
        var table = editor.find('.list-table');
        var thead = table.find('thead tr');
        var numCols = thead.find('th').length - 1; // Exclude actions column
        
        var row = jQuery('<tr data-row-index="0" class="row-enter">');
        for (var i = 0; i < numCols; i++) {
            row.append(jQuery('<td class="cell-editable" data-col="' + i + '"></td>'));
        }
        row.append(jQuery('<td><button type="button" class="btn-link btn-remove-row" title="Remove row">✕</button></td>'));
        
        table.find('tbody').append(row);
        
        // Reattach events to new row
        row.find('.cell-editable').on('click', function() {
            if (config.canEdit) editCell(jQuery(this));
        });
        row.find('.btn-remove-row').on('click', function() {
            removeTableRow(jQuery(this).closest('tr'), editor);
        });
        
        syncTableToText(editor);
    }
    
    /**
     * Remove table row
     */
    function removeTableRow(row, editor) {
        if (!config.canEdit) return;
        if (!confirm('Remove this row?')) return;
        
        row.addClass('row-exit');
        setTimeout(function() {
            row.remove();
            syncTableToText(editor);
        }, 300);
    }
    
    /**
     * Open column editor dialog
     */
    function openColumnEditor(editor) {
        var table = editor.find('.list-table');
        var displayToggle = editor.find('.toggle-header-format').is(':checked');
        var fullHeaders = [];
        
        // Collect full header names from data attributes or text content
        table.find('thead th').not(':last').each(function() {
            var th = jQuery(this);
            var fullHeader = th.data('full-header') || (displayToggle ? '{#' + th.text() + '}' : th.text());
            fullHeaders.push(fullHeader);
        });
        
        var dialog = jQuery('#column-editor-dialog');
        var cols = dialog.find('#column-editor-columns');
        cols.empty();
        
        jQuery.each(fullHeaders, function(i, fullHeader) {
            var displayValue = fullHeader;
            
            // Show value based on current toggle setting
            if (displayToggle) {
                // Toggle is on: show text-only
                if (fullHeader.startsWith('{#') && fullHeader.endsWith('}')) {
                    displayValue = fullHeader.substring(2, fullHeader.length - 1);
                }
            } else {
                // Toggle is off: show full {#name} format
                displayValue = fullHeader;
            }
            
            var item = jQuery('<div class="column-item">');
            item.append(jQuery('<input type="text" value="' + escapeHtml(displayValue) + '" data-col-index="' + i + '" data-full-header="' + escapeHtml(fullHeader) + '" class="column-header-input">'));
            item.append(jQuery('<button type="button" class="btn-danger">Remove</button>').on('click', function() {
                jQuery(this).closest('.column-item').remove();
            }));
            cols.append(item);
        });
        
        dialog.addClass('active').data('editor', editor).data('display-toggle', displayToggle);
        jQuery('#column-editor-overlay').addClass('active');
    }
    
    /**
     * Add new column to column editor
     */
    function addColumnToEditor() {
        var dialog = jQuery('#column-editor-dialog');
        var cols = dialog.find('#column-editor-columns');
        var newIndex = cols.find('.column-item').length;
        var item = jQuery('<div class="column-item">');
        item.append(jQuery('<input type="text" placeholder="Column name..." data-col-index="' + newIndex + '" class="column-header-input">'));
        item.append(jQuery('<button type="button" class="btn-danger">Remove</button>').on('click', function() {
            jQuery(this).closest('.column-item').remove();
        }));
        cols.append(item);
    }
    
    /**
     * Confirm column editor changes
     */
    function confirmColumnEditor() {
        var dialog = jQuery('#column-editor-dialog');
        var editor = dialog.data('editor');
        var table = editor.find('.list-table');
        // Get the CURRENT toggle state from the editor, not the stored state
        // This ensures if user toggled while editing, we use the current state
        var displayToggle = editor.find('.toggle-header-format').is(':checked');
        var newHeaders = [];
        var fullHeaders = [];  // Store full format names
        var oldHeaders = [];
        
        // Collect old headers for data mapping
        table.find('thead th').not(':last').each(function() {
            var th = jQuery(this);
            oldHeaders.push(th.data('full-header') || th.text());
        });
        
        // Validate and collect new headers
        var allValid = true;
        dialog.find('#column-editor-columns input').each(function(idx) {
            var val = jQuery(this).val().trim();
            if (!val) {
                showError('Column name cannot be empty');
                allValid = false;
                return false;
            }
            
            // Check for unique names (case-insensitive)
            var lowerVal = val.toLowerCase();
            if (newHeaders.some(h => h.toLowerCase() === lowerVal)) {
                showError('Column name "' + val + '" is already used. Column names must be unique.');
                allValid = false;
                return false;
            }
            
            // If toggle is off, user is entering names with {# }, validate that
            // If toggle is on, user is entering text-only names, validate without {# }
            var fullHeader = val;
            var displayHeader = val;
            
            if (!displayToggle) {
                // User entering full format {#name}
                if (!val.startsWith('{#') || !val.endsWith('}')) {
                    showError('Column header must be in format {#name}. For example: {#HOSTNAME}');
                    allValid = false;
                    return false;
                }
                if (!/^{#[a-zA-Z0-9_-]+}$/.test(val)) {
                    showError('Invalid column header format. Use only {#name} with letters, numbers, underscore, and hyphen.');
                    allValid = false;
                    return false;
                }
                fullHeader = val;
                displayHeader = val.substring(2, val.length - 1);
            } else {
                // User entering text-only name
                if (!/^[a-zA-Z0-9_-]+$/.test(val)) {
                    showError('Column name contains invalid characters. Use only letters, numbers, underscore, and hyphen.');
                    allValid = false;
                    return false;
                }
                // Convert to full format for storage
                fullHeader = '{#' + val + '}';
                displayHeader = val;
            }
            
            newHeaders.push(displayHeader);
            fullHeaders.push(fullHeader);
        });
        
        if (!allValid) return;
        
        // Calculate which columns are new (didn't exist before)
        var newColumnIndices = [];
        jQuery.each(fullHeaders, function(i, fullHeader) {
            if (oldHeaders.indexOf(fullHeader) === -1) {
                newColumnIndices.push(i);
            }
        });
        
        // Update table headers
        var thead = table.find('thead tr');
        thead.find('th').not(':last').remove();
        var actionHeaderCell = thead.find('th:last'); // Get the actions header
        jQuery.each(newHeaders, function(i, header) {
            actionHeaderCell.before(jQuery('<th data-full-header="' + escapeHtml(fullHeaders[i]) + '" class="table-header">' + escapeHtml(header) + '</th>'));
        });
        
        // Update/rebuild data rows to match new column structure
        table.find('tbody tr').each(function(rowIdx) {
            var tr = jQuery(this);
            var cells = tr.find('td.cell-editable');
            var oldCells = cells.clone(true); // Keep old data temporarily
            var oldData = [];
            oldCells.each(function() {
                oldData.push(jQuery(this).text());
            });
            
            // Remove all editable cells
            cells.remove();
            
            // Rebuild cells in new order
            var actionCell = tr.find('td:last'); // Keep action cell at end
            jQuery.each(fullHeaders, function(colIdx, fullHeader) {
                var oldIdx = oldHeaders.indexOf(fullHeader);
                var cellValue = (oldIdx >= 0 && oldIdx < oldData.length) ? oldData[oldIdx] : '';
                var newCell = jQuery('<td class="cell-editable" data-col="' + colIdx + '"></td>');
                newCell.text(cellValue);
                newCell.on('click', function() {
                    if (config.canEdit) editCell(jQuery(this));
                });
                actionCell.before(newCell);
            });
        });
        
        syncTableToText(editor);
        closeColumnEditor();
    }
    
    /**
     * Close column editor dialog
     */
    function closeColumnEditor() {
        jQuery('#column-editor-dialog').removeClass('active');
        jQuery('#column-editor-overlay').removeClass('active');
    }
    
    /**
     * Toggle bulk editor panel
     */
    function toggleBulkEditor() {
        jQuery(this).closest('.bulk-editor-panel').toggleClass('collapsed');
    }
    
    /**
     * Sync table changes to text area
     */
    function syncTableToText(editor) {
        var table = editor.find('.list-table');
        var format = editor.data('format');
        var newValue;
        
        if (format === 'pipe-separated') {
            var rows = [];
            var headers = [];
            table.find('thead th').not(':last').each(function() {
                // Use full header from data attribute, not displayed text
                var fullHeader = jQuery(this).data('full-header') || jQuery(this).text();
                headers.push(fullHeader);
            });
            rows.push(headers.join('|'));
            
            table.find('tbody tr').each(function() {
                var cells = [];
                jQuery(this).find('td.cell-editable').each(function() {
                    cells.push(jQuery(this).text());
                });
                if (cells.length > 0) {
                    rows.push(cells.join('|'));
                }
            });
            newValue = rows.join(',');
        } else if (format === 'json-array') {
            var items = [];
            table.find('tbody tr').each(function() {
                var value = jQuery(this).find('td.cell-editable').text();
                if (value) items.push(value);
            });
            newValue = JSON.stringify(items);
        }
        
        editor.find('.bulk-editor-textarea').val(newValue);
    }
    
    /**
     * Sync text area changes to table
     */
    function syncTextToTable(textarea, editor) {
        var format = editor.data('format');
        var value = textarea.val();
        var table = editor.find('.list-table');
        
        try {
            if (format === 'pipe-separated') {
                var parts = value.split(',');
                if (parts.length > 0) {
                    var headers = parts[0].split('|');
                    var tbody = table.find('tbody');
                    tbody.empty();
                    
                    for (var i = 1; i < parts.length; i++) {
                        var cells = parts[i].split('|');
                        var row = jQuery('<tr class="row-enter">');
                        jQuery.each(cells, function(j, cell) {
                            row.append(jQuery('<td class="cell-editable" data-col="' + j + '">' + escapeHtml(cell) + '</td>'));
                        });
                        row.append(jQuery('<td><button type="button" class="btn-link btn-remove-row">✕</button></td>'));
                        tbody.append(row);
                    }
                    
                    // Reattach events
                    editor.find('.cell-editable').on('click', function() {
                        if (config.canEdit) editCell(jQuery(this));
                    });
                    editor.find('.btn-remove-row').on('click', function() {
                        removeTableRow(jQuery(this).closest('tr'), editor);
                    });
                }
            } else if (format === 'json-array') {
                var items = JSON.parse(value);
                var tbody = table.find('tbody');
                tbody.empty();
                
                jQuery.each(items, function(i, item) {
                    var row = jQuery('<tr class="row-enter">');
                    row.append(jQuery('<td class="cell-editable" data-col="0">' + escapeHtml(item) + '</td>'));
                    row.append(jQuery('<td><button type="button" class="btn-link btn-remove-row">✕</button></td>'));
                    tbody.append(row);
                });
                
                // Reattach events
                editor.find('.cell-editable').on('click', function() {
                    if (config.canEdit) editCell(jQuery(this));
                });
                editor.find('.btn-remove-row').on('click', function() {
                    removeTableRow(jQuery(this).closest('tr'), editor);
                });
            }
        } catch(e) {
            console.error('Error syncing text to table:', e);
        }
    }
    
    /**
     * Save macro changes
     */
    function saveMacro(macroId, editor) {
        var table = editor.find('.list-table');
        var format = editor.data('format');
        var newValue;
        
        // Get value from textarea if it exists, otherwise rebuild from table
        var textarea = editor.find('.bulk-editor-textarea');
        if (textarea.length > 0) {
            newValue = textarea.val();
        } else {
            if (format === 'pipe-separated') {
                var rows = [];
                var headers = [];
                table.find('thead th').not(':last').each(function() {
                    headers.push(jQuery(this).text());
                });
                rows.push(headers.join('|'));
                
                table.find('tbody tr').each(function() {
                    var cells = [];
                    jQuery(this).find('td.cell-editable').each(function() {
                        cells.push(jQuery(this).text());
                    });
                    if (cells.length > 0) {
                        rows.push(cells.join('|'));
                    }
                });
                newValue = rows.join(',');
            } else if (format === 'json-array') {
                var items = [];
                table.find('tbody tr').each(function() {
                    var value = jQuery(this).find('td.cell-editable').text();
                    if (value) items.push(value);
                });
                newValue = JSON.stringify(items);
            }
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
     * Initialize empty macro
     */
    function initializeMacro(macroId, format) {
        var defaultValue = format === 'pipe' ? '{#COL1}|{#COL2},value1|value2' : '["value1","value2"]';
        
        jQuery.ajax({
            url: 'zabbix.php?action=listedit.macroupdate',
            method: 'POST',
            data: {
                hostmacroid: macroId,
                value: defaultValue
            },
            dataType: 'json',
            success: function(response) {
                if (response.error) {
                    showError('Failed to initialize: ' + response.error);
                    return;
                }
                loadMacros();
            },
            error: function(xhr, status, error) {
                showError('Failed to initialize macro: ' + error);
            }
        });
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
        
        headers = parts[0].split('|');
        var numCols = headers.length;
        
        for (var i = 1; i < parts.length; i++) {
            var cells = parts[i].split('|');
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
     * Detect macro value format
     */
    function detectFormat(value) {
        if (!value || value.trim() === '') {
            return 'empty';
        }
        
        if (value.trim().startsWith('[') && value.trim().endsWith(']')) {
            try {
                JSON.parse(value);
                return 'json-array';
            } catch(e) {}
        }
        
        if (value.includes('|') || value.includes(',')) {
            return 'pipe-separated';
        }
        
        return 'unknown';
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
        loadMacros: loadMacros,
        selectMacro: selectMacro,
        handleHostSearch: handleHostSearch
    };
})();

// Initialize on document ready
jQuery(document).ready(function($) {
    MacroListEditor.init({
        userType: <?= $data['user_type'] ?>,
        canEdit: <?= ($data['user_type'] >= USER_TYPE_ZABBIX_ADMIN) ? 'true' : 'false' ?>
    });
    
    MacroListEditor.loadHostList();
});
</script>
