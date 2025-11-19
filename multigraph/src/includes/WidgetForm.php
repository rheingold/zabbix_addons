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
	public const COLOR_SET_CPU = 0;
	public const COLOR_SET_CPU2 = 1;
	public const COLOR_SET_RAM = 2;
	public const COLOR_SET_STORAGE = 3;
	public const COLOR_SET_STORAGE2 = 4;
	public const COLOR_SET_STORAGEHW = 5;
	public const COLOR_SET_NETWORK = 6;
	public const COLOR_SET_NETWORK2 = 7;
	public const COLOR_SET_SERVICES = 8;
	public const COLOR_SET_SERVICES2 = 9;
	public const COLOR_SET_SERVICES3 = 10;
	public const COLOR_SET_SERVICES4 = 11;
	public const COLOR_SET_SERVICES5 = 12;
	public const COLOR_SET_RAINBOW = 13;
	public const COLOR_SET_RAINBOW_INVERSE = 14;
	public const COLOR_SET_RED = 15;
	public const COLOR_SET_DARK_RED = 16;
	public const COLOR_SET_LIGHT_RED = 17;
	public const COLOR_SET_PINK = 18;
	public const COLOR_SET_DARK_GREEN = 19;
	public const COLOR_SET_GREEN = 20;
	public const COLOR_SET_LIGHT_GREEN = 21;
	public const COLOR_SET_DARK_BLUE = 22;
	public const COLOR_SET_BLUE = 23;
	public const COLOR_SET_LIGHT_BLUE = 24;
	public const COLOR_SET_YELLOW = 25;
	public const COLOR_SET_ORANGE = 26;
	public const COLOR_SET_VIOLET = 27;
	public const COLOR_SET_PURPLE = 28;
	public const COLOR_SET_BROWN = 29;
	public const COLOR_SET_GRAYSCALE = 30;

	/**
	 * Named color sets for quick selection
	 * Format: 'SetName:#RRGGBB,#RRGGBB,...'
	 * Name before colon is displayed in UI but ignored during rendering
	 * @const array Predefined color schemes with integer keys
	 */
	public const COLOR_SETS = [
		self::COLOR_SET_CPU => 'CPU:#000080,#9999ff,#3333b3,#b3b3ff,#1a1a99,#6666e6,#4d4dcc,#8080ff',
		self::COLOR_SET_CPU2 => 'CPU2:#1a1a1a,#573a2d,#0d0d0d,#3d2e26,#5a2e2e,#2d2420,#4d2626,#4a3429',
		self::COLOR_SET_RAM => 'RAM:#87ceeb,#00ced1,#b0e0e6,#00cca3,#87d3db,#40e0d0,#7dd3c0,#00e5cc',
		self::COLOR_SET_STORAGE => 'Storage/DB:#ff8c00,#ffa500,#d2a679,#cd853f,#f4d68e,#e6cc80,#f5deb3,#ffe4a3',
		self::COLOR_SET_STORAGE2 => 'Storage/DB2:#ffff00,#ffb300,#ffd700,#ffe680,#ffcc00,#ffe066,#ffdb4d,#ffcc66',
		self::COLOR_SET_STORAGEHW => 'StorageHW:#8b4513,#a0522d,#6b3410,#8b3a3a,#a0684a,#6d3030,#b88661,#5c2e2e',
		self::COLOR_SET_NETWORK => 'Network:#008b8b,#40e0d0,#00ced1,#7fffd4,#20b2aa,#afeeee,#48d1cc,#5fd8d8',
		self::COLOR_SET_NETWORK2 => 'Network2:#800080,#6a1a3a,#8b008b,#800020,#9932cc,#6b2d5c,#8b0058,#9400d3',
		self::COLOR_SET_SERVICES => 'Services:#9acd32,#8fbc3f,#7a9d2e,#aed850,#6b8e23,#c0e060,#5a7d1f,#b5d96b',
		self::COLOR_SET_SERVICES2 => 'Services2:#006400,#228b22,#004d00,#3cb371,#008000,#00662e,#2e8b57,#003d00',
		self::COLOR_SET_SERVICES3 => 'Services3:#39ff14,#7fff00,#28e028,#90ff90,#32cd32,#20b020,#00ff00,#57ff57',
		self::COLOR_SET_SERVICES4 => 'Services4:#013220,#005f56,#004d40,#006b54,#1f3f1f,#2c5c3c,#4a6b2f,#3d5c3d',
		self::COLOR_SET_SERVICES5 => 'Services5:#39ff14,#00ffff,#0dff92,#ccff00,#00ff9f,#adff2f,#00e5e5,#88ff00',
		self::COLOR_SET_RAINBOW => 'Rainbow:#ff0000,#ff8800,#ffff00,#00ff00,#00ffff,#0088ff,#8800ff,#ff00ff',
		self::COLOR_SET_RAINBOW_INVERSE => 'Rainbow Inverse:#ff00ff,#8800ff,#0088ff,#00ffff,#00ff00,#ffff00,#ff8800,#ff0000',
		self::COLOR_SET_RED => 'Red:#8b0000,#a02020,#cd0000,#ff0000,#ff3333,#ff6666,#ff9999,#ffcccc',
		self::COLOR_SET_DARK_RED => 'Dark Red:#5c0000,#6b0000,#7a0000,#8b0000,#9a1515,#a02020,#b03030,#c04040',
		self::COLOR_SET_LIGHT_RED => 'Light Red:#ff6666,#ff7777,#ff8888,#ff9999,#ffaaaa,#ffbbbb,#ffcccc,#ffdddd',
		self::COLOR_SET_PINK => 'Pink:#ff1493,#ff3399,#ff66b3,#ff69b4,#ff99cc,#ffb3d9,#ffcce6,#ffe6f2',
		self::COLOR_SET_DARK_GREEN => 'Dark Green:#003300,#004400,#005500,#006400,#007300,#008200,#009100,#00a000',
		self::COLOR_SET_GREEN => 'Green:#008000,#009900,#00b300,#00cc00,#00e600,#00ff00,#66ff66,#99ff99',
		self::COLOR_SET_LIGHT_GREEN => 'Light Green:#66ff66,#77ff77,#88ff88,#99ff99,#aaffaa,#bbffbb,#ccffcc,#ddffdd',
		self::COLOR_SET_DARK_BLUE => 'Dark Blue:#000066,#000080,#00009a,#0000b3,#0000cd,#1a1aff,#3333ff,#4d4dff',
		self::COLOR_SET_BLUE => 'Blue:#0000ff,#1a1aff,#3333ff,#4d4dff,#6666ff,#8080ff,#9999ff,#b3b3ff',
		self::COLOR_SET_LIGHT_BLUE => 'Light Blue:#6666ff,#7777ff,#87ceeb,#99ccff,#aaddff,#bbddff,#cceeff,#e6f5ff',
		self::COLOR_SET_YELLOW => 'Yellow:#cccc00,#e6e600,#ffff00,#ffff33,#ffff66,#ffff99,#ffffcc,#ffffe6',
		self::COLOR_SET_ORANGE => 'Orange:#cc6600,#ff7700,#ff8800,#ff9933,#ffaa66,#ffbb99,#ffccaa,#ffddcc',
		self::COLOR_SET_VIOLET => 'Violet:#6600cc,#7700ee,#8800ff,#9933ff,#aa66ff,#bb99ff,#ccbbff,#ddccff',
		self::COLOR_SET_PURPLE => 'Purple:#660066,#800080,#990099,#b300b3,#cc66cc,#d999d9,#e6cce6,#f2e6f2',
		self::COLOR_SET_BROWN => 'Brown:#4d2600,#663300,#804000,#994d00,#b36600,#cc8033,#d99966,#e6b399',
		self::COLOR_SET_GRAYSCALE => 'Grayscale:#000000,#1a1a1a,#333333,#4d4d4d,#666666,#808080,#999999,#b3b3b3'
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
					[-1 => _('— Select color set —')] + array_map(function($v) { return explode(':', $v)[0]; }, self::COLOR_SETS)
				))
					->setDefault(-1)
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
