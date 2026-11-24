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
static constexpr float LIGHT = -2.36f;   /* the light: from the top left (radians, 0 up, clockwise) */

/* a grain at the screen's own pixels: n x n, tiling, speckles of light and dark (powder coat, enamel) */
static Image grain(int n, float amount, uint32 seed)
{
    Image img(Image::ARGB, n, n, true);
    Image::BitmapData bd(img, Image::BitmapData::writeOnly);
    Random rnd((int64)seed);
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            float v = rnd.nextFloat() * 2.0f - 1.0f;
            v = v * std::fabs(v);        /* mostly faint, now and then a fleck */
            uint8 a = (uint8)jlimit(0, 255, (int)(std::fabs(v) * amount * 255.0f));
            bd.setPixelColour(x, y, (v > 0.0f ? Colours::white : Colours::black).withAlpha(a));
        }
    return img;
}

/* the plate inside the red frame */
Rectangle<float> plateArea(Rectangle<float> r) { return {r.getX() + 7.0f, r.getY() + 6.0f, r.getWidth() - 14.0f, r.getHeight() - 48.0f}; }

void drawPanel(Graphics &g, Rectangle<float> r, float sc)
{
    {
        Graphics::ScopedSaveState s(g);
        g.addTransform(AffineTransform::scale(sc));
        /* the red frame: enamel over steel, a soft light from above */
        g.setGradientFill(ColourGradient(Colour(0xffce1a22), r.getX(), r.getY(), Colour(0xffa50f17), r.getX(), r.getBottom(), false));
        g.fillRect(r);
        auto p = plateArea(r);
        /* the plate: anthracite, printed */
        g.setColour(Colours::black.withAlpha(0.55f));
        g.fillRoundedRectangle(p.expanded(1.5f), 7.0f);
        g.setGradientFill(ColourGradient(Colour(0xff3e3f43), p.getX(), p.getY(), Colour(0xff2f3033), p.getX(), p.getBottom(), false));
        g.fillRoundedRectangle(p, 6.0f);
    }
    /* the finish at the screen's own pixels: the enamel's fleck on the red, a finer grain on the plate */
    /* (made here, not kept in statics: an image freed while the host unloads the plugin can hang it) */
    const Image red = grain(256, 0.10f, 11), plate = grain(256, 0.06f, 23);
    g.setTiledImageFill(red, 0, 0, 1.0f);
    {
        Graphics::ScopedSaveState s(g);
        RectangleList<int> outside((r * sc).toNearestInt());
        outside.subtract((plateArea(r) * sc).toNearestInt());
        g.reduceClipRegion(outside);
        g.fillAll();
    }
    {
        Graphics::ScopedSaveState s(g);
        g.reduceClipRegion((plateArea(r) * sc).toNearestInt());
        g.setTiledImageFill(plate, 0, 0, 1.0f);
        g.fillAll();
    }
    Graphics::ScopedSaveState s(g);
    g.addTransform(AffineTransform::scale(sc));
    auto p = plateArea(r);
    g.setColour(Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(p.reduced(0.5f), 6.0f, 1.0f);
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.14f), r.getX(), r.getY(), Colours::transparentWhite, r.getX(),
                                     r.getY() + 8.0f, false));
    g.fillRect(r.withHeight(8.0f));
    g.setColour(Colours::black.withAlpha(0.5f));
    g.drawRect(r, 1.5f);
}

