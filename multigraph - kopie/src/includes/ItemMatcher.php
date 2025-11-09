<?php declare(strict_types = 0);

namespace Widgets\Multigraph\Includes;

use API;

/**
 * Item Matcher - Regex/Wildcard matching for items
 * 
 * SHARED COMPONENT (B2.1 Widget + B2.2 Patched Graphs)
 * This class handles item name pattern matching logic
 */
class ItemMatcher {

	/**
	 * Pattern matching modes
	 */
	public const PATTERN_MODE_WILDCARD = 0;
	public const PATTERN_MODE_REGEX = 1;

	/**
	 * Find items matching the pattern from Zabbix API
	 *
	 * @param array $hostids        Host IDs to search in
	 * @param string $pattern       Item name pattern
	 * @param int $pattern_mode     Pattern mode (wildcard or regex)
	 *
	 * @return array Array of matching items with keys: itemid, name, key_, value_type, units, hostid
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
