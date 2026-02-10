#include "PolySynth.h"
#include "../../core/PresetManager.h"
#include "IPlugPaths.h"
#include "IPlug_include_in_plug_src.h"
#include <cstdlib>

namespace {
static const double kToPercentage = 100.0;
static const double kToMs = 1000.0;
} // namespace

void PolySynthPlugin::SyncUIState() {
  GetParam(kParamGain)->Set(mState.masterGain * kToPercentage);
  GetParam(kParamNoteGlideTime)->Set(mState.glideTime * kToMs);
  GetParam(kParamAttack)->Set(mState.ampAttack * kToMs);
  GetParam(kParamDecay)->Set(mState.ampDecay * kToMs);
  GetParam(kParamSustain)->Set(mState.ampSustain * kToPercentage);
  GetParam(kParamRelease)->Set(mState.ampRelease * kToMs);
  GetParam(kParamLFOShape)->Set((double)mState.lfoShape);
  GetParam(kParamLFORateHz)->Set(mState.lfoRate);
  GetParam(kParamLFODepth)->Set(mState.lfoDepth * kToPercentage);
  GetParam(kParamFilterCutoff)->Set(mState.filterCutoff);
  GetParam(kParamFilterResonance)->Set(mState.filterResonance * kToPercentage);
  GetParam(kParamFilterEnvAmount)->Set(mState.filterEnvAmount * kToPercentage);
  GetParam(kParamFilterModel)->Set((double)mState.filterModel);
  GetParam(kParamOscWave)->Set((double)mState.oscAWaveform);
  GetParam(kParamOscBWave)->Set((double)mState.oscBWaveform);
  GetParam(kParamOscMix)->Set(mState.mixOscB * kToPercentage);
  GetParam(kParamOscPulseWidthA)->Set(mState.oscAPulseWidth * kToPercentage);
  GetParam(kParamOscPulseWidthB)->Set(mState.oscBPulseWidth * kToPercentage);
  GetParam(kParamPolyModOscBToFreqA)
      ->Set(mState.polyModOscBToFreqA * kToPercentage);
  GetParam(kParamPolyModOscBToPWM)
      ->Set(mState.polyModOscBToPWM * kToPercentage);
  GetParam(kParamPolyModOscBToFilter)
      ->Set(mState.polyModOscBToFilter * kToPercentage);
  GetParam(kParamPolyModFilterEnvToFreqA)
      ->Set(mState.polyModFilterEnvToFreqA * kToPercentage);
  GetParam(kParamPolyModFilterEnvToPWM)
      ->Set(mState.polyModFilterEnvToPWM * kToPercentage);
  GetParam(kParamPolyModFilterEnvToFilter)
      ->Set(mState.polyModFilterEnvToFilter * kToPercentage);
  GetParam(kParamChorusRate)->Set(mState.fxChorusRate);
  GetParam(kParamChorusDepth)->Set(mState.fxChorusDepth * kToPercentage);
  GetParam(kParamChorusMix)->Set(mState.fxChorusMix * kToPercentage);
  GetParam(kParamDelayTime)->Set(mState.fxDelayTime * kToMs);
  GetParam(kParamDelayFeedback)->Set(mState.fxDelayFeedback * kToPercentage);
  GetParam(kParamDelayMix)->Set(mState.fxDelayMix * kToPercentage);
  GetParam(kParamLimiterThreshold)
      ->Set(mState.fxLimiterThreshold * kToPercentage);

  for (int i = 0; i < kNumParams; ++i) {
    SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
  }
}

