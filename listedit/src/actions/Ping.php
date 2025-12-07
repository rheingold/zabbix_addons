<?php declare(strict_types = 0);
namespace Modules\ListEdit\Actions;

use CController;
use CControllerResponseData;

class Ping extends CController {
    protected function init(): void {}

    protected function checkPermissions(): bool {
        // Return true to see if global role gating still denies.
        return true;
    }

    protected function checkInput(): bool { return true; }

    protected function doAction(): void {
        $response = new CControllerResponseData(['pong' => true, 'ts' => time()]);
        // Render minimal output without view to avoid section dependencies
        $response->setTitle('ListEdit Ping');
        $this->setResponse($response);
    }
}
