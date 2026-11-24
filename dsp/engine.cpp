/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The engine: the keys (hold, sustain, the arpeggiator, poly / mono / legato, unison), the voices (oscillators,
 * filter, envelopes) at twice the host's rate, then down to it, then the effects. */
#include "nique.h"
#include "fx.h"
#include "maps.h"
#include "waves.h"
#include <algorithm>
#include <atomic>

namespace nq
{
namespace
{
constexpr int NV = 24;                   /* the voices */
constexpr int BLOCK = 16;                /* host samples a control step */
constexpr int MAXOS = 2;

enum Stage { ST_OFF, ST_ATT, ST_DEC, ST_SUS, ST_REL };

/* an ADR / ASR envelope: a linear attack, then an exponential decay to nothing (or held: SUST), the release */
struct Env {
    int st = ST_OFF;
    float lev = 0.0f, attInc = 1.0f, decK = 0.999f, relK = 0.99f;
    bool sus = false, freeAtZero = false;
    void on() { st = ST_ATT; }
    void off()
    {
        if (st != ST_OFF)
            st = ST_REL;
    }
    float tick()
    {
        switch (st) {
        case ST_ATT:
            lev += attInc;
            if (lev >= 1.0f) {
                lev = 1.0f;
                st = sus ? ST_SUS : ST_DEC;
            }
            break;
        case ST_DEC:
            if (sus) {
                st = ST_SUS;
                break;
            }
            lev *= decK;
            if (freeAtZero && lev < 1e-5f) {
                lev = 0.0f;
                st = ST_OFF;
            }
            break;
        case ST_SUS:
            if (!sus)
                st = ST_DEC;
            break;
        case ST_REL:
            lev *= relK;
            if (lev < 1e-5f) {
                lev = 0.0f;
                st = ST_OFF;
            }
            break;
        default:
            break;
        }
        return lev;
    }
};

inline float frac(float x) { return x - std::floor(x); }
/* tan(x) for 0 <= x <= 1.45 (the filters' prewarp) */
inline float ftan(float x)
{
    float x2 = x * x;
    return x * (15.0f - x2) / (15.0f - 6.0f * x2);
}
/* a triangle fold: the same for |x| <= 1, reflected beyond */
inline float fold(float x)
{
    x = (x + 1.0f) * 0.25f;
    x -= std::floor(x);
    return x < 0.5f ? 4.0f * x - 1.0f : 3.0f - 4.0f * x;
}

struct Voice {
    bool on = false, gate = false;       /* sounding; the key (or its stand-in) down */
    int note = 60, group = 0;
    float vel = 0.8f;
    uint32_t age = 0;
    float pitch = 60.0f, target = 60.0f; /* semitones (glide) */
    float detune = 0.0f, gl = 1.0f, gr = 1.0f;   /* unison: its detune, its pan gains */
    float ph1 = 0.0f, ph2 = 0.0f, ph3 = 0.0f, pm = 0.0f;
    float inc1 = 0.0f;                   /* (the last block's, for the ramp) */
    float cutLog = 10.0f;                /* log2 of the cutoff, the last block's */
    Env amp, mod;
    float lfoT = 0.0f, lfoSH = 0.0f;     /* the LFO's envelope mode: time since the note, its random value */
    float age_s = 0.0f;                  /* seconds since the note (delayed vibrato) */
    float s[4] = {}, a1 = 0.0f, a2 = 0.0f, b1 = 0.0f, b2 = 0.0f, hpz = 0.0f;   /* the filters' states */
    Rng rng;
};

/* 2x down: a half-band FIR (55 taps, Blackman-Harris) */
struct Decimator {
    static constexpr int N = 55, C = 27;
    float h[N] = {};
    float x[64] = {};
    int w = 0;
    void design()
    {
        double sum = 0.0;
        for (int k = 0; k < N; k++) {
            double m = k - C;
            double sinc = m == 0 ? 1.0 : std::sin(3.14159265358979 * 0.5 * m) / (3.14159265358979 * 0.5 * m);
            double t = 2.0 * 3.14159265358979 * k / (N - 1);
            double win = 0.35875 - 0.48829 * std::cos(t) + 0.14128 * std::cos(2 * t) - 0.01168 * std::cos(3 * t);
            h[k] = (float)(sinc * win);
            sum += h[k];
        }
        for (float &c : h)
            c = (float)(c / sum);
    }
    void clear() { std::fill(std::begin(x), std::end(x), 0.0f); }
    float run(float a, float b)          /* two in, one out */
    {
        x[w] = a;
        w = (w + 1) & 63;
        x[w] = b;
        w = (w + 1) & 63;
        float y = 0.0f;
        int r = w - 1;
        for (int k = 0; k < N; k++)
            y += h[k] * x[(r - k) & 63];
        return y;
    }
};

const float ARP_DIV_BEATS[9] = {2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 4.0f / 3.0f, 2.0f / 3.0f, 1.0f / 3.0f, 1.0f / 6.0f};
const float LFO_DIV_BEATS[17] = {16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f,
                                 16.0f * 2 / 3, 8.0f * 2 / 3, 4.0f * 2 / 3, 2.0f * 2 / 3, 2.0f / 3, 1.0f / 3, 0.5f / 3,
                                 0.25f / 3};
const float DLY_DIV_BEATS[17] = {2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f, 1.5f, 0.75f, 0.375f,
                                 4.0f / 3, 2.0f / 3, 1.0f / 3, 0.5f / 3, 0.25f / 3, 1.25f, 0.625f, 0.3125f};
} // namespace

struct Impl {
    float val[NPARAMS] = {};
    int morph[NPARAMS] = {};             /* the morph of each parameter, -1 none */
    double sr = 44100.0;
    int os = 2;
    float fsI = 88200.0f;                /* the voices' rate */

