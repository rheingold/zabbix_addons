<?php
/**
 * Simple Module Rescan Helper
 * 
 * This script reads module/widget manifests and triggers directory touch
 * to help indicate changes. Still requires manual "Scan directory" click.
 * 
 * Access: https://zabbix.plachy.eu/modules_rescan.php?token=YOUR_TOKEN
 */

// Simple authentication
define('RESCAN_TOKEN', 'd52aecac75ee7af2dd2a31b1725b423ce68027db0567303f859a78b750cf0d83');

if (!isset($_GET['token']) || $_GET['token'] !== RESCAN_TOKEN) {
    http_response_code(403);
    header('Content-Type: application/json');
    die(json_encode(['error' => 'Unauthorized']));
}

header('Content-Type: application/json');

try {
    $modules_dir = '/usr/share/zabbix/modules';
    $widgets_dir = '/usr/share/zabbix/widgets';
    
    // Touch directories to update modification time
    @touch($modules_dir);
    @touch($widgets_dir);
    
    // List available modules
    $modules = [];
    if (is_dir($modules_dir)) {
        $items = scandir($modules_dir);
        foreach ($items as $item) {
            if ($item != '.' && $item != '..' && is_dir($modules_dir . '/' . $item)) {
                $manifest_file = $modules_dir . '/' . $item . '/manifest.json';
                if (file_exists($manifest_file)) {
                    $manifest = json_decode(file_get_contents($manifest_file), true);
                    $modules[] = [
                        'id' => $manifest['id'] ?? $item,
                        'name' => $manifest['name'] ?? $item,
                        'version' => $manifest['version'] ?? 'unknown'
                    ];
                }
            }
        }
    }
    
    // List available widgets
    $widgets = [];
    if (is_dir($widgets_dir)) {
        $items = scandir($widgets_dir);
        foreach ($items as $item) {
            if ($item != '.' && $item != '..' && is_dir($widgets_dir . '/' . $item)) {
                $manifest_file = $widgets_dir . '/' . $item . '/manifest.json';
                if (file_exists($manifest_file)) {
                    $manifest = json_decode(file_get_contents($manifest_file), true);
                    $widgets[] = [
                        'id' => $manifest['id'] ?? $item,
                        'name' => $manifest['name'] ?? $item,
                        'version' => $manifest['version'] ?? 'unknown',
                        'size' => $manifest['widget']['size'] ?? null
                    ];
                }
            }
        }
    }
    
    echo json_encode([
        'success' => true,
        'message' => 'Directories touched. Please click "Scan directory" in Administration → Modules to complete registration.',
        'timestamp' => date('Y-m-d H:i:s'),
        'modules_found' => count($modules),
        'widgets_found' => count($widgets),
        'modules' => $modules,
        'widgets' => $widgets
    ], JSON_PRETTY_PRINT);
    
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode([
        'success' => false,
        'error' => $e->getMessage()
    ], JSON_PRETTY_PRINT);
}