PolySynthPlugin::PolySynthPlugin(const InstanceInfo &info)
    : Plugin(info, MakeConfig(kNumParams, kNumPresets)) {
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamNoteGlideTime)
      ->InitMilliseconds("Note Glide Time", 0., 0.0, 30.);
  GetParam(kParamAttack)
      ->InitDouble("Attack", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone,
                   "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamDecay)
      ->InitDouble("Decay", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone,
                   "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamSustain)
      ->InitDouble("Sustain", 50., 0., 100., 1, "%", IParam::kFlagsNone,
                   "ADSR");
  GetParam(kParamRelease)
      ->InitDouble("Release", 10., 2., 1000., 0.1, "ms", IParam::kFlagsNone,
                   "ADSR");

  // LFO Params
  GetParam(kParamLFOShape)
      ->InitEnum("LFO Shape", 1, {"Sine", "Triangle", "Square", "Saw"});
  GetParam(kParamLFORateHz)->InitFrequency("LFO Rate", 1., 0.01, 40.);
  GetParam(kParamLFORateTempo)
      ->InitEnum("LFO Rate", 0, {"1/1", "1/2", "1/4", "1/8", "1/16"});
  GetParam(kParamLFORateMode)->InitBool("LFO Sync", true);
  GetParam(kParamLFODepth)->InitPercentage("LFO Depth");

  // Filter Params
  GetParam(kParamFilterCutoff)
      ->InitDouble("Cutoff", 20000., 20., 20000., 1., "Hz", IParam::kFlagsNone,
                   "Filter", IParam::ShapeExp());
  GetParam(kParamFilterResonance)
      ->InitDouble("Resonance", 0., 0., 100., 1., "%");
  GetParam(kParamFilterEnvAmount)->InitPercentage("Filter Env");
  GetParam(kParamFilterModel)
      ->InitEnum("Filter Model", 0,
                 {"Classic", "Ladder", "Prophet 12", "Prophet 24"});

  // Oscillator Params
  GetParam(kParamOscWave)
      ->InitEnum("Osc Waveform",
                 (int)PolySynthCore::Oscillator::WaveformType::Saw,
                 {"Saw", "Square", "Triangle", "Sine"});
  GetParam(kParamOscBWave)
      ->InitEnum("Osc B Waveform",
                 (int)PolySynthCore::Oscillator::WaveformType::Sine,
                 {"Saw", "Square", "Triangle", "Sine"});
  GetParam(kParamOscMix)->InitDouble("Osc Mix", 0., 0., 100., 1., "%");
  GetParam(kParamOscPulseWidthA)->InitPercentage("Pulse Width A");
  GetParam(kParamOscPulseWidthB)->InitPercentage("Pulse Width B");

  // Poly-Mod Params
  GetParam(kParamPolyModOscBToFreqA)->InitPercentage("B -> Freq A");
  GetParam(kParamPolyModOscBToPWM)->InitPercentage("B -> PWM A");
  GetParam(kParamPolyModOscBToFilter)->InitPercentage("B -> Filter");
  GetParam(kParamPolyModFilterEnvToFreqA)->InitPercentage("Env -> Freq A");
  GetParam(kParamPolyModFilterEnvToPWM)->InitPercentage("Env -> PWM A");
  GetParam(kParamPolyModFilterEnvToFilter)->InitPercentage("Env -> Filter");

  // FX Params
  GetParam(kParamChorusRate)->InitFrequency("Chorus Rate", 0.25, 0.05, 2.0);
  GetParam(kParamChorusDepth)
      ->InitDouble("Chorus Depth", 50., 0., 100., 1., "%");
  GetParam(kParamChorusMix)->InitDouble("Chorus Mix", 0., 0., 100., 1., "%");
  GetParam(kParamDelayTime)->InitMilliseconds("Delay Time", 350., 50., 1200.);
  GetParam(kParamDelayFeedback)
      ->InitDouble("Delay Feedback", 35., 0., 95., 1., "%");
  GetParam(kParamDelayMix)->InitDouble("Delay Mix", 0., 0., 100., 1., "%");
  GetParam(kParamLimiterThreshold)
      ->InitDouble("Limiter Threshold", 95., 50., 100., 1., "%");

#if IPLUG_EDITOR
  mEditorInitFunc = [&]() {
#if defined _DEBUG
    // In debug, load the built index.html from dist/ folder
    std::string path = std::string(__FILE__);
    path = path.substr(0, path.find_last_of("/\\"));
    path += "/resources/web/dist/index.html";
    LoadFile(path.c_str(), nullptr);
#else
    LoadIndexHtml(__FILE__, "com.PolySynth.app.PolySynth");
#endif
    EnableScroll(false);
  };
#endif
}

#if IPLUG_EDITOR
void PolySynthPlugin::OnUIOpen() {
  for (int paramIdx = 0; paramIdx < kNumParams; ++paramIdx) {
    SendParameterValueFromDelegate(paramIdx,
                                   GetParam(paramIdx)->GetNormalized(), true);
  }
}

void PolySynthPlugin::OnParamChangeUI(int paramIdx, EParamSource source) {
  (void)source;
  SendParameterValueFromDelegate(paramIdx, GetParam(paramIdx)->GetNormalized(),
                                 true);
}
#endif

#if IPLUG_DSP
void PolySynthPlugin::ProcessBlock(sample **inputs, sample **outputs,
                                   int nFrames) {
  mDemoSequencer.Process(nFrames, GetSampleRate(), mDSP);

  mDSP.UpdateState(mState);
  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);
}