    Voice v[NV];
    uint32_t ageCount = 0;
    int groupCount = 0;

    /* the keys */
    bool down[128] = {}, latched[128] = {};
    float keyVel[128] = {};
    int downCount = 0;
    bool sustainDown = false;
    int stack[128] = {}, stackN = 0;     /* mono / legato: the notes held, the last on top */
    float lastVel = 0.8f;

    /* the arpeggiator */
    int arpNotes[128] = {}, arpN = 0;
    int arpPos = -1, arpNote = -1, arpDir = 1;
    double arpPhase = 1.0, arpLastIdx = -1.0;
    bool arpGate = false;

    /* the controllers and the clock */
    float wheelT = 0.0f, wheelS = 0.0f, bendV = 0.0f;
    double bpm = 120.0, ppqBase = 0.0;
    bool playing = false;
    int64_t sinceBase = 0;

    /* the LFO, the vibrato */
    float lfoPh = 0.0f, lfoSH = 0.5f, vibPh = 0.0f;
    float lfoU[BLOCK * MAXOS] = {}, lfoB[BLOCK * MAXOS] = {};
    Rng rng;

    /* the buffers */
    float osL[BLOCK * MAXOS] = {}, osR[BLOCK * MAXOS] = {};
    float outL[BLOCK] = {}, outR[BLOCK] = {};
    int outPos = BLOCK;
    Decimator dec[2];
    FxUnit fx;
    DelayUnit dly;
    ReverbUnit rev;

    std::atomic<float> mLfo{0.0f}, mOut{0.0f};
    std::atomic<int> mVoices{0};

    Impl()
    {
        for (int i = 0; i < NPARAMS; i++) {
            val[i] = PARAMS[i].def;
            morph[i] = morphOf(i);
        }
        rng.seed(0x5eed1u);
        for (int i = 0; i < NV; i++)
            v[i].rng.seed(0x1000u + 77u * (uint32_t)i);
    }

    int ch(int p) const { return (int)std::lround(val[p]); }
    bool sw(int p) const { return val[p] >= 0.5f; }

    /* a parameter as the wheel and a velocity take it (normalised for a choice: its index back) */
    float eff(int p, float vel) const
    {
        int m = morph[p];
        if (m < 0)
            return val[p];
        const ParamDef &d = PARAMS[p];
        float span = d.kind == K_CH ? (float)(d.nChoices - 1) : 1.0f;
        float x = val[p] / span;
        x += (val[MORPHS[m].wheel] - 0.5f) * 2.0f * wheelS;
        if (MORPHS[m].vel >= 0)
            x += (val[MORPHS[m].vel] - 0.5f) * 2.0f * vel;
        x = clampf(x, 0.0f, 1.0f);
        return d.kind == K_CH ? std::round(x * span) : x;
    }

    int unisonCount() const
    {
        static const int U[4] = {1, 2, 4, 4};
        return U[std::min(3, std::max(0, ch(UNISON)))];
    }

    /* ------------------------------------------------------------------------------------------- voices --- */
    void startVoice(Voice &x, int note, float vel, int group, int uIndex, int uCount, bool retrigger, bool glide)
    {
        static const float DET[4][4] = {{0, 0, 0, 0}, {-0.08f, 0.08f, 0, 0}, {-0.12f, -0.05f, 0.05f, 0.12f},
                                        {-0.25f, -0.12f, 0.12f, 0.25f}};
        static const float PAN[4][4] = {{0, 0, 0, 0}, {-0.6f, 0.6f, 0, 0}, {-0.8f, -0.3f, 0.3f, 0.8f},
                                        {-1.0f, -0.4f, 0.4f, 1.0f}};
        int u = std::min(3, std::max(0, ch(UNISON)));
        bool fresh = !x.on;
        x.detune = uCount > 1 ? DET[u][uIndex] : 0.0f;
        float p = uCount > 1 ? PAN[u][uIndex] : 0.0f;
        float g = 1.0f / std::sqrt((float)uCount);
        x.gl = g * std::cos((p + 1.0f) * 0.25f * PI) * 1.41421356f;
        x.gr = g * std::sin((p + 1.0f) * 0.25f * PI) * 1.41421356f;
        x.note = note;
        x.vel = vel;
        x.group = group;
        x.gate = true;
        x.age = ++ageCount;
        x.target = (float)note;
        if (fresh || !glide)
            x.pitch = x.target;
        if (fresh) {
            x.ph1 = uCount > 1 ? x.rng.uni() : 0.0f;
            x.ph2 = x.rng.uni();
            x.ph3 = 0.0f;
            x.pm = 0.0f;
            x.inc1 = -1.0f;
            x.cutLog = -1.0f;
            for (float &s : x.s)
                s = 0.0f;
            x.a1 = x.a2 = x.b1 = x.b2 = x.hpz = 0.0f;
            x.amp.lev = x.mod.lev = 0.0f;
        }
        if (retrigger || fresh) {
            x.amp.on();
            x.mod.on();
            x.lfoT = 0.0f;
            x.lfoSH = x.rng.uni();
            x.age_s = 0.0f;
        }
        x.on = true;
    }

