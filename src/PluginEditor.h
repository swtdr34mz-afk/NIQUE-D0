/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The panel: every section laid out on a page of 1840 x 770, scaled to the window. On top the logo, the program,
 * LIKE, the sound's tools and the readout of the control in hand; on the left the morphs, the octave, HOLD, the
 * pitch stick and the mod wheel. */
#pragma once
#include "PluginProcessor.h"
#include "ui/Controls.h"

class NiqueEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int W = 1840, H = 770;
    explicit NiqueEditor(NiqueProcessor &);
    ~NiqueEditor() override;
    void resized() override;
    void paint(juce::Graphics &) override {}

    /* the page: the background drawn once (the panel, the sections, the print), the controls on it */
    struct Page : juce::Component {
        struct Sec {
            juce::Rectangle<float> r;
            juce::String title;
            bool sub;
        };
        struct Text {
            juce::Rectangle<float> r;
            juce::String text;
            float size;
            juce::Justification j;
        };
        std::vector<Sec> sections;
        std::vector<Text> texts;
        std::vector<juce::Point<float>> screws;
        std::vector<juce::Line<float>> arrows;
        ~Page() override { deleteAllChildren(); }
        void paint(juce::Graphics &) override;

    private:
        juce::Image cache;
        float cacheScale = 0.0f;
    };

private:
    void timerCallback() override;
    void programMenu();
    void saveProgram();
    void touched(juce::RangedAudioParameter &);

    NiqueProcessor &proc;
    juce::SharedResourcePointer<nq::UiResources> resources;   /* (before the page: freed after it) */
    nq::Look look;
    nq::UiState ui;
    Page page;
    juce::RangedAudioParameter *inHand = nullptr;
    struct Header;
    Header *header = nullptr;            /* (the page's) */
    std::vector<juce::Component *> live;  /* repainted by the timer */
    std::vector<std::pair<juce::Component *, juce::Component *>> syncPairs;   /* free / synced knob */
    std::vector<int> syncParams;
    juce::Component *ctrlHint = nullptr;

    template <class C> C *add(C *c, juce::Rectangle<int> r)
    {
        page.addAndMakeVisible(c);
        c->setBounds(r);
        return c;
    }
    nq::Knob *knob(const char *id, const juce::String &legend, juce::Rectangle<int> r, float size = 0.0f);
    void section(juce::Rectangle<int> r, const juce::String &title, bool sub = false);
    void text(juce::Rectangle<int> r, const juce::String &t, float size = 11.0f,
              juce::Justification j = juce::Justification::centred);
};
