<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: WidgetEdit.php
 * Created: 2025-11-09 19:30:00
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * Widget edit controller - handles widget configuration form requests.
 * Prepares form fields and renders configuration dialog.
 * 
 * RELATIONS:
 * - Extends: CControllerDashboardWidgetEdit (Zabbix MVC framework)
 * - Uses: includes/WidgetForm.php for field definitions
 * - Renders: views/widget.edit.php template
 * - Called by: Zabbix dashboard when user clicks widget configuration
 * 
 * DATA FLOW:
 * 1. Receives existing widget configuration
 * 2. Creates form with current values
 * 3. Prepares fields for view
 * 4. Passes to edit template
 */

namespace Widgets\Multigraph\Actions;

use CControllerDashboardWidgetEdit; // Zabbix base controller for widget edit actions
use CControllerResponseData; // Response wrapper for passing data to views
use Widgets\Multigraph\Includes\WidgetForm; // Widget form field definitions
use Zabbix\Widgets\Fields\{ // Zabbix widget field types for manual field creation
	CWidgetFieldTextBox,      // Text input fields
	CWidgetFieldRadioButtonList // Radio button selection fields
};

/**
 * Multigraph Widget Edit Controller
 * 
 * Handles HTTP requests for widget configuration form display.
 * Implements MVC Controller pattern for Zabbix dashboard widgets.
 */
class WidgetEdit extends CControllerDashboardWidgetEdit {

	/**
	 * Main action - prepare and display widget configuration form
	 * 
	 * PURPOSE:
	 * 1. Create form with existing widget values
	 * 2. Ensure all fields are properly initialized
	 * 3. Handle field defaults and null values
	 * 4. Pass prepared fields to edit template
	 * 
	 * DEPENDENCIES:
	 * - Called by: Zabbix MVC framework when configuration dialog opens
	 * - Creates: WidgetForm instance with current widget values
	 * - Renders: views/widget.edit.php
	 * 
	 * WORKAROUND:
	 * Manually creates color fields if not present - handles edge case
	 * where fieldsToView() might return null for certain field types.
	 * 
	 * @return void
	 */
	protected function doAction(): void {
		$form = new WidgetForm($this->widget->fields_values); // Initialize form with current widget configuration
		$fields = $form->fieldsToView(); // Convert form fields to view-ready format
		
		// Manually create color fields if they don't exist or are null
		if (!isset($fields['graph_colors']) || $fields['graph_colors'] === null) {
			$graph_colors_field = new CWidgetFieldTextBox('graph_colors', _('Graph colors'));
			$graph_colors_field->setValue($this->widget->fields_values['graph_colors'] ?? '#1f77b4,#ff7f0e,#2ca02c,#d62728,#9467bd');
			$fields['graph_colors'] = $graph_colors_field;
		}
		
		if (!isset($fields['color_mode']) || $fields['color_mode'] === null) {
			$color_mode_field = new CWidgetFieldRadioButtonList('color_mode', _('Color mode'), [
				'cycle' => _('Cycle through colors'),
				'random' => _('Random colors'),
				'offset' => _('Offset shade of last color')
			]);
			$color_mode_field->setValue($this->widget->fields_values['color_mode'] ?? 'cycle');
			$fields['color_mode'] = $color_mode_field;
		}

		// Pass context to view for conditional field display
		$response_data = [
			'name' => $this->getInput('name', $this->widget->getDefaultName()),
			'fields' => $fields
		];
		
		$this->setResponse(new CControllerResponseData($response_data));
	}
}
