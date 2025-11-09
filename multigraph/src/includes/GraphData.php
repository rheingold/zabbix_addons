<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: GraphData.php
 * Created: 2025-11-09 19:30:00
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * Graph data preparation and history fetching - core business logic for graph rendering.
 * Fetches historical data from Zabbix API and transforms it into a structure
 * suitable for client-side JavaScript graph rendering.
 * 
 * RELATIONS:
 * - Used by: actions/WidgetView.php controller
 * - Uses: Zabbix API (API::History(), CSettingsHelper)
 * - Outputs: Data structure consumed by assets/js/class.widget.js
 * 
 * RESPONSIBILITIES:
 * 1. Fetch history data from Zabbix for matched items
 * 2. Transform data into JavaScript-friendly format
 * 3. Apply color schemes (cycle/random/offset modes)
 * 4. Prepare complete graph configuration
 * 
 * DATA STRUCTURE OUTPUT:
 * {
 *   'series': [{ name, data: [[timestamp_ms, value], ...], color, units }],
 *   'legend_position': int,
 *   'fill_opacity': float,
 *   'y_axis_label': string,
 *   'y_min': string|number,
 *   'y_max': string|number,
 *   'show_grid': bool,
 *   'grid_density': string|number,
 *   'text_color_*': string (hex colors)
 * }
 */

namespace Widgets\Multigraph\Includes;

use API; // Zabbix API gateway for history and item queries
use CSettingsHelper; // Zabbix settings helper (unused currently, kept for future)

/**
 * Graph Data Preparation Class
 * 
 * Stateless utility class providing static methods for graph data operations.
 * Handles all server-side graph data preparation.
 */
class GraphData {

	/**
	 * Color mode: Cycle through colors sequentially
	 * @const int
	 */
	private const COLOR_MODE_CYCLE = 0;
	
	/**
	 * Color mode: Random color assignment
	 * @const int
	 */
	private const COLOR_MODE_RANDOM = 1;
	
	/**
	 * Color mode: Cycle with offset
	 * @const int
	 */
	private const COLOR_MODE_OFFSET = 2;

	/**
	 * Fetch historical data for items from Zabbix
	 * 
	 * PURPOSE:
	 * Retrieves time-series data points for given items within specified timeframe.
	 * Groups items by value_type to optimize API calls (different history tables).
	 * 
	 * DEPENDENCIES:
	 * - Called by: actions/WidgetView.php
	 * - Calls: API::History()->get() (Zabbix API)
	 * 
	 * ALGORITHM:
	 * 1. Group items by value_type (0=float, 3=uint, etc.)
	 * 2. Fetch history for each group in single API call
	 * 3. Organize results by itemid
	 * 
	 * @param array $items Item metadata from MatchedItemsData::findItems()
	 *                     Format: [['itemid' => int, 'value_type' => int, ...], ...]
	 * @param int $time_from Start timestamp (Unix epoch seconds)
	 * @param int $time_till End timestamp (Unix epoch seconds)
	 * 
	 * @return array History data grouped by itemid
	 *               Format: ['itemid' => [['clock' => int, 'value' => string, 'ns' => int], ...], ...]
	 *               Returns empty array if no items provided
	 */
	public static function fetchHistoryData(array $items, int $time_from, int $time_till): array {
		if (empty($items)) {
			return [];
		}

		$history_data = []; // Result array: itemid => history points

		// Group items by value type (history table)
		$items_by_type = [];
		foreach ($items as $item) {
			$value_type = $item['value_type'];
			$items_by_type[$value_type][] = $item['itemid'];
		}

		// Fetch history for each value type
		foreach ($items_by_type as $value_type => $itemids) {
			$history = API::History()->get([
				'output' => ['itemid', 'clock', 'value', 'ns'],
				'history' => $value_type,
				'itemids' => $itemids,
				'time_from' => $time_from,
				'time_till' => $time_till,
				'sortfield' => 'clock',
				'sortorder' => 'ASC'
			]);

			if ($history) {
				foreach ($history as $point) {
					$itemid = $point['itemid'];
					if (!isset($history_data[$itemid])) {
						$history_data[$itemid] = [];
					}
					$history_data[$itemid][] = [
						'clock' => (int)$point['clock'],
						'value' => $point['value'],
						'ns' => isset($point['ns']) ? (int)$point['ns'] : 0
					];
				}
			}
		}

		return $history_data;
	}

