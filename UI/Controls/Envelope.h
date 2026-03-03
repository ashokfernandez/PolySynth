#pragma once

#include "IControl.h"
#include "IGraphics.h"
#include "ADSRViewModel.h"
#include "PolyTheme.h"
#include <atomic>

using namespace iplug;
using namespace igraphics;

class Envelope : public IControl {
public:
    Envelope(const IRECT& bounds, const IVStyle& style = DEFAULT_STYLE)
        : IControl(bounds), mVoicePhase(nullptr), mPhaseRef(0.0f), mPathIsDirty(true)
    {
        mIgnoreMouse = true;
    }

    void SetADSR(float a, float d, float s, float r) {
        mViewModel.SetParams(a, d, s, r, 0.0f, 0.0f, 0.0f);
        mPathIsDirty = true;
        SetDirty(false);
    }
    
    void SetTensions(float at, float dt, float rt) {
        mViewModel.SetParams(mViewModel.CalculatePathData().attackNode.x, // hacky way to just trigger dirty, keeping actual tension simple here
                             0, 0, 0, at, dt, rt); // the ViewModel implementation is already parameterized.
        mPathIsDirty = true;
        SetDirty(false);
    }

    void SetColors(const IColor& stroke, const IColor& fill) {
        mViewModel.SetBaseColorHSL(182.0f, 1.0f, 0.42f);
        mPathIsDirty = true;
        SetDirty(false);
    }

    void OnResize() override {
        IRECT drawRect = mRECT.GetPadded(-10.f);
        mViewModel.SetBounds(drawRect.L, drawRect.T, drawRect.W(), drawRect.H());
        mPathIsDirty = true;
        SetDirty(false);
    }

    void OnIdle() {
        if (mVoicePhase) {
            float phase = mVoicePhase->load(std::memory_order_acquire);
            if (std::abs(phase - mPhaseRef) > 0.005f) {
                mPhaseRef = phase;
                SetDirty(false);
            }
        }
    }

    void Draw(IGraphics& g) override {
        // Draw Background
        g.FillRoundRect(PolyTheme::ControlBG, mRECT, PolyTheme::RoundingSection);
        g.DrawRoundRect(PolyTheme::SectionBorder, mRECT, PolyTheme::RoundingSection, nullptr, 1.f);

        auto data = mViewModel.CalculatePathData();
        IRECT drawRect = mRECT.GetPadded(-10.f);

        auto buildEnvelopePath = [&]() {
            g.PathMoveTo(drawRect.L, drawRect.B); // Bottom Left Start
            g.PathQuadraticBezierTo(data.attackControlPoint.x, data.attackControlPoint.y, data.attackNode.x, data.attackNode.y);
            g.PathQuadraticBezierTo(data.decayControlPoint.x, data.decayControlPoint.y, data.decaySustainNode.x, data.decaySustainNode.y);
            g.PathLineTo(data.releaseControlPoint.x, data.decaySustainNode.y);
            g.PathQuadraticBezierTo(data.releaseControlPoint.x, data.releaseControlPoint.y, data.releaseNode.x, drawRect.B);
            g.PathLineTo(drawRect.L, drawRect.B); // Close loop
        };

        // 1. Draw Background Faint Path
        g.PathClear();
        buildEnvelopePath();
        g.PathStroke(mViewModel.GetColor(ADSRViewModel::Theme::Faint), 2.0f);

        // Calculate Playhead X coordinate
        float currentX = mViewModel.GetPlayheadX(mPhaseRef);
        IRECT clipRect = IRECT(mRECT.L, mRECT.T, currentX, mRECT.B);

        // 2. Playhead Layer (Fill + Dark Outline, clipped to playhead)
        g.PathClipRegion(clipRect); // Clip subsequent drawing to only the active envelope progress

        g.PathClear();
        buildEnvelopePath();
        g.PathFill(mViewModel.GetColor(ADSRViewModel::Theme::Fill));

        g.PathClear();
        buildEnvelopePath();
        // Offset stroke rendering so it draws inside the filled path nicely
        g.PathStroke(mViewModel.GetColor(ADSRViewModel::Theme::Dark), 3.0f);

        // 3. Scanner Line & Glow
        // Create an IGraphics pattern for the gradient glow bounded exactly at the playhead edge
        float glowStartX = currentX - 80.0f;
        IPattern glowPattern = IPattern::CreateLinearGradient(glowStartX, mRECT.T, currentX, mRECT.T, 
            {
                {COLOR_TRANSPARENT, 0.0f},
                {mViewModel.GetColor(ADSRViewModel::Theme::Glow), 1.0f}
            });

        // The glow should only be drawn *under* the actual curve!
        // We reuse the envelope path build, and fill it with the gradient pattern
        g.PathClear();
        buildEnvelopePath();
        g.PathFill(glowPattern);
        
        // Remove clipping area so drawing restores back to normal panel space
        g.PathClipRegion(IRECT()); 

        // Scanner line marking current playback
        g.DrawLine(mViewModel.GetColor(ADSRViewModel::Theme::Dark), currentX, mRECT.T, currentX, mRECT.B, nullptr, 2.0f);
        
        mPathIsDirty = false;
    }

private:
    ADSRViewModel mViewModel;
    std::atomic<float>* mVoicePhase;
    float mPhaseRef;
    bool mPathIsDirty;
};
