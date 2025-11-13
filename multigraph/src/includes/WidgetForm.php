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
	CWidgetFieldMultiSelectHost,   // Host picker with multi-selection
	CWidgetFieldRadioButtonList,   // Radio button group selection
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
	 * - item_pattern: Required in host dashboards, optional in template dashboards
	 * - Other fields: Validated by parent (type checking, ranges, etc.)
	 * 
	 * CRITICAL: Template Dashboard Support
	 * - In template dashboards, hostid is not available (templates don't have hosts until applied)
	 * - Item pattern validation is skipped when in template context
	 * - Detection: Check if hasInput('templateid') or context contains templateid
	 * 
	 * @param bool $strict Whether to perform strict validation
	 * 
	 * @return array Array of error messages, empty if validation passes
	 */
	public function validate(bool $strict = false): array {
		$errors = parent::validate($strict); // Perform base validation (field types, ranges, etc.)

		if ($errors) {
			return $errors;
		}

		// Check if we're in a template dashboard context
		// In template dashboards, hostids and item patterns may not be fully specified yet
		$is_template_dashboard = $this->hasInput('templateid') || 
		                          (method_exists($this, 'getContext') && 
		                           isset($this->getContext()['templateid']));

		// Item pattern is required only in non-template dashboards
		if (!$is_template_dashboard && !$this->getFieldValue('item_pattern')) {
			$errors[] = _s('Invalid parameter "%1$s": %2$s.', _('Item pattern'), _('cannot be empty'));
		}

		return $errors;
	}

	public function addFields(): self {
		return $this
			->addField(
				(new CWidgetFieldMultiSelectHost('hostids', _('Host')))
					->setMultiple(false)
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
					->setDefault('#000000')
			)
			->addField(
				(new CWidgetFieldTextBox('text_color_xaxis', _('X-axis text color')))
					->setDefault('#000000')
			)
			->addField(
				(new CWidgetFieldTextBox('text_color_yaxis', _('Y-axis text color')))
					->setDefault('#000000')
			);
	}
}