    Voice *freeVoice()
    {
        Voice *best = nullptr;
        for (auto &x : v)                /* a silent one */
            if (!x.on)
                return &x;
        for (auto &x : v)                /* the oldest released */
            if (!x.gate && (!best || x.age < best->age))
                best = &x;
        if (best)
            return best;
        for (auto &x : v)                /* the oldest */
            if (!best || x.age < best->age)
                best = &x;
        return best;
    }

    bool mono() const { return ch(VOICE_MODE) != 0; }

    void playOn(int note, float vel)
    {
        int n = unisonCount();
        if (!mono()) {
            int grp = ++groupCount;
            for (int k = 0; k < n; k++)
                startVoice(*freeVoice(), note, vel, grp, k, n, true, false);
            return;
        }
        bool legato = ch(VOICE_MODE) == 2, had = stackN > 0;
        removeStack(note);
        stack[stackN++] = note;
        bool retrig = !(legato && had);
        bool glide = legato ? had : true;
        for (int k = 0; k < n; k++)
            startVoice(v[k], note, vel, 0, k, n, retrig, glide);
        for (int k = n; k < NV; k++)     /* (unison changed: the others go) */
            if (v[k].on && v[k].gate) {
                v[k].gate = false;
                v[k].amp.off();
                v[k].mod.off();
            }
    }

    void removeStack(int note)
    {
        int j = 0;
        for (int i = 0; i < stackN; i++)
            if (stack[i] != note)
                stack[j++] = stack[i];
        stackN = j;
    }

    void release(Voice &x)
    {
        x.gate = false;
        x.amp.off();
        x.mod.off();
    }

    void playOff(int note)
    {
        if (!mono()) {
            for (auto &x : v)
                if (x.on && x.gate && x.note == note)
                    release(x);
            return;
        }
        bool top = stackN > 0 && stack[stackN - 1] == note;
        removeStack(note);
        int n = unisonCount();
        if (stackN == 0) {
            for (auto &x : v)
                if (x.on && x.gate)
                    release(x);
        } else if (top) {                /* back to the note still held */
            int back = stack[stackN - 1];
            bool legato = ch(VOICE_MODE) == 2;
            for (int k = 0; k < n; k++)
                startVoice(v[k], back, keyVel[back], 0, k, n, !legato, true);
        }
    }

    void releaseAllVoices()
    {
        for (auto &x : v)
            if (x.on)
                release(x);
        stackN = 0;
    }

    /* ---------------------------------------------------------------------------------------- the keys --- */
    bool arpOn() const { return sw(ARP_RUN); }

    void arpAdd(int note)
    {
        for (int i = 0; i < arpN; i++)
            if (arpNotes[i] == note)
                return;
        int i = arpN++;
        while (i > 0 && arpNotes[i - 1] > note) {
            arpNotes[i] = arpNotes[i - 1];
            i--;
        }
        arpNotes[i] = note;
        if (arpN == 1 && !(sw(ARP_SYNC) && playing)) {
            arpPhase = 1.0;              /* the first note sounds at once */
            arpPos = -1;
            arpDir = 1;
        }
    }

    void arpRemove(int note)
    {
        int j = 0;
        for (int i = 0; i < arpN; i++)
            if (arpNotes[i] != note)
                arpNotes[j++] = arpNotes[i];
        arpN = j;
    }

    void logicalOn(int note)
    {
        if (arpOn())
            arpAdd(note);
        else
            playOn(note, keyVel[note]);
    }

    void logicalOff(int note)
    {
        if (arpOn())
            arpRemove(note);
        else
            playOff(note);
    }

    void releaseLatched(bool keepDown)
    {
        for (int n = 0; n < 128; n++)
            if (latched[n] && !(keepDown && down[n])) {
                latched[n] = false;
                logicalOff(n);
            }
    }

    void noteOn(int note, float vel)
    {
        if (note < 0 || note > 127)
            return;
        vel = clampf(vel, 0.0f, 1.0f);
        lastVel = vel;
        if (sw(HOLD) && downCount == 0)  /* HOLD: a new chord replaces the one held */
            releaseLatched(false);
        if (!down[note])
            downCount++;
        down[note] = true;
        keyVel[note] = vel;
        if (latched[note] && !arpOn())
            playOff(note);
        latched[note] = true;
        logicalOn(note);
    }

    void noteOff(int note)
    {
        if (note < 0 || note > 127 || !down[note])
            return;
        down[note] = false;
        downCount = std::max(0, downCount - 1);
        if (sw(HOLD) || sustainDown)
            return;
        latched[note] = false;
        logicalOff(note);
    }

    void allNotesOff()
    {
        for (int n = 0; n < 128; n++)
            down[n] = latched[n] = false;
        downCount = 0;
        arpN = 0;
        arpNote = -1;
        releaseAllVoices();
    }

    void reroute()                       /* the arpeggiator on or off: the notes held go the new way */
    {
        releaseAllVoices();
        arpN = 0;
        arpNote = -1;
        arpPos = -1;
        arpPhase = 1.0;
        for (int n = 0; n < 128; n++)
            if (latched[n])
                logicalOn(n);
    }

