<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: widget.edit.js.php
 * Created: 2025-11-09 19:30:00
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * Widget edit form JavaScript initialization template.
 * Generates JavaScript code for form instance creation.
 * Minimal implementation - most logic in assets/js/widget.edit.js.
 * 
 * RELATIONS:
 * - Called by: Zabbix framework when rendering widget edit form
 * - Loaded after: views/widget.edit.php (form HTML)
 * - Loads: assets/js/widget.edit.js (main form logic)
 * - Extends: CWidgetForm (Zabbix widget form framework)
 * 
 * ARCHITECTURE:
 * This file is kept minimal because main form logic is in widget.edit.js.
 * Zabbix loads this template-generated JavaScript to create form instance.
 * The actual init() implementation is in widget.edit.js which overrides this.
 * 
 * @var CView $this Zabbix view object
 * @var array $data Widget data from controller
 */
?>
window.widget_form = new class extends CWidgetForm {

	constructor() {
		super();
		this.setupColorSetHandler();
		this.setupPatternBuilder();
	}

	/**
	 * Setup color set dropdown handler
	 */
	setupColorSetHandler() {
		// Color sets definition (must match PHP constants)
		this._color_sets = {
			0: '#000080, #9999ff, #3333b3, #b3b3ff, #1a1a99, #6666e6, #4d4dcc, #8080ff',
			1: '#1a1a1a, #573a2d, #0d0d0d, #3d2e26, #5a2e2e, #2d2420, #4d2626, #4a3429',
			2: '#87ceeb, #00ced1, #b0e0e6, #00cca3, #87d3db, #40e0d0, #7dd3c0, #00e5cc',
			3: '#ff8c00, #ffa500, #d2a679, #cd853f, #f4d68e, #e6cc80, #f5deb3, #ffe4a3',
			4: '#ffff00, #ffb300, #ffd700, #ffe680, #ffcc00, #ffe066, #ffdb4d, #ffcc66',
			5: '#8b4513, #a0522d, #6b3410, #8b3a3a, #a0684a, #6d3030, #b88661, #5c2e2e',
			6: '#008b8b, #40e0d0, #00ced1, #7fffd4, #20b2aa, #afeeee, #48d1cc, #5fd8d8',
			7: '#800080, #6a1a3a, #8b008b, #800020, #9932cc, #6b2d5c, #8b0058, #9400d3',
			8: '#9acd32, #8fbc3f, #7a9d2e, #aed850, #6b8e23, #c0e060, #5a7d1f, #b5d96b',
			9: '#006400, #228b22, #004d00, #3cb371, #008000, #00662e, #2e8b57, #003d00',
			10: '#39ff14, #7fff00, #28e028, #90ff90, #32cd32, #20b020, #00ff00, #57ff57',
			11: '#013220, #005f56, #004d40, #006b54, #1f3f1f, #2c5c3c, #4a6b2f, #3d5c3d',
			12: '#39ff14, #00ffff, #0dff92, #ccff00, #00ff9f, #adff2f, #00e5e5, #88ff00',
			13: '#ff0000, #ff8800, #ffff00, #00ff00, #00ffff, #0088ff, #8800ff, #ff00ff',
			14: '#ff00ff, #8800ff, #0088ff, #00ffff, #00ff00, #ffff00, #ff8800, #ff0000',
			15: '#8b0000, #a02020, #cd0000, #ff0000, #ff3333, #ff6666, #ff9999, #ffcccc',
			16: '#5c0000, #6b0000, #7a0000, #8b0000, #9a1515, #a02020, #b03030, #c04040',
			17: '#ff6666, #ff7777, #ff8888, #ff9999, #ffaaaa, #ffbbbb, #ffcccc, #ffdddd',
			18: '#ff1493, #ff3399, #ff66b3, #ff69b4, #ff99cc, #ffb3d9, #ffcce6, #ffe6f2',
			19: '#003300, #004400, #005500, #006400, #007300, #008200, #009100, #00a000',
			20: '#008000, #009900, #00b300, #00cc00, #00e600, #00ff00, #66ff66, #99ff99',
			21: '#66ff66, #77ff77, #88ff88, #99ff99, #aaffaa, #bbffbb, #ccffcc, #ddffdd',
			22: '#000066, #000080, #00009a, #0000b3, #0000cd, #1a1aff, #3333ff, #4d4dff',
			23: '#0000ff, #1a1aff, #3333ff, #4d4dff, #6666ff, #8080ff, #9999ff, #b3b3ff',
			24: '#6666ff, #7777ff, #87ceeb, #99ccff, #aaddff, #bbddff, #cceeff, #e6f5ff',
			25: '#cccc00, #e6e600, #ffff00, #ffff33, #ffff66, #ffff99, #ffffcc, #ffffe6',
			26: '#cc6600, #ff7700, #ff8800, #ff9933, #ffaa66, #ffbb99, #ffccaa, #ffddcc',
			27: '#6600cc, #7700ee, #8800ff, #9933ff, #aa66ff, #bb99ff, #ccbbff, #ddccff',
			28: '#660066, #800080, #990099, #b300b3, #cc66cc, #d999d9, #e6cce6, #f2e6f2',
			29: '#4d2600, #663300, #804000, #994d00, #b36600, #cc8033, #d99966, #e6b399',
			30: '#000000, #1a1a1a, #333333, #4d4d4d, #666666, #808080, #999999, #b3b3b3'
		};

		// Try to find and setup fields immediately and after delay
		this.trySetupFields();
		setTimeout(() => this.trySetupFields(), 100);
	}

	/**
	 * Find form fields and setup event handler
	 */
	trySetupFields() {
		const form = document.getElementById('widget-dialogue-form') || 
		             document.querySelector('form[name="widget_dialogue_form"]');
		
		const color_set = form?.querySelector('select[name="color_set"]') || 
		                  document.querySelector('select[name="color_set"]') ||
		                  document.getElementById('color_set');
		
		const graph_colors = form?.querySelector('input[name="graph_colors"]') ||
		                     document.querySelector('input[name="graph_colors"]') ||
		                     document.getElementById('graph_colors');

		if (color_set && graph_colors && !this._handler_installed) {
			this._color_set = color_set;
			this._graph_colors = graph_colors;
			
			this._color_set.addEventListener('change', () => {
				const selectedSet = parseInt(this._color_set.value);
				if (!isNaN(selectedSet) && selectedSet >= 0 && this._color_sets[selectedSet]) {
					this._graph_colors.value = this._color_sets[selectedSet];
					this._graph_colors.dispatchEvent(new Event('input', { bubbles: true }));
					this._graph_colors.dispatchEvent(new Event('change', { bubbles: true }));
				}
			});
			
			this._handler_installed = true;
		}
	}

	/**
	 * Setup Pattern Builder button
	 */
	setupPatternBuilder() {
		// Try to find and setup button immediately and after delays
		this.trySetupPatternBuilder();
		setTimeout(() => this.trySetupPatternBuilder(), 100);
		setTimeout(() => this.trySetupPatternBuilder(), 500);
	}

	/**
	 * Find item_pattern field and add Pattern Builder button
	 */
	trySetupPatternBuilder() {
		if (this._pattern_button_installed) {
			return;
		}

		const form = document.getElementById('widget-dialogue-form') || 
		             document.querySelector('form[name="widget_dialogue_form"]');
		
		const item_pattern = form?.querySelector('input[name="item_pattern"]') ||
		                     document.querySelector('input[name="item_pattern"]') ||
		                     document.getElementById('item_pattern');

		if (item_pattern && item_pattern.parentNode) {
			const button = document.createElement('button');
			button.type = 'button';
			button.className = 'btn-alt';
			button.textContent = 'Pattern Builder';
			button.style.marginLeft = '5px';
			
			item_pattern.parentNode.insertBefore(button, item_pattern.nextSibling);
			
			button.addEventListener('click', () => {
				this.openItemSelector();
			});
			
			this._pattern_button_installed = true;
			this._item_pattern = item_pattern;
		}
	}

	/**
	 * Open item selector dialog
	 */
	openItemSelector() {
		const form = document.getElementById('widget-dialogue-form') || 
		             document.querySelector('form[name="widget_dialogue_form"]') ||
		             this._item_pattern?.closest('form');
		
		// Check if we're on a template dashboard
		const urlParams = new URLSearchParams(window.location.search);
		const templateid = urlParams.get('templateid');
		const action = urlParams.get('action');
		
		// If action contains "template", we're on a template dashboard
		if (action && action.includes('template')) {
			// Try to get templateid from dashboard page context
			const dashboardElement = document.querySelector('[data-templateid]');
			if (dashboardElement) {
				const tid = dashboardElement.getAttribute('data-templateid');
				if (tid) {
					this.showItemList(tid, true);
					return;
				}
			}
			
			// Try to find it in the page's JS context
			if (window.templateid) {
				this.showItemList(window.templateid, true);
				return;
			}
			
			// Last resort: Get dashboardid and fetch template from breadcrumb
			const dashboardid = urlParams.get('dashboardid');
			if (dashboardid) {
				this.fetchTemplateFromDashboard(dashboardid);
				return;
			}
		}
		
		if (templateid) {
			this.showItemList(templateid, true);
			return;
		}
		
		// Find hostids from multiselect (for host dashboards)
		let hostid_inputs = form.querySelectorAll('input[name^="hostids["]');
		
		if (hostid_inputs.length === 0) {
			hostid_inputs = form.querySelectorAll('input[name="hostids_"]');
		}
		
		if (hostid_inputs.length === 0) {
			hostid_inputs = form.querySelectorAll('input[name*="hostids"]');
		}
		
		const hostids = [];
		hostid_inputs.forEach(input => {
			if (input.value) {
				hostids.push(input.value);
			}
		});

		if (hostids.length === 0) {
			overlayDialogue({
				'title': 'Error',
				'content': jQuery('<span>').text('Please select a host first, or use this on a template dashboard.'),
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
		this.showItemList(hostids[0], false);
	}

	/**
	 * Fetch template ID from dashboard by examining the page
	 */
	fetchTemplateFromDashboard(dashboardid) {
		// Check breadcrumb links for template
		const breadcrumbs = document.querySelectorAll('.breadcrumbs a');
		for (let i = 0; i < breadcrumbs.length; i++) {
			const href = breadcrumbs[i].getAttribute('href');
			if (href && href.includes('templateid=')) {
				const match = href.match(/[?&]templateid=(\d+)/);
				if (match) {
					this.showItemList(match[1], true);
					return;
				}
			}
		}
		
		// Check all links on page for templates.php
		const allLinks = document.querySelectorAll('a[href*="templates.php"]');
		for (let i = 0; i < allLinks.length; i++) {
			const href = allLinks[i].getAttribute('href');
			if (href && href.includes('templateid=')) {
				const match = href.match(/[?&]templateid=(\d+)/);
				if (match) {
					this.showItemList(match[1], true);
					return;
				}
			}
		}
		
		// Check for data attributes
		const dashboardDiv = document.querySelector('[data-templateid]');
		if (dashboardDiv) {
			const tid = dashboardDiv.getAttribute('data-templateid');
			this.showItemList(tid, true);
			return;
		}
		
		// Check all elements with data attributes
		const allDataElements = document.querySelectorAll('[data-templateid], [data-template-id]');
		for (let i = 0; i < allDataElements.length; i++) {
			const tid = allDataElements[i].getAttribute('data-templateid') || 
			            allDataElements[i].getAttribute('data-template-id');
			if (tid) {
				this.showItemList(tid, true);
				return;
			}
		}
		
		// Failed to find templateid
		overlayDialogue({
			'title': 'Error',
			'content': jQuery('<span>').text('Could not determine template ID. Please add a host filter to the widget first, or contact support.'),
			'buttons': [
				{
					'title': 'Ok',
					'focused': true,
					'action': function() {}
				}
			]
		});
	}

	/**
	 * Show item list via AJAX
	 */
	showItemList(hostid, isTemplate = false) {
		const form = document.getElementById('widget-dialogue-form') || 
		             document.querySelector('form[name="widget_dialogue_form"]') ||
		             this._item_pattern?.closest('form');
		const pattern_mode = form?.querySelector('input[name="pattern_mode"]:checked')?.value || '0';
		
		const curl = new Curl('zabbix.php');
		curl.setArgument('action', 'widget.multigraphwidget.itemlist');
		curl.setArgument(isTemplate ? 'templateid' : 'hostid', hostid);
		curl.setArgument('pattern_mode', pattern_mode);
		
		jQuery.ajax({
			url: curl.getUrl(),
			method: 'GET',
			dataType: 'json',
			success: (response) => {
				if (response.error) {
					const errorMsg = typeof response.error === 'string' 
						? response.error 
						: JSON.stringify(response.error);
					overlayDialogue({
						'title': 'Error',
						'content': jQuery('<span>').text(errorMsg),
						'buttons': [
							{
								'title': 'Ok',
								'focused': true,
								'action': function() {}
							}
						]
					});
				} else if (response.items && Array.isArray(response.items)) {
					this.displayItemSelector(response.items);
				} else {
					overlayDialogue({
						'title': 'Error',
						'content': jQuery('<span>').text('Unexpected response format from server'),
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
			error: (xhr, status, error) => {
				const errorMsg = xhr.responseText || error || 'Unknown error';
				overlayDialogue({
					'title': 'Error',
					'content': jQuery('<span>').text('Failed to load items: ' + errorMsg),
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
	 */
	displayItemSelector(items) {
		const content = jQuery('<div>').css({
			'max-height': '400px',
			'overflow-y': 'auto'
		});
		
		const list = jQuery('<ul>').css({
			'list-style': 'none',
			'padding': '0',
			'margin': '0'
		});
		
		items.forEach(item => {
			const li = jQuery('<li>').css({
				'padding': '5px 10px',
				'cursor': 'pointer',
				'border-bottom': '1px solid #eee'
			}).text(item.name);
			
			li.on('click', () => {
				const pattern = this.itemNameToPattern(item.name);
				if (this._item_pattern) {
					this._item_pattern.value = pattern;
					this._item_pattern.dispatchEvent(new Event('input', { bubbles: true }));
					this._item_pattern.dispatchEvent(new Event('change', { bubbles: true }));
				}
				
				// Close the dialog using the stored dialogue object
				if (this._currentDialogue && typeof this._currentDialogue.cancel === 'function') {
					this._currentDialogue.cancel();
				} else if (this._currentDialogue && typeof this._currentDialogue.close === 'function') {
					this._currentDialogue.close();
				} else if (this._currentDialogue && this._currentDialogue.$dialogue) {
					// Try multiple methods to remove the dialog
					if (typeof this._currentDialogue.unsetLoading === 'function') {
						this._currentDialogue.unsetLoading();
					}
					// Remove the actual DOM element
					jQuery('[data-dialogueid="item-selector"]').remove();
					// Also remove by ID if it has one
					if (this._currentDialogue.dialogueid) {
						jQuery('[data-dialogueid="' + this._currentDialogue.dialogueid + '"]').remove();
					}
					// Remove the dialogue's own element
					if (this._currentDialogue.$dialogue && this._currentDialogue.$dialogue.remove) {
						this._currentDialogue.$dialogue.remove();
					}
					// Remove associated overlay background
					const dialogZIndex = this._currentDialogue.$dialogue ? parseInt(this._currentDialogue.$dialogue.css('z-index')) : 0;
					jQuery('.overlay-bg').each(function() {
						const bgZIndex = parseInt(jQuery(this).css('z-index')) || 0;
						if (bgZIndex === dialogZIndex - 1) {
							jQuery(this).remove();
						}
					});
				}
				
				// Reactivate the widget configuration dialog
				const self = this;
				setTimeout(function() {
					// Find all remaining overlay dialogues
					const allDialogs = jQuery('.overlay-dialogue');
					
					// Find the widget configuration dialog
					const widgetDialog = allDialogs.filter(function() {
						return jQuery(this).find('.dashboard-widget-form').length > 0;
					}).first();
					
					if (widgetDialog.length) {
						// Remove inactive state
						widgetDialog.removeClass('inactive');
						widgetDialog.addClass('active');
						
						// Get all overlay backgrounds and find the highest z-index
						const overlayBgs = jQuery('.overlay-bg');
						let maxZIndex = 900;
						overlayBgs.each(function() {
							const zIndex = parseInt(jQuery(this).css('z-index')) || 0;
							if (zIndex > maxZIndex) {
								maxZIndex = zIndex;
							}
						});
						
						// Set widget dialog z-index higher than all overlays
						widgetDialog.css('z-index', maxZIndex + 1);
						
						// Make sure the corresponding overlay-bg is just below the dialog
						const widgetBg = overlayBgs.filter(function() {
							const zIndex = parseInt(jQuery(this).css('z-index')) || 0;
							return zIndex === maxZIndex;
						}).first();
						
						if (widgetBg.length) {
							widgetBg.css('z-index', maxZIndex);
						}
						
						// Enable interactions
						widgetDialog.find('.overlay-dialogue-body').css('pointer-events', 'auto');
						widgetDialog.find('.overlay-dialogue-footer').css('pointer-events', 'auto');
						
						// Try to trigger a click on the dialog to reactivate it
						widgetDialog.trigger('click');
						
						// Focus the item pattern field that was just updated
						if (self._item_pattern) {
							self._item_pattern.focus();
							self._item_pattern.select();
						}
					}
				}, 100);
			});
			
			li.hover(
				function() { jQuery(this).css('background-color', '#f0f0f0'); },
				function() { jQuery(this).css('background-color', 'transparent'); }
			);
			
			list.append(li);
		});
		
		content.append(list);
		
		const dialogue = overlayDialogue({
			'title': 'Select Item Pattern',
			'content': content,
			'buttons': [
				{
					'title': 'Cancel',
					'focused': true,
					'action': function() {}
				}
			],
			'dialogueid': 'item-selector'
		});
		
		// Store the dialogue reference for later closing
		this._currentDialogue = dialogue;
	}

	/**
	 * Convert item name to pattern
	 */
	itemNameToPattern(itemName) {
		const form = document.getElementById('widget-dialogue-form') || 
		             document.querySelector('form[name="widget_dialogue_form"]') ||
		             this._item_pattern?.closest('form');
		const pattern_mode = form?.querySelector('input[name="pattern_mode"]:checked')?.value || '0';
		
		// Replace {#MACROS} with wildcards or regex
		if (pattern_mode === '1') {
			// Regex mode: {#MACRO} -> .+
			return itemName.replace(/\{#[^}]+\}/g, '.+');
		} else {
			// Wildcard mode: {#MACRO} -> *
			return itemName.replace(/\{#[^}]+\}/g, '*');
		}
	}

	/**
	 * Initialize widget form
	 */
	init() {
		this.ready();
	}

};



