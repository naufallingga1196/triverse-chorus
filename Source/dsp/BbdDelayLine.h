#pragma once

#include <algorithm>
#include <vector>

namespace trichorus::dsp
{

// Delay line sirkular satu-tulis banyak-baca dengan interpolasi Hermite 4-titik.
class BbdDelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        int size = 1;
        while (size < maxDelaySamples + 4)
            size <<= 1;

        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        writeIndex = 0;
        maxDelay = (float) (size - 3);
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    void write (float x) noexcept
    {
        buffer[(size_t) writeIndex] = x;
        writeIndex = (writeIndex + 1) & mask;
    }

    // delaySamples diukur dari sampel yang terakhir ditulis (0 = sampel terbaru).
    float readHermite (float delaySamples) const noexcept
    {
        const float d    = std::clamp (delaySamples, 1.0f, maxDelay);
        const int   i    = (int) d;
        const float t    = d - (float) i;
        const int   newest = writeIndex - 1;

        const float ym1 = at (newest - (i - 1));
        const float y0  = at (newest - i);
        const float y1  = at (newest - (i + 1));
        const float y2  = at (newest - (i + 2));

        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);

        return ((c3 * t + c2) * t + c1) * t + y0;
    }

private:
    float at (int index) const noexcept { return buffer[(size_t) (index & mask)]; }

    std::vector<float> buffer;
    int mask = 0, writeIndex = 0;
    float maxDelay = 1.0f;
};

} // namespace trichorus::dsp