    /* -------------------------------------------------------------------------------- the arpeggiator --- */
    void arpStep()
    {
        int range = ch(ARP_RANGE) + 1, total = arpN * range;
        if (total <= 0)
            return;
        switch (ch(ARP_DIR)) {
        case 0:
            arpPos = (arpPos + 1) % total;
            break;
        case 1:
            arpPos = arpPos <= 0 ? total - 1 : arpPos - 1;
            break;
        case 2:
            if (total == 1)
                arpPos = 0;
            else {
                if (arpPos < 0) {
                    arpPos = 0;
                    arpDir = 1;
                } else {
                    if (arpPos + arpDir >= total || arpPos + arpDir < 0)
                        arpDir = -arpDir;
                    arpPos += arpDir;
                }
            }
            break;
        default:
            arpPos = (int)(rng.uni() * (float)total) % total;
            break;
        }
        arpPos = std::min(std::max(arpPos, 0), total - 1);
        int note = arpNotes[arpPos % arpN] + 12 * (arpPos / arpN);
        if (note > 127)
            note -= 12 * ((note - 116) / 12 + 1);
        if (arpNote >= 0)
            playOff(arpNote);
        arpNote = note;
        int src = arpNotes[arpPos % arpN];
        playOn(note, keyVel[src] > 0.0f ? keyVel[src] : lastVel);
        arpGate = true;
    }

    void arpRun(double ppq, float dt)
    {
        if (!arpOn() || arpN == 0) {
            if (arpNote >= 0) {
                playOff(arpNote);
                arpNote = -1;
            }
            arpPos = -1;
            return;
        }
        bool sync = sw(ARP_SYNC);
        if (sync && playing) {
            double len = ARP_DIV_BEATS[std::min(8, std::max(0, (int)eff(ARP_DIV, lastVel)))];
            double idx = std::floor(ppq / len);
            double ph = ppq / len - idx;
            if (idx != arpLastIdx) {
                arpLastIdx = idx;
                arpStep();
            } else if (arpGate && ph >= 0.5) {
                arpGate = false;
                if (arpNote >= 0)
                    playOff(arpNote);
                arpNote = -1;
            }
            return;
        }
        arpLastIdx = -1.0;
        double stepSec = sync ? ARP_DIV_BEATS[std::min(8, std::max(0, (int)eff(ARP_DIV, lastVel)))] * 60.0 / bpm
                              : 30.0 / map::arpBpm(eff(ARP_RATE, lastVel));
        arpPhase += dt / stepSec;
        if (arpPhase >= 1.0) {
            arpPhase -= std::floor(arpPhase);
            arpStep();
        } else if (arpGate && arpPhase >= 0.5) {
            arpGate = false;
            if (arpNote >= 0)
                playOff(arpNote);
            arpNote = -1;
        }
    }

    /* -------------------------------------------------------------------------------- the control step --- */
    void block()
    {
        const int m = BLOCK * os;
        const float dtBlock = (float)BLOCK / (float)sr;
        wheelS += (wheelT - wheelS) * std::min(1.0f, dtBlock / 0.012f);
        double ppq = ppqBase + (double)sinceBase * bpm / (60.0 * sr);
        sinceBase += BLOCK;
        arpRun(ppq, dtBlock);

        /* the LFO, at the voices' rate */
        int lw = ch(LFO_WAVE);
        float lfoInc;
        bool lsync = sw(LFO_SYNC);
        if (lsync) {
            float beats = LFO_DIV_BEATS[std::min(16, std::max(0, (int)eff(LFO_DIV, lastVel)))];
            lfoInc = (float)(bpm / 60.0 / beats) / fsI;
            if (playing) {               /* on the host's bars */
                double p = ppq / beats;
                float want = (float)(p - std::floor(p));
                float d = want - lfoPh;
                if (std::fabs(d) > 0.02f && std::fabs(d) < 0.98f)
                    lfoPh = want;
            }
        } else
            lfoInc = map::lfoHz(eff(LFO_RATE, lastVel)) / fsI;
        for (int i = 0; i < m; i++) {
            lfoPh += lfoInc;
            if (lfoPh >= 1.0f) {
                lfoPh -= std::floor(lfoPh);
                lfoSH = rng.uni();
            }
            float u;
            switch (lw) {
            case 0: u = lfoPh < 0.5f ? 1.0f : 0.0f; break;
            case 1: u = lfoPh; break;
            case 2: u = 1.0f - lfoPh; break;
            case 3: u = lfoPh < 0.5f ? 2.0f * lfoPh : 2.0f - 2.0f * lfoPh; break;
            default: u = lfoSH; break;
            }
            lfoU[i] = u;
            lfoB[i] = 2.0f * u - 1.0f;
        }
        mLfo = lfoU[m - 1];

        vibPh += map::vibHz(val[VIB_RATE]) * dtBlock;
        vibPh -= std::floor(vibPh);

        std::fill(osL, osL + m, 0.0f);
        std::fill(osR, osR + m, 0.0f);
        int sounding = 0;
        for (auto &x : v)
            if (x.on) {
                renderVoice(x, m, dtBlock);
                sounding++;
            }
        mVoices = sounding;

        float bl[BLOCK], br[BLOCK];
        if (os == 2)
            for (int i = 0; i < BLOCK; i++) {
                bl[i] = dec[0].run(osL[2 * i], osL[2 * i + 1]);
                br[i] = dec[1].run(osR[2 * i], osR[2 * i + 1]);
            }
        else
            for (int i = 0; i < BLOCK; i++) {
                bl[i] = osL[i];
                br[i] = osR[i];
            }

        fx.process(bl, br, BLOCK, ch(FX_TYPE), eff(FX_AMT, 0.0f), sw(FX_ON));
        float pan = val[OUT_PAN];        /* (the balance; the level is the voices') */
        float pl = std::min(1.0f, 2.0f * (1.0f - pan)), pr = std::min(1.0f, 2.0f * pan);
        for (int i = 0; i < BLOCK; i++) {
            bl[i] *= pl;
            br[i] *= pr;
        }
        float secs = sw(DLY_SYNC) ? DLY_DIV_BEATS[std::min(16, std::max(0, (int)eff(DLY_DIV, 0.0f)))] * 60.0f / (float)bpm
                                  : map::delayMs(eff(DLY_TIME, 0.0f)) * 0.001f;
        dly.process(bl, br, BLOCK, std::min(secs, 1.9f), ch(DLY_FB), sw(DLY_PING), eff(DLY_MIX, 0.0f), sw(DLY_ANALOG),
                    sw(DLY_ON));
        rev.process(bl, br, BLOCK, ch(REV_TYPE), eff(REV_MIX, 0.0f), sw(REV_ON));

        float peak = 0.0f;
        bool bad = false;
        for (int i = 0; i < BLOCK; i++) {
            if (!std::isfinite(bl[i]) || !std::isfinite(br[i]))
                bad = true;
            outL[i] = bl[i];
            outR[i] = br[i];
            peak = std::max(peak, std::max(std::fabs(bl[i]), std::fabs(br[i])));
        }
        if (bad) {                       /* (never: but a NaN would stay in the effects for good) */
            panic();
            std::fill(outL, outL + BLOCK, 0.0f);
            std::fill(outR, outR + BLOCK, 0.0f);
            peak = 0.0f;
        }
        mOut = peak;
    }

