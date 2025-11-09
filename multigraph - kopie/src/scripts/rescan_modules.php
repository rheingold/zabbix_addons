<?php
/**
 * Module Rescan Script
 * 
 * This script triggers Zabbix module directory scan from inside the container.
 * Usage: php rescan_modules.php
 */

require_once dirname(__FILE__) . '/../../../../conf/zabbix.conf.php';
require_once dirname(__FILE__) . '/../../../../include/classes/core/CAutoloader.php';

try {
    // Initialize Zabbix autoloader
    CAutoloader::setRootPath(dirname(__FILE__) . '/../../../..');
    
    // Scan modules directory
    $module_manager = new CModuleManager(APP_LOCATION_FRONTEND);
    $modules = $module_manager->scanModules();
    
    echo "Module scan completed successfully.\n";
    echo "Found " . count($modules['modules']) . " modules and " . count($modules['widgets']) . " widgets.\n";
    
    // Show widget details
    foreach ($modules['widgets'] as $widget) {
        echo "Widget: {$widget['id']} - {$widget['name']}\n";
        if (isset($widget['size'])) {
            echo "  Size: {$widget['size']['width']}x{$widget['size']['height']}\n";
        }
    }
    
    exit(0);
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
    exit(1);
}
