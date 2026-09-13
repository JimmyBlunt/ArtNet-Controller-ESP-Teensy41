async page => {
  const results = [];
  for (const [width, height] of [[1440, 900], [1920, 1080], [390, 844]]) {
    await page.setViewportSize({width, height});
    const metrics = await page.evaluate(async () => {
      const style = getComputedStyle(document.body, '::before');
      const img = new Image();
      img.src = '/orbital-prism.webp';
      await img.decode();
      return {
        width: innerWidth, height: innerHeight, imageWidth: img.naturalWidth,
        imageHeight: img.naturalHeight, opacity: style.opacity,
        tangent: parseFloat(style.left) + parseFloat(style.width) * 373 / 1672,
        overflow: document.documentElement.scrollWidth > innerWidth,
        background: style.backgroundImage,
        runtime: document.getElementById('runtimeState').textContent
      };
    });
    if (metrics.opacity !== '1' || metrics.overflow ||
        Math.abs(metrics.tangent - 2) > 1 || metrics.imageWidth !== 1672)
      throw new Error(JSON.stringify(metrics));
    results.push(metrics);
    await page.screenshot({path: `output/playwright/orbital-${width}.png`});
  }
  await page.setViewportSize({width: 1440, height: 900});
  return results;
}
