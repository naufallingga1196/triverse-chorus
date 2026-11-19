#pragma once

#include "DspUtils.h"

namespace trichorus::dsp
{

// Average detector (rectifier + one-pole), not true RMS — like the NE570.
class EnvelopeFollower
{
public:
    void setTime (double sampleRate, double ms) noexcept
    {
        coef = (float) std::exp (-1.0 / (sampleRate * ms * 0.001));
    }

    void reset (float value = 0.0f) noexcept { env = value; }

    float process (float x) noexcept
    {
        const float r = std::abs (x);
        env = r + coef * (env - r);
        return env;
    }

private:
    float coef = 0.999f, env = 0.0f;
};

// Feedforward topology: each detector reads its own stage's input, so the
// expander is the exact inverse of the compressor (as long as time constants match).
namespace compander
{
    constexpr float kReference = 0.1f;               // unity point (mean |x|)
    constexpr float kFloor     = kReference * 1e-3f; // compressor gain limit: +30 dB
}

// 2:1 compressor — g = 1 / sqrt(env / ref)
class Compressor
{
public:
    void setTime (double sampleRate, double ms) noexcept { detector.setTime (sampleRate, ms); }
    void reset() noexcept { detector.reset (compander::kReference); }

    float process (float x) noexcept
    {
        const float e = std::max (detector.process (x), compander::kFloor);
        return x * std::sqrt (compander::kReference / e);
    }

private:
    EnvelopeFollower detector;
};

// Expander 1:2 — g = env / ref
class Expander
{
public:
    void setTime (double sampleRate, double ms) noexcept { detector.setTime (sampleRate, ms); }
    void reset() noexcept { detector.reset (compander::kReference); }

    float process (float x) noexcept
    {
        return x * (detector.process (x) / compander::kReference);
    }

private:
    EnvelopeFollower detector;
};

} // namespace trichorus::dsp
