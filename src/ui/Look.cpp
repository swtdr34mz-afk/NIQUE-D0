/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (NIQUE D0) */
#include "Look.h"
#include "BinaryData.h"

namespace nq
{
using namespace juce;

/* the fonts and the tag: made when first drawn, freed when the last editor closes (UiResources). Never function
 * statics: those are freed while the host unloads the plugin, under Windows' loader lock, where freeing fonts and
 * images can hang the host (FL Studio froze on closing). */
namespace
{
struct Store {
    Typeface::Ptr t[4];
    Image tag;
};
Store *store = nullptr;
Store &get()
{
    if (store == nullptr) {
        store = new Store();
        store->t[0] = Typeface::createSystemTypefaceFor(BinaryData::FiraSansSemiBold_ttf, BinaryData::FiraSansSemiBold_ttfSize);
        store->t[1] = Typeface::createSystemTypefaceFor(BinaryData::FiraSansBold_ttf, BinaryData::FiraSansBold_ttfSize);
        store->t[2] = Typeface::createSystemTypefaceFor(BinaryData::FiraSansExtraBold_ttf, BinaryData::FiraSansExtraBold_ttfSize);
        store->t[3] = Typeface::createSystemTypefaceFor(BinaryData::AnonymousProBold_ttf, BinaryData::AnonymousProBold_ttfSize);
        store->tag = ImageFileFormat::loadFrom(BinaryData::swtdr34mztag_png, BinaryData::swtdr34mztag_pngSize);
    }
    return *store;
}
} // namespace

UiResources::~UiResources()
{
    delete store;
    store = nullptr;
    Typeface::clearTypefaceCache();
}

Font font(Face f, float height) { return Font(FontOptions(get().t[(int)f]).withHeight(height)); }

Look::Look()
{
    setColour(ResizableWindow::backgroundColourId, col::grey);
    setColour(PopupMenu::backgroundColourId, Colour(0xff222327));
    setColour(PopupMenu::textColourId, col::print);
    setColour(PopupMenu::highlightedBackgroundColourId, col::red);
    setColour(PopupMenu::highlightedTextColourId, Colours::white);
    setColour(PopupMenu::headerTextColourId, Colour(0xffa0a2a8));
    setColour(TextEditor::backgroundColourId, col::lcd);
    setColour(TextEditor::textColourId, col::lcdText);
    setColour(TextEditor::outlineColourId, Colour(0xff55575c));
    setColour(AlertWindow::backgroundColourId, Colour(0xff222327));
    setColour(AlertWindow::textColourId, col::print);
    setColour(TextButton::buttonColourId, col::greyHi);
    setColour(TextButton::textColourOffId, col::print);
}

Typeface::Ptr Look::getTypefaceForFont(const Font &) { return get().t[0]; }

void Look::drawPopupMenuBackground(Graphics &g, int w, int h)
{
    g.fillAll(findColour(PopupMenu::backgroundColourId));
    g.setColour(Colour(0xff55575c));
    g.drawRect(0, 0, w, h, 1);
}

/* ------------------------------------------------------------------------------------------- the panel --- */
void drawPanel(Graphics &g, Rectangle<float> r)
{
    /* red enamel on metal: a soft light from above, a fine grain */
    g.setGradientFill(ColourGradient(Colour(0xffcf1621), r.getX(), r.getY(), Colour(0xffa90d17), r.getX(), r.getBottom(), false));
    g.fillRect(r);
    Random rnd(0xA1);
    for (int i = 0; i < 9000; i++) {
        float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight();
        g.setColour((rnd.nextBool() ? Colours::white : Colours::black).withAlpha(0.025f + 0.03f * rnd.nextFloat()));
        g.fillRect(x, y, 1.2f, 1.2f);
    }
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.10f), r.getX(), r.getY(), Colours::transparentWhite,
                                     r.getX(), r.getY() + 70.0f, false));
    g.fillRect(r.withHeight(70.0f));
    g.setColour(Colours::black.withAlpha(0.45f));
    g.drawRect(r, 2.0f);
}

