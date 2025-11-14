<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: ItemList.php
 * Created: 2025-11-14 (Pattern Builder Feature)
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * AJAX endpoint for fetching item prototypes from a host.
 * Used by pattern builder UI to show available items with LLD macros.
 * 
 * RELATIONS:
 * - Called by: assets/js/widget.edit.js (AJAX request)
 * - Uses: Zabbix API for item discovery
 * - Returns: JSON response with item list
 * 
 * RESPONSE FORMAT:
 * Success: {'items': [{'itemid': string, 'name': string, 'key_': string}, ...]}
 * Error: {'error': string}
 */

namespace Widgets\Multigraph\Actions;

use CController; // Zabbix base controller
use CControllerResponseData; // Response wrapper for JSON data
use API; // Zabbix API gateway

/**
 * Item List Controller
 * 
 * Handles AJAX requests for fetching item prototypes from a host.
 * Returns item list with LLD macros preserved for pattern building.
 */
class ItemList extends CController {

	/**
	 * Initialize controller
	 * 
	 * Sets up required permissions and disables page headers for AJAX response.
	 * 
	 * @return void
	 */
	protected function init(): void {
		$this->disableCsrfValidation(); // AJAX endpoint, CSRF handled by session
	}

	/**
	 * Check user permissions
	 * 
	 * @return bool True if user has read access to hosts
	 */
	protected function checkPermissions(): bool {
		return $this->getUserType() >= USER_TYPE_ZABBIX_USER; // Requires at least read access
	}

	/**
	 * Check input parameters
	 * 
	 * @return bool True if hostid parameter is valid
	 */
	protected function checkInput(): bool {
		$fields = [
			'hostid' => 'required|db hosts.hostid' // Validate hostid exists
		];

		$ret = $this->validateInput($fields);

		if (!$ret) {
			$this->setResponse(
				new CControllerResponseData(['error' => 'Invalid input parameters'])
			);
		}

		return $ret;
	}

	/**
	 * Main action - fetch and return item list
	 * 
	 * PURPOSE:
	 * 1. Fetch regular items from host
	 * 2. Fetch item prototypes (LLD items)
	 * 3. Combine and sort by name
	 * 4. Return as JSON
	 * 
	 * ALGORITHM:
	 * - Uses API::Item()->get() for regular items
	 * - Uses API::ItemPrototype()->get() for LLD items
	 * - Preserves {#MACRO} notation in item names
	 * 
	 * @return void
	 */
	protected function doAction(): void {
		$hostid = $this->getInput('hostid');

		try {
			// Fetch regular items
			$regular_items = API::Item()->get([
				'output' => ['itemid', 'name', 'key_'],
				'hostids' => $hostid,
				'filter' => [
					'flags' => ZBX_FLAG_DISCOVERY_NORMAL // Regular items only
				],
				'sortfield' => 'name',
				'limit' => 1000 // Reasonable limit for UI
			]);

			// Fetch item prototypes (LLD items with {#MACROS})
			$item_prototypes = API::ItemPrototype()->get([
				'output' => ['itemid', 'name', 'key_'],
				'hostids' => $hostid,
				'sortfield' => 'name',
				'limit' => 1000
			]);

			// Combine items
			$all_items = array_merge($regular_items ?: [], $item_prototypes ?: []);

			// Sort by name
			usort($all_items, function($a, $b) {
				return strcmp($a['name'], $b['name']);
			});

			// Output JSON directly
			header('Content-Type: application/json');
			echo json_encode([
				'items' => $all_items
			]);
			exit;

		} catch (\Exception $e) {
			// Output error as JSON
			header('Content-Type: application/json');
			echo json_encode([
				'error' => 'Failed to fetch items: ' . $e->getMessage()
			]);
			exit;
		}
	}
}
