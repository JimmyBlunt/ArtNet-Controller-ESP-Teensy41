async page => {
  const results = [];
  for (const host of ['10.0.0.248', '10.0.0.251']) {
    await page.goto(`http://${host}/`, {waitUntil: 'domcontentloaded'});
    await page.waitForTimeout(2000);
    const data = await page.evaluate(async () => {
      const read = async path => {
        const response = await fetch(path);
        if (!response.ok) throw new Error(`${path}: ${response.status}`);
        return response.json();
      };
      return {config: await read('/api/config'), storage: await read('/api/storage'),
        status: await read('/api/status'), test: await read('/api/test-pattern')};
    });
    if (!data.storage.saved || !data.storage.matches) throw new Error(`${host}: NVS mismatch`);
    if (data.test.active || data.test.blackout) throw new Error(`${host}: test mode still active`);
    if (!Object.hasOwn(data.status, 'rxPackets')) throw new Error(`${host}: missing RX firmware`);
    const expectedProfile = host.endsWith('248') ? 'esp32-wroom-flex-8ws-2apa' : 'flex8-ws2812-apa102';
    if (data.config.hardwareProfile !== expectedProfile) throw new Error(`${host}: wrong profile`);
    await page.screenshot({path: `output/playwright/${host}-final-overview.png`, fullPage: true});
    results.push({host, ...data});
  }
  await page.goto('about:blank');
  return {checkedAt: new Date().toISOString(), checksPassed: true, controllers: results};
}