    void panic()
    {
        allNotesOff();
        for (auto &x : v) {
            x.on = false;
            x.amp.st = x.mod.st = ST_OFF;
            x.amp.lev = x.mod.lev = 0.0f;
        }
        dec[0].clear();
        dec[1].clear();
        fx.clear();
        dly.clear();
        rev.clear();
    }

    /* ----------------------------------------------------------------------------------------- a voice --- */
    void renderVoice(Voice &x, int m, float dtBlock)
    {
        const float vel = x.vel, fs = fsI;
        /* the envelopes' rates for this step */
        auto per = [fs](float sec) { return std::exp(-6.9078f / std::max(1.0f, sec * fs)); };
        float aA = eff(AENV_A, vel), aD = eff(AENV_D, vel), aR = eff(AENV_R, vel);
        x.amp.attInc = 1.0f / std::max(1.0f, map::ampAttack(aA) * fs);
        x.amp.sus = map::isSustain(aD);
        x.amp.decK = per(map::ampDecay(aD));
        x.amp.relK = per(map::release(aR));
        x.amp.freeAtZero = true;
        float mA = eff(MENV_A, vel), mD = eff(MENV_D, vel), mR = eff(MENV_R, vel);
        x.mod.attInc = 1.0f / std::max(1.0f, map::modAttack(mA) * fs);
        x.mod.sus = map::isSustain(mD);
        x.mod.decK = per(map::modDecay(mD));
        x.mod.relK = per(map::release(mR));

        /* the pitch: glide, bend, octave, unison, vibrato */
        float gt = map::glide(eff(GLIDE, vel));
        if (mono() && gt > 0.0005f)
            x.pitch += (x.target - x.pitch) * (1.0f - std::exp(-dtBlock * 4.0f / gt));
        else
            x.pitch = x.target;
        static const float BUP[17] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 24, 48, 2, 2};
        static const float BDN[17] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 24, 48, 12, 24};
        int bi = std::min(16, std::max(0, ch(BEND)));
        float bend = bendV >= 0.0f ? bendV * BUP[bi] : bendV * BDN[bi];
        float vib = 0.0f;
        int vm = ch(VIBRATO);
        x.age_s += dtBlock;
        if (vm > 0) {
            float depth = map::vibSemis(val[VIB_AMT]);
            if (vm == 3)
                depth *= wheelS;
            else {
                float delay = vm == 1 ? 0.5f : 1.0f;
                depth *= clampf((x.age_s - delay) / 0.4f, 0.0f, 1.0f);
            }
            vib = depth * fsin01(vibPh);
        }
        float semis = x.pitch + 12.0f * (float)(ch(OCTAVE) - 2) + bend + x.detune + vib;

