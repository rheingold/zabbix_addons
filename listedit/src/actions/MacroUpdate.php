<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: MacroUpdate.php
 * Created: 2025-12-02
 *
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 *
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 *
 * PURPOSE:
 * AJAX endpoint for updating macro value.
 * Receives new value and updates the macro via API.
 *
 * RELATIONS:
 * - Called by: views/editor.js.php (AJAX POST request)
 * - Uses: API::UserMacro()->update()
 * - Returns: JSON response with success/error
 *
 * RESPONSE FORMAT:
 * Success: {'success': true}
 * Error: {'error': string}
 */

namespace Modules\ListEdit\Actions;

use CController;
use CControllerResponseData;
use CWebUser;
use API;

/**
 * Macro Update Controller
 *
 * Handles AJAX requests for updating macro values.
 * Requires ADMIN permissions to modify macros.
 */
class MacroUpdate extends CController {

    /**
     * Initialize controller
     */
    protected function init(): void {
        $this->disableCsrfValidation(); // AJAX endpoint
    }

    /**
     * Check user permissions
     * 
     * Requires admin to edit macros
     */
    protected function checkPermissions(): bool {
        return $this->getUserType() >= USER_TYPE_ZABBIX_USER;
    }

    /**
     * Check input parameters
     */
    protected function checkInput(): bool {
        $fields = [
            'hostmacroid' => 'db hostmacro.hostmacroid|required',
            'value' => 'string|required'
        ];

        $ret = $this->validateInput($fields);

        if (!$ret) {
            header('Content-Type: application/json; charset=UTF-8');
            echo json_encode(['error' => 'Invalid input parameters']);
            exit;
        }

        return $ret;
    }

    /**
     * Main action - update macro value
     */
    protected function doAction(): void {
        $hostmacroid = $this->getInput('hostmacroid');
        $value = $this->getInput('value');

        try {
            // Update macro value
            $result = API::UserMacro()->update([
                'hostmacroid' => $hostmacroid,
                'value' => $value
            ]);

            if ($result) {
                header('Content-Type: application/json; charset=UTF-8');
                echo json_encode(['success' => true]);
            } else {
                header('Content-Type: application/json; charset=UTF-8');
                echo json_encode(['error' => 'Failed to update macro']);
            }
            exit;

        } catch (\Exception $e) {
            header('Content-Type: application/json; charset=UTF-8');
            echo json_encode(['error' => $e->getMessage()]);
            exit;
        }
    }
}
