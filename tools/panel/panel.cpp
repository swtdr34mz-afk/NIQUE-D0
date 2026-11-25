/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
/* The plugin's panel as a picture, drawn by its own editor without a host:  nq_panel <out.png> [scale] [43] */
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

int main(int argc, char **argv)
{
    ScopedJuceInitialiser_GUI gui;
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
