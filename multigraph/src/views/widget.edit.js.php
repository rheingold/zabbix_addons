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
			0: '#1f77b4,#ff7f0e,#2ca02c,#d62728,#9467bd,#8c564b,#e377c2,#7f7f7f,#bcbd22,#17becf',
			1: '#aec7e8,#ffbb78,#98df8a,#ff9896,#c5b0d5,#c49c94,#f7b6d2,#c7c7c7,#dbdb8d,#9edae5',
			2: '#e60049,#0bb4ff,#50e991,#e6d800,#9b19f5,#ffa300,#dc0ab4,#b3d4ff,#00bfa0',
			3: '#8b4513,#daa520,#228b22,#4682b4,#d2691e,#708238,#cd853f,#556b2f,#b8860b',
			4: '#003f5c,#2f4b7c,#665191,#a05195,#d45087,#f95d6a,#ff7c43,#ffa600',
			5: '#ff6b6b,#ff8e53,#ffbe0b,#fb5607,#ff006e,#8338ec,#3a86ff',
			6: '#1b4332,#2d6a4f,#40916c,#52b788,#74c69d,#95d5b2,#b7e4c7,#d8f3dc',
			7: '#000000,#2d2d2d,#5a5a5a,#878787,#b4b4b4,#e1e1e1,#ffffff'
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
				if (this._color_sets[selectedSet]) {
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



