#pragma once

#include "IControl.h"
#include "IGraphics.h"
#include "ADSRViewModel.h"
#include "PolyTheme.h"
#include <chrono>
#include <cmath>
#include <algorithm>

using namespace iplug;
using namespace igraphics;

// Maximum simultaneous voice cursors the envelope can display.
// Must be >= kParamPolyphonyLimit max value (16).
static constexpr int kMaxEnvelopeVoices = 16;

class Envelope : public IControl {
public:
    explicit Envelope(const IRECT& bounds, const IVStyle& /*style*/ = DEFAULT_STYLE)
        : IControl(bounds)
    {
        mIgnoreMouse = true;
    }

    void SetADSR(float a, float d, float s, float r) {
        mViewModel.SetParams(a, d, s, r, 0.0f, 0.0f, 0.0f);
        SetDirty(false);
    }

    void SetColors(const IColor& base, const IColor& /*fill*/ = IColor()) {
        // Convert RGB to HSL for the view model
        float r = base.R / 255.f, g = base.G / 255.f, b = base.B / 255.f;
        float maxC = std::max({r, g, b}), minC = std::min({r, g, b});
        float l = (maxC + minC) * 0.5f;
        float s = 0.f, h = 0.f;
        if (maxC != minC) {
            float d = maxC - minC;
            s = (l > 0.5f) ? d / (2.f - maxC - minC) : d / (maxC + minC);
            if (maxC == r)      h = (g - b) / d + (g < b ? 6.f : 0.f);
            else if (maxC == g) h = (b - r) / d + 2.f;
            else                h = (r - g) / d + 4.f;
            h *= 60.f;
        }
        mViewModel.SetBaseColorHSL(h, s, l);
        SetDirty(false);
    }

    bool IsDirty() override {
        if (IControl::IsDirty()) return true;
        for (int i = 0; i < kMaxEnvelopeVoices; i++) {
            if (mVoices[i].active) return true;
        }
        return false;
    }

    void OnResize() override {
        IRECT drawRect = mRECT.GetPadded(-10.f);
        mViewModel.SetBounds(drawRect.L, drawRect.T, drawRect.W(), drawRect.H());
        SetDirty(false);
    }

    // Called by the plugin (UI thread) when a voice slot starts playing.
    // slotIdx is 0-based and assigned/managed entirely by the plugin layer.
    void OnVoiceOn(int slotIdx) {
        if (slotIdx < 0 || slotIdx >= kMaxEnvelopeVoices) return;
        mVoices[slotIdx].active     = true;
        mVoices[slotIdx].released   = false;
        mVoices[slotIdx].noteOnTime = Clock::now();
        SetDirty(false);
    }

    // Called by the plugin (UI thread) when a voice slot enters release phase.
    void OnVoiceOff(int slotIdx) {
        if (slotIdx < 0 || slotIdx >= kMaxEnvelopeVoices) return;
        if (!mVoices[slotIdx].active) return;
        mVoices[slotIdx].released    = true;
        mVoices[slotIdx].noteOffTime = Clock::now();
    }

