<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: WidgetView.php  
 * Created: 2025-11-09 19:30:00
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * Main widget view controller - handles widget display requests from dashboard.
 * Orchestrates data fetching, processing, and preparation for client-side rendering.
 * Implements timeframe parsing (absolute/relative) and error handling.
 * 
 * RELATIONS:
 * - Extends: CControllerDashboardWidgetView (Zabbix MVC framework)
 * - Uses: includes/MatchedItemsData.php for item pattern matching
 * - Uses: includes/GraphData.php for history fetching and data preparation
 * - Renders: views/widget.view.php template
 * - Called by: Zabbix dashboard when widget needs to display/refresh
 * 
 * DATA FLOW:
 * 1. Receives widget configuration from dashboard
 * 2. Parses timeframe (from/to parameters)
 * 3. Finds items matching pattern (via MatchedItemsData)
 * 4. Fetches history data (via GraphData)
 * 5. Prepares data structure for JavaScript
 * 6. Passes to view template for rendering
 */

namespace Widgets\Multigraph\Actions;

use CControllerDashboardWidgetView; // Zabbix base controller for widget view actions
use CControllerResponseData; // Response wrapper for passing data to views
use Widgets\Multigraph\Includes\MatchedItemsData; // Item pattern matching logic
use Widgets\Multigraph\Includes\GraphData; // Graph data preparation and history fetching

/**
 * Multigraph Widget View Controller
 * 
 * Handles HTTP requests for widget display and data refresh.
 * Implements MVC Controller pattern for Zabbix dashboard widgets.
 */
class WidgetView extends CControllerDashboardWidgetView {

	/**
	 * Initialize controller - set up validation rules
	 * 
	 * DEPENDENCIES:
	 * - Called by: Zabbix MVC framework before doAction()
	 * - Calls: parent::init() for base initialization
	 * 
	 * VALIDATION:
	 * - 'from': Optional string for start time (dashboard time selector)
	 * - 'to': Optional string for end time (dashboard time selector)
	 * 
	 * @return void
	 */
	protected function init(): void {
		parent::init();
		
		$this->addValidationRules([
			'from' => 'string', // Dashboard timeframe start (e.g., "now-6h", "2025-01-01 00:00:00")
			'to' => 'string'    // Dashboard timeframe end (e.g., "now", "2025-01-31 23:59:59")
		]);
	}

	/**
	 * Main action - fetch data and prepare widget for display
	 * 
	 * PURPOSE:
	 * 1. Extract widget configuration from $this->fields_values
	 * 2. Parse timeframe (absolute timestamps or relative like "now-6h")
	 * 3. Find items matching the configured pattern
	 * 4. Fetch history data for matched items
	 * 5. Prepare data structure for JavaScript rendering
	 * 6. Pass to view template via setResponse()
	 * 
	 * DEPENDENCIES:
	 * - Called by: Zabbix MVC framework after init()
	 * - Calls: MatchedItemsData::findItems() for pattern matching
	 * - Calls: GraphData::fetchHistoryData() for history retrieval
	 * - Calls: GraphData::prepareGraphData() for data structuring
	 * - Renders: views/widget.view.php
	 * 
	 * TIMEFRAME PARSING:
	 * - Supports absolute: Unix timestamps, "YYYY-MM-DD HH:MM:SS"
	 * - Supports relative: "now", "now-1h", "now-6h", "now-1d", "now-1w", "now-1M", "now-1y"
	 * - Falls back to last hour if parsing fails
	 * 
	 * ERROR HANDLING:
	 * - Missing hosts: Shows "Please configure the widget"
	 * - No matching items: Shows pattern and sample item names
	 * - No data: Shows count of items/points and time range
	 * - Exceptions: Caught and displayed as error message
	 * 
	 * @return void
	 */
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
			
