#pragma once

#include "DspUtils.h"

#include <array>

namespace trichorus::dsp
{

// V1 = left, V2 = centre, V3 = right. Width 0 → all centred.
// Constant-power pan law (−3 dB at centre).
struct PanMatrix
{
    static constexpr int kNumVoices = 3;
    std::array<float, kNumVoices> left {}, right {};

    void setWidth (float width) noexcept
    {
        static constexpr float positions[kNumVoices] = { -1.0f, 0.0f, 1.0f };
        for (int v = 0; v < kNumVoices; ++v)
        {
            const float theta = (width * positions[v] + 1.0f) * (kPi * 0.25f);
            left[(size_t) v]  = std::cos (theta);
            right[(size_t) v] = std::sin (theta);
        }
    }
};

} // namespace trichorus::dsp
