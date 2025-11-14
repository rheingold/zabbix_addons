<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: Widget.php
 * Created: 2025-11-09 19:30:00
 * 
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 * 
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 * 
 * PURPOSE:
 * Widget entry point - main class that registers the multigraph widget with Zabbix.
 * This is the minimal required implementation that Zabbix uses to identify and 
 * instantiate the widget.
 * 
 * RELATIONS:
 * - Extended by Zabbix framework when widget is loaded
 * - Referenced in manifest.json as the main widget class
 * - Used by actions/WidgetView.php and actions/WidgetEdit.php controllers
 * 
 * ZABBIX INTEGRATION:
 * Must be in root of widget directory, namespace must match manifest.json
 */

namespace Widgets\Multigraph;

use Zabbix\Core\CWidget; // Zabbix base widget class providing framework integration
use CWidgetsData; // Dashboard data types (DATA_TYPE_HOST_ID)

/**
 * Multigraph Widget - Main widget class
 * 
 * Provides enhanced multi-item graph visualization with regex/wildcard item matching.
 * Extends Zabbix's CWidget to integrate with dashboard system.
 */
class Widget extends CWidget {

	/**
	 * Default widget name shown in dashboard
	 * @const string
	 */
	public const DEFAULT_NAME = 'Multigraph';

	/**
	 * Get widget default display name
	 * 
	 * Called by Zabbix framework when widget is first added to dashboard.
	 * 
	 * DEPENDENCIES:
	 * - Called by: Zabbix dashboard widget management (CWidget framework)
	 * 
	 * @return string Widget default name
	 */
	public function getDefaultName(): string {
		return self::DEFAULT_NAME;
	}

	/**
	 * Declare dashboard inputs that this widget accepts
	 * 
	 * This tells Zabbix dashboard to pass host context from template dashboards
	 * applied to hosts, enabling the widget to show data for the current host.
	 * 
	 * CRITICAL: Without this, template dashboards cannot pass hostid to widget,
	 * causing configured host to always take precedence.
	 * 
	 * @return array Dashboard data types this widget accepts as input
	 */
	public function getIn(): array {
		return [
			CWidgetsData::DATA_TYPE_HOST_ID // Accept host ID from dashboard context
		];
	}
}