void drawSection(Graphics &g, Rectangle<float> r, const String &title)
{
    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(r.translated(1.5f, 2.5f), 7.0f);
    g.setGradientFill(ColourGradient(Colour(0xff33363b), r.getX(), r.getY(), Colour(0xff26282c), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(col::frame.withAlpha(0.9f));
    g.drawRoundedRectangle(r.reduced(3.0f), 5.0f, 1.6f);
    if (title.isNotEmpty()) {
        auto f = font(Face::title, 15.0f);
        float w = GlyphArrangement::getStringWidth(f, title) + 22.0f;
        auto t = Rectangle<float>(w, 18.0f).withCentre({r.getCentreX(), r.getY() + 3.0f});
        g.setColour(col::bar);
        g.fillRoundedRectangle(t, 3.0f);
        g.setColour(col::frame.withAlpha(0.9f));
        g.drawRoundedRectangle(t, 3.0f, 1.2f);
        g.setColour(col::print);
        g.setFont(f);
        g.drawText(title, t, Justification::centred, false);
    }
}

void drawSub(Graphics &g, Rectangle<float> r, const String &title)
{
    g.setColour(col::frame.withAlpha(0.45f));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);
    if (title.isNotEmpty()) {
        auto f = font(Face::title, 11.5f);
        float w = GlyphArrangement::getStringWidth(f, title) + 12.0f;
        auto t = Rectangle<float>(w, 14.0f).withCentre({r.getCentreX(), r.getY()});
        g.setColour(Colour(0xff2c2e33));
        g.fillRect(t);
        g.setColour(col::print);
        g.setFont(f);
        g.drawText(title, t, Justification::centred, false);
    }
}

/* ------------------------------------------------------------------------------------------- the knobs --- */
static constexpr float A0 = MathConstants<float>::pi * 0.75f, ASPAN = MathConstants<float>::pi * 1.5f;   /* 7 to 5 o'clock */

void drawKnob(Graphics &g, Rectangle<float> r, float pos, bool redCap, bool scale, float morphTo, bool bipolar)
{
    const float d = jmin(r.getWidth(), r.getHeight());
    auto c = r.getCentre();
    const float rad = d * 0.5f;
    const bool numbers = scale && d > 70.0f;
    const float body = scale ? (numbers ? rad - 21.0f : rad - 10.0f) : rad * 0.86f;
    if (scale) {                         /* the printed scale: 0 .. 10 */
        g.setColour(col::print.withAlpha(0.85f));
        for (int k = 0; k <= 10; k++) {
            float a = -A0 + ASPAN * (float)k / 10.0f;
            Point<float> p1 = c.getPointOnCircumference(body + 3.0f, a), p2 = c.getPointOnCircumference(body + (k % 5 == 0 ? 8.0f : 6.0f), a);
            g.drawLine({p1, p2}, k % 5 == 0 ? 1.6f : 1.1f);
        }
        if (numbers) {
            g.setFont(font(Face::label, 9.0f));
            for (int k : {0, 5, 10}) {
                float a = -A0 + ASPAN * (float)k / 10.0f;
                auto p = c.getPointOnCircumference(body + 14.5f, a);
                g.drawText(String(k), Rectangle<float>(18.0f, 10.0f).withCentre(p), Justification::centred, false);
            }
        }
    }
    if (morphTo >= 0.0f) {               /* the morph's reach: a green arc */
        float a1 = -A0 + ASPAN * pos, a2 = -A0 + ASPAN * morphTo;
        Path arc;
        arc.addCentredArc(c.x, c.y, body + 4.5f, body + 4.5f, 0.0f, jmin(a1, a2), jmax(a1, a2), true);
        g.setColour(col::green.withAlpha(0.9f));
        g.strokePath(arc, PathStrokeType(3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(col::green);
        g.fillEllipse(Rectangle<float>(5.0f, 5.0f).withCentre(c.getPointOnCircumference(body + 4.5f, a2)));
    }
    (void)bipolar;
    /* the skirt, the cap */
    g.setColour(Colours::black.withAlpha(0.45f));
    g.fillEllipse(Rectangle<float>(body * 2.0f, body * 2.0f).withCentre(c.translated(1.0f, 2.5f)));
    g.setGradientFill(ColourGradient(Colour(0xff3b3d42), c.x, c.y - body, Colour(0xff0c0c0e), c.x, c.y + body, false));
    g.fillEllipse(Rectangle<float>(body * 2.0f, body * 2.0f).withCentre(c));
    const float cap = body * 0.80f;
    if (redCap)
        g.setGradientFill(ColourGradient(Colour(0xffe8323a), c.x - cap * 0.4f, c.y - cap, Colour(0xff8a0c12), c.x + cap * 0.3f, c.y + cap, true));
    else
        g.setGradientFill(ColourGradient(Colour(0xff2e3034), c.x - cap * 0.4f, c.y - cap, Colour(0xff060607), c.x + cap * 0.3f, c.y + cap, true));
    g.fillEllipse(Rectangle<float>(cap * 2.0f, cap * 2.0f).withCentre(c));
    g.setColour(Colours::white.withAlpha(0.10f));
    g.drawEllipse(Rectangle<float>(cap * 2.0f, cap * 2.0f).withCentre(c), 1.0f);
    /* the pointer */
    float a = -A0 + ASPAN * jlimit(0.0f, 1.0f, pos);
    auto p1 = c.getPointOnCircumference(body * 0.30f, a), p2 = c.getPointOnCircumference(body * 0.98f, a);
    g.setColour(Colours::white);
    g.drawLine({p1, p2}, jmax(2.0f, body * 0.11f));
}

void drawEndless(Graphics &g, Rectangle<float> r, float angle)
{
    auto c = r.getCentre();
    const float body = jmin(r.getWidth(), r.getHeight()) * 0.46f;
    g.setColour(Colours::black.withAlpha(0.45f));
    g.fillEllipse(Rectangle<float>(body * 2.0f, body * 2.0f).withCentre(c.translated(1.0f, 2.5f)));
    g.setGradientFill(ColourGradient(Colour(0xff36383d), c.x, c.y - body, Colour(0xff0a0a0c), c.x, c.y + body, false));
    g.fillEllipse(Rectangle<float>(body * 2.0f, body * 2.0f).withCentre(c));
    g.setColour(Colour(0xff1d1e21));
    for (int k = 0; k < 24; k++) {       /* the grip */
        float a = angle + MathConstants<float>::twoPi * (float)k / 24.0f;
        g.drawLine({c.getPointOnCircumference(body * 0.82f, a), c.getPointOnCircumference(body * 0.98f, a)}, 1.6f);
    }
    g.setGradientFill(ColourGradient(Colour(0xff2a2c30), c.x - body * 0.3f, c.y - body * 0.7f, Colour(0xff08080a), c.x, c.y + body * 0.7f, true));
    g.fillEllipse(Rectangle<float>(body * 1.5f, body * 1.5f).withCentre(c));
    g.setColour(Colours::white.withAlpha(0.75f));
    g.fillEllipse(Rectangle<float>(4.0f, 4.0f).withCentre(c.getPointOnCircumference(body * 0.55f, angle)));
}

void drawKey(Graphics &g, Rectangle<float> r, bool down)
{
    auto k = r.translated(0.0f, down ? 1.0f : 0.0f);
    g.setColour(Colours::black.withAlpha(0.55f));
    g.fillRoundedRectangle(r.translated(0.0f, 2.5f), 4.0f);
    g.setGradientFill(ColourGradient(down ? Colour(0xff2a2b2f) : Colour(0xff4a4c52), k.getX(), k.getY(),
                                     down ? Colour(0xff1b1c1f) : Colour(0xff26272b), k.getX(), k.getBottom(), false));
    g.fillRoundedRectangle(k, 4.0f);
    g.setColour(Colours::white.withAlpha(down ? 0.05f : 0.16f));
    g.drawRoundedRectangle(k.reduced(0.5f), 4.0f, 1.0f);
}

void drawLed(Graphics &g, Point<float> c, float r, float level, Colour on)
{
    level = jlimit(0.0f, 1.0f, level);
    Colour off = on == col::green ? col::greenOff : col::ledOff;
    if (level > 0.02f) {
        g.setColour(on.withAlpha(0.28f * level));
        g.fillEllipse(Rectangle<float>(r * 4.4f, r * 4.4f).withCentre(c));
    }
    g.setColour(off.interpolatedWith(on, level));
    g.fillEllipse(Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c));
    g.setColour(Colours::white.withAlpha(0.25f + 0.45f * level));
    g.fillEllipse(Rectangle<float>(r * 0.8f, r * 0.8f).withCentre(c.translated(-r * 0.3f, -r * 0.3f)));
    g.setColour(Colours::black.withAlpha(0.5f));
    g.drawEllipse(Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c), 0.8f);
}

void drawDisplay(Graphics &g, Rectangle<float> r, const String &text, int ghost)
{
    g.setColour(Colour(0xff050505));
    g.fillRoundedRectangle(r.expanded(2.0f), 3.0f);
    g.setColour(col::lcd);
    g.fillRoundedRectangle(r, 2.0f);
    const float h = r.getHeight() * 0.78f;
    auto f = font(Face::mono, h);
    g.setFont(f);
    g.setColour(col::lcdText.withAlpha(0.08f));   /* the unlit segments */
    g.drawText(String::repeatedString("8", ghost), r, Justification::centred, false);
    g.setColour(col::lcdText.withAlpha(0.25f));
    for (float dx : {-0.8f, 0.8f})
        g.drawText(text, r.translated(dx, 0.0f), Justification::centred, false);
    g.setColour(col::lcdText);
    g.drawText(text, r, Justification::centred, false);
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.07f), r.getX(), r.getY(), Colours::transparentWhite, r.getX(),
                                     r.getCentreY(), false));
    g.fillRoundedRectangle(r, 2.0f);
}

