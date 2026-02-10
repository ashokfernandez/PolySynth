const createGalleryFrame = () => {
  const wrapper = document.createElement('div');
  wrapper.style.width = '1024px';
  wrapper.style.height = '768px';
  wrapper.style.margin = '0 auto';
  wrapper.style.background = 'rgb(70, 70, 70)';

  const frame = document.createElement('iframe');
  frame.title = 'Component Gallery';
  frame.src = 'gallery/index.html';
  frame.style.width = '100%';
  frame.style.height = '100%';
  frame.style.border = '0';

  frame.addEventListener('load', () => {
    const doc = frame.contentDocument;
    if (!doc) return;

    const buttons = doc.getElementById('buttons');
    if (buttons) {
      buttons.style.display = 'none';
    }

    const greyout = doc.getElementById('greyout');
    if (greyout) {
      greyout.style.display = 'none';
    }
  });

  wrapper.appendChild(frame);
  return wrapper;
};

export default {
  title: 'ComponentGallery/UI Components'
};

export const Knob = {
  render: () => createGalleryFrame(),
  parameters: {
    controls: { disable: true }
  }
};
