<?php declare(strict_types = 0);
/**
 * ============================================================================
 * File: MacroList.php
 * Created: 2025-12-02
 *
 * Lead & Architecture: lukas@plachy.eu
 * Development: Claude Sonnet 4 (AI Assistant, Anthropic)
 *
 * License: MIT (unless specified otherwise in project root)
 * ============================================================================
 *
 * PURPOSE:
 * AJAX endpoint for fetching _LIST} macros from a host or template.
 * Filters macros ending with _LIST} suffix and returns their details.
 *
 * RELATIONS:
 * - Called by: views/editor.js.php (AJAX request)
 * - Uses: Zabbix API::UserMacro()->get()
 * - Returns: JSON response with macro list
 *
 * RESPONSE FORMAT:
 * Success: {'macros': [{'hostmacroid': string, 'macro': string, 'value': string, 'type': int}, ...]}
 * Error: {'error': string}
 */

namespace Modules\ListEdit\Actions;

use CController;
use CControllerResponseData;
use CWebUser;
use API;

/**
 * Macro List Controller
 *
 * Handles AJAX requests for fetching _LIST} macros from a host/template.
 */
class MacroList extends CController {

    /**
     * Initialize controller
     */
    protected function init(): void {
        $this->disableCsrfValidation(); // AJAX endpoint
    }

    /**
     * Check user permissions
     */
    protected function checkPermissions(): bool {
        return $this->getUserType() >= USER_TYPE_ZABBIX_USER;
    }

    /**
     * Check input parameters
     */
    protected function checkInput(): bool {
        $fields = [
            'hostid' => 'db hosts.hostid',
            'type' => 'string'  // 'host' or 'template'
        ];

        $ret = $this->validateInput($fields);

        if (!$ret) {
            $this->setResponse(
                new CControllerResponseData(['error' => 'Invalid input parameters'])
            );
        }

        if ($ret && !$this->hasInput('hostid')) {
            header('Content-Type: application/json');
            echo json_encode(['error' => 'hostid is required']);
            exit;
        }

        return $ret;
    }

