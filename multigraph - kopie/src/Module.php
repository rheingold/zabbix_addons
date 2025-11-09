<?php declare(strict_types = 1);

namespace Modules\Multigraph;

use APP;
use CController as CAction;
use CMenu;
use CMenuItem;
use Zabbix\Core\CModule;

/**
 * Multigraph Module - Enhanced multi-graph visualization
 * 
 * REGISTRATION NOTE (Phase A - Nov 5, 2025):
 * After deployment, modules must be discovered via UI:
 * Administration → Modules → "Scan directory" button
 * This triggers module.scan action which registers new modules in database.
 * Module files alone are not sufficient - DB registration required.
 * 
 * PERMISSIONS NOTE (Phase B1 - Nov 5, 2025):
 * Ensure module directories have 755 permissions before Docker build.
 * Run: chmod -R 755 /root/customzabbix/usr/share/zabbix/modules/multigraph
 * Otherwise subdirectories (actions/, views/) won't be readable in container.
 */
class Module extends CModule {

    /**
     * Initialize module
     */
    public function init(): void {
        // Phase B1: Add menu entry for testing
        /** @var CMenu $menu */
        $menu = APP::Component()->get('menu.main');
        $menu
            ->findOrAdd(_('Monitoring'))
            ->getSubMenu()
            ->add(
                (new CMenuItem(_('Multigraph Test')))
                    ->setAction('multigraph.test')
            );
    }

    /**
     * Called before action execution
     */
    public function onBeforeAction(CAction $action): void {
        // Future: Pre-action hooks
    }

    /**
     * Called after action execution
     */
    public function onTerminate(CAction $action): void {
        // Future: Post-action cleanup
    }
}
