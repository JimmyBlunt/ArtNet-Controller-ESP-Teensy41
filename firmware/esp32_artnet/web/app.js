"use strict";
const $ = id => document.getElementById(id);
const embedded = window.CONTROLLER_EMBEDDED === true;
const state = { config: null, status: null, test: null, storage: null, online: false, dirty: false,
  base: "", generation: 0, timer: null, polling: false, busy: false };
let storedHost = "";
try { storedHost = localStorage.getItem("schrankControllerHost") || ""; } catch (_) {}
$("controllerHost").value = embedded ? location.host : storedHost || "10.0.0.251";
$("controllerHost").readOnly = embedded;
$("footerMode").textContent = embedded ? "Direkt auf dem Controller" : "Lokales Bedienpult";
const fmt = value => typeof value === "number" && Number.isFinite(value) ? value.toLocaleString("de-DE") : "—";
const esc = value => String(value ?? "").replace(/[&<>"']/g, c => ({ "&":"&amp;", "<":"&lt;", ">":"&gt;", '"':"&quot;", "'":"&#39;" }[c]));
const FLEX_PROFILE = "flex8-ws2812-apa102";
const EXTENSION_PROFILE = "esp32-wroom-flex-8ws-2apa";
const FLEX_WS_PINS = [32,33,25,26,27,14,13,16,17,18,19,21,22,23];
const FLEX_APA_PAIRS = [[18,19],[23,18],[13,14],[25,26],[32,33],[16,17],[21,22]];
const EXTENSION_WS_PINS = [25,26,27,14,19,18,5,17];
const EXTENSION_APA_PAIRS = [[18,5],[26,27]];
const isExtensionBoard = config => config?.hardwareProfile === EXTENSION_PROFILE;
const isFlexible = config => [FLEX_PROFILE,EXTENSION_PROFILE].includes(config?.hardwareProfile);
const wsPinsFor = config => isExtensionBoard(config) ? EXTENSION_WS_PINS : FLEX_WS_PINS;
const apaPairsFor = config => isExtensionBoard(config) ? EXTENSION_APA_PAIRS : FLEX_APA_PAIRS;
const maxOutputsFor = config => 8;
const totalLimitFor = config => isFlexible(config) ? 8192 : 2400;
const pinLabel = (config,pin) => isExtensionBoard(config) ? "G" + pin + " (GPIO " + pin + ")" : "GPIO " + pin;
const universesForPixels = pixelCount => Math.ceil(pixelCount / 170);
function mappedUniverseCount(outputs) {
  const universes = new Set();
  outputs.filter(output=>output.enabled).forEach(output => {
    for (let offset=0; offset<universesForPixels(output.pixelCount); ++offset) universes.add(output.startUniverse+offset);
  });
  return universes.size;
}
const options = (values, selected, label = value => "GPIO " + value) => values.map(value =>
  '<option value="' + esc(value) + '" ' + (String(value) === String(selected) ? "selected" : "") + '>' + esc(label(value)) + '</option>').join("");
function log(message) {
  $("logs").textContent = ("[" + new Date().toLocaleTimeString("de-DE") + "] " + message + "\n" + $("logs").textContent).slice(0, 7000);
}
function notice(message, error = false) {
  $("notice").textContent = message;
  $("notice").className = error ? "error" : "";
}
function lockControls() {
  $("controllerHost").disabled = state.busy;
  $("connectController").disabled = state.busy;
  document.querySelectorAll("[data-live]").forEach(el => { el.disabled = !state.online || state.busy; });
  document.querySelectorAll("[data-test-output]").forEach(el => {
    el.disabled = !state.online || state.busy || state.dirty || !state.config?.outputs.find(o => o.id === Number(el.dataset.testOutput))?.enabled;
  });
  document.querySelectorAll('[data-action="start"],[data-action="loop"]').forEach(el => {
    el.disabled = !state.online || state.busy || state.dirty;
  });
  document.querySelectorAll("[data-edit]").forEach(el => { el.disabled = !state.online || state.busy; });
  $("saveConfig").disabled = !state.online || state.busy || state.dirty || !state.config;
  $("addOutput").disabled = !state.online || state.busy || !isFlexible(state.config) ||
    state.config.outputs.length >= maxOutputsFor(state.config);
}
function setOnline(online) {
  state.online = online;
  $("connectController").textContent = online ? "Neu verbinden" : "Verbinden";
  $("connectionStatus").textContent = online ? "Verbunden" : "Offline";
  $("connectionStatus").className = "badge" + (online ? " online" : "");
  $("railState").textContent = online ? "Controller verbunden" : "Nicht verbunden";
  $("railState").className = "signal" + (online ? " online" : "");
  if (!online) {
    $("signalState").textContent = "Signal unbekannt";
    $("signalState").className = "badge";
    $("testPhase").textContent = "Teststatus unbekannt";
    $("testPhase").className = "badge";
    $("lastSeen").textContent = "Keine aktuelle Verbindung · Werte ggf. veraltet";
    $("testVisual").className = "test-visual";
    $("testVisualState").textContent = "Keine aktuellen Testdaten";
    $("testVisualTarget").textContent = "Controller nicht erreichbar";
    $("testVisualDetail").textContent = "Nach der Wiederverbindung wird der laufende Zustand erneut geladen.";
    document.querySelectorAll(".output-card .badge").forEach(el => { el.textContent = "Status unbekannt"; el.className = "badge"; });
  }
  lockControls();
}
function baseUrl() {
  if (embedded) return location.origin;
  const raw = $("controllerHost").value.trim();
  const url = new URL(/^https?:\/\//i.test(raw) ? raw : "http://" + raw);
  if (!["http:", "https:"].includes(url.protocol) || url.username || url.password) throw new Error("Ungültige Controller-Adresse");
  return url.origin;
}
async function api(path, options = {}) {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 4500);
  try {
    const response = await fetch(state.base + path, { cache: "no-store", ...options, signal: controller.signal });
    let result;
    try { result = await response.json(); } catch (_) { throw new Error("Controller liefert keine gültige JSON-Antwort"); }
    if (!response.ok) throw new Error(result.error || "HTTP " + response.status);
    return result;
  } finally { clearTimeout(timeout); }
}
const post = (path, body) => api(path, { method:"POST", headers:{"Content-Type":"application/json"}, body:JSON.stringify(body || {}) });
function showPage(page) {
  document.querySelectorAll("[data-view]").forEach(el => { el.hidden = el.dataset.view !== page; });
  document.querySelectorAll("[data-page]").forEach(el => {
    const active = el.dataset.page === page;
    el.classList.toggle("active", active);
    if (active) el.setAttribute("aria-current", "page"); else el.removeAttribute("aria-current");
  });
}
function markDirty() {
  state.dirty = true;
  $("configMsg").textContent = "Änderungen noch nicht angewendet.";
  $("configMsg").className = "";
  $("testDeckTitle").textContent = "Entwurf zuerst anwenden";
  document.querySelectorAll(".output-card .badge").forEach(el => { el.textContent = "Entwurf · noch nicht aktiv"; });
  notice("Neue oder geänderte Ausgänge sind noch nicht im ESP aktiv. Zuerst Anwenden drücken.");
  lockControls();
}
function storageLabel() {
  $("storageState").textContent = !state.storage ? "Speicherstatus unbekannt" :
    state.storage.matches ? "Dauerhaft gespeichert" :
    state.storage.saved ? "Laufende Änderungen nicht gespeichert" : "Nur laufende Konfiguration";
  $("storageState").className = "badge" + (state.storage?.matches ? " online" : "");
}
function cardStatus(o) {
  if (!state.online) return "Status unbekannt";
  if (!o.enabled) return "Deaktiviert";
  if (state.test?.blackout) return "Blackout gehalten";
  if (state.test?.active && (state.test.outputId == null || state.test.outputId < 0 || state.test.outputId === o.id)) return "Test aktiv";
  return state.status?.packetsPerSecond > 0 ? "Art-Net empfangen" : "Bereit";
}
function testPhaseInfo(test) {
  const phase = test?.phase || "stopped";
  if (test?.blackout || phase.includes("blackout")) return { label:"Schwarzphase", color:"#11141b", running:false };
  if (phase.includes("red")) return { label:"Rot", color:"#ff4057", running:true };
  if (phase.includes("green")) return { label:"Grün", color:"#36d978", running:true };
  if (phase.includes("blue")) return { label:"Blau", color:"#4c83ff", running:true };
  return { label:test?.active ? "Test startet" : "Keine Testdaten", color:"#80a9ff", running:false };
}
function renderTestVisual(status, test) {
  const info = testPhaseInfo(test);
  const visual = $("testVisual");
  const active = Boolean(test?.active);
  visual.className = "test-visual" + (active && info.running ? " running" : "") +
    (test?.blackout || (active && !info.running) ? " blackout" : "");
  visual.style.setProperty("--test-color",info.color);
  const output = test?.outputId >= 0 ? state.config?.outputs.find(item=>item.id===test.outputId) : null;
  $("testVisualState").textContent = test?.blackout ? "Blackout wird gehalten" :
    active ? "ESP sendet: " + info.label + (info.running ? " · Laufblock" : "") : "Keine Testdaten";
  $("testVisualTarget").textContent = output ? "Output " + output.id + " · " + pinLabel(state.config,output.dataPin) :
    active ? "Alle aktiven Ausgänge" : "Kein Ausgang aktiv";
  $("testVisualDetail").textContent = test?.blackout ? "Alle LED-Ausgänge bleiben schwarz · Art-Net ist gesperrt." : active ?
    "Testframes werden ausgegeben · LED-Ausgaben gesamt: " + fmt(status?.outputFrames) +
      (test.loop ? " · Loop aktiv" : " · einmaliger 20-s-Test") :
    "Der ESP gibt derzeit kein Testmuster aus · Art-Net ist freigegeben.";
}
function renderCards() {
  if (!state.config) return;
  $("outputCards").innerHTML = state.config.outputs.map(o => '<article class="panel output-card ' + (o.type === "APA102" ? "apa" : "ws") + '">' +
    '<div class="card-top"><div><p class="eyebrow">OUTPUT ' + esc(o.id) + '</p><h3>' + esc(o.type) + '</h3></div><span class="badge">' + esc(cardStatus(o)) + '</span></div>' +
    '<div class="led-count">' + fmt(o.pixelCount) + ' <small>LEDs</small></div><div class="muted">Data ' + esc(pinLabel(state.config,o.dataPin)) +
    (o.clockPin >= 0 ? ' · Clock ' + esc(pinLabel(state.config,o.clockPin)) : '') + ' · Universe ' + esc(o.startUniverse) +
    '–' + esc(o.startUniverse + universesForPixels(o.pixelCount) - 1) + ' · ' + esc(o.colorOrder) + '</div>' +
    '<div class="pixel-track" aria-hidden="true">' + "<i></i>".repeat(28) + '</div>' +
    '<div class="muted">Pixel ' + fmt(o.startPixel) + '–' + fmt(o.startPixel + o.pixelCount - 1) + ' · ' + (o.enabled ? "Aktiviert" : "Deaktiviert") + '</div>' +
    '<div class="actions"><button data-test-output="' + esc(o.id) + '" data-live ' + (!o.enabled || !state.online ? "disabled" : "") + '>Ausgang testen</button><button data-go="outputs" class="quiet">Einstellungen ↗</button></div></article>').join("");
  $("patchOverview").innerHTML = state.config.outputs.map(o => {
    const start = o.startUniverse;
    const end = o.startUniverse + universesForPixels(o.pixelCount) - 1;
    return '<div class="patch-row"><strong>' + esc(o.type) + ' · ' + fmt(o.pixelCount) + ' LEDs</strong><span class="muted">' +
      'Universe ' + start + '–' + end + ' · Startkanal 1 · ' + esc(pinLabel(state.config,o.dataPin)) +
      (o.clockPin >= 0 ? " / " + esc(pinLabel(state.config,o.clockPin)) : "") + '</span></div>';
  }).join("");
}
function renderConfig(config) {
  state.config = config;
  state.dirty = false;
  $("cfgPixels").value = config.pixelCount;
  $("cfgUniverse").value = config.startUniverse;
  $("cfgFps").value = config.targetFps;
  $("cfgUniverseCount").value = config.universeCount;
  $("hardwareNote").textContent = isExtensionBoard(config) ?
    "ESP32-WROOM: WS2812 wählbar auf G25/G26/G27/G14 und G19/G18/G5/G17. APA102: Data G18 / Clock G5 oder Data G26 / Clock G27. Bis 1.200 LEDs je Ausgang. Bei 1.122 WS2812 sind wegen ca. 33,7 ms Signaldauer praktisch höchstens etwa 29 FPS möglich; 25 FPS gibt Reserve. Die APA-Pins können nicht gleichzeitig als WS2812 belegt werden. G5 beim Einschalten nicht extern auf LOW ziehen; G34/G35 sind reine Eingänge." : "";
  const activeOutputs=config.outputs.filter(output=>output.enabled).length;
  $("testDeckTitle").textContent=activeOutputs === 1 ? "Aktiven Ausgang testen" : "Alle " + activeOutputs + " Ausgänge testen";
  const flexible = isFlexible(config);
  const wsPins = wsPinsFor(config);
  const apaPairs = apaPairsFor(config);
  $("outputEditor").innerHTML = config.outputs.map((o, i) => {
    const pairValue = apaPairs.some(pair => pair[0] === o.dataPin && pair[1] === o.clockPin) ? o.dataPin + "/" + o.clockPin : apaPairs[0].join("/");
    const hardwareFields = flexible ?
      '<label>LED-Typ<select data-k="type" data-edit>' + options(["WS2812B","APA102"],o.type,value=>value) + '</select></label>' +
      '<label data-pin-field="ws" ' + (o.type === "WS2812B" ? "" : "hidden") + '>WS2812 Data<select data-k="dataPin" data-edit>' + options(wsPins,o.dataPin,pin=>pinLabel(config,pin)) + '</select></label>' +
      '<label data-pin-field="apa" ' + (o.type === "APA102" ? "" : "hidden") + '>APA102 Data / Clock<select data-k="pinPair" data-edit>' + options(apaPairs.map(pair=>pair.join("/")),pairValue,value=>value.split("/").map(pin=>pinLabel(config,Number(pin))).join(" / ")) + '</select></label>' : "";
    return '<div class="outedit" data-i="' + i + '"><div class="card-top"><h3>' + esc(o.type) + ' · Output ' + esc(o.id) + '</h3>' +
    (flexible ? '<button type="button" class="danger" data-remove-output="' + i + '">Entfernen</button>' : '') + '</div><p class="muted">Data ' + esc(pinLabel(config,o.dataPin)) +
    (o.clockPin >= 0 ? " · Clock " + esc(pinLabel(config,o.clockPin)) + " · 4 MHz" : "") + ' · ' + esc(o.colorOrder) + '</p>' +
    '<div class="config-grid">' + hardwareFields + '<label>LED-Anzahl<input type="number" data-k="pixelCount" data-edit min="1" max="' + (flexible ? 1200 : (o.type === "WS2812B" ? 1024 : 1200)) + '" value="' + esc(o.pixelCount) + '"></label>' +
    '<label>Art-Net Start-Universe<input type="number" data-k="startUniverse" data-edit min="0" max="32767" value="' + esc(o.startUniverse) + '"></label>' +
    '<label>Farbreihenfolge<select data-k="colorOrder" data-edit>' + options(["RGB","RBG","GRB","GBR","BRG","BGR"],o.colorOrder,value=>value) + '</select></label>' +
    '<label>Startpixel<input data-k="startPixel" readonly value="' + esc(o.startPixel) + '"></label>' +
    '<div><label class="toggle"><input data-k="enabled" data-edit type="checkbox" ' + (o.enabled ? "checked" : "") + '>Aktiviert</label><label class="toggle"><input data-k="reverse" data-edit type="checkbox" ' + (o.reverse ? "checked" : "") + '>Pixelrichtung umkehren</label></div></div></div>';
  }).join("");
  renderCards();
  lockControls();
}
function normalizeLayout(outputs) {
  let cursor = 0;
  return outputs.map(o => {
    const next = { ...o, startPixel: cursor };
    if (o.enabled) cursor += o.pixelCount;
    return next;
  });
}
function readConfig() {
  if (!state.config) throw new Error("Zuerst verbinden");
  const startUniverse = Number($("cfgUniverse").value), targetFps = Number($("cfgFps").value);
  if (!Number.isInteger(startUniverse) || startUniverse < 0 || startUniverse > 32767 ||
      !Number.isInteger(targetFps) || targetFps < 1 || targetFps > 60) throw new Error("Universe oder Ziel-FPS außerhalb des gültigen Bereichs");
  let outputs = state.config.outputs.map((o, i) => {
    const box = document.querySelector('.outedit[data-i="' + i + '"]');
    const pixelCount = Number(box.querySelector('[data-k="pixelCount"]').value);
    const flexible = isFlexible(state.config);
    const type = flexible ? box.querySelector('[data-k="type"]').value : o.type;
    const max = flexible ? 1200 : (type === "WS2812B" ? 1024 : 1200);
    if (!Number.isInteger(pixelCount) || pixelCount < 1 || pixelCount > max) throw new Error(o.type + ": 1–" + max + " LEDs");
    let dataPin=o.dataPin, clockPin=o.clockPin, colorOrder=box.querySelector('[data-k="colorOrder"]').value, spiHz=o.spiHz;
    if (flexible && type === "WS2812B") {
      dataPin=Number(box.querySelector('[data-k="dataPin"]').value); clockPin=-1; spiHz=0;
    } else if (flexible) {
      [dataPin,clockPin]=box.querySelector('[data-k="pinPair"]').value.split("/").map(Number); spiHz=4000000;
    }
    const outputStartUniverse = Number(box.querySelector('[data-k="startUniverse"]').value);
    if (!Number.isInteger(outputStartUniverse) || outputStartUniverse < 0 ||
        outputStartUniverse + universesForPixels(pixelCount) > 32768) {
      throw new Error("Output " + o.id + ": ungültiger Universe-Bereich");
    }
    return { ...o, type, dataPin, clockPin, colorOrder, spiHz, pixelCount, startUniverse:outputStartUniverse,
      enabled:box.querySelector('[data-k="enabled"]').checked,
      reverse:box.querySelector('[data-k="reverse"]').checked, targetFps };
  });
  const reserved = new Set();
  for (const output of outputs) {
    for (const pin of [output.dataPin,output.clockPin].filter(pin=>pin>=0)) {
      if (reserved.has(pin)) throw new Error("GPIO " + pin + " ist bereits durch einen anderen Ausgang reserviert");
      reserved.add(pin);
    }
  }
  outputs = normalizeLayout(outputs);
  const pixelCount = outputs.reduce((n, o) => n + (o.enabled ? o.pixelCount : 0), 0);
  const universeCount = mappedUniverseCount(outputs);
  const totalLimit = totalLimitFor(state.config);
  if (!pixelCount || pixelCount > totalLimit) throw new Error("Mindestens ein aktiver Ausgang und maximal " + totalLimit + " Pixel");
  return { ...state.config, pixelCount, startUniverse, targetFps, universeCount, outputs };
}

function backupFileName() {
  let controller = "controller";
  try { controller = new URL(state.base).hostname.replace(/[^a-z0-9.-]/gi,"-"); } catch (_) {}
  return "artnet-led-backup-" + controller + "-" + new Date().toISOString().replace(/[:.]/g,"-") + ".json";
}

function exportConfigBackup() {
  if (!state.config) throw new Error("Zuerst mit dem Controller verbinden");
  const backup = {
    schema:"artnet-led-controller-backup",
    version:1,
    exportedAt:new Date().toISOString(),
    controller:state.base || "embedded",
    config:structuredClone(state.config)
  };
  const blob = new Blob([JSON.stringify(backup,null,2) + "\n"],{type:"application/json"});
  const url = URL.createObjectURL(blob), link = document.createElement("a");
  link.href=url; link.download=backupFileName(); document.body.appendChild(link); link.click(); link.remove();
  setTimeout(()=>URL.revokeObjectURL(url),0);
  $("backupMsg").textContent = "JSON-Backup erstellt · enthält die aktuell vom Controller geladene Konfiguration.";
  log("Konfigurations-Backup heruntergeladen");
}

function importedConfig(documentValue) {
  if (!documentValue || typeof documentValue !== "object" || Array.isArray(documentValue)) throw new Error("Backup ist kein gültiges JSON-Objekt");
  if (documentValue.schema && documentValue.schema !== "artnet-led-controller-backup") throw new Error("Unbekanntes Backup-Format");
  if (documentValue.version != null && documentValue.version !== 1) throw new Error("Diese Backup-Version wird nicht unterstützt");
  const config = structuredClone(documentValue.config || documentValue);
  if (!config || !Array.isArray(config.outputs) || !config.outputs.length) throw new Error("Backup enthält keine LED-Ausgänge");
  if (config.outputs.length > maxOutputsFor(state.config)) throw new Error("Backup enthält zu viele LED-Ausgänge");
  const wsPins=wsPinsFor(state.config), apaPairs=apaPairsFor(state.config), reserved=new Set();
  for (const output of config.outputs) {
    if (!Number.isInteger(output.id) || !["WS2812B","APA102"].includes(output.type)) throw new Error("Backup enthält einen ungültigen Ausgang");
    if (output.type === "WS2812B" && (!wsPins.includes(output.dataPin) || output.clockPin !== -1))
      throw new Error("WS2812-Pin GPIO " + output.dataPin + " ist mit diesem ESP-Profil nicht kompatibel");
    if (output.type === "APA102" && !apaPairs.some(pair=>pair[0]===output.dataPin&&pair[1]===output.clockPin))
      throw new Error("APA102-Paar GPIO " + output.dataPin + "/" + output.clockPin + " ist mit diesem ESP-Profil nicht kompatibel");
    for (const pin of [output.dataPin,output.clockPin].filter(pin=>pin>=0)) {
      if (reserved.has(pin)) throw new Error("Backup belegt GPIO " + pin + " mehrfach");
      reserved.add(pin);
    }
  }
  // Das Zielprofil kommt immer vom angeschlossenen ESP, niemals aus der Datei.
  config.hardwareProfile=state.config.hardwareProfile;
  return config;
}

async function restoreConfigBackup(file) {
  if (!file) return;
  let parsed;
  try { parsed=JSON.parse(await file.text()); } catch (_) { throw new Error("Die gewählte Datei enthält kein gültiges JSON"); }
  const candidate=importedConfig(parsed);
  if (!confirm("Backup jetzt anwenden und dauerhaft auf diesem Controller speichern?")) return;
  const applied=await post("/api/config",candidate);
  const storage=await post("/api/config/save");
  state.storage=storage; renderConfig(applied); storageLabel();
  $("configMsg").textContent="Backup angewendet und dauerhaft gespeichert.";
  $("backupMsg").textContent="Wiederhergestellt: " + file.name + " · bleibt nach Neustart erhalten.";
  log("Konfigurations-Backup wiederhergestellt: " + file.name);
}

function showDraft(config, message) {
  renderConfig(config);
  markDirty();
  $("configMsg").textContent = message;
}

function addOutput() {
  const config = readConfig();
  const maxOutputs = maxOutputsFor(config);
  if (!isFlexible(config) || config.outputs.length >= maxOutputs) return;
  const usedPins = new Set(config.outputs.flatMap(o => [o.dataPin,o.clockPin]).filter(pin=>pin>=0));
  const dataPin = wsPinsFor(config).find(pin => !usedPins.has(pin));
  const id = Array.from({length:maxOutputs},(_,i)=>i).find(value => !config.outputs.some(o=>o.id===value));
  if (dataPin == null || id == null) throw new Error("Kein freier Ausgang verfügbar");
  const nextUniverse = config.outputs.reduce((next,o)=>Math.max(next,o.startUniverse+universesForPixels(o.pixelCount)),config.startUniverse);
  const outputs = normalizeLayout([...config.outputs,
    {id,type:"WS2812B",enabled:true,startPixel:0,pixelCount:256,dataPin,clockPin:-1,colorOrder:"GRB",reverse:false,spiHz:0,startUniverse:nextUniverse,targetFps:config.targetFps}]);
  const pixelCount = outputs.reduce((sum,o)=>sum+(o.enabled?o.pixelCount:0),0);
  showDraft({...config,outputs,pixelCount,universeCount:mappedUniverseCount(outputs)},"Ausgang hinzugefügt · Start-Universe kann jetzt eingestellt werden.");
}

function removeOutput(index) {
  const config = readConfig();
  if (!isFlexible(config) || config.outputs.length <= 1) throw new Error("Mindestens ein Ausgang muss vorhanden bleiben");
  const outputs = normalizeLayout(config.outputs.filter((_,i)=>i!==index));
  const pixelCount = outputs.reduce((sum,o)=>sum+(o.enabled?o.pixelCount:0),0);
  showDraft({...config,outputs,pixelCount,universeCount:mappedUniverseCount(outputs)},"Ausgang entfernt · noch nicht angewendet.");
}
function renderStatus(status, test) {
  state.status = status; state.test = test;
  $("statPixels").textContent = fmt(status.pixelCount);
  $("statFps").textContent = fmt(status.fps);
  $("statPackets").textContent = fmt(status.packetsPerSecond);
  $("statIncomplete").textContent = fmt(status.framesIncomplete);
  $("lastSeen").textContent = "Aktualisiert " + new Date().toLocaleTimeString("de-DE");
  const receiving = status.packetsPerSecond > 0;
  $("signalState").textContent = test.blackout ? "Blackout gehalten" : test.active ? "Testmodus" : receiving ? "Art-Net empfangen" : "Warte auf Art-Net";
  $("signalState").className = "badge" + (receiving && !test.active && !test.blackout ? " online" : "");
  $("testPhase").textContent = test.blackout ? "Blackout · Art-Net gesperrt" : test.active ? test.phase + (test.loop ? " · Loop" : "") : "Test aus · Art-Net freigegeben";
  const loopButton = document.querySelector('[data-action="loop"]');
  const onceButton = document.querySelector('[data-action="start"]');
  loopButton.textContent = test.active && test.loop ? "Loop läuft" : "Im Loop";
  loopButton.setAttribute("aria-pressed",test.active && test.loop ? "true" : "false");
  onceButton.textContent = test.active && !test.loop ? "Einmal läuft" : "Einmal";
  onceButton.setAttribute("aria-pressed",test.active && !test.loop ? "true" : "false");
  renderTestVisual(status,test);
  const items = [
    ["Vollständige Sammlungen", status.framesComplete], ["Verworfene Pakete", status.droppedPackets],
    ["Timeouts", status.timeouts], ["Sequenzfehler", status.sequenceErrors],
    ["Ausgabetiming (µs)", status.outputTimeUs], ["Heap frei (Bytes)", status.heapFree],
    ["Minimaler Heap (Bytes)", status.heapMinFree], ["LED-Ausgaben", status.outputFrames],
    ["Universe-Anzahl", status.universeCount]
  ];
  $("diagnostics").innerHTML = items.map(([name,value]) => "<div><span>" + name + "</span><strong>" + fmt(value) + "</strong></div>").join("");
  $("raw").textContent = JSON.stringify(status, null, 2);
  // Polling must not replace focused buttons, fields or unsaved edits.
  document.querySelectorAll(".output-card .badge").forEach((el,i) => { el.textContent = cardStatus(state.config.outputs[i]); });
}
async function refresh() {
  if (state.polling || state.busy || !state.base) return;
  state.polling = true;
  const generation = state.generation;
  try {
    const [status,test] = await Promise.all([api("/api/status"), api("/api/test-pattern")]);
    if (generation !== state.generation) return;
    const recovered = !state.online;
    setOnline(true);
    renderStatus(status,test);
    if (recovered) notice("");
  } catch (error) {
    if (generation === state.generation) {
      setOnline(false);
      notice("Controller nicht erreichbar. Verbindung wird erneut geprüft.", true);
    }
  } finally {
    state.polling = false;
    if (generation === state.generation) state.timer = setTimeout(refresh, state.test?.active ? 500 : 1500);
  }
}
async function connect() {
  if (state.busy) return;
  clearTimeout(state.timer);
  const generation = ++state.generation;
  state.busy = true;
  state.config = null; state.storage = null;
  setOnline(false);
  $("connectionStatus").textContent = "Verbinde…";
  $("connectController").textContent = "Verbindet…";
  $("outputCards").innerHTML = '<div class="empty panel">Controllerdaten werden geladen…</div>';
  $("outputEditor").replaceChildren();
  try {
    state.base = baseUrl();
    const [config,status,test,storage] = await Promise.all([api("/api/config"),api("/api/status"),api("/api/test-pattern"),api("/api/storage")]);
    if (generation !== state.generation) return;
    if (!Array.isArray(config.outputs)) throw new Error("Ungültige Controller-Konfiguration");
    state.storage = storage;
    setOnline(true);
    renderConfig(config);
    renderStatus(status,test);
    storageLabel();
    $("configMsg").textContent = "Konfiguration vom Controller geladen.";
    notice("");
    try { localStorage.setItem("schrankControllerHost", $("controllerHost").value.trim()); } catch (_) {}
    log("Verbunden: " + state.base);
    state.timer = setTimeout(refresh,1500);
  } catch (error) {
    setOnline(false);
    $("outputCards").innerHTML = '<div class="empty panel">Keine Controllerdaten. Adresse und Netzwerkverbindung prüfen.</div>';
    notice(error.message, true); log("Verbindung fehlgeschlagen: " + error.message);
  } finally { state.busy = false; lockControls(); }
}
async function action(task) {
  if (state.busy || !state.online) return;
  state.busy = true; clearTimeout(state.timer); lockControls();
  try { await task(); notice(""); }
  catch (error) { notice(error.message, true); log("Fehler: " + error.message); }
  finally { state.busy = false; lockControls(); clearTimeout(state.timer); state.timer = setTimeout(refresh,200); }
}
async function testAction(command, outputId = -1) {
  await action(async () => {
    const result = await post("/api/test-pattern", { action:command, outputId });
    state.test = result;
    if (state.status) renderStatus(state.status,result);
    log("Test: " + command + (outputId >= 0 ? " · Output " + outputId : ""));
  });
  if (command === "start" || command === "loop") {
    const output = outputId >= 0 ? state.config?.outputs.find(item=>item.id===outputId) : null;
    notice((command === "loop" ? "Test läuft im Loop" : "Test läuft einmal") +
      (output ? " · Output " + output.id + " · GPIO " + output.dataPin : " · alle aktiven Ausgänge"));
  } else if (command === "stop") notice("Test beendet · Art-Net wieder freigegeben");
  else if (command === "blackout") notice("Blackout aktiv · LED-Ausgänge bleiben schwarz");
}
document.addEventListener("click", event => {
  const b = event.target.closest("button");
  if (!b || b.disabled) return;
  if (b.dataset.removeOutput !== undefined) {
    try { removeOutput(Number(b.dataset.removeOutput)); } catch (error) { notice(error.message,true); }
    return;
  }
  if (b.dataset.page) showPage(b.dataset.page);
  if (b.dataset.go) showPage(b.dataset.go);
  if (b.dataset.action) testAction(b.dataset.action);
  if (b.dataset.testOutput !== undefined) testAction("start",Number(b.dataset.testOutput));
});
$("blackout").onclick = () => testAction("blackout");
$("addOutput").onclick = () => { try { addOutput(); } catch (error) { notice(error.message,true); } };
$("connectController").onclick = connect;
$("controllerHost").onkeydown = e => { if (e.key === "Enter") connect(); };
$("controllerHost").oninput = () => { clearTimeout(state.timer); ++state.generation; state.base = ""; setOnline(false); };
$("outputEditor").addEventListener("input",markDirty);
$("outputEditor").addEventListener("change",event => {
  if (event.target.dataset.k !== "type") return;
  const box=event.target.closest(".outedit"), apa=event.target.value === "APA102";
  box.querySelector('[data-pin-field="ws"]').hidden=apa;
  box.querySelector('[data-pin-field="apa"]').hidden=!apa;
  box.querySelector("h3").textContent=event.target.value + " · Output " + state.config.outputs[Number(box.dataset.i)].id;
});
$("cfgUniverse").oninput = markDirty;
$("cfgFps").oninput = markDirty;
$("autoLayout").onclick = () => {
  try {
    const config = readConfig();
    let nextUniverse = config.startUniverse;
    const outputs = config.outputs.map(output => {
      const arranged = {...output,startUniverse:nextUniverse};
      if (output.enabled) nextUniverse += universesForPixels(output.pixelCount);
      return arranged;
    });
    showDraft({...config,outputs,universeCount:mappedUniverseCount(outputs)},
      "Universe-Bereiche automatisch angeordnet · jetzt Anwenden drücken, danach bei Bedarf Dauerhaft speichern.");
  }
  catch (error) { notice(error.message,true); }
};
$("applyConfig").onclick = () => action(async () => {
  const result = await post("/api/config",readConfig());
  renderConfig(result);
  state.storage = await api("/api/storage"); storageLabel();
  $("configMsg").textContent = "Angewendet: " + result.pixelCount + " Pixel · noch nicht dauerhaft gespeichert.";
  log("Konfiguration angewendet");
});
$("saveConfig").onclick = () => action(async () => {
  if (state.dirty) throw new Error("Änderungen zuerst anwenden");
  state.storage = await post("/api/config/save"); storageLabel();
  $("configMsg").textContent = "Dauerhaft gespeichert · bleibt nach Neustart erhalten.";
  log("Konfiguration dauerhaft gespeichert");
});
$("exportConfig").onclick = () => { try { exportConfigBackup(); } catch (error) { notice(error.message,true); } };
$("importConfig").onclick = () => $("importConfigFile").click();
$("importConfigFile").onchange = event => {
  const file=event.target.files?.[0]; event.target.value="";
  if (file) action(()=>restoreConfigBackup(file));
};
$("reloadConfig").onclick = () => action(async () => {
  const [config,storage] = await Promise.all([api("/api/config"),api("/api/storage")]);
  state.storage = storage; renderConfig(config); storageLabel();
  $("configMsg").textContent = "Neu geladen: " + config.pixelCount + " Pixel";
});
$("savePreview").onclick = () => action(async () => {
  const host = $("previewHost").value.trim(), port = Number($("previewPort").value);
  if (!host || !Number.isInteger(port) || port < 1 || port > 65535) throw new Error("Empfänger und gültigen UDP-Port eingeben");
  const result = await post("/api/preview/target",{host,port});
  $("previewMsg").textContent = "Preview Ziel: " + result.host + ":" + result.port;
  log("Processing-Preview verbunden");
});
$("rebootController").onclick = () => {
  if (!confirm("Controller neu starten? Nicht dauerhaft gespeicherte Änderungen gehen verloren.")) return;
  action(async () => { await post("/api/reboot"); ++state.generation; state.base = ""; setOnline(false); log("Neustart angefordert · danach erneut verbinden"); });
};
$("firmwareUpdate").onclick = () => {
  if (state.online) window.open(state.base + "/update", "_blank", "noopener,noreferrer");
};
showPage("overview");
if (embedded) connect();
