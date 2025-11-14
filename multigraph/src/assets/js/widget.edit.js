/**
 * ============================================================================
 * File: widget.edit.js
 * Created: 2025-11-09 19:30:00
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * Widget configuration form JavaScript for edit mode.
 * Extends Zabbix CWidgetForm to provide interactive form controls.
 * Handles item pattern building and host/item selection.
 * 
 * RELATIONS:
 * - Extends: CWidgetForm (Zabbix dashboard widget form framework)
 * - Called by: views/widget.edit.js.php (loaded in edit form)
 * - Uses: Zabbix overlayDialogue, Curl, jQuery AJAX
 * - Interacts with: includes/WidgetForm.php (form field definitions)
 * 
 * FEATURES:
 * - Form initialization and field binding
 * - Item pattern builder (currently disabled, uses manual entry)
 * - Host/item selection dialog
 * - Pattern generation from item names (wildcard/regex modes)
 * - LLD macro replacement ({#MACRO} → * or .+)
 * 
 * LIFECYCLE:
 * 1. Class instantiation as window.widget_form
 * 2. init() - Bind form fields
 * 3. User interactions trigger item selector
 * 4. Pattern generation and form updates
 */

/**
 * Multigraph Widget Form Handler
 * 
 * Singleton class managing widget configuration form interactions.
 * Instantiated as window.widget_form.
 * 
 * @extends CWidgetForm
 */
