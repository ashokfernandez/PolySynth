import { defineConfig } from '@playwright/test';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

export default defineConfig({
  testDir: './specs',
  timeout: 60_000,
  fullyParallel: false,
  retries: process.env.CI ? 2 : 0,
  reporter: process.env.CI ? [['github'], ['html', { open: 'never' }]] : [['list']],
  use: {
    baseURL: 'http://localhost:6006',
    viewport: { width: 1024, height: 768 },
    trace: 'on-first-retry'
  },
  snapshotPathTemplate: '{testDir}/{testFilePath}-snapshots/{arg}{ext}',
  webServer: {
    command: 'npm run storybook',
    cwd: __dirname,
    url: 'http://localhost:6006',
    reuseExistingServer: !process.env.CI,
    timeout: 180_000
  },
  projects: [
    {
      name: 'chromium',
      use: { browserName: 'chromium' }
    }
  ]
});
