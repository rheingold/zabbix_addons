<?php declare(strict_types = 0);

namespace Widgets\Multigraph;

use Zabbix\Core\CWidget;

/**
 * Multigraph Widget
 * 
 * Enhanced multi-graph visualization with regex/wildcard item matching
 */
class Widget extends CWidget {

	public const DEFAULT_NAME = 'Multigraph';

	/**
	 * Get widget default name
	 */
	public function getDefaultName(): string {
		return self::DEFAULT_NAME;
	}
}