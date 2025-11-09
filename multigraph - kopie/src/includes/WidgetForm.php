<?php declare(strict_types = 0);

namespace Widgets\Multigraph\Includes;

use Zabbix\Widgets\{
	CWidgetField,
	CWidgetForm
};

use Zabbix\Widgets\Fields\{
	CWidgetFieldCheckBox,
	CWidgetFieldMultiSelectHost,
	CWidgetFieldRadioButtonList,
	CWidgetFieldTextBox,
	CWidgetFieldTimePeriod
};

use CWidgetsData;

/**
 * Multigraph widget form
 */
class WidgetForm extends CWidgetForm {

	// Pattern matching modes
	public const PATTERN_MODE_WILDCARD = 0;
	public const PATTERN_MODE_REGEX = 1;

	// Color modes
	public const COLOR_MODE_CYCLE = 0;
	public const COLOR_MODE_RANDOM = 1;
	public const COLOR_MODE_OFFSET = 2;

	// Legend positions
	public const LEGEND_POS_TOP_LEFT = 0;
	public const LEGEND_POS_RIGHT = 1;
	public const LEGEND_POS_BOTTOM = 2;

	public function validate(bool $strict = false): array {
		$errors = parent::validate($strict);

		if ($errors) {
			return $errors;
		}

		// Item pattern is always required
		if (!$this->getFieldValue('item_pattern')) {
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
					->setFlags(CWidgetField::FLAG_NOT_EMPTY | CWidgetField::FLAG_LABEL_ASTERISK)
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
