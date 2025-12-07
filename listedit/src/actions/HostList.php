<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: HostList.php
 * Created: 2025-12-02
 *
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 *
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 *
 * PURPOSE:
 * AJAX endpoint for fetching hosts and templates list for macro editor.
 * Returns both hosts and templates that user has access to.
 *
 * RELATIONS:
 * - Called by: views/editor.js.php (AJAX request)
 * - Uses: Zabbix API for host/template discovery
 * - Returns: JSON response with combined list
 *
 * RESPONSE FORMAT:
 * Success: {'items': [{'id': string, 'name': string, 'type': 'host'|'template'}, ...]}
 * Error: {'error': string}
 */

namespace Modules\ListEdit\Actions;

use CController;
use CControllerResponseData;
use CWebUser;
use API;

/**
 * Host List Controller
 *
 * Handles AJAX requests for fetching hosts and templates.
 */
class HostList extends CController {

    /**
     * Initialize controller
     */
    protected function init(): void {
        $this->disableCsrfValidation();
    }

    /**
     * Check user permissions
     */
    protected function checkPermissions(): bool {
        return $this->getUserType() >= USER_TYPE_ZABBIX_USER;
    }

    /**
     * Check input parameters
     */
    protected function checkInput(): bool {
        return true; // No required parameters
    }

    /**
     * Main action - fetch and return host/template list
     */
    protected function doAction(): void {
        try {
            // Fetch templates
            $templates = API::Template()->get([
                'output' => ['templateid', 'host', 'name'],
                'sortfield' => 'name',
                'limit' => 5000
            ]);

            // Fetch monitored hosts
            $hosts = API::Host()->get([
                'output' => ['hostid', 'host', 'name'],
                'filter' => ['status' => HOST_STATUS_MONITORED],
                'sortfield' => 'name',
                'limit' => 5000
            ]);

            // Combine into unified format
            $items = [];
            
            foreach ($templates as $template) {
                $items[] = [
                    'id' => $template['templateid'],
                    'name' => $template['name'],
                    'host' => $template['host'],
                    'type' => 'template'
                ];
            }
            
            foreach ($hosts as $host) {
                $items[] = [
                    'id' => $host['hostid'],
                    'name' => $host['name'],
                    'host' => $host['host'],
                    'type' => 'host'
                ];
            }

            // Sort by name
            usort($items, function($a, $b) {
                return strcmp($a['name'], $b['name']);
            });

            header('Content-Type: application/json; charset=UTF-8');
            echo json_encode(['items' => $items]);
            exit;

        } catch (\Exception $e) {
            header('Content-Type: application/json; charset=UTF-8');
            echo json_encode(['error' => $e->getMessage()]);
            exit;
        }
    }
}
