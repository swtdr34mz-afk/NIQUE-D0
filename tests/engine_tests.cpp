/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The engine's tests, without JUCE: every program at three rates, any buffer size the same, every parameter swept,
 * every waveform in every configuration, every filter, the arpeggiator, hold, the morphs, MUTATE and RANDOMIZE.
 *   cmake -S tests -B build-tests && cmake --build build-tests && build-tests/engine_tests [--wav dir] */
#include "nique.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace nq;

namespace nq
{
int presetProblems(char *msg, int n);
}

static int failures = 0;
#define CHECK(c, ...)                                                                                                 \
    do {                                                                                                              \
        if (!(c)) {                                                                                                   \
            failures++;                                                                                               \
            std::printf("FAIL %s:%d  ", __FILE__, __LINE__);                                                          \
            std::printf(__VA_ARGS__);                                                                                 \
            std::printf("\n");                                                                                        \
        }                                                                                                             \
    } while (0)

struct Stats {
    float peak = 0.0f, rms = 0.0f;
    bool finite = true;
};

static Stats run(Engine &e, int frames, int blockSize, std::vector<float> *keepL = nullptr, std::vector<float> *keepR = nullptr)
{
    Stats s;
    std::vector<float> l((size_t)blockSize), r((size_t)blockSize);
    double sum = 0.0;
    int done = 0;
    while (done < frames) {
        int n = std::min(blockSize, frames - done);
        e.render(l.data(), r.data(), n);
        for (int i = 0; i < n; i++) {
            if (!std::isfinite(l[(size_t)i]) || !std::isfinite(r[(size_t)i]))
                s.finite = false;
            s.peak = std::max(s.peak, std::max(std::fabs(l[(size_t)i]), std::fabs(r[(size_t)i])));
            sum += (double)l[(size_t)i] * l[(size_t)i] + (double)r[(size_t)i] * r[(size_t)i];
            if (keepL) {
                keepL->push_back(l[(size_t)i]);
                keepR->push_back(r[(size_t)i]);
            }
        }
        done += n;
    }
    s.rms = (float)std::sqrt(sum / (2.0 * frames));
    return s;
}

static void load(Engine &e, const float *v)
{
    for (int i = 0; i < NPARAMS; i++)
        e.set(i, v[i]);
}

static void writeWav(const std::string &path, const std::vector<float> &l, const std::vector<float> &r, int sr)
{
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f)
        return;
    uint32_t n = (uint32_t)l.size(), bytes = n * 4;
    auto w32 = [f](uint32_t x) { std::fwrite(&x, 4, 1, f); };
    auto w16 = [f](uint16_t x) { std::fwrite(&x, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f);
    w32(36 + bytes);
    std::fwrite("WAVEfmt ", 1, 8, f);
    w32(16);
    w16(1);
    w16(2);
    w32((uint32_t)sr);
    w32((uint32_t)sr * 4);
    w16(4);
    w16(16);
    std::fwrite("data", 1, 4, f);
    w32(bytes);
    for (uint32_t i = 0; i < n; i++) {
        int16_t a = (int16_t)std::lround(std::max(-1.0f, std::min(1.0f, l[i])) * 32767.0f);
        int16_t b = (int16_t)std::lround(std::max(-1.0f, std::min(1.0f, r[i])) * 32767.0f);
        std::fwrite(&a, 2, 1, f);
        std::fwrite(&b, 2, 1, f);
    }
    std::fclose(f);
}

