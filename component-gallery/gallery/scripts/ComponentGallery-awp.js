/* Declares the ComponentGallery Audio Worklet Processor */

class ComponentGallery_AWP extends AudioWorkletGlobalScope.WAMProcessor
{
  constructor(options) {
    options = options || {}
    options.mod = AudioWorkletGlobalScope.WAM.ComponentGallery;
    super(options);
  }
}

registerProcessor("ComponentGallery", ComponentGallery_AWP);
