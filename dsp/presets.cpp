/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The factory programs: each INIT SOUND with its own settings, written "id=value" (a choice by its name, spaces as
 * "_"; a switch on/off; a knob 0..1). The morphs: mw_<target> (the wheel), mv_<target> (velocity), 0.5 none. */
#include "nique.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace nq
{
namespace
{
struct Preset {
    const char *group, *name, *spec;
};

const Preset PRESETS[] = {
    {"INIT", "INIT SOUND", ""},
    /* ------------------------------------------------------------------------------------------------ leads */
    {"LEAD", "SYNC LEAD",
     "osc_wave=SAW osc_config=SYNC osc_ctrl=0.32 osc_menv=0.42 menv_d=0.5 flt_type=LP24 flt_freq=0.76 flt_res=0.18 "
     "flt_menv=0.12 voice_mode=LEGATO glide=0.22 dly_on=on dly_sync=on dly_div=1/8D dly_mix=0.22 dly_fb=2 "
     "mw_osc_ctrl=0.85 aenv_r=0.28"},
    {"LEAD", "A1 SOLO",
     "osc_wave=PULSE osc_config=SHAPE osc_ctrl=0.3 osc_lfo=0.45 lfo_wave=TRIANGLE lfo_rate=0.38 flt_type=LP_M "
     "flt_freq=0.6 flt_res=0.32 flt_menv=0.28 menv_d=0.46 voice_mode=LEGATO glide=0.18 vibrato=WHEEL "
     "dly_on=on dly_sync=on dly_div=1/8D dly_mix=0.2 mw_flt_freq=0.75 mv_flt_freq=0.65"},
    {"LEAD", "SUPERSAW LEAD",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.24 unison=2 flt_type=LP24 flt_freq=0.78 flt_res=0.1 flt_menv=0.1 "
     "voice_mode=MONO glide=0.12 dly_on=on dly_ping=on dly_sync=on dly_div=1/4D dly_mix=0.24 rev_on=on "
     "rev_type=HALL_1 rev_mix=0.2 mw_flt_freq=0.7"},
    {"LEAD", "SQUARE LEAD",
     "osc_wave=PULSE osc_config=PITCH flt_type=LP12 flt_freq=0.64 flt_res=0.25 flt_menv=0.22 menv_d=0.42 "
     "voice_mode=MONO glide=0.14 vibrato=DLY_1 vib_amt=0.4 flt_drive=0.22 rev_on=on rev_type=STAGE_1 rev_mix=0.15 "
     "mw_flt_freq=0.75"},
    {"LEAD", "FM LEAD",
     "osc_wave=SINE osc_config=FM_2 osc_ctrl=0.34 osc_menv=0.3 menv_d=0.47 flt_type=LP24 flt_freq=0.88 "
     "voice_mode=LEGATO glide=0.15 dly_on=on dly_sync=on dly_div=1/8 dly_mix=0.2 mw_osc_ctrl=0.8 mv_osc_ctrl=0.65"},
    {"LEAD", "TB ACID",
     "osc_wave=SAW osc_config=PITCH flt_type=LP_TB flt_freq=0.34 flt_res=0.85 flt_menv=0.45 menv_d=0.38 "
     "aenv_r=0.12 voice_mode=LEGATO glide=0.12 flt_drive=0.3 mw_flt_freq=0.8 mv_flt_menv=0.7"},
    {"LEAD", "WHISTLE",
     "osc_wave=SINE osc_config=PITCH flt_type=LP12 flt_freq=0.9 aenv_a=0.32 voice_mode=MONO glide=0.2 "
     "vibrato=DLY_1 vib_amt=0.45 vib_rate=0.5 rev_on=on rev_type=HALL_2 rev_mix=0.3 dly_on=on dly_mix=0.18"},
    {"LEAD", "VOX LEAD",
     "osc_wave=F1 osc_config=PITCH flt_type=LP12 flt_freq=0.78 voice_mode=LEGATO glide=0.16 vibrato=DLY_2 "
     "vib_amt=0.45 aenv_a=0.2 rev_on=on rev_type=HALL_1 rev_mix=0.25 mw_osc_ctrl=0.5"},
    {"LEAD", "SAW SCREAM",
     "osc_wave=A6 osc_config=SHAPE osc_ctrl=0.4 flt_type=LP_M flt_freq=0.62 flt_res=0.55 flt_menv=0.2 menv_d=0.5 "
     "flt_drive=0.45 voice_mode=MONO glide=0.1 fx_on=on fx_type=DRIVE fx_amt=0.3 dly_on=on dly_mix=0.15 "
     "mw_flt_res=0.8 mw_osc_ctrl=0.8"},
    /* ------------------------------------------------------------------------------------------------- bass */
    {"BASS", "FAT BASS",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.16 flt_type=LP_M flt_freq=0.32 flt_res=0.25 flt_menv=0.4 "
     "menv_d=0.4 aenv_r=0.15 voice_mode=MONO mv_flt_menv=0.7 mw_flt_freq=0.75"},
    {"BASS", "SUB BASS",
     "osc_wave=SINE osc_config=SIN_SUB osc_ctrl=0.55 flt_type=LP24 flt_freq=0.45 flt_menv=0.05 aenv_r=0.15 "
     "voice_mode=MONO"},
    {"BASS", "PLUCK BASS",
     "osc_wave=PULSE osc_config=PITCH flt_type=LP24 flt_freq=0.28 flt_res=0.35 flt_menv=0.5 menv_d=0.33 "
     "menv_vel=on aenv_d=0.56 aenv_r=0.18 aenv_vel=on voice_mode=MONO"},
    {"BASS", "FM BASS",
     "osc_wave=SINE osc_config=FM_1 osc_ctrl=0.45 osc_menv=0.4 menv_d=0.36 flt_type=LP24 flt_freq=0.6 "
     "aenv_r=0.15 voice_mode=MONO mv_osc_ctrl=0.7"},
    {"BASS", "ACID BASS",
     "osc_wave=PULSE osc_config=PITCH flt_type=LP_TB flt_freq=0.3 flt_res=0.75 flt_menv=0.5 menv_d=0.3 "
     "flt_drive=0.4 aenv_r=0.1 voice_mode=LEGATO glide=0.1 mw_flt_freq=0.8"},
    {"BASS", "SYNC BASS",
     "osc_wave=SAW osc_config=SYNC osc_ctrl=0.08 osc_menv=0.45 menv_d=0.32 flt_type=LP_M flt_freq=0.45 "
     "flt_menv=0.3 aenv_r=0.15 voice_mode=MONO"},
    {"BASS", "REESE",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.12 unison=1 flt_type=LP24 flt_freq=0.42 flt_res=0.2 "
     "voice_mode=LEGATO glide=0.08 fx_on=on fx_type=PHASER fx_amt=0.12 mw_flt_freq=0.8"},
    {"BASS", "ORGAN BASS",
     "osc_wave=O1 osc_config=PITCH flt_type=LP12 flt_freq=0.5 aenv_r=0.1 voice_mode=MONO"},
    {"BASS", "WOBBLE BASS",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.14 flt_type=LP_M flt_freq=0.3 flt_res=0.45 flt_menv=0.1 "
     "flt_drive=0.35 voice_mode=LEGATO glide=0.1 l2_sync=on l2_div=1/8 l2_wave=TRIANGLE l2_dest=FILTER l2_amt=0.7 "
     "mw_flt_drive=0.8"},
    /* ------------------------------------------------------------------------------------------------- pads */
    {"PAD", "STRING PAD",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.26 flt_type=LP12 flt_freq=0.62 flt_res=0.1 flt_menv=0.1 "
     "aenv_a=0.55 aenv_r=0.58 fx_on=on fx_type=ENSEMBLE fx_amt=0.7 rev_on=on rev_type=HALL_1 rev_mix=0.35 "
     "mw_flt_freq=0.7"},
    {"PAD", "WARM PAD",
     "osc_wave=PULSE osc_config=SHAPE osc_ctrl=0.2 osc_lfo=0.32 lfo_rate=0.35 flt_type=LP24 flt_freq=0.55 "
     "aenv_a=0.6 aenv_r=0.62 fx_on=on fx_type=CHORUS fx_amt=0.6 rev_on=on rev_type=HALL_1 rev_mix=0.4"},
    {"PAD", "GLASS PAD",
     "osc_wave=D8 osc_config=DETUNE osc_ctrl=0.2 flt_type=LP12 flt_freq=0.8 aenv_a=0.5 aenv_r=0.65 "
     "fx_on=on fx_type=CHORUS fx_amt=0.5 rev_on=on rev_type=HALL_2 rev_mix=0.45 dly_on=on dly_ping=on "
     "dly_sync=on dly_div=1/4D dly_mix=0.2"},
    {"PAD", "SWEEP PAD",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.25 flt_type=LP24 flt_freq=0.36 flt_res=0.45 flt_lfo=0.4 "
     "lfo_rate=0.22 aenv_a=0.5 aenv_r=0.6 fx_on=on fx_type=PHASER fx_amt=0.15 rev_on=on rev_type=HALL_2 "
     "rev_mix=0.4"},
    {"PAD", "MOVING PAD",
     "osc_wave=A4 osc_config=DETUNE osc_ctrl=0.24 flt_type=LP24 flt_freq=0.5 flt_res=0.3 aenv_a=0.55 aenv_r=0.62 "
     "l2_rate=0.2 l2_wave=TRIANGLE l2_dest=FILTER l2_amt=0.35 l3_rate=0.32 l3_wave=TRIANGLE l3_dest=PAN "
     "l3_amt=0.7 l4_rate=0.14 l4_wave=S/H l4_dest=OSC_CTRL l4_amt=0.3 fx_on=on fx_type=CHORUS fx_amt=0.5 "
     "rev_on=on rev_type=HALL_2 rev_mix=0.4"},
    {"PAD", "VOX PAD",
     "osc_wave=F3 osc_config=DETUNE osc_ctrl=0.22 flt_type=LP12 flt_freq=0.7 aenv_a=0.55 aenv_r=0.6 "
     "fx_on=on fx_type=ENSEMBLE fx_amt=0.6 rev_on=on rev_type=HALL_2 rev_mix=0.4 mw_osc_ctrl=0.65"},
    {"PAD", "DARK MATTER",
     "osc_wave=A4 osc_config=SAW_SUB osc_ctrl=0.6 flt_type=LP_M flt_freq=0.38 flt_res=0.4 flt_lfo=0.25 "
     "lfo_rate=0.15 aenv_a=0.62 aenv_r=0.7 fx_on=on fx_type=FLANGER fx_amt=0.12 rev_on=on rev_type=HALL_1 "
     "rev_mix=0.5"},
    /* ---------------------------------------------------------------------------------------------- strings */
    {"STRINGS", "PWM STRINGS",
     "osc_wave=PULSE osc_config=SHAPE osc_ctrl=0.3 osc_lfo=0.42 lfo_rate=0.38 flt_type=LP12 flt_freq=0.63 "
     "aenv_a=0.45 aenv_r=0.55 fx_on=on fx_type=ENSEMBLE fx_amt=0.8 rev_on=on rev_type=HALL_1 rev_mix=0.3 "
     "mv_aenv_a=0.35"},
    {"STRINGS", "SOLINA",
     "osc_wave=SAW osc_config=SAW_2 osc_ctrl=0.5 flt_type=LP12 flt_freq=0.68 aenv_a=0.42 aenv_r=0.5 "
     "fx_on=on fx_type=ENSEMBLE fx_amt=1.0 rev_on=on rev_type=STAGE_1 rev_mix=0.3"},
    {"STRINGS", "SLOW STRINGS",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.3 flt_type=LP24 flt_freq=0.5 flt_menv=0.2 menv_a=0.6 menv_d=0.7 "
     "aenv_a=0.62 aenv_r=0.66 vibrato=DLY_2 vib_amt=0.25 fx_on=on fx_type=CHORUS fx_amt=0.5 rev_on=on "
     "rev_type=HALL_2 rev_mix=0.4"},
    /* ------------------------------------------------------------------------------------------------ brass */
    {"BRASS", "BRASS SECTION",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.18 flt_type=LP24 flt_freq=0.44 flt_res=0.1 flt_menv=0.32 "
     "menv_a=0.38 menv_d=0.52 aenv_a=0.3 aenv_r=0.35 fx_on=on fx_type=CHORUS fx_amt=0.35 rev_on=on "
     "rev_type=STAGE_1 rev_mix=0.2 mv_flt_freq=0.7"},
    {"BRASS", "SYNTH BRASS",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.15 unison=1 flt_type=LP_M flt_freq=0.4 flt_menv=0.36 "
     "menv_a=0.32 menv_d=0.55 aenv_r=0.3 mv_flt_freq=0.75 rev_on=on rev_type=STAGE_2 rev_mix=0.2"},
    {"BRASS", "STAB",
     "osc_wave=A2 osc_config=DETUNE osc_ctrl=0.14 flt_type=LP24 flt_freq=0.38 flt_menv=0.45 menv_d=0.4 "
     "aenv_d=0.58 aenv_r=0.3 rev_on=on rev_type=ROOM rev_mix=0.25"},
    /* ------------------------------------------------------------------------------------------------- keys */
    {"KEYS", "E-PIANO",
     "osc_wave=E1 osc_config=PITCH flt_type=LP12 flt_freq=0.62 aenv_d=0.72 aenv_r=0.4 aenv_vel=on "
     "mv_flt_freq=0.8 fx_on=on fx_type=PHASER fx_amt=0.2 rev_on=on rev_type=ROOM rev_mix=0.2"},
    {"KEYS", "WURLY",
     "osc_wave=E2 osc_config=PITCH flt_type=LP12 flt_freq=0.6 aenv_d=0.66 aenv_r=0.3 aenv_vel=on mv_flt_freq=0.8 "
     "fx_on=on fx_type=CHORUS fx_amt=0.4"},
    {"KEYS", "CLAVINET",
     "osc_wave=E3 osc_config=PITCH flt_type=LP12 flt_freq=0.72 flt_res=0.2 flt_menv=0.2 menv_d=0.3 "
     "aenv_d=0.55 aenv_r=0.12 aenv_vel=on fx_on=on fx_type=PHASER fx_amt=0.3"},
    {"KEYS", "FM PIANO",
     "osc_wave=SINE osc_config=FM_1 osc_ctrl=0.36 osc_menv=0.35 menv_d=0.45 menv_vel=on aenv_d=0.7 aenv_r=0.35 "
     "aenv_vel=on rev_on=on rev_type=STAGE_2 rev_mix=0.2"},
    {"KEYS", "DX KEYS",
     "osc_wave=SINE osc_config=FM_3.5 osc_ctrl=0.25 osc_menv=0.35 menv_d=0.4 menv_vel=on aenv_d=0.66 "
     "aenv_r=0.35 aenv_vel=on fx_on=on fx_type=CHORUS fx_amt=0.4 rev_on=on rev_type=HALL_1 rev_mix=0.2"},
    /* ------------------------------------------------------------------------------------------------ organ */
    {"ORGAN", "JIMMY ORGAN",
     "out_level=0.6 "
     "osc_wave=O3 osc_config=PITCH flt_type=LP12 flt_freq=0.92 aenv_r=0.1 fx_on=on fx_type=ENSEMBLE fx_amt=0.35 "
     "vibrato=OFF mw_fx_amt=0.9"},
    {"ORGAN", "GOSPEL ORGAN",
     "out_level=0.6 "
     "osc_wave=O5 osc_config=PITCH flt_type=LP12 flt_freq=0.9 aenv_r=0.12 fx_on=on fx_type=DRIVE fx_amt=0.25 "
     "rev_on=on rev_type=ROOM rev_mix=0.2 vibrato=OFF"},
    {"ORGAN", "CHURCH ORGAN",
     "out_level=0.6 "
     "osc_wave=O8 osc_config=PITCH flt_type=LP12 flt_freq=0.85 aenv_a=0.25 aenv_r=0.45 rev_on=on "
     "rev_type=HALL_2 rev_mix=0.45 vibrato=OFF"},
    /* ------------------------------------------------------------------------------------------------ bells */
    {"BELLS", "BELL TREE",
     "osc_wave=B1 osc_config=PITCH flt_type=LP12 flt_freq=0.9 aenv_d=0.78 aenv_r=0.6 rev_on=on rev_type=HALL_2 "
     "rev_mix=0.4 dly_on=on dly_ping=on dly_sync=on dly_div=1/4D dly_mix=0.25"},
    {"BELLS", "METALLOPHONE",
     "osc_wave=B4 osc_config=PITCH flt_type=LP12 flt_freq=0.8 aenv_d=0.62 aenv_r=0.5 aenv_vel=on rev_on=on "
     "rev_type=STAGE_2 rev_mix=0.3"},
    {"BELLS", "TUBULAR",
     "osc_wave=B5 osc_config=PITCH flt_type=LP12 flt_freq=0.85 aenv_d=0.82 aenv_r=0.7 rev_on=on rev_type=HALL_1 "
     "rev_mix=0.35"},
    {"BELLS", "FM BELL",
     "osc_wave=SINE osc_config=FM_2.76 osc_ctrl=0.35 osc_menv=0.25 menv_d=0.55 aenv_d=0.78 aenv_r=0.6 aenv_vel=on "
     "rev_on=on rev_type=HALL_2 rev_mix=0.35"},
    /* ----------------------------------------------------------------------------------------------- plucks */
    {"PLUCK", "POLY PLUCK",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.15 flt_type=LP24 flt_freq=0.3 flt_res=0.3 flt_menv=0.55 "
     "menv_d=0.35 aenv_d=0.56 aenv_r=0.4 dly_on=on dly_sync=on dly_div=1/8D dly_mix=0.3 rev_on=on "
     "rev_type=HALL_1 rev_mix=0.2 mv_flt_menv=0.7"},
    {"PLUCK", "HARP",
     "osc_wave=TRI osc_config=PITCH flt_type=LP12 flt_freq=0.55 flt_menv=0.4 menv_d=0.4 aenv_d=0.6 aenv_r=0.55 "
     "aenv_vel=on rev_on=on rev_type=HALL_1 rev_mix=0.35"},
    {"PLUCK", "DIGI PLUCK",
     "osc_wave=D3 osc_config=PITCH flt_type=LP12 flt_freq=0.45 flt_menv=0.5 menv_d=0.3 aenv_d=0.5 aenv_r=0.35 "
     "dly_on=on dly_ping=on dly_sync=on dly_div=1/8 dly_mix=0.3"},
    {"PLUCK", "KALIMBA",
     "osc_wave=B3 osc_config=PITCH flt_type=LP12 flt_freq=0.7 aenv_d=0.5 aenv_r=0.4 aenv_vel=on rev_on=on "
     "rev_type=ROOM rev_mix=0.25"},
    /* ---------------------------------------------------------------------------------------------- choir */
    {"VOX", "CHOIR AAH",
     "osc_wave=F1 osc_config=DETUNE osc_ctrl=0.2 unison=1 flt_type=LP12 flt_freq=0.75 aenv_a=0.5 aenv_r=0.6 "
     "fx_on=on fx_type=ENSEMBLE fx_amt=0.6 rev_on=on rev_type=HALL_1 rev_mix=0.4"},
    {"VOX", "OOH",
     "osc_wave=F3 osc_config=DETUNE osc_ctrl=0.18 flt_type=LP12 flt_freq=0.7 aenv_a=0.45 aenv_r=0.55 "
     "vibrato=DLY_2 vib_amt=0.25 fx_on=on fx_type=CHORUS fx_amt=0.5 rev_on=on rev_type=HALL_2 rev_mix=0.4"},
    /* ------------------------------------------------------------------------------------------------ arps */
    {"ARP", "ARP SEQ",
     "osc_wave=PULSE osc_config=SHAPE osc_ctrl=0.25 flt_type=LP_M flt_freq=0.35 flt_res=0.55 flt_menv=0.4 "
     "menv_d=0.3 aenv_d=0.5 aenv_r=0.25 arp_run=on arp_sync=on arp_div=16 arp_range=2_OCT dly_on=on dly_sync=on "
     "dly_div=1/8D dly_mix=0.3 mw_flt_freq=0.8"},
    {"ARP", "ARP BELLS",
     "osc_wave=B2 osc_config=PITCH flt_type=LP12 flt_freq=0.85 aenv_d=0.6 aenv_r=0.4 arp_run=on arp_sync=on "
     "arp_div=16 arp_range=3_OCT arp_dir=RANDOM dly_on=on dly_ping=on dly_sync=on dly_div=1/8D dly_mix=0.3 "
     "rev_on=on rev_type=HALL_2 rev_mix=0.3"},
    {"ARP", "TRANCE GATE",
     "osc_wave=SAW osc_config=DETUNE osc_ctrl=0.22 unison=1 flt_type=LP24 flt_freq=0.55 flt_lfo=0.3 "
     "lfo_wave=SQUARE lfo_sync=on lfo_div=1/16 aenv_r=0.2 dly_on=on dly_ping=on dly_sync=on dly_div=1/8D "
     "dly_mix=0.25 rev_on=on rev_type=HALL_1 rev_mix=0.25"},
    /* --------------------------------------------------------------------------------------------- effects */
    {"FX", "S/H BLEEPS",
     "osc_wave=SAW osc_config=PITCH flt_type=LP24 flt_freq=0.42 flt_res=0.65 flt_lfo=0.5 lfo_wave=S/H "
     "lfo_sync=on lfo_div=1/16 dly_on=on dly_ping=on dly_sync=on dly_div=1/8D dly_mix=0.3 rev_on=on "
     "rev_type=HALL_1 rev_mix=0.25"},
    {"FX", "NOISE SWEEP",
     "osc_wave=SAW osc_config=NOISE osc_ctrl=0.85 flt_type=BP flt_freq=0.42 flt_res=0.7 flt_lfo=0.45 "
     "lfo_rate=0.2 aenv_a=0.5 aenv_r=0.7 rev_on=on rev_type=HALL_2 rev_mix=0.5"},
    {"FX", "LASER",
     "osc_wave=SINE osc_config=PITCH osc_ctrl=0.75 osc_menv=0.8 menv_inv=on menv_d=0.35 flt_type=LP12 "
     "flt_freq=0.9 aenv_d=0.45 aenv_r=0.3 dly_on=on dly_ping=on dly_mix=0.3"},
    {"FX", "RING THING",
     "osc_wave=SAW osc_config=AM osc_ctrl=0.55 osc_lfo=0.3 lfo_rate=0.3 flt_type=LP24 flt_freq=0.7 aenv_d=0.7 "
     "aenv_r=0.5 rev_on=on rev_type=STAGE_2 rev_mix=0.3"},
};
constexpr int N_PRESETS = (int)(sizeof(PRESETS) / sizeof(PRESETS[0]));

/* "LP_M" against "LP M" (any case) */
bool sameName(const std::string &a, const char *b)
{
    size_t n = std::strlen(b);
    if (a.size() != n)
        return false;
    for (size_t i = 0; i < n; i++) {
        char x = a[i] == '_' ? ' ' : (char)std::toupper((unsigned char)a[i]);
        char y = (char)std::toupper((unsigned char)b[i]);
        if (x != y)
            return false;
    }
    return true;
}

void apply(float *v, const char *spec)
{
    std::string s(spec);
    size_t at = 0;
    while (at < s.size()) {
        while (at < s.size() && s[at] == ' ')
            at++;
        size_t end = s.find(' ', at);
        if (end == std::string::npos)
            end = s.size();
        std::string tok = s.substr(at, end - at);
        at = end;
        size_t eq = tok.find('=');
        if (eq == std::string::npos)
            continue;
        std::string id = tok.substr(0, eq), value = tok.substr(eq + 1);
        int p = paramIndex(id.c_str());
        if (p < 0)
            continue;
        const ParamDef &d = PARAMS[p];
        if (d.kind == K_CH) {
            int k = -1;
            for (int c = 0; c < d.nChoices; c++)
                if (sameName(value, d.choices[c]))
                    k = c;
            if (k < 0)
                k = std::atoi(value.c_str());
            v[p] = (float)k;
        } else if (d.kind == K_SW)
            v[p] = (value == "on" || value == "1") ? 1.0f : 0.0f;
        else
            v[p] = (float)std::atof(value.c_str());
    }
}
} // namespace

int numPresets() { return N_PRESETS; }
const char *presetName(int k) { return k >= 0 && k < N_PRESETS ? PRESETS[k].name : ""; }
const char *presetGroup(int k) { return k >= 0 && k < N_PRESETS ? PRESETS[k].group : ""; }

void presetValues(int k, float *out)
{
    initSound(out);
    if (k >= 0 && k < N_PRESETS)
        apply(out, PRESETS[k].spec);
}

/* (for the tests: every token of every program names a parameter and a value it has) */
int presetProblems(char *msg, int n)
{
    int bad = 0;
    for (int k = 0; k < N_PRESETS; k++) {
        std::string s(PRESETS[k].spec);
        size_t at = 0;
        while (at < s.size()) {
            while (at < s.size() && s[at] == ' ')
                at++;
            size_t end = s.find(' ', at);
            if (end == std::string::npos)
                end = s.size();
            std::string tok = s.substr(at, end - at);
            at = end;
            if (tok.empty())
                continue;
            size_t eq = tok.find('=');
            int p = eq == std::string::npos ? -1 : paramIndex(tok.substr(0, eq).c_str());
            bool ok = p >= 0;
            if (ok && PARAMS[p].kind == K_CH) {
                ok = false;
                for (int c = 0; c < PARAMS[p].nChoices; c++)
                    if (sameName(tok.substr(eq + 1), PARAMS[p].choices[c]))
                        ok = true;
            }
            if (!ok) {
                if (bad == 0 && msg)
                    std::snprintf(msg, (size_t)n, "%s: %s", PRESETS[k].name, tok.c_str());
                bad++;
            }
        }
    }
    return bad;
}
} // namespace nq
