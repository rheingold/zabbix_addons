<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: MacroList.php
 * Created: 2025-12-02
 *
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 *
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 *
 * PURPOSE:
 * AJAX endpoint for fetching _LIST} macros from a host or template.
 * Filters macros ending with _LIST} suffix and returns their details.
 *
 * RELATIONS:
 * - Called by: views/editor.js.php (AJAX request)
 * - Uses: Zabbix API::UserMacro()->get()
 * - Returns: JSON response with macro list
 *
 * RESPONSE FORMAT:
 * Success: {'macros': [{'hostmacroid': string, 'macro': string, 'value': string, 'type': int}, ...]}
 * Error: {'error': string}
 */

namespace Modules\ListEdit\Actions;

use CController;
use CControllerResponseData;
use CWebUser;
use API;

/**
 * Macro List Controller
 *
 * Handles AJAX requests for fetching _LIST} macros from a host/template.
 */
class MacroList extends CController {

    /**
     * Initialize controller
     */
    protected function init(): void {
        $this->disableCsrfValidation(); // AJAX endpoint
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
        $fields = [
            'hostid' => 'db hosts.hostid',
            'type' => 'string'  // 'host' or 'template'
        ];

        $ret = $this->validateInput($fields);

        if (!$ret) {
            $this->setResponse(
                new CControllerResponseData(['error' => 'Invalid input parameters'])
            );
        }

        if ($ret && !$this->hasInput('hostid')) {
            header('Content-Type: application/json');
            echo json_encode(['error' => 'hostid is required']);
            exit;
        }

        return $ret;
    }

    /**
     * Main action - fetch and return _LIST} macros
     */
    protected function doAction(): void {
        $hostid = $this->getInput('hostid');
        $type = $this->getInput('type', 'template');

        try {
            // Determine API parameter based on type
            $id_param = ($type === 'template') ? 'templateids' : 'hostids';

            // Fetch all user macros
            $macros = API::UserMacro()->get([
                'output' => ['hostmacroid', 'macro', 'value', 'type', 'description'],
                $id_param => [$hostid],
                'sortfield' => 'macro'
            ]);

            // Filter macros ending with _LIST}
            $list_macros = [];
            foreach ($macros as $macro) {
                if (preg_match('/_LIST\}$/', $macro['macro'])) {
                    $list_macros[] = $macro;
                }
            }

            header('Content-Type: application/json; charset=UTF-8');
            echo json_encode(['macros' => $list_macros]);
            exit;

        } catch (\Exception $e) {
            header('Content-Type: application/json; charset=UTF-8');
            echo json_encode(['error' => $e->getMessage()]);
            exit;
        }
    }
}
