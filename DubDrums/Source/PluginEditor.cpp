// Interface do Dub Drums — visual copiado do DubMastor Dub Chords
#include "PluginEditor.h"

#ifndef DD_VERSION_SHORT
 #define DD_VERSION_SHORT "1.0"
#endif

// Visual da família DubMastor (mesmo do Big Knob): painel de metal creme envelhecido,
// laterais de madeira, knobs de baquelite com saia de alumínio, lâmpadas piloto e displays âmbar.
namespace colours
{
    const juce::Colour plate     { 0xffe4d8bc };
    const juce::Colour plateDark { 0xffc9b994 };
    const juce::Colour ink       { 0xff2b241d };
    const juce::Colour inkSoft   { 0xff5c5144 };
    const juce::Colour labelBg   { 0xff1a1714 };
    const juce::Colour cream     { 0xffefe5cc };
    const juce::Colour jamGreen  { 0xff0b8a3c };
    const juce::Colour jamGold   { 0xffeabd12 };
    const juce::Colour amber     { 0xffffa928 };
    const juce::Colour glass     { 0xff17110a };
    const juce::Colour woodDark  { 0xff2e1a0d };
    const juce::Colour woodLight { 0xff6b4423 };
}

static constexpr float kStartAngle = juce::MathConstants<float>::pi * 1.2f;
static constexpr float kEndAngle   = juce::MathConstants<float>::pi * 2.8f;

static juce::Font stencil (float size, float kerning = 0.08f)
{
    return juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (kerning);
}

static float knobReduction (const juce::Slider& s) { return s.getName() == "root" ? 34.0f : 13.0f; }

// área circular do knob dentro do slider (mesma conta do LookAndFeel)
static juce::Rectangle<float> knobCircle (const juce::Slider& s)
{
    auto r = s.getBounds().toFloat();
    if (s.getTextBoxPosition() == juce::Slider::TextBoxBelow) r.removeFromBottom ((float) s.getTextBoxHeight());
    r = r.reduced (knobReduction (s));
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    return r.withSizeKeepingCentre (d, d);
}

// display âmbar de vidro escuro
static void glowText (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, float size, float alpha = 1.0f)
{
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r.expanded (3), 4.0f);
    juce::ColourGradient gl (colours::glass.brighter (0.15f), r.getX(), r.getY(), colours::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r, 3.0f);
    g.setFont (stencil (size, 0.1f));
    g.setColour (colours::amber.withAlpha (0.18f * alpha));
    for (auto o : { juce::Point<float> (-1, 0), { 1, 0 }, { 0, -1 }, { 0, 1 } })
        g.drawText (t, r.translated (o.x, o.y), juce::Justification::centred);
    g.setColour (colours::amber.withAlpha (alpha));
    g.drawText (t, r, juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f), 3.0f);
}

//==============================================================================
DubLookAndFeel::DubLookAndFeel()
{
    setColour (juce::Label::textColourId, colours::ink);
    setColour (juce::Slider::textBoxTextColourId, colours::inkSoft);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void DubLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float startAngle, float endAngle, juce::Slider& s)
{
    const bool isBig = s.getName() == "root";
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (knobReduction (s));
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    auto circle = [&] (float r) { return juce::Rectangle<float> (r * 2, r * 2).withCentre (c); };

    for (int i = 4; i >= 1; --i)                           // sombra projetada no painel
    {
        g.setColour (juce::Colours::black.withAlpha (0.07f));
        g.fillEllipse (circle (radius + (float) i).translated (1.5f, 3.0f));
    }

    juce::ColourGradient skirt (juce::Colour (0xffd8d6d0), c.x - radius, c.y - radius,   // saia de alumínio
                                juce::Colour (0xff6f6c66), c.x + radius, c.y + radius, false);
    skirt.addColour (0.5, juce::Colour (0xffa9a69f));
    g.setGradientFill (skirt);
    g.fillEllipse (circle (radius));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawEllipse (circle (radius), 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.10f));  // usinagem
    for (int i = 0; i < 60; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 60.0f;
        g.drawLine (c.x + std::sin (a) * radius * 0.82f, c.y - std::cos (a) * radius * 0.82f,
                    c.x + std::sin (a) * radius * 0.98f, c.y - std::cos (a) * radius * 0.98f, 0.6f);
    }

    const float capR = radius * (isBig ? 0.80f : 0.74f);  // corpo de baquelite
    juce::ColourGradient body (juce::Colour (0xff3d3833), c.x - capR, c.y - capR,
                               juce::Colour (0xff070605), c.x + capR * 0.6f, c.y + capR, false);
    g.setGradientFill (body);
    g.fillEllipse (circle (capR));

    if (isBig)                                             // serrilhado
    {
        g.setColour (juce::Colours::black);
        for (int i = 0; i < 40; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 40.0f;
            g.drawLine (c.x + std::sin (a) * capR * 0.90f, c.y - std::cos (a) * capR * 0.90f,
                        c.x + std::sin (a) * capR,          c.y - std::cos (a) * capR, 2.2f);
        }
    }

    juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.16f), c.x, c.y - capR,
                                juce::Colours::transparentWhite, c.x, c.y + capR * 0.2f, false);
    g.setGradientFill (gloss);
    g.fillEllipse (circle (capR * 0.86f).translated (0, -capR * 0.06f));

    if (isBig)                                             // tampa central dourada
    {
        const float r = capR * 0.30f;
        juce::ColourGradient cap (colours::jamGold.brighter (0.25f), c.x - r, c.y - r, colours::jamGold.darker (0.55f), c.x + r, c.y + r, false);
        g.setGradientFill (cap);
        g.fillEllipse (circle (r));
    }

    auto rot = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
    juce::Path line;
    const float lw = isBig ? 4.0f : 2.6f;
    line.addRoundedRectangle (-lw * 0.5f, -capR * 0.95f, lw, capR * (isBig ? 0.5f : 0.62f), lw * 0.5f);
    g.setColour (colours::cream);
    g.fillPath (line, rot);
    juce::Path notch;
    notch.addRectangle (-1.2f, -radius * 0.99f, 2.4f, radius - capR + 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillPath (notch, rot);
}

// botão com tampa escura e lâmpada piloto âmbar
void DubLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.0f);
    const bool on = b.getToggleState();

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r.translated (0, 2), 3.0f);
    juce::ColourGradient capG (juce::Colour (0xff3a3632).brighter (over ? 0.08f : 0.0f), r.getX(), r.getY(),
                               juce::Colour (0xff141210), r.getX(), r.getBottom(), false);
    g.setGradientFill (capG);
    g.fillRoundedRectangle (r.translated (0, down ? 1.0f : 0.0f), 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);

    const auto lamp = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ r.getX() + 14.0f, r.getCentreY() });
    if (on)
    {
        g.setColour (colours::amber.withAlpha (0.30f));
        g.fillEllipse (lamp.expanded (5.0f));
        juce::ColourGradient lit (juce::Colour (0xffffe3a0), lamp.getCentreX(), lamp.getCentreY(),
                                  colours::amber.darker (0.2f), lamp.getRight(), lamp.getBottom(), true);
        g.setGradientFill (lit);
    }
    else
        g.setColour (juce::Colour (0xff4a3a22));
    g.fillEllipse (lamp);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (lamp, 1.0f);

    g.setColour (colours::cream.withAlpha (on ? 1.0f : 0.65f));
    g.setFont (stencil (11.0f, 0.08f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (22), juce::Justification::centred);
}

