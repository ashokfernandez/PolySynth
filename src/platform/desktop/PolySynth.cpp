#include "PolySynth.h"
#include "../../core/PresetManager.h"
#include "IPlugPaths.h"
#include "IPlug_include_in_plug_src.h"
#include "LFO.h"
#include <cstdlib>

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

  // Oscillator Params
  GetParam(kParamOscWave)
      ->InitEnum("Osc Waveform",
                 (int)PolySynthCore::Oscillator::WaveformType::Saw,
                 {"Saw", "Square", "Triangle", "Sine"});
  GetParam(kParamOscMix)->InitDouble("Osc Mix", 0., 0., 100., 1., "%");

#if IPLUG_EDITOR
  mEditorInitFunc = [&]() {
#if defined _DEBUG
    // In debug, load the built index.html from dist/ folder
    std::string path = std::string(__FILE__);
    path = path.substr(0, path.find_last_of("/\\"));
    path += "/resources/web/dist/index.html";
    LoadFile(path.c_str(), nullptr);
#else
#if defined _DEBUG
    // In debug, load the built index.html from dist/ folder
    std::string path = std::string(__FILE__);
    path = path.substr(0, path.find_last_of("/\\"));
    path += "/resources/web/dist/index.html";
    LoadFile(path.c_str(), nullptr);
#else
    LoadIndexHtml(__FILE__, "com.PolySynth.app.PolySynth");
#endif
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
  if (mDemoMode > 0) {
    int samplesPerStep = (int)(GetSampleRate() * 0.25);
    mDemoSampleCounter += nFrames;

    if (mDemoSampleCounter > samplesPerStep) {
      mDemoSampleCounter = 0;
      mDemoNoteIndex++;

      IMidiMsg msg;
      if (mDemoMode == 1) { // Mono
        int seq[] = {48, 55, 60, 67};
        int prevNote = seq[(mDemoNoteIndex - 1) % 4];
        int currNote = seq[mDemoNoteIndex % 4];

        if (mDemoNoteIndex > 0) {
          msg.MakeNoteOffMsg(prevNote, 0);
          mDSP.ProcessMidiMsg(msg);
        }
        msg.MakeNoteOnMsg(currNote, 100, 0);
        mDSP.ProcessMidiMsg(msg);
      } else if (mDemoMode == 2) { // Poly
        int chords[4][3] = {
            {60, 64, 67}, // C
            {60, 65, 69}, // F
            {59, 62, 67}, // G
            {60, 64, 67}  // C
        };

        int prevIdx = (mDemoNoteIndex - 1) % 4;
        int currIdx = mDemoNoteIndex % 4;

        if (mDemoNoteIndex > 0) {
          for (int i = 0; i < 3; i++) {
            msg.MakeNoteOffMsg(chords[prevIdx][i], 0);
            mDSP.ProcessMidiMsg(msg);
          }
        }
        for (int i = 0; i < 3; i++) {
          msg.MakeNoteOnMsg(chords[currIdx][i], 90, 0);
          mDSP.ProcessMidiMsg(msg);
        }
      }
    }
  }

  // Push state to DSP before processing
  mDSP.UpdateState(mState);
  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);
}

void PolySynthPlugin::OnIdle() {}

void PolySynthPlugin::OnReset() {
  mDSP.Reset(GetSampleRate(), GetBlockSize());
  // Ensure DSP has latest state on reset
  mDSP.UpdateState(mState);
}

void PolySynthPlugin::ProcessMidiMsg(const IMidiMsg &msg) {
  mDSP.ProcessMidiMsg(msg);
}

void PolySynthPlugin::OnParamChange(int paramIdx) {
  double value = GetParam(paramIdx)->Value();

  switch (paramIdx) {
  case kParamGain:
    mState.masterGain = value / 100.0;
    break;
  case kParamNoteGlideTime:
    mState.glideTime = value / 1000.0;
    break;
  case kParamAttack:
    mState.ampAttack = value / 1000.0;
    break;
  case kParamDecay:
    mState.ampDecay = value / 1000.0;
    break;
  case kParamSustain:
    mState.ampSustain = value / 100.0;
    break;
  case kParamRelease:
    mState.ampRelease = value / 1000.0;
    break;
  case kParamLFOShape:
    mState.lfoShape = (int)value;
    break;
  case kParamLFORateHz:
    mState.lfoRate = value;
    break;
  case kParamLFORateTempo:
    // TODO: Tempo sync
    break;
  case kParamLFORateMode:
    // TODO: Sync mode
    break;
  case kParamLFODepth:
    mState.lfoDepth = value / 100.0;
    break;
  case kParamFilterCutoff:
    mState.filterCutoff = value;
    break;
  case kParamFilterResonance:
    mState.filterResonance = value / 100.0;
    break;
  case kParamOscWave:
    mState.oscAWaveform = (int)value;
    break;
  case kParamOscMix:
    mState.mixOscA = 1.0 - (value / 100.0);
    mState.mixOscB = value / 100.0;
    static_cast<void>(mState.mixOscB); // Suppress unused for now
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
    if (mDemoMode == 1) {
      mDemoMode = 0;
      mDSP.Reset(GetSampleRate(), GetBlockSize());
    } else {
      mDSP.Reset(GetSampleRate(), GetBlockSize());
      mDemoMode = 1;
      mDemoSampleCounter = (int)(GetSampleRate() * 0.25); // Trigger immediately
      mDemoNoteIndex = -1;
    }
    return true;
  } else if (msgTag == kMsgTagDemoPoly) {
    if (mDemoMode == 2) {
      mDemoMode = 0;
      mDSP.Reset(GetSampleRate(), GetBlockSize());
    } else {
      mDSP.Reset(GetSampleRate(), GetBlockSize());
      mDemoMode = 2;
      mDemoSampleCounter = (int)(GetSampleRate() * 0.25); // Trigger immediately
      mDemoNoteIndex = -1;
    }
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
      GetParam(kParamOscWave)->Set((double)mState.oscAWaveform);
      GetParam(kParamLFOShape)->Set((double)mState.lfoShape);
      GetParam(kParamLFORateHz)->Set(mState.lfoRate);
      GetParam(kParamLFODepth)->Set(mState.lfoDepth * 100.0);

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
