#pragma once

#include "DspUtils.h"

namespace trichorus::dsp
{

// Detektor rata-rata (rectifier + one-pole), bukan true RMS — seperti NE570.
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

// Topologi feedforward: detektor membaca input masing-masing tahap, sehingga
// expander adalah kebalikan persis dari compressor (selama konstanta waktu sama).
namespace compander
{
    constexpr float kReference = 0.1f;               // titik unity (rata-rata |x|)
    constexpr float kFloor     = kReference * 1e-3f; // batas gain kompresor: +30 dB
}

// Kompresor 2:1 — g = 1 / sqrt(env / ref)
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
