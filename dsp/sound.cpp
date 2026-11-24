/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The sound's own tools: INIT SOUND, MUTATE (a near relative: most parameters nudged, now and then a waveform, a
 * configuration or a filter changed), RANDOMIZE (anything at all, but playable). Both also make morphs: with the
 * wheel's (or velocity's) morph chosen they change only that morph. Left alone, as on the instrument: the output
 * level, the arpeggiator, the reverb, the voice mode. */
#include "nique.h"
#include "dsp.h"
#include <algorithm>
#include <cmath>

namespace nq
{
void initSound(float *v)
{
    for (int i = 0; i < NPARAMS; i++)
        v[i] = PARAMS[i].def;
    v[OSC_WAVE] = 1;                     /* saw */
    v[OSC_CONFIG] = 0;                   /* pitch */
    v[OSC_CTRL] = 0.5f;
    v[FLT_TYPE] = 1;                     /* LP24 */
    v[FLT_FREQ] = 0.86f;
    v[FLT_RES] = 0.0f;
    v[FLT_MENV] = 0.0f;
    v[AENV_A] = 0.0f;
    v[AENV_D] = 1.0f;
    v[AENV_R] = 0.25f;
}

namespace
{
/* the parameters the tools change */
const int SOUND[] = {LFO_RATE, LFO_WAVE, LFO_ENV, MENV_A, MENV_D, MENV_R, MENV_INV, OSC_WAVE, OSC_CONFIG, OSC_CTRL,
                     OSC_LFO, OSC_MENV, FLT_TYPE, FLT_FREQ, FLT_RES, FLT_LFO, FLT_MENV, FLT_KB, FLT_DRIVE, AENV_A,
                     AENV_D, AENV_R, FX_ON, FX_TYPE, FX_AMT, DLY_ON, DLY_TIME, DLY_FB, DLY_PING, DLY_MIX};

/* the waveforms in families (a mutation stays in one now and then) */
int family(int w)
{
    return w < 4 ? 0 : w < 11 ? 1 : w < 14 ? 2 : w < 23 ? 3 : w < 28 ? 4 : w < 36 ? 5 : w < 40 ? 6 : 7;
}
const int FAMILY_START[9] = {0, 4, 11, 14, 23, 28, 36, 40, 47};

float gauss(Rng &r)
{
    return (r.uni() + r.uni() + r.uni() + r.uni() - 2.0f) * 0.866f;   /* about one deviation */
}

void morphs(float *v, Rng &r, MorphSel m, int count, float reach)
{
    /* clear the morph, then give it a few targets */
    for (int k = 0; k < NMORPH; k++) {
        int p = m == MS_WHEEL ? MORPHS[k].wheel : MORPHS[k].vel;
        if (p >= 0)
            v[p] = 0.5f;
    }
    for (int n = 0; n < count; n++) {
        int k = (int)(r.uni() * NMORPH) % NMORPH;
        const MorphDef &d = MORPHS[k];
        int p = m == MS_WHEEL ? d.wheel : d.vel;
        if (p < 0 || d.target == OUT_LEVEL || d.target == ARP_RATE || d.target == ARP_DIV)
            continue;
        float base = PARAMS[d.target].kind == K_CH ? v[d.target] / paramMax(d.target) : v[d.target];
        /* towards where there is room (and never shutting the sound: the filter and the decay only open, the
         * attack only quickens, the effects only come in) */
        float dir = base > 0.6f ? -1.0f : base < 0.4f ? 1.0f : (r.uni() < 0.5f ? -1.0f : 1.0f);
        if (d.target == FLT_FREQ || d.target == AENV_D || d.target == FX_AMT || d.target == DLY_MIX ||
            d.target == REV_MIX)
            dir = 1.0f;
        else if (d.target == AENV_A)
            dir = -1.0f;
        float amount = reach * (0.4f + 0.6f * r.uni());
        v[p] = clampf(0.5f + 0.5f * dir * amount, 0.0f, 1.0f);
        if (m == MS_WHEEL) {             /* an effect off: on, from nothing, the wheel bringing it in */
            if (d.target == FX_AMT && v[FX_ON] < 0.5f) {
                v[FX_ON] = 1.0f;
                v[FX_AMT] = 0.0f;
                v[p] = 0.5f + 0.5f * amount;
            }
            if (d.target == DLY_MIX && v[DLY_ON] < 0.5f) {
                v[DLY_ON] = 1.0f;
                v[DLY_MIX] = 0.0f;
                v[p] = 0.5f + 0.4f * amount;
            }
        }
    }
}
} // namespace

void mutate(float *v, uint32_t seed, MorphSel m)
{
    Rng r;
    r.seed(seed * 2654435761u + 17u);
    if (m != MS_NONE) {
        morphs(v, r, m, 3 + (int)(r.uni() * 3.0f), 0.55f);
        return;
    }
    for (int p : SOUND) {
        const ParamDef &d = PARAMS[p];
        if (d.kind == K_CONT || d.kind == K_BIP) {
            float spread = (p == FLT_FREQ || p == OSC_CTRL) ? 0.10f : 0.08f;
            if (r.uni() < 0.75f)
                v[p] = clampf(v[p] + gauss(r) * spread, 0.0f, 1.0f);
        } else if (d.kind == K_SW) {
            if (r.uni() < 0.06f && p != DLY_ON && p != FX_ON)
                v[p] = v[p] >= 0.5f ? 0.0f : 1.0f;
        } else if (p == OSC_WAVE) {
            if (r.uni() < 0.2f) {        /* another of its family */
                int f = family((int)v[p]);
                int n = FAMILY_START[f + 1] - FAMILY_START[f];
                v[p] = (float)(FAMILY_START[f] + (int)(r.uni() * (float)n) % n);
            }
        } else if (p == OSC_CONFIG) {
            if (r.uni() < 0.15f) {
                int c = (int)v[p];
                if (c >= 4 && c < 40)     /* a mix: another harmonic, or another wave on it */
                    c = 4 + ((c - 4) / 9) * 9 + (int)(r.uni() * 9.0f) % 9;
                else if (c >= 40 && c < 53)
                    c = 40 + (int)(r.uni() * 13.0f) % 13;
                else
                    c = (int)(r.uni() * 4.0f) % 4;
                v[p] = (float)c;
            }
        } else if (r.uni() < 0.08f)
            v[p] = (float)((int)(r.uni() * (float)d.nChoices) % d.nChoices);
    }
    /* still a sound */
    if (v[FLT_FREQ] < 0.25f && v[FLT_MENV] < 0.2f)
        v[FLT_FREQ] = 0.25f + 0.1f * r.uni();
    if (v[FLT_RES] > 0.9f)
        v[FLT_RES] = 0.9f;
    if (v[FLT_TYPE] >= 5.0f && v[FLT_FREQ] + v[FLT_MENV] * 0.35f > 0.7f) {   /* HP: not above everything */
        v[FLT_FREQ] = std::min(v[FLT_FREQ], 0.55f);
        v[FLT_MENV] = std::min(v[FLT_MENV], 0.3f);
    }
}

void randomize(float *v, uint32_t seed, MorphSel m)
{
    Rng r;
    r.seed(seed * 2246822519u + 3u);
    if (m != MS_NONE) {
        morphs(v, r, m, 5 + (int)(r.uni() * 6.0f), 0.9f);
        return;
    }
    for (int p : SOUND) {
        const ParamDef &d = PARAMS[p];
        if (d.kind == K_CH)
            v[p] = (float)((int)(r.uni() * (float)d.nChoices) % d.nChoices);
        else if (d.kind == K_SW)
            v[p] = r.uni() < 0.35f ? 1.0f : 0.0f;
        else
            v[p] = r.uni();
    }
    /* playable: mostly quick attacks, a filter that lets something through, resonance and drive within reason */
    v[AENV_A] = r.uni() < 0.7f ? 0.3f * r.uni() : v[AENV_A];
    v[AENV_D] = r.uni() < 0.5f ? 1.0f : 0.35f + 0.6f * v[AENV_D];
    v[AENV_R] = 0.15f + 0.6f * v[AENV_R];
    v[FLT_FREQ] = 0.35f + 0.6f * v[FLT_FREQ];
    v[FLT_RES] *= 0.85f;
    v[FLT_DRIVE] *= 0.6f;
    v[OSC_LFO] *= 0.7f;
    v[FX_ON] = r.uni() < 0.4f ? 1.0f : 0.0f;
    v[DLY_ON] = r.uni() < 0.3f ? 1.0f : 0.0f;
    v[DLY_MIX] *= 0.5f;
    if (v[FLT_TYPE] >= 5.0f) {           /* HP: room to be heard */
        v[FLT_FREQ] = 0.2f + 0.35f * v[FLT_FREQ];
        v[FLT_MENV] *= 0.3f;
    } else if (v[FLT_TYPE] >= 4.0f)      /* BP: in the middle */
        v[FLT_FREQ] = 0.4f + 0.3f * v[FLT_FREQ];
}
} // namespace nq
