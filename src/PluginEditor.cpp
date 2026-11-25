/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
#include "PluginEditor.h"

using namespace nq;
using namespace juce;
using R = Rectangle<int>;

static String U(const char *s) { return String::fromUTF8(s); }

/* --------------------------------------------------------------------------- names for the displays --- */
static String waveShort(int w)
{
    static const char *const P[4] = {"PU", "SA", "tr", "Si"};
    if (w >= 0 && w < 4)
        return P[w];
    return PARAMS[OSC_WAVE].choices[jlimit(0, 46, w)];
}

static String waveGroup(int w)
{
    return w < 4 ? "PANEL" : w < 11 ? "EXTENDED ANALOG" : w < 14 ? "FIXED PULSE WIDTH" : w < 23 ? "DRAWBAR ORGAN"
         : w < 28 ? "BELLS AND BARS" : w < 36 ? "DIGITAL" : w < 40 ? "ELECTRIC" : "FORMANTS";
}

static String configShort(int c)
{
    static const char *const ONE[4] = {"Pit", "SHP", "SYn", "dEt"};
    if (c < 4)
        return ONE[c];
    if (c < 40) {
        static const char *const W[4] = {"Si", "tr", "SA", "Sq"};
        int h = (c - 4) % 9;
        return String(W[(c - 4) / 9]) + (h == 0 ? "Sb" : " " + String(h));
    }
    if (c < 53) {
        static const char *const F[13] = {"F.5", "F 1", "F1.4", "F1.5", "F 2", "F2.8", "F 3", "F3.5", "F 4", "F 5", "F 6", "F 7", "F 8"};
        return F[c - 40];
    }
    return c == 53 ? "An" : "nOiS";
}

static String configGroup(int c)
{
    return c < 3 ? "ONE OSCILLATOR" : c == 3 ? "TWO OSCILLATORS" : c < 13 ? "MIX: OSC 1 + SINE" : c < 22 ? "MIX: OSC 1 + TRIANGLE"
         : c < 31 ? "MIX: OSC 1 + SAW" : c < 40 ? "MIX: OSC 1 + SQUARE" : c < 53 ? "FM (OSC 2: SINE AT A RATIO)" : "MODULATION";
}

static String hintFor(int cfg, int wave)
{
    if (cfg == 0)
        return "CTRL: THE PITCH (+-2 OCT)";
    if (cfg == 1)
        return wave == 0 ? "CTRL: PULSE WIDTH" : wave == 1 ? "CTRL: 2ND SAW TO THE OCTAVE"
             : wave < 4 ? "CTRL: WAVE FOLDING" : "CTRL: PHASE DISTORTION";
    if (cfg == 2)
        return "CTRL: THE SYNCED PITCH";
    if (cfg == 3)
        return "CTRL: DETUNE, THEN SEMITONES";
    if (cfg < 40)
        return "CTRL: THE LEVEL OF OSC 2";
    if (cfg < 53)
        return "CTRL: THE FM AMOUNT";
    return cfg == 53 ? "CTRL: THE PITCH OF THE AM" : "CTRL: THE NOISE";
}

