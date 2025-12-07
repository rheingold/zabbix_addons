<?php
$class = 'Modules\\ListEdit\\Actions\\Editor';
$file = '/usr/share/zabbix/modules/listedit/actions/Editor.php';

echo "Class: $class\n";
echo "File exists: " . (file_exists($file) ? 'yes' : 'no') . "\n";

if (file_exists($file)) {
    require_once $file;
    echo "File loaded\n";
    echo "Class exists: " . (class_exists($class) ? 'yes' : 'no') . "\n";
}
