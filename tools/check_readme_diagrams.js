// Playwright CLI run-code callback, using the local readme-diagrams.html preview.
async page => {
  await page.setViewportSize({width: 1280, height: 1900});
  await page.locator('img').evaluateAll(async images => {
    await Promise.all(images.map(image => image.decode()));
  });
  const images = page.locator('img');
  const result = await images.evaluateAll(nodes => nodes.map(image => ({
    source: image.getAttribute('src'), decoded: image.complete && image.naturalWidth > 0,
    width: image.naturalWidth, height: image.naturalHeight,
  })));
  if (result.length !== 3 || result.some(image => !image.decoded)) {
    throw new Error('README diagrams missing or undecodable');
  }
  for (let i = 0; i < result.length; i++) {
    await images.nth(i).screenshot({path: `output/playwright/readme-diagram-${i+1}.png`});
  }
  await page.screenshot({path: 'output/playwright/readme-diagrams-desktop.png', fullPage: true});
  await page.setViewportSize({width: 390, height: 844});
  const overflow = await page.evaluate(() => document.documentElement.scrollWidth > innerWidth);
  if (overflow) throw new Error('Diagram preview overflows mobile viewport');
  await page.screenshot({path: 'output/playwright/readme-diagrams-mobile.png', fullPage: true});
  return {images: result, mobileOverflow: overflow};
}
