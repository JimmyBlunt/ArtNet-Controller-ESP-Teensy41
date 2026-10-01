const assert = require("assert");
const fs = require("fs");
const path = require("path");
const { chromium } = require("playwright");

const target = process.argv[2] || "http://10.0.0.249/";
const passed = [];
const mark = name => { passed.push(name); console.log("PASS " + name); };

async function waitConnected(page, timeout = 20000) {
  await page.waitForFunction(() => document.querySelector("#connectionStatus")?.textContent === "Verbunden", {}, { timeout });
}

(async () => {
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1440, height: 1100 } });
  const browserErrors = [];
  page.on("pageerror", error => browserErrors.push(error.message));
  try {
    await page.goto(target, { waitUntil: "domcontentloaded", timeout: 20000 });
    await waitConnected(page);
    assert.strictEqual(await page.locator("#connectController").textContent(), "Neu verbinden");
    mark("automatische Verbindung und Reconnect-Beschriftung");

    const original = await page.evaluate(() => fetch("/api/config", { cache:"no-store" }).then(r => r.json()));
    assert.strictEqual(original.outputs.length, 1);
    assert.strictEqual(original.outputs[0].dataPin, 32);

    await page.click('[data-test-output="0"]');
    await page.waitForFunction(() => document.querySelector("#testVisualState")?.textContent.includes("Rot"), {}, { timeout: 5000 });
    assert((await page.locator("#testVisualTarget").textContent()).includes("GPIO 32"));
    assert((await page.locator("#testVisual").getAttribute("class")).includes("running"));
    const framesBefore = await page.evaluate(() => fetch("/api/status", { cache:"no-store" }).then(r => r.json()).then(s => s.outputFrames));
    await page.waitForTimeout(700);
    const framesAfter = await page.evaluate(() => fetch("/api/status", { cache:"no-store" }).then(r => r.json()).then(s => s.outputFrames));
    assert(framesAfter > framesBefore, "LED frame counter did not advance");
    mark("Ausgang testen, Live-Farbe, Laufblock und Frame-Zähler");

    await page.click('[data-action="start"]');
    await page.waitForFunction(() => document.querySelector("#testVisualTarget")?.textContent.includes("Alle aktiven"));
    mark("Einmal für alle aktiven Ausgänge");

    await page.click('[data-action="loop"]');
    await page.waitForFunction(() => document.querySelector("#testVisualDetail")?.textContent.includes("Loop aktiv"));
    mark("Im Loop");

    await page.click("#blackout");
    await page.waitForFunction(() => document.querySelector("#testVisualState")?.textContent.includes("Blackout"));
    mark("Blackout");

    await page.click('[data-action="stop"]');
    await page.waitForFunction(() => document.querySelector("#testPhase")?.textContent.includes("Art-Net freigegeben"));
    mark("Stop und Art-Net-Freigabe");

    await page.click('[data-go="outputs"]');
    assert.strictEqual(await page.locator('[data-view="outputs"]').isVisible(), true);
    mark("Einstellungen-Verknüpfung");

    await page.click("#addOutput");
    assert.strictEqual(await page.locator(".outedit").count(), 2);
    const second = page.locator(".outedit").nth(1);
    await second.locator('[data-k="type"]').selectOption("APA102");
    assert.strictEqual(await second.locator('[data-pin-field="apa"]').isVisible(), true);
    await page.locator('[data-remove-output="1"]').click();
    assert.strictEqual(await page.locator(".outedit").count(), 1);
    mark("Ausgang hinzufügen, LED-Typ umschalten und entfernen");

    await page.click("#autoLayout");
    assert((await page.locator("#configMsg").textContent()).includes("jetzt Anwenden"));
    mark("Auto Layout und verständlicher Folgeschritt");

    await page.click("#applyConfig");
    await page.waitForFunction(() => document.querySelector("#configMsg")?.textContent.includes("Angewendet"));
    await page.click("#saveConfig");
    await page.waitForFunction(() => document.querySelector("#storageState")?.textContent === "Dauerhaft gespeichert");
    await page.click("#reloadConfig");
    await page.waitForFunction(() => document.querySelector("#configMsg")?.textContent.includes("Neu geladen"));
    const afterConfigActions = await page.evaluate(() => fetch("/api/config", { cache:"no-store" }).then(r => r.json()));
    assert.deepStrictEqual(afterConfigActions.outputs, original.outputs);
    mark("Anwenden, dauerhaft speichern und neu laden ohne Konfigurationsänderung");

    await page.click('[data-page="mapping"]');
    assert.strictEqual(await page.locator('[data-view="mapping"]').isVisible(), true);
    await page.fill("#previewHost", "127.0.0.1");
    await page.fill("#previewPort", "6455");
    await page.click("#savePreview");
    await page.waitForFunction(() => document.querySelector("#previewMsg")?.textContent.includes("127.0.0.1:6455"));
    mark("Mapping-Navigation und Preview-Ziel");

    await page.click('[data-page="diagnostics"]');
    assert.strictEqual(await page.locator('[data-view="diagnostics"]').isVisible(), true);
    assert((await page.locator("#diagnostics").textContent()).includes("LED-Ausgaben"));
    mark("Diagnose-Navigation und Live-Daten");

    await page.click('[data-page="system"]');
    assert.strictEqual(await page.locator('[data-view="system"]').isVisible(), true);
    page.once("dialog", dialog => dialog.accept());
    await page.click("#rebootController");
    await page.waitForFunction(() => document.querySelector("#connectionStatus")?.textContent === "Offline", {}, { timeout: 5000 });
    await page.waitForTimeout(8000);
    await page.click("#connectController");
    await waitConnected(page, 20000);
    mark("Controller-Neustart und WLAN-Wiederverbindung");

    await page.click('[data-page="overview"]');
    await page.click("#connectController");
    await waitConnected(page);
    mark("Übersicht-Navigation und manuelles Neu verbinden");

    const finalConfig = await page.evaluate(() => fetch("/api/config", { cache:"no-store" }).then(r => r.json()));
    assert.deepStrictEqual(finalConfig.outputs, original.outputs);
    assert.deepStrictEqual(browserErrors, []);
    const outDir = path.resolve("output/playwright");
    fs.mkdirSync(outDir, { recursive:true });
    await page.screenshot({ path:path.join(outDir, "live-device-button-audit.png"), fullPage:true });
    console.log("AUDIT COMPLETE: " + passed.length + " Funktionsgruppen bestanden");
  } finally {
    await browser.close();
  }
})().catch(error => { console.error(error); process.exitCode = 1; });
