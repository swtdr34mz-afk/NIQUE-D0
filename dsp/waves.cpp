/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The waveforms: each defined by one cycle (drawn) or by its harmonics (amplitude, phase), turned into a spectrum
 * (FFT), then into ten tables with fewer and fewer harmonics. */
#include "waves.h"
#include <complex>
#include <mutex>
#include <vector>

namespace nq
{
using cpx = std::complex<double>;

static void fft(std::vector<cpx> &a, bool inverse)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        double ang = 2.0 * 3.14159265358979323846 / (double)len * (inverse ? 1.0 : -1.0);
        cpx wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            cpx w(1.0, 0.0);
            for (size_t k = 0; k < len / 2; k++) {
                cpx u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

namespace
{
constexpr int H = WT_N / 2;              /* harmonics 1 .. 1023 */
constexpr double TAU = 6.283185307179586;

/* a spectrum: harmonic n's complex amplitude (n = 1 .. H-1) */
struct Spec {
    std::vector<cpx> c = std::vector<cpx>(H, cpx(0.0, 0.0));
    void sine(int n, double amp, double phase = 0.0)   /* amp * sin(2 pi n t + phase) */
    {
        if (n >= 1 && n < H)
            c[(size_t)n] += std::polar(amp, phase - TAU / 4.0);
    }
};

/* one cycle drawn: its spectrum */
template <class F> Spec drawn(F f)
{
    std::vector<cpx> a(WT_N);
    for (int i = 0; i < WT_N; i++)
        a[(size_t)i] = cpx(f((double)i / WT_N), 0.0);
    fft(a, false);
    Spec s;
    for (int n = 1; n < H; n++)
        s.c[(size_t)n] = a[(size_t)n] * (2.0 / WT_N);
    return s;
}

/* phases that keep a dense spectrum's peaks low (Schroeder) */
double schroeder(int n, int count) { return 3.14159265358979 * (double)n * (double)n / (double)(count > 1 ? count : 2); }

Spec pulse(double width)
{
    return drawn([width](double t) { return t < width ? 1.0 : -1.0; });
}

/* the pickup of a clavinet over a narrow pulse: a comb on its harmonics */
Spec clavi(double width, double pickup, double bright)
{
    Spec s;
    for (int n = 1; n < H; n++) {
        double a = 2.0 / (3.14159265358979 * n) * std::sin(3.14159265358979 * n * width);
        a *= std::fabs(std::sin(3.14159265358979 * n * pickup)) * std::pow((double)n, bright);
        s.sine(n, a);
    }
    return s;
}

/* a vowel: the harmonics of a 130.8 Hz voice under three formants */
Spec vowel(double f1, double f2, double f3, double g2, double g3)
{
    Spec s;
    const double f0 = 130.8;
    for (int n = 1; n < 160; n++) {
        double f = n * f0;
        auto peak = [f](double c, double bw) { double x = (f - c) / bw; return std::exp(-0.5 * x * x); };
        double a = peak(f1, 90.0) + g2 * peak(f2, 120.0) + g3 * peak(f3, 160.0) + 0.015 / n;
        a *= 1.0 / std::sqrt((double)n);
        s.sine(n, a, schroeder(n, 40));
    }
    return s;
}

/* partials at inharmonic ratios on a table 1/8 of the note (ratio r: harmonic round(8 r)) */
Spec partials(std::initializer_list<std::pair<double, double>> ps)
{
    Spec s;
    int k = 0;
    for (auto &p : ps)
        s.sine((int)std::lround(p.first * 8.0), p.second, 0.7 * k++);
    return s;
}

/* the drawbars 16', 5 1/3', 8', 4', 2 2/3', 2', 1 3/5', 1 1/3', 1' (harmonics of 16') */
Spec drawbars(const char *reg, bool plus)
{
    static const int HARM[9] = {1, 3, 2, 4, 6, 8, 10, 12, 16};
    Spec s;
    for (int i = 0; i < 9; i++) {
        int d = reg[i] - '0';
        if (d > 0)
            s.sine(HARM[i], std::pow(10.0, (d - 8) * 3.0 / 20.0), 0.3 * i);
    }
    if (plus)                            /* the "+": the upper partials of a worn organ's tonewheels */
        for (int n : {20, 24, 32, 40})
            s.sine(n, 0.12, 1.1 * n);
    return s;
}

struct Built {
    std::vector<float> data;             /* NWAVES * WT_LEVELS * (WT_N + 1) */
    Wave waves[NWAVES];
};

Built *build()
{
    auto *b = new Built();
    b->data.assign((size_t)NWAVES * WT_LEVELS * (WT_N + 1), 0.0f);
    std::vector<Spec> sp(NWAVES);
    std::vector<float> mul(NWAVES, 1.0f);
    Rng rng;
    rng.seed(0xA1A1u);

    /* the panel's four */
    sp[0] = pulse(0.5);
    for (int n = 1; n < H; n++)
        sp[1].sine(n, 2.0 / (3.14159265358979 * n));                     /* saw (falling) */
    sp[2] = drawn([](double t) { return t < 0.25 ? 4 * t : t < 0.75 ? 2 - 4 * t : 4 * t - 4; });
    sp[3].sine(1, 1.0);
    /* A1 .. A7: the extended analogue ones */
    for (int n = 1; n < H; n++)
        sp[4].sine(n, 2.0 / (3.14159265358979 * std::pow((double)n, 1.45)));   /* soft saw */
    sp[5] = drawn([](double t) { return 0.55 * (1 - 2 * t) + 0.45 * (t < 0.5 ? 1.0 : -1.0); });
    sp[6] = drawn([](double t) { return 1.0 - 2.0 * std::floor(t * 4.0) / 3.0; });   /* stepped */
    sp[7] = drawn([](double t) { return t < 0.8 ? -1 + 2 * t / 0.8 : 1 - 2 * (t - 0.8) / 0.2; });
    sp[8] = drawn([](double t) { return t < 0.5 ? 2.0 * std::sin(TAU * t) - 0.64 : -0.64; });
    sp[9] = drawn([](double t) { return 2.0 * std::exp(-4.0 * t) - 1.0; });          /* exponential saw */
    sp[10] = drawn([](double t) {
        return t < 0.5 ? 2.0 * std::exp(-5.0 * t) - 0.6 : -(2.0 * std::exp(-5.0 * (t - 0.5)) - 0.6);
    });
    /* P1 .. P3 */
    sp[11] = pulse(0.25);
    sp[12] = pulse(0.10);
    sp[13] = pulse(0.04);
    /* O1 .. O9 */
    const char *REG[9] = {"808000000", "880000000", "888000000", "888800000", "888800008",
                          "808808008", "800008888", "888888888", "888888888"};
    for (int i = 0; i < 9; i++) {
        sp[(size_t)(14 + i)] = drawbars(REG[i], i == 8);
        mul[(size_t)(14 + i)] = 0.5f;
    }
    /* B1 .. B5: free bars, with overtones, a clamped bar, a metallophone, tubular bells */
    sp[23] = partials({{1.0, 1.0}, {2.756, 0.7}, {5.404, 0.5}, {8.933, 0.35}, {13.34, 0.2}});
    sp[24] = partials({{1.0, 1.0}, {2.0, 0.4}, {2.756, 0.6}, {3.0, 0.25}, {5.404, 0.45}, {8.933, 0.3}, {13.34, 0.2},
                       {18.64, 0.12}});
    sp[25] = partials({{1.0, 1.0}, {6.267, 0.6}, {17.55, 0.35}, {34.39, 0.2}});
    sp[26] = partials({{1.0, 1.0}, {3.99, 0.55}, {9.2, 0.3}, {15.1, 0.15}});
    sp[27] = partials({{1.0, 1.0}, {2.32, 0.8}, {4.25, 0.6}, {6.63, 0.45}, {9.38, 0.3}, {12.4, 0.2}});
    for (int i = 23; i <= 27; i++)
        mul[(size_t)i] = 0.125f;
    /* D1 .. D8: digital spectra */
    for (int n = 1; n < H; n++) {
        sp[28].sine(n, 1.0 / n, TAU * rng.uni());
        if (n & 1)
            sp[29].sine(n, 1.0 / n, TAU * rng.uni());
        if (!(n & 1))
            sp[30].sine(n, 1.0 / n, schroeder(n, 200));
        if (n == 1 || n % 3 == 0)
            sp[31].sine(n, 1.0 / n, schroeder(n, 200));
        if (n == 1 || n % 4 == 0)
            sp[32].sine(n, 1.0 / n, schroeder(n, 200));
        if (n == 1 || n % 5 == 0)
            sp[33].sine(n, 1.0 / n, schroeder(n, 200));
        if ((n - 1) % 6 < 3)
            sp[34].sine(n, 1.0 / std::pow((double)n, 0.8), schroeder(n, 200));
    }
    {
        int k = 0;
        for (auto p : std::initializer_list<std::pair<int, double>>{{1, 1.0}, {7, 0.6}, {8, 0.5}, {13, 0.45}, {14, 0.4},
                                                                     {15, 0.35}, {21, 0.3}, {22, 0.22}, {29, 0.15}})
            sp[35].sine(p.first, p.second, 0.9 * k++);
    }
    /* E1 .. E4: electric pianos, clavinets */
    for (auto p : std::initializer_list<std::pair<int, double>>{{1, 1.0}, {2, 0.30}, {3, 0.12}, {4, 0.06}, {5, 0.03},
                                                                 {7, 0.09}, {9, 0.03}})
        sp[36].sine(p.first, p.second);
    for (auto p : std::initializer_list<std::pair<int, double>>{{1, 1.0}, {2, 0.55}, {3, 0.45}, {4, 0.25}, {5, 0.2},
                                                                 {6, 0.12}, {7, 0.08}, {8, 0.05}, {9, 0.03}})
        sp[37].sine(p.first, p.second, 0.4 * p.first);
    sp[38] = clavi(0.2, 0.12, 0.0);
    sp[39] = clavi(0.1, 0.08, 0.15);
    /* F1 .. F7: vowels */
    sp[40] = vowel(730, 1090, 2440, 0.5, 0.25);
    sp[41] = vowel(270, 2290, 3010, 0.3, 0.2);
    sp[42] = vowel(300, 870, 2240, 0.35, 0.1);
    sp[43] = vowel(250, 1750, 2160, 0.3, 0.2);
    sp[44] = vowel(570, 840, 2410, 0.6, 0.2);
    sp[45] = vowel(660, 1720, 2410, 0.5, 0.25);
    sp[46] = vowel(400, 1500, 2400, 0.4, 0.2);

    std::vector<cpx> a(WT_N);
    for (int w = 0; w < NWAVES; w++) {
        float scale = 1.0f;
        b->waves[w].freqMul = mul[(size_t)w];
        for (int l = 0; l < WT_LEVELS; l++) {
            int top = (H >> l) - (l == 0 ? 1 : 0);   /* 1023, 512, 256, ... 2 */
            if (l == WT_LEVELS - 1)
                top = 1;
            std::fill(a.begin(), a.end(), cpx(0.0, 0.0));
            for (int n = 1; n <= top && n < H; n++) {
                a[(size_t)n] = sp[(size_t)w].c[(size_t)n] * 0.5;
                a[(size_t)(WT_N - n)] = std::conj(a[(size_t)n]);
            }
            fft(a, true);
            float *t = b->data.data() + ((size_t)w * WT_LEVELS + (size_t)l) * (WT_N + 1);
            for (int i = 0; i < WT_N; i++)
                t[i] = (float)a[(size_t)i].real();
            t[WT_N] = t[0];
            if (l == 0) {                /* the level: peak 1 at most, about the loudness of a saw */
                double peak = 1e-9, rms = 0.0;
                for (int i = 0; i < WT_N; i++) {
                    peak = std::max(peak, (double)std::fabs(t[i]));
                    rms += (double)t[i] * t[i];
                }
                rms = std::sqrt(rms / WT_N) + 1e-9;
                scale = (float)std::min(0.98 / peak, 0.6 / rms);
            }
            for (int i = 0; i <= WT_N; i++)
                t[i] *= scale;
            b->waves[w].level[l] = t;
        }
    }
    return b;
}
} // namespace

const Wave &wave(int w)
{
    static Built *built = nullptr;       /* (never freed: plain memory, shared while the plugin is loaded) */
    static std::once_flag once;
    std::call_once(once, [] { built = build(); });
    return built->waves[w < 0 ? 0 : w >= NWAVES ? NWAVES - 1 : w];
}
} // namespace nq