void drawSection(Graphics &g, Rectangle<float> r, const String &title)
{
    /* a lighter grey field, let into the plate (a dark groove round it, a lit lower edge) */
    g.setColour(Colour(0xff1d1e20));
    g.fillRoundedRectangle(r.expanded(1.2f), 6.0f);
    g.setGradientFill(ColourGradient(Colour(0xff5c5d61), r.getX(), r.getY(), Colour(0xff4f5054), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 5.5f);
    g.setColour(Colours::white.withAlpha(0.10f));
    g.drawLine(r.getX() + 6.0f, r.getY() + 0.8f, r.getRight() - 6.0f, r.getY() + 0.8f, 1.0f);
    if (title.isNotEmpty()) {            /* the name in a darker tab on the top edge */
        auto f = font(Face::logo, 14.5f).withExtraKerningFactor(0.06f);
        float w = GlyphArrangement::getStringWidth(f, title) + 20.0f;
        auto t = Rectangle<float>(r.getX() + 10.0f, r.getY() - 1.0f, w, 19.0f);
        Path tab;
        tab.addRoundedRectangle(t.getX(), t.getY(), t.getWidth(), t.getHeight(), 4.0f, 4.0f, false, false, true, true);
        g.setColour(Colour(0xff36373b));
        g.fillPath(tab);
        g.setColour(Colours::white);
        g.setFont(f);
        g.drawText(title, t.translated(0.0f, 0.5f), Justification::centred, false);
    }
}

void drawSub(Graphics &g, Rectangle<float> r, const String &title)
{
    g.setColour(col::print.withAlpha(0.75f));
    g.drawRoundedRectangle(r, 4.0f, 1.1f);
    if (title.isNotEmpty()) {
        auto f = font(Face::title, 11.5f);
        float w = GlyphArrangement::getStringWidth(f, title) + 12.0f;
        auto t = Rectangle<float>(w, 14.0f).withCentre({r.getCentreX(), r.getY()});
        g.setColour(Colour(0xff58595d));
        g.fillRect(t);
        g.setColour(col::print);
        g.setFont(f);
        g.drawText(title, t, Justification::centred, false);
    }
}

/* ------------------------------------------------------------------------------------------- the knobs --- */
static constexpr float A0 = MathConstants<float>::pi * 0.75f, ASPAN = MathConstants<float>::pi * 1.5f;   /* 7 to 5 o'clock */

/* the knob: tall, black, its sides fluted (seen from above: the flutes round the rim), a matte top, a white line from
 * the middle over the edge; its shadow falls to the lower right */
void drawKnob(Graphics &g, Rectangle<float> r, float pos, bool redCap, bool scale, float morphTo, bool bipolar)
{
    const float d = jmin(r.getWidth(), r.getHeight());
    auto c = r.getCentre();
    const float rad = d * 0.5f;
    const bool numbers = scale && d > 66.0f;
    const float body = scale ? (numbers ? rad - 19.0f : rad - 9.0f) : rad * 0.9f;
    const float PI_ = MathConstants<float>::pi;
    if (scale) {                         /* the printed scale: 0 .. 10, a figure at each step on the larger ones */
        g.setColour(col::print);
        float fs = jlimit(7.5f, 10.0f, body * 0.3f);
        g.setFont(font(Face::title, fs));
        for (int k = 0; k <= 10; k++) {
            float a = -A0 + ASPAN * (float)k / 10.0f;
            if (numbers) {
                auto p = c.getPointOnCircumference(body + 10.5f, a);
                g.drawText(String(k), Rectangle<float>(16.0f, 11.0f).withCentre(p), Justification::centred, false);
            } else {
                auto p1 = c.getPointOnCircumference(body + 3.0f, a), p2 = c.getPointOnCircumference(body + (k % 5 == 0 ? 7.5f : 5.5f), a);
                g.drawLine({p1, p2}, k % 5 == 0 ? 1.5f : 1.0f);
            }
        }
    }
    if (morphTo >= 0.0f) {               /* the morph's reach: a green arc */
        float a1 = -A0 + ASPAN * pos, a2 = -A0 + ASPAN * morphTo;
        Path arc;
        arc.addCentredArc(c.x, c.y, body + 3.5f, body + 3.5f, 0.0f, jmin(a1, a2), jmax(a1, a2), true);
        g.setColour(col::green.withAlpha(0.3f));
        g.strokePath(arc, PathStrokeType(6.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(col::green);
        g.strokePath(arc, PathStrokeType(2.4f, PathStrokeType::curved, PathStrokeType::rounded));
        g.fillEllipse(Rectangle<float>(5.5f, 5.5f).withCentre(c.getPointOnCircumference(body + 3.5f, a2)));
    }
    (void)bipolar;
    Path disc;
    disc.addEllipse(Rectangle<float>(body * 2.0f, body * 2.0f).withCentre(c));
    /* the shadow of a tall knob: long, soft, to the lower right */
    for (int k = 4; k >= 1; k--) {
        float o = body * 0.07f * (float)k;
        g.setColour(Colours::black.withAlpha(0.13f));
        g.fillEllipse(Rectangle<float>(body * 2.0f + o, body * 2.0f + o).withCentre(c.translated(o * 0.7f, o * 1.3f)));
    }
    /* the fluted side, seen round the rim: each flute lit by where it faces */
    g.setColour(Colour(0xff0b0b0c));
    g.fillPath(disc);
    const int N = jmax(18, (int)(body * 1.15f));
    for (int i = 0; i < N; i++) {
        float a = 2.0f * PI_ * ((float)i + 0.5f) / (float)N;
        float L = 0.5f + 0.5f * std::cos(a - LIGHT);
        float b = 16.0f + 62.0f * L * L * L;
        float w = 2.0f * PI_ / (float)N * 0.55f;
        Path f;
        f.addPieSegment(Rectangle<float>(body * 2.0f, body * 2.0f).withCentre(c), a - w * 0.5f, a + w * 0.5f, 0.80f);
        g.setColour(Colour((uint8)b, (uint8)b, (uint8)(b + 2)));
        g.fillPath(f);
    }
    /* the top: matte, a little dished, lit from the top left */
    const float top = body * 0.80f;
    auto tr = Rectangle<float>(top * 2.0f, top * 2.0f).withCentre(c);
    if (redCap)
        g.setGradientFill(ColourGradient(Colour(0xffe2333b), tr.getX(), tr.getY(), Colour(0xff7d0a10), tr.getRight(), tr.getBottom(), false));
    else
        g.setGradientFill(ColourGradient(Colour(0xff2c2d31), tr.getX(), tr.getY(), Colour(0xff0d0d0f), tr.getRight(), tr.getBottom(), false));
    g.fillEllipse(tr);
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(redCap ? 0.22f : 0.12f), tr.getX() + top * 0.3f, tr.getY() + top * 0.25f,
                                     Colours::transparentWhite, c.x, c.y, true));
    g.fillEllipse(tr.reduced(top * 0.1f));
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.28f), tr.getX(), tr.getY(), Colours::black.withAlpha(0.5f),
                                     tr.getRight(), tr.getBottom(), false));
    g.drawEllipse(tr.reduced(0.5f), 1.0f);
    if (pos < 0.0f)                      /* (an endless knob: no line) */
        return;
    /* the line: from near the middle over the edge */
    float a = -A0 + ASPAN * jlimit(0.0f, 1.0f, pos);
    auto p1 = c.getPointOnCircumference(body * 0.22f, a), p2 = c.getPointOnCircumference(body * 1.0f, a);
    float lw = jmax(2.2f, body * 0.12f);
    g.setColour(Colours::black.withAlpha(0.35f));
    g.drawLine(Line<float>(p1, p2).withShortenedStart(-0.5f), lw + 1.6f);
    g.setColour(Colour(0xfff4f4f2));
    g.drawLine({p1, p2}, lw);
}