// EXPORTAR: tampa verde grande de botão de pedal (igual ao THROW do Big Knob)
void DubLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (r, 6.0f);                 // moldura
    auto cap = r.reduced (4.0f).translated (0, down ? 1.5f : 0.0f);
    juce::ColourGradient capG ((down ? colours::jamGreen.brighter (0.4f) : colours::jamGreen).brighter (over ? 0.1f : 0.0f),
                               cap.getX(), cap.getY(), colours::jamGreen.darker (0.75f), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (capG);
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.10f : 0.22f));
    g.fillRoundedRectangle (cap.withHeight (cap.getHeight() * 0.38f).reduced (4, 2), 3.0f);
    if (down)
    {
        g.setColour (colours::jamGold.withAlpha (0.45f));
        g.drawRoundedRectangle (r.expanded (2), 8.0f, 3.0f);
    }
}

void DubLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setColour (colours::jamGold);
    g.setFont (stencil (15.0f, 0.18f));
    g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, down ? 2 : 1), juce::Justification::centred);
}

//==============================================================================
static void pushButtonCap (juce::Graphics& g, juce::Rectangle<float> r, bool on, bool over, const juce::String& text)
{
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (r.translated (0, 2.5f), 3.0f);
    auto cap = r.translated (0, on ? 2.0f : 0.0f);
    juce::ColourGradient cg (juce::Colour (on ? 0xff2c2925 : 0xff3a3632).brighter (over && ! on ? 0.08f : 0.0f), cap.getX(), cap.getY(),
                             juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.04f : 0.12f));
    g.drawHorizontalLine ((int) cap.getY() + 1, cap.getX() + 3, cap.getRight() - 3);

    const auto lamp = juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ cap.getX() + 17.0f, cap.getCentreY() });
    if (on)
    {
        g.setColour (colours::amber.withAlpha (0.30f));
        g.fillEllipse (lamp.expanded (5.0f));
        juce::ColourGradient lit (juce::Colour (0xffffe3a0), lamp.getCentreX(), lamp.getCentreY(),
                                  colours::amber.darker (0.2f), lamp.getRight(), lamp.getBottom(), true);
        g.setGradientFill (lit);
    }
    else
        g.setColour (juce::Colour (0xff4a3a22));
    g.fillEllipse (lamp);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (lamp, 1.0f);

    g.setColour (on ? colours::cream : colours::cream.withAlpha (0.5f));
    g.setFont (stencil (12.5f, 0.16f));
    g.drawText (text, cap.withTrimmedLeft (26.0f), juce::Justification::centred);
}

void MidiDragZone::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r, 5.0f);
    juce::ColourGradient gl (colours::glass.brighter (isMouseOver() ? 0.30f : 0.15f), r.getX(), r.getY(),
                             colours::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r.reduced (3.0f), 4.0f);

    juce::Path outline, dashed;
    outline.addRoundedRectangle (r.reduced (6.0f), 3.0f);
    const float dashes[] = { 5.0f, 4.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, outline, dashes, 2);
    g.setColour (colours::amber.withAlpha (isMouseOver() ? 0.9f : 0.55f));
    g.fillPath (dashed);

    g.setColour (colours::amber);
    g.setFont (stencil (13.0f, 0.16f));
    g.drawText ("ARRASTE O MIDI", r.withTrimmedBottom (r.getHeight() * 0.42f).withTrimmedTop (4), juce::Justification::centredBottom);
    g.setColour (colours::amber.withAlpha (0.6f));
    g.setFont (stencil (9.5f, 0.2f));
    g.drawText ("PARA A TIMELINE DA DAW", r.withTrimmedTop (r.getHeight() * 0.58f), juce::Justification::centredTop);
}

