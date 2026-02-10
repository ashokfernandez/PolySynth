import React, { useState } from 'react';
import { useIPlug } from './hooks/useIPlug';
import Knob from './components/Knob';
import Fader from './components/Fader';
import Envelope from './components/Envelope';
import { PARAMS, PARAM_META, mapParamValue } from './constants/params';
import './index.css';

const PresetBrowser = ({ sendMsg }) => {
  const [activePreset, setActivePreset] = useState(null);
  const [activeDemo, setActiveDemo] = useState(null);

  const factoryPresets = [
    { id: 'warm', name: '🎹 Warm Pad', tag: 11, description: 'Slow attack, mellow' },
    { id: 'lead', name: '⚡ Bright Lead', tag: 12, description: 'Punchy, resonant' },
    { id: 'bass', name: '🎸 Dark Bass', tag: 13, description: 'Deep, growling' },
  ];

  const handlePreset = (preset) => {
    setActivePreset(preset.id);
    sendMsg(preset.tag);
  };

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
    <div className="preset-browser">
      <div className="preset-section">
        <span className="preset-label">Factory Presets</span>
        <div className="preset-grid">
          {factoryPresets.map(preset => (
            <button
              key={preset.id}
              className={`preset-btn ${activePreset === preset.id ? 'active' : ''}`}
              onClick={() => handlePreset(preset)}
              title={preset.description}
            >
              {preset.name}
            </button>
          ))}
        </div>
      </div>

      <div className="preset-section">
        <span className="preset-label">User Preset</span>
        <div className="user-preset-row">
          <button
            className="preset-btn user-save"
            onClick={() => sendMsg(9)}
            title="Save current settings"
          >
            💾 Save
          </button>
          <button
            className="preset-btn user-load"
            onClick={() => sendMsg(10)}
            title="Load saved preset"
          >
            📂 Load
          </button>
        </div>
      </div>

      <div className="preset-section">
        <span className="preset-label">Audio Demo</span>
        <div className="demo-row">
          <button
            className={`demo-btn ${activeDemo === 'mono' ? 'active' : ''}`}
            onClick={() => handleDemo('mono', 7)}
          >
            🎵 Mono
          </button>
          <button
            className={`demo-btn ${activeDemo === 'poly' ? 'active' : ''}`}
            onClick={() => handleDemo('poly', 8)}
          >
            🎶 Poly
          </button>
          <button
            className={`demo-btn ${activeDemo === 'fx' ? 'active' : ''}`}
            onClick={() => handleDemo('fx', 14)}
          >
            ✨ FX
          </button>
        </div>
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

        {/* FX */}
        <div className="module fx">
          <h2>FX</h2>
          <div className="fx-grid">
            <div className="knob-group">
              <Knob {...commonKnobProps(PARAMS.CHORUS_RATE)} label="Chorus Rate" />
              <Knob {...commonKnobProps(PARAMS.CHORUS_DEPTH)} label="Chorus Depth" />
              <Knob {...commonKnobProps(PARAMS.CHORUS_MIX)} label="Chorus Mix" />
            </div>
            <div className="knob-group">
              <Knob {...commonKnobProps(PARAMS.DELAY_TIME)} label="Delay Time" />
              <Knob {...commonKnobProps(PARAMS.DELAY_FEEDBACK)} label="Delay Feedback" />
              <Knob {...commonKnobProps(PARAMS.DELAY_MIX)} label="Delay Mix" />
            </div>
            <div className="knob-group">
              <Knob
                {...commonKnobProps(PARAMS.LIMITER_THRESHOLD)}
                label="Limiter"
              />
            </div>
          </div>
        </div>

        {/* Presets & Demo */}
        <div className="module presets">
          <h2>Presets</h2>
          <PresetBrowser sendMsg={sendMsg} />
        </div>
      </div>
    </div>
  );
}

export default App;
