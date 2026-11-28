/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
#include "PluginProcessor.h"
#include "PanelDialog.h"
#include "PluginEditor.h"

using namespace nq;

juce::AudioProcessorValueTreeState::ParameterLayout NiqueProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < NPARAMS; i++) {
        const ParamDef &d = PARAMS[i];
        juce::ParameterID pid{d.id, 1};
        if (d.kind == K_CH) {
            juce::StringArray ch;
            for (int k = 0; k < d.nChoices; k++)
                ch.add(d.choices[k]);
            layout.add(std::make_unique<juce::AudioParameterChoice>(pid, d.name, ch, (int)d.def));
        } else if (d.kind == K_SW) {     /* (a choice of two: its value always 0 or 1, whatever a host sets) */
            layout.add(std::make_unique<juce::AudioParameterChoice>(pid, d.name, juce::StringArray{"OFF", "ON"}, d.def >= 0.5f ? 1 : 0));
        } else {
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                pid, d.name, juce::NormalisableRange<float>(0.0f, 1.0f), d.def,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction([i](float v, int) {
                    char t[32];
                    paramText(i, v, t, sizeof t);
                    return juce::String(t);
                })));
        }
    }
    return layout;
}

NiqueProcessor::NiqueProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "NIQUED0", createLayout())
{
    for (int i = 0; i < NPARAMS; i++)
        raw.push_back(apvts.getRawParameterValue(paramId(i)));
    loadUserPresets();
    setCurrentProgram(2);                /* (a program that sounds when the plugin opens) */
}

/* ------------------------------------------------------------------------------------------ programs --- */
juce::File NiqueProcessor::presetFolder()
{
    return pdlg::docsOrAppData().getChildFile("NIQUE D0").getChildFile("Presets");
}

void NiqueProcessor::loadUserPresets()
{
    userPresets.clear();
    auto files = presetFolder().findChildFiles(juce::File::findFiles, false, "*.nq0");
    files.sort();
    for (auto &f : files)
        if (auto xml = juce::XmlDocument::parse(f)) {
            if (!xml->hasTagName("NIQUED0PRESET"))
                continue;
            UserPreset u;
            u.name = xml->getStringAttribute("name", f.getFileNameWithoutExtension());
            u.v.resize(NPARAMS);
            for (int i = 0; i < NPARAMS; i++)   /* (a parameter the file lacks: its default) */
                u.v[(size_t)i] = juce::jlimit(0.0f, paramMax(i), (float)xml->getDoubleAttribute(PARAMS[i].id, PARAMS[i].def));
            userPresets.push_back(std::move(u));
        }
}

int NiqueProcessor::writePreset(const juce::String &name)
{
    auto clean = name.trim().toUpperCase().substring(0, 16);
    if (clean.isEmpty())
        return -1;
    juce::XmlElement xml("NIQUED0PRESET");
    xml.setAttribute("name", clean);
    xml.setAttribute("version", 1);
    for (int i = 0; i < NPARAMS; i++)
        xml.setAttribute(PARAMS[i].id, raw[(size_t)i]->load());
    auto folder = presetFolder();
    folder.createDirectory();
    if (!xml.writeTo(folder.getChildFile(juce::File::createLegalFileName(clean) + ".nq0")))
        return -1;
    loadUserPresets();
    for (size_t i = 0; i < userPresets.size(); i++)
        if (userPresets[i].name == clean) {
            program = numPresets() + (int)i;
            updateHostDisplay(ChangeDetails().withProgramChanged(true));
            return program;
        }
    return -1;
}

std::vector<float> NiqueProcessor::programValues(int index)
{
    std::vector<float> v((size_t)NPARAMS);
    if (index >= 0 && index < numPresets())
        presetValues(index, v.data());
    else if (index >= numPresets() && index < getNumPrograms())
        v = userPresets[(size_t)(index - numPresets())].v;
    else
        v.clear();
    return v;
}

void NiqueProcessor::setCurrentProgram(int index)
{
    auto v = programValues(index);
    if (v.empty())
        return;
    program = index;
    setValues(v);
}

