<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: WidgetForm.php
 * Created: 2025-11-09 19:30:00
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * Widget configuration form definition - defines all widget configuration fields,
 * their types, validation rules, and default values.
 * 
 * RELATIONS:
 * - Extends: CWidgetForm (Zabbix widget framework)
 * - Used by: actions/WidgetEdit.php controller
 * - Rendered by: views/widget.edit.php template
 * - Constants used by: includes/GraphData.php, includes/MatchedItemsData.php
 * 
 * FIELD DEFINITIONS:
 * - Host selection: Multi-select host picker
 * - Pattern matching: Text input + radio (wildcard/regex)
 * - Graph appearance: Colors, legend, fills, grid
 * - Y-axis: Label, min, max
 * - Text colors: Legend, X-axis, Y-axis
 * 
 * IMPORTANT:
 * - Use integer constants for RadioButtonList (NOT strings!)
 * - Validation rules defined in validate() method
 * - Defaults set via setDefault() on fields
 */

namespace Widgets\Multigraph\Includes;

use Zabbix\Widgets\{ // Zabbix widget framework base classes
	CWidgetField,     // Base field class
	CWidgetForm       // Base form class providing field management
};

use Zabbix\Widgets\Fields\{ // Zabbix widget field types
	CWidgetFieldCheckBox,          // Boolean checkbox fields
	CWidgetFieldIntegerBox,        // Integer input with min/max validation
	CWidgetFieldMultiSelectHost,   // Host picker with multi-selection
	CWidgetFieldRadioButtonList,   // Radio button group selection
	CWidgetFieldSelect,             // Dropdown select field
	CWidgetFieldTextBox,            // Single-line text input
	CWidgetFieldTimePeriod          // Time period picker
};

use CWidgetsData; // Zabbix widgets data helper (for field processing)

/**
 * Multigraph Widget Form
 * 
 * Defines all configuration fields for the widget.
 * Handles field creation, validation, and default values.
 */
class WidgetForm extends CWidgetForm {

	/**
	 * Pattern mode: Wildcard matching (* and ?)
	 * @const int Used in pattern_mode RadioButtonList
	 */
	public const PATTERN_MODE_WILDCARD = 0;
	
	/**
	 * Pattern mode: Regular expression (PCRE)
	 * @const int Used in pattern_mode RadioButtonList
	 */
	public const PATTERN_MODE_REGEX = 1;

	/**
	 * Color mode: Cycle through colors sequentially
	 * @const int Used in color_mode RadioButtonList
	 */
	public const COLOR_MODE_CYCLE = 0;
	
	/**
	 * Color mode: Random color assignment
	 * @const int Used in color_mode RadioButtonList
	 */
	public const COLOR_MODE_RANDOM = 1;
	
	/**
	 * Color mode: Cycle with offset
	 * @const int Used in color_mode RadioButtonList
	 */
	public const COLOR_MODE_OFFSET = 2;

	/**
	 * Color set constants for dropdown selection
	 * @const int Color set identifiers
	 */
	public const COLOR_SET_DEFAULT = 0;
	public const COLOR_SET_PASTEL = 1;
	public const COLOR_SET_VIBRANT = 2;
	public const COLOR_SET_EARTH = 3;
	public const COLOR_SET_OCEAN = 4;
	public const COLOR_SET_SUNSET = 5;
	public const COLOR_SET_FOREST = 6;
	public const COLOR_SET_MONOCHROME = 7;

	/**
	 * Named color sets for quick selection
	 * Format: 'SetName:#RRGGBB,#RRGGBB,...'
	 * Name before colon is displayed in UI but ignored during rendering
	 * @const array Predefined color schemes with integer keys
	 */
	public const COLOR_SETS = [
		self::COLOR_SET_DEFAULT => 'Default:#1f77b4,#ff7f0e,#2ca02c,#d62728,#9467bd,#8c564b,#e377c2,#7f7f7f,#bcbd22,#17becf',
		self::COLOR_SET_PASTEL => 'Pastel:#aec7e8,#ffbb78,#98df8a,#ff9896,#c5b0d5,#c49c94,#f7b6d2,#c7c7c7,#dbdb8d,#9edae5',
		self::COLOR_SET_VIBRANT => 'Vibrant:#e60049,#0bb4ff,#50e991,#e6d800,#9b19f5,#ffa300,#dc0ab4,#b3d4ff,#00bfa0',
		self::COLOR_SET_EARTH => 'Earth:#8b4513,#daa520,#228b22,#4682b4,#d2691e,#708238,#cd853f,#556b2f,#b8860b',
		self::COLOR_SET_OCEAN => 'Ocean:#003f5c,#2f4b7c,#665191,#a05195,#d45087,#f95d6a,#ff7c43,#ffa600',
		self::COLOR_SET_SUNSET => 'Sunset:#ff6b6b,#ff8e53,#ffbe0b,#fb5607,#ff006e,#8338ec,#3a86ff',
		self::COLOR_SET_FOREST => 'Forest:#1b4332,#2d6a4f,#40916c,#52b788,#74c69d,#95d5b2,#b7e4c7,#d8f3dc',
		self::COLOR_SET_MONOCHROME => 'Monochrome:#000000,#2d2d2d,#5a5a5a,#878787,#b4b4b4,#e1e1e1,#ffffff'
	];