//==============================================================================
void DubDrumsPanel::renderBackground()
{
    if (getWidth() <= 0) return;
    const float scale = 2.0f;   // 2x para telas retina
    background = juce::Image (juce::Image::ARGB, (int) (getWidth() * scale), (int) (getHeight() * scale), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    juce::Random rng (1975);

    // --- laterais de madeira
    for (auto wood : { woodL, woodR })
    {
        const float wx = (float) wood.getX(), ww = (float) wood.getWidth();
        juce::ColourGradient wg (colours::woodLight.darker (0.1f), wx, 0, colours::woodDark, wx + ww, 0, false);
        wg.addColour (0.35, colours::woodLight.darker (0.25f));
        wg.addColour (0.7,  colours::woodLight.darker (0.45f));
        g.setGradientFill (wg);
        g.fillRect (wood);
        for (int i = 0; i < 40; ++i)
        {
            juce::Path grain;
            const float xx = wx + rng.nextFloat() * ww, ph = rng.nextFloat() * 6.0f, amp = 0.8f + rng.nextFloat() * 2.2f;
            grain.startNewSubPath (xx, 0);
            for (int yy = 0; yy <= getHeight(); yy += 14)
                grain.lineTo (xx + std::sin ((float) yy * 0.011f + ph) * amp, (float) yy);
            g.setColour ((i % 3 ? juce::Colours::black : colours::woodLight.brighter (0.3f)).withAlpha (0.10f + rng.nextFloat() * 0.16f));
            g.strokePath (grain, juce::PathStrokeType (0.5f + rng.nextFloat() * 1.5f));
        }
        {
            const juce::Point<float> kc (wx + ww * (0.35f + rng.nextFloat() * 0.3f), (float) getHeight() * (0.25f + rng.nextFloat() * 0.5f));
            for (int k = 5; k >= 1; --k)
            {
                g.setColour (colours::woodDark.darker (0.4f).withAlpha (0.18f));
                g.drawEllipse (juce::Rectangle<float> ((float) k * 2.6f, (float) k * 5.5f).withCentre (kc), 0.9f);
            }
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (3.0f, 6.0f).withCentre (kc));
        }
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.10f), wx, 0, juce::Colours::transparentWhite, wx + 4.0f, 0, false);
        g.setGradientFill (edge);
        g.fillRect (wood.withWidth (4));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRect (wood, 1);
    }

    // --- painel de metal pintado, amarelado pelo tempo
    juce::ColourGradient pg (colours::plate.brighter (0.03f), (float) plate.getX(), (float) plate.getY(),
                             colours::plateDark.darker (0.06f), (float) plate.getRight(), (float) plate.getBottom(), false);
    pg.addColour (0.5, colours::plate.interpolatedWith (juce::Colour (0xffd9c48f), 0.35f));
    g.setGradientFill (pg);
    g.fillRect (plate);

    auto randIn = [&] (juce::Rectangle<int> r)
    { return juce::Point<float> ((float) r.getX() + rng.nextFloat() * (float) r.getWidth(), (float) r.getY() + rng.nextFloat() * (float) r.getHeight()); };

    for (int i = 0; i < 22; ++i)                       // amarelado irregular
    {
        const float rr = 60.0f + rng.nextFloat() * 220.0f;
        const auto pc = randIn (plate);
        juce::ColourGradient y (juce::Colour (0xffc9a64e).withAlpha (0.07f + rng.nextFloat() * 0.05f), pc.x, pc.y,
                                juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
        g.setGradientFill (y);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 1.4f).withCentre (pc));
    }
    for (int i = 0; i < 60000; ++i)                    // granulado
    {
        const auto pt = randIn (plate);
        g.setColour ((rng.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.03f + rng.nextFloat() * 0.07f));
        g.fillRect (pt.x, pt.y, 1.0f, 1.0f);
    }
    for (int i = 0; i < 30; ++i)                       // manchas de uso
    {
        const float rr = 10.0f + rng.nextFloat() * 90.0f;
        const auto pc = randIn (plate);
        juce::ColourGradient stain (juce::Colour (0xff7a5a2c).withAlpha (0.05f + rng.nextFloat() * 0.07f), pc.x, pc.y,
                                    juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
        g.setGradientFill (stain);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (pc));
    }
    for (int i = 0; i < 3; ++i)                        // anéis de copo
    {
        const float rr = 22.0f + rng.nextFloat() * 14.0f;
        const auto pc = randIn (plate.reduced (60));
        g.setColour (juce::Colour (0xff6b4d22).withAlpha (0.10f));
        g.drawEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (pc), 1.6f);
    }
    for (int i = 0; i < 140; ++i)                      // riscos finos
    {
        const auto p0 = randIn (plate);
        const float ang = rng.nextFloat() * 3.14159f, len = 4.0f + rng.nextFloat() * 38.0f;
        g.setColour ((rng.nextFloat() < 0.6f ? juce::Colours::white : juce::Colour (0xff4a3a24)).withAlpha (0.10f + rng.nextFloat() * 0.18f));
        g.drawLine (p0.x, p0.y, p0.x + std::cos (ang) * len, p0.y + std::sin (ang) * len, 0.5f);
    }
    {                                                   // sujeira nas bordas
        const auto pf = plate.toFloat();
        const float d = 34.0f;
        const juce::Colour grime (0xff3d2b14);
        auto band = [&] (juce::Rectangle<float> r, juce::Point<float> from, juce::Point<float> to)
        {
            juce::ColourGradient eg (grime.withAlpha (0.30f), from.x, from.y, grime.withAlpha (0.0f), to.x, to.y, false);
            g.setGradientFill (eg);
            g.fillRect (r);
        };
        band (pf.withWidth (d),                { pf.getX(), 0 },      { pf.getX() + d, 0 });
        band (pf.withLeft (pf.getRight() - d), { pf.getRight(), 0 },  { pf.getRight() - d, 0 });
        band (pf.withHeight (d),               { 0, pf.getY() },      { 0, pf.getY() + d });
        band (pf.withTop (pf.getBottom() - d), { 0, pf.getBottom() }, { 0, pf.getBottom() - d });
    }
    juce::ColourGradient vig (juce::Colours::transparentBlack, (float) plate.getCentreX(), (float) plate.getCentreY(),
                              juce::Colours::black.withAlpha (0.28f), (float) plate.getX(), (float) plate.getY(), true);
    g.setGradientFill (vig);
    g.fillRect (plate);

    auto screw = [&] (float sx, float sy)
    {
        const auto r = juce::Rectangle<float> (11, 11).withCentre ({ sx, sy });
        juce::ColourGradient sg (juce::Colour (0xffe0ded8), r.getX(), r.getY(), juce::Colour (0xff5e5b55), r.getRight(), r.getBottom(), false);
        g.setGradientFill (sg);
        g.fillEllipse (r);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (r, 0.8f);
        const float a = rng.nextFloat() * 3.14f;
        g.drawLine (sx - std::cos (a) * 4.0f, sy - std::sin (a) * 4.0f, sx + std::cos (a) * 4.0f, sy + std::sin (a) * 4.0f, 1.4f);
    };
    const float m = 12.0f;
    screw ((float) plate.getX() + m, (float) plate.getY() + m);
    screw ((float) plate.getRight() - m, (float) plate.getY() + m);
    screw ((float) plate.getX() + m, (float) plate.getBottom() - m);
    screw ((float) plate.getRight() - m, (float) plate.getBottom() - m);

    // --- cabeçalho
    g.setColour (colours::ink);
    g.setFont (stencil (30.0f, 0.22f));
    const juce::String title ("DUBMASTOR DUB DRUMS");
    g.drawText (title, plate.getX() + 32, 20, 760, 34, juce::Justification::left);
    const float titleRight = (float) plate.getX() + 32.0f + juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title);

    {   // placa de latão "DUB STATION"
        g.setFont (stencil (15.0f, 0.34f));
        const juce::String t ("DUB STATION " DD_VERSION_SHORT);
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t);
        auto pl = juce::Rectangle<float> (titleRight + 26.0f, 21.0f, tw + 64.0f, 32.0f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (pl.translated (1.5f, 3.0f), 3.0f);
        juce::ColourGradient brass (juce::Colour (0xffe3cd85), pl.getX(), pl.getY(), juce::Colour (0xff8a6d2c), pl.getX(), pl.getBottom(), false);
        brass.addColour (0.45, juce::Colour (0xffc7a95a));
        brass.addColour (0.55, juce::Colour (0xffb39446));
        g.setGradientFill (brass);
        g.fillRoundedRectangle (pl, 3.0f);
        for (int i = 0; i < 70; ++i)
        {
            const float yy = pl.getY() + 1.0f + rng.nextFloat() * (pl.getHeight() - 2.0f);
            const float x0 = pl.getX() + rng.nextFloat() * pl.getWidth() * 0.5f;
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colour (0xff5a4316)).withAlpha (0.06f + rng.nextFloat() * 0.08f));
            g.drawHorizontalLine ((int) yy, x0, juce::jmin (pl.getRight() - 1.0f, x0 + 20.0f + rng.nextFloat() * pl.getWidth() * 0.6f));
        }
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawLine (pl.getX() + 2, pl.getY() + 1, pl.getRight() - 2, pl.getY() + 1, 1.0f);
        g.setColour (juce::Colour (0xff3d2c0c).withAlpha (0.7f));
        g.drawLine (pl.getX() + 2, pl.getBottom() - 1, pl.getRight() - 2, pl.getBottom() - 1, 1.2f);
        g.setColour (juce::Colour (0xff3d2c0c).withAlpha (0.55f));
        g.drawRoundedRectangle (pl, 3.0f, 0.8f);
        auto inner = pl.reduced (24.0f, 5.0f);
        g.setColour (juce::Colour (0xff4a3712).withAlpha (0.55f));
        g.drawRoundedRectangle (inner, 2.0f, 0.8f);
        for (float rx : { pl.getX() + 12.0f, pl.getRight() - 12.0f })
        {
            const auto rv = juce::Rectangle<float> (9, 9).withCentre ({ rx, pl.getCentreY() });
            juce::ColourGradient rg (juce::Colour (0xfff0dfa6), rv.getX(), rv.getY(), juce::Colour (0xff6b5220), rv.getRight(), rv.getBottom(), false);
            g.setGradientFill (rg);
            g.fillEllipse (rv);
        }
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawText (t, pl.translated (0.8f, 1.0f), juce::Justification::centred);
        g.setColour (juce::Colour (0xff2e2108));
        g.drawText (t, pl, juce::Justification::centred);
    }
    g.setColour (colours::inkSoft);
    g.setFont (stencil (10.5f, 0.25f));
    g.drawText ("RIDDIM DRUM GENERATOR", plate.getX() + 34, 52, 500, 14, juce::Justification::left);

    {   // faixas da bandeira da Jamaica, pintadas à mão e envelhecidas
        const float sx = (float) plate.getRight() - 330.0f, sw = 290.0f, sh = 7.0f;
        const juce::Colour faded[] = {
            colours::jamGreen.interpolatedWith (juce::Colour (0xff6f7a4a), 0.35f),
            colours::jamGold.interpolatedWith (juce::Colour (0xffb08a3a), 0.40f),
            juce::Colour (0xff2b2621) };
        juce::Path all;
        for (int k = 0; k < 3; ++k)
        {
            const float y = 30.0f + 9.0f * (float) k;
            juce::Path band;
            band.startNewSubPath (sx, y + rng.nextFloat() * 0.8f);
            for (float x = sx; x <= sx + sw; x += 6.0f) band.lineTo (x, y + (rng.nextFloat() - 0.3f) * 0.9f);
            for (float x = sx + sw; x >= sx; x -= 6.0f) band.lineTo (x, y + sh + (rng.nextFloat() - 0.7f) * 0.9f);
            band.closeSubPath();
            all.addPath (band);
            juce::ColourGradient fade (faded[k].withAlpha (0.92f), sx, y, faded[k].withAlpha (0.70f), sx + sw, y, false);
            fade.addColour (0.3, faded[k].brighter (0.12f).withAlpha (0.85f));
            g.setGradientFill (fade);
            g.fillPath (band);
        }
        g.saveState();
        g.reduceClipRegion (all);
        const auto area = all.getBounds();
        for (int i = 0; i < 12; ++i)
        {
            const float rr = 6.0f + rng.nextFloat() * 26.0f;
            const juce::Point<float> pc (area.getX() + rng.nextFloat() * area.getWidth(), area.getY() + rng.nextFloat() * area.getHeight());
            const bool light = rng.nextBool();
            juce::ColourGradient st ((light ? colours::plate : juce::Colour (0xff4a3518)).withAlpha (light ? 0.30f : 0.25f), pc.x, pc.y,
                                     juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
            g.setGradientFill (st);
            g.fillEllipse (juce::Rectangle<float> (rr * 2, rr).withCentre (pc));
        }
        g.setColour (colours::plate.withAlpha (0.14f));
        g.fillRect (area);
        for (int i = 0; i < 900; ++i)
        {
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colour (0xff3d2b14)).withAlpha (0.08f + rng.nextFloat() * 0.10f));
            g.fillRect (area.getX() + rng.nextFloat() * area.getWidth(), area.getY() + rng.nextFloat() * area.getHeight(), 1.0f, 1.0f);
        }
        g.restoreState();
    }

    // --- seções: moldura gravada + plaqueta preta com o nome
    auto section = [&] (juce::Rectangle<int> r, const juce::String& t)
    {
        auto rf = r.toFloat();
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (rf.translated (1, 1), 4.0f, 1.0f);
        g.setColour (colours::ink.withAlpha (0.55f));
        g.drawRoundedRectangle (rf, 4.0f, 1.2f);
        g.setFont (stencil (11.5f, 0.22f));
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 22.0f;
        auto tag = juce::Rectangle<float> (rf.getX() + 12, rf.getY() - 9, tw, 18);
        g.setColour (colours::labelBg);
        g.fillRoundedRectangle (tag, 2.0f);
        g.setColour (colours::cream);
        g.drawText (t, tag, juce::Justification::centred);
    };

    section (styleMod,  "ESTILO");
    section (loopMod,   "LOOP");
    section (hatMod,    "HI-HAT");
    section (dynMod,    "DINAMICA");
    section (randPanel, "ALEATORIEDADE");
    section (soundPanel, "SOM");
    section (midiPanel, "MIDI");
    section (grid.expanded (10, 14).withTrimmedBottom (2), "PADRAO");

    // rótulos gravados acima dos controles
    g.setColour (colours::inkSoft);
    g.setFont (stencil (9.0f, 0.3f));
    for (auto [c, t] : { std::pair<juce::Component*, const char*> { &styleButtons, "ESCOLHA O ESTILO" },
                         { &bars, "COMPASSOS" }, { &seed, "SEMENTE" }, { &hatButtons, "FIGURA" } })
        g.drawText (t, c->getBounds().translated (0, -15).withHeight (12), juce::Justification::centred);
    g.drawText ("BUMBO NOS TEMPOS", kickDots.translated (0, -15).withHeight (12), juce::Justification::centred);
    g.drawText ("HUMANIZA FORCA E TEMPO", randPanel.reduced (14, 0).removeFromBottom (18).translated (0, -4),
                juce::Justification::centredRight);

    // escalas
    drawKnobScale (g, swing.slider, 5, { "RETO", "LEVE", "MEDIO", "PESADO", "SHUF." });
    drawKnobScale (g, dynamics.slider, 6, { "SUAVE", "MEDIA", "FORTE", "ACENT.", "CRESC.", "RESPIRA" });
    for (auto* k : { &rndKick, &rndSnare, &rndHat, &rndPerc })
        drawKnobScale (g, k->slider, 11);
    drawKnobScale (g, channel.slider, 16);
    drawKnobScale (g, drummer.slider, 3, { "SCOTT", "SANTA", "CARLTON" });
    drawKnobScale (g, bpm.slider, 11);
    drawKnobScale (g, volume.slider, 11);
}

