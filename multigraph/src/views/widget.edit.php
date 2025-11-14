<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: widget.edit.php
 * Created: 2025-11-09 19:30:00
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * Widget edit form template - renders configuration form in dashboard edit mode.
 * Builds Zabbix form with all widget configuration fields.
 * 
 * RELATIONS:
 * - Called by: actions/WidgetEdit.php doAction() method
 * - Receives data from: actions/WidgetEdit.php via $data array
 * - Uses fields from: includes/WidgetForm.php field definitions
 * - Extended by: views/widget.edit.js.php (JavaScript initialization)
 * - Extends: CWidgetFormView (Zabbix widget form framework)
 * 
 * DATA STRUCTURE:
 * $data = [
 *   'widgetid' => Widget instance ID
 *   'name' => Widget name
 *   'fields' => [
 *     'hostids' => CWidgetFieldMultiSelectHost,
 *     'item_pattern' => CWidgetFieldTextBox,
 *     'pattern_mode' => CWidgetFieldRadioButtonList,
 *     'time_period' => CWidgetFieldTimePeriod,
 *     'show_legend' => CWidgetFieldCheckBox,
 *     ... (see WidgetForm.php for all fields)
 *   ]
 * ]
 * 
 * FORM FIELDS:
 * Required fields:
 * - hostids: Multi-select host picker
 * - item_pattern: Text pattern for matching items
 * - pattern_mode: Wildcard (0) or Regex (1)
 * - time_period: Dashboard-integrated time selector
 * - show_legend: Enable/disable legend
 * 
 * Optional fields (conditionally added):
 * - legend_position, graph_colors, color_mode, fill_opacity
 * - y_axis_label, y_min, y_max, show_grid, grid_density
 * - text colors (legend, x-axis, y-axis)
 * 
 * @var CView $this Zabbix view object
 * @var array $data Widget data from controller
 */

// Create form view with all required fields
$form = new CWidgetFormView($data);

// === REQUIRED FIELDS ===

// Always show hostids field - FOREIGN_REFERENCE_KEY will auto-fill from dashboard when left empty
$form->addField(new CWidgetFieldMultiSelectHostView($data['fields']['hostids']));

$form
	->addField(new CWidgetFieldTextBoxView($data['fields']['item_pattern']))        // Item name pattern
	->addField(new CWidgetFieldRadioButtonListView($data['fields']['pattern_mode'])) // Wildcard/Regex
	->addField(new CWidgetFieldTimePeriodView($data['fields']['time_period']))      // Time range
	->addField(new CWidgetFieldRadioButtonListView($data['fields']['missing_data'])) // Missing data handling
	->addField(new CWidgetFieldRadioButtonListView($data['fields']['graph_type']))  // Graph type (line/bar)
	->addField(new CWidgetFieldCheckBoxView($data['fields']['show_legend']));       // Show/hide legend

// === OPTIONAL FIELDS: Bar Chart Configuration ===
if (isset($data['fields']['bar_separation'])) {
	$form->addField(new CWidgetFieldIntegerBoxView($data['fields']['bar_separation'])); // Bar separation (only for bar chart)
}

if (isset($data['fields']['bar_display_mode'])) {
	$form->addField(new CWidgetFieldRadioButtonListView($data['fields']['bar_display_mode'])); // Grouped or stacked (only for bar chart)
}

// === OPTIONAL FIELDS: Distribution Configuration ===
if (isset($data['fields']['distribution_bins'])) {
	$form->addField(new CWidgetFieldIntegerBoxView($data['fields']['distribution_bins'])); // Number of bins (only for distribution)
}

// === OPTIONAL FIELDS: Legend Configuration ===
if (isset($data['fields']['legend_position'])) {
	$form->addField(new CWidgetFieldRadioButtonListView($data['fields']['legend_position']));
}

// === OPTIONAL FIELDS: Color Configuration ===
if (isset($data['fields']['color_set'])) {
	$form->addField(new CWidgetFieldSelectView($data['fields']['color_set']));
}

if (isset($data['fields']['graph_colors'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['graph_colors']));
}

if (isset($data['fields']['color_mode'])) {
	$form->addField(new CWidgetFieldRadioButtonListView($data['fields']['color_mode']));
}

if (isset($data['fields']['fill_opacity'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['fill_opacity']));
}

// === OPTIONAL FIELDS: Y-Axis Configuration ===
if (isset($data['fields']['y_axis_label'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['y_axis_label']));
}

if (isset($data['fields']['y_min'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['y_min']));
}

if (isset($data['fields']['y_max'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['y_max']));
}

// === OPTIONAL FIELDS: Grid Configuration ===
if (isset($data['fields']['show_grid'])) {
	$form->addField(new CWidgetFieldCheckBoxView($data['fields']['show_grid']));
}

if (isset($data['fields']['grid_density'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['grid_density']));
}

if (isset($data['fields']['text_color_legend'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['text_color_legend']));
}

if (isset($data['fields']['text_color_xaxis'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['text_color_xaxis']));
}

if (isset($data['fields']['text_color_yaxis'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['text_color_yaxis']));
}

$form->includeJsFile('widget.edit.js.php')->show();
