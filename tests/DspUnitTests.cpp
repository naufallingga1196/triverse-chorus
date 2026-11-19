// Framework-free DSP unit tests: run `ctest` or ./trichorus_tests.

#include "dsp/BbdDelayLine.h"
#include "dsp/BbdFilter.h"
#include "dsp/Compander.h"
#include "dsp/LfoBank.h"
#include "dsp/TriChorusEngine.h"

#include <cstdio>
#include <vector>

using namespace trichorus::dsp;

static int failures = 0;

#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        if (! (cond)) {                                                    \
            ++failures;                                                    \
            std::printf ("FAIL %s:%d: %s — ", __FILE__, __LINE__, #cond);  \
            std::printf (__VA_ARGS__);                                     \
            std::printf ("\n");                                            \
        }                                                                  \
    } while (0)

static void testDelayLineIntegerAndFractional()
{
    BbdDelayLine d;
    d.prepare (64);

    // Impulse → at an integer delay of 10 the output must be exactly 1.
    d.write (1.0f);
    for (int i = 0; i < 10; ++i)
        d.write (0.0f);
    CHECK (std::abs (d.readHermite (10.0f) - 1.0f) < 1e-6f, "integer delay = %f", d.readHermite (10.0f));

    // Linear ramp → Hermite interpolation must be exact at half a sample.
    d.reset();
    for (int i = 0; i < 40; ++i)
        d.write ((float) i);
    const float v = d.readHermite (5.5f); // newest is 39 → 39 − 5.5
    CHECK (std::abs (v - 33.5f) < 1e-4f, "ramp interp = %f", v);
}

static float magnitudeAt (BbdFilter& f, double fs, double freq)
{
    f.reset();
    float peak = 0.0f;
    const int n = (int) fs;
    for (int i = 0; i < n; ++i)
    {
        const float y = f.process ((float) std::sin (2.0 * 3.14159265358979 * freq * i / fs));
        if (i > n / 2)
            peak = std::max (peak, std::abs (y));
    }
    return peak;
}

static void testButterworthLowpass()
{
    BbdFilter f;
    const double fs = 48000.0;
    f.setCutoff (fs, 9000.0);

    const float pass = magnitudeAt (f, fs, 200.0);
    const float atFc = magnitudeAt (f, fs, 9000.0);
    const float stop = magnitudeAt (f, fs, 18000.0);

    CHECK (std::abs (pass - 1.0f) < 0.01f, "passband gain = %f", pass);
    CHECK (std::abs (atFc - 0.7071f) < 0.03f, "gain at fc = %f", atFc);
    CHECK (stop < 0.1f, "stopband gain = %f", stop);
}

static void testCompanderIsInverse()
{
    const double fs = 48000.0;
    Compressor c;
    Expander e;
    c.setTime (fs, 12.0);
    e.setTime (fs, 12.0);
    c.reset();
    e.reset();

    // Steady-state level measured after settling.
    for (float amp : { 0.5f, 0.05f })
    {
        float peakIn = 0.0f, peakComp = 0.0f, peakOut = 0.0f;
        for (int i = 0; i < 48000; ++i)
        {
            const float x  = amp * (float) std::sin (2.0 * 3.14159265358979 * 500.0 * i / fs);
            const float xc = c.process (x);
            const float y  = e.process (xc);
            if (i > 24000)
            {
                peakIn   = std::max (peakIn, std::abs (x));
                peakComp = std::max (peakComp, std::abs (xc));
                peakOut  = std::max (peakOut, std::abs (y));
            }
        }
        CHECK (std::abs (peakOut / peakIn - 1.0f) < 0.05f, "compander round trip at %f = %f", amp, peakOut / peakIn);
        (void) peakComp;
    }

    // 2:1 ratio — input drops 20 dB, compressor output drops ~10 dB.
    auto compressedPeak = [&] (float amp)
    {
        Compressor cc;
        cc.setTime (fs, 12.0);
        cc.reset();
        float peak = 0.0f;
        for (int i = 0; i < 48000; ++i)
        {
            const float y = cc.process (amp * (float) std::sin (2.0 * 3.14159265358979 * 500.0 * i / fs));
            if (i > 24000) peak = std::max (peak, std::abs (y));
        }
        return peak;
    };
    const float ratioDb = 20.0f * std::log10 (compressedPeak (0.5f) / compressedPeak (0.05f));
    CHECK (std::abs (ratioDb - 10.0f) < 1.0f, "compressed level change = %f dB", ratioDb);
}

