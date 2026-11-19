#pragma once

#include "BbdDelayLine.h"
#include "BbdFilter.h"
#include "Compander.h"
#include "DspUtils.h"
#include "LfoBank.h"
#include "PanMatrix.h"
#include "Saturation.h"

#include <array>

namespace trichorus::dsp
{

struct TriChorusParams
{
    float inputDb   = 0.0f;
    float rateHz    = 0.6f;
    std::array<float, 3> intensity { 0.45f, 0.45f, 0.45f }; // 0 … 1, per voice
    float delayMs   = 7.0f;
    LfoShape shape  = LfoShape::Sine;
    float width     = 1.0f;    // 0 … 1
    float toneHz    = 9000.0f;
    float character = 0.3f;    // 0 … 1
    float mix       = 0.5f;    // 0 … 1
    float outputDb  = 0.0f;
};

// Level A tri-stereo chorus (modulated delay + filter + compander).
// Has no JUCE dependency so it can be tested on its own.
class TriChorusEngine
{
public:
    static constexpr int   kNumVoices      = 3;
    static constexpr float kStages         = 1024.0f; // BBD stages
    static constexpr float kMaxClockDev    = 0.3f;    // clock deviation at 100 % intensity
    static constexpr float kMaxDelayMs     = 20.0f;
    static constexpr int   kSubBlock       = 32;      // coefficient update rate

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // immediate = true skips smoothing (used on prepare / state load).
    void setParameters (const TriChorusParams& p, bool immediate = false);

    // May be in-place (inL == outL, inR == outR).
    void process (const float* inL, const float* inR, float* outL, float* outR, int numSamples) noexcept;

private:
    void updateSlowParameters (int samplesElapsed) noexcept;
    void processSubBlock (const float* inL, const float* inR, float* outL, float* outR, int numSamples) noexcept;

    double sampleRate = 48000.0;
    float fs = 48000.0f;

    // Parameters smoothed per sample
    LinearSmoother inputGain, outputGain, rate, delayMs, mix;
    std::array<LinearSmoother, kNumVoices> intensity;
    // Parameters smoothed per sub-block
    LinearSmoother tone, character, width;

    LfoShape shape = LfoShape::Sine;

    DcBlocker dcBlocker;
    Compressor compressor;
    BbdFilter aaFilter;
    BbdDelayLine delayLine;
    LfoBank lfo;
    Random noiseRng;

    std::array<BbdFilter, kNumVoices> reconFilter;
    std::array<Expander, kNumVoices> expander;
    std::array<OnePoleLowpass, kNumVoices> hfLoss;      // clock-dependent HF loss
    std::array<OnePoleLowpass, kNumVoices> modSmoother; // prevents clicks when the shape changes
    float modSmoothCoef = 0.0f;

    BbdSaturation saturation;
    PanMatrix pan;
    float noiseAmp = 0.0f, driftAmount = 0.0f;
    float lastExpanderMs = -1.0f;

    float dryGain = 0.0f, wetGain = 0.0f, lastMix = -1.0f;
};

} // namespace trichorus::dsp
