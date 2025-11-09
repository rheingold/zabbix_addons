#!/usr/bin/php
<?php
define('ZBX_SESSION_NAME', '');
require_once '/usr/share/zabbix/include/config.inc.php';
require_once '/usr/share/zabbix/include/classes/core/APP.php';

APP::getInstance()->run(APP::EXEC_MODE_DEFAULT);

// Run module scan
$module_manager = new CModuleManager(APP::getRootDir());
$db_modules = API::Module()->get([
    'output' => ['id', 'relative_path'],
    'sortfield' => 'relative_path',
    'preservekeys' => true
]);

$db_moduleids = [];
foreach ($db_modules as $moduleid => $db_module) {
    $db_moduleids[$db_module['relative_path']] = $moduleid;
}

$db_modules_create = [];
foreach (['widgets', 'modules'] as $modules_dir) {
    foreach (new DirectoryIterator(APP::getRootDir().'/'.$modules_dir) as $item) {
        if (!$item->isDir() || $item->isDot()) {
            continue;
        }
        
        $relative_path = $modules_dir.'/'.$item->getFilename();
        $manifest = $module_manager->addModule($relative_path);
        
        if (!$manifest) {
            continue;
        }
        
        if (!array_key_exists($relative_path, $db_moduleids)) {
            $db_modules_create[] = [
                'id' => $manifest['id'],
                'relative_path' => $relative_path,
                'status' => 0, // MODULE_STATUS_DISABLED
                'config' => $manifest['config']
            ];
            echo "Found new module: {$manifest['name']} at $relative_path\n";
        }
    }
}

if ($db_modules_create) {
    $result = API::Module()->create($db_modules_create);
    if ($result) {
        echo "Successfully added " . count($db_modules_create) . " module(s)\n";
    } else {
        echo "Failed to add modules\n";
    }
} else {
    echo "No new modules found\n";
}
