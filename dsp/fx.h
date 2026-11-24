/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The effects after the voices, stereo, at the host's rate: the FX unit (flanger, phaser, ring mod, chorus, ensemble,
 * drive), the delay (ping pong, analogue mode), the reverb (room, two stages, two halls). */
#pragma once
#include "dsp.h"

namespace nq
{
class FxUnit
{
public:
    void prepare(float sampleRate);
    void clear();
    /* in place; type 0..5; amt 0..1 (0: off); on: the unit's switch */
    void process(float *l, float *r, int n, int type, float amt, bool on);

private:
    float sr = 44100.0f;
    int cur = -1;
    float gain = 0.0f;                   /* the unit's level (fades on switching) */
    Delay dl[2];
    float lfoA = 0.0f, lfoB = 0.0f, lfoC = 0.0f, ringPh = 0.0f;
    float fb[2] = {}, ap[2][6] = {}, phFb[2] = {};
    float dcx[2] = {}, dcy[2] = {};
    float amtS = 0.0f;
};

class DelayUnit
{
public:
    void prepare(float sampleRate);
    void clear();
    /* in place; seconds (the target), feedback level 0..3, mix 0..1 */
    void process(float *l, float *r, int n, float seconds, int fbLevel, bool ping, float mix, bool analog, bool on);

private:
    float sr = 44100.0f;
    Delay line[2];
    float cur = 0.3f, old = 0.3f, xf = 1.0f;   /* the read time, the one faded from, the crossfade */
    float target = 0.3f;
    float lp[2] = {}, hp[2] = {}, wet = 0.0f, mixS = 0.0f;
};

class ReverbUnit
{
public:
    void prepare(float sampleRate);
    void clear();
    void process(float *l, float *r, int n, int type, float mix, bool on);

private:
    static constexpr int NL = 8;
    float sr = 44100.0f;
    int cur = -1;
    Delay line[NL], pre;
    Allpass diff[2][3];
    float len[NL] = {}, g[NL] = {}, damp[NL] = {}, ph[NL] = {};
    float dampA = 0.3f, size = 1.0f, preS = 0.0f, wet = 0.0f, mixS = 0.0f;
    void setType(int t);
};
} // namespace nq