/* ======================================================================================== the page === */
void NiqueEditor::Page::paint(Graphics &g)
{
    const float sc = jlimit(1.0f, 6.0f, (float)g.getInternalContext().getPhysicalPixelScaleFactor());
    if (!cache.isValid() || std::fabs(sc - cacheScale) > 0.02f) {
        cacheScale = sc;
        cache = Image(Image::RGB, (int)std::ceil((float)getWidth() * sc), (int)std::ceil((float)getHeight() * sc), false);
        Graphics cg(cache);
        drawPanel(cg, getLocalBounds().toFloat(), sc);
        cg.addTransform(AffineTransform::scale(sc));
        for (auto &s : sections)
            if (s.sub)
                drawSub(cg, compact ? s.rc : s.r, s.title);
            else
                drawSection(cg, compact ? s.rc : s.r, s.title);
        for (auto &t : texts)
            drawLabel(cg, t.text, compact ? t.rc : t.r, t.size, t.j);
        cg.setColour(col::print.withAlpha(0.7f));
        for (auto &l0 : arrows) {
            auto l = compact ? l0.second : l0.first;
            cg.drawLine(l, 1.4f);
            Path p;
            auto e = l.getEnd();
            float a = l.getAngle();
            p.addTriangle(e, e.getPointOnCircumference(7.0f, a + MathConstants<float>::pi * 0.85f),
                          e.getPointOnCircumference(7.0f, a - MathConstants<float>::pi * 0.85f));
            cg.fillPath(p);
        }
        const float pw = (float)getWidth(), ph = (float)getHeight();
        drawTag(cg, {pw - 250.0f, ph - 34.0f, 230.0f, 26.0f});
        drawLabel(cg, U("VIRTUAL ANALOG LEAD SYNTHESIZER  ·  swtdr34mz"), {24.0f, ph - 32.0f, 420.0f, 22.0f}, 11.0f,
                  Justification::centredLeft);
        for (float sx : {8.0f, pw * 0.5f, pw - 8.0f})   /* the panel's screws */
            for (float sy : {8.0f, ph - 8.0f})
                drawScrew(cg, {sx, sy}, 5.0f);
    }
    Graphics::ScopedSaveState s(g);
    g.setImageResamplingQuality(Graphics::highResamplingQuality);
    g.drawImage(cache, getLocalBounds().toFloat());
}

/* ====================================================================================== the header === */
struct NiqueEditor::Header : Component {
    NiqueEditor &ed;
    explicit Header(NiqueEditor &e) : ed(e) {}
    Rectangle<float> logo() const { return {26.0f, 12.0f, 220.0f, 38.0f}; }
    Rectangle<float> num() const { return {330.0f, 16.0f, 64.0f, 32.0f}; }
    Rectangle<float> name() const { return {404.0f, 16.0f, 250.0f, 32.0f}; }
    Rectangle<float> likeNum() const { return {872.0f, 16.0f, 70.0f, 32.0f}; }
    Rectangle<float> read() const   /* (4:3: the second row) */
    {
        return ed.page.compact ? Rectangle<float>{420.0f, 72.0f, 796.0f, 32.0f} : Rectangle<float>{1484.0f, 16.0f, 340.0f, 32.0f};
    }
    bool edited = false;
    void paint(Graphics &g) override
    {
        drawLogo(g, logo());
        drawLed(g, {290.0f, 26.0f}, 3.4f, ed.proc.voices.load() > 0 ? 1.0f : 0.0f);
        drawLabel(g, "PLAY", {266.0f, 34.0f, 48.0f, 16.0f}, 9.5f);
        drawLabel(g, "PROGRAM", {330.0f, 0.0f, 120.0f, 16.0f}, 9.5f, Justification::centredLeft);
        int p = ed.proc.getCurrentProgram();
        drawDisplay(g, num(), String(p + 1).paddedLeft('0', 3) + (edited ? "." : ""), 3);
        drawLcd(g, name());
        g.setFont(font(Face::mono, 18.0f));
        g.setColour(col::lcdText);
        g.drawText(ed.proc.getProgramName(p), name().reduced(10.0f, 0.0f), Justification::centredLeft, true);
        g.setColour(col::lcdText.withAlpha(0.55f));
        g.setFont(font(Face::mono, 12.5f));
        String grp = p < numPresets() ? String(presetGroup(p)) : String("YOURS");
        g.drawText(grp, name().reduced(8.0f, 0.0f), Justification::centredRight, true);
        drawLabel(g, "LIKES", {872.0f, 0.0f, 70.0f, 16.0f}, 9.5f, Justification::centredLeft);
        int n = ed.proc.numLikes();
        drawDisplay(g, likeNum(), n == 0 ? String("--") : String(ed.proc.likePos + 1) + "-" + String(n), 5);
        drawLcd(g, read());
        if (ed.inHand) {
            g.setFont(font(Face::mono, 17.0f));
            g.setColour(col::lcdText.withAlpha(0.7f));
            g.drawText(ed.inHand->getName(40), read().reduced(12.0f, 0.0f), Justification::centredLeft, true);
            g.setColour(col::lcdText);
            g.drawText(ed.inHand->getCurrentValueAsText(), read().reduced(12.0f, 0.0f), Justification::centredRight, true);
        } else {
            g.setFont(font(Face::mono, 14.0f));
            g.setColour(col::lcdText.withAlpha(0.5f));
            g.drawText(ed.ui.morph == 0 ? "TOUCH A CONTROL: ITS NAME AND VALUE"
                                        : (ed.ui.morph == 1 ? "MORPH WHEEL: TURN A KNOB TO WHERE THE WHEEL TAKES IT"
                                                            : "MORPH VELOCITY: TURN A KNOB TO WHERE A HARD KEY TAKES IT"),
                       read().reduced(12.0f, 0.0f), Justification::centredLeft, true);
        }
    }
    void mouseDown(const MouseEvent &e) override
    {
        if (name().contains(e.position) || num().contains(e.position))
            ed.programMenu();
    }
};