        /* the oscillators */
        const int wv = std::min(46, std::max(0, ch(OSC_WAVE)));
        const int cfg = std::max(0, ch(OSC_CONFIG));
        const Wave &W = wave(wv);
        const float ctrl = eff(OSC_CTRL, vel);
        float oLfo = eff(OSC_LFO, vel), oEnv = eff(OSC_MENV, vel);
        const bool envVel = sw(MENV_VEL);
        const float envSign = (sw(MENV_INV) ? -1.0f : 1.0f) * (envVel ? vel : 1.0f);
        enum { C_PITCH, C_SHAPE, C_SYNC, C_DETUNE, C_MIX, C_FM, C_AM, C_NOISE };
        int kind, mixWave = 0;
        float mixH = 1.0f, fmRatio = 1.0f;
        static const float HARMS[9] = {0.5f, 1, 2, 3, 4, 5, 6, 7, 8};
        static const float FMR[13] = {0.5f, 1, 1.41f, 1.5f, 2, 2.76f, 3, 3.5f, 4, 5, 6, 7, 8};
        if (cfg < 4)
            kind = cfg;
        else if (cfg < 40) {
            kind = C_MIX;
            static const int MW[4] = {3, 2, 1, 0};   /* sine, triangle, saw, square (the pulse at 50 %) */
            mixWave = MW[(cfg - 4) / 9];
            mixH = HARMS[(cfg - 4) % 9];
        } else if (cfg < 53) {
            kind = C_FM;
            fmRatio = FMR[cfg - 40];
        } else
            kind = cfg == 53 ? C_AM : C_NOISE;
        /* the LFO's polarity: bipolar for pitch, and for the pulse's width, with the triangle or S/H */
        const bool lfoBip = (kind == C_PITCH || (kind == C_SHAPE && wv == 0)) && (ch(LFO_WAVE) >= 3);
        const bool lfoEnvMode = sw(LFO_ENV);
        const float lfoTime = map::lfoEnvTime(eff(LFO_RATE, vel));

        float f0 = 440.0f * fexp2((semis - 69.0f) / 12.0f);
        /* the tables' level for this step (the modulations as they stand at its start) */
        float menv0 = x.mod.lev * envSign;
        float ctrl0 = clampf(ctrl + oEnv * menv0, 0.0f, 1.0f);
        float fTab = f0 * W.freqMul;
        WaveReader r1, r2, rs;
        float levelF = fTab;
        if (kind == C_SHAPE)
            levelF *= 1.0f + 4.0f * ctrl0;
        else if (kind == C_SYNC)
            levelF *= map::syncRatio(ctrl0);
        else if (kind == C_FM) {         /* room left under the top for the sidebands */
            float cap = 0.36f * fs, side = map::fmIndex(ctrl0) * fmRatio * f0;
            levelF *= cap / std::max(cap - side, 0.25f * cap);
        }
        else if (kind == C_PITCH)
            levelF *= fexp2((map::pitchSemis(ctrl0) + 48.0f * oEnv * oEnv * std::max(0.0f, menv0)) / 12.0f);
        r1.set(W, waveLevel(levelF, fs));
        if (kind == C_SHAPE && wv == 0)
            rs.set(wave(1), waveLevel(f0, fs));
        if (kind == C_MIX)
            r2.set(wave(mixWave), waveLevel(f0 * mixH, fs));
        float inc2 = 0.0f;
        if (kind == C_DETUNE) {
            float ds = map::detuneSemis(ctrl0);
            inc2 = fTab * fexp2(ds / 12.0f) / fs;
        }

        /* the filter */
        const int ft = std::min(5, std::max(0, ch(FLT_TYPE)));
        static const float KB[4] = {0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f};
        float cutBase = std::log2(map::filterHz(eff(FLT_FREQ, vel))) + KB[std::min(3, std::max(0, ch(FLT_KB)))] *
                                                                            (x.pitch - 60.0f) / 12.0f;
        float fEnv = eff(FLT_MENV, vel) * 8.0f, fLfo = eff(FLT_LFO, vel);
        fLfo = fLfo * fLfo * 6.0f;
        float res = eff(FLT_RES, vel), drive = eff(FLT_DRIVE, vel);
        const float cutMax = std::log2(0.44f * fs), cutMin = std::log2(14.0f);
        const float kSvf = 2.0f * (1.0f - 0.985f * res);
        const float kLad = ft == 2 ? 4.15f * res : 3.95f * res;
        float lvl = map::level(eff(OUT_LEVEL, vel));
        float velGain = sw(AENV_VEL) ? 0.12f + 0.88f * std::pow(vel, 1.3f) : 1.0f;
        float gainL = 0.42f * lvl * velGain * x.gl, gainR = 0.42f * lvl * velGain * x.gr;
        float dGain = 1.0f + 9.0f * drive * drive, dComp = (1.0f + drive) / (1.0f + 2.5f * drive);

