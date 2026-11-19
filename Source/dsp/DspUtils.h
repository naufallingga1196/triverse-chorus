#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace trichorus::dsp
{

constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

// Generator acak deterministik (xorshift32). Seed tetap per instance agar
// render offline selalu identik.
class Random
{
public:
    explicit Random (uint32_t seed = 0x9E3779B9u) noexcept { setSeed (seed); }

    void setSeed (uint32_t seed) noexcept { state = seed != 0 ? seed : 0x9E3779B9u; }

    uint32_t nextUInt() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    // Uniform −1 … +1
    float nextBipolar() noexcept
    {
        return (float) (nextUInt() >> 8) * (2.0f / 16777216.0f) - 1.0f;
    }

private:
    uint32_t state;
};

// Smoother linear sederhana (pengganti juce::SmoothedValue agar DSP bebas JUCE).
class LinearSmoother
{
public:
    void prepare (double sampleRate, double rampSeconds) noexcept
    {
        rampSteps = std::max (1, (int) std::lround (sampleRate * rampSeconds));
    }

    void snapTo (float v) noexcept
    {
        current = target = v;
        remaining = 0;
    }

    void setTarget (float v) noexcept
    {
        if (v == target)
            return;

        target    = v;
        remaining = rampSteps;
        step      = (target - current) / (float) rampSteps;
    }

    float next() noexcept
    {
        if (remaining > 0)
        {
            current += step;
            if (--remaining == 0)
                current = target;
        }
        return current;
    }

    // Maju n sampel sekaligus (untuk parameter yang diperbarui per sub-blok).
    float skip (int n) noexcept
    {
        if (remaining > 0)
        {
            if (n >= remaining) { current = target; remaining = 0; }
            else                { current += step * (float) n; remaining -= n; }
        }
        return current;
    }

    float getCurrent() const noexcept { return current; }
    bool isSmoothing() const noexcept { return remaining > 0; }

private:
    float current = 0.0f, target = 0.0f, step = 0.0f;
    int remaining = 0, rampSteps = 1;
};

// One-pole low-pass dengan koefisien yang boleh berubah tiap sampel.
struct OnePoleLowpass
{
    float z = 0.0f;

    void reset() noexcept { z = 0.0f; }

    // a = 1 − e^(−w), didekati dengan w / (1 + w) — cukup akurat dan murah.
    static float coefficientFor (float cutoffHz, float sampleRate) noexcept
    {
        const float w = kTwoPi * cutoffHz / sampleRate;
        return w / (1.0f + w);
    }

    float process (float x, float a) noexcept
    {
        z += a * (x - z);
        return z;
    }
};

// DC blocker (HPF satu-pole, ~10 Hz).
class DcBlocker
{
public:
    void prepare (double sampleRate) noexcept
    {
        r = (float) std::exp (-kTwoPi * 10.0 / sampleRate);
    }

    void reset() noexcept { x1 = y1 = 0.0f; }

    float process (float x) noexcept
    {
        const float y = x - x1 + r * y1;
        x1 = x;
        y1 = y;
        return y;
    }

private:
    float r = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

} // namespace trichorus::dsp
