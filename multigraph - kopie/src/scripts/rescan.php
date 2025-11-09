<?php
/**
 * Module Rescan Web Endpoint
 * 
 * Accessible via: https://zabbix.plachy.eu/widgets/multigraph/scripts/rescan.php
 * Simple authentication via secret token in URL parameter.
 */

// Simple secret token (change this to something secure)
define('RESCAN_TOKEN', 'd52aecac75ee7af2dd2a31b1725b423ce68027db0567303f859a78b750cf0d83');

// Check token
if (!isset($_GET['token']) || $_GET['token'] !== RESCAN_TOKEN) {
    http_response_code(403);
    die(json_encode(['error' => 'Unauthorized']));
}

require_once dirname(__FILE__) . '/../../../../conf/zabbix.conf.php';
require_once dirname(__FILE__) . '/../../../../include/classes/core/CAutoloader.php';

header('Content-Type: application/json');

try {
    // Initialize Zabbix autoloader
    CAutoloader::setRootPath(dirname(__FILE__) . '/../../../..');
    
    // Scan modules directory
    $module_manager = new CModuleManager(APP_LOCATION_FRONTEND);
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
    
    echo json_encode($result, JSON_PRETTY_PRINT);
    exit(0);
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode([
        'success' => false,
        'error' => $e->getMessage()
    ], JSON_PRETTY_PRINT);
    exit(1);
}
