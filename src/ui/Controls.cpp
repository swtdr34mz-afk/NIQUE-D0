/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
#include "Controls.h"

namespace nq
{
using namespace juce;

static RangedAudioParameter &need(APVTS &a, const String &id)
{
    auto *p = a.getParameter(id);
    jassert(p != nullptr);
    return *p;
}

/* a right-click: the host's menu for this parameter (MIDI learn, automation ...), never a change of its value */
static bool hostMenu(Component &c, const MouseEvent &e, RangedAudioParameter &p)
{
    if (!e.mods.isPopupMenu())
        return false;
    if (auto *ed = c.findParentComponentOfClass<AudioProcessorEditor>())
        if (auto *host = ed->getHostContext())
            if (auto menu = host->getContextMenuForParameter(&p))
                menu->showNativeMenu(ed->getMouseXYRelative());
    return true;
}

/* ============================================================================================== Knob === */
Knob::Knob(APVTS &a, const String &id, const String &lg, UiState &u)
    : param(need(a, id)), legend(lg), ui(u), att(param, [this](float) { repaint(); }, nullptr)
{
    mw = a.getParameter("mw_" + id);
    mv = a.getParameter("mv_" + id);
    if (mw)
        attW = std::make_unique<ParameterAttachment>(*mw, [this](float) { repaint(); }, nullptr);
    if (mv)
        attV = std::make_unique<ParameterAttachment>(*mv, [this](float) { repaint(); }, nullptr);
    att.sendInitialUpdate();
    setRepaintsOnMouseActivity(false);
}

Rectangle<float> Knob::knobArea() const
{
    auto r = getLocalBounds().toFloat();
    if (legend.isNotEmpty())
        r.removeFromBottom(19.0f);
    float d = knobSize > 0.0f ? knobSize : jmin(r.getWidth(), r.getHeight());
    return Rectangle<float>(d, d).withCentre(r.getCentre());
}

float Knob::morphTarget(int which) const
{
    auto *m = which == 1 ? mw : which == 2 ? mv : nullptr;
    if (!m)
        return -1.0f;
    float off = (m->getValue() - 0.5f) * 2.0f;
    if (std::fabs(off) < 0.004f)
        return -1.0f;
    return jlimit(0.0f, 1.0f, param.getValue() + off);
}

void Knob::paint(Graphics &g)
{
    auto k = knobArea();
    float to = ui.morph ? morphTarget(ui.morph) : -1.0f;
    drawKnob(g, k, param.getValue(), redCap, scale, to, bipolar);
    if (mw || mv) {                      /* the morph LED: lit if a morph takes this knob anywhere */
        bool any = ui.morph ? morphTarget(ui.morph) >= 0.0f : (morphTarget(1) >= 0.0f || morphTarget(2) >= 0.0f);
        bool can = ui.morph == 0 || (ui.morph == 1 ? mw != nullptr : mv != nullptr);
        auto c = Point<float>(k.getX() + 2.0f, k.getBottom() - 1.0f);
        drawLed(g, c, 3.0f, any ? 1.0f : (ui.morph && can ? 0.25f : 0.0f), col::green);
    }
    if (legend.isNotEmpty())
        drawLabel(g, legend, getLocalBounds().toFloat().removeFromBottom(19.0f), 11.0f);
}

void Knob::mouseEnter(const MouseEvent &) { ui.touched(editing ? *editing : param); }

void Knob::mouseDown(const MouseEvent &e)
{
    editing = nullptr;
    if (hostMenu(*this, e, param))
        return;
    editing = &param;
    if (ui.morph == 1 && mw)
        editing = mw;
    else if (ui.morph == 2 && mv)
        editing = mv;
    startY = e.position.y;
    startValue = editing == &param ? param.getValue() : jmax(0.0f, morphTarget(ui.morph) < 0.0f ? param.getValue() : morphTarget(ui.morph));
    if (editing == &param)
        att.beginGesture();
    else
        (editing == mw ? attW : attV)->beginGesture();
    ui.touched(*editing);
}

void Knob::setFromDrag(float v)
{
    v = jlimit(0.0f, 1.0f, v);
    if (editing == &param)
        att.setValueAsPartOfGesture(param.convertFrom0to1(v));
    else {                               /* a morph: where it takes the knob, kept as its distance from it */
        float m = jlimit(0.0f, 1.0f, 0.5f + (v - param.getValue()) * 0.5f);
        auto &a = editing == mw ? *attW : *attV;
        a.setValueAsPartOfGesture(editing->convertFrom0to1(m));
    }
    ui.touched(*editing);
    repaint();
}

void Knob::mouseDrag(const MouseEvent &e)
{
    if (!editing || e.mods.isPopupMenu())
        return;
    float speed = e.mods.isShiftDown() || e.mods.isCommandDown() ? 1.0f / 1000.0f : 1.0f / 220.0f;
    setFromDrag(startValue + (startY - e.position.y) * speed);
}

void Knob::mouseUp(const MouseEvent &)
{
    if (editing == nullptr)              /* (after a right-click: nothing was begun) */
        return;
    if (editing == &param)
        att.endGesture();
    else if (editing == mw)
        attW->endGesture();
    else if (editing == mv)
        attV->endGesture();
    editing = nullptr;
}

void Knob::mouseDoubleClick(const MouseEvent &e)
{
    if (e.mods.isPopupMenu())
        return;
    if (ui.morph == 1 && mw)
        attW->setValueAsCompleteGesture(mw->convertFrom0to1(0.5f));
    else if (ui.morph == 2 && mv)
        attV->setValueAsCompleteGesture(mv->convertFrom0to1(0.5f));
    else
        att.setValueAsCompleteGesture(param.convertFrom0to1(param.getDefaultValue()));
}

void Knob::mouseWheelMove(const MouseEvent &, const MouseWheelDetails &w)
{
    float d = (w.deltaY != 0.0f ? w.deltaY : -w.deltaX) * (w.isReversed ? -1.0f : 1.0f);
    float step = d > 0.0f ? 0.01f : -0.01f;
    if (std::fabs(d) < 0.001f)
        return;
    editing = &param;
    if (ui.morph == 1 && mw)
        editing = mw;
    else if (ui.morph == 2 && mv)
        editing = mv;
    float base = editing == &param ? param.getValue() : jmax(0.0f, morphTarget(ui.morph) < 0.0f ? param.getValue() : morphTarget(ui.morph));
    if (editing == &param)
        att.setValueAsCompleteGesture(param.convertFrom0to1(jlimit(0.0f, 1.0f, base + step)));
    else {
        float m = jlimit(0.0f, 1.0f, 0.5f + (jlimit(0.0f, 1.0f, base + step) - param.getValue()) * 0.5f);
        (editing == mw ? attW : attV)->setValueAsCompleteGesture(editing->convertFrom0to1(m));
    }
    ui.touched(*editing);
    editing = nullptr;
}

/* =============================================================================================== Key === */
Key::Key(APVTS &a, const String &id, const String &lg, UiState &u)
    : param(need(a, id)), legend(lg), ui(u), att(param, [this](float) { repaint(); }, nullptr)
{
    att.sendInitialUpdate();
}

void Key::paint(Graphics &g)
{
    auto r = getLocalBounds().toFloat();
    auto ledArea = r.removeFromTop(12.0f);
    drawLed(g, ledArea.getCentre(), 3.6f, on() ? 1.0f : 0.0f);
    Rectangle<float> lab;
    if (legend.isNotEmpty())
        lab = r.removeFromBottom(18.0f);
    auto k = Rectangle<float>(jmin(r.getWidth() - 2.0f, 40.0f), jmin(r.getHeight() - 4.0f, 20.0f)).withCentre(r.getCentre());
    drawKey(g, k, down);
    if (legend.isNotEmpty())
        drawLabel(g, legend, lab.expanded(8.0f, 0.0f), 10.5f);
}

void Key::mouseEnter(const MouseEvent &) { ui.touched(param); }

void Key::mouseDown(const MouseEvent &e)
{
    if (hostMenu(*this, e, param))
        return;
    down = true;
    att.setValueAsCompleteGesture(on() ? 0.0f : 1.0f);
    ui.touched(param);
    repaint();
}

void Key::mouseUp(const MouseEvent &)
{
    down = false;
    repaint();
}

/* ========================================================================================== Selector === */
Selector::Selector(APVTS &a, const String &id, StringArray n, UiState &u, std::vector<int> sub)
    : param(need(a, id)), names(std::move(n)), ui(u), subset(std::move(sub)), att(param, [this](float) { repaint(); }, nullptr)
{
    if (subset.empty())
        for (int k = 0; k < names.size(); k++)
            subset.push_back(k);
    att.sendInitialUpdate();
}

int Selector::current() const { return (int)std::lround(param.convertFrom0to1(param.getValue())); }

Rectangle<float> Selector::keyArea() const
{
    auto r = getLocalBounds().toFloat();
    float h = legend.isNotEmpty() ? 38.0f : 24.0f;
    auto k = keyAtTop ? r.removeFromTop(h) : r.removeFromBottom(h);
    return Rectangle<float>(jmin(k.getWidth() - 4.0f, 40.0f), 20.0f).withCentre({k.getCentreX(), k.getY() + 12.0f});
}

Rectangle<float> Selector::rowArea(int k) const
{
    auto r = getLocalBounds().toFloat();
    if (hasKey) {
        float h = legend.isNotEmpty() ? 38.0f : 24.0f;
        if (keyAtTop)
            r.removeFromTop(h);
        else
            r.removeFromBottom(h);
    }
    int n = (int)subset.size();
    if (twoColumns) {
        int rows = (n + 1) / 2;
        float w = r.getWidth() * 0.5f;
        return {r.getX() + (float)(k / rows) * w, r.getY() + (float)(k % rows) * (float)rowH, w, (float)rowH};
    }
    float top = r.getY() + jmax(0.0f, (r.getHeight() - (float)(n * rowH)) * 0.5f);
    return {r.getX(), top + (float)(k * rowH), r.getWidth(), (float)rowH};
}

void Selector::paint(Graphics &g)
{
    int cur = current();
    for (int k = 0; k < (int)subset.size(); k++) {
        auto row = rowArea(k);
        bool lit = subset[(size_t)k] == cur;
        drawLed(g, {row.getX() + 7.0f, row.getCentreY()}, 3.3f, lit ? 1.0f : 0.0f);
        drawLabel(g, names[k], row.withTrimmedLeft(15.0f), 10.5f, Justification::centredLeft,
                  lit ? Colours::white : col::print.withAlpha(0.8f));
    }
    if (hasKey) {
        drawKey(g, keyArea(), down);
        if (legend.isNotEmpty())
            drawLabel(g, legend, keyArea().translated(0.0f, 20.0f).expanded(20.0f, 0.0f).withHeight(17.0f), 10.5f);
    }
}

void Selector::choose(int c)
{
    float n = param.convertTo0to1((float)c);
    att.setValueAsCompleteGesture(param.convertFrom0to1(n));
    ui.touched(param);
}

void Selector::mouseEnter(const MouseEvent &) { ui.touched(param); }

void Selector::mouseDown(const MouseEvent &e)
{
    if (hostMenu(*this, e, param))
        return;
    if (hasKey && keyArea().expanded(4.0f).contains(e.position)) {
        down = true;
        int cur = current(), pos = -1;
        for (int k = 0; k < (int)subset.size(); k++)
            if (subset[(size_t)k] == cur)
                pos = k;
        int next = e.mods.isShiftDown() ? (pos <= 0 ? (int)subset.size() - 1 : pos - 1) : (pos + 1) % (int)subset.size();
        choose(subset[(size_t)next]);
        repaint();
        return;
    }
    for (int k = 0; k < (int)subset.size(); k++)
        if (rowArea(k).contains(e.position))
            choose(subset[(size_t)k]);
}

void Selector::mouseUp(const MouseEvent &)
{
    down = false;
    repaint();
}

/* =========================================================================================== Endless === */
Endless::Endless(APVTS &a, const String &id, UiState &u, std::function<String(int)> sn, std::function<String(int)> ln,
                 std::function<String(int)> grp)
    : param(need(a, id)), ui(u), shortName(std::move(sn)), longName(std::move(ln)), group(std::move(grp)),
      att(param, [this](float) { repaint(); }, nullptr)
{
    att.sendInitialUpdate();
}

int Endless::count() const
{
    if (auto *c = dynamic_cast<AudioParameterChoice *>(&param))
        return c->choices.size();
    return 2;
}

int Endless::current() const { return (int)std::lround(param.convertFrom0to1(param.getValue())); }

Rectangle<float> Endless::displayArea() const
{
    auto r = getLocalBounds().toFloat();
    if (!knob)
        return r.withTrimmedBottom(legend.isNotEmpty() ? 16.0f : 0.0f).reduced(3.0f);
    return r.removeFromTop(38.0f).reduced(6.0f, 4.0f);
}

void Endless::paint(Graphics &g)
{
    drawDisplay(g, displayArea(), shortName(current()), ghost);
    auto r = getLocalBounds().toFloat();
    if (knob) {
        r.removeFromTop(42.0f);
        if (legend.isNotEmpty())
            drawLabel(g, legend, r.removeFromBottom(15.0f), 11.0f);
        float d = jmin(r.getWidth(), r.getHeight());
        drawEndless(g, Rectangle<float>(d, d).withCentre(r.getCentre()), angle);
    } else if (legend.isNotEmpty())
        drawLabel(g, legend, r.removeFromBottom(15.0f), 10.5f);
}

void Endless::step(int d)
{
    int n = count(), c = jlimit(0, n - 1, current() + d);
    att.setValueAsCompleteGesture((float)c);
    angle += 0.26f * (float)d;
    ui.touched(param);
    repaint();
}

void Endless::menu()
{
    PopupMenu m;
    String last;
    for (int k = 0; k < count(); k++) {
        if (group) {
            String gname = group(k);
            if (gname != last) {
                if (k > 0)
                    m.addSeparator();
                m.addSectionHeader(gname);
                last = gname;
            }
        }
        m.addItem(k + 1, longName(k), true, k == current());
    }
    SafePointer<Endless> self(this);
    m.showMenuAsync(PopupMenu::Options().withTargetComponent(this).withMaximumNumColumns(4), [self](int r) {
        if (self && r > 0) {
            self->att.setValueAsCompleteGesture((float)(r - 1));
            self->ui.touched(self->param);
        }
    });
}

void Endless::mouseEnter(const MouseEvent &) { ui.touched(param); }

void Endless::mouseDown(const MouseEvent &e)
{
    popup = hostMenu(*this, e, param);
    if (popup)
        return;
    acc = 0.0f;
    lastY = e.position.y;
    dragged = false;
    ui.touched(param);
}

void Endless::mouseDrag(const MouseEvent &e)
{
    if (popup || e.mods.isPopupMenu())
        return;
    acc += (lastY - e.position.y) / 14.0f;
    lastY = e.position.y;
    while (acc >= 1.0f) {
        step(1);
        acc -= 1.0f;
        dragged = true;
    }
    while (acc <= -1.0f) {
        step(-1);
        acc += 1.0f;
        dragged = true;
    }
}

void Endless::mouseUp(const MouseEvent &e)
{
    if (popup) {
        popup = false;
        return;
    }
    if (!dragged && e.mouseWasClicked())
        menu();
}

void Endless::mouseWheelMove(const MouseEvent &, const MouseWheelDetails &w)
{
    float d = (w.deltaY != 0.0f ? w.deltaY : -w.deltaX) * (w.isReversed ? -1.0f : 1.0f);
    if (d > 0.0f)
        step(1);
    else if (d < 0.0f)
        step(-1);
}

/* ========================================================================================= ActionKey === */
ActionKey::ActionKey(const String &lg, std::function<void(const ModifierKeys &)> a, std::function<float()> l)
    : legend(lg), action(std::move(a)), led(std::move(l))
{
}

void ActionKey::paint(Graphics &g)
{
    auto r = getLocalBounds().toFloat();
    if (led)
        drawLed(g, r.removeFromTop(12.0f).getCentre(), 3.6f, led());
    if (legend == "<" || legend == ">") {  /* an arrow on the key */
        auto k = Rectangle<float>(jmin(r.getWidth() - 2.0f, 44.0f), jmin(r.getHeight() - 4.0f, 22.0f)).withCentre(r.getCentre());
        drawKey(g, k, down, true);
        auto c = k.getCentre().translated(0.0f, down ? 1.0f : 0.0f);
        float d = legend == ">" ? 1.0f : -1.0f;
        Path t;
        t.addTriangle(c.x + 5.0f * d, c.y, c.x - 4.0f * d, c.y - 5.0f, c.x - 4.0f * d, c.y + 5.0f);
        g.setColour(col::print);
        g.fillPath(t);
        return;
    }
    Rectangle<float> lab;
    if (legend.isNotEmpty())
        lab = legendAbove ? r.removeFromTop(18.0f) : r.removeFromBottom(18.0f);
    auto k = Rectangle<float>(jmin(r.getWidth() - 2.0f, 44.0f), jmin(r.getHeight() - 4.0f, 20.0f)).withCentre(r.getCentre());
    drawKey(g, k, down, true);
    if (legend.isNotEmpty())
        drawLabel(g, legend, lab.expanded(10.0f, 0.0f), 10.5f);
}

void ActionKey::mouseDown(const MouseEvent &e)
{
    if (e.mods.isPopupMenu())            /* (no parameter: a right-click does nothing) */
        return;
    down = true;
    repaint();
    if (action)
        action(e.mods);
}

void ActionKey::mouseUp(const MouseEvent &)
{
    down = false;
    repaint();
}

/* ============================================================================================= Wheel === */
Wheel::Wheel(bool s, std::function<float()> g, std::function<void(float)> st) : spring(s), get(std::move(g)), set(std::move(st)) {}

void Wheel::paint(Graphics &g) { drawWheel(g, getLocalBounds().toFloat(), get(), spring); }

void Wheel::mouseDown(const MouseEvent &e)
{
    if (e.mods.isPopupMenu())
        return;
    startV = get();
    startY = e.position.y;
}

void Wheel::mouseDrag(const MouseEvent &e)
{
    if (e.mods.isPopupMenu())
        return;
    float h = (float)getHeight() - 16.0f;
    set(jlimit(0.0f, 1.0f, startV + (startY - e.position.y) / jmax(10.0f, h)));
    repaint();
}

void Wheel::mouseUp(const MouseEvent &)
{
    if (spring)
        set(0.5f);
    repaint();
}
} // namespace nq