	/**
	 * Missing data: Leave gaps (don't draw line)
	 * @const int Used in missing_data RadioButtonList
	 */
	public const MISSING_DATA_NONE = 0;
	
	/**
	 * Missing data: Connect across gaps
	 * @const int Used in missing_data RadioButtonList
	 */
	public const MISSING_DATA_CONNECTED = 1;
	
	/**
	 * Missing data: Treat as zero value
	 * @const int Used in missing_data RadioButtonList
	 */
	public const MISSING_DATA_ZERO = 2;

	/**
	 * Graph type: Line chart
	 * @const int Used in graph_type RadioButtonList
	 */
	public const GRAPH_TYPE_LINE = 0;
	
	/**
	 * Graph type: Bar chart
	 * @const int Used in graph_type RadioButtonList
	 */
	public const GRAPH_TYPE_BAR = 1;
	
	/**
	 * Graph type: Distribution/Histogram
	 * @const int Used in graph_type RadioButtonList
	 */
	public const GRAPH_TYPE_DISTRIBUTION = 2;

	/**
	 * Bar display mode: Grouped (side-by-side)
	 * @const int Used in bar_display_mode RadioButtonList
	 */
	public const BAR_DISPLAY_GROUPED = 0;

	/**
	 * Bar display mode: Stacked
	 * @const int Used in bar_display_mode RadioButtonList
	 */
	public const BAR_DISPLAY_STACKED = 1;

	/**
	 * Legend position: Top-left corner
	 * @const int Used in legend_position RadioButtonList
	 */
	public const LEGEND_POS_TOP_LEFT = 0;
	
	/**
	 * Legend position: Right side
	 * @const int Used in legend_position RadioButtonList
	 */
	public const LEGEND_POS_RIGHT = 1;
	
	/**
	 * Legend position: Bottom
	 * @const int Used in legend_position RadioButtonList
	 */
	public const LEGEND_POS_BOTTOM = 2;

	/**
	 * Validate form fields
	 * 
	 * PURPOSE:
	 * Ensures all required fields have valid values before saving widget configuration.
	 * 
	 * DEPENDENCIES:
	 * - Called by: Zabbix framework before saving widget
	 * - Calls: parent::validate() for base validation
	 * 
	 * VALIDATION RULES:
	 * - item_pattern: No longer enforced as required (removed FLAG_NOT_EMPTY)
	 *   - Allows empty pattern for template dashboards
	 *   - WidgetView will handle empty pattern gracefully (show appropriate message)
	 * - Other fields: Validated by parent (type checking, ranges, etc.)
	 * 
	 * TEMPLATE DASHBOARD SUPPORT:
	 * - Template dashboards can save widgets without item pattern
	 * - Pattern can be configured after dashboard is applied to hosts
	 * 
	 * @param bool $strict Whether to perform strict validation
	 * 
	 * @return array Array of error messages, empty if validation passes
	 */
	public function validate(bool $strict = false): array {
		$errors = parent::validate($strict); // Perform base validation (field types, ranges, etc.)

		// No additional validation needed - parent handles all field type checks
		// Item pattern is optional (FLAG_NOT_EMPTY removed) to support template dashboards
		
		return $errors;
	}

