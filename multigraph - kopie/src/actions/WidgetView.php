<?php declare(strict_types = 0);

namespace Widgets\Multigraph\Actions;

use CControllerDashboardWidgetView;
use CControllerResponseData;
use Widgets\Multigraph\Includes\ItemMatcher;
use Widgets\Multigraph\Includes\GraphRenderer;

/**
 * Multigraph widget view controller
 */
class WidgetView extends CControllerDashboardWidgetView {

	protected function init(): void {
		parent::init();
		
		$this->addValidationRules([
			'from' => 'string',
			'to' => 'string'
		]);
	}

	protected function doAction(): void {
		// Get widget field values
		$hostids = $this->fields_values['hostids'];
		$item_pattern = $this->fields_values['item_pattern'] ?? '';
		$pattern_mode = (int)($this->fields_values['pattern_mode'] ?? 0);
		$show_legend = $this->fields_values['show_legend'] ?? 1;
		$legend_position = $this->fields_values['legend_position'] ?? 0;
		$graph_colors = $this->fields_values['graph_colors'] ?? '#1f77b4,#ff7f0e,#2ca02c,#d62728,#9467bd';
		$color_mode = $this->fields_values['color_mode'] ?? 0;
		$fill_opacity = $this->fields_values['fill_opacity'] ?? '20';
		$y_axis_label = $this->fields_values['y_axis_label'] ?? '';
		$y_min = $this->fields_values['y_min'] ?? 'auto';
		$y_max = $this->fields_values['y_max'] ?? 'auto';
		$show_grid = $this->fields_values['show_grid'] ?? 1;
		$grid_density = $this->fields_values['grid_density'] ?? 'auto';
		$text_color_legend = $this->fields_values['text_color_legend'] ?? '#000000';
		$text_color_xaxis = $this->fields_values['text_color_xaxis'] ?? '#000000';
		$text_color_yaxis = $this->fields_values['text_color_yaxis'] ?? '#000000';
		
		// If no host selected, use dashboard context (template/host dashboard)
		if (empty($hostids) && $this->hasInput('hostid')) {
			$hostids = [$this->getInput('hostid')];
		}

		// Get time range from dashboard time selector or widget time period
		$time_period = $this->fields_values['time_period'];
		
		if ($this->hasInput('from') && $this->hasInput('to')) {
			// Use dashboard time selector
			$from_input = $this->getInput('from');
			$to_input = $this->getInput('to');
			
			// Parse as timestamps (could be strings like "now-1h" or actual timestamps)
			if (is_numeric($from_input)) {
				$time_from = (int)$from_input;
			} else {
				$time_from = strtotime($from_input) ?: (time() - 3600);
			}
			
			if (is_numeric($to_input)) {
				$time_till = (int)$to_input;
			} else {
				$time_till = strtotime($to_input) ?: time();
			}
		} else {
			// Use widget's time period setting
			$time_till = time();
			$time_from = $time_till - 3600; // Default to 1 hour
			
			if (is_array($time_period) && isset($time_period['from']) && isset($time_period['to'])) {
				// The time period field contains 'from' and 'to' as relative time strings
				// We need to convert them to timestamps
				$from_str = $time_period['from'];
				$to_str = $time_period['to'];
				
				// Parse relative time strings like "now-1h", "now-1d", etc.
				if ($to_str === 'now' || empty($to_str)) {
					$time_till = time();
				} else {
					$time_till = strtotime($to_str);
					if ($time_till === false) {
						$time_till = time();
					}
				}
				
				if (preg_match('/^now-(\d+)([smhdwMy])$/', $from_str, $matches)) {
					$amount = (int)$matches[1];
					$unit = $matches[2];
					$seconds = 0;
					
					switch ($unit) {
						case 's': $seconds = $amount; break;
						case 'm': $seconds = $amount * 60; break;
						case 'h': $seconds = $amount * 3600; break;
						case 'd': $seconds = $amount * 86400; break;
						case 'w': $seconds = $amount * 604800; break;
						case 'M': $seconds = $amount * 2592000; break; // ~30 days
						case 'y': $seconds = $amount * 31536000; break;
					}
					
					$time_from = $time_till - $seconds;
				} else {
					$time_from = strtotime($from_str);
					if ($time_from === false) {
						$time_from = $time_till - 3600;
					}
				}
			}
		}

		// Initialize graph data
		$graph_data = null;
		$error = null;

		// Find matching items and fetch data
		if (!empty($hostids) && $item_pattern !== '') {
			try {
				// Find items matching the pattern
				$items = ItemMatcher::findItems($hostids, $item_pattern, $pattern_mode);

				if (empty($items)) {
					// Get all items to show sample
					$all_items = \API::Item()->get([
						'output' => ['name'],
						'hostids' => $hostids,
						'monitored' => true,
						'filter' => ['status' => ITEM_STATUS_ACTIVE],
						'limit' => 5
					]);
					$sample_names = array_column($all_items, 'name');
					$error = _('No items match the pattern') . ' (' . $item_pattern . '). Sample items: ' . implode(', ', $sample_names);
				} else {
				// Fetch history data
				$history_data = GraphRenderer::fetchHistoryData($items, $time_from, $time_till);

				// Prepare graph data for rendering
				$graph_data = GraphRenderer::prepareGraphData($items, $history_data, [
					'show_legend' => $show_legend,
					'legend_position' => $legend_position,
					'graph_colors' => $graph_colors,
					'color_mode' => $color_mode,
					'fill_opacity' => $fill_opacity,
					'y_axis_label' => $y_axis_label,
					'y_min' => $y_min,
					'y_max' => $y_max,
					'show_grid' => $show_grid,
					'grid_density' => $grid_density,
					'text_color_legend' => $text_color_legend,
					'text_color_xaxis' => $text_color_xaxis,
					'text_color_yaxis' => $text_color_yaxis
				]);					if (empty($graph_data['series'])) {
						$total_points = 0;
						foreach ($history_data as $item_history) {
							$total_points += count($item_history);
						}
						$error = _('No data available for the selected time period') . ' (Found ' . count($items) . ' items, ' . $total_points . ' history points, time: ' . date('Y-m-d H:i:s', $time_from) . ' to ' . date('Y-m-d H:i:s', $time_till) . ')';
					}
				}
			} catch (\Exception $e) {
				$error = $e->getMessage();
			}
		} else {
			$error = _('Please configure the widget');
		}
		
		$this->setResponse(new CControllerResponseData([
			'name' => $this->getInput('name', $this->widget->getDefaultName()),
			'hostids' => $hostids,
			'item_pattern' => $item_pattern,
			'pattern_mode' => $pattern_mode,
			'show_legend' => $show_legend,
			'graph_data' => $graph_data,
			'error' => $error,
			'user' => [
				'debug_mode' => $this->getDebugMode()
			]
		]));
	}
}