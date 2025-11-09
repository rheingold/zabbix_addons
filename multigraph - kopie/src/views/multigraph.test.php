<?php
/**
 * @var CView $this
 * @var array $data
 */

$widget = (new CWidget())
    ->setTitle($data['title'])
    ->addItem(
        (new CDiv([
            new CTag('h2', true, $data['message']),
            new CTag('p', true, 'Timestamp: ' . $data['timestamp']),
            new CTag('p', true, 'This is a Phase B1 test to verify the Multigraph module is functioning correctly.')
        ]))->addClass('multigraph-test')
    );

$widget->show();
