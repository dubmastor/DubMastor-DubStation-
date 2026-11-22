#pragma once

#include <juce_core/juce_core.h>
#include <algorithm>
#include <array>

//==============================================================================
// FIFO lock-free: áudio thread escreve, GUI (osciloscópio) lê.
//==============================================================================
struct ScopeFifo
{
    static constexpr int size = 16384;

    void push (const float* src, int n)
    {
        const auto w = fifo.write (n);
        if (w.blockSize1 > 0) std::copy (src, src + w.blockSize1, data.begin() + w.startIndex1);
        if (w.blockSize2 > 0) std::copy (src + w.blockSize1, src + w.blockSize1 + w.blockSize2, data.begin() + w.startIndex2);
    }

    int pull (float* dest, int maxN)
    {
        const auto r = fifo.read (maxN);
        if (r.blockSize1 > 0) std::copy (data.begin() + r.startIndex1, data.begin() + r.startIndex1 + r.blockSize1, dest);
        if (r.blockSize2 > 0) std::copy (data.begin() + r.startIndex2, data.begin() + r.startIndex2 + r.blockSize2, dest + r.blockSize1);
        return r.blockSize1 + r.blockSize2;
    }

private:
    juce::AbstractFifo fifo { size };
    std::array<float, size> data {};
};
