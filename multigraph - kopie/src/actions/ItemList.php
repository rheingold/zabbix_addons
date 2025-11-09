<?php declare(strict_types = 0);

namespace Modules\Multigraph\Actions;

use CControllerResponseData;
use CController;
use API;

/**
 * Item list action for pattern selector
 */
class ItemList extends CController {

	protected function init(): void {
		$this->disableCsrfValidation();
	}

	protected function checkInput(): bool {
		$fields = [
			'hostid' => 'required|db hosts.hostid'
		];

		$ret = $this->validateInput($fields);

		if (!$ret) {
			$this->setResponse(
				new CControllerResponseData(['main_block' => json_encode([
					'error' => $this->getValidationError()
				])])
			);
		}

		return $ret;
	}

	protected function checkPermissions(): bool {
		return $this->getUserType() >= USER_TYPE_ZABBIX_USER;
	}

	protected function doAction(): void {
		$hostid = $this->getInput('hostid');

		$items = API::Item()->get([
			'output' => ['itemid', 'name', 'key_'],
			'hostids' => [$hostid],
			'monitored' => true,
			'filter' => [
				'status' => ITEM_STATUS_ACTIVE
			],
			'sortfield' => 'name',
			'limit' => 1000
		]);

		$this->setResponse(
			new CControllerResponseData([
				'main_block' => json_encode([
					'items' => $items ?: []
				])
			])
		);
	}
}