	public function addFields(): self {
		return $this
			->addField(
				(new CWidgetFieldMultiSelectHost('hostids', _('Host')))
					->setMultiple(false)
					->setDefault([
						CWidgetField::FOREIGN_REFERENCE_KEY => CWidgetField::createTypedReference(
							CWidgetField::REFERENCE_DASHBOARD, CWidgetsData::DATA_TYPE_HOST_ID
						)
					])
			)
			->addField(
				(new CWidgetFieldTextBox('item_pattern', _('Item pattern')))
					// Remove FLAG_NOT_EMPTY to allow empty pattern in template dashboards
					// Validation is handled in validate() method based on context
					->setFlags(CWidgetField::FLAG_LABEL_ASTERISK)
			)
			->addField(
				(new CWidgetFieldRadioButtonList('pattern_mode', _('Pattern mode'), [
					self::PATTERN_MODE_WILDCARD => _('Wildcard'),
					self::PATTERN_MODE_REGEX => _('Regular expression')
				]))
					->setDefault(self::PATTERN_MODE_WILDCARD)
			)
			->addField(
				(new CWidgetFieldTimePeriod('time_period', _('Time period')))
					->setDefault([
						CWidgetField::FOREIGN_REFERENCE_KEY => CWidgetField::createTypedReference(
							CWidgetField::REFERENCE_DASHBOARD, CWidgetsData::DATA_TYPE_TIME_PERIOD
						)
					])
					->setDefaultPeriod(['from' => 'now-1h', 'to' => 'now'])
					->setFlags(CWidgetField::FLAG_NOT_EMPTY | CWidgetField::FLAG_LABEL_ASTERISK)
			)
			->addField(
				(new CWidgetFieldRadioButtonList('missing_data', _('Missing data'), [
					self::MISSING_DATA_NONE => _('None'),
					self::MISSING_DATA_CONNECTED => _('Connected'),
					self::MISSING_DATA_ZERO => _('Treat as 0')
				]))
					->setDefault(self::MISSING_DATA_CONNECTED)
			)
			->addField(
				(new CWidgetFieldRadioButtonList('graph_type', _('Graph type'), [
					self::GRAPH_TYPE_LINE => _('Line'),
					self::GRAPH_TYPE_BAR => _('Bar'),
					self::GRAPH_TYPE_DISTRIBUTION => _('Distribution')
				]))
					->setDefault(self::GRAPH_TYPE_LINE)
			)
			->addField(
				(new CWidgetFieldIntegerBox('bar_separation', _('Bar separation (px)')))
					->setDefault(5)
			)
			->addField(
				(new CWidgetFieldRadioButtonList('bar_display_mode', _('Bar display mode'), [
					self::BAR_DISPLAY_GROUPED => _('Grouped'),
					self::BAR_DISPLAY_STACKED => _('Stacked')
				]))
					->setDefault(self::BAR_DISPLAY_GROUPED)
			)
			->addField(
				(new CWidgetFieldIntegerBox('distribution_bins', _('Number of bins')))
					->setDefault(10)
			)
			->addField(
				(new CWidgetFieldCheckBox('show_legend', _('Show legend')))->setDefault(1)
			)
			->addField(
				(new CWidgetFieldRadioButtonList('legend_position', _('Legend position'), [
					self::LEGEND_POS_TOP_LEFT => _('Top left'),
					self::LEGEND_POS_RIGHT => _('Right'),
					self::LEGEND_POS_BOTTOM => _('Bottom')
				]))
					->setDefault(self::LEGEND_POS_TOP_LEFT)
			)
			->addField(
				(new CWidgetFieldTextBox('graph_colors', _('Graph colors')))
					->setDefault('#1f77b4,#ff7f0e,#2ca02c,#d62728,#9467bd')
			)
			->addField(
				(new CWidgetFieldSelect('color_set', _('Color set'), 
					array_map(function($v) { return explode(':', $v)[0]; }, self::COLOR_SETS)
				))
					->setDefault(self::COLOR_SET_DEFAULT)
			)
			->addField(
				(new CWidgetFieldRadioButtonList('color_mode', _('Color mode'), [
					self::COLOR_MODE_CYCLE => _('Cycle through colors'),
					self::COLOR_MODE_RANDOM => _('Random colors'),
					self::COLOR_MODE_OFFSET => _('Offset shade of last color')
				]))
					->setDefault(self::COLOR_MODE_CYCLE)
			)
			->addField(
				(new CWidgetFieldTextBox('fill_opacity', _('Fill opacity (%)')))
					->setDefault('20')
			)
			->addField(
				(new CWidgetFieldTextBox('y_axis_label', _('Y-axis label')))
					->setDefault('')
			)
			->addField(
				(new CWidgetFieldTextBox('y_min', _('Y-axis min')))
					->setDefault('auto')
			)
			->addField(
				(new CWidgetFieldTextBox('y_max', _('Y-axis max')))
					->setDefault('auto')
			)
			->addField(
				(new CWidgetFieldCheckBox('show_grid', _('Show grid')))->setDefault(1)
			)
			->addField(
				(new CWidgetFieldTextBox('grid_density', _('Grid density')))
					->setDefault('auto')
			)
			->addField(
				(new CWidgetFieldTextBox('text_color_legend', _('Legend text color')))
					->setDefault('#909090')
			)
			->addField(
				(new CWidgetFieldTextBox('text_color_xaxis', _('X-axis text color')))
					->setDefault('#909090')
			)
			->addField(
				(new CWidgetFieldTextBox('text_color_yaxis', _('Y-axis text color')))
					->setDefault('#909090')
			);
	}
}
