/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The plugin's panel as a picture, drawn by its own editor without a host:  nq_panel <out.png> [scale] [43]
 * Or the right-click check:  nq_panel --rightclick  (a right-click and right-drag on every control: no value changes) */
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

int main(int argc, char **argv)
{
    ScopedJuceInitialiser_GUI gui;
    if (argc > 1 && String(argv[1]) == "--rightclick") {
        NiqueProcessor proc;
        std::unique_ptr<AudioProcessorEditor> ed(proc.createEditor());
        ed->setSize(NiqueEditor::W, NiqueEditor::H);
        auto before = proc.values();
        int tried = 0;
        std::function<void(Component &)> walk = [&](Component &c) {
            for (auto *ch : c.getChildren()) {
                if (dynamic_cast<nq::Knob *>(ch) || dynamic_cast<nq::Key *>(ch) || dynamic_cast<nq::Selector *>(ch) ||
                    dynamic_cast<nq::Endless *>(ch)) {
                    auto centre = ch->getLocalBounds().getCentre().toFloat();
                    auto mods = ModifierKeys(ModifierKeys::rightButtonModifier);
                    auto t = Time::getCurrentTime();
                    MouseEvent down(Desktop::getInstance().getMainMouseSource(), centre, mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                    ch, ch, t, centre, t, 1, false);
                    auto to = centre.translated(0.0f, -80.0f);
                    MouseEvent drag(Desktop::getInstance().getMainMouseSource(), to, mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, ch, ch,
                                    t, centre, t, 1, true);
                    ch->mouseDown(down);
                    ch->mouseDrag(drag);
                    ch->mouseUp(drag);
                    ch->mouseDoubleClick(down);
                    tried++;
                }
                walk(*ch);
            }
        };
        walk(*ed);
        auto after = proc.values();
        int changed = 0;
        for (size_t i = 0; i < before.size(); i++)
            if (before[i] != after[i]) {
                std::printf("changed by a right-click: %s\n", nq::PARAMS[i].id);
                changed++;
            }
        std::printf("right-clicked %d controls, %d values changed\n", tried, changed);
        return changed == 0 && tried > 50 ? 0 : 1;
    }
    if (argc < 2) {
        std::fprintf(stderr, "usage: nq_panel <out.png> [scale]\n");
        return 2;
    }
    float scale = argc > 2 ? (float)std::atof(argv[2]) : 1.0f;
    NiqueProcessor proc;
    const bool compact = argc > 3 && String(argv[3]) == "43";
    proc.uiLayout = compact ? 1 : 0;
    std::unique_ptr<AudioProcessorEditor> ed(proc.createEditor());
    if (compact)
        ed->setSize(NiqueEditor::WC, NiqueEditor::HC);
    else
        ed->setSize(NiqueEditor::W, NiqueEditor::H);
    Image img = ed->createComponentSnapshot(ed->getLocalBounds(), true, scale);
    File f = File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    f.deleteFile();
    FileOutputStream out(f);
    return out.openedOk() && PNGImageFormat().writeImageToStream(img, out) ? 0 : 1;
}
