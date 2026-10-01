async page => {
  const host = new URL(page.url()).hostname;
  const get = path => page.evaluate(path => fetch(path).then(r => r.json()), path);
  const original = await get('/api/config');
  const initial = await get('/api/status');
  await page.getByRole('button', {name: 'Im Loop', exact: true}).click();
  await page.waitForTimeout(1200);
  const loop = await get('/api/test-pattern');
  const running = await get('/api/status');
  if (!loop.active || !loop.loop || running.outputFrames <= initial.outputFrames)
    throw new Error('Loop did not produce LED frames');
  await page.screenshot({path: `output/playwright/${host}-test-loop.png`, fullPage: true});
  await page.getByRole('button', {name: 'Blackout', exact: true}).click();
  const blackout = await get('/api/test-pattern');
  if (!blackout.blackout) throw new Error('Blackout not held');
  await page.getByRole('button', {name: 'Stop → Art-Net', exact: true}).click();
  const stopped = await get('/api/test-pattern');
  if (stopped.active || stopped.blackout) throw new Error('Art-Net not released');
  const final = await get('/api/config');
  if (JSON.stringify(original) !== JSON.stringify(final)) throw new Error('Test changed config');
  return {host, loop, blackout, stopped, outputFramesAdvanced: running.outputFrames - initial.outputFrames,
          configUnchanged: true};
}