	/**
	 * Prepare graph data for JavaScript rendering
	 * 
	 * @param array $items Matched items
	 * @param array $history_data History data from fetchHistoryData()
	 * @param array $config Widget configuration
	 * @return array Graph data ready for JS
	 */
	public static function prepareGraphData(array $items, array $history_data, array $config): array {
		$series = [];
		$colors = self::parseColors($config['graph_colors'] ?? '');
		$color_mode = $config['color_mode'] ?? self::COLOR_MODE_CYCLE;
		$color_index = 0;

		foreach ($items as $item) {
			$itemid = $item['itemid'];
			
			if (!isset($history_data[$itemid]) || empty($history_data[$itemid])) {
				continue;
			}

			// Convert history to [timestamp, value] pairs
			$data_points = [];
			foreach ($history_data[$itemid] as $point) {
				// Convert to milliseconds for JavaScript
				$timestamp_ms = ($point['clock'] * 1000) + (int)($point['ns'] / 1000000);
				$data_points[] = [$timestamp_ms, (float)$point['value']];
			}

			$color = self::getColorForIndex($color_index, $colors, $color_mode);

			$series[] = [
				'name' => $item['name'],
				'data' => $data_points,
				'color' => $color,
				'units' => $item['units'] ?? ''
			];

			$color_index++;
		}

		return [
			'series' => $series,
			'show_legend' => $config['show_legend'] ?? true,
			'legend_position' => $config['legend_position'] ?? 0,
			'fill_opacity' => min(100, max(0, (int)($config['fill_opacity'] ?? 20))) / 100,
			'y_axis_label' => $config['y_axis_label'] ?? '',
			'y_min' => $config['y_min'] ?? 'auto',
			'y_max' => $config['y_max'] ?? 'auto',
			'show_grid' => $config['show_grid'] ?? true,
			'grid_density' => $config['grid_density'] ?? 'auto',
			'text_color_legend' => $config['text_color_legend'] ?? '#000000',
			'text_color_xaxis' => $config['text_color_xaxis'] ?? '#000000',
			'text_color_yaxis' => $config['text_color_yaxis'] ?? '#000000'
		];
	}

	/**
	 * Parse color string into array of hex codes
	 * 
	 * @param string $colors_str Comma or semicolon separated hex codes
	 * @return array Array of hex color codes
	 */
	private static function parseColors(string $colors_str): array {
		if (empty($colors_str)) {
			return self::getDefaultColors();
		}

		// Split by comma or semicolon
		$colors = preg_split('/[,;]/', $colors_str);
		$parsed = [];

		foreach ($colors as $color) {
			$color = trim($color);
			// Validate hex color format (#RRGGBB or #RGB)
			if (preg_match('/^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$/', $color)) {
				$parsed[] = $color;
			}
		}

		return !empty($parsed) ? $parsed : self::getDefaultColors();
	}

	/**
	 * Get color for item index based on color mode
	 * 
	 * @param int $index Item index
	 * @param array $colors Available colors
	 * @param int $mode Color mode (COLOR_MODE_CYCLE, COLOR_MODE_RANDOM, COLOR_MODE_OFFSET)
	 * @return string Hex color code
	 */
	private static function getColorForIndex(int $index, array $colors, int $mode): string {
		if (empty($colors)) {
			$colors = self::getDefaultColors();
		}

		// In offset mode, use only first N-1 colors
		$available_colors = ($mode === self::COLOR_MODE_OFFSET && count($colors) > 1) 
			? array_slice($colors, 0, count($colors) - 1) 
			: $colors;

		if ($index < count($available_colors)) {
			return $available_colors[$index];
		}

		// More items than colors - use mode to generate color
		switch ($mode) {
			case self::COLOR_MODE_RANDOM:
				return self::generateRandomColor();
			
			case self::COLOR_MODE_OFFSET:
				// Cycle through available colors (first N-1), then apply offset
				$base_index = ($index - count($available_colors)) % count($available_colors);
				$offset_level = (int)(($index - count($available_colors)) / count($available_colors)) + 1;
				return self::offsetColor($available_colors[$base_index], $offset_level);
			
			case self::COLOR_MODE_CYCLE:
			default:
				return $colors[$index % count($colors)];
		}
	}

	/**
	 * Generate a random hex color
	 * 
	 * @return string Hex color code
	 */
	private static function generateRandomColor(): string {
		return '#' . strtoupper(dechex(mt_rand(0, 0xFFFFFF)));
	}

	/**
	 * Generate offset shade of a color with wrapping overflow
	 * 
	 * @param string $baseColor Base hex color code
	 * @param int $offset Offset level (adds fixed value to each RGB component with wrapping)
	 * @return string Hex color code
	 */
	private static function offsetColor(string $baseColor, int $offset): string {
		// Remove # and parse RGB
		$hex = ltrim($baseColor, '#');
		$r = hexdec(substr($hex, 0, 2));
		$g = hexdec(substr($hex, 2, 2));
		$b = hexdec(substr($hex, 4, 2));

		// Add offset to each component with wrapping (0-255)
		$offset_value = $offset * 40; // 40 units per level
		$r = ($r + $offset_value) % 256;
		$g = ($g + $offset_value) % 256;
		$b = ($b + $offset_value) % 256;

		return '#' . strtoupper(sprintf('%02x%02x%02x', $r, $g, $b));
	}

	/**
	 * Get default graph colors (Zabbix-like palette)
	 * 
	 * @return array Array of color hex codes
	 */
	private static function getDefaultColors(): array {
		return [
			'#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd',
			'#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf',
			'#aec7e8', '#ffbb78', '#98df8a', '#ff9896', '#c5b0d5',
			'#c49c94', '#f7b6d2', '#c7c7c7', '#dbdb8d', '#9edae5'
		];
	}
}
