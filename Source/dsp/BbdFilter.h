#pragma once

#include "DspUtils.h"

namespace trichorus::dsp
{

// TDF-II biquad. Coefficients may be updated without resetting state.
class Biquad
{
public:
    void setLowpass (double sampleRate, double cutoffHz, double q) noexcept
    {
        const double fc    = std::min (cutoffHz, 0.45 * sampleRate);
        const double w0    = 2.0 * 3.14159265358979323846 * fc / sampleRate;
        const double cosw  = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * q);
        const double a0    = 1.0 + alpha;

        b0 = (float) ((1.0 - cosw) * 0.5 / a0);
        b1 = (float) ((1.0 - cosw) / a0);
        b2 = b0;
        a1 = (float) (-2.0 * cosw / a0);
        a2 = (float) ((1.0 - alpha) / a0);
    }

    void reset() noexcept { z1 = z2 = 0.0f; }

    float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

private:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};

// 4-pole Butterworth low-pass (two biquads) — used for the AA and recon filters.
class BbdFilter
{
public:
    void setCutoff (double sampleRate, double cutoffHz) noexcept
    {
        stage1.setLowpass (sampleRate, cutoffHz, 0.54119610);
        stage2.setLowpass (sampleRate, cutoffHz, 1.30656296);
    }

    void reset() noexcept
    {
        stage1.reset();
        stage2.reset();
    }

    float process (float x) noexcept { return stage2.process (stage1.process (x)); }

private:
    Biquad stage1, stage2;
};

} // namespace trichorus::dsp
