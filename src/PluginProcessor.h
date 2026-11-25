/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* NIQUE D0 as a plugin (VST3, AU on the Mac) and a standalone app: the engine (dsp/nique.h), all its parameters for
 * the host (automation, saved with the project), its programs as the host's programs (the factory's, then your own:
 * files in Documents/NIQUE D0/Presets), MIDI in (notes, pitch bend, the mod wheel, sustain, CC 120/121/123), the
 * host's tempo and position (MST CLK). And the sound's own tools: INIT, MUTATE, RANDOMIZE, LIKE. */
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "nique.h"

class NiqueProcessor : public juce::AudioProcessor
{
public:
    NiqueProcessor();
    ~NiqueProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NIQUE D0"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return nq::numPresets() + (int)userPresets.size(); }
    int getCurrentProgram() override { return program; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String &) override {}

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    /* your own programs */
    struct UserPreset {
        juce::String name;
        std::vector<float> v;
    };
    std::vector<UserPreset> userPresets;
    static juce::File presetFolder();
    void loadUserPresets();
    int writePreset(const juce::String &name);   /* the sound as it is, kept under this name: its program number */
    std::vector<float> programValues(int index);

    /* the sound's own tools (message thread); morph: 0 the sound, 1 the wheel's morph, 2 velocity's */
    void initSound();
    void mutate(int morph);
    void randomize(int morph);
    void clearMorph(int morph);
    /* LIKE: up to 50 versions of the sound while it is made (kept with the project) */
    void like();
    void recallLike(int k);
    int numLikes() const { return (int)likes.size(); }
    int likePos = -1;

    std::vector<float> values();         /* the sound as it is now */
    void setValues(const std::vector<float> &v);

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String paramId(int i) { return nq::PARAMS[i].id; }

    /* the panel's wheel and stick (-1: untouched), and what the panel shows */
    std::atomic<float> uiWheel{-1.0f}, uiBend{-1.0f};
    std::atomic<float> wheelPos{0.0f}, bendPos{0.5f}, lfoLevel{0.0f}, outLevel{0.0f};
    std::atomic<int> voices{0};
    std::atomic<int> uiLayout{0};          /* 0 NORMAL (wide), 1 4:3 */
    std::atomic<float> uiScale{-1.0f};    /* the window's size (-1: never set: as large as the screen allows) */
    std::atomic<bool> panicRequest{false};

private:
    nq::Engine engine;
    std::vector<std::atomic<float> *> raw;
    float sent[nq::NPARAMS] = {};
    bool fresh = true;
    int program = 0;
    float lastWheelUi = -1.0f, lastBendUi = -1.0f;
    std::vector<std::vector<float>> likes;
    uint32_t seed = 1;
    void handleMidi(const juce::MidiMessage &m);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NiqueProcessor)
};
