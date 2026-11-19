#pragma once

#include "DspUtils.h"

#include <array>

namespace trichorus::dsp
{

enum class LfoShape { Sine = 0, Triangle, Analog };

// Nilai acak halus (interpolasi smoothstep antar-target acak) untuk drift.
class SmoothRandom
{
public:
    void prepare (double sampleRate, float frequencyHz) noexcept
    {
        increment = (float) (frequencyHz / sampleRate);
    }

    void reset (Random& rng) noexcept
    {
        from = rng.nextBipolar();
        to   = rng.nextBipolar();
        t    = 0.0f;
    }

    float next (Random& rng) noexcept
    {
        t += increment;
        if (t >= 1.0f)
        {
            t -= 1.0f;
            from = to;
            to = rng.nextBipolar();
        }
        const float s = t * t * (3.0f - 2.0f * t);
        return from + (to - from) * s;
    }

private:
    float increment = 0.0f, from = 0.0f, to = 0.0f, t = 0.0f;
};

// Tiga LFO dengan satu fase master dan offset tetap (default 0°, 120°, 240°).
class LfoBank
{
public:
    static constexpr int kNumVoices = 3;

    void prepare (double sampleRate) noexcept
    {
        fs = (float) sampleRate;
        static constexpr float driftHz[kNumVoices] = { 0.13f, 0.17f, 0.11f };
        for (int v = 0; v < kNumVoices; ++v)
            drift[(size_t) v].prepare (sampleRate, driftHz[v]);
    }

    void reset (uint32_t seed) noexcept
    {
        rng.setSeed (seed);
        phase = 0.0f;
        for (auto& d : drift)
            d.reset (rng);
        update();
    }

    void setRate (float hz) noexcept         { increment = hz / fs; }
    void setShape (LfoShape s) noexcept      { shape = s; }
    void setSpreadDegrees (float d) noexcept { spread = d / 360.0f; }

    // −1 … +1
    float value (int voice) const noexcept      { return values[(size_t) voice]; }
    float driftValue (int voice) const noexcept { return drifts[(size_t) voice]; }

    void advance() noexcept
    {
        phase += increment;
        if (phase >= 1.0f)
            phase -= 1.0f;
        update();
    }

    static float evaluate (LfoShape s, float p) noexcept
    {
        if (s == LfoShape::Sine)
            return std::sin (kTwoPi * p);

        // Segitiga sefase dengan sinus (0 di p = 0, naik).
        const float tri = p < 0.25f ? 4.0f * p
                        : p < 0.75f ? 2.0f - 4.0f * p
                                    : 4.0f * p - 4.0f;

        if (s == LfoShape::Triangle)
            return tri;

        // "Analog": puncak segitiga dibulatkan seperti LFO integrator op-amp.
        return tri * (1.5f - 0.5f * tri * tri);
    }

private:
    void update() noexcept
    {
        for (int v = 0; v < kNumVoices; ++v)
        {
            float p = phase + spread * (float) v;
            p -= std::floor (p);
            values[(size_t) v] = evaluate (shape, p);
            drifts[(size_t) v] = drift[(size_t) v].next (rng);
        }
    }

    float fs = 48000.0f, phase = 0.0f, increment = 0.0f, spread = 1.0f / 3.0f;
    LfoShape shape = LfoShape::Sine;
    Random rng;
    std::array<SmoothRandom, kNumVoices> drift;
    std::array<float, kNumVoices> values {}, drifts {};
};

} // namespace trichorus::dsp
