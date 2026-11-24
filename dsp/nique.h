/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* NIQUE D0: a virtual-analogue lead synth after the idea of a famous red Swedish one with an A and a 1 in its name.
 *
 * One synth part (in a DAW: one instance per part), 24 voices (Unison takes 2 or 4 a note):
 *   OSCILLATORS    47 waveforms (analogue, pulses, drawbars, bells, digital, electric, formants) and the
 *                  configurations PITCH, SHAPE, SYNC, DETUNE, the mixes (a sine, triangle, saw or square on a harmonic),
 *                  FM at a ratio, AM and NOISE, each set by OSC CTRL; the LFO and the mod envelope move OSC CTRL
 *   FILTER         LP12, LP24, LP M (a transistor ladder), LP TB (a diode ladder), BP, HP; drive after it, KB track
 *   ENVELOPES      the mod envelope and the amp envelope, ADR (decay at its end: ASR), velocity, inverted mod env
 *   LFO            square, saw, inverted saw, triangle, S/H; the host's tempo (MST CLK); or a per-voice envelope
 *   VOICE          poly, mono, legato, unison 1-3, glide, vibrato (delayed or on the wheel), bend range, octave
 *   ARPEGGIATOR    up, down, up/down, random over 1-4 octaves, free or on the host's tempo; HOLD
 *   EFFECTS        flanger, phaser, ring mod, chorus, ensemble, drive; a delay (ping pong, analogue mode); a reverb
 *   MORPH          the mod wheel and the key velocity each take 25 parameters to values of their own
 *
 * The voices run at twice the host's rate (up to 48 kHz; above it at the host's), the effects at the host's.
 * Every parameter is a float: K_CONT and K_BIP 0..1, K_SW 0 or 1, K_CH the choice's index. The engine runs on a grid
 * of 16 samples (control rate), so any buffer size renders the same. All calls from the audio thread except prepare.
 * The JUCE plugin is in src/; tests/ checks the engine without it. */
#pragma once
#include <cstdint>
#include <memory>

namespace nq
{
enum Kind { K_CONT, K_BIP, K_SW, K_CH };

#define P(e, id, name, shortName, kind, ch, def) e,
enum ParamId {
#include "params_list.inc"
    NPARAMS
};
#undef P

struct ParamDef {
    const char *id, *name, *shortName;
    int kind;
    const char *const *choices;          /* K_CH: the names */
    int nChoices;                        /* K_CH: how many; K_SW: 2; else 0 */
    float def;
};
extern const ParamDef PARAMS[NPARAMS];
int paramIndex(const char *id);          /* -1: none */
void paramText(int i, float v, char *out, int n);   /* the value as the panel and the host show it */
float paramMax(int i);                   /* 1 (K_CONT, K_BIP, K_SW) or nChoices - 1 */

/* the morphs: target parameter, its wheel parameter, its velocity parameter (-1: none) */
struct MorphDef {
    int target, wheel, vel;
};
constexpr int NMORPH = 25;
extern const MorphDef MORPHS[NMORPH];
int morphOf(int param);                  /* the morph whose target is this parameter, -1: none */

/* the waveforms' and configurations' groups (for the panel's menus) */
const char *waveLongName(int w);         /* "A6  EXPONENTIAL SAW" */
const char *configLongName(int c);

/* the factory programs */
int numPresets();
const char *presetName(int k);
const char *presetGroup(int k);          /* "LEAD", "BASS", ... */
void presetValues(int k, float *out);    /* NPARAMS values */

/* the sound's own tools (on a whole set of values; the morphs: which one to change) */
enum MorphSel { MS_NONE, MS_WHEEL, MS_VEL };
void initSound(float *v);                /* INIT SOUND: a plain saw through an open filter */
void mutate(float *v, uint32_t seed, MorphSel m);      /* a near relative of the sound (or of its morph) */
void randomize(float *v, uint32_t seed, MorphSel m);   /* anything at all (but playable) */

struct Impl;
class Engine
{
public:
    Engine();
    ~Engine();
    void prepare(double sampleRate);     /* a fresh engine (not on the audio thread: it sizes the delay lines) */
    void set(int i, float v);            /* a parameter */
    float get(int i) const;
    void noteOn(int note, float vel);    /* vel 0..1 */
    void noteOff(int note);
    void sustain(bool down);
    void allNotesOff();
    void panic();                        /* silent at once: the voices, the effects' tails */
    void bend(float v);                  /* -1..1 */
    void modWheel(float v);              /* 0..1: the wheel's morph (and vibrato on WHEEL) */
    void tempo(double bpm);
    void position(double ppq, bool playing);
    void render(float *left, float *right, int n);

    /* for the panel (read on another thread: approximate) */
    float lfoLevel() const;              /* 0..1 */
    float outLevel() const;              /* 0..1, the last block's peak */
    int voicesSounding() const;
    float wheel() const;

private:
    std::unique_ptr<Impl> d;
};
} // namespace nq
