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
		const form = document.getElementById('widget-dialogue-form') || this._form;
		
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
	 * Initialize widget form
	 */
	init() {
		this.ready();
	}

};