//==============================================================================
static const char* kStyleNames[] = { "ONE DROP", "ROCKERS", "STEPPA" };
static const char* kHatNames[]   = { "COLCHEIAS", "SEMICOLCHEIAS", "CONTRATEMPO", "TERCINAS" };
static const char* kDynNames[]   = { "SUAVE", "MEDIA", "FORTE", "ACENTUADA", "CRESCENDO", "RESPIRANDO" };
static const char* kDrummerNames[] = { "STYLE SCOTT", "SANTA DAVIS", "CARLTON BARRETT" };

// cor de cada compasso na grade (intro + 4 compassos), nas cores do reggae
static juce::Colour barColour (int bar)
{
    static const juce::Colour c[] = { juce::Colour (0xffffa928),   // compasso 1: âmbar
                                      juce::Colour (0xff3fc46a),   // compasso 2: verde
                                      juce::Colour (0xffffe066),   // compasso 3: dourado
                                      juce::Colour (0xffff5b3a) }; // compasso 4: vermelho
    if (bar < 0) return juce::Colour (0xffb9d8ff);                // intro: azul claro
    return c[bar % 4];
}

juce::Rectangle<float> ChoiceButtons::cell (int i) const
{
    auto inner = getLocalBounds().toFloat().reduced (4.0f);
    const int n = names.size(), rows = (n + cols - 1) / cols;
    const float cw = inner.getWidth() / (float) cols, ch = inner.getHeight() / (float) rows;
    return juce::Rectangle<float> (inner.getX() + (float) (i % cols) * cw, inner.getY() + (float) (i / cols) * ch, cw, ch)
               .reduced (1.5f, 2.0f);
}

