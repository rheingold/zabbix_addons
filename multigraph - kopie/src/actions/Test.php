<?php declare(strict_types = 1);

namespace Modules\Multigraph\Actions;

use CController;
use CControllerResponseData;

/**
 * Phase B1: Hello World test action
 */
class Test extends CController {

    protected function init(): void {
        $this->disableCsrfValidation();
    }

    protected function checkInput(): bool {
        return true;
    }

    protected function checkPermissions(): bool {
        return true;
    }

    protected function doAction(): void {
        $response = new CControllerResponseData([
            'title' => 'Multigraph Test',
            'message' => 'Hello World! Multigraph module is working.',
            'timestamp' => date('Y-m-d H:i:s')
        ]);

        $response->setTitle('Multigraph Test');
        $this->setResponse($response);
    }
}
