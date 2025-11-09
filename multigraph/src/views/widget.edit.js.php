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

	/**
	 * Initialize widget form
	 * 
	 * Minimal implementation - see assets/js/widget.edit.js for full logic.
	 * 
	 * @returns {void}
	 */
	init() {
		this.ready(); // Signal form ready to Zabbix framework
	}

};


