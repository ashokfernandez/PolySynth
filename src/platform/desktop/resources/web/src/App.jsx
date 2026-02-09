import React, { useState } from 'react';
import { useIPlug } from './hooks/useIPlug';
import Knob from './components/Knob';
import Fader from './components/Fader';
import Envelope from './components/Envelope';
import { PARAMS, PARAM_META, mapParamValue } from './constants/params';
import './index.css';

const DemoSection = ({ sendMsg }) => {
  const [activeDemo, setActiveDemo] = useState(null);

  const handleDemo = (type, tag) => {
    if (activeDemo === type) {
      setActiveDemo(null);
      sendMsg(tag);
    } else {
      setActiveDemo(type);
      sendMsg(tag);
    }
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
      <button
        className={`demo-btn ${activeDemo === 'mono' ? 'active' : ''}`}
        onClick={() => handleDemo('mono', 7)}
      >
        Demo Mono
      </button>
      <button
        className={`demo-btn ${activeDemo === 'poly' ? 'active' : ''}`}
        onClick={() => handleDemo('poly', 8)}
      >
        Demo Poly
      </button>
      <div style={{ borderTop: '1px solid #444', paddingTop: '10px', marginTop: '5px' }}>
        <span style={{ fontSize: '12px', color: '#888', marginBottom: '8px', display: 'block' }}>Preset Demo</span>
        <button
          className="demo-btn preset-btn"
          onClick={() => sendMsg(9)}
          title="Save current settings to Desktop"
        >
          💾 Save Preset
        </button>
        <button
          className="demo-btn preset-btn"
          onClick={() => sendMsg(10)}
          style={{ marginTop: '5px' }}
          title="Load preset from Desktop"
        >
          📂 Load Preset
        </button>
      </div>
    </div>
  );
};

function App() {
  const { params, setParam, sendMsg } = useIPlug();

  // Helper to get current value or default from meta
  const getValue = (paramId) => {
    if (params.has(paramId)) return params.get(paramId);
    return 0.5;
  };

  const commonKnobProps = (paramId) => {
    const meta = PARAM_META[paramId];
    return {
      paramId,
      value: getValue(paramId),
      onChange: setParam,
      label: meta.name.split(' ').pop(),
      min: meta.min || 0,
      max: meta.max || 1,
      unit: meta.unit,
      decimals: meta.decimals,
      mapValue: meta.items ? (v) => mapParamValue(paramId, v) : undefined
    };
  };

  return (
    <div className="synth-container">
      <div className="header">
        <h1>PolySynth</h1>
        <div className="status-led"></div>
      </div>

      <div className="controls-row">
        {/* Oscillators */}
        <div className="module oscillators">
          <h2>Oscillators</h2>
          <div className="knob-group">
            <Knob {...commonKnobProps(PARAMS.OSC_WAVE)} label="Waveform" />
            <Knob {...commonKnobProps(PARAMS.OSC_MIX)} label="Mix" />
          </div>
        </div>

        {/* Filter */}
        <div className="module filter">
          <h2>Filter</h2>
          <div className="knob-group">
            <Knob {...commonKnobProps(PARAMS.CUTOFF)} label="Cutoff" />
            <Knob {...commonKnobProps(PARAMS.RESONANCE)} label="Resonance" />
          </div>
        </div>

        {/* LFO */}
        <div className="module lfo">
          <h2>LFO</h2>
          <div className="knob-group">
            <Knob {...commonKnobProps(PARAMS.LFO_SHAPE)} label="Shape" />
            <Knob {...commonKnobProps(PARAMS.LFO_RATE)} label="Rate" />
            <Knob {...commonKnobProps(PARAMS.LFO_DEPTH)} label="Depth" />
          </div>
        </div>
      </div>

      <div className="controls-row">
        {/* Envelope */}
        <div className="module envelope">
          <h2>Amplitude Envelope</h2>
          <Envelope
            attack={getValue(PARAMS.ATTACK)}
            decay={getValue(PARAMS.DECAY)}
            sustain={getValue(PARAMS.SUSTAIN)}
            release={getValue(PARAMS.RELEASE)}
          />
          <div className="fader-group">
            <Fader {...commonKnobProps(PARAMS.ATTACK)} label="A" />
            <Fader {...commonKnobProps(PARAMS.DECAY)} label="D" />
            <Fader {...commonKnobProps(PARAMS.SUSTAIN)} label="S" />
            <Fader {...commonKnobProps(PARAMS.RELEASE)} label="R" />
          </div>
        </div>

        {/* Master */}
        <div className="module master">
          <h2>Master</h2>
          <div className="knob-group">
            <Knob {...commonKnobProps(PARAMS.GAIN)} label="Gain" />
          </div>
        </div>

        {/* Demo */}
        <div className="module demo">
          <h2>Demo</h2>
          <DemoSection sendMsg={sendMsg} />
        </div>
      </div>
    </div>
  );
}

export default App;