/* ================================================================================= building the panel === */
void NiqueEditor::section(R r, const String &title, bool sub, R rc)
{
    page.sections.push_back({r.toFloat(), (rc.isEmpty() ? r.translated(cdx, cdy) : rc).toFloat(), title, sub});
}
void NiqueEditor::text(R r, const String &t, float size, Justification j, R rc)
{
    page.texts.push_back({r.toFloat(), (rc.isEmpty() ? r.translated(cdx, cdy) : rc).toFloat(), t, size, j});
}
void NiqueEditor::arrow(Line<float> l, Line<float> lc)
{
    if (lc.getLength() <= 0.0f)
        lc = Line<float>(l.getStart().translated((float)cdx, (float)cdy), l.getEnd().translated((float)cdx, (float)cdy));
    page.arrows.push_back({l, lc});
}

Knob *NiqueEditor::knob(const char *id, const String &legend, R r, R rc, float size)
{
    auto *k = add(new Knob(proc.apvts, id, legend, ui), r, rc);
    k->knobSize = size;
    return k;
}

/* both layouts: NORMAL (21:9) and 4:3 (the modules on the right in two rows below) */
void NiqueEditor::place(bool compact)
{
    page.compact = compact;
    page.dropCache();
    for (auto &it : placed)
        it.c->setBounds(compact ? it.compact : it.normal);
    for (auto *sl : compactTwoCols) {    /* (4:3: the reverb's names in one column, there is height for it) */
        sl->twoColumns = !compact;
        sl->rowH = compact ? 17 : 14;
    }
    const int w = compact ? WC : W, h = compact ? HC : H;
    page.setSize(w, h);
    header->setBounds(0, 0, w, compact ? 112 : 64);
    proc.uiLayout = compact ? 1 : 0;
    float fit = 1.0f;
    if (auto *d = Desktop::getInstance().getDisplays().getPrimaryDisplay())
        fit = jmin(1.25f, 0.96f * (float)d->userArea.getWidth() / (float)w, 0.88f * (float)d->userArea.getHeight() / (float)h);
    getConstrainer()->setFixedAspectRatio((double)w / (double)h);
    setResizeLimits(w * 2 / 5, h * 2 / 5, (int)(w * 1.5f), (int)(h * 1.5f));
    float s = placedOnce ? fit : proc.uiScale.load();
    s = s < 0.0f ? fit : jmin(s, jmax(fit, 0.4f));
    placedOnce = true;
    setSize((int)std::lround((float)w * jmax(0.4f, s)), (int)std::lround((float)h * jmax(0.4f, s)));
    resized();
    page.repaint();
}

