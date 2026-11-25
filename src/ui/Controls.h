/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The panel's controls, each tied to host parameters: a knob (and its morphs: with a morph chosen, turning the knob
 * sets where that morph takes it), a key with its LED (a switch), a selector (a key stepping through LEDs; a click
 * on a name picks it), an endless knob with a display (the waveform, the configuration), a momentary key with a
 * job, a wheel (the mod wheel, the pitch stick), a lamp. Touching one shows its name and value in the readout. */
#pragma once
#include "Look.h"

namespace nq
{
using APVTS = juce::AudioProcessorValueTreeState;

/* what every control shares: the morph being set (0 none, 1 the wheel, 2 velocity), the readout */
struct UiState {
    int morph = 0;
    std::function<void(juce::RangedAudioParameter &)> touched = [](juce::RangedAudioParameter &) {};
};

class Knob : public juce::Component
{
public:
    Knob(APVTS &, const juce::String &id, const juce::String &legend, UiState &);
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseDoubleClick(const juce::MouseEvent &) override;
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &) override;
    void mouseEnter(const juce::MouseEvent &) override;
    juce::RangedAudioParameter &param;
    juce::RangedAudioParameter *mw = nullptr, *mv = nullptr;   /* its morphs (none: null) */
    juce::String legend;
    bool redCap = false, scale = true, bipolar = false;
    float knobSize = 0.0f;               /* 0: as large as fits */

private:
    UiState &ui;
    juce::ParameterAttachment att;
    std::unique_ptr<juce::ParameterAttachment> attW, attV;
    juce::RangedAudioParameter *editing = nullptr;   /* the one being dragged */
    float startValue = 0.0f, startY = 0.0f;
    float morphTarget(int which) const;  /* where a morph takes it (normalised), -1 none */
    void setFromDrag(float v);
    juce::Rectangle<float> knobArea() const;
};

/* a key that switches a parameter, its LED above it */
class Key : public juce::Component
{
public:
    Key(APVTS &, const juce::String &id, const juce::String &legend, UiState &);
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseEnter(const juce::MouseEvent &) override;
    juce::RangedAudioParameter &param;
    juce::String legend;
    bool on() const { return param.getValue() >= 0.5f; }

private:
    UiState &ui;
    bool down = false;
    juce::ParameterAttachment att;
};

/* a choice: a column of LEDs with names, a key stepping through them (subset: only these choices) */
class Selector : public juce::Component
{
public:
    Selector(APVTS &, const juce::String &id, juce::StringArray names, UiState &, std::vector<int> subset = {});
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseEnter(const juce::MouseEvent &) override;
    juce::RangedAudioParameter &param;
    juce::StringArray names;
    juce::String legend;                 /* under the key */
    bool keyAtTop = false, twoColumns = false, hasKey = true;
    int rowH = 17;

private:
    UiState &ui;
    std::vector<int> subset;
    bool down = false;
    juce::ParameterAttachment att;
    int current() const;
    juce::Rectangle<float> keyArea() const;
    juce::Rectangle<float> rowArea(int k) const;
    void choose(int choice);
};

/* an endless knob over a display: a choice stepped by turning, the full list by a click on the display */
class Endless : public juce::Component
{
public:
    Endless(APVTS &, const juce::String &id, UiState &, std::function<juce::String(int)> shortName,
            std::function<juce::String(int)> longName, std::function<juce::String(int)> group = nullptr);
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &) override;
    void mouseEnter(const juce::MouseEvent &) override;
    juce::RangedAudioParameter &param;
    int ghost = 3;
    bool knob = true;
    juce::String legend;

private:
    UiState &ui;
    std::function<juce::String(int)> shortName, longName, group;
    juce::ParameterAttachment att;
    float acc = 0.0f, lastY = 0.0f, angle = 0.0f;
    bool dragged = false;
    int count() const;
    int current() const;
    void step(int d);
    void menu();
    juce::Rectangle<float> displayArea() const;
};

/* a momentary key with a job (and an LED, if it has a level) */
class ActionKey : public juce::Component
{
public:
    ActionKey(const juce::String &legend, std::function<void(const juce::ModifierKeys &)> action,
              std::function<float()> led = nullptr);
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    juce::String legend;
    bool legendAbove = false;

private:
    std::function<void(const juce::ModifierKeys &)> action;
    std::function<float()> led;
    bool down = false;
};

/* the mod wheel (stays where it is put) or the pitch stick (springs back): get / set 0..1 */
class Wheel : public juce::Component
{
public:
    Wheel(bool spring, std::function<float()> get, std::function<void(float)> set);
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;

private:
    bool spring;
    std::function<float()> get;
    std::function<void(float)> set;
    float startV = 0.0f, startY = 0.0f;
};

/* anything lit: a lambda's 0..1 */
class Lamp : public juce::Component
{
public:
    Lamp(std::function<float()> level, juce::Colour c = col::led) : level(std::move(level)), colour(c)
    {
        setInterceptsMouseClicks(false, false);
    }
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat();
        drawLed(g, r.getCentre(), jmin(r.getWidth(), r.getHeight()) * 0.3f, level(), colour);
    }
    std::function<float()> level;
    juce::Colour colour;

private:
    static float jmin(float a, float b) { return a < b ? a : b; }
};
} // namespace nq