void ChoiceButtons::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1714));                        // moldura metálica comum
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);
    const auto mouse = getMouseXYRelative().toFloat();
    for (int i = 0; i < names.size(); ++i)
    {
        auto c = cell (i);
        pushButtonCap (g, c.withTrimmedBottom (2.5f), index() == i, isMouseOver() && c.contains (mouse), names[i]);
    }
}

void ChoiceButtons::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < names.size(); ++i)
        if (cell (i).contains (e.position)) { select (i); return; }
}

//==============================================================================
void BarsWindow::mouseDown (const juce::MouseEvent& e)
{
    const int dir = e.position.x < (float) getWidth() * 0.5f ? -1 : 1;
    select (juce::jlimit (0, 3, index() + dir));
}

void BarsWindow::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const int i = juce::jlimit (0, 3, index());

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (b.translated (0, 2.0f), 5.0f);
    juce::ColourGradient frame (juce::Colour (0xff3a3632), b.getX(), b.getY(), juce::Colour (0xff141210), b.getX(), b.getBottom(), false);
    g.setGradientFill (frame);
    g.fillRoundedRectangle (b, 5.0f);

    auto arrow = [&] (float cx, int dir)
    {
        const bool lit = side == dir;
        const bool possible = (dir < 0 && i > 0) || (dir > 0 && i < 3);
        juce::Path p;
        const float cy = b.getCentreY(), s = 6.5f;
        if (dir < 0) p.addTriangle (cx + s * 0.6f, cy - s, cx + s * 0.6f, cy + s, cx - s * 0.8f, cy);
        else         p.addTriangle (cx - s * 0.6f, cy - s, cx - s * 0.6f, cy + s, cx + s * 0.8f, cy);
        g.setColour (! possible ? colours::cream.withAlpha (0.15f) : lit ? colours::amber : colours::cream.withAlpha (0.6f));
        g.fillPath (p);
    };
    arrow (b.getX() + 16.0f, -1);
    arrow (b.getRight() - 16.0f, 1);

    auto win = b.reduced (32.0f, 7.0f);
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (win.expanded (2.0f), 3.0f);
    juce::ColourGradient drum (juce::Colour (0xff8f8264), win.getX(), win.getY(), juce::Colour (0xff8f8264), win.getX(), win.getBottom(), false);
    drum.addColour (0.25, juce::Colour (0xffefe5cc));
    drum.addColour (0.55, juce::Colour (0xfff6ecd6));
    drum.addColour (0.85, juce::Colour (0xffcbbd98));
    g.setGradientFill (drum);
    g.fillRoundedRectangle (win, 2.0f);

    const juce::String num (i + 1), unit = i == 0 ? "COMPASSO" : "COMPASSOS";
    g.setColour (colours::ink);
    g.setFont (stencil (21.0f, 0.04f));
    const float nw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), num);
    g.setFont (stencil (10.0f, 0.22f));
    const float uw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), unit);
    auto line = win.withSizeKeepingCentre (nw + 8.0f + uw, win.getHeight());
    g.setFont (stencil (21.0f, 0.04f));
    g.drawText (num, line.removeFromLeft (nw), juce::Justification::centredLeft);
    line.removeFromLeft (8.0f);
    g.setFont (stencil (10.0f, 0.22f));
    g.drawText (unit, line, juce::Justification::centredLeft);

    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillRoundedRectangle (win.withHeight (win.getHeight() * 0.3f).reduced (2.0f, 1.0f), 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (win, 2.0f, 1.0f);
}

//==============================================================================
void SeedWindow::mouseDrag (const juce::MouseEvent& e)
{
    setDuring ((float) juce::jlimit (1, 9999, dragStart - e.getDistanceFromDragStartY() / 4));
}

void SeedWindow::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const int dir = w.deltaY > 0 ? 1 : (w.deltaY < 0 ? -1 : 0);
    if (dir != 0) select (juce::jlimit (1, 9999, index() + dir));
}

void SeedWindow::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    glowText (g, r, juce::String (index()).paddedLeft ('0', 4), 20.0f, isMouseOver() ? 1.0f : 0.9f);
}

void PushButton::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (b, 5.0f);
    pushButtonCap (g, b.reduced (4.0f).withTrimmedBottom (2.5f), down, isMouseOver(), text);
}

void MidiDragZone::mouseDrag (const juce::MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (proc.suggestedFileName());
    proc.writeMidiFile (file);
    if (file.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (file.getFullPathName()), false, this);
}

//==============================================================================
DubDrumsPanel::DubDrumsPanel (DubDrumsProcessor& p)
    : proc (p),
      styleButtons (*p.apvts.getParameter ("style"), { "ONE DROP", "ROCKERS", "STEPPA" }, 1),
      hatButtons (*p.apvts.getParameter ("hatPattern"), { "1/8", "1/16", "CONTRA", "TERCINA" }, 2),
      bars (*p.apvts.getParameter ("bars")),
      seed (*p.apvts.getParameter ("seed")),
      noteMap (p),
      dragZone (p)
{
    setLookAndFeel (&lnf);

    for (juce::Component* c : { (juce::Component*) &styleButtons, (juce::Component*) &hatButtons,
                                (juce::Component*) &bars, (juce::Component*) &seed, (juce::Component*) &variation })
        addAndMakeVisible (c);
    variation.onClick = [this] { proc.newVariation(); };

    setupToggle (fill, "fill", "VIRADA NO FIM");
    setupToggle (intro, "intro", "VIRADA DE INTRO");
    setupToggle (swingAll, "swingAll", "SWING EM TUDO");
    setupToggle (preview, "preview", "PREVIEW");
    setupToggle (sound, "sound", "SOM INTERNO");
    setupToggle (colorMidi, "colorMidi", "MIDI COLORIDO");

    setupKnob (swing, "swing", "SWING");
    setupKnob (drummer, "drummer", "BATERISTA");
    setupKnob (dynamics, "dynamics", {});
    drummer.slider.setRotaryParameters (juce::MathConstants<float>::twoPi - juce::MathConstants<float>::pi / 3.0f,
                                        juce::MathConstants<float>::twoPi + juce::MathConstants<float>::pi / 3.0f, true);
    for (auto* k : { &swing, &dynamics, &drummer })
        k->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    dynamics.label.setVisible (false);

    setupKnob (bpm, "bpm", "BPM");
    setupKnob (volume, "volume", "VOLUME");
    volume.slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
    volume.slider.updateText();
    bpm.slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)); };
    bpm.slider.updateText();

    setupKnob (rndKick,  "rndKick",  "BUMBO");
    setupKnob (rndSnare, "rndSnare", "CAIXA");
    setupKnob (rndHat,   "rndHat",   "HI-HAT");
    setupKnob (rndPerc,  "rndPerc",  "TONS");
    for (auto* k : { &rndKick, &rndSnare, &rndHat, &rndPerc })
    {
        k->slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + "%"; };
        k->slider.updateText();
    }
    setupKnob (channel, "channel", "CANAL");

    exportBtn.onClick = [this] { exportMidi(); };
    addAndMakeVisible (exportBtn);
    addAndMakeVisible (dragZone);

    notesButton.onClick = [this] { noteMap.setVisible (true); noteMap.toFront (false); };
    addAndMakeVisible (notesButton);
    addChildComponent (noteMap);
    noteMap.onClose = [this] { noteMap.setVisible (false); };

    setSize (1280, 868);
    timerCallback();
    startTimerHz (30);
}

