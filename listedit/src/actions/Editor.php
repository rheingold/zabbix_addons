<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: Editor.php
 * Created: 2025-12-02
 *
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 *
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 *
 * PURPOSE:
 * Main editor page controller.
 * Renders the macro list editor interface.
 *
 * RELATIONS:
 * - Called by: Module menu item (listedit.editor)
 * - Renders: views/editor.php
 * - Uses: views/editor.js.php for JavaScript
 */

namespace Modules\ListEdit\Actions;

use CController;
use CControllerResponseData;
use CControllerResponseFatal;
use CView;
use CWebUser;

/**
 * Editor Controller
 *
 * Main page controller for the macro list editor.
 */
class Editor extends CController {

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
        // Temporarily allow all authenticated users
        return true;
    }

    /**
     * Check input parameters
     */
    protected function checkInput(): bool {
        return true; // No required parameters for main page
    }

    /**
     * Main action - render editor page
     */
    protected function doAction(): void {
        $data = [
            'title' => _('Macro List Editor'),
            'user_type' => CWebUser::getType()
        ];

        $response = new CControllerResponseData($data);
        $response->setTitle($data['title']);
        $this->setResponse($response);
    }
}
