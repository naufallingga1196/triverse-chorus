#pragma once

#include "DspUtils.h"

namespace trichorus::dsp
{

// Input-stage soft clip: ~ +6 dB headroom, nearly transparent at normal levels.
inline float softClip (float x) noexcept
{
    constexpr float headroom = 2.0f;
    return headroom * std::tanh (x / headroom);
}

// BBD saturation: slightly asymmetric tanh (bias) for even harmonics.
// Small-signal gain ≈ 1; the DC offset from the bias is removed.
struct BbdSaturation
{
    float drive = 1.0f, bias = 0.0f, biasOffset = 0.0f;
    bool active = false;

    void setAmount (float amount) noexcept
    {
        active     = amount > 1.0e-4f;
        drive      = 1.0f + 3.0f * amount;
        bias       = 0.12f * amount;
        biasOffset = std::tanh (drive * bias);
    }

    float process (float x) const noexcept
    {
        if (! active)
            return x;
        return (std::tanh (drive * (x + bias)) - biasOffset) / drive;
    }
};

} // namespace trichorus::dsp