    /**
     * Main action - fetch and return _LIST} macros
     */
    protected function doAction(): void {
        $hostid = $this->getInput('hostid');
        $type = $this->getInput('type', 'template');

        try {
            // Base output fields + hostid to identify origin
            $output_fields = ['hostmacroid', 'hostid', 'macro', 'value', 'type', 'description'];

            // If template selected, return only template macros
            if ($type === 'template') {
                $tpl_macros = API::UserMacro()->get([
                    'output' => $output_fields,
                    'templateids' => [$hostid],
                    'sortfield' => 'macro'
                ]);

                $list_macros = [];
                foreach ($tpl_macros as $macro) {
                    if (preg_match('/_LIST\}$/i', $macro['macro'])) {
                        // Annotate origin for UI purposes
                        $macro['origin'] = 'template';
                        $macro['origin_name'] = null; // filled only in host case
                        $list_macros[] = $macro;
                    }
                }

                header('Content-Type: application/json; charset=UTF-8');
                echo json_encode(['macros' => $list_macros]);
                exit;
            }

            // Host selected: include both host macros and inherited template macros
            $host_macros = API::UserMacro()->get([
                'output' => $output_fields,
                'hostids' => [$hostid],
                'sortfield' => 'macro'
            ]);

            // DEBUG: Write to file
            $debug_file = '/tmp/macrolist_debug.log';
            $debug_data = "=== MacroList DEBUG " . date('Y-m-d H:i:s') . " ===\n";
            $debug_data .= "hostid: $hostid\n";
            $debug_data .= "type: $type\n";
            $debug_data .= "host_macros count: " . count($host_macros) . "\n";
            $debug_data .= "host_macros: " . json_encode($host_macros, JSON_PRETTY_PRINT) . "\n\n";
            file_put_contents($debug_file, $debug_data, FILE_APPEND);

            // Get parent templates of the host (including nested ancestors)
            $hosts = API::Host()->get([
                'hostids' => [$hostid],
                'output' => ['hostid', 'host', 'name'],
                'selectParentTemplates' => ['templateid', 'name', 'host']
            ]);

            // DEBUG: Log host and templates
            $debug_data = "hosts: " . json_encode($hosts, JSON_PRETTY_PRINT) . "\n\n";
            file_put_contents($debug_file, $debug_data, FILE_APPEND);

            $template_ids = [];
            $template_names = [];
            if (!empty($hosts) && !empty($hosts[0]['parentTemplates'])) {
                foreach ($hosts[0]['parentTemplates'] as $tpl) {
                    $template_ids[] = $tpl['templateid'];
                    $template_names[$tpl['templateid']] = $tpl['name'] ?? $tpl['host'] ?? ('templateid:' . $tpl['templateid']);
                }
            }

            // Resolve nested/ancestor templates recursively (BFS)
            if (!empty($template_ids)) {
                $visited = array_fill_keys($template_ids, true);
                $queue = $template_ids;
                $debug_data = "BFS starting with queue: " . json_encode($queue) . "\n";
                file_put_contents($debug_file, $debug_data, FILE_APPEND);
                
                while (!empty($queue)) {
                    // Fetch parents of current batch of templates
                    $batch = array_splice($queue, 0, min(100, count($queue)));
                    $debug_data = "BFS batch: " . json_encode($batch) . "\n";
                    file_put_contents($debug_file, $debug_data, FILE_APPEND);
                    
                    $templates = API::Template()->get([
                        'templateids' => $batch,
                        'output' => ['templateid', 'name', 'host'],
                        'selectParentTemplates' => ['templateid', 'name', 'host']
                    ]);

                    $debug_data = "BFS templates response: " . json_encode($templates, JSON_PRETTY_PRINT) . "\n";
                    file_put_contents($debug_file, $debug_data, FILE_APPEND);

                    foreach ($templates as $t) {
                        if (!empty($t['parentTemplates'])) {
                            foreach ($t['parentTemplates'] as $pt) {
                                $ptid = $pt['templateid'];
                                if (!isset($visited[$ptid])) {
                                    $visited[$ptid] = true;
                                    $template_ids[] = $ptid;
                                    $template_names[$ptid] = $pt['name'] ?? $pt['host'] ?? ('templateid:' . $ptid);
                                    $queue[] = $ptid;
                                    $debug_data = "BFS added ancestor: $ptid ({$template_names[$ptid]})\n";
                                    file_put_contents($debug_file, $debug_data, FILE_APPEND);
                                }
                            }
                        }
                    }
                }
                $debug_data = "BFS final template_ids: " . json_encode($template_ids) . "\n\n";
                file_put_contents($debug_file, $debug_data, FILE_APPEND);
            }

            $inherited_macros = [];
            if (!empty($template_ids)) {
                // DEBUG: Log template IDs before fetching macros
                $debug_data = "template_ids: " . json_encode($template_ids, JSON_PRETTY_PRINT) . "\n";
                $debug_data .= "template_names: " . json_encode($template_names, JSON_PRETTY_PRINT) . "\n\n";
                file_put_contents($debug_file, $debug_data, FILE_APPEND);

                // Fetch macros for each template - use hostids for templates to get template-level macros
                $tpl_macros = [];
                foreach ($template_ids as $tid) {
                    // Templates are queried via hostids parameter, NOT templateids
                    // templateids returns host overrides, hostids returns the template's own macros
                    $macros_for_tpl = API::UserMacro()->get([
                        'output' => $output_fields,
                        'hostids' => [$tid],  // Changed from templateids to hostids
                        'sortfield' => 'macro'
                    ]);
                    
                    $debug_data = "Template $tid ({$template_names[$tid]}): " . count($macros_for_tpl) . " macros\n";
                    $debug_data .= json_encode($macros_for_tpl, JSON_PRETTY_PRINT) . "\n";
                    file_put_contents($debug_file, $debug_data, FILE_APPEND);
                    
                    foreach ($macros_for_tpl as $m) {
                        $tpl_macros[] = $m;
                    }
                }

                // DEBUG: Log template macros
                $debug_data = "\ntpl_macros total count: " . count($tpl_macros) . "\n";
                $debug_data .= "tpl_macros: " . json_encode($tpl_macros, JSON_PRETTY_PRINT) . "\n\n";
                file_put_contents($debug_file, $debug_data, FILE_APPEND);

                foreach ($tpl_macros as $macro) {
                    if (preg_match('/_LIST\}$/i', $macro['macro'])) {
                        // Annotate origin for UI
                        $macro['origin'] = 'template';
                        // Map template name via template hostid if available
                        $origin_name = null;
                        if (!empty($macro['hostid'])) {
                            // hostid here is the template id context
                            $origin_name = $template_names[$macro['hostid']] ?? null;
                        }
                        $macro['origin_name'] = $origin_name;
                        $inherited_macros[] = $macro;
                    }
                }
            }

            // Filter host macros ending with _LIST} and annotate origin
            $filtered_host_macros = [];
            foreach ($host_macros as $macro) {
                if (preg_match('/_LIST\}$/i', $macro['macro'])) {
                    $macro['origin'] = 'host';
                    $macro['origin_name'] = null;
                    $filtered_host_macros[] = $macro;
                }
            }

            // Merge: keep all unique macros by hostmacroid
            // Host macros take priority, but show all template variations
            $merged = [];
            $host_macro_names = [];
            
            // First add host macros
            foreach ($filtered_host_macros as $m) {
                $merged[$m['hostmacroid']] = $m;
                $host_macro_names[$m['macro']] = true;
            }
            
            // Then add template macros, but skip if same macro name exists in host
            foreach ($inherited_macros as $m) {
                if (!isset($host_macro_names[$m['macro']])) {
                    $merged[$m['hostmacroid']] = $m;
                }
            }

            // Sort by macro name, then by hostmacroid for stable UI
            $list_macros = array_values($merged);
            usort($list_macros, function($a, $b) {
                $cmp = strcmp($a['macro'], $b['macro']);
                if ($cmp !== 0) return $cmp;
                return strcmp($a['hostmacroid'], $b['hostmacroid']);
            });

            // DEBUG: Log final merged result
            $debug_data = "filtered_host_macros count: " . count($filtered_host_macros) . "\n";
            $debug_data .= "inherited_macros count: " . count($inherited_macros) . "\n";
            $debug_data .= "merged count: " . count($list_macros) . "\n";
            $debug_data .= "final list_macros: " . json_encode($list_macros, JSON_PRETTY_PRINT) . "\n";
            $debug_data .= "=== END DEBUG ===\n\n";
            file_put_contents($debug_file, $debug_data, FILE_APPEND);

            header('Content-Type: application/json; charset=UTF-8');
            echo json_encode(['macros' => $list_macros]);
            exit;

        } catch (\Exception $e) {
            header('Content-Type: application/json; charset=UTF-8');
            echo json_encode(['error' => $e->getMessage()]);
            exit;
        }
    }
}