static void testLfoPhaseOffsets()
{
    LfoBank lfo;
    lfo.prepare (48000.0);
    lfo.setShape (LfoShape::Sine);
    lfo.setRate (1.0f);
    lfo.reset (1);

    // Phase 0: sin(0), sin(120°), sin(240°)
    CHECK (std::abs (lfo.value (0)) < 1e-6f, "v0 = %f", lfo.value (0));
    CHECK (std::abs (lfo.value (1) - 0.8660254f) < 1e-4f, "v1 = %f", lfo.value (1));
    CHECK (std::abs (lfo.value (2) + 0.8660254f) < 1e-4f, "v2 = %f", lfo.value (2));

    for (auto s : { LfoShape::Triangle, LfoShape::Analog })
        for (float p : { 0.0f, 0.25f, 0.5f, 0.75f, 0.999f })
        {
            const float v = LfoBank::evaluate (s, p);
            CHECK (v >= -1.0001f && v <= 1.0001f, "shape out of range %f", v);
        }
    CHECK (std::abs (LfoBank::evaluate (LfoShape::Triangle, 0.25f) - 1.0f) < 1e-6f, "tri peak");
}

static bool allFinite (const std::vector<float>& v, float limit)
{
    for (float x : v)
        if (! std::isfinite (x) || std::abs (x) > limit)
            return false;
    return true;
}

static void testEngineStability()
{
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        TriChorusEngine engine;
        engine.prepare (fs, 512);

        TriChorusParams p;
        engine.setParameters (p, true);

        Random rng (42);
        std::vector<float> l (512), r (512);
        bool ok = true;

        for (int block = 0; block < 400; ++block)
        {
            // Random automation of every parameter each block (stress test).
            if (block % 3 == 0)
            {
                p.inputDb   = 12.0f * rng.nextBipolar();
                p.rateHz    = 0.05f + 4.975f * (rng.nextBipolar() + 1.0f);
                for (auto& i : p.intensity) i = 0.5f * (rng.nextBipolar() + 1.0f);
                p.delayMs   = 2.0f + 9.0f * (rng.nextBipolar() + 1.0f);
                p.shape     = (LfoShape) (rng.nextUInt() % 3);
                p.width     = 0.5f * (rng.nextBipolar() + 1.0f);
                p.toneHz    = 4000.0f + 5000.0f * (rng.nextBipolar() + 1.0f);
                p.character = 0.5f * (rng.nextBipolar() + 1.0f);
                p.mix       = 0.5f * (rng.nextBipolar() + 1.0f);
                p.outputDb  = 12.0f * rng.nextBipolar();
                engine.setParameters (p);
            }

            const int n = 1 + (int) (rng.nextUInt() % 512); // varying block size
            for (int i = 0; i < n; ++i)
            {
                l[(size_t) i] = 0.8f * rng.nextBipolar();
                r[(size_t) i] = 0.8f * rng.nextBipolar();
            }
            engine.process (l.data(), r.data(), l.data(), r.data(), n);
            ok = ok && allFinite (l, 50.0f) && allFinite (r, 50.0f);
        }
        CHECK (ok, "engine produced non-finite / runaway output at %.0f Hz", fs);
    }
}

static void testDryPassthroughAtZeroMix()
{
    TriChorusEngine engine;
    engine.prepare (48000.0, 256);
    TriChorusParams p;
    p.mix = 0.0f;
    engine.setParameters (p, true);

    std::vector<float> l (256), r (256), refL (256), refR (256);
    for (int i = 0; i < 256; ++i)
    {
        refL[(size_t) i] = l[(size_t) i] = std::sin ((float) i * 0.05f);
        refR[(size_t) i] = r[(size_t) i] = std::cos ((float) i * 0.07f);
    }
    engine.process (l.data(), r.data(), l.data(), r.data(), 256);

    float maxErr = 0.0f;
    for (int i = 0; i < 256; ++i)
        maxErr = std::max ({ maxErr, std::abs (l[(size_t) i] - refL[(size_t) i]), std::abs (r[(size_t) i] - refR[(size_t) i]) });
    CHECK (maxErr < 1e-5f, "dry path error = %g", maxErr);
}

static void testDeterministicRender()
{
    auto render = []
    {
        TriChorusEngine engine;
        engine.prepare (48000.0, 512);
        engine.setParameters ({}, true);
        std::vector<float> l (4096), r (4096);
        for (int i = 0; i < 4096; ++i)
            l[(size_t) i] = r[(size_t) i] = std::sin ((float) i * 0.03f);
        engine.process (l.data(), r.data(), l.data(), r.data(), 4096);
        return l;
    };
    CHECK (render() == render(), "offline render is not deterministic");
}

int main()
{
    testDelayLineIntegerAndFractional();
    testButterworthLowpass();
    testCompanderIsInverse();
    testLfoPhaseOffsets();
    testEngineStability();
    testDryPassthroughAtZeroMix();
    testDeterministicRender();

    if (failures == 0)
        std::printf ("All DSP tests passed.\n");
    return failures == 0 ? 0 : 1;
}