NiqueEditor::NiqueEditor(NiqueProcessor &p) : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&look);
    ui.touched = [this](RangedAudioParameter &rp) { touched(rp); };
    auto &a = proc.apvts;
    auto sel = [&](const char *id, StringArray names, R r, const String &legend = {}, R rc = {}) {
        auto *s = add(new Selector(a, id, std::move(names), ui), r, rc);
        s->legend = legend;
        return s;
    };
    auto key = [&](const char *id, const String &legend, R r, R rc = {}) { return add(new Key(a, id, legend, ui), r, rc); };
    auto sync = [&](Component *freeK, Component *syncK, int param) {
        syncPairs.push_back({freeK, syncK});
        syncParams.push_back(param);
    };

    header = new Header(*this);
    page.addAndMakeVisible(header);

    /* ------------------------------------------------------------------- the header's keys --- */
    add(new ActionKey("<", [this](const ModifierKeys &) {
            int n = proc.getNumPrograms();
            proc.setCurrentProgram((proc.getCurrentProgram() + n - 1) % n);
        }), {662, 16, 34, 32});
    add(new ActionKey(">", [this](const ModifierKeys &) {
            proc.setCurrentProgram((proc.getCurrentProgram() + 1) % proc.getNumPrograms());
        }), {698, 16, 34, 32});
    auto *store = add(new ActionKey("STORE", [this](const ModifierKeys &) { saveProgram(); }), {742, 2, 50, 50});
    store->legendAbove = true;
    auto *likeK = add(new ActionKey("LIKE", [this](const ModifierKeys &) { proc.like(); }), {804, 2, 56, 50});
    likeK->legendAbove = true;
    auto *lp = add(new ActionKey("<", [this](const ModifierKeys &) { proc.recallLike(proc.likePos - 1); }), {948, 16, 30, 32});
    auto *ln = add(new ActionKey(">", [this](const ModifierKeys &) { proc.recallLike(proc.likePos + 1); }), {980, 16, 30, 32});
    (void)lp;
    (void)ln;
    auto tool = [&](const String &legend, std::function<void(const ModifierKeys &)> f, int x, int w) {
        auto *k = add(new ActionKey(legend, std::move(f)), {x, 2, w, 50}, {x - 1000, 58, w, 50});
        k->legendAbove = true;
    };
    tool("INIT", [this](const ModifierKeys &) { proc.initSound(); }, 1024, 56);
    tool("MUTATE", [this](const ModifierKeys &) { proc.mutate(ui.morph); }, 1084, 66);
    tool("RANDOM", [this](const ModifierKeys &) { proc.randomize(ui.morph); }, 1154, 66);
    tool("PANIC", [this](const ModifierKeys &) { proc.panicRequest = true; }, 1238, 64);
    {                                    /* the voices' oversampling: 2, 4 or 8 times (click: the list) */
        auto *q = add(new Endless(a, "quality", ui, [](int c) { return String(2 << c); },
                                  [](int c) { return String(2 << c) + "x oversampling" + (c == 0 ? " (light on the CPU)" : c == 1 ? " (the default)" : " (for the final mix)"); }),
                      {1318, 6, 92, 52}, {318, 62, 92, 52});
        q->knob = false;
        q->ghost = 1;
        q->legend = "OVERSAMPLING";
    }
    {                                    /* the layout: NORMAL (wide) or 4:3 (for small screens) */
        auto *lay = add(new ActionKey("4:3", [this](const ModifierKeys &) { place(!page.compact); },
                                      [this] { return page.compact ? 1.0f : 0.0f; }),
                        {1416, 2, 60, 54}, {1160, 2, 60, 54});
        (void)lay;
    }
    cdy = 48;                            /* (4:3: below the taller header) */

    /* ------------------------------------------------------- MORPH, the octave, HOLD, the wheels --- */
    cdx = 11;
    section({12, 72, 176, 476}, "MORPH");
    add(new ActionKey("WHEEL", [this](const ModifierKeys &) { ui.morph = ui.morph == 1 ? 0 : 1; page.repaint(); },
                      [this] { return ui.morph == 1 ? 1.0f : 0.0f; }), {24, 94, 70, 54});
    add(new ActionKey("VELOCITY", [this](const ModifierKeys &) { ui.morph = ui.morph == 2 ? 0 : 2; page.repaint(); },
                      [this] { return ui.morph == 2 ? 1.0f : 0.0f; }), {106, 94, 70, 54});
    add(new ActionKey("CLEAR", [this](const ModifierKeys &) { proc.clearMorph(ui.morph); }), {64, 154, 72, 40});
    text({20, 196, 160, 30}, "pick one, then turn knobs:\nthe green arc is where it goes", 9.5f);
    section({22, 236, 156, 92}, "OCTAVE SHIFT", true);
    auto *oct = a.getParameter("octave");
    for (int k = 0; k < 5; k++)
        add(new Lamp([oct, k] { return (int)std::lround(oct->convertFrom0to1(oct->getValue())) == k ? 1.0f : 0.0f; }),
            {44 + k * 22, 250, 20, 16});
    text({36, 264, 128, 12}, U("-2   -1    0   +1  +2"), 9.0f);
    auto octStep = [oct](int d) {
        int v = jlimit(0, 4, (int)std::lround(oct->convertFrom0to1(oct->getValue())) + d);
        oct->beginChangeGesture();
        oct->setValueNotifyingHost(oct->convertTo0to1((float)v));
        oct->endChangeGesture();
    };
    add(new ActionKey("<", [octStep](const ModifierKeys &) { octStep(-1); }), {40, 282, 50, 30});
    add(new ActionKey(">", [octStep](const ModifierKeys &) { octStep(1); }), {110, 282, 50, 30});
    key("hold", "HOLD", {70, 334, 60, 52});
    add(new Wheel(true, [this] { return proc.bendPos.load(); }, [this](float v) { proc.uiBend = v; }), {38, 400, 48, 124});
    add(new Wheel(false, [this] { return proc.wheelPos.load(); }, [this](float v) { proc.uiWheel = v; }), {112, 400, 48, 124});
    text({30, 526, 64, 14}, "PITCH", 10.0f);
    text({104, 526, 64, 14}, "MOD", 10.0f);

    /* ------------------------------------------------------------------------- ARPEGGIATOR --- */
    /* (4:3: lying down, in the second row) */
    const int r2 = 604, r3 = 782;
    section({196, 72, 200, 230}, "ARPEGGIATOR", false, {23, r2, 330, 170});
    key("arp_run", "RUN", {206, 96, 50, 54}, {31, r2 + 40, 50, 54});
    auto *arpRate = knob("arp_rate", "RATE", {262, 90, 76, 92}, {83, r2 + 30, 80, 98});
    auto *arpDiv = knob("arp_div", "RATE", {262, 90, 76, 92}, {83, r2 + 30, 80, 98});
    sync(arpRate, arpDiv, ARP_SYNC);
    key("arp_sync", "MST CLK", {340, 96, 52, 54}, {165, r2 + 40, 54, 54});
    sel("arp_range", {"1 OCT", "2 OCT", "3 OCT", "4 OCT"}, {206, 188, 88, 104}, "RANGE", {221, r2 + 28, 64, 132});
    sel("arp_dir", {"UP", "DOWN", "UP/DN", "RND"}, {300, 188, 88, 104}, "DIRECTION", {285, r2 + 28, 64, 132});

    /* -------------------------------------------------------------------------- VOICE MODE --- */
    section({196, 310, 200, 238}, "VOICE MODE", false, {361, r2, 310, 170});
    knob("glide", "GLIDE", {204, 330, 70, 84}, {367, r2 + 30, 78, 98});
    sel("voice_mode", {"POLY", "MONO", "LEGATO"}, {284, 326, 104, 90}, "MODE", {447, r2 + 28, 86, 132});
    sel("unison", {"OFF", "1", "2", "3"}, {206, 424, 80, 114}, "UNISON", {535, r2 + 28, 56, 132});
    {
        auto *bend = add(new Endless(a, "bend", ui, [](int c) { return c == 0 ? String("oFF") : c == 15 ? String("-12") : c == 16 ? String("-24") : String(PARAMS[BEND].choices[c]); },
                                     [](int c) { return String(PARAMS[BEND].choices[c]) + (c == 0 ? "" : c < 15 ? " semitones" : " (up / down)"); }),
                         {296, 436, 92, 54}, {593, r2 + 52, 74, 54});
        bend->knob = false;
        bend->ghost = 3;
        bend->legend = "BEND RANGE";
    }

    /* --------------------------------------------------------------------------------- LFO --- */
    cdx = -197;
    section({404, 72, 168, 300}, "LFO");
    auto *lfoRate = knob("lfo_rate", "RATE/TIME", {410, 90, 84, 96});
    auto *lfoDiv = knob("lfo_div", "RATE/TIME", {410, 90, 84, 96});
    sync(lfoRate, lfoDiv, LFO_SYNC);
    key("lfo_sync", "MST CLK", {502, 96, 60, 54});
    add(new Lamp([this] { return proc.lfoLevel.load(); }), {524, 160, 16, 16});
    sel("lfo_wave", {"SQUARE", "SAW", "INV SAW", "TRIANGLE", "S/H"}, {414, 194, 96, 104});
    key("lfo_env", "ENVELOPE", {506, 222, 58, 54});
    text({412, 300, 150, 64}, "ENVELOPE: per voice,\nonce per note, RATE/TIME\nits length", 9.5f);

    /* ----------------------------------------------------------------------------- VIBRATO --- */
    section({404, 380, 168, 168}, "VIBRATO");   /* (4:3 too: under the LFO) */
    sel("vibrato", {"OFF", "DLY 1", "DLY 2", "WHEEL"}, {412, 398, 84, 138});
    knob("vib_rate", "RATE", {500, 396, 64, 70});
    knob("vib_amt", "AMOUNT", {500, 470, 64, 70});

    /* ----------------------------------------------------------------------------- MOD ENV --- */
    section({580, 72, 118, 476}, "MOD ENV");
    knob("menv_a", "ATTACK", {588, 92, 102, 104});
    knob("menv_d", "DECAY", {588, 204, 102, 104});
    knob("menv_r", "RELEASE", {588, 316, 102, 104});
    key("menv_inv", "INVERT", {590, 440, 48, 54});
    key("menv_vel", "VEL", {642, 440, 48, 54});

    /* ------------------------------------------------------------------------- OSCILLATORS --- */
    section({706, 72, 400, 476}, "OSCILLATORS");
    section({716, 102, 116, 304}, "MODULATION", true);
    knob("osc_lfo", "LFO AMT", {722, 112, 104, 114});
    knob("osc_menv", "MODENV AMT", {722, 240, 104, 114});
    arrow({800.0f, 408.0f, 880.0f, 440.0f});
    {
        auto *w = add(new Endless(a, "osc_wave", ui, waveShort, [](int w) { return String(waveLongName(w)); }, waveGroup),
                      {846, 90, 120, 146});
        w->legend = "WAVE";
        w->ghost = 3;
        auto *c = add(new Endless(a, "osc_config", ui, configShort, [](int c) { return String(configLongName(c)); }, configGroup),
                      {978, 90, 120, 146});
        c->legend = "OSC CONFIG";
        c->ghost = 4;
    }
    {
        auto *pw = add(new Selector(a, "osc_wave", {"PULSE", "SAW", "TRIANGLE", "SINE"}, ui, {0, 1, 2, 3}), {846, 244, 120, 88});
        pw->rowH = 15;
        pw->hasKey = true;
    }
    text({978, 244, 120, 84}, "turn the knobs,\nor click a display\nfor the whole list", 9.5f);
    {
        auto *k = knob("osc_ctrl", "OSC CTRL", {880, 340, 170, 174});
        (void)k;
    }
    {
        struct Hint : Component {
            NiqueEditor &ed;
            explicit Hint(NiqueEditor &e) : ed(e) { setInterceptsMouseClicks(false, false); }
            void paint(Graphics &g) override
            {
                auto &ap = ed.proc.apvts;
                int c = (int)std::lround(ap.getRawParameterValue("osc_config")->load());
                int w = (int)std::lround(ap.getRawParameterValue("osc_wave")->load());
                drawLabel(g, hintFor(c, w), getLocalBounds().toFloat(), 10.5f, Justification::centred, col::print.withAlpha(0.85f));
            }
        };
        ctrlHint = add(new Hint(*this), {716, 520, 380, 16});
        live.push_back(ctrlHint);
    }

    /* ------------------------------------------------------------------------------ FILTER --- */
    section({1114, 72, 300, 476}, "FILTER");
    section({1122, 102, 100, 304}, "MODULATION", true);
    knob("flt_lfo", "LFO AMT", {1126, 112, 92, 108});
    knob("flt_menv", "MODENV AMT", {1126, 240, 92, 108});
    arrow({1224.0f, 214.0f, 1246.0f, 214.0f});
    auto *freq = knob("flt_freq", "FREQ", {1228, 84, 180, 196});
    freq->redCap = true;
    knob("flt_res", "RESONANCE", {1228, 292, 92, 104});
    sel("flt_type", {"LP12", "LP24", "LP M", "LP TB", "BP", "HP"}, {1326, 280, 82, 146}, "TYPE")->rowH = 16;
    knob("flt_drive", "DRIVE", {1126, 416, 92, 104});
    {
        auto *kb = sel("flt_kb", {"OFF", "1/3", "2/3", "1"}, {1228, 430, 176, 84}, "KB TRACK");
        kb->twoColumns = true;
    }

    /* ----------------------------------------------------------------------------- AMP ENV --- */
    section({1422, 72, 118, 476}, "AMP ENV", false, {679, r2, 538, 170});   /* (4:3: the second row's rest) */
    knob("aenv_a", "ATTACK", {1430, 92, 102, 104}, {697, r2 + 18, 130, 146});
    knob("aenv_d", "DECAY", {1430, 204, 102, 104}, {847, r2 + 18, 130, 146});
    knob("aenv_r", "RELEASE", {1430, 316, 102, 104}, {997, r2 + 18, 130, 146});
    key("aenv_vel", "VEL", {1456, 440, 50, 54}, {1145, r2 + 52, 56, 54});

    /* ---------------------------------------------------------------------------------- FX --- */
    section({1548, 72, 136, 236}, "FX", false, {23, r3, 300, 170});
    knob("fx_amt", "RATE/AMT", {1554, 90, 80, 94}, {29, r3 + 30, 84, 100});
    key("fx_on", "ON", {1638, 96, 40, 54}, {115, r3 + 40, 44, 54});
    sel("fx_type", {"FLANGER", "PHASER", "RING MOD", "CHORUS", "ENSEMBLE", "DRIVE"}, {1556, 184, 122, 120}, {},
        {163, r3 + 26, 154, 136})->rowH = 16;

    /* ------------------------------------------------------------------------------ OUTPUT --- */
    section({1548, 316, 136, 232}, "OUTPUT", false, {1019, r3, 198, 170});
    knob("out_level", "LEVEL", {1558, 334, 116, 120}, {1025, r3 + 24, 104, 118});
    knob("out_pan", "PAN", {1572, 458, 88, 84}, {1129, r3 + 34, 84, 96})->bipolar = true;

    /* ------------------------------------------------------------------------------- DELAY --- */
    section({1692, 72, 136, 300}, "DELAY", false, {331, r3, 440, 170});
    auto *dTime = knob("dly_time", "TEMPO", {1698, 90, 82, 94}, {337, r3 + 30, 84, 100});
    auto *dDiv = knob("dly_div", "TEMPO", {1698, 90, 82, 94}, {337, r3 + 30, 84, 100});
    sync(dTime, dDiv, DLY_SYNC);
    key("dly_sync", "MST CLK", {1782, 96, 44, 54}, {423, r3 + 40, 54, 54});
    sel("dly_fb", {"1", "2", "3", "4"}, {1700, 190, 62, 106}, "FEEDBK", {481, r3 + 28, 58, 132});
    key("dly_ping", "PING", {1770, 186, 52, 54}, {543, r3 + 40, 52, 54});
    key("dly_analog", "ANALOG", {1770, 244, 52, 54}, {597, r3 + 40, 56, 54});
    knob("dly_mix", "DRY/WET", {1698, 294, 70, 74}, {655, r3 + 34, 72, 92});
    key("dly_on", "ON", {1772, 306, 48, 54}, {727, r3 + 40, 40, 54});

    /* ------------------------------------------------------------------------------ REVERB --- */
    section({1692, 380, 136, 168}, "REVERB", false, {779, r3, 232, 170});
    knob("rev_mix", "DRY/WET", {1698, 396, 70, 74}, {785, r3 + 30, 80, 98});
    key("rev_on", "ON", {1772, 404, 48, 54}, {867, r3 + 40, 44, 54});
    {
        auto *rt = sel("rev_type", {"ROOM", "STAGE 1", "STAGE 2", "HALL 1", "HALL 2"}, {1698, 474, 126, 70}, {},
                       {915, r3 + 24, 92, 140});
        rt->twoColumns = true;
        rt->rowH = 14;
        compactTwoCols.push_back(rt);
    }


    for (auto *c : page.getChildren())
        if (dynamic_cast<Lamp *>(c) || dynamic_cast<Wheel *>(c))
            live.push_back(c);
    live.push_back(header);

    addAndMakeVisible(page);
    setResizable(true, true);
    /* the window: as it was left, or (the first time, or on a new layout) as large as the screen allows */
    place(proc.uiLayout.load() == 1);
    timerCallback();
    startTimerHz(30);
}