DubDrumsPanel::~DubDrumsPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DubDrumsPanel::setupKnob (Knob& k, const juce::String& id, const juce::String& name)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (kStartAngle, kEndAngle, true);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 90, 16);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxTextColourId, colours::inkSoft);
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<SliderAttach> (proc.apvts, id, k.slider);
    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (stencil (11.0f, 0.12f));
    k.label.setColour (juce::Label::textColourId, colours::ink);
    addAndMakeVisible (k.label);
}

void DubDrumsPanel::setupToggle (Toggle& t, const juce::String& id, const juce::String& text)
{
    t.button.setButtonText (text);
    addAndMakeVisible (t.button);
    t.att = std::make_unique<ButtonAttach> (proc.apvts, id, t.button);
}

void DubDrumsPanel::placeKnob (Knob& k, juce::Rectangle<int> r)
{
    k.label.setBounds (r.removeFromTop (15));
    k.slider.setBounds (r);
}

int DubDrumsPanel::choice (const char* id) const
{
    return juce::roundToInt (proc.apvts.getRawParameterValue (id)->load());
}

void DubDrumsPanel::exportMidi()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Dub Drums");
    dir.createDirectory();
    chooser = std::make_unique<juce::FileChooser> ("Salvar bateria em MIDI", dir.getChildFile (proc.suggestedFileName()), "*.mid");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              if (! f.hasFileExtension ("mid")) f = f.withFileExtension ("mid");
                              proc.writeMidiFile (f);
                              if (! f.existsAsFile())
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                                          "Dub Drums", "Nao foi possivel salvar o MIDI.");
                          });
}

void DubDrumsPanel::timerCallback()
{
    const int v = proc.getPatternVersion();
    if (v != lastVersion)
    {
        lastVersion = v;
        pattern = proc.getPatternCopy();
        repaint();
    }
    else if (proc.isHostPlaying() || isMouseOver (true))
    {
        repaint (grid.expanded (4));
    }
}

//==============================================================================
void DubDrumsPanel::drawKnobScale (juce::Graphics& g, juce::Slider& s, int ticks, const juce::StringArray& labels)
{
    const auto rp = s.getRotaryParameters();
    const float a0 = rp.startAngleRadians, a1 = rp.endAngleRadians;
    const auto circ = knobCircle (s);
    const auto c = circ.getCentre();
    const float r = circ.getWidth() * 0.5f;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / (float) juce::jmax (1, ticks - 1);
        const float r1 = r + 4.0f, r2 = r + (labels.isEmpty() ? 9.0f : 11.0f);
        g.setColour (colours::ink.withAlpha (0.85f));
        g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1,
                    c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, labels.isEmpty() ? 1.1f : 1.6f);
        if (i < labels.size())
        {
            g.setFont (stencil (9.5f, 0.02f));
            const float lr = r + 24.0f;
            g.drawText (labels[i], juce::Rectangle<float> (64, 14).withCentre ({ c.x + std::sin (a) * lr, c.y - std::cos (a) * lr }),
                        juce::Justification::centred);
        }
    }
}

void DubDrumsPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::woodDark);
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    const int st = juce::jlimit (0, 2, choice ("style"));
    glowText (g, styleDisplay.toFloat(), kStyleNames[st], 19.0f);
    glowText (g, hatDisplay.toFloat(), kDrummerNames[juce::jlimit (0, 2, choice ("drummer"))], 17.0f);
    juce::ignoreUnused (kHatNames);
    glowText (g, dynDisplay.toFloat(), kDynNames[juce::jlimit (0, 5, choice ("dynamics"))], 17.0f);

    // onde cai o bumbo: quatro tempos gravados no painel, preenchidos onde o bumbo toca
    static const bool kick[3][4] = { { false, false, true, false }, { true, false, true, false }, { true, true, true, true } };
    const auto kd = kickDots.toFloat();
    const float step = kd.getWidth() / 4.0f;
    g.setColour (colours::ink.withAlpha (0.35f));
    g.drawLine (kd.getX() + step * 0.5f, kd.getY() + 10.0f, kd.getRight() - step * 0.5f, kd.getY() + 10.0f, 1.0f);
    for (int b = 0; b < 4; ++b)
    {
        const juce::Point<float> c (kd.getX() + step * ((float) b + 0.5f), kd.getY() + 10.0f);
        if (kick[st][b])
        {
            g.setColour (colours::amber.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (20, 20).withCentre (c));
            g.setColour (colours::labelBg);
            g.fillEllipse (juce::Rectangle<float> (14, 14).withCentre (c));
            g.setColour (colours::amber);
            g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre (c));
        }
        else
        {
            g.setColour (colours::plate);
            g.fillEllipse (juce::Rectangle<float> (10, 10).withCentre (c));
            g.setColour (colours::ink.withAlpha (0.6f));
            g.drawEllipse (juce::Rectangle<float> (10, 10).withCentre (c), 1.2f);
        }
        g.setColour (colours::inkSoft);
        g.setFont (stencil (10.0f, 0.1f));
        g.drawText (juce::String (b + 1), juce::Rectangle<float> (20, 12).withCentre ({ c.x, c.y + 18.0f }), juce::Justification::centred);
    }

    drawPattern (g, grid.toFloat());
}

