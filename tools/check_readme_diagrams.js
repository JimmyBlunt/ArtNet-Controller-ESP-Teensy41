// Playwright CLI run-code callback, using the local readme-diagrams.html preview.
async page => {
  await page.reload();
  await page.setViewportSize({width: 1280, height: 1900});
  await page.locator('img').evaluateAll(async images => {
    await Promise.all(images.map(image => image.decode()));
  });
  const images = page.locator('img');
  const result = await images.evaluateAll(nodes => nodes.map(image => ({
    source: image.getAttribute('src'), decoded: image.complete && image.naturalWidth > 0,
    width: image.naturalWidth, height: image.naturalHeight,
  })));
  if (result.length !== 5 || result.some(image => !image.decoded)) {
    throw new Error('README diagrams missing or undecodable');
  }
  const textOverflow = await page.evaluate(async () => {
    const problems = [];
    for (const image of document.querySelectorAll('img')) {
      const host = document.createElement('div');
      host.style.cssText = 'position:absolute;left:-10000px;top:0';
      host.innerHTML = await (await fetch(image.src)).text();
      document.body.append(host);
      const svg = host.querySelector('svg');
      const panels = [...svg.querySelectorAll('rect[rx="18"]')].map(r => r.getBBox());
      for (const text of svg.querySelectorAll('text')) {
        const b = text.getBBox();
        const panel = panels.find(r => b.x >= r.x+8 && b.x < r.x+r.width && b.y >= r.y && b.y+b.height <= r.y+r.height);
        if (b.x < 0 || b.x+b.width > svg.viewBox.baseVal.width || (panel && b.x+b.width > panel.x+panel.width-8)) {
          problems.push({source:image.getAttribute('src'), text:text.textContent});
        }
      }
      host.remove();
    }
    return problems;
  });
  if (textOverflow.length) throw new Error(JSON.stringify(textOverflow));
  for (let i = 0; i < result.length; i++) {
    await images.nth(i).screenshot({path: `output/playwright/readme-diagram-${i+1}.png`});
  }
  await page.screenshot({path: 'output/playwright/readme-diagrams-desktop.png', fullPage: true});
  await page.setViewportSize({width: 390, height: 844});
  const overflow = await page.evaluate(() => document.documentElement.scrollWidth > innerWidth);
  if (overflow) throw new Error('Diagram preview overflows mobile viewport');
  await page.screenshot({path: 'output/playwright/readme-diagrams-mobile.png', fullPage: true});
  return {images: result, textOverflow, mobileOverflow: overflow};
}