NiqueEditor::~NiqueEditor()
{
    stopTimer();
    page.deleteAllChildren();
    setLookAndFeel(nullptr);
}

void NiqueEditor::resized()
{
    float s = (float)getWidth() / (float)(page.compact ? WC : W);
    page.setTransform(AffineTransform::scale(s));
    proc.uiScale = s;
}

void NiqueEditor::timerCallback()
{
    for (size_t i = 0; i < syncPairs.size(); i++) {   /* MST CLK: the knob shows the divisions */
        bool synced = proc.apvts.getRawParameterValue(PARAMS[syncParams[i]].id)->load() >= 0.5f;
        if (syncPairs[i].first->isVisible() == synced) {
            syncPairs[i].first->setVisible(!synced);
            syncPairs[i].second->setVisible(synced);
        }
    }
    for (auto *c : live)
        c->repaint();
    static int tick = 0;                 /* the program's dot: changed since it was chosen */
    if (++tick % 10 == 0) {
        auto v = proc.programValues(proc.getCurrentProgram());
        bool ed = false;
        auto now = proc.values();
        for (size_t i = 0; i < v.size() && i < now.size(); i++)
            if (std::fabs(v[i] - now[i]) > 1e-4f)
                ed = true;
        header->edited = ed;
    }
}

void NiqueEditor::touched(RangedAudioParameter &p)
{
    inHand = &p;
    header->repaint();
}