        float sh = x.lfoSH;
        float *L = osL, *R = osR;
        for (int i = 0; i < m; i++) {
            float menv = x.mod.tick() * envSign;
            float a = x.amp.tick();
            float lfo;
            if (lfoEnvMode) {            /* the LFO as an envelope of this voice */
                float p = x.lfoT / lfoTime;
                x.lfoT += 1.0f / fs;
                switch (ch(LFO_WAVE)) {
                case 0: lfo = p < 1.0f ? 1.0f : 0.0f; break;
                case 1: lfo = std::max(0.0f, 1.0f - p); break;
                case 2: lfo = std::min(1.0f, p); break;
                case 3: lfo = p < 0.5f ? 2.0f * p : std::max(0.0f, 2.0f - 2.0f * p); break;
                default: lfo = sh; break;
                }
                if (lfoBip)
                    lfo = 2.0f * lfo - 1.0f;
            } else
                lfo = lfoBip ? lfoB[i] : lfoU[i];

            const float rfs = 1.0f / fs;
            float y, c = clampf(ctrl + oLfo * oLfo * lfo + oEnv * menv, 0.0f, 1.0f);
            switch (kind) {
            case C_PITCH: {
                float s = map::pitchSemis(ctrl) + 24.0f * oLfo * oLfo * lfo + 48.0f * oEnv * oEnv * menv;
                x.ph1 += fTab * fexp2(s / 12.0f) * rfs;
                x.ph1 -= std::floor(x.ph1);
                y = r1(x.ph1);
                break;
            }
            case C_SHAPE: {
                x.ph1 += fTab * rfs;
                x.ph1 -= std::floor(x.ph1);
                float p = x.ph1;
                if (wv == 0) {           /* the pulse's width: 50 % .. 4 % */
                    float w = 0.5f - 0.46f * c;
                    y = rs(p) - rs(p + w);
                } else if (wv == 1)      /* the saw and a second one sliding to the octave */
                    y = 0.5f * (r1(p) + r1(p + 0.5f * c)) * (1.0f + 0.5f * c);
                else if (wv == 2 || wv == 3)   /* folded */
                    y = fold(r1(p) * (1.0f + 5.0f * c));
                else {                   /* phase distortion */
                    float k = 0.5f - 0.45f * c;
                    float q = p < k ? 0.5f * p / k : 0.5f + 0.5f * (p - k) / (1.0f - k);
                    y = r1(q);
                }
                break;
            }
            case C_SYNC: {
                float inc = f0 * rfs, ratio = fexp2(4.0f * c) * W.freqMul;
                x.ph3 += inc;
                x.ph3 -= std::floor(x.ph3);
                float s = frac(x.ph3 * ratio);
                float h = r1(0.0f) - r1(frac(ratio));
                y = r1(s) + 0.5f * h * blep(x.ph3, inc);
                break;
            }
            case C_DETUNE: {
                x.ph1 += fTab * rfs;
                x.ph1 -= std::floor(x.ph1);
                x.ph2 += inc2;
                x.ph2 -= std::floor(x.ph2);
                y = 0.65f * (r1(x.ph1) + r1(x.ph2));
                break;
            }
            case C_MIX: {
                x.pm += 0.5f * f0 * rfs;
                if (x.pm >= 8.0f)
                    x.pm -= 8.0f;
                float p1 = frac(2.0f * x.pm * W.freqMul), p2 = frac(2.0f * mixH * x.pm);
                y = (r1(p1) + c * r2(p2)) / (1.0f + 0.5f * c);
                break;
            }
            case C_FM: {
                x.ph1 += fTab * rfs;
                x.ph1 -= std::floor(x.ph1);
                x.ph2 += f0 * fmRatio * rfs;
                x.ph2 -= std::floor(x.ph2);
                y = r1(x.ph1 + map::fmIndex(c) * 0.15915f * fsin01(x.ph2));
                break;
            }
            case C_AM: {
                x.ph1 += fTab * rfs;
                x.ph1 -= std::floor(x.ph1);
                x.ph2 += f0 * map::amRatio(c) * rfs;
                x.ph2 -= std::floor(x.ph2);
                y = r1(x.ph1) * fsin01(x.ph2);
                break;
            }
            default: {
                x.ph1 += fTab * rfs;
                x.ph1 -= std::floor(x.ph1);
                y = r1(x.ph1) * (1.0f - c) + x.rng.bi() * c * 0.8f;
                break;
            }
            }

            /* the filter */
            float cl = clampf(cutBase + fEnv * menv + fLfo * (lfoBip ? 0.5f * (lfo + 1.0f) : lfo), cutMin, cutMax);
            float g = ftan(std::min(1.45f, PI * fexp2(cl) / fs));
            float in = y * 0.85f;
            float out;
            if (ft == 0 || ft == 4 || ft == 5) {   /* the state-variable ones: LP12, BP, HP */
                float k = kSvf;
                float A1 = 1.0f / (1.0f + g * (g + k)), A2 = g * A1, A3 = g * A2;
                float xin = ftanh(in);
                float v3 = xin - x.a2, v1 = A1 * x.a1 + A2 * v3, v2 = x.a2 + A2 * x.a1 + A3 * v3;
                x.a1 = 2.0f * v1 - x.a1;
                x.a2 = 2.0f * v2 - x.a2;
                float lp = v2, bp = v1, hp = xin - k * v1 - v2;
                if (ft == 0)
                    out = lp;
                else {                   /* a second stage: 12 dB each side (BP), 24 dB (HP) */
                    float k2 = ft == 4 ? k : 1.41421356f;
                    float in2 = ft == 4 ? bp * k : hp;
                    float B1 = 1.0f / (1.0f + g * (g + k2)), B2 = g * B1, B3 = g * B2;
                    float w3 = in2 - x.b2, w1 = B1 * x.b1 + B2 * w3, w2 = x.b2 + B2 * x.b1 + B3 * w3;
                    x.b1 = 2.0f * w1 - x.b1;
                    x.b2 = 2.0f * w2 - x.b2;
                    out = ft == 4 ? w1 * k2 : in2 - k2 * w1 - w2;
                }
            } else {                     /* the ladders: LP24, LP M (transistor), LP TB (diode) */
                float G = g / (1.0f + g), be = 1.0f - G;
                float G2 = G * G, G3 = G2 * G, G4 = G2 * G2;
                float S = G3 * be * x.s[0] + G2 * be * x.s[1] + G * be * x.s[2] + be * x.s[3];
                float k = kLad;
                if (ft == 3) {           /* the diode ladder: less resonance down low, never self-oscillating */
                    float t = clampf((cl - 6.3f) / 3.3f, 0.25f, 1.0f);
                    k = 3.3f * res * t;
                    S -= x.hpz;
                }
                float u = (in - k * S) / (1.0f + k * G4);
                u = ftanh(u);
                float y1, y2, y3, y4;
                auto stage = [G](float xi, float &s) {
                    float vv = (xi - s) * G;
                    float yy = vv + s;
                    s = yy + vv;
                    return yy;
                };
                if (ft == 2) {           /* the transistor ladder: each stage a little saturated */
                    y1 = stage(u, x.s[0]);
                    y2 = stage(ftanh(y1), x.s[1]);
                    y3 = stage(ftanh(y2), x.s[2]);
                    y4 = stage(ftanh(y3), x.s[3]);
                    out = y4 * (1.0f + 0.45f * k);
                } else {
                    y1 = stage(u, x.s[0]);
                    y2 = stage(y1, x.s[1]);
                    y3 = stage(y2, x.s[2]);
                    y4 = stage(y3, x.s[3]);
                    if (ft == 3) {
                        x.hpz += 0.012f * (y4 - x.hpz);
                        out = (0.75f * y4 + 0.3f * y3) * (1.0f + 0.3f * k);
                    } else
                        out = y4;
                }
            }
            if (drive > 0.001f)
                out = ftanh(out * dGain) * dComp;
            L[i] += out * a * gainL;
            R[i] += out * a * gainR;
        }
        x.inc1 = f0;
        x.cutLog = cutBase;
        if (x.amp.st == ST_OFF) {
            x.on = false;
            x.gate = false;
        }
    }
};