    void Draw(IGraphics& g) override {
        // Background
        g.FillRoundRect(PolyTheme::ControlBG, mRECT, PolyTheme::RoundingSection);
        g.DrawRoundRect(PolyTheme::SectionBorder, mRECT, PolyTheme::RoundingSection, nullptr, 1.f);

        auto data      = mViewModel.CalculatePathData();
        IRECT drawRect = mRECT.GetPadded(-10.f);

        // Outline: continuous ADSR shape including the sustain plateau.
        auto buildEnvelopeOutline = [&]() {
            g.PathMoveTo(drawRect.L, drawRect.B);
            g.PathLineTo(data.attackNode.x, data.attackNode.y);
            g.PathLineTo(data.decaySustainNode.x, data.decaySustainNode.y);
            g.PathLineTo(data.releaseStartNode.x, data.releaseStartNode.y);
            g.PathLineTo(data.releaseNode.x, drawRect.B);
        };

        // Fill: closed polygon including sustain segment and bottom edge.
        auto buildEnvelopeFill = [&]() {
            g.PathMoveTo(drawRect.L, drawRect.B);
            g.PathLineTo(data.attackNode.x, data.attackNode.y);
            g.PathLineTo(data.decaySustainNode.x, data.decaySustainNode.y);
            g.PathLineTo(data.releaseStartNode.x, data.releaseStartNode.y);
            g.PathLineTo(data.releaseNode.x, drawRect.B);
            g.PathLineTo(drawRect.L, drawRect.B);
        };

        // 1. Faint background outline
        g.PathClear();
        buildEnvelopeOutline();
        g.PathStroke(mViewModel.GetColor(ADSRViewModel::Theme::Faint), 2.0f);

        // Compute per-voice X positions from elapsed wall-clock time + ADSR params.
        auto now = Clock::now();
        float a  = mViewModel.GetAttack();
        float d  = mViewModel.GetDecay();
        float s  = mViewModel.GetSustain();
        float r  = mViewModel.GetRelease();

        float maxVoiceX = -1.0f;

        for (int i = 0; i < kMaxEnvelopeVoices; i++) {
            auto& voice = mVoices[i];
            if (!voice.active) continue;

            float x = ComputeNoteX(voice, now, a, d, s, r, data, drawRect);
            if (x < 0.0f) {
                voice.active = false; // release phase complete
                continue;
            }
            voice.currentX = x;
            maxVoiceX = std::max(maxVoiceX, x);
        }

        // 2. Reveal layer — clipped to the furthest active voice.
        if (maxVoiceX >= drawRect.L) {
            IRECT clipRect = IRECT(mRECT.L, mRECT.T, maxVoiceX, mRECT.B);
            g.PathClipRegion(clipRect);

            g.PathClear();
            buildEnvelopeFill();
            g.PathFill(mViewModel.GetColor(ADSRViewModel::Theme::Fill));

            g.PathClear();
            buildEnvelopeOutline();
            g.PathStroke(mViewModel.GetColor(ADSRViewModel::Theme::Dark), 3.0f);

            // Trailing glow gradient
            float glowStartX = maxVoiceX - 80.0f;
            IPattern glowPattern = IPattern::CreateLinearGradient(
                glowStartX, mRECT.T, maxVoiceX, mRECT.T,
                {{COLOR_TRANSPARENT, 0.0f},
                 {mViewModel.GetColor(ADSRViewModel::Theme::Glow), 1.0f}});
            g.PathClear();
            buildEnvelopeFill();
            g.PathFill(glowPattern);

            g.PathClipRegion(IRECT());
        }

        // 3. Per-voice scan lines — opacity fades with envelope amplitude (Y position).
        //    A cursor near the top (loud) is fully opaque; one near the bottom (silent)
        //    fades to invisible, so release tails naturally disappear.
        IColor baseColor = mViewModel.GetColor(ADSRViewModel::Theme::Base);
        for (int i = 0; i < kMaxEnvelopeVoices; i++) {
            if (!mVoices[i].active || mVoices[i].currentX < 0.0f) continue;

            float scanX  = mVoices[i].currentX;
            float curveY = ComputeEnvelopeYAtX(scanX, data, drawRect);

            // Alpha = 1 when envelope is at full amplitude, 0 when silent.
            float alpha = (drawRect.B - curveY) / drawRect.H();
            alpha = std::max(0.0f, std::min(1.0f, alpha));

            IColor lineColor(static_cast<int>(alpha * 255),
                             baseColor.R, baseColor.G, baseColor.B);
            g.DrawLine(lineColor, scanX, curveY, scanX, drawRect.B, nullptr, 2.0f);
        }
    }

private:
    using Clock = std::chrono::steady_clock;

    struct VoiceTracker {
        bool      active      = false;
        bool      released    = false;
        Clock::time_point noteOnTime;
        Clock::time_point noteOffTime;
        float     currentX    = -1.0f;
    };

    // Returns the Y coordinate on the piecewise-linear ADSR path at a given X.
    static float ComputeEnvelopeYAtX(float x, const ADSRPathData& data, const IRECT& drawRect)
    {
        // Attack: drawRect.L → attackNode.x
        if (x <= data.attackNode.x) {
            float segW = data.attackNode.x - drawRect.L;
            if (segW < 0.001f) return data.attackNode.y;
            float t = (x - drawRect.L) / segW;
            return drawRect.B + t * (data.attackNode.y - drawRect.B);
        }
        // Decay: attackNode.x → decaySustainNode.x
        if (x <= data.decaySustainNode.x) {
            float segW = data.decaySustainNode.x - data.attackNode.x;
            if (segW < 0.001f) return data.decaySustainNode.y;
            float t = (x - data.attackNode.x) / segW;
            return data.attackNode.y + t * (data.decaySustainNode.y - data.attackNode.y);
        }
        // Sustain: flat at sustainY
        if (x <= data.releaseStartNode.x) {
            return data.decaySustainNode.y;
        }
        // Release: releaseStartNode.x → releaseNode.x
        if (x <= data.releaseNode.x) {
            float segW = data.releaseNode.x - data.releaseStartNode.x;
            if (segW < 0.001f) return drawRect.B;
            float t = (x - data.releaseStartNode.x) / segW;
            return data.releaseStartNode.y + t * (drawRect.B - data.releaseStartNode.y);
        }
        return drawRect.B;
    }

    static float ComputeNoteX(const VoiceTracker& voice,
                               const Clock::time_point& now,
                               float a, float d, float s, float r,
                               const ADSRPathData& data,
                               const IRECT& drawRect)
    {
        if (voice.released) {
            float elapsed = std::chrono::duration<float>(now - voice.noteOffTime).count();
            if (r < 0.001f || elapsed >= r) return -1.0f;
            float progress = elapsed / r;
            return data.releaseStartNode.x
                 + progress * (data.releaseNode.x - data.releaseStartNode.x);
        }

        float elapsed = std::chrono::duration<float>(now - voice.noteOnTime).count();

        if (elapsed < a) {
            float progress = (a > 0.0f) ? elapsed / a : 1.0f;
            return drawRect.L + progress * (data.attackNode.x - drawRect.L);
        }
        if (elapsed < a + d) {
            float progress = (d > 0.0f) ? (elapsed - a) / d : 1.0f;
            return data.attackNode.x
                 + progress * (data.decaySustainNode.x - data.attackNode.x);
        }
        return data.releaseStartNode.x; // sustain hold
    }

    ADSRViewModel mViewModel;
    VoiceTracker  mVoices[kMaxEnvelopeVoices] = {};
};
