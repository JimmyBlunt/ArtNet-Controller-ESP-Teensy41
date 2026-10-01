const fs = require("fs");
const path = require("path");
const { chromium } = require("playwright");

const outputDir = path.resolve("docs/assets/screenshots");
const espUrl = process.env.ESP_WEB_URL || "http://10.0.0.251/";
const localWebUrl = `file://${path.resolve("web/index.html")}`;

async function screenshot(page, url, fileName, selector) {
  await page.goto(url, { waitUntil: "domcontentloaded", timeout: 15000 });
  if (selector) await page.waitForSelector(selector, { timeout: 10000 });
  await page.screenshot({ path: path.join(outputDir, fileName), fullPage: true });
}

(async () => {
  fs.mkdirSync(outputDir, { recursive: true });
  const browser = await chromium.launch({ headless: true });

  const espPage = await browser.newPage({ viewport: { width: 1440, height: 1100 } });
  await screenshot(espPage, espUrl, "esp-web-menu-runtime-config.png", "#applyConfig");

  const localPage = await browser.newPage({ viewport: { width: 1440, height: 1000 } });
  await screenshot(localPage, localWebUrl, "desktop-web-ui-overview.png", "body");

  await browser.close();
  console.log(`Screenshots written to ${outputDir}`);
})().catch(error => {
  console.error(error);
  process.exit(1);
});