void drawEndless(Graphics &g, Rectangle<float> r, float angle)
{
    /* an endless knob: as the others, no line, a dimple */
    auto c = r.getCentre();
    const float body = jmin(r.getWidth(), r.getHeight()) * 0.46f;
    drawKnob(g, Rectangle<float>(body * 2.0f / 0.9f, body * 2.0f / 0.9f).withCentre(c), -1.0f, false, false, -1.0f, false);
    g.setColour(Colour(0xff050506));
    g.fillEllipse(Rectangle<float>(5.0f, 5.0f).withCentre(c.getPointOnCircumference(body * 0.5f, angle)));
    g.setColour(Colours::white.withAlpha(0.15f));
    g.drawEllipse(Rectangle<float>(5.0f, 5.0f).withCentre(c.getPointOnCircumference(body * 0.5f, angle)).translated(0.3f, 0.5f), 0.6f);
}

/* a rubber key: grey (the program's and the tools') or black (the selectors' and switches'), its top a soft pillow */
void drawKey(Graphics &g, Rectangle<float> r, bool down, bool grey)
{
    auto k = r.translated(0.0f, down ? 1.2f : 0.0f);
    const float rr = jmin(5.0f, k.getHeight() * 0.3f);
    g.setColour(Colours::black.withAlpha(0.5f));   /* the well round it, the shadow */
    g.fillRoundedRectangle(r.expanded(1.5f).translated(0.0f, 1.0f), rr + 1.5f);
    if (!down) {
        g.setColour(Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(r.translated(1.0f, 3.0f), rr);
    }
    Colour hi = grey ? Colour(0xffc4c5c9) : Colour(0xff3a3b3f), lo = grey ? Colour(0xff8e8f94) : Colour(0xff161618);
    if (down) {
        hi = hi.darker(0.25f);
        lo = lo.darker(0.25f);
    }
    g.setGradientFill(ColourGradient(hi, k.getX(), k.getY(), lo, k.getX(), k.getBottom(), false));
    g.fillRoundedRectangle(k, rr);
    auto pillow = k.reduced(k.getWidth() * 0.12f, k.getHeight() * 0.22f).translated(0.0f, -k.getHeight() * 0.06f);
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(grey ? 0.35f : 0.12f), pillow.getX(), pillow.getY(),
                                     Colours::transparentWhite, pillow.getX(), pillow.getBottom(), false));
    g.fillRoundedRectangle(pillow, rr);
    g.setColour(Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(k, rr, 0.8f);
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

/* the 7-segment LED display: a segment's bits a b c d e f g, a decimal point after any figure */
static int segBits(juce_wchar ch)
{
    switch (CharacterFunctions::toUpperCase(ch)) {
    case '0': case 'O': case 'D': return 0x3f;
    case '1': case 'I': return 0x06;
    case '2': case 'Z': return 0x5b;
    case '3': return 0x4f;
    case '4': return 0x66;
    case '5': case 'S': return 0x6d;
    case '6': case 'G': return 0x7d;
    case '7': return 0x07;
    case '8': case 'B': return 0x7f;
    case '9': return 0x6f;
    case 'A': return 0x77;
    case 'C': return 0x39;
    case 'E': return 0x79;
    case 'F': return 0x71;
    case 'H': case 'K': case 'X': return 0x76;
    case 'J': return 0x1e;
    case 'L': return 0x38;
    case 'M': case 'N': return 0x37;
    case 'P': return 0x73;
    case 'Q': return 0x67;
    case 'R': return 0x50;
    case 'T': return 0x78;
    case 'U': case 'V': case 'W': return 0x3e;
    case 'Y': return 0x6e;
    case '-': return 0x40;
    case '_': return 0x08;
    default: return 0;
    }
}
/* lower case where a 7-segment display shows it so (b, d, n, o, r, t, u, h, c, q) */
static int segLower(juce_wchar ch)
{
    switch (ch) {
    case 'b': return 0x7c;
    case 'c': return 0x58;
    case 'd': return 0x5e;
    case 'h': return 0x74;
    case 'n': return 0x54;
    case 'o': return 0x5c;
    case 'q': return 0x67;
    case 'r': return 0x50;
    case 't': return 0x78;
    case 'u': return 0x1c;
    case 'i': return 0x04;
    default: return segBits(ch);
    }
}

static void drawDigit(Graphics &g, Rectangle<float> c, int bits, bool dp, Colour on, Colour off)
{
    const float w = c.getWidth(), h = c.getHeight(), t = jmax(1.6f, w * 0.17f), sl = 0.10f;   /* thickness, slant */
    auto P = [&](float x, float y) { return Point<float>(c.getX() + x + (h - y) * sl, c.getY() + y); };
    auto hseg = [&](float y) {
        Path p;
        float x0 = t * 0.55f, x1 = w - t * 0.55f;
        p.startNewSubPath(P(x0, y));
        p.lineTo(P(x0 + t * 0.5f, y - t * 0.5f));
        p.lineTo(P(x1 - t * 0.5f, y - t * 0.5f));
        p.lineTo(P(x1, y));
        p.lineTo(P(x1 - t * 0.5f, y + t * 0.5f));
        p.lineTo(P(x0 + t * 0.5f, y + t * 0.5f));
        p.closeSubPath();
        return p;
    };
    auto vseg = [&](float x, float y0, float y1) {
        Path p;
        y0 += t * 0.55f;
        y1 -= t * 0.55f;
        p.startNewSubPath(P(x, y0));
        p.lineTo(P(x + t * 0.5f, y0 + t * 0.5f));
        p.lineTo(P(x + t * 0.5f, y1 - t * 0.5f));
        p.lineTo(P(x, y1));
        p.lineTo(P(x - t * 0.5f, y1 - t * 0.5f));
        p.lineTo(P(x - t * 0.5f, y0 + t * 0.5f));
        p.closeSubPath();
        return p;
    };
    const float m = h * 0.5f, top = t * 0.5f, bot = h - t * 0.5f, l = t * 0.5f, rgt = w - t * 0.5f;
    Path s[7] = {hseg(top), vseg(rgt, top, m), vseg(rgt, m, bot), hseg(bot), vseg(l, m, bot), vseg(l, top, m), hseg(m)};
    for (int k = 0; k < 7; k++) {
        bool lit = (bits >> k) & 1;
        if (lit) {                       /* the glow, then the segment */
            g.setColour(on.withAlpha(0.22f));
            g.strokePath(s[k], PathStrokeType(t * 0.9f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour(lit ? on : off);
        g.fillPath(s[k]);
    }
    auto dpc = P(w + t * 0.55f, h - t * 0.4f);
    g.setColour(dp ? on : off);
    g.fillEllipse(Rectangle<float>(t * 1.05f, t * 1.05f).withCentre(dpc));
}

void drawDisplay(Graphics &g, Rectangle<float> r, const String &text, int cells)
{
    g.setColour(Colour(0xff060606));     /* the bezel and the dark red glass */
    g.fillRoundedRectangle(r.expanded(2.5f), 3.5f);
    g.setGradientFill(ColourGradient(Colour(0xff2a0606), r.getX(), r.getY(), Colour(0xff160303), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 2.0f);
    /* the figures and their points */
    std::vector<std::pair<int, bool>> figs;
    for (int i = 0; i < text.length(); i++) {
        juce_wchar ch = text[i];
        if ((ch == '.' || ch == ',') && !figs.empty())
            figs.back().second = true;
        else if (ch == ' ')
            figs.push_back({0, false});
        else
            figs.push_back({segLower(ch), false});
    }
    cells = jmax(cells, (int)figs.size());
    const float ch = r.getHeight() * 0.68f, cw = ch * 0.52f, gap = cw * 0.42f;
    const float total = (float)cells * cw + (float)(cells - 1) * gap;
    float x = r.getCentreX() - total * 0.5f - ch * 0.05f;
    const float y = r.getCentreY() - ch * 0.5f;
    int pad = cells - (int)figs.size();  /* (right-aligned, as a display does) */
    for (int k = 0; k < cells; k++) {
        auto f = k < pad ? std::pair<int, bool>{0, false} : figs[(size_t)(k - pad)];
        drawDigit(g, {x, y, cw, ch}, f.first, f.second, col::lcdText, Colour(0xff3a0b08));
        x += cw + gap;
    }
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.09f), r.getX(), r.getY(), Colours::transparentWhite, r.getX(),
                                     r.getY() + r.getHeight() * 0.45f, false));
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

/* the mod wheel (a dark ribbed wheel in its slot) or the pitch stick (a block of wood, as on the instrument) */
void drawWheel(Graphics &g, Rectangle<float> r, float pos, bool spring)
{
    g.setColour(Colour(0xff050506));
    g.fillRoundedRectangle(r, 5.0f);
    auto w = r.reduced(5.0f, 8.0f);
    if (spring) {                        /* the stick: its block slides along the slot */
        g.setColour(Colour(0xff101012));
        g.fillRoundedRectangle(w.withSizeKeepingCentre(10.0f, w.getHeight()), 3.0f);
        float bh = jmin(54.0f, w.getHeight() * 0.3f);
        float y = w.getBottom() - bh - (w.getHeight() - bh) * pos;
        auto b = Rectangle<float>(w.getX() - 2.0f, y, w.getWidth() + 4.0f, bh);
        g.setColour(Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(b.translated(2.0f, 4.0f), 4.0f);
        g.setGradientFill(ColourGradient(Colour(0xffd89a55), b.getX(), b.getY(), Colour(0xff9c5f25), b.getRight(), b.getBottom(), false));
        g.fillRoundedRectangle(b, 4.0f);
        Random rnd(77);
        for (int k = 0; k < 18; k++) {   /* the grain */
            float gx = b.getX() + 3.0f + rnd.nextFloat() * (b.getWidth() - 6.0f);
            Path p;
            p.startNewSubPath(gx, b.getY() + 2.0f);
            p.cubicTo(gx + rnd.nextFloat() * 3.0f - 1.5f, b.getY() + bh * 0.35f, gx + rnd.nextFloat() * 3.0f - 1.5f,
                      b.getY() + bh * 0.7f, gx + rnd.nextFloat() * 2.0f - 1.0f, b.getBottom() - 2.0f);
            g.setColour(Colour(0xff6b3a12).withAlpha(0.18f + 0.25f * rnd.nextFloat()));
            g.strokePath(p, PathStrokeType(0.6f + rnd.nextFloat() * 0.8f));
        }
        g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.35f), b.getX(), b.getY(), Colours::transparentWhite, b.getX(),
                                         b.getY() + 10.0f, false));
        g.fillRoundedRectangle(b, 4.0f);
        g.setColour(Colour(0xff5a3010).withAlpha(0.6f));
        g.drawRoundedRectangle(b, 4.0f, 0.8f);
        return;
    }
    g.setColour(Colour(0xff151618));
    g.fillRoundedRectangle(w, 4.0f);
    for (int k = 0; k < 26; k++) {       /* the ribs, turning with it */
        float t = std::fmod((float)k / 26.0f + pos * 0.6f, 1.0f);
        float y = w.getY() + w.getHeight() * (0.5f - 0.5f * std::cos(t * MathConstants<float>::pi));
        float shade = std::sin(t * MathConstants<float>::pi);
        g.setColour(Colour(0xff45474c).withAlpha(0.2f + 0.65f * shade));
        g.fillRect(w.getX() + 1.0f, y, w.getWidth() - 2.0f, 1.3f);
    }
    float y = w.getBottom() - w.getHeight() * pos;
    g.setColour(Colours::white.withAlpha(0.9f));
    g.fillRoundedRectangle(Rectangle<float>(w.getX(), y - 2.5f, w.getWidth(), 5.0f), 2.0f);
    g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.8f), w.getX(), w.getY(), Colours::transparentBlack, w.getX(),
                                     w.getY() + 24.0f, false));
    g.fillRect(w.withHeight(24.0f));
    g.setGradientFill(ColourGradient(Colours::transparentBlack, w.getX(), w.getBottom() - 24.0f, Colours::black.withAlpha(0.8f),
                                     w.getX(), w.getBottom(), false));
    g.fillRect(w.withTrimmedTop(w.getHeight() - 24.0f));
}
} // namespace nq
