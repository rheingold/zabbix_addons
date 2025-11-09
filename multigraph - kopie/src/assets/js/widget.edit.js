window.widget_form = new class extends CWidgetForm {

	init() {
		console.log('Multigraph widget form init');
		
		this._form = this.getForm();
		this._item_pattern = document.getElementById('item_pattern');
		this._pattern_mode = document.getElementById('pattern_mode');
		
		console.log('Item pattern field:', this._item_pattern);
		
		// Add pattern builder button
		this.addPatternBuilderButton();
		
		this.ready();
	}

	addPatternBuilderButton() {
		// Pattern builder disabled - use text field for pattern matching
		// User can manually enter patterns like "CPU*" or "Memory.*"
		return;
	}

	openItemSelector() {
		// Get hostid from multiselect field
		// Try multiple possible field name patterns
		console.log('Form element:', this._form);
		
		// Pattern 1: hostids[0], hostids[1]
		let hostid_inputs = this._form.querySelectorAll('input[name^="hostids["]');
		console.log('Pattern hostids[]: found', hostid_inputs.length, 'inputs');
		
		// Pattern 2: hostids_
		if (hostid_inputs.length === 0) {
			hostid_inputs = this._form.querySelectorAll('input[name="hostids_"]');
			console.log('Pattern hostids_: found', hostid_inputs.length, 'inputs');
		}
		
		// Pattern 3: Any input with hostids in the name
		if (hostid_inputs.length === 0) {
			hostid_inputs = this._form.querySelectorAll('input[name*="hostids"]');
			console.log('Pattern *hostids*: found', hostid_inputs.length, 'inputs');
			hostid_inputs.forEach((inp, idx) => {
				console.log(`  Input ${idx}: name="${inp.name}", value="${inp.value}", type="${inp.type}"`);
			});
		}
		
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

	showItemList(hostid) {
		console.log('showItemList called with hostid:', hostid);
		
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
				
				const pattern = this.itemNameToPattern(item.name, is_regex);
				div.textContent = item.name + ' → ' + pattern;
				div.title = 'Click to use this pattern';
				
				div.addEventListener('click', () => {
					this._item_pattern.value = pattern;
					this._item_pattern.dispatchEvent(new Event('change'));
					overlayDialogueDestroy('item-selector');
				});

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

	itemNameToPattern(itemName, is_regex) {
		// Find LLD macros like {#MACRO}
		const macroRegex = /\{#[A-Z0-9_]+\}/g;
		
		if (is_regex) {
			// Regex mode: replace {#MACRO} with .+
			let pattern = itemName.replace(macroRegex, '.+');
			// Escape other regex special chars
			pattern = pattern.replace(/([.*+?^${}()|[\]\\])/g, '\\$1');
			// Restore the .+ we want
			pattern = pattern.replace(/\\\.\\\+/g, '.+');
			return pattern;
		} else {
			// Wildcard mode: replace {#MACRO} with *
			return itemName.replace(macroRegex, '*');
		}
	}
};

widget_form.init();
