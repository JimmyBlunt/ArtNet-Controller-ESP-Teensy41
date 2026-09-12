/* Local Teensy control desk. No external assets or network preview services. */
"use strict";
const PROFILE = "PJRC_OCTO_ADAPTER_T41";
const PINS = [2, 14, 7, 8, 6, 20, 21, 5];
const ORDERS = ["RGB", "RBG", "GRB", "GBR", "BRG", "BGR"];
const $ = id => document.getElementById(id);
const clone = value => JSON.parse(JSON.stringify(value));
let config = null, status = null, dirty = false, connected = false, busy = false;
let packetSample = null, pollTimer = null, requestQueue = Promise.resolve();
const events = [];

function log(message) {
  events.unshift(`${new Date().toLocaleTimeString("de-DE")}  ${message}`);
  $("logs").textContent = events.slice(0, 40).join("\n");
}
function notice(message, error = false) {
  $("notice").textContent = message;
  $("notice").classList.toggle("error", error);
  if (message) log(message);
}
function request(path, body) {
  // One request at a time, including polling and mutations, to keep the MCU responsive.
  const run = async () => {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 6500);
    try {
      const response = await fetch(path, {
        method: body === undefined ? "GET" : "POST", cache: "no-store",
        headers: body === undefined ? {} : {"Content-Type": "application/json"},
        body: body === undefined ? undefined : JSON.stringify(body), signal: controller.signal
      });
      const data = await response.json();
      if (!response.ok || data.ok === false) throw new Error(data.error || `HTTP ${response.status}`);
      return data;
    } finally { clearTimeout(timeout); }
  };
  const result = requestQueue.then(run, run);
  requestQueue = result.catch(() => {});
  return result;
}
function integer(value, min, max, label) {
  if (!Number.isInteger(value) || value < min || value > max) throw new Error(`${label}: ganze Zahl von ${min} bis ${max} erforderlich.`);
}
function ipv4(value, label) {
  if (typeof value !== "string" || !/^(\d{1,3}\.){3}\d{1,3}$/.test(value) || value.split(".").some(part => Number(part) > 255)) throw new Error(`${label}: gültige IPv4-Adresse erforderlich.`);
}
function validate(candidate) {
  if (!candidate || typeof candidate !== "object" || Array.isArray(candidate)) throw new Error("Ungültige Konfigurationsdatei.");
  if (candidate.schemaVersion !== 1 || candidate.hardwareProfile !== PROFILE) throw new Error("Nur Schema 1 mit PJRC_OCTO_ADAPTER_T41 wird unterstützt. ESP-GPIO-Profile können nicht importiert werden.");
  integer(candidate.brightness, 0, 255, "Helligkeit");
  integer(candidate.targetFps, 1, 60, "Ziel-FPS");
  const network = candidate.network;
  if (!network || typeof network.dhcp !== "boolean") throw new Error("Netzwerk/DHCP fehlt oder ist ungültig.");
  for (const [key, label] of [["ip", "IP-Adresse"], ["netmask", "Netzmaske"], ["gateway", "Gateway"], ["dns", "DNS"]]) ipv4(network[key], label);
  if (!network.dhcp) {
    const number = address => address.split(".").reduce((value, octet) => ((value << 8) | Number(octet)) >>> 0, 0);
    const ip = number(network.ip), mask = number(network.netmask), hostMask = (~mask) >>> 0;
    const gateway = number(network.gateway), dns = number(network.dns);
    const unicast = address => (address >>> 24) > 0 && (address >>> 24) < 224 && (address >>> 24) !== 127;
    const host = address => unicast(address) && ((address & hostMask) >>> 0) !== 0 && ((address & hostMask) >>> 0) !== hostMask;
    if (!mask || hostMask < 3 || (hostMask & (hostMask + 1))) throw new Error("Statische Netzmaske muss zusammenhängend sein (/1 bis /30).");
    if (!host(ip)) throw new Error("Statische IP muss eine gültige Hostadresse sein.");
    if (gateway && (!host(gateway) || (gateway & mask) !== (ip & mask) || gateway === ip)) throw new Error("Gateway muss 0.0.0.0 oder ein anderer Host im selben Subnetz sein.");
    if (dns && !unicast(dns)) throw new Error("DNS muss 0.0.0.0 oder eine gültige Unicast-Adresse sein.");
  }
  if (!Array.isArray(candidate.outputs) || candidate.outputs.length !== 8) throw new Error("Das Boardprofil benötigt genau acht physische Ausgänge.");
  const ranges = [];
  let expectedOffset = 0, expectedUniverses = 0;
  candidate.outputs.forEach((out, id) => {
    const label = `Ausgang ${id + 1}`;
    if (!out || out.id !== id || out.dataPin !== PINS[id] || out.clockPin !== -1 || out.type !== "WS2812B") throw new Error(`${label}: falsche ID, Pinbelegung oder LED-Typ für dieses Boardprofil.`);
    if (typeof out.enabled !== "boolean" || typeof out.reverse !== "boolean" || !ORDERS.includes(out.colorOrder)) throw new Error(`${label}: ungültige Aktivierung, Richtung oder Farbreihenfolge.`);
    integer(out.pixelCount, 0, 1200, `${label} LED-Anzahl`);
    integer(out.startUniverse, 0, 32767, `${label} Start-Universe`);
    integer(out.targetFps, 1, 60, `${label} Ziel-FPS`);
    integer(out.startPixel, 0, 9600, `${label} Pufferposition`);
    if (out.targetFps !== candidate.targetFps || out.startPixel !== expectedOffset) throw new Error(`${label}: abgeleitete Ziel-FPS oder Pufferposition passen nicht zur Konfiguration.`);
    if (out.enabled && out.pixelCount === 0) throw new Error(`${label}: aktiver Ausgang benötigt mindestens eine LED.`);
    const end = out.startUniverse + Math.ceil(out.pixelCount / 170) - 1;
    if (end > 32767) throw new Error(`${label}: Universe-Bereich endet oberhalb von 32767.`);
    if (out.enabled) {
      for (const previous of ranges) if (out.startUniverse <= previous.end && end >= previous.start) throw new Error(`${label}: Universe-Bereich überschneidet sich mit Ausgang ${previous.id + 1}.`);
      ranges.push({start: out.startUniverse, end, id});
      expectedOffset += out.pixelCount;
      expectedUniverses += Math.ceil(out.pixelCount / 170);
    }
  });
  if (candidate.pixelCount !== undefined && candidate.pixelCount !== expectedOffset) throw new Error("Gesamtpixelzahl passt nicht zu den aktiven Ausgängen.");
  if (candidate.universeCount !== undefined && candidate.universeCount !== expectedUniverses) throw new Error("Universe-Anzahl passt nicht zu den aktiven Ausgängen.");
  return candidate;
}
function numericInput(id, value, min, max) {
  const input = document.createElement("input");
  Object.assign(input, {id, type: "number", min, max, step: 1, value, required: true});
  input.dataset.edit = "";
  return input;
}
function field(labelText, control) {
  const label = document.createElement("label");
  label.append(document.createTextNode(labelText), control);
  return label;
}
function checkbox(id, value) {
  const input = document.createElement("input");
  Object.assign(input, {id, type: "checkbox", checked: value});
  input.dataset.edit = "";
  return input;
}
function readOnly(value) {
  const input = document.createElement("input");
  Object.assign(input, {value, readOnly: true, tabIndex: -1});
  return input;
}
function buildEditor(candidate) {
  $("cfgBrightness").value = candidate.brightness;
  $("cfgFps").value = candidate.targetFps;
  $("cfgDhcp").checked = candidate.network.dhcp;
  for (const key of ["ip", "netmask", "gateway", "dns"]) $("cfg" + key[0].toUpperCase() + key.slice(1)).value = candidate.network[key];
  const fragment = document.createDocumentFragment();
  candidate.outputs.forEach((out, id) => {
    const prefix = `out${id}`;
    const box = document.createElement("fieldset"); box.className = "outedit";
    const legend = document.createElement("legend"); legend.textContent = `OUT ${id + 1} · Teensy Pin ${PINS[id]}`; box.append(legend);
    const toggle = field("Ausgang aktiv", checkbox(prefix + "Enabled", out.enabled)); toggle.className = "toggle"; box.append(toggle);
    const grid = document.createElement("div"); grid.className = "config-grid";
    const fps = readOnly(candidate.targetFps); fps.id = prefix + "Fps";
    grid.append(field("LED-Anzahl · 0–1200", numericInput(prefix + "Count", out.pixelCount, 0, 1200)), field("Start-Universe · 0-basiert", numericInput(prefix + "Universe", out.startUniverse, 0, 32767)), field("Ziel-FPS · global", fps));
    const select = document.createElement("select"); select.id = prefix + "Order"; select.dataset.edit = "";
    ORDERS.forEach(order => { const option = document.createElement("option"); option.value = option.textContent = order; select.append(option); }); select.value = out.colorOrder;
    grid.append(field("Farbreihenfolge", select), field("LED-Typ / fester Datenpin", readOnly(`WS2812B / Pin ${PINS[id]}`)));
    const reverse = field("Pixelrichtung umkehren", checkbox(prefix + "Reverse", out.reverse)); reverse.className = "toggle"; grid.append(reverse);
    box.append(grid);
    const range = document.createElement("p"); range.id = prefix + "Range"; range.className = "range"; box.append(range);
    fragment.append(box);
  });
  $("outputEditor").replaceChildren(fragment);
  updateDraftSummary(); updateControls();
}
function getDraft() {
  if (!config) throw new Error("Noch keine Konfiguration geladen.");
  const draft = clone(config);
  draft.brightness = $("cfgBrightness").value === "" ? NaN : Number($("cfgBrightness").value);
  draft.targetFps = $("cfgFps").value === "" ? NaN : Number($("cfgFps").value);
  draft.network.dhcp = $("cfgDhcp").checked;
  for (const key of ["ip", "netmask", "gateway", "dns"]) draft.network[key] = $("cfg" + key[0].toUpperCase() + key.slice(1)).value.trim();
  let offset = 0;
  draft.outputs.forEach((out, id) => {
    const prefix = `out${id}`;
    out.enabled = $(prefix + "Enabled").checked;
    for (const [property, suffix] of [["pixelCount", "Count"], ["startUniverse", "Universe"]]) out[property] = $(prefix + suffix).value === "" ? NaN : Number($(prefix + suffix).value);
    out.targetFps = draft.targetFps;
    out.colorOrder = $(prefix + "Order").value;
    out.reverse = $(prefix + "Reverse").checked;
    out.startPixel = offset;
    if (out.enabled && Number.isFinite(out.pixelCount)) offset += out.pixelCount;
  });
  draft.pixelCount = offset;
  draft.universeCount = draft.outputs.reduce((sum, out) => sum + (out.enabled ? Math.ceil(out.pixelCount / 170) : 0), 0);
  return draft;
}
function updateDraftSummary() {
  if (!config) return;
  const draft = getDraft();
  let pixels = 0, universes = 0;
  draft.outputs.forEach((out, id) => {
    $("out" + id + "Fps").value = draft.targetFps;
    const count = Number.isFinite(out.pixelCount) ? Math.ceil(out.pixelCount / 170) : 0;
    $("out" + id + "Range").textContent = out.enabled ? `Universe ${out.startUniverse}–${out.startUniverse + count - 1} · ${count} Universes · Puffer ab Pixel ${out.startPixel}` : "Deaktiviert · kein aktiver Universe-Bereich";
    if (out.enabled && Number.isFinite(out.pixelCount)) { pixels += out.pixelCount; universes += count; }
  });
  $("cfgPixels").value = pixels; $("cfgUniverses").value = universes;
  $("draftNotice").hidden = !dirty;
}
function buildCards() {
  const fragment = document.createDocumentFragment();
  config.outputs.forEach((out, id) => {
    const card = document.createElement("article"); card.className = "panel output-card" + (out.enabled ? "" : " inactive");
    const top = document.createElement("div"); top.className = "card-top";
    const title = document.createElement("strong"); title.textContent = `OUT ${id + 1} · Pin ${PINS[id]}`;
    const badge = document.createElement("span"); badge.className = "badge"; badge.textContent = out.enabled ? "Aktiv konfiguriert" : "Deaktiviert"; top.append(title, badge);
    const count = document.createElement("div"); count.className = "led-count"; count.textContent = out.pixelCount + " "; const suffix = document.createElement("small"); suffix.textContent = "LEDs"; count.append(suffix);
    const detail = document.createElement("p"); detail.className = "muted"; detail.textContent = out.enabled ? `Universe ${out.startUniverse}–${out.startUniverse + Math.ceil(out.pixelCount / 170) - 1} · ${out.colorOrder} · ${out.reverse ? "umgekehrt" : "vorwärts"}` : "Zum Aktivieren unter Ausgänge konfigurieren.";
    const button = document.createElement("button"); button.dataset.test = String(id + 1); button.textContent = `OUT ${id + 1} identifizieren · 30 s`; button.disabled = true;
    card.append(top, count, detail, button); fragment.append(card);
  });
  $("outputCards").replaceChildren(fragment);
  $("statPixels").textContent = config.outputs.reduce((sum, out) => sum + (out.enabled ? out.pixelCount : 0), 0).toLocaleString("de-DE");
}
function isStopped() { return !!status && ["DISARMED", "STOPPED"].includes(status.state); }
function updateControls() {
  const available = connected && !busy;
  const editable = available && !!config;
  document.querySelectorAll("[data-edit]").forEach(input => { input.disabled = !editable; });
  document.querySelectorAll("[data-network]").forEach(input => { input.disabled = !editable || $("cfgDhcp").checked; });
  $("stop").disabled = !available;
  $("start").disabled = !available || !config || !isStopped() || !!status?.reboot_required || !!status?.configuration_fault;
  $("apply").disabled = !editable || !isStopped() || !!status?.config_locked;
  $("save").disabled = !editable || !isStopped() || dirty;
  $("reload").disabled = !available;
  $("export").disabled = !config || busy;
  $("import").disabled = !editable;
  $("reboot").disabled = !available || !isStopped() || !!status?.unsaved;
  const testReady = available && !!config && isStopped() && !status?.reboot_required && !status?.configuration_fault;
  $("testAll").disabled = !testReady || !config?.outputs.some(out => out.enabled && out.pixelCount > 0);
  document.querySelectorAll("[data-test]").forEach(button => { const out = config?.outputs[Number(button.dataset.test) - 1]; button.disabled = !testReady || !out?.enabled || !out.pixelCount; });
  $("draftNotice").hidden = !dirty;
  $("configMsg").textContent = !config ? "Konfiguration wird geladen." : dirty ? "Formularentwurf noch nicht angewendet. Speichern ist erst nach Anwenden möglich." : !isStopped() ? "Ausgabe stoppen, um Einstellungen anzuwenden oder dauerhaft zu speichern." : status?.unsaved ? "RAM-Konfiguration geändert; noch nicht dauerhaft gespeichert." : "Konfiguration geladen. Änderungen werden nur durch die jeweiligen Schaltflächen übertragen.";
}
function format(value, digits = 0) { return typeof value === "number" && Number.isFinite(value) ? value.toLocaleString("de-DE", {maximumFractionDigits: digits}) : value === undefined || value === null ? "—" : String(value); }
function renderStatus(next) {
  status = next; connected = true;
  $("connectionStatus").textContent = next.ip || "Verbunden"; $("connectionStatus").className = "badge online";
  $("railState").textContent = "Verbunden"; $("railState").className = "signal online";
  const states = {DISARMED: "Ausgabe nicht gestartet", STOPPED: "Ausgabe gestoppt · Schwarzbild abgeschlossen", ARTNET_RUNNING: "Art-Net-Ausgabe aktiv", TEST_RUNNING: "Ausgangstest aktiv", STOP_WAIT_PREVIOUS_DMA: "Stopp läuft · vorherige Übertragung abwarten", STOP_WAIT_BLACK_DMA: "Stopp läuft · Schwarzbild wird übertragen"};
  $("runtimeState").textContent = states[next.state] || `Ausgabestatus: ${next.state || "unbekannt"}`;
  $("runtimeDetail").textContent = `Ethernet ${next.link ? "verbunden" : "ohne Link"} · ${next.initialized ? "LED-Ausgabe initialisiert" : "LED-Ausgabe noch nicht initialisiert"}`;
  $("rebootNotice").hidden = !next.reboot_required;
  $("storageState").textContent = `${next.unsaved ? "Ungespeicherte Änderungen" : "RAM / Speicher"} · ${next.storage_state || "unbekannt"}`;
  $("statDma").textContent = format(next.dma_fps, 2); $("statIncomplete").textContent = format(next.artnet_incomplete);
  const now = performance.now();
  if (packetSample && now > packetSample.time && next.artnet_packets >= packetSample.packets) $("statPackets").textContent = format((next.artnet_packets - packetSample.packets) * 1000 / (now - packetSample.time), 1);
  else $("statPackets").textContent = "—";
  packetSample = {time: now, packets: next.artnet_packets};
  const metrics = [["Submit-FPS", "submit_fps"], ["DMA-Abschlüsse / s", "dma_fps"], ["Vollständige Art-Net-Bilder / s", "artnet_complete_fps"], ["Bilder eingereicht", "frames_submitted"], ["DMA-Abschlüsse gesamt", "dma_completed"], ["Vollständige Art-Net-Bilder", "artnet_complete"], ["Art-Net-Pakete gesamt", "artnet_packets"], ["Unvollständige Bilder", "artnet_incomplete"], ["Veraltete Bilder", "artnet_stale"], ["Wartende Bilder ersetzt", "artnet_overwritten"], ["UDP-Pakete verworfen", "udp_dropped"], ["show() Maximum · µs", "show_us_max"], ["DMA beobachtet Maximum · µs", "dma_observed_us_max"]];
  const fragment = document.createDocumentFragment();
  for (const [label, key] of metrics) { const row = document.createElement("div"), title = document.createElement("span"), value = document.createElement("strong"); title.textContent = label; value.textContent = format(next[key], 2); row.append(title, value); fragment.append(row); }
  $("diagnostics").replaceChildren(fragment); $("raw").textContent = JSON.stringify(next, null, 2);
  updateControls();
}
async function loadConfig() {
  const next = validate(await request("/api/config"));
  config = clone(next); dirty = false; buildEditor(next); buildCards(); updateControls();
}
async function poll() {
  clearTimeout(pollTimer);
  try {
    if (!busy) { renderStatus(await request("/api/status")); if (!config) await loadConfig(); }
  } catch (error) {
    if (connected) log("Verbindung unterbrochen: " + error.message);
    connected = false; packetSample = null;
    $("connectionStatus").textContent = "Nicht erreichbar"; $("connectionStatus").className = "badge";
    $("railState").textContent = "Nicht verbunden"; $("railState").className = "signal";
    $("runtimeState").textContent = "Laufzeitstatus unbekannt";
    $("runtimeDetail").textContent = "Letzte Daten sind veraltet. Verbindung wird erneut geprüft.";
    $("statDma").textContent = $("statPackets").textContent = "—"; updateControls();
  } finally { pollTimer = setTimeout(poll, 1000); }
}
async function action(work, refresh = true) {
  if (busy) return;
  busy = true; updateControls();
  try { await work(); if (refresh) renderStatus(await request("/api/status")); }
  catch (error) { notice(error.name === "AbortError" ? "Controller antwortet nicht rechtzeitig. Status und Konfiguration vor einem erneuten Schreibversuch neu laden." : error.message, true); }
  finally { busy = false; updateControls(); }
}
function go(page) {
  const allowed = ["overview", "outputs", "diagnostics", "system"];
  if (!allowed.includes(page)) page = "overview";
  document.querySelectorAll("[data-view]").forEach(section => { section.hidden = section.dataset.view !== page; });
  document.querySelectorAll("[data-page]").forEach(button => { const active = button.dataset.page === page; button.classList.toggle("active", active); if (active) button.setAttribute("aria-current", "page"); else button.removeAttribute("aria-current"); });
  if (location.hash !== "#" + page) history.replaceState(null, "", "#" + page);
}
document.addEventListener("click", event => {
  const nav = event.target.closest("[data-page],[data-go]"); if (nav) go(nav.dataset.page || nav.dataset.go);
  const test = event.target.closest("[data-test]"); if (test && !test.disabled) action(async () => { await request("/api/test", {output: Number(test.dataset.test), seconds: 30}); notice(`OUT ${test.dataset.test}: Identifikation gestartet, maximal 30 Sekunden. Leuchtmuster am tatsächlichen Ausgang prüfen.`); });
});
document.addEventListener("input", event => { if (event.target.matches("[data-edit]") && config) { dirty = true; updateDraftSummary(); updateControls(); } });
document.addEventListener("change", event => { if (event.target.matches("[data-edit]") && config) { dirty = true; updateDraftSummary(); updateControls(); } });
$("configForm").addEventListener("submit", event => event.preventDefault());
$("start").onclick = () => action(async () => { await request("/api/start", {}); notice("Art-Net-Ausgabe gestartet."); });
$("stop").onclick = () => action(async () => { await request("/api/stop", {}); notice("Stopp angefordert. Der Schwarzbildabschluss wird abgewartet; den bestätigten Zustand zeigt die Statuszeile. Art-Net danach bei Bedarf ausdrücklich starten."); });
$("testAll").onclick = () => action(async () => { await request("/api/test", {output: 0, seconds: 30}); notice("Paralleler Test gestartet, maximal 30 Sekunden. Alle acht physischen Ausgänge prüfen."); });
$("apply").onclick = () => action(async () => { const draft = validate(getDraft()); await request("/api/config", draft); await loadConfig(); notice("Konfiguration im RAM angewendet. Dauerhaft speichern erhält sie nach einem Neustart."); });
$("save").onclick = () => action(async () => { await request("/api/save", {}); notice("Konfiguration dauerhaft gespeichert."); });
$("reload").onclick = () => { if (!dirty || window.confirm("Ungespeicherten Formularentwurf verwerfen und Controller-Konfiguration neu laden?")) action(async () => { await loadConfig(); notice("Konfiguration neu geladen."); }); };
$("export").onclick = () => {
  try { const draft = validate(getDraft()); const url = URL.createObjectURL(new Blob([JSON.stringify(draft, null, 2) + "\n"], {type: "application/json"})); const link = document.createElement("a"); link.href = url; link.download = "teensy-octo-config.json"; document.body.append(link); link.click(); link.remove(); setTimeout(() => URL.revokeObjectURL(url), 1000); notice(dirty ? "Formularentwurf als JSON exportiert; am Controller noch nicht angewendet." : "Konfiguration als JSON exportiert."); } catch (error) { notice(error.message, true); }
};
$("import").onclick = () => $("importFile").click();
$("importFile").onchange = async event => {
  const file = event.target.files[0]; if (!file) return;
  try {
    if (file.size > 32768) throw new Error("Konfigurationsdatei ist zu groß (maximal 32 KiB).");
    const candidate = validate(JSON.parse(await file.text()));
    if (dirty && !window.confirm("Aktuellen Formularentwurf durch die importierte Konfiguration ersetzen?")) return;
    buildEditor(candidate); dirty = true; updateDraftSummary(); updateControls(); notice("JSON als Formularentwurf geladen. Zum Übertragen unter Ausgänge anwenden.");
  } catch (error) { notice("Import fehlgeschlagen: " + error.message, true); }
  finally { event.target.value = ""; }
};
$("reboot").onclick = () => {
  const warning = dirty ? "Lokalen Formularentwurf verwerfen und den bereits gestoppten Controller neu starten?" : "Den bereits gestoppten Controller jetzt neu starten?";
  if (!window.confirm(warning)) return;
  action(async () => { await request("/api/reboot", {}); notice("Neustart angefordert. Bei geänderter Netzwerkadresse die neue IP öffnen."); connected = false; packetSample = null; config = null; dirty = false; $("connectionStatus").textContent = "Neustart …"; $("connectionStatus").className = "badge"; }, false);
};
window.addEventListener("beforeunload", event => { if (dirty) { event.preventDefault(); event.returnValue = ""; } });
window.addEventListener("hashchange", () => go(location.hash.slice(1)));
go(location.hash.slice(1)); poll();