void NiqueEditor::programMenu()
{
    PopupMenu m;
    String last;
    PopupMenu sub;
    auto flush = [&] {
        if (last.isNotEmpty())
            m.addSubMenu(last, sub);
        sub = PopupMenu();
    };
    for (int i = 0; i < numPresets(); i++) {
        String g = presetGroup(i);
        if (g != last) {
            flush();
            last = g;
        }
        sub.addItem(i + 1, String(i + 1).paddedLeft('0', 3) + "  " + proc.getProgramName(i), true, i == proc.getCurrentProgram());
    }
    flush();
    if (!proc.userPresets.empty()) {
        PopupMenu yours;
        for (int i = numPresets(); i < proc.getNumPrograms(); i++)
            yours.addItem(i + 1, String(i + 1).paddedLeft('0', 3) + "  " + proc.getProgramName(i), true, i == proc.getCurrentProgram());
        m.addSubMenu("YOURS", yours);
    }
    m.addSeparator();
    m.addItem(-2, "Open the presets folder");
    SafePointer<NiqueEditor> self(this);
    m.showMenuAsync(PopupMenu::Options().withTargetComponent(header), [self](int r) {
        if (!self)
            return;
        if (r > 0)
            self->proc.setCurrentProgram(r - 1);
        else if (r == -2) {
            NiqueProcessor::presetFolder().createDirectory();
            NiqueProcessor::presetFolder().startAsProcess();
        }
        self->header->repaint();
    });
}

void NiqueEditor::saveProgram()
{
    auto *w = new AlertWindow("STORE", "The sound as it is now, under a name of its own (the same name again replaces it):",
                              MessageBoxIconType::NoIcon, this);
    w->addTextEditor("name", proc.getProgramName(proc.getCurrentProgram()));
    w->addButton("STORE", 1, KeyPress(KeyPress::returnKey));
    w->addButton("CANCEL", 0, KeyPress(KeyPress::escapeKey));
    SafePointer<NiqueEditor> self(this);
    w->enterModalState(true, ModalCallbackFunction::create([self, w](int r) {
                           if (self && r == 1)
                               self->proc.writePreset(w->getTextEditorContents("name"));
                           if (self)
                               self->header->repaint();
                       }),
                       true);
}

juce::AudioProcessorEditor *NiqueProcessor::createEditor() { return new NiqueEditor(*this); }
