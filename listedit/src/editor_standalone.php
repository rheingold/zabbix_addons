<?php
// Standalone Macro List Editor (no Zabbix MVC)
// Usage: /modules/listedit/editor_standalone.php?token=YOUR_API_TOKEN
// Optional: &api=https://zabbix.plachy.eu/api_jsonrpc.php

$api = $_GET['api'] ?? 'https://zabbix.plachy.eu/api_jsonrpc.php';
$token = $_GET['token'] ?? '';

header('Content-Type: text/html; charset=utf-8');
?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Macro List Editor (Standalone)</title>
  <style>
    body { font-family: system-ui, Arial, sans-serif; margin: 20px; }
    .row { display: flex; gap: 12px; align-items: center; margin-bottom: 12px; }
    select, input, textarea, button { font-size: 14px; padding: 6px 8px; }
    textarea { width: 100%; height: 160px; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
    .card { border: 1px solid #ccc; border-radius: 6px; padding: 12px; }
    .muted { color: #666; }
    .ok { color: #0a8020; }
    .err { color: #b00020; }
  </style>
</head>
<body>
  <h2>Macro List Editor (Standalone)</h2>
  <p class="muted">API: <code id="api-url"></code></p>
  <div class="row">
    <label>Token: <input id="api-token" type="password" placeholder="Paste Zabbix API token" /></label>
    <button id="connect">Connect</button>
    <span id="status" class="muted"></span>
  </div>

  <div class="grid">
    <div class="card">
      <div class="row">
        <label>Host/Template:</label>
        <select id="host-select"><option value="">Select...</option></select>
        <button id="load-macros">Load Macros</button>
      </div>
      <div class="row">
        <label>Macro:</label>
        <select id="macro-select"><option value="">Select macro ending with _LIST}</option></select>
      </div>
      <div class="row">
        <button id="load-value">Load Value</button>
        <button id="save-value">Save Value</button>
        <span id="op-status" class="muted"></span>
      </div>
    </div>
    <div class="card">
      <div class="row"><strong>Value</strong></div>
      <textarea id="macro-value" placeholder="Pipe-separated or JSON array values..."></textarea>
      <div class="row">
        <button id="format-pipe">Format → Pipe</button>
        <button id="format-json">Format → JSON</button>
      </div>
    </div>
  </div>

<script>
const apiUrlEl = document.getElementById('api-url');
const tokenEl = document.getElementById('api-token');
const connectBtn = document.getElementById('connect');
const statusEl = document.getElementById('status');
const hostSel = document.getElementById('host-select');
const loadMacrosBtn = document.getElementById('load-macros');
const macroSel = document.getElementById('macro-select');
const loadValBtn = document.getElementById('load-value');
const saveValBtn = document.getElementById('save-value');
const opStatusEl = document.getElementById('op-status');
const valArea = document.getElementById('macro-value');
const fmtPipeBtn = document.getElementById('format-pipe');
const fmtJsonBtn = document.getElementById('format-json');

const api = new URLSearchParams(location.search).get('api') || '<?php echo htmlspecialchars($api, ENT_QUOTES); ?>';
apiUrlEl.textContent = api;
const qsToken = new URLSearchParams(location.search).get('token') || '<?php echo htmlspecialchars($token, ENT_QUOTES); ?>';
if (qsToken) tokenEl.value = qsToken;

let authToken = '';

async function rpc(method, params = {}) {
  const payload = { jsonrpc: '2.0', method, params, id: Date.now(), auth: authToken || undefined };
  const res = await fetch(api, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload) });
  const data = await res.json();
  if (data.error) throw new Error(data.error.message || 'API error');
  return data.result;
}

connectBtn.onclick = async () => {
  authToken = tokenEl.value.trim();
  statusEl.textContent = authToken ? 'Token set' : 'Token missing';
  statusEl.className = authToken ? 'ok' : 'err';
  if (!authToken) return;
  try {
    const ver = await rpc('apiinfo.version');
    statusEl.textContent = `Connected (API ${ver})`;
    statusEl.className = 'ok';
    await loadHosts();
  } catch (e) {
    statusEl.textContent = `Connect failed: ${e.message}`;
    statusEl.className = 'err';
  }
};

async function loadHosts() {
  hostSel.innerHTML = '<option value="">Loading...</option>';
  const hosts = await rpc('host.get', { output: ['hostid','name'], selectParentTemplates: ['templateid','name'] });
  const templates = await rpc('template.get', { output: ['templateid','name'] });
  const options = [];
  for (const t of templates) options.push({ id: t.templateid, name: `[T] ${t.name}` });
  for (const h of hosts) options.push({ id: h.hostid, name: `[H] ${h.name}` });
  hostSel.innerHTML = '<option value="">Select...</option>' + options.map(o => `<option value="${o.id}">${o.name}</option>`).join('');
}

loadMacrosBtn.onclick = async () => {
  const id = hostSel.value;
  if (!id) return;
  macroSel.innerHTML = '<option value="">Loading...</option>';
  const macros = await rpc('usermacro.get', { output: ['macro','value'], hostids: [id] });
  const listMacros = macros.filter(m => /_LIST}\s*$/.test(m.macro));
  if (!listMacros.length) {
    macroSel.innerHTML = '<option value="">No _LIST} macros</option>';
    return;
  }
  macroSel.innerHTML = '<option value="">Select macro...</option>' + listMacros.map(m => `<option value="${m.macro}">${m.macro}</option>`).join('');
};

loadValBtn.onclick = async () => {
  const id = hostSel.value; const macro = macroSel.value;
  if (!id || !macro) return;
  const macros = await rpc('usermacro.get', { output: ['macro','value'], hostids: [id], filter: { macro } });
  const m = macros[0];
  valArea.value = m ? m.value : '';
  opStatusEl.textContent = m ? 'Loaded' : 'Not found';
};

saveValBtn.onclick = async () => {
  const id = hostSel.value; const macro = macroSel.value; const value = valArea.value;
  if (!id || !macro) return;
  try {
    await rpc('usermacro.update', { hostmacroid: null, hostid: id, macro, value });
    opStatusEl.textContent = 'Saved'; opStatusEl.className = 'ok';
  } catch (e) {
    opStatusEl.textContent = `Save failed: ${e.message}`; opStatusEl.className = 'err';
  }
};

fmtPipeBtn.onclick = () => {
  try {
    const arr = JSON.parse(valArea.value);
    if (Array.isArray(arr)) valArea.value = arr.join('|');
  } catch {}
};

fmtJsonBtn.onclick = () => {
  const parts = valArea.value.split('|').map(s => s.trim()).filter(Boolean);
  valArea.value = JSON.stringify(parts, null, 2);
};
</script>
</body>
</html>
