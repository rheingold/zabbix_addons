<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: MatchedItemsData.php
 * Created: 2025-11-09 19:30:00
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * Item pattern matching and metadata retrieval - finds Zabbix items by name patterns.
 * Supports both wildcard (*,?) and regex matching for flexible item selection.
 * 
 * RELATIONS:
 * - Used by: actions/WidgetView.php controller
 * - Uses: Zabbix API (API::Item())
 * - Outputs: Item metadata array for GraphData::fetchHistoryData()
 * 
 * PATTERN MATCHING:
 * - Wildcard mode: * (any chars), ? (single char) - converted to regex internally
 * - Regex mode: Full PCRE regex patterns (e.g., "/^CPU.*load$/i")
 * 
 * USE CASES:
 * - "CPU*" → Matches "CPU load", "CPU temperature", etc.
 * - "Disk*free*" → Matches "Disk C: free space", "Disk /home free %", etc.
 * - "/Interface.*eth[0-9]+.*bits/" → Regex for network interface metrics
 */

namespace Widgets\Multigraph\Includes;

use API; // Zabbix API gateway for item queries

/**
 * Matched Items Data Class
 * 
 * Stateless utility class providing item pattern matching functionality.
 * All methods are static as no instance state is needed.
 */
class MatchedItemsData {

	/**
	 * Pattern mode: Wildcard matching (* and ?)
	 * @const int
	 */
	public const PATTERN_MODE_WILDCARD = 0;
	
	/**
	 * Pattern mode: Regular expression (PCRE)
	 * @const int
	 */
	public const PATTERN_MODE_REGEX = 1;

	/**
	 * Find items matching the pattern from Zabbix
	 * 
	 * PURPOSE:
	 * Retrieves items from Zabbix that match the given name pattern.
	 * Fetches all active monitored items from specified hosts, then filters by pattern.
	 * 
	 * DEPENDENCIES:
	 * - Called by: actions/WidgetView.php
	 * - Calls: API::Item()->get() (Zabbix API)
	 * - Called before: GraphData::fetchHistoryData()
	 * 
	 * ALGORITHM:
	 * 1. Fetch all active items from hosts (single API call)
	 * 2. Apply pattern matching client-side (wildcard or regex)
	 * 3. Return matched items with metadata
	 * 
	 * PERFORMANCE:
	 * Fetches all items first, filters locally - suitable for reasonable item counts.
	 * For large environments, consider adding server-side filtering.
	 *
	 * @param array $hostids Host IDs to search in (from widget configuration)
	 * @param string $pattern Item name pattern (e.g., "CPU*", "/disk.*free/i")
	 * @param int $pattern_mode Matching mode: PATTERN_MODE_WILDCARD or PATTERN_MODE_REGEX
	 *
	 * @return array Matching items with keys: itemid, name, key_, value_type, units, hostid
	 *               Returns empty array if no hosts, empty pattern, or no matches
	 */
	public static function findItems(array $hostids, string $pattern, int $pattern_mode): array {
		if (empty($hostids) || trim($pattern) === '') {
			return [];
		}

		// Get all items from specified hosts
		$items = API::Item()->get([
			'output' => ['itemid', 'name', 'key_', 'value_type', 'units', 'hostid'],
			'hostids' => $hostids,
			'monitored' => true,
			'filter' => [
				'status' => ITEM_STATUS_ACTIVE
			],
			'sortfield' => 'name'
		]);

		if (!$items) {
			return [];
		}

		// Filter items by pattern
		return self::matchItems($items, $pattern, $pattern_mode);
	}

	/**
	 * Match items by pattern (regex or wildcard)
	 * 
	 * @param array $items Available items
	 * @param string $pattern Pattern to match
	 * @param int $pattern_mode Pattern mode (wildcard=0 or regex=1)
	 * @return array Matched items
	 */
	public static function matchItems(array $items, string $pattern, int $pattern_mode): array {
		$matched = [];
		
		foreach ($items as $item) {
			$item_name = $item['name'];
			
			if (self::matchPattern($item_name, $pattern, $pattern_mode)) {
				$matched[] = $item;
			}
		}

		return $matched;
	}

	/**
	 * Check if item name matches the pattern
	 *
	 * @param string $item_name     Item name to check
	 * @param string $pattern       Pattern to match against
	 * @param int $pattern_mode     Pattern mode (wildcard or regex)
	 *
	 * @return bool True if matches
	 */
	private static function matchPattern(string $item_name, string $pattern, int $pattern_mode): bool {
		if ($pattern_mode === self::PATTERN_MODE_REGEX) {
			// Regex mode - use pattern directly (case-sensitive by default)
			return @preg_match('/' . $pattern . '/', $item_name) === 1;
		}

		// Wildcard mode - convert to regex (case-insensitive)
		$regex = self::wildcardToRegex($pattern);
		return preg_match('/' . $regex . '/i', $item_name) === 1;
	}

	/**
	 * Convert wildcard pattern to regex
	 * 
	 * @param string $wildcard Wildcard pattern (* and ?)
	 * @return string Regex pattern
	 */
	public static function wildcardToRegex(string $wildcard): string {
		// Escape regex special chars except * and ?
		$pattern = preg_quote($wildcard, '/');
		
		// Convert wildcards to regex
		$pattern = str_replace(['\*', '\?'], ['.*', '.'], $pattern);
		
		return '^' . $pattern . '$';
	}
}
