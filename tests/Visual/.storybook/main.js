/** @type {import('@storybook/web-components-vite').StorybookConfig} */
const config = {
  stories: ['../stories/**/*.stories.@(js|jsx|mjs|ts|tsx)'],
  addons: ['@storybook/addon-vitest'],
  framework: {
    name: '@storybook/web-components-vite',
    options: {}
  },
  staticDirs: [
    { from: '../../../ComponentGallery/build-web', to: '/gallery' }
  ]
};

export default config;