void PolySynthPlugin::OnIdle() {}

void PolySynthPlugin::OnReset() {
  mDSP.Reset(GetSampleRate(), GetBlockSize());
  // Ensure DSP has latest state on reset
  mDSP.UpdateState(mState);
}

// Log note on messages
void PolySynthPlugin::ProcessMidiMsg(const IMidiMsg &msg) {
  mDSP.ProcessMidiMsg(msg);
}

void PolySynthPlugin::OnParamChange(int paramIdx) {
  double value = GetParam(paramIdx)->Value();

  switch (paramIdx) {
  case kParamGain:
    mState.masterGain = value / kToPercentage;
    break;
  case kParamNoteGlideTime:
    mState.glideTime = value / kToMs;
    break;
  case kParamAttack:
    mState.ampAttack = value / kToMs;
    break;
  case kParamDecay:
    mState.ampDecay = value / kToMs;
    break;
  case kParamSustain:
    mState.ampSustain = value / kToPercentage;
    break;
  case kParamRelease:
    mState.ampRelease = value / kToMs;
    break;
  case kParamLFOShape:
    mState.lfoShape = static_cast<int>(value);
    break;
  case kParamLFORateHz:
    mState.lfoRate = value;
    break;
  case kParamLFODepth:
    mState.lfoDepth = value / kToPercentage;
    break;
  case kParamFilterCutoff:
    mState.filterCutoff = value;
    break;
  case kParamFilterResonance:
    mState.filterResonance = value / kToPercentage;
    break;
  case kParamFilterEnvAmount:
    mState.filterEnvAmount = value / kToPercentage;
    break;
  case kParamFilterModel:
    mState.filterModel = static_cast<int>(value);
    break;
  case kParamOscWave:
    mState.oscAWaveform = static_cast<int>(value);
    break;
  case kParamOscBWave:
    mState.oscBWaveform = static_cast<int>(value);
    break;
  case kParamOscMix:
    mState.mixOscA = 1.0 - (value / kToPercentage);
    mState.mixOscB = value / kToPercentage;
    break;
  case kParamOscPulseWidthA:
    mState.oscAPulseWidth = value / kToPercentage;
    break;
  case kParamOscPulseWidthB:
    mState.oscBPulseWidth = value / kToPercentage;
    break;
  case kParamPolyModOscBToFreqA:
    mState.polyModOscBToFreqA = value / kToPercentage;
    break;
  case kParamPolyModOscBToPWM:
    mState.polyModOscBToPWM = value / kToPercentage;
    break;
  case kParamPolyModOscBToFilter:
    mState.polyModOscBToFilter = value / kToPercentage;
    break;
  case kParamPolyModFilterEnvToFreqA:
    mState.polyModFilterEnvToFreqA = value / kToPercentage;
    break;
  case kParamPolyModFilterEnvToPWM:
    mState.polyModFilterEnvToPWM = value / kToPercentage;
    break;
  case kParamPolyModFilterEnvToFilter:
    mState.polyModFilterEnvToFilter = value / kToPercentage;
    break;
  case kParamChorusRate:
    mState.fxChorusRate = value;
    break;
  case kParamChorusDepth:
    mState.fxChorusDepth = value / kToPercentage;
    break;
  case kParamChorusMix:
    mState.fxChorusMix = value / kToPercentage;
    break;
  case kParamDelayTime:
    mState.fxDelayTime = value / kToMs;
    break;
  case kParamDelayFeedback:
    mState.fxDelayFeedback = value / kToPercentage;
    break;
  case kParamDelayMix:
    mState.fxDelayMix = value / kToPercentage;
    break;
  case kParamLimiterThreshold:
    mState.fxLimiterThreshold = value / kToPercentage;
    break;
  default:
    break;
  }

  // Legacy support for direct param setting is removed in favor of UpdateState
  // loop mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}

bool PolySynthPlugin::OnMessage(int msgTag, int ctrlTag, int dataSize,
                                const void *pData) {
  if (msgTag == kMsgTagTestLoaded) {
    if (std::getenv("POLYSYNTH_TEST_UI")) {
      printf("TEST_PASS: UI Loaded\n");
      exit(0);
    }
  } else if (msgTag == kMsgTagDemoMono) {
    mDemoSequencer.SetMode(DemoSequencer::Mode::Mono, GetSampleRate(), mDSP);
    return true;
  } else if (msgTag == kMsgTagDemoPoly) {
    mDemoSequencer.SetMode(DemoSequencer::Mode::Poly, GetSampleRate(), mDSP);
    return true;
  } else if (msgTag == kMsgTagDemoFX) {
    mDemoSequencer.SetMode(DemoSequencer::Mode::FX, GetSampleRate(), mDSP);
    if (mDemoSequencer.GetMode() == DemoSequencer::Mode::FX) {
      mState.fxChorusRate = 0.35;
      mState.fxChorusDepth = 0.65;
      mState.fxChorusMix = 0.35;
      mState.fxDelayTime = 0.45;
      mState.fxDelayFeedback = 0.45;
      mState.fxDelayMix = 0.35;
      mState.fxLimiterThreshold = 0.7;
    } else {
      mState.fxChorusMix = 0.0;
      mState.fxDelayMix = 0.0;
      mState.fxLimiterThreshold = 0.95;
    }
    // Update UI and DSP
    SyncUIState();
    return true;
  } else if (msgTag == kMsgTagSavePreset) {
    // Save current state to demo preset file
    WDL_String presetPath;
    DesktopPath(presetPath);
    presetPath.Append("/PolySynth_DemoPreset.json");
    bool success =
        PolySynthCore::PresetManager::SaveToFile(mState, presetPath.Get());
    printf("PRESET_SAVE: %s (%s)\n", presetPath.Get(), success ? "OK" : "FAIL");
    return true;
  } else if (msgTag == kMsgTagLoadPreset) {
    // Load state from demo preset file
    WDL_String presetPath;
    DesktopPath(presetPath);
    presetPath.Append("/PolySynth_DemoPreset.json");
    PolySynthCore::SynthState loadedState;
    bool success = PolySynthCore::PresetManager::LoadFromFile(presetPath.Get(),
                                                              loadedState);
    if (success) {
      mState = loadedState;
      // Sync UI with loaded state by updating all parameters
      GetParam(kParamGain)->Set(mState.masterGain * 100.0);
      GetParam(kParamAttack)->Set(mState.ampAttack * 1000.0);
      GetParam(kParamDecay)->Set(mState.ampDecay * 1000.0);
      GetParam(kParamSustain)->Set(mState.ampSustain * 100.0);
      GetParam(kParamRelease)->Set(mState.ampRelease * 1000.0);
      GetParam(kParamFilterCutoff)->Set(mState.filterCutoff);
      GetParam(kParamFilterResonance)->Set(mState.filterResonance * 100.0);
      GetParam(kParamFilterEnvAmount)->Set(mState.filterEnvAmount * 100.0);
      GetParam(kParamFilterModel)->Set((double)mState.filterModel);
      GetParam(kParamOscWave)->Set((double)mState.oscAWaveform);
      GetParam(kParamOscBWave)->Set((double)mState.oscBWaveform);
      GetParam(kParamLFOShape)->Set((double)mState.lfoShape);
      GetParam(kParamLFORateHz)->Set(mState.lfoRate);
      GetParam(kParamLFODepth)->Set(mState.lfoDepth * 100.0);
      GetParam(kParamOscPulseWidthA)->Set(mState.oscAPulseWidth * 100.0);
      GetParam(kParamOscPulseWidthB)->Set(mState.oscBPulseWidth * 100.0);
      GetParam(kParamPolyModOscBToFreqA)
          ->Set(mState.polyModOscBToFreqA * 100.0);
      GetParam(kParamPolyModOscBToPWM)->Set(mState.polyModOscBToPWM * 100.0);
      GetParam(kParamPolyModOscBToFilter)
          ->Set(mState.polyModOscBToFilter * 100.0);
      GetParam(kParamPolyModFilterEnvToFreqA)
          ->Set(mState.polyModFilterEnvToFreqA * 100.0);
      GetParam(kParamPolyModFilterEnvToPWM)
          ->Set(mState.polyModFilterEnvToPWM * 100.0);
      GetParam(kParamPolyModFilterEnvToFilter)
          ->Set(mState.polyModFilterEnvToFilter * 100.0);
      GetParam(kParamChorusRate)->Set(mState.fxChorusRate);
      GetParam(kParamChorusDepth)->Set(mState.fxChorusDepth * 100.0);
      GetParam(kParamChorusMix)->Set(mState.fxChorusMix * 100.0);
      GetParam(kParamDelayTime)->Set(mState.fxDelayTime * 1000.0);
      GetParam(kParamDelayFeedback)->Set(mState.fxDelayFeedback * 100.0);
      GetParam(kParamDelayMix)->Set(mState.fxDelayMix * 100.0);
      GetParam(kParamLimiterThreshold)->Set(mState.fxLimiterThreshold * 100.0);

      // Notify UI of all param changes
      for (int i = 0; i < kNumParams; ++i) {
        SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
      }
      printf("PRESET_LOAD: %s (OK)\n", presetPath.Get());
    } else {
      printf("PRESET_LOAD: %s (FAIL - file not found or invalid)\n",
             presetPath.Get());
    }
    return true;
  } else if (msgTag == kMsgTagPreset1 || msgTag == kMsgTagPreset2 ||
             msgTag == kMsgTagPreset3) {
    // Factory Presets with distinct sounds
    if (msgTag == kMsgTagPreset1) {
      // Warm Pad - Slow attack, low cutoff, no resonance
      mState.masterGain = 0.8;
      mState.ampAttack = 0.5; // 500ms
      mState.ampDecay = 0.3;
      mState.ampSustain = 0.7;
      mState.ampRelease = 1.0; // 1 second
      mState.filterCutoff = 800.0;
      mState.filterResonance = 0.1;
      mState.filterModel = 0;
      mState.oscAWaveform = 0; // Saw
      mState.oscBWaveform = 3; // Sine
      mState.oscAPulseWidth = 0.5;
      mState.oscBPulseWidth = 0.5;
      mState.filterEnvAmount = 0.0;
      mState.lfoShape = 0; // Sine
      mState.lfoRate = 0.5;
      mState.lfoDepth = 0.3;
      mState.polyModOscBToFreqA = 0.0;
      mState.polyModOscBToPWM = 0.0;
      mState.polyModOscBToFilter = 0.0;
      mState.polyModFilterEnvToFreqA = 0.0;
      mState.polyModFilterEnvToPWM = 0.0;
      mState.polyModFilterEnvToFilter = 0.0;
      printf("PRESET: Warm Pad loaded\n");
    } else if (msgTag == kMsgTagPreset2) {
      // Bright Lead - Fast attack, high cutoff, high resonance
      mState.masterGain = 1.0;
      mState.ampAttack = 0.005; // 5ms
      mState.ampDecay = 0.1;
      mState.ampSustain = 0.6;
      mState.ampRelease = 0.2;
      mState.filterCutoff = 18000.0;
      mState.filterResonance = 0.7;
      mState.filterModel = 2;
      mState.oscAWaveform = 1; // Square
      mState.oscBWaveform = 1; // Square
      mState.oscAPulseWidth = 0.5;
      mState.oscBPulseWidth = 0.5;
      mState.filterEnvAmount = 0.0;
      mState.lfoShape = 2; // Square LFO
      mState.lfoRate = 6.0;
      mState.lfoDepth = 0.0; // No LFO
      mState.polyModOscBToFreqA = 0.0;
      mState.polyModOscBToPWM = 0.0;
      mState.polyModOscBToFilter = 0.0;
      mState.polyModFilterEnvToFreqA = 0.0;
      mState.polyModFilterEnvToPWM = 0.0;
      mState.polyModFilterEnvToFilter = 0.0;
      printf("PRESET: Bright Lead loaded\n");
    } else if (msgTag == kMsgTagPreset3) {
      // Dark Bass - Medium attack, very low cutoff, medium resonance
      mState.masterGain = 0.9;
      mState.ampAttack = 0.02; // 20ms
      mState.ampDecay = 0.5;
      mState.ampSustain = 0.4;
      mState.ampRelease = 0.15;
      mState.filterCutoff = 300.0;
      mState.filterResonance = 0.5;
      mState.filterModel = 1;
      mState.oscAWaveform = 0; // Saw
      mState.oscBWaveform = 2; // Triangle
      mState.oscAPulseWidth = 0.5;
      mState.oscBPulseWidth = 0.5;
      mState.filterEnvAmount = 0.0;
      mState.lfoShape = 1; // Triangle
      mState.lfoRate = 2.0;
      mState.lfoDepth = 0.5;
      mState.polyModOscBToFreqA = 0.0;
      mState.polyModOscBToPWM = 0.0;
      mState.polyModOscBToFilter = 0.0;
      mState.polyModFilterEnvToFreqA = 0.0;
      mState.polyModFilterEnvToPWM = 0.0;
      mState.polyModFilterEnvToFilter = 0.0;
      printf("PRESET: Dark Bass loaded\n");
    }

    mState.fxChorusRate = 0.25;
    mState.fxChorusDepth = 0.5;
    mState.fxChorusMix = 0.0;
    mState.fxDelayTime = 0.35;
    mState.fxDelayFeedback = 0.35;
    mState.fxDelayMix = 0.0;
    mState.fxLimiterThreshold = 0.95;

    // Sync UI with loaded state by updating all parameters
    GetParam(kParamGain)->Set(mState.masterGain * 100.0);
    GetParam(kParamAttack)->Set(mState.ampAttack * 1000.0);
    GetParam(kParamDecay)->Set(mState.ampDecay * 1000.0);
    GetParam(kParamSustain)->Set(mState.ampSustain * 100.0);
    GetParam(kParamRelease)->Set(mState.ampRelease * 1000.0);
    GetParam(kParamFilterCutoff)->Set(mState.filterCutoff);
    GetParam(kParamFilterResonance)->Set(mState.filterResonance * 100.0);
    GetParam(kParamFilterEnvAmount)->Set(mState.filterEnvAmount * 100.0);
    GetParam(kParamFilterModel)->Set((double)mState.filterModel);
    GetParam(kParamOscWave)->Set((double)mState.oscAWaveform);
    GetParam(kParamOscBWave)->Set((double)mState.oscBWaveform);
    GetParam(kParamLFOShape)->Set((double)mState.lfoShape);
    GetParam(kParamLFORateHz)->Set(mState.lfoRate);
    GetParam(kParamLFODepth)->Set(mState.lfoDepth * 100.0);
    GetParam(kParamOscPulseWidthA)->Set(mState.oscAPulseWidth * 100.0);
    GetParam(kParamOscPulseWidthB)->Set(mState.oscBPulseWidth * 100.0);
    GetParam(kParamPolyModOscBToFreqA)->Set(mState.polyModOscBToFreqA * 100.0);
    GetParam(kParamPolyModOscBToPWM)->Set(mState.polyModOscBToPWM * 100.0);
    GetParam(kParamPolyModOscBToFilter)
        ->Set(mState.polyModOscBToFilter * 100.0);
    GetParam(kParamPolyModFilterEnvToFreqA)
        ->Set(mState.polyModFilterEnvToFreqA * 100.0);
    GetParam(kParamPolyModFilterEnvToPWM)
        ->Set(mState.polyModFilterEnvToPWM * 100.0);
    GetParam(kParamPolyModFilterEnvToFilter)
        ->Set(mState.polyModFilterEnvToFilter * 100.0);
    GetParam(kParamChorusRate)->Set(mState.fxChorusRate);
    GetParam(kParamChorusDepth)->Set(mState.fxChorusDepth * 100.0);
    GetParam(kParamChorusMix)->Set(mState.fxChorusMix * 100.0);
    GetParam(kParamDelayTime)->Set(mState.fxDelayTime * 1000.0);
    GetParam(kParamDelayFeedback)->Set(mState.fxDelayFeedback * 100.0);
    GetParam(kParamDelayMix)->Set(mState.fxDelayMix * 100.0);
    GetParam(kParamLimiterThreshold)->Set(mState.fxLimiterThreshold * 100.0);

    // Notify UI of all param changes
    for (int i = 0; i < kNumParams; ++i) {
      SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
    }
    return true;
  } else if (msgTag == kMsgTagNoteOn) {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(ctrlTag, 100, 0);
    mDSP.ProcessMidiMsg(msg);
    return true;
  } else if (msgTag == kMsgTagNoteOff) {
    IMidiMsg msg;
    msg.MakeNoteOffMsg(ctrlTag, 0);
    mDSP.ProcessMidiMsg(msg);
    return true;
  }
  return false;
}

bool PolySynthPlugin::CanNavigateToURL(const char *url) { return true; }

bool PolySynthPlugin::OnCanDownloadMIMEType(const char *mimeType) {
  return std::string_view(mimeType) != "text/html";
}

void PolySynthPlugin::OnDownloadedFile(const char *path) {
  WDL_String str;
  str.SetFormatted(64, "Downloaded file to %s\n", path);
  LoadHTML(str.Get());
}

void PolySynthPlugin::OnFailedToDownloadFile(const char *path) {
  WDL_String str;
  str.SetFormatted(64, "Failed to download file to %s\n", path);
  LoadHTML(str.Get());
}

void PolySynthPlugin::OnGetLocalDownloadPathForFile(const char *fileName,
                                                    WDL_String &localPath) {
  DesktopPath(localPath);
  localPath.AppendFormatted(MAX_WIN32_PATH_LEN, "/%s", fileName);
}
#endif
