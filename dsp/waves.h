/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The 47 waveforms as band-limited wavetables: one cycle of 2048 samples, in ten versions each with fewer harmonics
 * (1023, 511, ... 1), so a note reads the one that does not alias. Built once (all instances share them). */
#pragma once
#include "dsp.h"

namespace nq
{
constexpr int WT_N = 2048, WT_MASK = WT_N - 1, WT_LEVELS = 10, NWAVES = 47;

struct Wave {
    float freqMul = 1.0f;                /* the table's cycle against the note (organs 1/2, bells 1/8) */
    const float *level[WT_LEVELS] = {};  /* WT_N + 1 samples each (the last repeats the first) */
};
const Wave &wave(int w);                 /* (the first call builds them all: call it before the audio starts) */

/* the version to read for a cycle of f0 Hz at a rate of sr (fractional: two are mixed) */
inline float waveLevel(float f0, float sr)
{
    float cap = 0.36f * sr;              /* the highest harmonic wanted */
    float x = std::log2(1024.0f * (f0 > 1.0f ? f0 : 1.0f) / cap);
    return clampf(x, 0.0f, (float)(WT_LEVELS - 1) - 0.001f);
}

/* a reader: two levels mixed, phase 0..1 (any: wrapped) */
struct WaveReader {
    const float *a = nullptr, *b = nullptr;
    float mix = 0.0f;
    void set(const Wave &w, float lev)
    {
        int l = (int)lev;
        a = w.level[l];
        b = w.level[l + 1 < WT_LEVELS ? l + 1 : l];
        mix = lev - (float)l;
    }
    float operator()(float ph) const
    {
        float x = (ph - std::floor(ph)) * (float)WT_N;
        int i = (int)x;
        float f = x - (float)i;
        i &= WT_MASK;
        float va = a[i] + f * (a[i + 1] - a[i]);
        float vb = b[i] + f * (b[i + 1] - b[i]);
        return va + mix * (vb - va);
    }
};
} // namespace nq