const juce::String NiqueProcessor::getProgramName(int index)
{
    if (index >= 0 && index < numPresets())
        return presetName(index);
    if (index >= numPresets() && index < getNumPrograms())
        return userPresets[(size_t)(index - numPresets())].name;
    return {};
}

/* -------------------------------------------------------------------------------- the sound's tools --- */
std::vector<float> NiqueProcessor::values()
{
    std::vector<float> v((size_t)NPARAMS);
    for (int i = 0; i < NPARAMS; i++)
        v[(size_t)i] = raw[(size_t)i]->load();
    return v;
}

void NiqueProcessor::setValues(const std::vector<float> &v)
{
    for (int i = 0; i < NPARAMS && i < (int)v.size(); i++)
        if (auto *p = apvts.getParameter(paramId(i))) {
            float n = p->convertTo0to1(v[(size_t)i]);
            if (std::fabs(p->getValue() - n) > 1e-7f) {
                p->beginChangeGesture();
                p->setValueNotifyingHost(n);
                p->endChangeGesture();
            }
        }
}

void NiqueProcessor::initSound()
{
    std::vector<float> v((size_t)NPARAMS);
    nq::initSound(v.data());
    auto now = values();                 /* (as on the instrument: the output level stays) */
    v[OUT_LEVEL] = now[OUT_LEVEL];
    setValues(v);
}

void NiqueProcessor::mutate(int morph)
{
    auto v = values();
    nq::mutate(v.data(), seed++ * 7919u + (uint32_t)juce::Time::getMillisecondCounter(), (MorphSel)morph);
    setValues(v);
}

void NiqueProcessor::randomize(int morph)
{
    auto v = values();
    nq::randomize(v.data(), seed++ * 104729u + (uint32_t)juce::Time::getMillisecondCounter(), (MorphSel)morph);
    setValues(v);
}

void NiqueProcessor::clearMorph(int morph)
{
    auto v = values();
    for (int m = 0; m < NMORPH; m++) {
        if (morph != 2 && MORPHS[m].wheel >= 0)
            v[(size_t)MORPHS[m].wheel] = 0.5f;
        if (morph != 1 && MORPHS[m].vel >= 0)
            v[(size_t)MORPHS[m].vel] = 0.5f;
    }
    setValues(v);
}

void NiqueProcessor::like()
{
    auto v = values();
    for (auto &l : likes)                /* (the same again: kept once) */
        if (l == v)
            return;
    if (likes.size() >= 50)
        likes.erase(likes.begin());
    likes.push_back(v);
    likePos = (int)likes.size() - 1;
}

void NiqueProcessor::recallLike(int k)
{
    if (k < 0 || k >= (int)likes.size())
        return;
    likePos = k;
    setValues(likes[(size_t)k]);
}

/* ---------------------------------------------------------------------------------------------- audio --- */
bool NiqueProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NiqueProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate);
    fresh = true;
}

