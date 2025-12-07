<?php declare(strict_types = 1);
namespace Modules\ListEdit;

use APP;
use CController as CAction;
use CMenu;
use CMenuItem;
use Zabbix\Core\CModule;

// Version compatibility: ensure Core\CModule alias exists for newer versions
if (version_compare(ZABBIX_VERSION, '6.4.0', '>')) {
    if (!class_exists('\\Core\\CModule', false)) {
        class_alias('\\Zabbix\\Core\\CModule', '\\Core\\CModule');
    }
}

class Module extends CModule {
    public function init(): void {
        $this->registerMenuEntry();
    }

    protected function registerMenuEntry(): void {
        /** @var CMenu $menu */
        $menu = APP::Component()->get('menu.main');
        
        // Try registering under Monitoring instead of Administration
        $menu
            ->find(_('Monitoring'))
            ->getSubMenu()
            ->insertAfter(_('Hosts'), 
                (new CMenuItem(_('Macro List Editor')))
                    ->setAction('listedit.editor')
            );
    }

    public function getActions(): array {
        return [
            'listedit.editor' => [
                'class' => 'Editor',
                'view' => 'editor'
            ],
            'listedit.hostlist' => [
                'class' => 'HostList',
                'layout' => 'layout.json'
            ],
            'listedit.macrolist' => [
                'class' => 'MacroList',
                'layout' => 'layout.json'
            ],
            'listedit.macroupdate' => [
                'class' => 'MacroUpdate',
                'layout' => 'layout.json'
            ]
        ];
    }

    public function onBeforeAction(CAction $action): void {
        // No-op for now
    }

    public function onTerminate(CAction $action): void {
        // No-op for now
    }
}