			// Parse from timestamp using Zabbix-aware parser
			$time_from = $this->parseZabbixTime($from_input, time() - 3600);
			// Parse to timestamp using Zabbix-aware parser
			$time_till = $this->parseZabbixTime($to_input, time());
		} else {
			// Use widget's time period setting
			$time_till = time();
			$time_from = $time_till - 3600; // Default to 1 hour
			
			if (is_array($time_period) && isset($time_period['from']) && isset($time_period['to'])) {
				// The time period field contains 'from' and 'to' as relative time strings
				// Parse 'to' timestamp first (needed as reference for relative 'from')
				$to_str = $time_period['to'];
				$time_till = $this->parseZabbixTime($to_str, time());
				
				// Parse 'from' timestamp using 'to' as reference
				$from_str = $time_period['from'];
				$time_from = $this->parseZabbixTime($from_str, $time_till - 3600, $time_till);
			}
		}

		// Initialize graph data
		$graph_data = null;
		$error = null;

		// Find matching items and fetch data
		if (!empty($hostids) && $item_pattern !== '') {
			try {
				// Find items matching the pattern
				$items = MatchedItemsData::findItems($hostids, $item_pattern, $pattern_mode);

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
				$history_data = GraphData::fetchHistoryData($items, $time_from, $time_till);

				// Prepare graph data for rendering
				$graph_data = GraphData::prepareGraphData($items, $history_data, [
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

	/**
	 * Parse Zabbix time expression to Unix timestamp
	 * 
	 * PURPOSE:
	 * Converts Zabbix time expressions to Unix timestamps.
	 * Handles absolute timestamps, relative offsets, and period rounding.
	 * 
	 * SUPPORTED FORMATS:
	 * - Numeric: "1699564800" → Unix timestamp as-is
	 * - Now: "now" → Current time
	 * - Relative offset: "now-6h", "now-1d", "now-2w", "now-3M", "now-1y"
	 * - Period rounding: "now/d" (start of today), "now/w" (start of week), "now/M" (start of month)
	 * - Fallback: strtotime() for other formats (e.g., "2025-01-01 00:00:00")
	 * 
	 * ALGORITHM:
	 * 1. If numeric, return as-is (Unix timestamp)
	 * 2. If "now", return current time (or reference time)
	 * 3. If "now-Xu" format, subtract duration from reference time
	 * 4. If "now/u" format, round reference time to start of period
	 * 5. Otherwise, try strtotime() as fallback
	 * 6. If all parsing fails, return default
	 * 
	 * DEPENDENCIES:
	 * - Called by: doAction() for timeframe parsing
	 * 
	 * @param string $time_str Time expression (e.g., "now-6h", "now/M", "1699564800")
	 * @param int $default Default timestamp if parsing fails
	 * @param int|null $reference_time Reference time for "now" (defaults to current time)
	 * 
	 * @return int Unix timestamp
	 */
	private function parseZabbixTime(string $time_str, int $default, ?int $reference_time = null): int {
		// Use current time as reference if not provided
		if ($reference_time === null) {
			$reference_time = time();
		}
		
		$time_str = trim($time_str);
		
		// Handle numeric timestamps (already Unix timestamp)
		if (is_numeric($time_str)) {
			return (int)$time_str;
		}
		
		// Handle "now" (current time or reference time)
		if ($time_str === 'now') {
			return $reference_time;
		}
		
		// Handle relative offsets: "now-6h", "now-1d", "now-2w", "now-3M", "now-1y"
		if (preg_match('/^now-(\d+)([smhdwMy])$/', $time_str, $matches)) {
			$amount = (int)$matches[1];
			$unit = $matches[2];
			$seconds = 0;
			
			switch ($unit) {
				case 's': $seconds = $amount; break;                  // Seconds
				case 'm': $seconds = $amount * 60; break;             // Minutes
				case 'h': $seconds = $amount * 3600; break;           // Hours
				case 'd': $seconds = $amount * 86400; break;          // Days
				case 'w': $seconds = $amount * 604800; break;         // Weeks (7 days)
				case 'M': $seconds = $amount * 2592000; break;        // Months (~30 days)
				case 'y': $seconds = $amount * 31536000; break;       // Years (365 days)
			}
			
			return $reference_time - $seconds;
		}
		
		// Handle period rounding: "now/d" (today), "now/w" (this week), "now/M" (this month), "now/y" (this year)
		if (preg_match('/^now\/([dwMy])$/', $time_str, $matches)) {
			$unit = $matches[1];
			
			// Get date components for rounding calculations
			$dt = getdate($reference_time);
			
			switch ($unit) {
				case 'd': // Start of today (00:00:00)
					return mktime(0, 0, 0, $dt['mon'], $dt['mday'], $dt['year']);
				
				case 'w': // Start of this week (Monday 00:00:00)
					// PHP's week starts on Sunday (wday=0), we want Monday
					$days_since_monday = ($dt['wday'] + 6) % 7; // Convert to days since Monday
					return mktime(0, 0, 0, $dt['mon'], $dt['mday'] - $days_since_monday, $dt['year']);
				
				case 'M': // Start of this month (1st day 00:00:00)
					return mktime(0, 0, 0, $dt['mon'], 1, $dt['year']);
				
				case 'y': // Start of this year (January 1st 00:00:00)
					return mktime(0, 0, 0, 1, 1, $dt['year']);
			}
		}
		
		// Fallback: Try strtotime() for other formats (e.g., "2025-01-01 00:00:00")
		$timestamp = strtotime($time_str);
		if ($timestamp !== false) {
			return $timestamp;
		}
		
		// If all parsing failed, return default
		return $default;
	}
}