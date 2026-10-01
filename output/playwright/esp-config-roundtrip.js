async page => {
  const host = new URL(page.url()).hostname;
  const before = await page.evaluate(() => fetch('/api/config').then(r => r.json()));
  await page.getByRole('button', {name: 'Anwenden', exact: true}).click();
  await page.waitForFunction(() => document.querySelector('#configMsg').textContent.includes('Angewendet'));
  await page.getByRole('button', {name: 'Dauerhaft speichern', exact: true}).click();
  await page.waitForFunction(() => document.querySelector('#storageState').textContent === 'Dauerhaft gespeichert');
  await page.getByRole('button', {name: 'Neu laden', exact: true}).click();
  await page.waitForFunction(() => document.querySelector('#configMsg').textContent.includes('Neu geladen'));
  const after = await page.evaluate(() => fetch('/api/config').then(r => r.json()));
  const storage = await page.evaluate(() => fetch('/api/storage').then(r => r.json()));
  if (JSON.stringify(before) !== JSON.stringify(after)) throw new Error('Configuration changed during roundtrip');
  if (!storage.saved || !storage.matches) throw new Error('NVS mismatch');
  await page.screenshot({path: `output/playwright/${host}-config-roundtrip.png`, fullPage: true});
  return {host, apply: true, save: true, reload: true, configUnchanged: true, storage};
}
