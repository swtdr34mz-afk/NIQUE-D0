#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 swtdr34mz (NIQUE D0)
"""Writes dsp/params_list.inc and dsp/choices.inc: NIQUE D0's parameters, one line each, in their permanent order.

    python3 tools/gen_params.py

The order is the plugin's parameter order and the ids are kept in projects and presets: never reorder or rename,
only append at the end.
Each line: P(ENUM, "id", "LONG NAME", "Short name", kind, choices, default)
  kind: K_CONT (0..1), K_BIP (0..1, centre 0.5), K_SW (0/1), K_CH (0..n-1 of the named choice list)
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
rows = []
choices = {}


def CH(name, items):
    choices[name] = items
    return name


def P(enum, pid, name, short, kind="K_CONT", ch="0", default=0.0):
    assert len(short) <= 12, short
    if kind == "K_CH":
        assert ch in choices, ch
        assert 0 <= default < len(choices[ch]), (enum, default)
    rows.append((enum, pid, name, short, kind, ch, repr(float(default))))


# ---------------------------------------------------------------------------------------------- choice lists ---
# the 47 waveforms: the four on the panel, then the extended analogue ones, fixed pulse widths, drawbar organs,
# bells and metal bars, digital spectra, electric pianos and clavinets, vocal formants (the display's names)
WAVES = ["PULSE", "SAW", "TRI", "SINE",
         "A1", "A2", "A3", "A4", "A5", "A6", "A7",
         "P1", "P2", "P3",
         "O1", "O2", "O3", "O4", "O5", "O6", "O7", "O8", "O9",
         "B1", "B2", "B3", "B4", "B5",
         "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8",
         "E1", "E2", "E3", "E4",
         "F1", "F2", "F3", "F4", "F5", "F6", "F7"]
assert len(WAVES) == 47
CH("CH_WAVE", WAVES)

# the oscillator configurations: one oscillator (PITCH, SHAPE, SYNC), two (DETUNE, the mixes: a sine, triangle,
# saw or square on a harmonic of oscillator 1), the modulations (FM at a ratio, AM) and NOISE
CFG = ["PITCH", "SHAPE", "SYNC", "DETUNE"]
HARM = ["SUB", "1", "2", "3", "4", "5", "6", "7", "8"]
for w in ("SIN", "TRI", "SAW", "SQR"):
    for h in HARM:
        CFG.append("%s %s" % (w, h))
FM_RATIOS = ["0.5", "1", "1.41", "1.5", "2", "2.76", "3", "3.5", "4", "5", "6", "7", "8"]
for r in FM_RATIOS:
    CFG.append("FM " + r)
CFG += ["AM", "NOISE"]
CH("CH_CONFIG", CFG)

CH("CH_ARP_DIV", ["2", "4", "8", "16", "32", "2T", "4T", "8T", "16T"])
CH("CH_ARP_RANGE", ["1 OCT", "2 OCT", "3 OCT", "4 OCT"])
CH("CH_ARP_DIR", ["UP", "DOWN", "UP/DOWN", "RANDOM"])
CH("CH_VOICE", ["POLY", "MONO", "LEGATO"])
CH("CH_UNISON", ["OFF", "1", "2", "3"])
CH("CH_VIBRATO", ["OFF", "DLY 1", "DLY 2", "WHEEL"])
CH("CH_BEND", ["OFF", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "24", "48", "+2/-12", "+2/-24"])
CH("CH_OCTAVE", ["-2", "-1", "0", "+1", "+2"])
CH("CH_LFO_WAVE", ["SQUARE", "SAW", "INV SAW", "TRIANGLE", "S/H"])
CH("CH_LFO_DIV", ["4 BARS", "2 BARS", "1 BAR", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64",
                  "4 BARS T", "2 BARS T", "1 BAR T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"])
CH("CH_FILTER", ["LP12", "LP24", "LP M", "LP TB", "BP", "HP"])
CH("CH_KBTRACK", ["OFF", "1/3", "2/3", "1"])
CH("CH_FX", ["FLANGER", "PHASER", "RING MOD", "CHORUS", "ENSEMBLE", "DRIVE"])
CH("CH_DLY_DIV", ["1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/4D", "1/8D", "1/16D",
                  "1/2T", "1/4T", "1/8T", "1/16T", "1/32T", "1/4S", "1/8S", "1/16S"])
CH("CH_FEEDBACK", ["1", "2", "3", "4"])
CH("CH_REVERB", ["ROOM", "STAGE 1", "STAGE 2", "HALL 1", "HALL 2"])
CH("CH_LFO_DEST", ["PITCH", "OSC CTRL", "FILTER", "RESONANCE", "DRIVE", "AMP", "PAN"])

# ------------------------------------------------------------------------------------------------ parameters ---
P("OUT_LEVEL", "out_level", "OUTPUT LEVEL", "Level", default=0.7)
P("OUT_PAN", "out_pan", "OUTPUT PAN", "Pan", "K_BIP", default=0.5)

# the arpeggiator
P("ARP_RUN", "arp_run", "ARP RUN", "Arp Run", "K_SW", default=0)
P("ARP_RATE", "arp_rate", "ARP RATE", "Arp Rate", default=0.45)
P("ARP_SYNC", "arp_sync", "ARP MST CLK", "Arp MstClk", "K_SW", default=1)
P("ARP_DIV", "arp_div", "ARP CLOCK DIV", "Arp Div", "K_CH", "CH_ARP_DIV", default=3)
P("ARP_RANGE", "arp_range", "ARP RANGE", "Arp Range", "K_CH", "CH_ARP_RANGE", default=0)
P("ARP_DIR", "arp_dir", "ARP DIRECTION", "Arp Dir", "K_CH", "CH_ARP_DIR", default=0)
P("HOLD", "hold", "HOLD", "Hold", "K_SW", default=0)

# the voices
P("VOICE_MODE", "voice_mode", "VOICE MODE", "Voice Mode", "K_CH", "CH_VOICE", default=0)
P("UNISON", "unison", "UNISON", "Unison", "K_CH", "CH_UNISON", default=0)
P("GLIDE", "glide", "GLIDE", "Glide", default=0.0)
P("VIBRATO", "vibrato", "VIBRATO", "Vibrato", "K_CH", "CH_VIBRATO", default=3)
P("VIB_RATE", "vib_rate", "VIBRATO RATE", "Vib Rate", default=0.45)
P("VIB_AMT", "vib_amt", "VIBRATO AMOUNT", "Vib Amount", default=0.35)
P("BEND", "bend", "BEND RANGE", "Bend Range", "K_CH", "CH_BEND", default=2)
P("OCTAVE", "octave", "OCTAVE SHIFT", "Octave", "K_CH", "CH_OCTAVE", default=2)

# the LFO (and its envelope mode)
P("LFO_RATE", "lfo_rate", "LFO RATE/TIME", "LFO Rate", default=0.5)
P("LFO_SYNC", "lfo_sync", "LFO MST CLK", "LFO MstClk", "K_SW", default=0)
P("LFO_DIV", "lfo_div", "LFO CLOCK DIV", "LFO Div", "K_CH", "CH_LFO_DIV", default=4)
P("LFO_WAVE", "lfo_wave", "LFO WAVEFORM", "LFO Wave", "K_CH", "CH_LFO_WAVE", default=3)
P("LFO_ENV", "lfo_env", "LFO ENVELOPE MODE", "LFO Env", "K_SW", default=0)

# the modulation envelope (ADR / ASR)
P("MENV_A", "menv_a", "MOD ENV ATTACK", "ModEnv Att", default=0.0)
P("MENV_D", "menv_d", "MOD ENV DECAY", "ModEnv Dec", default=0.45)
P("MENV_R", "menv_r", "MOD ENV RELEASE", "ModEnv Rel", default=0.3)
P("MENV_INV", "menv_inv", "MOD ENV INVERT", "ModEnv Inv", "K_SW", default=0)
P("MENV_VEL", "menv_vel", "MOD ENV VELOCITY", "ModEnv Vel", "K_SW", default=0)

# the oscillators
P("OSC_WAVE", "osc_wave", "OSC WAVEFORM", "Osc Wave", "K_CH", "CH_WAVE", default=1)
P("OSC_CONFIG", "osc_config", "OSC CONFIG", "Osc Config", "K_CH", "CH_CONFIG", default=0)
P("OSC_CTRL", "osc_ctrl", "OSC CTRL", "Osc Ctrl", default=0.5)
P("OSC_LFO", "osc_lfo", "OSC LFO AMOUNT", "Osc LFO Amt", default=0.0)
P("OSC_MENV", "osc_menv", "OSC MOD ENV AMOUNT", "Osc ModEnv", default=0.0)

# the filter
P("FLT_TYPE", "flt_type", "FILTER TYPE", "Filter Type", "K_CH", "CH_FILTER", default=1)
P("FLT_FREQ", "flt_freq", "FILTER FREQ", "Filter Freq", default=0.7)
P("FLT_RES", "flt_res", "FILTER RESONANCE", "Filter Res", default=0.15)
P("FLT_LFO", "flt_lfo", "FILTER LFO AMOUNT", "Filt LFO Amt", default=0.0)
P("FLT_MENV", "flt_menv", "FILTER MOD ENV AMOUNT", "Filt ModEnv", default=0.3)
P("FLT_KB", "flt_kb", "FILTER KB TRACK", "Filt KbTrack", "K_CH", "CH_KBTRACK", default=2)
P("FLT_DRIVE", "flt_drive", "FILTER DRIVE", "Filt Drive", default=0.0)

# the amplifier's envelope
P("AENV_A", "aenv_a", "AMP ENV ATTACK", "AmpEnv Att", default=0.0)
P("AENV_D", "aenv_d", "AMP ENV DECAY", "AmpEnv Dec", default=1.0)
P("AENV_R", "aenv_r", "AMP ENV RELEASE", "AmpEnv Rel", default=0.3)
P("AENV_VEL", "aenv_vel", "AMP ENV VELOCITY", "AmpEnv Vel", "K_SW", default=0)

# the effects
P("FX_ON", "fx_on", "FX ON", "FX On", "K_SW", default=0)
P("FX_TYPE", "fx_type", "FX TYPE", "FX Type", "K_CH", "CH_FX", default=3)
P("FX_AMT", "fx_amt", "FX RATE/AMOUNT", "FX Rate/Amt", default=0.5)
P("DLY_ON", "dly_on", "DELAY ON", "Delay On", "K_SW", default=0)
P("DLY_TIME", "dly_time", "DELAY TEMPO", "Delay Tempo", default=0.5)
P("DLY_SYNC", "dly_sync", "DELAY MST CLK", "Dly MstClk", "K_SW", default=0)
P("DLY_DIV", "dly_div", "DELAY CLOCK DIV", "Delay Div", "K_CH", "CH_DLY_DIV", default=7)
P("DLY_FB", "dly_fb", "DELAY FEEDBACK", "Delay Fdbk", "K_CH", "CH_FEEDBACK", default=1)
P("DLY_PING", "dly_ping", "DELAY PING PONG", "Ping Pong", "K_SW", default=0)
P("DLY_MIX", "dly_mix", "DELAY DRY/WET", "Delay Mix", default=0.3)
P("DLY_ANALOG", "dly_analog", "DELAY ANALOG MODE", "Delay Analog", "K_SW", default=0)
P("REV_ON", "rev_on", "REVERB ON", "Reverb On", "K_SW", default=0)
P("REV_TYPE", "rev_type", "REVERB TYPE", "Reverb Type", "K_CH", "CH_REVERB", default=3)
P("REV_MIX", "rev_mix", "REVERB DRY/WET", "Reverb Mix", default=0.3)

# the morphs: how far the mod wheel (W) and the key velocity (V) take each of these parameters (centre: none).
# The effects, the delay and the reverb morph by the wheel only (as on the instrument)
MORPH = [("ARP_RATE", "ARP RATE", "ArpRt"), ("ARP_DIV", "ARP CLOCK DIV", "ArpDv"), ("GLIDE", "GLIDE", "Glide"),
         ("LFO_RATE", "LFO RATE", "LFORt"), ("LFO_DIV", "LFO CLOCK DIV", "LFODv"),
         ("MENV_A", "MOD ENV ATTACK", "MEnvA"), ("MENV_D", "MOD ENV DECAY", "MEnvD"),
         ("MENV_R", "MOD ENV RELEASE", "MEnvR"), ("OSC_LFO", "OSC LFO AMOUNT", "OsLFO"),
         ("OSC_MENV", "OSC MOD ENV AMOUNT", "OsMEn"), ("OSC_CTRL", "OSC CTRL", "OsCtl"),
         ("FLT_LFO", "FILTER LFO AMOUNT", "FlLFO"), ("FLT_MENV", "FILTER MOD ENV AMOUNT", "FlMEn"),
         ("FLT_FREQ", "FILTER FREQ", "FlFrq"), ("FLT_RES", "FILTER RESONANCE", "FlRes"),
         ("FLT_DRIVE", "FILTER DRIVE", "FlDrv"), ("AENV_A", "AMP ENV ATTACK", "AEnvA"),
         ("AENV_D", "AMP ENV DECAY", "AEnvD"), ("AENV_R", "AMP ENV RELEASE", "AEnvR"),
         ("OUT_LEVEL", "OUTPUT LEVEL", "Level"), ("FX_AMT", "FX RATE/AMOUNT", "FXAmt"),
         ("DLY_TIME", "DELAY TEMPO", "DlyTm"), ("DLY_DIV", "DELAY CLOCK DIV", "DlyDv"),
         ("DLY_MIX", "DELAY DRY/WET", "DlyMx"), ("REV_MIX", "REVERB DRY/WET", "RevMx")]
WHEEL_ONLY = {"FX_AMT", "DLY_TIME", "DLY_DIV", "DLY_MIX", "REV_MIX"}
for e, n, s in MORPH:
    P("MW_" + e, "mw_" + e.lower(), "MORPH WHEEL " + n, "MW " + s, "K_BIP", default=0.5)
for e, n, s in MORPH:
    if e not in WHEEL_ONLY:
        P("MV_" + e, "mv_" + e.lower(), "MORPH VELOCITY " + n, "MV " + s, "K_BIP", default=0.5)

# LFO 2, 3, 4: each its own rate (or the host's tempo), waveform, destination and amount (all bipolar)
LDEF = {2: (0.42, 3, 2), 3: (0.3, 3, 5), 4: (0.55, 4, 1)}
for n in (2, 3, 4):
    rate, wave, dest = LDEF[n]
    P("L%d_RATE" % n, "l%d_rate" % n, "LFO %d RATE" % n, "LFO%d Rate" % n, default=rate)
    P("L%d_SYNC" % n, "l%d_sync" % n, "LFO %d MST CLK" % n, "LFO%d MstClk" % n, "K_SW", default=0)
    P("L%d_DIV" % n, "l%d_div" % n, "LFO %d CLOCK DIV" % n, "LFO%d Div" % n, "K_CH", "CH_LFO_DIV", default=4)
    P("L%d_WAVE" % n, "l%d_wave" % n, "LFO %d WAVEFORM" % n, "LFO%d Wave" % n, "K_CH", "CH_LFO_WAVE", default=wave)
    P("L%d_DEST" % n, "l%d_dest" % n, "LFO %d DESTINATION" % n, "LFO%d Dest" % n, "K_CH", "CH_LFO_DEST", default=dest)
    P("L%d_AMT" % n, "l%d_amt" % n, "LFO %d AMOUNT" % n, "LFO%d Amount" % n, default=0.0)

# -------------------------------------------------------------------------------------------------- output ---
ids = [r[1] for r in rows]
assert len(ids) == len(set(ids))
with open(os.path.join(ROOT, "dsp", "params_list.inc"), "w") as f:
    f.write("/* generated by tools/gen_params.py: do not edit (the order is permanent; append only) */\n")
    for e, pid, name, short, kind, ch, d in rows:
        f.write('P(%s, "%s", "%s", "%s", %s, %s, %sf)\n' % (e, pid, name, short, kind, ch, d))
with open(os.path.join(ROOT, "dsp", "choices.inc"), "w") as f:
    f.write("/* generated by tools/gen_params.py: do not edit */\n")
    for name, items in choices.items():
        f.write("static const char *const %s[] = {%s};\n" % (name, ", ".join('"%s"' % i for i in items)))
    f.write("#define NCH(x) ((int)(sizeof(x) / sizeof(x[0])))\n")
# the morphs' targets, for the engine and the panel: (target, wheel parameter, velocity parameter or -1)
with open(os.path.join(ROOT, "dsp", "morph_list.inc"), "w") as f:
    f.write("/* generated by tools/gen_params.py: do not edit */\n")
    for e, n, s in MORPH:
        f.write("M(%s, MW_%s, %s)\n" % (e, e, "-1" if e in WHEEL_ONLY else "MV_" + e))
print(len(rows), "parameters,", len(MORPH), "morph targets")