/* a phrase: a chord, then a melody, then release */
static Stats phrase(Engine &e, int sr, int blockSize, std::vector<float> *kl = nullptr, std::vector<float> *kr = nullptr)
{
    Stats all;
    auto acc = [&all](Stats s) {
        all.peak = std::max(all.peak, s.peak);
        all.rms = std::max(all.rms, s.rms);
        all.finite = all.finite && s.finite;
    };
    e.tempo(120.0);
    e.position(0.0, false);
    e.noteOn(48, 0.9f);
    e.noteOn(55, 0.7f);
    e.noteOn(64, 0.8f);
    acc(run(e, sr / 2, blockSize, kl, kr));
    e.modWheel(1.0f);
    acc(run(e, sr / 4, blockSize, kl, kr));
    e.noteOff(48);
    e.noteOff(55);
    e.noteOff(64);
    e.modWheel(0.0f);
    const int mel[6] = {60, 67, 72, 71, 67, 62};
    for (int k = 0; k < 6; k++) {
        e.noteOn(mel[k], 0.4f + 0.1f * (float)k);
        acc(run(e, sr / 8, blockSize, kl, kr));
        e.noteOff(mel[k]);
    }
    acc(run(e, sr, blockSize, kl, kr));
    return all;
}

int main(int argc, char **argv)
{
    std::string wavDir;
    for (int i = 1; i + 1 < argc; i++)
        if (std::strcmp(argv[i], "--wav") == 0)
            wavDir = argv[i + 1];

    /* the table */
    char msg[200] = {};
    CHECK(presetProblems(msg, sizeof msg) == 0, "a program's setting is unknown: %s", msg);
    for (int i = 0; i < NPARAMS; i++) {
        CHECK(paramIndex(PARAMS[i].id) == i, "id %s", PARAMS[i].id);
        CHECK(PARAMS[i].def >= 0.0f && PARAMS[i].def <= paramMax(i), "default of %s", PARAMS[i].id);
        char t[64];
        paramText(i, PARAMS[i].def, t, sizeof t);
        CHECK(t[0] != 0, "text of %s", PARAMS[i].id);
    }
    for (int m = 0; m < NMORPH; m++)
        CHECK(morphOf(MORPHS[m].target) == m, "morph %d", m);

    std::vector<float> v((size_t)NPARAMS);

    /* every program, three rates */
    std::printf("%-18s %8s %8s\n", "program", "peak", "rms");
    for (int sr : {44100, 48000, 96000}) {
        for (int k = 0; k < numPresets(); k++) {
            Engine e;
            e.prepare(sr);
            presetValues(k, v.data());
            load(e, v.data());
            std::vector<float> kl, kr;
            bool keep = !wavDir.empty() && sr == 48000;
            Stats s = phrase(e, sr, 512, keep ? &kl : nullptr, keep ? &kr : nullptr);
            CHECK(s.finite, "%s at %d: not finite", presetName(k), sr);
            CHECK(s.peak < 1.6f, "%s at %d: too loud (%.2f)", presetName(k), sr, s.peak);
            CHECK(s.peak > 0.02f, "%s at %d: silent (%.4f)", presetName(k), sr, s.peak);
            if (sr == 48000)
                std::printf("%-18s %8.3f %8.3f\n", presetName(k), s.peak, s.rms);
            if (keep) {
                std::string name = presetName(k);
                for (char &c : name)
                    if (c == ' ' || c == '/')
                        c = '_';
                writeWav(wavDir + "/" + std::to_string(k) + "_" + name + ".wav", kl, kr, sr);
            }
        }
    }

    /* any buffer size: the same samples */
    {
        presetValues(2, v.data());
        std::vector<float> a, b, al, bl;
        for (int bs : {512, 37}) {
            Engine e;
            e.prepare(48000);
            load(e, v.data());
            phrase(e, 48000, bs, bs == 512 ? &a : &b, bs == 512 ? &al : &bl);
        }
        float diff = 0.0f;
        for (size_t i = 0; i < a.size() && i < b.size(); i++)
            diff = std::max(diff, std::fabs(a[i] - b[i]));
        CHECK(a.size() == b.size() && diff < 1e-6f, "buffer sizes differ: %g", diff);
    }

    /* every waveform in every configuration, every filter */
    {
        int bad = 0;
        for (int w = 0; w < 47; w++)
            for (int c = 0; c < PARAMS[OSC_CONFIG].nChoices; c++) {
                Engine e;
                e.prepare(44100);
                initSound(v.data());
                v[OSC_WAVE] = (float)w;
                v[OSC_CONFIG] = (float)c;
                v[OSC_CTRL] = (float)((w * 7 + c * 3) % 10) / 9.0f;
                v[OSC_MENV] = 0.5f;
                v[OSC_LFO] = 0.4f;
                v[FLT_TYPE] = (float)((w + c) % 6);
                v[FLT_RES] = 0.9f;
                v[FLT_FREQ] = 0.55f;
                load(e, v.data());
                e.noteOn(36 + (w + c) % 60, 1.0f);
                Stats s = run(e, 6000, 256);
                if (!s.finite || s.peak > 2.0f || s.peak < 1e-4f) {
                    if (bad < 6)
                        std::printf("  wave %d config %d filter %d: peak %.4f finite %d\n", w, c, (w + c) % 6, s.peak,
                                    (int)s.finite);
                    bad++;
                }
            }
        CHECK(bad == 0, "%d waveform/configuration pairs misbehave", bad);
    }

    /* every parameter swept, everything on, under notes */
    {
        Engine e;
        e.prepare(48000);
        presetValues(3, v.data());
        v[FX_ON] = v[DLY_ON] = v[REV_ON] = 1.0f;
        load(e, v.data());
        for (int n : {40, 52, 59, 64, 71})
            e.noteOn(n, 0.8f);
        bool fin = true;
        float peak = 0.0f;
        for (int i = 0; i < NPARAMS; i++)
            for (int s = 0; s <= 4; s++) {
                e.set(i, paramMax(i) * (float)s / 4.0f);
                Stats st = run(e, 600, 128);
                fin = fin && st.finite;
                peak = std::max(peak, st.peak);
            }
        CHECK(fin, "a sweep went non-finite");
        CHECK(peak < 6.0f, "a sweep got too loud: %.2f", peak);
    }

    /* the arpeggiator: free and with the host's clock */
    {
        Engine e;
        e.prepare(48000);
        presetValues(0, v.data());
        v[ARP_RUN] = 1.0f;
        v[ARP_SYNC] = 0.0f;
        v[AENV_R] = 0.0f;
        load(e, v.data());
        e.noteOn(60, 0.8f);
        e.noteOn(64, 0.8f);
        Stats s = run(e, 48000, 512);
        CHECK(s.finite && s.peak > 0.05f, "arp free: peak %.3f", s.peak);
        e.set(ARP_SYNC, 1.0f);
        double ppq = 0.0;
        for (int b = 0; b < 100; b++) {
            e.position(ppq, true);
            run(e, 480, 480);
            ppq += 480.0 * 2.0 / 48000.0;
        }
        e.noteOff(60);
        e.noteOff(64);
        run(e, 24000, 512);
        CHECK(e.voicesSounding() == 0, "arp: voices left: %d", e.voicesSounding());
    }

    /* HOLD keeps the notes; turning it off lets them go */
    {
        Engine e;
        e.prepare(48000);
        initSound(v.data());
        v[HOLD] = 1.0f;
        v[AENV_R] = 0.0f;
        load(e, v.data());
        e.noteOn(60, 0.8f);
        e.noteOff(60);
        run(e, 4800, 256);
        CHECK(e.voicesSounding() == 1, "hold: %d voices", e.voicesSounding());
        e.set(HOLD, 0.0f);
        run(e, 9600, 256);
        CHECK(e.voicesSounding() == 0, "hold off: %d voices", e.voicesSounding());
    }

    /* unison: 4 voices a note; mono: 1 */
    {
        Engine e;
        e.prepare(48000);
        initSound(v.data());
        v[UNISON] = 2.0f;
        load(e, v.data());
        e.noteOn(60, 0.8f);
        e.noteOn(64, 0.8f);
        run(e, 960, 64);
        CHECK(e.voicesSounding() == 8, "unison 2: %d voices", e.voicesSounding());
        e.allNotesOff();
        e.set(UNISON, 0.0f);
        e.set(VOICE_MODE, 1.0f);
        run(e, 48000, 512);
        e.noteOn(60, 0.8f);
        e.noteOn(64, 0.8f);
        run(e, 960, 64);
        CHECK(e.voicesSounding() == 1, "mono: %d voices", e.voicesSounding());
    }

    /* LFO 2, 3, 4: every waveform to every destination moves the sound */
    {
        int still = 0;
        for (int n = 0; n < 3; n++)
            for (int d = 0; d < 7; d++) {
                float peak[2];
                std::vector<float> outs[2];
                for (int on = 0; on < 2; on++) {
                    Engine e;
                    e.prepare(48000);
                    initSound(v.data());
                    v[FLT_FREQ] = 0.55f;
                    v[FLT_RES] = 0.4f;
                    v[FLT_DRIVE] = 0.3f;
                    const int b = L2_RATE + 6 * n;
                    v[(size_t)b] = 0.55f;
                    v[(size_t)(b + 3)] = (float)((n + d) % 5);
                    v[(size_t)(b + 4)] = (float)d;
                    v[(size_t)(b + 5)] = on ? 0.8f : 0.0f;
                    load(e, v.data());
                    e.noteOn(52, 0.8f);
                    std::vector<float> r;
                    Stats st = run(e, 24000, 256, &outs[on], &r);
                    CHECK(st.finite, "lfo %d dest %d: not finite", n + 2, d);
                    peak[on] = st.peak;
                    if (d == 6)          /* (pan: the right side tells) */
                        outs[on] = r;
                }
                double diff = 0.0;
                for (size_t i = 0; i < outs[0].size(); i++)
                    diff += std::fabs(outs[0][i] - outs[1][i]);
                if (diff < 1.0 || peak[1] > 2.0f) {
                    std::printf("  lfo %d dest %d: diff %.3f peak %.3f\n", n + 2, d, diff, peak[1]);
                    still++;
                }
            }
        CHECK(still == 0, "%d LFO destinations do nothing", still);
    }

    /* the morph: the wheel opens the filter */
    {
        float rms[2];
        for (int w = 0; w < 2; w++) {
            Engine e;
            e.prepare(48000);
            initSound(v.data());
            v[FLT_FREQ] = 0.3f;
            v[MW_FLT_FREQ] = 1.0f;
            load(e, v.data());
            e.modWheel(w ? 1.0f : 0.0f);
            e.noteOn(48, 0.8f);
            run(e, 4800, 256);
            std::vector<float> l, r;
            run(e, 4800, 256, &l, &r);
            double hi = 0.0;                  /* the brightness: the first difference's energy */
            for (size_t i = 1; i < l.size(); i++)
                hi += (double)(l[i] - l[i - 1]) * (l[i] - l[i - 1]);
            rms[w] = (float)hi;
        }
        CHECK(rms[1] > rms[0] * 4.0f, "the wheel's morph: %.4f -> %.4f", rms[0], rms[1]);
    }

    /* MUTATE and RANDOMIZE: in range, still sounding */
    {
        int silent = 0;
        for (uint32_t seed = 1; seed <= 60; seed++) {
            presetValues((int)(seed % (uint32_t)numPresets()), v.data());
            v[ARP_RUN] = 0.0f;
            if (seed % 2)
                mutate(v.data(), seed, MS_NONE);
            else
                randomize(v.data(), seed, MS_NONE);
            if (seed % 5 == 0)
                mutate(v.data(), seed, MS_WHEEL);
            if (seed % 7 == 0)
                randomize(v.data(), seed, MS_VEL);
            for (int i = 0; i < NPARAMS; i++)
                CHECK(v[(size_t)i] >= 0.0f && v[(size_t)i] <= paramMax(i), "seed %u: %s = %f", seed, PARAMS[i].id,
                      v[(size_t)i]);
            Engine e;
            e.prepare(44100);
            load(e, v.data());
            e.noteOn(57, 0.9f);
            Stats s = run(e, 22050, 512);
            CHECK(s.finite, "seed %u: not finite", seed);
            if (s.peak < 0.005f)
                silent++;
        }
        CHECK(silent <= 3, "%d of 60 mutations are silent", silent);
    }

    if (failures == 0)
        std::printf("all good\n");
    return failures == 0 ? 0 : 1;
}
