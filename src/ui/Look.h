/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* NIQUE D0's look: a red metal panel, dark grey sections framed in white, their names in black bars; black knobs
 * with a printed 0-10 scale (a red cap on FREQ), a green morph LED under each knob that can morph; dark rubber keys
 * with red LEDs; red LED displays. Fonts embedded (Fira Sans, Anonymous Pro: SIL OFL, fonts/). */
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

namespace nq
{
namespace col
{
const juce::Colour red{0xffc3111d}, redDark{0xff9c0c15}, grey{0xff2c2e33}, greyHi{0xff3a3d43}, frame{0xffeeeeee},
    print{0xfff2f2f2}, bar{0xff141518}, led{0xffff2c1c}, ledOff{0xff3d1210}, green{0xff46ff62}, greenOff{0xff12301a},
    lcd{0xff1a0505}, lcdText{0xffff3a26};
}

enum class Face { label, title, logo, mono };

/* the editors' fonts and pictures: they live while an editor is open (a SharedResourcePointer in each) and go when
 * the last one closes, on the message thread, never while the host unloads the plugin (that can hang a host) */
struct UiResources {
    UiResources() = default;
    ~UiResources();
};
juce::Font font(Face f, float height);

class Look : public juce::LookAndFeel_V4
{
public:
    Look();
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override;
    juce::Font getPopupMenuFont() override { return font(Face::label, 15.0f); }
    void drawPopupMenuBackground(juce::Graphics &, int w, int h) override;
};

void drawPanel(juce::Graphics &, juce::Rectangle<float> r);
void drawSection(juce::Graphics &, juce::Rectangle<float> r, const juce::String &title);
void drawSub(juce::Graphics &, juce::Rectangle<float> r, const juce::String &title);   /* a part of a section */
/* a knob: pos 0..1; morph: where the morph takes it (-1: none shown); scale: the printed 0-10 */
void drawKnob(juce::Graphics &, juce::Rectangle<float> r, float pos, bool redCap, bool scale, float morphTo,
              bool bipolar);
void drawEndless(juce::Graphics &, juce::Rectangle<float> r, float angle);
void drawKey(juce::Graphics &, juce::Rectangle<float> r, bool down);
void drawLed(juce::Graphics &, juce::Point<float> c, float r, float level, juce::Colour on = col::led);
void drawDisplay(juce::Graphics &, juce::Rectangle<float> r, const juce::String &text, int ghost);
void drawLcd(juce::Graphics &, juce::Rectangle<float> r);
void drawLabel(juce::Graphics &, const juce::String &, juce::Rectangle<float>, float size = 11.0f,
               juce::Justification j = juce::Justification::centred, juce::Colour c = col::print);
void drawLogo(juce::Graphics &, juce::Rectangle<float> r);
void drawTag(juce::Graphics &, juce::Rectangle<float> r);
void drawScrew(juce::Graphics &, juce::Point<float> c, float r);
void drawWheel(juce::Graphics &, juce::Rectangle<float> r, float pos, bool spring);
} // namespace nq
