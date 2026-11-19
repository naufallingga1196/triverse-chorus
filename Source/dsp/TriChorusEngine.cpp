#include "TriChorusEngine.h"

namespace trichorus::dsp
{

namespace
{
    constexpr double kSmoothingSeconds = 0.02;
    constexpr double kCompressorMs     = 12.0;
    constexpr float  kWetGain          = 0.8f;
    constexpr float  kHfLossRatio      = 0.3f;   // HF-loss cutoff = ratio × clock
    constexpr uint32_t kSeed           = 0x5EEDC0DEu;
}

void TriChorusEngine::prepare (double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = newSampleRate;
    fs = (float) newSampleRate;

    for (auto* s : { &inputGain, &outputGain, &rate, &delayMs, &mix, &tone, &character, &width })
        s->prepare (sampleRate, kSmoothingSeconds);
    for (auto& s : intensity)
        s.prepare (sampleRate, kSmoothingSeconds);

    // Maximum delay: centre delay ÷ (1 − max deviation − max drift) + margin.
    const double maxDelaySeconds = kMaxDelayMs * 0.001 / (1.0 - kMaxClockDev - 0.1) * 1.25;
    delayLine.prepare ((int) std::ceil (maxDelaySeconds * sampleRate) + 8);

    dcBlocker.prepare (sampleRate);
    compressor.setTime (sampleRate, kCompressorMs);
    lfo.prepare (sampleRate);
    modSmoothCoef = OnePoleLowpass::coefficientFor (30.0f, fs);

    lastExpanderMs = -1.0f;
    reset();
}

void TriChorusEngine::reset()
{
    dcBlocker.reset();
    compressor.reset();
    aaFilter.reset();
    delayLine.reset();
    lfo.reset (kSeed);
    noiseRng.setSeed (kSeed ^ 0xA5A5A5A5u);

    for (int v = 0; v < kNumVoices; ++v)
    {
        reconFilter[(size_t) v].reset();
        expander[(size_t) v].reset();
        hfLoss[(size_t) v].reset();
        modSmoother[(size_t) v].reset();
    }
}

void TriChorusEngine::setParameters (const TriChorusParams& p, bool immediate)
{
    auto set = [immediate] (LinearSmoother& s, float v)
    {
        if (immediate) s.snapTo (v);
        else           s.setTarget (v);
    };

    set (inputGain,  dbToGain (p.inputDb));
    set (outputGain, dbToGain (p.outputDb));
    set (rate,       p.rateHz);
    set (delayMs,    std::clamp (p.delayMs, 1.0f, kMaxDelayMs));
    set (mix,        std::clamp (p.mix, 0.0f, 1.0f));
    set (tone,       p.toneHz);
    set (character,  std::clamp (p.character, 0.0f, 1.0f));
    set (width,      std::clamp (p.width, 0.0f, 1.0f));

    for (int v = 0; v < kNumVoices; ++v)
        set (intensity[(size_t) v], std::clamp (p.intensity[(size_t) v], 0.0f, 1.0f));

    shape = p.shape;
    lfo.setShape (shape);

    if (immediate)
    {
        lastMix = -1.0f;
        lastExpanderMs = -1.0f;
        updateSlowParameters (0);
    }
}

void TriChorusEngine::updateSlowParameters (int samplesElapsed) noexcept
{
    const float toneHz = tone.skip (samplesElapsed);
    const float ch     = character.skip (samplesElapsed);
    const float w      = width.skip (samplesElapsed);

    aaFilter.setCutoff (sampleRate, toneHz);
    for (auto& f : reconFilter)
        f.setCutoff (sampleRate, toneHz);

    // One Character knob → three internal amounts.
    saturation.setAmount (ch);
    noiseAmp    = ch > 1.0e-4f ? dbToGain (-88.0f + 18.0f * ch) : 0.0f;
    driftAmount = 0.12f * ch;

    // Deliberate expander time-constant mismatch ("breathing").
    const float expanderMs = (float) kCompressorMs * (1.0f + 0.5f * ch);
    if (std::abs (expanderMs - lastExpanderMs) > 0.01f)
    {
        for (auto& e : expander)
            e.setTime (sampleRate, expanderMs);
        lastExpanderMs = expanderMs;
    }

    pan.setWidth (w);
}

void TriChorusEngine::process (const float* inL, const float* inR, float* outL, float* outR, int numSamples) noexcept
{
    for (int start = 0; start < numSamples; start += kSubBlock)
    {
        const int n = std::min (kSubBlock, numSamples - start);
        processSubBlock (inL + start, inR + start, outL + start, outR + start, n);
        updateSlowParameters (n);
    }
}

void TriChorusEngine::processSubBlock (const float* inL, const float* inR, float* outL, float* outR, int numSamples) noexcept
{
    for (int n = 0; n < numSamples; ++n)
    {
        const float gIn = inputGain.next();
        const float dryL = inL[n] * gIn;
        const float dryR = inR[n] * gIn;

        // --- Input stage + shared compressor + AA filter → BBD
        float x = softClip (dcBlocker.process (0.5f * (dryL + dryR)));
        x = aaFilter.process (compressor.process (x));

        const float noise = noiseAmp > 0.0f ? noiseAmp * noiseRng.nextBipolar() : 0.0f;
        delayLine.write (x + noise);

        // --- Three voices
        lfo.setRate (rate.next());
        const float baseClock = kStages / (2.0f * delayMs.next() * 0.001f);

        float wetL = 0.0f, wetR = 0.0f;
        for (int v = 0; v < kNumVoices; ++v)
        {
            const auto vi = (size_t) v;
            const float target = intensity[vi].next() * lfo.value (v) + driftAmount * lfo.driftValue (v);
            const float mod    = modSmoother[vi].process (target, modSmoothCoef);

            // Modulate in the clock domain; delay time = N / (2 f_clock).
            const float clock  = baseClock * std::max (0.5f, 1.0f + kMaxClockDev * mod);
            const float delaySamples = kStages / (2.0f * clock) * fs;

            float y = delayLine.readHermite (delaySamples);
            y = saturation.process (y);
            y = hfLoss[vi].process (y, OnePoleLowpass::coefficientFor (std::min (kHfLossRatio * clock, 0.45f * fs), fs));
            y = reconFilter[vi].process (y);
            y = expander[vi].process (y);

            wetL += y * pan.left[vi];
            wetR += y * pan.right[vi];
        }
        lfo.advance();

        // --- Mix equal-power + output
        const float m = mix.next();
        if (m != lastMix)
        {
            dryGain = std::cos (m * kPi * 0.5f);
            wetGain = std::sin (m * kPi * 0.5f) * kWetGain;
            lastMix = m;
        }

        const float gOut = outputGain.next();
        outL[n] = (dryL * dryGain + wetL * wetGain) * gOut;
        outR[n] = (dryR * dryGain + wetR * wetGain) * gOut;
    }
}

} // namespace trichorus::dsp
