/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
#include "fx.h"
#include "maps.h"
#include <algorithm>

namespace nq
{
/* ================================================================================================ the FX unit === */
void FxUnit::prepare(float sampleRate)
{
    sr = sampleRate;
    for (auto &d : dl)
        d.init((int)(0.05f * sr) + 8);
    clear();
}

void FxUnit::clear()
{
    for (auto &d : dl)
        d.clear();
    for (int c = 0; c < 2; c++) {
        fb[c] = phFb[c] = dcx[c] = dcy[c] = 0.0f;
        for (float &a : ap[c])
            a = 0.0f;
    }
    gain = 0.0f;
    cur = -1;
}

void FxUnit::process(float *l, float *r, int n, int type, float amt, bool on)
{
    const bool want = on && amt > 0.001f;
    if (type != cur && gain <= 0.0f) {   /* a new effect starts clean */
        for (auto &d : dl)
            d.clear();
        for (int c = 0; c < 2; c++) {
            fb[c] = phFb[c] = 0.0f;
            for (float &a : ap[c])
                a = 0.0f;
        }
        cur = type;
    }
    const float target = (want && type == cur) ? 1.0f : 0.0f;
    if (target == 0.0f && gain <= 0.0f)
        return;
    const float step = 1.0f / (0.01f * sr);
    amtS += (amt - amtS) * 0.2f;
    const float a = amtS;
    for (int i = 0; i < n; i++) {
        gain = target > gain ? std::min(target, gain + step) : std::max(target, gain - step);
        float in[2] = {l[i], r[i]}, wet[2];
        switch (cur) {
        case 0: {                        /* flanger: a short swept delay fed back */
            lfoA += map::fxRate(a) / sr;
            lfoA -= std::floor(lfoA);
            for (int c = 0; c < 2; c++) {
                float ph = lfoA + 0.25f * (float)c;
                float tri = std::fabs(2.0f * (ph - std::floor(ph)) - 1.0f);
                float d = (0.3f + 3.7f * tri * tri) * 0.001f * sr;
                float y = dl[c].read(d);
                dl[c].push(in[c] + fb[c] * 0.7f);
                fb[c] = y;
                wet[c] = 0.5f * (in[c] + y);
            }
            break;
        }
        case 1: {                        /* phaser: six allpasses swept, fed back */
            lfoA += map::fxRate(a) / sr;
            lfoA -= std::floor(lfoA);
            for (int c = 0; c < 2; c++) {
                float ph = lfoA + 0.2f * (float)c;
                float tri = std::fabs(2.0f * (ph - std::floor(ph)) - 1.0f);
                float hz = 180.0f * fexp2(4.2f * tri);
                float t = std::tan(PI * std::min(hz, 0.45f * sr) / sr);
                float k = (t - 1.0f) / (t + 1.0f);
                float x = in[c] + phFb[c] * 0.55f;
                for (float &z : ap[c]) {
                    float y = k * x + z;
                    z = x - k * y;
                    x = y;
                }
                phFb[c] = x;
                wet[c] = 0.5f * (in[c] + x);
            }
            break;
        }
        case 2: {                        /* ring modulation: times a sine */
            ringPh += map::ringHz(a) / sr;
            ringPh -= std::floor(ringPh);
            float s = fsin01(ringPh);
            wet[0] = in[0] * s;
            wet[1] = in[1] * s;
            break;
        }
        case 3: {                        /* chorus: two voices, slow, stereo */
            lfoA += 0.45f / sr;
            lfoA -= std::floor(lfoA);
            float depth = (0.8f + 3.2f * a) * 0.001f * sr, base = 0.007f * sr;
            for (int c = 0; c < 2; c++) {
                float s = fsin01(lfoA + 0.5f * (float)c);
                dl[c].push(in[c]);
                float y = dl[c].read(base + depth * (0.5f + 0.5f * s));
                float mix = 0.3f + 0.3f * a;
                wet[c] = in[c] * (1.0f - mix) + y * mix * 1.2f;
            }
            break;
        }
        case 4: {                        /* ensemble: three delay lines, a slow and a fast sweep, 120 degrees apart */
            lfoA += 0.62f / sr;
            lfoA -= std::floor(lfoA);
            lfoB += 6.1f / sr;
            lfoB -= std::floor(lfoB);
            float base = 0.009f * sr, slow = (0.6f + 2.4f * a) * 0.001f * sr, fast = 0.25f * 0.001f * sr;
            for (int c = 0; c < 2; c++) {
                dl[c].push(in[c]);
                float y = 0.0f;
                for (int k = 0; k < 3; k++) {
                    float off = (float)k / 3.0f + (c ? 0.5f / 3.0f : 0.0f);
                    float d = base + slow * (0.5f + 0.5f * fsin01(lfoA + off)) + fast * (0.5f + 0.5f * fsin01(lfoB + off));
                    y += dl[c].read(d);
                }
                float mix = 0.35f + 0.35f * a;
                wet[c] = in[c] * (1.0f - mix) + y * 0.45f * mix;
            }
            break;
        }
        default: {                       /* drive: a warm, uneven overdrive */
            float g = 1.0f + 24.0f * a * a;
            for (int c = 0; c < 2; c++) {
                float x = in[c] * g + 0.15f * a;
                float y = x > 0.0f ? ftanh(x) : 0.85f * ftanh(1.2f * x);
                float o = y - dcx[c] + 0.995f * dcy[c];   /* (the offset out) */
                dcx[c] = y;
                dcy[c] = o;
                wet[c] = o * (0.9f / (1.0f + 1.5f * a));
            }
            break;
        }
        }
        l[i] = in[0] + gain * (wet[0] - in[0]);
        r[i] = in[1] + gain * (wet[1] - in[1]);
    }
    (void)lfoC;
}

/* ================================================================================================ the delay === */
void DelayUnit::prepare(float sampleRate)
{
    sr = sampleRate;
    for (auto &d : line)
        d.init((int)(2.1f * sr) + 8);
    clear();
}

void DelayUnit::clear()
{
    for (auto &d : line)
        d.clear();
    lp[0] = lp[1] = hp[0] = hp[1] = 0.0f;
    wet = 0.0f;
}

void DelayUnit::process(float *l, float *r, int n, float seconds, int fbLevel, bool ping, float mix, bool analog, bool on)
{
    static const float FB[4] = {0.0f, 0.32f, 0.52f, 0.7f};
    const float fbk = FB[std::min(3, std::max(0, fbLevel))];
    seconds = clampf(seconds, 0.01f, 2.0f);
    if (std::fabs(seconds - target) > 0.0005f) {
        target = seconds;
        if (!analog && xf >= 1.0f) {     /* normal: fade to the new time (no pitch change) */
            old = cur;
            cur = target;
            xf = 0.0f;
        }
    }
    const float lpA = onePoleCoef(analog ? 3200.0f : 7500.0f, sr), hpA = onePoleCoef(90.0f, sr);
    const float smooth = 1.0f - std::exp(-1.0f / (0.12f * sr));
    const float onT = on ? 1.0f : 0.0f;
    for (int i = 0; i < n; i++) {
        wet += (onT - wet) * 0.002f;
        mixS += (mix - mixS) * 0.002f;
        if (analog) {                    /* analogue: the time glides (the echoes bend) */
            cur += (target - cur) * smooth;
            xf = 1.0f;
        } else if (xf < 1.0f) {
            xf = std::min(1.0f, xf + 1.0f / (0.05f * sr));
            if (xf >= 1.0f && std::fabs(target - cur) > 0.0005f) {
                old = cur;
                cur = target;
                xf = 0.0f;
            }
        }
        float y[2];
        for (int c = 0; c < 2; c++) {
            float d = cur * sr;
            float v = line[c].read3(d);
            if (xf < 1.0f)
                v = line[c].read3(old * sr) * (1.0f - xf) + v * xf;
            lp[c] += lpA * (v - lp[c]);
            hp[c] += hpA * (lp[c] - hp[c]);
            y[c] = lp[c] - hp[c];
            if (analog)
                y[c] = ftanh(y[c] * 1.1f) / 1.1f;
        }
        float inL = l[i] * wet, inR = r[i] * wet;
        if (ping) {                      /* the input into the left, each echo crossing over */
            line[0].push(0.5f * (inL + inR) + y[1] * fbk);
            line[1].push(y[0] * fbk);
        } else {
            line[0].push(inL + y[0] * fbk);
            line[1].push(inR + y[1] * fbk);
        }
        float m = mixS * wet;
        float dry = std::cos(m * 0.5f * PI), wt = std::sin(m * 0.5f * PI);
        if (wet <= 0.0001f) {
            dry = 1.0f;
            wt = 0.0f;
        }
        l[i] = l[i] * dry + y[0] * wt;
        r[i] = r[i] * dry + y[1] * wt;
    }
}

/* =============================================================================================== the reverb === */
static const float LINE_MS[8] = {29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 61.3f, 67.9f, 73.1f};

void ReverbUnit::prepare(float sampleRate)
{
    sr = sampleRate;
    for (int i = 0; i < NL; i++) {
        len[i] = LINE_MS[i] * 0.001f * sr;
        line[i].init((int)(len[i] * 1.6f) + 64);
        ph[i] = (float)i / (float)NL;
    }
    static const float DIFF_MS[2][3] = {{4.1f, 6.3f, 11.7f}, {3.7f, 5.9f, 12.3f}};
    for (int c = 0; c < 2; c++)
        for (int k = 0; k < 3; k++)
            diff[c][k].init((int)(DIFF_MS[c][k] * 0.001f * sr) + 1);
    pre.init((int)(0.06f * sr) + 8);
    clear();
}

void ReverbUnit::clear()
{
    for (auto &l : line)
        l.clear();
    for (auto &c : diff)
        for (auto &a : c)
            a.d.clear();
    pre.clear();
    for (float &d : damp)
        d = 0.0f;
    wet = 0.0f;
    cur = -1;
}

void ReverbUnit::setType(int t)
{
    /* room, stage 1 (soft), stage 2 (bright), hall 1 (soft), hall 2 (bright) */
    static const float SIZE[5] = {0.42f, 0.75f, 0.75f, 1.45f, 1.45f};
    static const float T60[5] = {0.6f, 1.4f, 1.3f, 2.9f, 3.2f};
    static const float DAMP[5] = {6500.0f, 3800.0f, 9500.0f, 3200.0f, 8500.0f};
    static const float PRE[5] = {0.004f, 0.012f, 0.012f, 0.025f, 0.025f};
    t = std::min(4, std::max(0, t));
    cur = t;
    size = SIZE[t];
    for (int i = 0; i < NL; i++)
        g[i] = std::pow(10.0f, -3.0f * len[i] * size / (T60[t] * sr));
    dampA = onePoleCoef(DAMP[t], sr);
    preS = PRE[t] * sr;
}

void ReverbUnit::process(float *l, float *r, int n, int type, float mix, bool on)
{
    if (type != cur)
        setType(type);
    const float onT = on ? 1.0f : 0.0f;
    for (int i = 0; i < n; i++) {
        wet += (onT - wet) * 0.002f;
        mixS += (mix - mixS) * 0.002f;
        if (wet <= 0.00005f && !on) {    /* (off: the tail stops; the lines still listen, quietly) */
            continue;
        }
        pre.push(0.5f * (l[i] + r[i]) * wet);
        float x = pre.read(preS);
        float xl = x, xr = x;
        for (auto &a : diff[0])
            xl = a.run(xl, 0.62f);
        for (auto &a : diff[1])
            xr = a.run(xr, 0.62f);
        float y[NL], s = 0.0f, ol = 0.0f, orr = 0.0f;
        for (int k = 0; k < NL; k++) {
            float d = len[k] * size;
            if (k & 1) {
                ph[k] += (0.11f + 0.04f * (float)k) / sr;
                ph[k] -= std::floor(ph[k]);
                d += 0.0025f * sr * (1.0f + fsin01(ph[k]));
            }
            float v = line[k].read(d);
            damp[k] += dampA * (v - damp[k]);
            y[k] = damp[k] * g[k];
            s += y[k];
            if (k < 4)
                ol += (k & 1) ? -y[k] : y[k];
            else
                orr += (k & 1) ? -y[k] : y[k];
        }
        s *= 2.0f / (float)NL;
        for (int k = 0; k < NL; k++)
            line[k].push((k < 4 ? xl : xr) * 0.4f + y[k] - s);
        float m = mixS * wet;
        float dry = std::cos(m * 0.5f * PI), wt = std::sin(m * 0.5f * PI) * 0.9f;
        l[i] = l[i] * dry + (ol + 0.3f * orr) * wt;
        r[i] = r[i] * dry + (orr + 0.3f * ol) * wt;
    }
}
} // namespace nq
