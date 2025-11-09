<?php declare(strict_types = 0);

/**
 * Multigraph widget edit form view
 *
 * @var CView $this
 * @var array $data
 */

$form = new CWidgetFormView($data);

$form
	->addField(new CWidgetFieldMultiSelectHostView($data['fields']['hostids']))
	->addField(new CWidgetFieldTextBoxView($data['fields']['item_pattern']))
	->addField(new CWidgetFieldRadioButtonListView($data['fields']['pattern_mode']))
	->addField(new CWidgetFieldTimePeriodView($data['fields']['time_period']))
	->addField(new CWidgetFieldCheckBoxView($data['fields']['show_legend']));

// Add all optional fields if they exist
if (isset($data['fields']['legend_position'])) {
	$form->addField(new CWidgetFieldRadioButtonListView($data['fields']['legend_position']));
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

if (isset($data['fields']['y_axis_label'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['y_axis_label']));
}

if (isset($data['fields']['y_min'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['y_min']));
}

if (isset($data['fields']['y_max'])) {
	$form->addField(new CWidgetFieldTextBoxView($data['fields']['y_max']));
}

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