void NiqueProcessor::handleMidi(const juce::MidiMessage &m)
{
    if (m.isNoteOn())
        engine.noteOn(m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff(m.getNoteNumber());
    else if (m.isPitchWheel()) {
        float b = (float)(m.getPitchWheelValue() - 8192) / 8192.0f;
        engine.bend(b);
        bendPos = 0.5f + 0.5f * b;
    } else if (m.isAllSoundOff())
        engine.panic();
    else if (m.isAllNotesOff())
        engine.allNotesOff();
    else if (m.isController()) {
        int cc = m.getControllerNumber(), v = m.getControllerValue();
        if (cc == 1) {
            engine.modWheel((float)v / 127.0f);
            wheelPos = (float)v / 127.0f;
        } else if (cc == 64)
            engine.sustain(v >= 64);
        else if (cc == 121) {
            engine.bend(0.0f);
            engine.modWheel(0.0f);
            engine.sustain(false);
            bendPos = 0.5f;
            wheelPos = 0.0f;
        }
    }
}

void NiqueProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    float *L = buffer.getWritePointer(0), *R = buffer.getWritePointer(buffer.getNumChannels() > 1 ? 1 : 0);
    for (int i = 0; i < NPARAMS; i++) {  /* the parameters, when they change */
        float v = raw[(size_t)i]->load();
        if (fresh || v != sent[i]) {
            engine.set(i, v);
            sent[i] = v;
        }
    }
    fresh = false;
    if (panicRequest.exchange(false))
        engine.panic();
    float w = uiWheel.load(), b = uiBend.load();   /* the panel's wheel and stick */
    if (w >= 0.0f && w != lastWheelUi) {
        engine.modWheel(w);
        wheelPos = w;
        lastWheelUi = w;
    }
    if (b >= 0.0f && b != lastBendUi) {
        engine.bend(b * 2.0f - 1.0f);
        bendPos = b;
        lastBendUi = b;
    }
    if (auto *ph = getPlayHead())        /* the host's tempo and position (MST CLK) */
        if (auto pos = ph->getPosition()) {
            if (auto bpm = pos->getBpm())
                engine.tempo(*bpm);
            auto ppq = pos->getPpqPosition();
            engine.position(ppq ? *ppq : 0.0, pos->getIsPlaying() && ppq.hasValue());
        }
    int done = 0;
    for (const auto meta : midi) {
        int at = juce::jlimit(0, n, meta.samplePosition);
        if (at > done) {
            engine.render(L + done, R + done, at - done);
            done = at;
        }
        handleMidi(meta.getMessage());
    }
    if (n > done)
        engine.render(L + done, R + done, n - done);
    for (int c = 2; c < buffer.getNumChannels(); c++)
        buffer.clear(c, 0, n);
    lfoLevel = engine.lfoLevel();
    outLevel = engine.outLevel();
    voices = engine.voicesSounding();
}

/* ---------------------------------------------------------------------------------------------- state --- */
void NiqueProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    state.setProperty("program", program, nullptr);
    state.setProperty("uiScale", uiScale.load(), nullptr);
    state.setProperty("uiVersion", 2, nullptr);
    state.setProperty("uiLayout", uiLayout.load(), nullptr);   /* (2: the larger print; an older size is not kept) */
    juce::MemoryOutputStream likeData;   /* the likes: how many values each, then the sounds' values */
    likeData.writeInt(NPARAMS);
    for (auto &l : likes)
        for (float x : l)
            likeData.writeFloat(x);
    state.setProperty("likes", likeData.getMemoryBlock().toBase64Encoding(), nullptr);
    state.setProperty("likePos", likePos, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void NiqueProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType())) {
            auto state = juce::ValueTree::fromXml(*xml);
            program = juce::jlimit(0, juce::jmax(0, getNumPrograms() - 1), (int)state.getProperty("program", 0));
            {
                float u = (int)state.getProperty("uiVersion", 1) >= 2 ? (float)state.getProperty("uiScale", -1.0f) : -1.0f;
                uiScale = u < 0.0f ? -1.0f : juce::jlimit(0.4f, 1.5f, u);
            }
            uiLayout = juce::jlimit(0, 1, (int)state.getProperty("uiLayout", 0));
            likes.clear();
            juce::MemoryBlock mb;
            if (mb.fromBase64Encoding(state.getProperty("likes", "").toString())) {
                juce::MemoryInputStream in(mb, false);
                const int per = in.getNumBytesRemaining() >= 4 ? in.readInt() : 0;
                while (per > 0 && per < 10000 && in.getNumBytesRemaining() >= (juce::int64)(per * 4)) {
                    std::vector<float> v((size_t)NPARAMS);   /* (a value an older version lacked: its default) */
                    for (int i = 0; i < NPARAMS; i++)
                        v[(size_t)i] = PARAMS[i].def;
                    for (int i = 0; i < per; i++) {
                        float x = in.readFloat();
                        if (i < NPARAMS)
                            v[(size_t)i] = juce::jlimit(0.0f, paramMax(i), x);
                    }
                    likes.push_back(std::move(v));
                }
            }
            likePos = juce::jlimit(-1, (int)likes.size() - 1, (int)state.getProperty("likePos", -1));
            state.removeProperty("likes", nullptr);
            apvts.replaceState(state);
        }
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new NiqueProcessor(); }