void DubDrumsPanel::drawPattern (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r.expanded (3), 4.0f);
    juce::ColourGradient gl (colours::glass.brighter (0.15f), r.getX(), r.getY(), colours::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r, 3.0f);

    struct Row { int note; const char* name; };
    static const Row rows[] = {
        { dubdrums::gm::Crash, "PRATO" }, { dubdrums::gm::OpenHat, "HH ABERTO" }, { dubdrums::gm::ClosedHat, "HH FECHADO" },
        { dubdrums::gm::TomHigh, "TOM 1" }, { dubdrums::gm::TomMid, "TOM 2" }, { dubdrums::gm::TomLow, "SURDO" },
        { dubdrums::gm::Snare, "CAIXA" }, { dubdrums::gm::SideStick, "ARO" }, { dubdrums::gm::Kick, "BUMBO" } };
    constexpr int numRows = 9;

    auto area = r.reduced (10.0f, 8.0f);
    auto labels = area.removeFromLeft (82.0f);
    auto header = area.removeFromTop (16.0f);
    area.removeFromTop (2.0f);
    const float rowH = area.getHeight() / (float) numRows;

    // linha do tempo: introLen (se houver) + loop
    const double introLen = pattern.introLengthInBeats;
    const double total = juce::jmax (1.0, introLen + pattern.lengthInBeats);
    const int steps = juce::roundToInt (total * 4.0);
    const float stepW = area.getWidth() / (float) steps;
    auto xFor = [&] (double beat) { return area.getX() + (float) (beat / total) * area.getWidth(); };

    // faixa colorida de cada compasso, com o nome em cima
    const int numBars = juce::roundToInt (total / 4.0);
    for (int i = 0; i < numBars; ++i)
    {
        const int bar = introLen > 0.0 ? i - 1 : i;
        const auto col = barColour (bar);
        const float x0 = xFor (i * 4.0), x1 = xFor ((i + 1) * 4.0);
        g.setColour (col.withAlpha (0.05f));
        g.fillRect (juce::Rectangle<float> (x0, area.getY(), x1 - x0, area.getHeight()));
        auto tag = juce::Rectangle<float> (x0 + 2.0f, header.getY(), x1 - x0 - 4.0f, header.getHeight()).reduced (0, 2.0f);
        g.setColour (col.withAlpha (0.22f));
        g.fillRoundedRectangle (tag, 2.0f);
        g.setColour (col);
        g.setFont (stencil (9.5f, 0.2f));
        g.drawText (bar < 0 ? juce::String ("INTRO") : "COMPASSO " + juce::String (bar + 1), tag, juce::Justification::centred);
    }

    for (int i = 0; i < numRows; ++i)
    {
        g.setColour (colours::amber.withAlpha (0.55f));
        g.setFont (stencil (9.0f, 0.12f));
        g.drawText (rows[i].name, juce::Rectangle<float> (labels.getX(), area.getY() + (float) i * rowH, labels.getWidth() - 8.0f, rowH),
                    juce::Justification::centredRight);
        if (i % 2 == 0)
        {
            g.setColour (colours::amber.withAlpha (0.025f));
            g.fillRect (area.getX(), area.getY() + (float) i * rowH, area.getWidth(), rowH);
        }
    }

    for (int st = 0; st <= steps; ++st)
    {
        const bool bar = st % 16 == 0, beat = st % 4 == 0;
        g.setColour (colours::amber.withAlpha (bar ? 0.35f : beat ? 0.14f : 0.05f));
        g.drawVerticalLine ((int) (area.getX() + (float) st * stepW), area.getY(), area.getBottom());
    }

    auto drawNotes = [&] (const std::vector<dubdrums::NoteEvent>& evs, double offset)
    {
        for (const auto& e : evs)
        {
            int row = -1;
            for (int i = 0; i < numRows; ++i) if (rows[i].note == e.note) { row = i; break; }
            if (row < 0) continue;
            const auto col = barColour (e.bar);
            const float v = (float) e.velocity / 127.0f;
            const float w = juce::jmax (3.0f, stepW * 0.68f), h = rowH * (0.35f + 0.5f * v);
            const auto nr = juce::Rectangle<float> (xFor (offset + e.beat), area.getY() + (float) row * rowH + (rowH - h) * 0.5f, w, h);
            g.setColour (col.withAlpha (0.12f * v));
            g.fillRoundedRectangle (nr.expanded (2.0f), 2.5f);
            g.setColour (col.withAlpha (0.35f + 0.65f * v));
            g.fillRoundedRectangle (nr, 1.5f);
        }
    };
    drawNotes (pattern.intro, 0.0);
    drawNotes (pattern.events, introLen);

    if (proc.isHostPlaying() || proc.isPreviewing())
    {
        const double pos = proc.getPlayheadBeat();
        if (pos >= 0.0)
        {
            const float px = xFor (pos);
            g.setColour (colours::amber.withAlpha (0.3f));
            g.fillRect (px - 3.0f, r.getY() + 2.0f, 6.0f, r.getHeight() - 4.0f);
            g.setColour (juce::Colour (0xffffe3a0));
            g.fillRect (px - 1.0f, r.getY() + 2.0f, 2.0f, r.getHeight() - 4.0f);
        }
    }

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f), 3.0f);
}

void DubDrumsPanel::resized()
{
    const int W = getWidth(), H = getHeight();
    woodL = { 0, 0, 28, H };
    woodR = { W - 28, 0, 28, H };
    plate = { 28, 0, W - 56, H };
    noteMap.setBounds (getLocalBounds());

    // --- 4 módulos de cima (o do hi-hat é mais largo: figura, baterista e swing)
    const int x0 = 52, my = 92, mh = 350, narrow = 252, wide = 384;
    const int gap = (W - 104 - narrow * 3 - wide) / 3;
    styleMod = { x0,                          my, narrow, mh };
    loopMod  = { styleMod.getRight() + gap,   my, narrow, mh };
    hatMod   = { loopMod.getRight() + gap,    my, wide,   mh };
    dynMod   = { hatMod.getRight() + gap,     my, narrow, mh };

    {   // ESTILO
        auto a = styleMod.reduced (14, 0);
        a.removeFromTop (26);
        styleDisplay = a.removeFromTop (36).reduced (2, 0);
        a.removeFromTop (30);
        styleButtons.setBounds (a.removeFromTop (150));
        a.removeFromTop (32);
        kickDots = a.removeFromTop (36).reduced (24, 0);
    }
    {   // LOOP
        auto a = loopMod.reduced (14, 0);
        a.removeFromTop (36);
        bars.setBounds (a.removeFromTop (52).reduced (8, 0));
        a.removeFromTop (14);
        fill.button.setBounds (a.removeFromTop (34).reduced (22, 0));
        a.removeFromTop (8);
        intro.button.setBounds (a.removeFromTop (34).reduced (22, 0));
        a.removeFromTop (30);
        seed.setBounds (a.removeFromTop (42).reduced (46, 0));
        a.removeFromTop (12);
        variation.setBounds (a.removeFromTop (42).reduced (18, 0));
    }
    {   // HI-HAT: baterista no display, figura, chave do baterista e swing
        auto a = hatMod.reduced (14, 0);
        a.removeFromTop (16);
        auto header = a.removeFromTop (28);
        swingAll.button.setBounds (header.removeFromRight (150));
        a.removeFromTop (6);
        hatDisplay = a.removeFromTop (36).reduced (2, 0);
        a.removeFromTop (22);
        hatButtons.setBounds (a.removeFromTop (76));
        a.removeFromTop (6);
        auto knobs = a;
        auto place = [] (Knob& k, juce::Rectangle<int> r)
        {
            k.label.setBounds (r.removeFromTop (15));
            const int cy = r.getY() + 80;
            k.slider.setBounds (juce::Rectangle<int> (104, 104).withCentre ({ r.getCentreX(), cy }));
        };
        place (drummer, knobs.removeFromLeft (knobs.getWidth() / 2));
        place (swing, knobs);
    }
    {   // DINÂMICA
        auto a = dynMod.reduced (14, 0);
        a.removeFromTop (26);
        dynDisplay = a.removeFromTop (36).reduced (2, 0);
        a.removeFromTop (16);
        dynamics.slider.setBounds (a.withTrimmedBottom (10).withSizeKeepingCentre (192, 192));
    }

    // --- linha do meio: ALEATORIEDADE | SOM | MIDI
    const int by = 466, bh = 170;
    randPanel  = { 52, by, 470, bh };
    soundPanel = { randPanel.getRight() + 14, by, 300, bh };
    midiPanel  = { soundPanel.getRight() + 14, by, W - 52 - (soundPanel.getRight() + 14), bh };
    {
        auto a = randPanel.reduced (10, 0);
        a.removeFromTop (18);
        a.removeFromBottom (22);
        const int kw = a.getWidth() / 4;
        for (auto* k : { &rndKick, &rndSnare, &rndHat, &rndPerc })
            placeKnob (*k, a.removeFromLeft (kw).withTrimmedTop (4));
    }
    {
        auto a = soundPanel.reduced (12, 0);
        a.removeFromTop (24);
        auto left = a.removeFromLeft (128);
        left.removeFromTop (10);
        preview.button.setBounds (left.removeFromTop (36).reduced (0, 2));
        left.removeFromTop (12);
        sound.button.setBounds (left.removeFromTop (36).reduced (0, 2));
        a.removeFromLeft (4);
        const int kw = a.getWidth() / 2;
        placeKnob (bpm, a.removeFromLeft (kw).withTrimmedBottom (10).withTrimmedTop (2));
        placeKnob (volume, a.withTrimmedBottom (10).withTrimmedTop (2));
    }
    {
        auto a = midiPanel.reduced (14, 0);
        a.removeFromTop (18);
        auto top = a.removeFromTop (86);
        exportBtn.setBounds (top.removeFromLeft (132).withSizeKeepingCentre (132, 54));
        top.removeFromLeft (2);
        placeKnob (channel, top.removeFromLeft (76));
        top.removeFromLeft (4);
        auto col = top.withSizeKeepingCentre (top.getWidth(), 78);
        notesButton.setBounds (col.removeFromTop (37));
        col.removeFromTop (4);
        colorMidi.button.setBounds (col.removeFromTop (37));
        a.removeFromTop (6);
        dragZone.setBounds (a.removeFromTop (56));
    }

    grid = { 62, 666, W - 124, 176 };
    renderBackground();
}

