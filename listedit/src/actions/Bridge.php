<?php declare(strict_types = 0);
namespace Modules\ListEdit\Actions;

use CController;
use CControllerResponseData;

class Bridge extends CController {
    protected function init(): void {}
    protected function checkPermissions(): bool { return true; }
    protected function checkInput(): bool { return true; }

    protected function doAction(): void {
        $data = [ 'title' => _('Macro List Editor (Bridge)') ];
        $response = new CControllerResponseData($data);
        $response->setTitle($data['title']);
        // Use the same view as the main editor
        $response->setView('modules/listedit/views/editor.php');
        $this->setResponse($response);
    }
}