void drawLcd(Graphics &g, Rectangle<float> r)
{
    g.setColour(Colour(0xff050505));
    g.fillRoundedRectangle(r.expanded(2.0f), 3.0f);
    g.setColour(col::lcd);
    g.fillRoundedRectangle(r, 2.0f);
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.06f), r.getX(), r.getY(), Colours::transparentWhite, r.getX(),
                                     r.getCentreY(), false));
    g.fillRoundedRectangle(r, 2.0f);
}

void drawLabel(Graphics &g, const String &s, Rectangle<float> r, float size, Justification j, Colour c)
{
    g.setColour(c);
    g.setFont(font(Face::label, size));
    g.drawFittedText(s, r.toNearestInt(), j, 2, 0.8f);
}

void drawLogo(Graphics &g, Rectangle<float> r)
{
    /* NIQUE in heavy white, D0 boxed */
    auto f = font(Face::logo, r.getHeight() * 0.95f).withExtraKerningFactor(-0.02f);
    String a = "NIQUE";
    float wa = GlyphArrangement::getStringWidth(f, a);
    g.setColour(Colours::black.withAlpha(0.25f));
    g.setFont(f);
    g.drawText(a, r.translated(1.5f, 2.0f), Justification::centredLeft, false);
    g.setColour(Colours::white);
    g.drawText(a, r, Justification::centredLeft, false);
    auto box = Rectangle<float>(r.getX() + wa + 12.0f, r.getY() + r.getHeight() * 0.08f, r.getHeight() * 1.55f, r.getHeight() * 0.86f);
    g.fillRoundedRectangle(box, 4.0f);
    g.setColour(col::red);
    g.setFont(font(Face::logo, r.getHeight() * 0.78f));
    g.drawText("D0", box, Justification::centred, false);
}

