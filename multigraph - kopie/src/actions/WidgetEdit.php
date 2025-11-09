<?php declare(strict_types = 0);

namespace Widgets\Multigraph\Actions;

use CControllerDashboardWidgetEdit;
use CControllerResponseData;
use Widgets\Multigraph\Includes\WidgetForm;
use Zabbix\Widgets\Fields\{
	CWidgetFieldTextBox,
	CWidgetFieldRadioButtonList
};

/**
 * Multigraph widget edit controller
 */
class WidgetEdit extends CControllerDashboardWidgetEdit {

	protected function doAction(): void {
		$form = new WidgetForm($this->widget->fields_values);
		$fields = $form->fieldsToView();
		
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

		$this->setResponse(new CControllerResponseData([
			'name' => $this->getInput('name', $this->widget->getDefaultName()),
			'fields' => $fields
		]));
	}
}
