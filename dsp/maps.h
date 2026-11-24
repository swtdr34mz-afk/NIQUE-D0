/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* What each knob's 0..1 means: shared by the engine and the value texts, so the panel shows what sounds. */
#pragma once
#include <cmath>

namespace nq
{
namespace map
{
inline float ex(float base, float v) { return std::pow(base, v); }
/* the envelopes (seconds); a decay at its end is a sustain */
inline bool isSustain(float v) { return v >= 0.995f; }
inline float ampAttack(float v) { return 0.0005f * ex(90000.0f, v); }      /* 0.5 ms .. 45 s */
inline float ampDecay(float v) { return 0.0005f * ex(90000.0f, v); }       /* 0.5 ms .. 45 s, then SUST */
inline float release(float v) { return 0.003f * ex(15000.0f, v); }         /* 3 ms .. 45 s */
inline float modAttack(float v) { return 0.00045f * ex(100000.0f, v); }    /* 0.45 ms .. 45 s */
inline float modDecay(float v) { return 0.003f * ex(15000.0f, v); }        /* 3 ms .. 45 s, then SUST */
inline float filterHz(float v) { return 14.0f * ex(2500.0f, v); }          /* 14 Hz .. 35 kHz */
inline float lfoHz(float v) { return 0.03f * ex(17433.0f, v); }            /* 0.03 .. 523 Hz */
inline float lfoEnvTime(float v) { return 1.0f / lfoHz(v); }               /* the envelope mode's time */
inline float arpBpm(float v) { return 16.0f * ex(64.0f, v); }              /* 16 .. 1024 (eighths) */
inline float delayMs(float v) { return 20.0f * ex(75.0f, v); }             /* 20 .. 1500 ms */
inline float glide(float v) { return v * v * 2.0f; }                       /* 0 .. 2 s */
inline float vibHz(float v) { return 2.0f + 8.0f * v; }                    /* 2 .. 10 Hz */
inline float vibSemis(float v) { return v * v; }                           /* 0 .. 1 semitone */
inline float level(float v) { return v * v * 2.0f; }                       /* 0.7: about 0 dB */
inline float bip(float v) { return (v - 0.5f) * 2.0f; }
inline float pitchSemis(float v) { return (v - 0.5f) * 48.0f; }            /* PITCH: +-24 semitones */
/* DETUNE: a few cents first, then semitone steps up to 60 */
inline float detuneSemis(float v)
{
    if (v < 0.4f) {
        float t = v / 0.4f;
        return t * t * 0.5f;             /* 0 .. 50 cents */
    }
    return std::round(1.0f + (v - 0.4f) / 0.6f * 59.0f);
}
inline float syncRatio(float v) { return std::pow(2.0f, v * 4.0f); }       /* 1 .. 16 times the note */
inline float amRatio(float v) { return 0.25f * std::pow(2.0f, v * 7.0f); } /* 0.25 .. 32 times the note */
inline float fmIndex(float v) { return v * v * 8.0f; }                     /* radians of phase */
inline float ringHz(float v) { return 0.5f * std::pow(10000.0f, v); }      /* the ring mod: 0.5 Hz .. 5 kHz */
inline float fxRate(float v) { return 0.03f * std::pow(200.0f, v); }       /* flanger, phaser: 0.03 .. 6 Hz */
} // namespace map
} // namespace nq
