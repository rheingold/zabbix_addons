<?php
// Test if module loads and registers actions
require_once '/usr/share/zabbix/include/config.inc.php';

$manager = APP::ModuleManager();
$modules = $manager->getModules();

echo "Loaded modules:\n";
foreach ($modules as $id => $mod) {
    echo "  - $id: " . get_class($mod) . "\n";
    if ($id === 'listedit') {
        echo "    Actions: " . json_encode($mod->getActions()) . "\n";
    }
}

echo "\nAll registered actions:\n";
$actions = $manager->getActions();
foreach ($actions as $name => $data) {
    if (strpos($name, 'listedit') !== false) {
        echo "  - $name: " . json_encode($data) . "\n";
    }
}