/* ------------------------------------------------------------------------------------------- the engine --- */
Engine::Engine() : d(new Impl()) {}
Engine::~Engine() = default;

void Engine::prepare(double sampleRate)
{
    wave(0);                             /* (the tables: built now, not on the audio thread) */
    auto keep = std::make_unique<Impl>();
    std::copy(d->val, d->val + NPARAMS, keep->val);
    d = std::move(keep);
    d->sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
    d->os = d->sr < 70000.0 ? 2 : 1;
    d->fsI = (float)(d->sr * d->os);
    d->dec[0].design();
    d->dec[1].design();
    d->fx.prepare((float)d->sr);
    d->dly.prepare((float)d->sr);
    d->rev.prepare((float)d->sr);
}

void Engine::set(int i, float v)
{
    if (i < 0 || i >= NPARAMS)
        return;
    v = clampf(v, 0.0f, paramMax(i));
    if (PARAMS[i].kind == K_SW)
        v = v >= 0.5f ? 1.0f : 0.0f;
    float old = d->val[i];
    d->val[i] = v;
    if (old == v)
        return;
    if (i == ARP_RUN)
        d->reroute();
    else if (i == HOLD && v < 0.5f && !d->sustainDown)
        d->releaseLatched(true);
    else if (i == VOICE_MODE || i == UNISON) {
        d->releaseAllVoices();
        if (!d->arpOn())
            for (int n = 0; n < 128; n++)
                if (d->latched[n])
                    d->playOn(n, d->keyVel[n]);
    }
}

float Engine::get(int i) const { return i >= 0 && i < NPARAMS ? d->val[i] : 0.0f; }
void Engine::noteOn(int note, float vel) { d->noteOn(note, vel); }
void Engine::noteOff(int note) { d->noteOff(note); }
void Engine::sustain(bool down)
{
    d->sustainDown = down;
    if (!down && !d->sw(HOLD))
        d->releaseLatched(true);
}
void Engine::allNotesOff() { d->allNotesOff(); }
void Engine::panic() { d->panic(); }
void Engine::bend(float v) { d->bendV = clampf(v, -1.0f, 1.0f); }
void Engine::modWheel(float v) { d->wheelT = clampf(v, 0.0f, 1.0f); }
void Engine::tempo(double bpm) { d->bpm = bpm > 10.0 && bpm < 1000.0 ? bpm : 120.0; }
void Engine::position(double ppq, bool playing)
{
    d->ppqBase = ppq;
    d->playing = playing;
    d->sinceBase = 0;
}

void Engine::render(float *left, float *right, int n)
{
    int done = 0;
    while (done < n) {
        if (d->outPos >= BLOCK) {
            d->block();
            d->outPos = 0;
        }
        int k = std::min(n - done, BLOCK - d->outPos);
        std::copy(d->outL + d->outPos, d->outL + d->outPos + k, left + done);
        std::copy(d->outR + d->outPos, d->outR + d->outPos + k, right + done);
        d->outPos += k;
        done += k;
    }
}

float Engine::lfoLevel() const { return d->mLfo.load(); }
float Engine::outLevel() const { return d->mOut.load(); }
int Engine::voicesSounding() const { return d->mVoices.load(); }
float Engine::wheel() const { return d->wheelS; }
} // namespace nq