window.widget_form = new class extends CWidgetForm {

	/**
	 * Initialize widget form
	 * 
	 * PURPOSE:
	 * Binds form fields and sets up event handlers.
	 * Called automatically when form loads.
	 * 
	 * DEPENDENCIES:
	 * - Called by: Zabbix dashboard framework on widget edit
	 * - Calls: getForm(), addPatternBuilderButton(), ready()
	 * 
	 * @returns {void}
	 */
	init() {
		console.log('Multigraph widget form init');
		
		this._form = this.getForm();                                   // Get form DOM element
		this._item_pattern = document.getElementById('item_pattern'); // Item pattern text field
		this._pattern_mode = document.getElementById('pattern_mode'); // Pattern mode radio buttons
		this._graph_type = document.getElementById('graph_type');     // Graph type radio buttons
		
		// Try multiple ways to find the color set field (Zabbix may add prefixes)
		this._color_set = document.getElementById('color_set') || 
		                  document.querySelector('[name="color_set"]') ||
		                  document.querySelector('select[id*="color_set"]');
		this._graph_colors = document.getElementById('graph_colors') ||
		                     document.querySelector('[name="graph_colors"]') ||
		                     document.querySelector('input[id*="graph_colors"]');
		
		console.log('Found fields:', {
			color_set: this._color_set,
			graph_colors: this._graph_colors,
			color_set_id: this._color_set?.id,
			graph_colors_id: this._graph_colors?.id
		});
		
		// Define color sets (must match PHP WidgetForm::COLOR_SETS)
		this._color_sets = {
			'default': '#1f77b4,#ff7f0e,#2ca02c,#d62728,#9467bd,#8c564b,#e377c2,#7f7f7f,#bcbd22,#17becf',
			'pastel': '#aec7e8,#ffbb78,#98df8a,#ff9896,#c5b0d5,#c49c94,#f7b6d2,#c7c7c7,#dbdb8d,#9edae5',
			'vibrant': '#e60049,#0bb4ff,#50e991,#e6d800,#9b19f5,#ffa300,#dc0ab4,#b3d4ff,#00bfa0',
			'earth': '#8b4513,#daa520,#228b22,#4682b4,#d2691e,#708238,#cd853f,#556b2f,#b8860b',
			'ocean': '#003f5c,#2f4b7c,#665191,#a05195,#d45087,#f95d6a,#ff7c43,#ffa600',
			'sunset': '#ff6b6b,#ff8e53,#ffbe0b,#fb5607,#ff006e,#8338ec,#3a86ff',
			'forest': '#1b4332,#2d6a4f,#40916c,#52b788,#74c69d,#95d5b2,#b7e4c7,#d8f3dc',
			'monochrome': '#000000,#2d2d2d,#5a5a5a,#878787,#b4b4b4,#e1e1e1,#ffffff'
		};
		
		const barSeparationField = document.getElementById('bar_separation');
		this._bar_separation_row = barSeparationField ? barSeparationField.closest('.form-field') : null;
		
		const barDisplayModeField = document.getElementById('bar_display_mode');
		this._bar_display_mode_row = barDisplayModeField ? barDisplayModeField.closest('.form-field') : null;
		
		const distributionBinsField = document.getElementById('distribution_bins');
		this._distribution_bins_row = distributionBinsField ? distributionBinsField.closest('.form-field') : null;
		
		console.log('Item pattern field:', this._item_pattern);
		
		// Add pattern builder button (currently disabled)
		this.addPatternBuilderButton();
		
		// Setup color set selector handler
		if (this._color_set && this._graph_colors) {
			console.log('Color set handler installed, color_set:', this._color_set, 'graph_colors:', this._graph_colors);
			this._color_set.addEventListener('change', () => {
				const selectedSet = this._color_set.value;
				console.log('Color set changed to:', selectedSet);
				if (selectedSet && this._color_sets[selectedSet]) {
					console.log('Setting graph_colors to:', this._color_sets[selectedSet]);
					this._graph_colors.value = this._color_sets[selectedSet];
					// Trigger change event to notify Zabbix form of the update
					this._graph_colors.dispatchEvent(new Event('change', { bubbles: true }));
					console.log('graph_colors field value after update:', this._graph_colors.value);
				}
			});
		} else {
			console.log('Color set handler NOT installed - color_set:', this._color_set, 'graph_colors:', this._graph_colors);
		}
		
		// Setup graph type change handler for conditional field visibility
		if (this._graph_type && (this._bar_separation_row || this._bar_display_mode_row || this._distribution_bins_row)) {
			const updateGraphTypeFields = () => {
				const graphType = document.querySelector('input[name="graph_type"]:checked')?.value;
				
				// Show bar_separation and bar_display_mode only for Bar chart (type 1)
				if (this._bar_separation_row) {
					this._bar_separation_row.style.display = (graphType === '1') ? '' : 'none';
				}
				if (this._bar_display_mode_row) {
					this._bar_display_mode_row.style.display = (graphType === '1') ? '' : 'none';
				}
				
				// Show distribution_bins only for Distribution chart (type 2)
				if (this._distribution_bins_row) {
					this._distribution_bins_row.style.display = (graphType === '2') ? '' : 'none';
				}
			};
			
			// Initial visibility
			updateGraphTypeFields();
			
			// Update on change
			document.querySelectorAll('input[name="graph_type"]').forEach(radio => {
				radio.addEventListener('change', updateGraphTypeFields);
			});
		}
		
		this.ready(); // Signal form ready to Zabbix framework
	}

	/**
	 * Add pattern builder button
	 * 
	 * PURPOSE:
	 * Placeholder for pattern builder UI.
	 * Currently disabled - users manually enter patterns.
	 * 
	 * FUTURE ENHANCEMENT:
	 * Could add button to open item selector dialog for pattern building.
	 * 
	 * @returns {void}
	 */
	addPatternBuilderButton() {
		// Pattern builder disabled - use text field for pattern matching
		// User can manually enter patterns like "CPU*" or "Memory.*"
		return;
	}

	/**
	 * Open item selector dialog
	 * 
	 * PURPOSE:
	 * Opens dialog showing available items for selected host.
	 * Allows user to click item to generate pattern.
	 * 
	 * ALGORITHM:
	 * 1. Find hostids from form multiselect field (try multiple patterns)
	 * 2. Validate at least one host is selected
	 * 3. Call showItemList() with first host
	 * 
	 * DEPENDENCIES:
	 * - Uses: Zabbix overlayDialogue for error messages
	 * - Calls: showItemList()
	 * 
	 * ERROR HANDLING:
	 * - Shows error dialog if no host selected
	 * 
	 * @returns {void}
	 */
	openItemSelector() {
		// Get hostid from multiselect field
		// Try multiple possible field name patterns
		console.log('Form element:', this._form);
		
		// Pattern 1: hostids[0], hostids[1] (Zabbix multiselect array format)
		let hostid_inputs = this._form.querySelectorAll('input[name^="hostids["]');
		console.log('Pattern hostids[]: found', hostid_inputs.length, 'inputs');
		
		// Pattern 2: hostids_ (Zabbix hidden field format)
		if (hostid_inputs.length === 0) {
			hostid_inputs = this._form.querySelectorAll('input[name="hostids_"]');
			console.log('Pattern hostids_: found', hostid_inputs.length, 'inputs');
		}
		
		// Pattern 3: Any input with hostids in the name (fallback)
		if (hostid_inputs.length === 0) {
			hostid_inputs = this._form.querySelectorAll('input[name*="hostids"]');
			console.log('Pattern *hostids*: found', hostid_inputs.length, 'inputs');
			hostid_inputs.forEach((inp, idx) => {
				console.log(`  Input ${idx}: name="${inp.name}", value="${inp.value}", type="${inp.type}"`);
			});
		}
		
		// Collect host IDs from inputs
		const hostids = [];
		hostid_inputs.forEach(input => {
			if (input.value) {
				hostids.push(input.value);
			}
		});

		console.log('Found hostids:', hostids);

		if (hostids.length === 0) {
			overlayDialogue({
				'title': 'Error',
				'content': jQuery('<span>').text('Please select a host first.'),
				'buttons': [
					{
						'title': 'Ok',
						'focused': true,
						'action': function() {}
					}
				]
			});
			return;
		}

		// Use first selected host
		this.showItemList(hostids[0]);
	}

	/**
	 * Show item list for host
	 * 
	 * PURPOSE:
	 * Fetches items from host via AJAX and displays in dialog.
	 * 
	 * ALGORITHM:
	 * 1. Determine pattern mode (wildcard/regex)
	 * 2. Build AJAX request URL
	 * 3. Fetch items from multigraph.itemlist action
	 * 4. Call displayItemSelector() with results
	 * 
	 * DEPENDENCIES:
	 * - Uses: Zabbix Curl class for URL building
	 * - Uses: jQuery AJAX for request
	 * - Calls: displayItemSelector()
	 * 
	 * ERROR HANDLING:
	 * - Shows error dialog on network failure
	 * - Shows error dialog if API returns error
	 * - Shows error dialog if no items found
	 * 
	 * @param {string} hostid Host ID to fetch items for
	 * 
	 * @returns {void}
	 */
	showItemList(hostid) {
		console.log('showItemList called with hostid:', hostid);
		
		// Get pattern mode from form
		const pattern_mode_input = this._form.querySelector('[name="pattern_mode"]:checked');
		const is_regex = pattern_mode_input ? (pattern_mode_input.value == '1') : false;

		console.log('Pattern mode:', is_regex ? 'regex' : 'wildcard');

		// Call our custom widget action
		const curl = new Curl('zabbix.php');
		curl.setArgument('action', 'multigraph.itemlist');
		curl.setArgument('hostid', hostid);

		console.log('Fetching items from:', curl.getUrl());

		jQuery.ajax({
			url: curl.getUrl(),
			method: 'GET',
			dataType: 'json',
			success: (data) => {
				console.log('API response:', data);
				if (data && data.items) {
					console.log('Found', data.items.length, 'items');
					this.displayItemSelector(data.items, is_regex);
				} else if (data && data.error) {
					console.error('API error:', data.error);
					overlayDialogue({
						'title': 'Error',
						'content': jQuery('<span>').text('Error: ' + data.error),
						'buttons': [
							{
								'title': 'Ok',
								'focused': true,
								'action': function() {}
							}
						]
					});
				} else {
					console.error('Unexpected response format:', data);
					overlayDialogue({
						'title': 'Error',
						'content': jQuery('<span>').text('No items found'),
						'buttons': [
							{
								'title': 'Ok',
								'focused': true,
								'action': function() {}
							}
						]
					});
				}
			},
			error: (jqXHR, textStatus, errorThrown) => {
				console.error('Failed to fetch items:', textStatus, errorThrown);
				console.log('Response text:', jqXHR.responseText);
				overlayDialogue({
					'title': 'Error',
					'content': jQuery('<span>').text('Network error: ' + textStatus),
					'buttons': [
						{
							'title': 'Ok',
							'focused': true,
							'action': function() {}
						}
					]
				});
			}
		});
	}

	/**
	 * Display item selector dialog
	 * 
	 * PURPOSE:
	 * Renders item list in overlay dialog with hover effects.
	 * Clicking item fills pattern field and closes dialog.
	 * 
	 * ALGORITHM:
	 * 1. Create scrollable item list container
	 * 2. For each item, generate pattern and create clickable div
	 * 3. Add hover effects for better UX
	 * 4. On click: fill pattern field, trigger change event, close dialog
	 * 
	 * DEPENDENCIES:
	 * - Uses: Zabbix overlayDialogue, overlayDialogueDestroy
	 * - Calls: itemNameToPattern() to generate patterns
	 * 
	 * @param {Array<Object>} items Array of item objects {itemid, name}
	 * @param {boolean} is_regex Whether to use regex mode (true) or wildcard mode (false)
	 * 
	 * @returns {void}
	 */
	displayItemSelector(items, is_regex) {
		console.log('displayItemSelector called with', items.length, 'items');
		
		const list = document.createElement('div');
		list.className = 'multiselect-list';
		list.style.maxHeight = '400px';
		list.style.overflow = 'auto';

		if (items.length === 0) {
			list.textContent = 'No items found';
		} else {
			items.forEach(item => {
				const div = document.createElement('div');
				div.className = 'multiselect-item';
				div.style.padding = '5px';
				div.style.cursor = 'pointer';
				div.style.borderBottom = '1px solid var(--widget-border-color)';
				
				// Generate pattern from item name
				const pattern = this.itemNameToPattern(item.name, is_regex);
				div.textContent = item.name + ' → ' + pattern;
				div.title = 'Click to use this pattern';
				
				// On click: fill pattern field and close dialog
				div.addEventListener('click', () => {
					this._item_pattern.value = pattern;
					this._item_pattern.dispatchEvent(new Event('change')); // Trigger form validation
					overlayDialogueDestroy('item-selector');
				});

				// Hover effects for better UX
				div.addEventListener('mouseenter', () => {
					div.style.backgroundColor = 'var(--hover-bg-color, #f4f4f4)';
				});

				div.addEventListener('mouseleave', () => {
					div.style.backgroundColor = '';
				});

				list.appendChild(div);
			});
		}

		console.log('Showing overlay dialogue');
		
		overlayDialogue({
			'title': 'Select item pattern',
			'content': jQuery(list),
			'buttons': [
				{
					'title': 'Cancel',
					'class': 'btn-alt',
					'action': function() {}
				}
			]
		}, null, 'item-selector');
		
		console.log('Overlay dialogue shown');
	}

	/**
	 * Convert item name to pattern
	 * 
	 * PURPOSE:
	 * Generates wildcard or regex pattern from item name.
	 * Replaces LLD macros ({#MACRO}) with appropriate wildcards.
	 * 
	 * ALGORITHM (Wildcard mode):
	 * - Replace {#MACRO} with *
	 * 
	 * ALGORITHM (Regex mode):
	 * 1. Replace {#MACRO} with .+ (matches any characters)
	 * 2. Escape other regex special characters
	 * 3. Restore .+ for macro replacements
	 * 
	 * EXAMPLES:
	 * - "CPU usage on {#CPU}" → "CPU usage on *" (wildcard)
	 * - "CPU usage on {#CPU}" → "CPU usage on .+" (regex)
	 * - "Memory [total]" → "Memory [total]" (no change)
	 * 
	 * @param {string} itemName Original item name (may contain LLD macros)
	 * @param {boolean} is_regex Whether to use regex mode (true) or wildcard mode (false)
	 * 
	 * @returns {string} Generated pattern for matching
	 */
	itemNameToPattern(itemName, is_regex) {
		// Find LLD macros like {#MACRO} (standard Zabbix format)
		const macroRegex = /\{#[A-Z0-9_]+\}/g;
		
		if (is_regex) {
			// Regex mode: replace {#MACRO} with .+
			let pattern = itemName.replace(macroRegex, '.+');
			// Escape other regex special chars for literal matching
			pattern = pattern.replace(/([.*+?^${}()|[\]\\])/g, '\\$1');
			// Restore the .+ we want for macros (was escaped above)
			pattern = pattern.replace(/\\\.\\\+/g, '.+');
			return pattern;
		} else {
			// Wildcard mode: replace {#MACRO} with *
			return itemName.replace(macroRegex, '*');
		}
	}
};

// Initialize form on load
widget_form.init();
