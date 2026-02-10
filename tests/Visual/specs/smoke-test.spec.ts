import { expect, test } from '@playwright/test';

test('hello world gallery renders', async ({ page }) => {
  await page.goto('/iframe.html?id=componentgallery-ui-components--knob&viewMode=story');
  const galleryFrame = page.frameLocator('iframe[title="Component Gallery"]');
  await galleryFrame.locator('#wam').waitFor({ state: 'visible' });
  await galleryFrame.locator('#wam canvas.pluginArea').waitFor({ state: 'visible' });
  await expect(page).toHaveScreenshot('smoke-test.png', {
    fullPage: true,
    maxDiffPixelRatio: 0.012
  });
});

test('knob story auto-initializes without manual start', async ({ page }) => {
  await page.goto('/iframe.html?id=componentgallery-ui-components--knob&viewMode=story');
  const galleryFrame = page.frameLocator('iframe[title="Component Gallery"]');
  await galleryFrame.locator('#wam').waitFor({ state: 'visible' });
  await galleryFrame.locator('#wam canvas.pluginArea').waitFor({ state: 'visible' });
  await expect(galleryFrame.locator('#buttons')).toBeHidden();
});
