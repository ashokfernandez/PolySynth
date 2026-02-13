const createGalleryFrame = (componentName) => {
  const wrapper = document.createElement('div');
  wrapper.style.width = '1024px';
  wrapper.style.height = '768px';
  wrapper.style.margin = '0 auto';
  wrapper.style.background = 'rgb(70, 70, 70)';

  const frame = document.createElement('iframe');
  frame.title = 'Component Gallery';
  frame.src = `gallery/${componentName}/index.html`;
  frame.style.width = '100%';
  frame.style.height = '100%';
  frame.style.border = '0';

  frame.addEventListener('load', () => {
    const doc = frame.contentDocument;
    if (!doc) return;

    // Hide UI buttons and greyout
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
  title: 'ComponentGallery/Components'
};

export const Knob = {
  render: () => createGalleryFrame('knob')
};

export const Fader = {
  render: () => createGalleryFrame('fader')
};

export const Envelope = {
  render: () => createGalleryFrame('envelope')
};

export const Button = {
  render: () => createGalleryFrame('button')
};

export const Switch = {
  render: () => createGalleryFrame('switch')
};

export const Toggle = {
  render: () => createGalleryFrame('toggle')
};

export const SlideSwitch = {
  render: () => createGalleryFrame('slideswitch')
};

export const TabSwitch = {
  render: () => createGalleryFrame('tabswitch')
};

export const RadioButton = {
  render: () => createGalleryFrame('radiobutton')
};

// --- Custom PolySynth controls ---

export const PolyKnob = {
  render: () => createGalleryFrame('polyknob')
};

export const PolySection = {
  render: () => createGalleryFrame('polysection')
};

export const PolyToggle = {
  render: () => createGalleryFrame('polytoggle')
};
