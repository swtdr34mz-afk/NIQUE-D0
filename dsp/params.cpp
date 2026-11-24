/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The parameters' table (dsp/params_list.inc, written by tools/gen_params.py), their value texts, the morphs. */
#include "nique.h"
#include "maps.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace nq
{
#include "choices.inc"

#define CHOICES_0 nullptr, 0
#define P(e, id, name, shortName, kind, ch, def) {id, name, shortName, kind, PICK_##kind(ch), def},
#define PICK_K_CONT(ch) nullptr, 0
#define PICK_K_BIP(ch) nullptr, 0
#define PICK_K_SW(ch) nullptr, 2
#define PICK_K_CH(ch) ch, NCH(ch)
const ParamDef PARAMS[NPARAMS] = {
#include "params_list.inc"
};
#undef P

#define M(t, w, v) {t, w, v},
const MorphDef MORPHS[NMORPH] = {
#include "morph_list.inc"
};
#undef M

int paramIndex(const char *id)
{
    for (int i = 0; i < NPARAMS; i++)
        if (std::strcmp(PARAMS[i].id, id) == 0)
            return i;
    return -1;
}

float paramMax(int i)
{
    if (i < 0 || i >= NPARAMS)
        return 1.0f;
    return PARAMS[i].kind == K_CH ? (float)(PARAMS[i].nChoices - 1) : 1.0f;
}

int morphOf(int param)
{
    for (int m = 0; m < NMORPH; m++)
        if (MORPHS[m].target == param)
            return m;
    return -1;
}

static void secs(float s, char *out, int n)
{
    if (s < 0.1f)
        std::snprintf(out, (size_t)n, "%.1f ms", s * 1000.0f);
    else if (s < 1.0f)
        std::snprintf(out, (size_t)n, "%.0f ms", s * 1000.0f);
    else
        std::snprintf(out, (size_t)n, "%.2f s", s);
}

static void hz(float f, char *out, int n)
{
    if (f < 10.0f)
        std::snprintf(out, (size_t)n, "%.2f Hz", f);
    else if (f < 1000.0f)
        std::snprintf(out, (size_t)n, "%.0f Hz", f);
    else
        std::snprintf(out, (size_t)n, "%.1f kHz", f * 0.001f);
}

void paramText(int i, float v, char *out, int n)
{
    if (n <= 0)
        return;
    out[0] = 0;
    if (i < 0 || i >= NPARAMS)
        return;
    const ParamDef &d = PARAMS[i];
    if (d.kind == K_CH) {
        int k = (int)std::lround(v);
        k = k < 0 ? 0 : k >= d.nChoices ? d.nChoices - 1 : k;
        std::snprintf(out, (size_t)n, "%s", d.choices[k]);
        return;
    }
    if (d.kind == K_SW) {
        std::snprintf(out, (size_t)n, "%s", v >= 0.5f ? "ON" : "OFF");
        return;
    }
    if (d.kind == K_BIP) {
        if (i == OUT_PAN) {
            int p = (int)std::lround((v - 0.5f) * 200.0f);
            if (p == 0)
                std::snprintf(out, (size_t)n, "CENTRE");
            else
                std::snprintf(out, (size_t)n, "%s %d", p < 0 ? "L" : "R", p < 0 ? -p : p);
        } else {                         /* a morph's reach: -10 .. +10 */
            float m = (v - 0.5f) * 20.0f;
            if (std::fabs(m) < 0.05f)
                std::snprintf(out, (size_t)n, "NONE");
            else
                std::snprintf(out, (size_t)n, "%+.1f", m);
        }
        return;
    }
    switch (i) {
    case OUT_LEVEL: {
        float g = map::level(v);
        if (g < 0.0001f)
            std::snprintf(out, (size_t)n, "-INF dB");
        else
            std::snprintf(out, (size_t)n, "%.1f dB", 20.0f * std::log10(g));
        return;
    }
    case ARP_RATE:
        std::snprintf(out, (size_t)n, "%.0f BPM", map::arpBpm(v));
        return;
    case GLIDE:
        if (v <= 0.0f)
            std::snprintf(out, (size_t)n, "OFF");
        else
            secs(map::glide(v), out, n);
        return;
    case VIB_RATE:
        hz(map::vibHz(v), out, n);
        return;
    case VIB_AMT:
        std::snprintf(out, (size_t)n, "%.0f cents", map::vibSemis(v) * 100.0f);
        return;
    case LFO_RATE:
        hz(map::lfoHz(v), out, n);
        return;
    case MENV_A:
        secs(map::modAttack(v), out, n);
        return;
    case MENV_D:
        if (map::isSustain(v))
            std::snprintf(out, (size_t)n, "SUST");
        else
            secs(map::modDecay(v), out, n);
        return;
    case MENV_R:
    case AENV_R:
        secs(map::release(v), out, n);
        return;
    case AENV_A:
        secs(map::ampAttack(v), out, n);
        return;
    case AENV_D:
        if (map::isSustain(v))
            std::snprintf(out, (size_t)n, "SUST");
        else
            secs(map::ampDecay(v), out, n);
        return;
    case FLT_FREQ:
        hz(map::filterHz(v), out, n);
        return;
    case DLY_TIME:
        std::snprintf(out, (size_t)n, "%.0f ms", map::delayMs(v));
        return;
    case DLY_MIX:
    case REV_MIX:
        std::snprintf(out, (size_t)n, "%.0f %% wet", v * 100.0f);
        return;
    case FLT_DRIVE:
    case FX_AMT:
        if (v <= 0.001f) {
            std::snprintf(out, (size_t)n, "OFF");
            return;
        }
        break;
    default:
        break;
    }
    std::snprintf(out, (size_t)n, "%.1f", v * 10.0f);   /* the panel's scale: 0 .. 10 */
}

/* ------------------------------------------------------------------------------- the waves' long names --- */
static const char *const WAVE_LONG[47] = {
    "PULSE", "SAW", "TRIANGLE", "SINE",
    "A1  SOFT SAW", "A2  SAW + SQUARE", "A3  STEPPED SAW", "A4  SKEWED TRIANGLE", "A5  HALF SINE",
    "A6  EXPONENTIAL SAW", "A7  EXPONENTIAL SQUARE",
    "P1  PULSE 25 %", "P2  PULSE 10 %", "P3  PULSE 4 %",
    "O1  SECOND", "O2  THIRD", "O3  JIMMY", "O4  BLUES", "O5  GOSPEL", "O6  CHURCH", "O7  SQUABBLE",
    "O8  FULL ORGAN", "O9  FULL ORGAN PLUS",
    "B1  METAL BARS", "B2  BARS + OVERTONES", "B3  CLAMPED BAR", "B4  METALLOPHONE", "B5  TUBULAR BELLS",
    "D1  SAW, RANDOM PHASE", "D2  SQUARE, RANDOM PHASE", "D3  EVEN HARMONICS", "D4  EVERY 3RD", "D5  EVERY 4TH",
    "D6  EVERY 5TH", "D7  TRIPLETS", "D8  ICE",
    "E1  E-PIANO 1", "E2  E-PIANO 2", "E3  CLAVI 1", "E4  CLAVI 2",
    "F1  VOICE AAA", "F2  VOICE EEE", "F3  VOICE OOO", "F4  VOICE YYY", "F5  VOICE AO", "F6  VOICE AE",
    "F7  VOICE OE"};

const char *waveLongName(int w) { return WAVE_LONG[w < 0 ? 0 : w > 46 ? 46 : w]; }

const char *configLongName(int c)
{
    static char buf[64][40];
    static bool made = false;
    if (!made) {                         /* (built once; the strings never change) */
        for (int k = 0; k < NCH(CH_CONFIG) && k < 64; k++) {
            const char *s = CH_CONFIG[k];
            if (std::strcmp(s, "PITCH") == 0)
                std::snprintf(buf[k], 40, "PITCH  (osc 1, ctrl: its pitch)");
            else if (std::strcmp(s, "SHAPE") == 0)
                std::snprintf(buf[k], 40, "SHAPE  (ctrl: the waveshape)");
            else if (std::strcmp(s, "SYNC") == 0)
                std::snprintf(buf[k], 40, "SYNC  (ctrl: the synced pitch)");
            else if (std::strcmp(s, "DETUNE") == 0)
                std::snprintf(buf[k], 40, "DETUNE  (2 oscs, ctrl: detune)");
            else if (std::strcmp(s, "AM") == 0)
                std::snprintf(buf[k], 40, "AM  (ctrl: osc 2's pitch)");
            else if (std::strcmp(s, "NOISE") == 0)
                std::snprintf(buf[k], 40, "NOISE  (ctrl: the noise)");
            else if (s[0] == 'F' && s[1] == 'M')
                std::snprintf(buf[k], 40, "%s  (ctrl: FM amount)", s);
            else
                std::snprintf(buf[k], 40, "%s  (ctrl: osc 2's level)", s);
        }
        made = true;
    }
    int n = NCH(CH_CONFIG);
    return buf[c < 0 ? 0 : c >= n ? n - 1 : c];
}
} // namespace nq