//==============================================================================
// Opção avançada: mapa de notas
static juce::String noteLabel (int n)
{
    return juce::MidiMessage::getMidiNoteName (n, true, true, 3) + "  " + juce::String (n);
}

void NoteWindow::mouseDown (const juce::MouseEvent& e)
{
    if (e.position.x < 30.0f)                           { select (juce::jmax (0, index() - 1)); return; }
    if (e.position.x > (float) getWidth() - 30.0f)      { select (juce::jmin (127, index() + 1)); return; }
    dragStart = index();
    dragging = true;
    beginGesture();
}

void NoteWindow::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
        setDuring ((float) juce::jlimit (0, 127, dragStart - e.getDistanceFromDragStartY() / 5));
}

void NoteWindow::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const int dir = w.deltaY > 0 ? 1 : (w.deltaY < 0 ? -1 : 0);
    if (dir != 0) select (juce::jlimit (0, 127, index() + dir));
}

void NoteWindow::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    juce::ColourGradient frame (juce::Colour (0xff3a3632), b.getX(), b.getY(), juce::Colour (0xff141210), b.getX(), b.getBottom(), false);
    g.setGradientFill (frame);
    g.fillRoundedRectangle (b, 4.0f);
    const auto mouse = getMouseXYRelative().toFloat();
    for (int dir : { -1, 1 })
    {
        const float cx = dir < 0 ? b.getX() + 15.0f : b.getRight() - 15.0f, cy = b.getCentreY(), sz = 5.5f;
        const bool lit = isMouseOver() && (dir < 0 ? mouse.x < 30.0f : mouse.x > b.getRight() - 30.0f);
        juce::Path p;
        if (dir < 0) p.addTriangle (cx + sz * 0.6f, cy - sz, cx + sz * 0.6f, cy + sz, cx - sz * 0.8f, cy);
        else         p.addTriangle (cx - sz * 0.6f, cy - sz, cx - sz * 0.6f, cy + sz, cx + sz * 0.8f, cy);
        g.setColour (lit ? colours::amber : colours::cream.withAlpha (0.6f));
        g.fillPath (p);
    }
    glowText (g, b.reduced (30.0f, 5.0f), noteLabel (index()), 14.0f);
}

NoteMapPanel::NoteMapPanel (DubDrumsProcessor& p) : proc (p)
{
    for (const auto& pc : DubDrumsProcessor::pieces)
    {
        notes.push_back (std::make_unique<NoteWindow> (*proc.apvts.getParameter (juce::String ("note_") + pc.id)));
        addAndMakeVisible (*notes.back());
    }
    resetGm.onClick = [this]
    {
        for (int i = 0; i < DubDrumsProcessor::numPieces; ++i)
            notes[(size_t) i]->setNote (DubDrumsProcessor::pieces[i].gmNote);
    };
    close.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (resetGm);
    addAndMakeVisible (close);
}

void NoteMapPanel::mouseDown (const juce::MouseEvent& e)
{
    if (! card.contains (e.getPosition()) && onClose) onClose();
}

void NoteMapPanel::resized()
{
    card = getLocalBounds().withSizeKeepingCentre (600, 520);
    auto a = card.reduced (36, 0);
    a.removeFromTop (92);
    for (int i = 0; i < DubDrumsProcessor::numPieces; ++i)
    {
        auto row = a.removeFromTop (38);
        notes[(size_t) i]->setBounds (row.removeFromRight (250).reduced (0, 4));
    }
    a.removeFromTop (16);
    auto buttons = a.removeFromTop (44);
    close.setBounds (buttons.removeFromRight (150));
    buttons.removeFromRight (12);
    resetGm.setBounds (buttons.removeFromRight (150));
}

void NoteMapPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.55f));

    auto c = card.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (c.translated (3.0f, 6.0f), 8.0f);
    juce::ColourGradient pg (colours::plate.brighter (0.04f), c.getX(), c.getY(), colours::plateDark, c.getRight(), c.getBottom(), false);
    g.setGradientFill (pg);
    g.fillRoundedRectangle (c, 8.0f);
    g.setColour (colours::ink.withAlpha (0.6f));
    g.drawRoundedRectangle (c.reduced (10.0f), 5.0f, 1.2f);

    // plaqueta preta com o título
    g.setFont (stencil (12.0f, 0.22f));
    const juce::String title ("NOTAS MIDI  -  AVANCADO");
    const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title) + 26.0f;
    auto tag = juce::Rectangle<float> (c.getX() + 24.0f, c.getY() + 22.0f, tw, 22.0f);
    g.setColour (colours::labelBg);
    g.fillRoundedRectangle (tag, 2.0f);
    g.setColour (colours::cream);
    g.drawText (title, tag, juce::Justification::centred);

    g.setColour (colours::inkSoft);
    g.setFont (stencil (9.5f, 0.2f));
    g.drawText ("QUAL NOTA CADA PECA DA BATERIA ENVIA  -  AJUSTE PARA O SEU KIT", c.reduced (36.0f, 0).withY (c.getY() + 56.0f).withHeight (16.0f),
                juce::Justification::centredLeft);

    g.setFont (stencil (12.0f, 0.16f));
    for (int i = 0; i < DubDrumsProcessor::numPieces; ++i)
    {
        const auto nb = notes[(size_t) i]->getBounds().toFloat();
        const auto row = juce::Rectangle<float> (c.getX() + 36.0f, nb.getY(), nb.getX() - c.getX() - 56.0f, nb.getHeight());
        g.setColour (colours::ink.withAlpha (0.25f));
        g.drawHorizontalLine ((int) nb.getBottom() + 3, row.getX(), nb.getRight());
        g.setColour (colours::ink);
        g.drawText (juce::String (DubDrumsProcessor::pieces[i].name).toUpperCase(), row, juce::Justification::centredLeft);
        g.setColour (colours::inkSoft);
        g.setFont (stencil (9.0f, 0.12f));
        g.drawText ("GM " + juce::String (DubDrumsProcessor::pieces[i].gmNote), row, juce::Justification::centredRight);
        g.setFont (stencil (12.0f, 0.16f));
    }
}
