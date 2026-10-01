async page => {
  const host = new URL(page.url()).hostname;
  const original = await page.evaluate(() => fetch('/api/config').then(r => r.json()));
  const downloadPromise = page.waitForEvent('download');
  await page.getByRole('button', {name: 'JSON-Backup herunterladen', exact: true}).click();
  const download = await downloadPromise;
  await download.saveAs(`C:/Users/jimmy/Documents/ESP-Backups/2026-10-02-before-artnet-rx-task/${host}-browser-export.json`);
  page.once('dialog', dialog => dialog.accept());
  await page.getByRole('button', {name: 'Controller neu starten', exact: true}).click();
  await page.waitForFunction(() => document.querySelector('#connectionStatus').textContent === 'Offline');
  await page.waitForTimeout(9000);
  await page.getByRole('button', {name: 'Verbinden', exact: true}).click();
  await page.waitForFunction(() => document.querySelector('#connectionStatus').textContent === 'Verbunden', {}, {timeout: 30000});
  const restored = await page.evaluate(() => fetch('/api/config').then(r => r.json()));
  const storage = await page.evaluate(() => fetch('/api/storage').then(r => r.json()));
  if (JSON.stringify(original) !== JSON.stringify(restored) || !storage.saved || !storage.matches)
    throw new Error('Config/NVS did not survive browser reboot');
  await page.screenshot({path: `output/playwright/${host}-after-reboot.png`, fullPage: true});
  await page.setViewportSize({width: 390, height: 844});
  const layout = await page.evaluate(() => ({width: innerWidth, content: document.documentElement.scrollWidth}));
  await page.screenshot({path: `output/playwright/${host}-mobile.png`, fullPage: true});
  await page.setViewportSize({width: 1440, height: 1000});
  return {host, exportFilename: download.suggestedFilename(), reboot: true, configUnchanged: true,
          storage, mobileLayout: layout, mobileNoOverflow: layout.content <= layout.width + 1};
}
