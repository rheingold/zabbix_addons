<?php declare(strict_types = 0);

namespace Modules\Multigraph\Actions;

use CControllerResponseData;
use CController;

/**
 * Controller for module rescan API endpoint
 */
class ModuleRescan extends CController {

	protected function init(): void {
		$this->disableCsrfValidation();
	}

	protected function checkInput(): bool {
		return true;
	}

	protected function checkPermissions(): bool {
		// Only super admins can rescan modules
		return $this->checkAccess(CRoleHelper::UI_ADMINISTRATION_MODULES);
	}

	protected function doAction(): void {
		try {
			// Scan modules directory
			$module_manager = new \CModuleManager(APP_LOCATION_FRONTEND);
			$modules = $module_manager->scanModules();
			
			$result = [
				'success' => true,
				'modules_count' => count($modules['modules']),
				'widgets_count' => count($modules['widgets']),
				'widgets' => []
			];
			
			foreach ($modules['widgets'] as $widget) {
				$widget_info = [
					'id' => $widget['id'],
					'name' => $widget['name']
				];
				if (isset($widget['size'])) {
					$widget_info['size'] = $widget['size'];
				}
				$result['widgets'][] = $widget_info;
			}
			
			$this->setResponse(new CControllerResponseData($result));
		} catch (\Exception $e) {
			$this->setResponse(new CControllerResponseData([
				'success' => false,
				'error' => $e->getMessage()
			]));
		}
	}
}
