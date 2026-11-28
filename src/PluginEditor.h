/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The panel: every section laid out on a page of 1840 x 600, scaled to the window. On top the logo, the program,
 * LIKE, the sound's tools and the readout of the control in hand; on the left the morphs, the octave, HOLD, the
 * pitch stick and the mod wheel. */
#pragma once
#include "PluginProcessor.h"
#include "ui/Controls.h"

class NiqueEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int W = 1840, H = 600;   /* NORMAL */
    static constexpr int WC = 1240, HC = 998; /* 4:3 */
    explicit NiqueEditor(NiqueProcessor &);
    ~NiqueEditor() override;
    void resized() override;
    bool sized = false;                      /* (resizes during the constructor keep nothing) */
    void paint(juce::Graphics &) override {}

    /* the page: the background drawn once (the panel, the sections, the print), the controls on it */
    struct Page : juce::Component {
        struct Sec {
            juce::Rectangle<float> r, rc;    /* (NORMAL, 4:3) */
            juce::String title;
            bool sub;
        };
        struct Text {
            juce::Rectangle<float> r, rc;
            juce::String text;
            float size;
            juce::Justification j;
        };
        std::vector<Sec> sections;
        std::vector<Text> texts;
        std::vector<std::pair<juce::Line<float>, juce::Line<float>>> arrows;
        bool compact = false;
        void dropCache() { cache = juce::Image(); }
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

    /* every control's place in both layouts (4:3 empty: NORMAL's moved by cdx, cdy) */
    struct Placed {
        juce::Component *c;
        juce::Rectangle<int> normal, compact;
    };
    std::vector<Placed> placed;
    std::vector<nq::Selector *> compactTwoCols;
    int cdx = 0, cdy = 0;
    bool placedOnce = false;
    template <class C> C *add(C *c, juce::Rectangle<int> r, juce::Rectangle<int> rc = {})
    {
        page.addAndMakeVisible(c);
        c->setBounds(r);
        placed.push_back({c, r, rc.isEmpty() ? r.translated(cdx, cdy) : rc});
        return c;
    }
    void place(bool compact);
    nq::Knob *knob(const char *id, const juce::String &legend, juce::Rectangle<int> r, juce::Rectangle<int> rc = {},
                   float size = 0.0f);
    void section(juce::Rectangle<int> r, const juce::String &title, bool sub = false, juce::Rectangle<int> rc = {});
    void text(juce::Rectangle<int> r, const juce::String &t, float size = 11.0f,
              juce::Justification j = juce::Justification::centred, juce::Rectangle<int> rc = {});
    void arrow(juce::Line<float> l, juce::Line<float> lc = {});
};
