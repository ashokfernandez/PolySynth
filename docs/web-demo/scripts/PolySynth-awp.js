/* Declares the PolySynth Audio Worklet Processor */

class PolySynth_AWP extends AudioWorkletGlobalScope.WAMProcessor
{
  constructor(options) {
    options = options || {}
    options.mod = AudioWorkletGlobalScope.WAM.PolySynth;
    super(options);
  }
}

registerProcessor("PolySynth", PolySynth_AWP);
