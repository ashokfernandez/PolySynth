import { expect, test } from '@playwright/test';

test('hello world gallery renders', async ({ page }) => {
  await page.goto('/iframe.html?id=componentgallery-components--envelope&viewMode=story');
  const componentFrame = page.locator('iframe[title="Component Gallery"]');
  await expect(componentFrame).toBeVisible();
  await expect(componentFrame).toHaveAttribute('src', /gallery\/envelope\/index\.html$/);
});

test('polyknob story auto-initializes without manual start', async ({ page }) => {
  await page.goto('/iframe.html?id=componentgallery-components--poly-knob&viewMode=story');
  const galleryFrame = page.frameLocator('iframe[title="Component Gallery"]');
  await galleryFrame.locator('body').waitFor({ state: 'visible' });
  await expect(galleryFrame.locator('#buttons')).toBeHidden();
});