void drawTag(Graphics &g, Rectangle<float> r)
{
    const Image &tag = get().tag;
    if (tag.isNull())
        return;
    auto fit = RectanglePlacement(RectanglePlacement::centred).appliedTo(tag.getBounds().toFloat(), r);
    g.setOpacity(0.92f);
    g.drawImage(tag, fit);
    g.setOpacity(1.0f);
}

void drawScrew(Graphics &g, Point<float> c, float r)
{
    g.setColour(Colours::black.withAlpha(0.4f));
    g.fillEllipse(Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c.translated(0.6f, 1.0f)));
    g.setGradientFill(ColourGradient(Colour(0xffd8d8d8), c.x - r, c.y - r, Colour(0xff6a6a6a), c.x + r, c.y + r, false));
    g.fillEllipse(Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c));
    g.setColour(Colour(0xff3a3a3a));
    g.drawLine(c.x - r * 0.6f, c.y, c.x + r * 0.6f, c.y, 1.3f);
    g.drawLine(c.x, c.y - r * 0.6f, c.x, c.y + r * 0.6f, 1.3f);
}

/* the mod wheel / the pitch stick: a dark slot, the wheel's ribbed face (pos 0..1) */
void drawWheel(Graphics &g, Rectangle<float> r, float pos, bool spring)
{
    g.setColour(Colour(0xff0b0b0c));
    g.fillRoundedRectangle(r, 5.0f);
    auto w = r.reduced(5.0f, 8.0f);
    g.setGradientFill(ColourGradient(Colour(0xff151618), w.getX(), w.getY(), Colour(0xff151618), w.getX(), w.getBottom(), false));
    g.fillRoundedRectangle(w, 4.0f);
    for (int k = 0; k < 22; k++) {       /* the ribs, turning with it */
        float t = std::fmod((float)k / 22.0f + pos * 0.6f, 1.0f);
        float y = w.getY() + w.getHeight() * (0.5f - 0.5f * std::cos(t * MathConstants<float>::pi));
        float shade = std::sin(t * MathConstants<float>::pi);
        g.setColour(Colour(0xff3a3c40).withAlpha(0.25f + 0.6f * shade));
        g.fillRect(w.getX() + 1.0f, y, w.getWidth() - 2.0f, 1.4f);
    }
    float y = w.getBottom() - w.getHeight() * pos;
    auto mark = Rectangle<float>(w.getX(), y - 3.0f, w.getWidth(), 6.0f);
    g.setColour(spring ? Colour(0xffc8a46a) : Colours::white.withAlpha(0.85f));
    g.fillRoundedRectangle(mark, 2.0f);
    g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.7f), w.getX(), w.getY(), Colours::transparentBlack, w.getX(),
                                     w.getY() + 18.0f, false));
    g.fillRect(w.withHeight(18.0f));
    g.setGradientFill(ColourGradient(Colours::transparentBlack, w.getX(), w.getBottom() - 18.0f, Colours::black.withAlpha(0.7f),
                                     w.getX(), w.getBottom(), false));
    g.fillRect(w.withTrimmedTop(w.getHeight() - 18.0f));
}
} // namespace nq
