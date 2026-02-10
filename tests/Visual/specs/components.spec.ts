import { expect, test } from '@playwright/test';

test('knob component renders isolated', async ({ page }) => {
  await page.goto('/iframe.html?id=componentgallery-components--knob&viewMode=story');
  const galleryFrame = page.frameLocator('iframe[title="Component Gallery"]');

  // Wait for WAM to load
  await galleryFrame.locator('#wam').waitFor({ state: 'visible' });
  await galleryFrame.locator('#wam canvas.pluginArea').waitFor({ state: 'visible' });

  // Wait a moment for component to fully render
  await page.waitForTimeout(500);

  // Take screenshot of the knob component
  await expect(page).toHaveScreenshot('knob-component.png', {
    fullPage: true,
    maxDiffPixelRatio: 0.012
  });
});

test('fader component renders isolated', async ({ page }) => {
  await page.goto('/iframe.html?id=componentgallery-components--fader&viewMode=story');
  const galleryFrame = page.frameLocator('iframe[title="Component Gallery"]');

  // Wait for WAM to load
  await galleryFrame.locator('#wam').waitFor({ state: 'visible' });
  await galleryFrame.locator('#wam canvas.pluginArea').waitFor({ state: 'visible' });

  // Wait a moment for component to fully render
  await page.waitForTimeout(500);

  // Take screenshot of the fader component
  await expect(page).toHaveScreenshot('fader-component.png', {
    fullPage: true,
    maxDiffPixelRatio: 0.012
  });
});

test('envelope component renders isolated', async ({ page }) => {
  await page.goto('/iframe.html?id=componentgallery-components--envelope&viewMode=story');
  const galleryFrame = page.frameLocator('iframe[title="Component Gallery"]');

  // Wait for WAM to load
  await galleryFrame.locator('#wam').waitFor({ state: 'visible' });
  await galleryFrame.locator('#wam canvas.pluginArea').waitFor({ state: 'visible' });

  // Wait a moment for component to fully render
  await page.waitForTimeout(500);

  // Take screenshot of the envelope component
  await expect(page).toHaveScreenshot('envelope-component.png', {
    fullPage: true,
    maxDiffPixelRatio: 0.012
  });
});

test('knob story auto-initializes without manual start', async ({ page }) => {
  await page.goto('/iframe.html?id=componentgallery-components--knob&viewMode=story');
  const galleryFrame = page.frameLocator('iframe[title="Component Gallery"]');
  await galleryFrame.locator('#wam').waitFor({ state: 'visible' });
  await galleryFrame.locator('#wam canvas.pluginArea').waitFor({ state: 'visible' });
  await expect(galleryFrame.locator('#buttons')).toBeHidden();
});

test('components render with correct dimensions', async ({ page }) => {
  const components = ['knob', 'fader', 'envelope'];

  for (const component of components) {
    await page.goto(`/iframe.html?id=componentgallery-components--${component}&viewMode=story`);
    const galleryFrame = page.frameLocator('iframe[title="Component Gallery"]');

    // Wait for WAM to load
    await galleryFrame.locator('#wam').waitFor({ state: 'visible' });
    const canvas = galleryFrame.locator('#wam canvas.pluginArea');
    await canvas.waitFor({ state: 'visible' });

    // Verify canvas dimensions (should match PLUG_WIDTH x PLUG_HEIGHT from config.h)
    const boundingBox = await canvas.boundingBox();
    expect(boundingBox).not.toBeNull();
    expect(boundingBox!.width).toBeGreaterThan(800); // Should be 1024px or scaled
    expect(boundingBox!.height).toBeGreaterThan(600); // Should be 768px or scaled
  }
});
