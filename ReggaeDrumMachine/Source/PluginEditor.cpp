// Interface do Mastor Reggae Drum Machine MRDM-12 (antigo Dub Drums) — visual copiado do DubMastor Dub Chords
#include "PluginEditor.h"
#include "GroovePresets.h"

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
    const juce::String title ("MASTOR REGGAE DRUM MACHINE MRDM-12 " DD_VERSION_SHORT);   // padrão: versão logo depois do nome
    {   // nome comprido: diminui a fonte até o título + a placa DUB STATION caberem antes das faixas da bandeira
        float fs = 30.0f;
        const float maxW = (float) plate.getWidth() - 330.0f - 32.0f - 26.0f - 250.0f;
        while (fs > 20.0f && juce::GlyphArrangement::getStringWidth (stencil (fs, 0.22f), title) > maxW)
            fs -= 1.0f;
        g.setFont (stencil (fs, 0.22f));
    }
    g.drawText (title, plate.getX() + 32, 20, 760, 34, juce::Justification::left);
    const float titleRight = (float) plate.getX() + 32.0f + juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title);

    {   // placa de latão "DUB STATION"
        g.setFont (stencil (15.0f, 0.34f));
        const juce::String t ("DUB STATION");
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
    section (drumMod,   "BATERISTA");
    section (hatMod,    "HI-HAT");
    section (dynMod,    "DINAMICA");
    section (randPanel, "ALEATORIEDADE");
    section (soundPanel, "SOM");
    section (midiPanel, "MIDI");
    section (grid.expanded (10, 14).withTrimmedBottom (2), "PADRAO");

    // rótulos gravados acima dos controles
    g.setColour (colours::inkSoft);
    g.setFont (stencil (9.0f, 0.3f));
    for (auto [c, t] : { std::pair<juce::Component*, const char*> { &styleButtons, "ESCOLHA O ESTILO" }, { &beatGrid, "BATIDA PRINCIPAL" },
                         { &bars, "COMPASSOS" }, { &seed, "SEMENTE" }, { &hatButtons, "FIGURA" },
                         { &backbeatButtons, "SOM DA CAIXA" }, { &kitButtons, "KIT" } })
        g.drawText (t, c->getBounds().translated (0, -15).withHeight (12), juce::Justification::centred);
    g.drawText ("HUMANIZA FORCA E TEMPO", randPanel.reduced (14, 0).removeFromBottom (18).translated (0, -4),
                juce::Justification::centredRight);
    g.drawText ("CLIQUE NA GRADE: POR / TIRAR NOTA  -  RODINHA: FORCA  -  ARRASTE A FAIXA DO COMPASSO PARA A DAW",
                juce::Rectangle<int> (grid.getX() + 100, grid.getY() - 33, grid.getWidth() - 320, 12), juce::Justification::centredLeft);

    // escalas
    drawKnobScale (g, swing.slider, 5, { "RETO", "LEVE", "MEDIO", "PESADO", "SHUF." });
    drawKnobScale (g, dynamics.slider, 6, { "SUAVE", "MEDIA", "FORTE", "ACENT.", "CRESC.", "RESPIRA" });
    for (auto* k : { &rndKick, &rndSnare, &rndHat, &rndPerc })
        drawKnobScale (g, k->slider, 11);
    drawKnobScale (g, channel.slider, 16);
    drawKnobScale (g, bpm.slider, 11);
    drawKnobScale (g, volume.slider, 11);
}

//==============================================================================
[[maybe_unused]] static const char* kStyleNames[] = { "ROCKERS", "ONE DROP", "BOOM BAP" };
static const char* kHatNames[]   = { "COLCHEIAS", "SEMICOLCHEIAS", "CONTRATEMPO", "TERCINAS" };
static const char* kDynNames[]   = { "SUAVE", "MEDIA", "FORTE", "ACENTUADA", "CRESCENDO", "RESPIRANDO" };
static const char* kDrummerNames[] = { "STYLE SCOTT", "SANTA DAVIS", "CARLTON BARRETT", "SLY DUNBAR", "DIGIKAL" };
// letreiro do ESTILO: o texto passa como num painel de letras de bar
static const char* kStyleMarquee[] = { "ROCKERS   *   BUMBO NO 1 E NO 3   *   CAIXA NO 2 E NO 4   *   CHANNEL ONE 1976   *   ",
                                       "ONE DROP   *   BUMBO E CAIXA JUNTOS NO 2 E NO 4   *   O 1 FICA VAZIO   *   ",
                                       "BOOM BAP   *   HIP-HOP ANOS 90   *   BUMBO NO 1, E DO 2, E DO 3   *   CAIXA NO 2 E NO 4   *   " };

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
        if (! compact)
        {
            pushButtonCap (g, c.withTrimmedBottom (2.5f), index() == i, isMouseOver() && c.contains (mouse), names[i]);
            continue;
        }
        // tampa menor: lâmpada pequena em cima, texto embaixo
        const bool on = index() == i, over = isMouseOver() && c.contains (mouse);
        auto cap = c.withTrimmedBottom (2.5f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (cap.translated (0, 2.5f), 3.0f);
        cap = cap.translated (0, on ? 2.0f : 0.0f);
        juce::ColourGradient cg (juce::Colour (on ? 0xff2c2925 : 0xff3a3632).brighter (over && ! on ? 0.08f : 0.0f), cap.getX(), cap.getY(),
                                 juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (cap, 3.0f);
        const auto lamp = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ cap.getCentreX(), cap.getY() + 9.0f });
        if (on) { g.setColour (colours::amber.withAlpha (0.35f)); g.fillEllipse (lamp.expanded (4.0f)); }
        g.setColour (on ? colours::amber : juce::Colour (0xff4a3a22));
        g.fillEllipse (lamp);
        g.setColour (on ? colours::cream : colours::cream.withAlpha (0.5f));
        g.setFont (stencil (10.0f, 0.1f));
        g.drawText (names[i], cap.withTrimmedTop (15.0f), juce::Justification::centred);
    }
}

void ChoiceButtons::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < names.size(); ++i)
        if (cell (i).contains (e.position)) { select (i); if (onSelect) onSelect (i); return; }
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

bool SeedWindow::animating() const
{
    return juce::Time::getMillisecondCounterHiRes() - animStart < 1100.0 || (shown != index() && shown >= 0);
}

// JACKPOT: quando a semente muda (NOVA VARIAÇÃO), cada número roda como num caça-níquel
// e para um de cada vez, da esquerda para a direita, no número sorteado
void SeedWindow::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    const int v = index();
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (shown < 0) shown = v;
    if (v != shown)
    {
        if (! isMouseButtonDown()) animStart = now;   // arrastando a semente na mão: sem animação
        shown = v;
    }
    const float alpha = isMouseOver() ? 1.0f : 0.9f;
    glowText (g, r, {}, 20.0f, alpha);
    const auto digits = juce::String (v).paddedLeft ('0', 4);
    const double t = now - animStart;
    const float cellW = 15.0f;
    auto cells = r.withSizeKeepingCentre (cellW * 4.0f, r.getHeight());
    g.setFont (stencil (20.0f, 0.1f));
    for (int k = 0; k < 4; ++k)
    {
        auto cell = cells.removeFromLeft (cellW);
        const double stopAt = 420.0 + 170.0 * k;
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (cell.expanded (2.0f, 0).toNearestInt());
        auto drawDigit = [&] (const juce::String& d, float dy, float a)
        {
            g.setColour (colours::amber.withAlpha (0.18f * a));
            for (auto o : { juce::Point<float> (-1, 0), { 1, 0 }, { 0, -1 }, { 0, 1 } })
                g.drawText (d, cell.translated (o.x, o.y + dy), juce::Justification::centred);
            g.setColour (colours::amber.withAlpha (a));
            g.drawText (d, cell.translated (0, dy), juce::Justification::centred);
        };
        if (t < stopAt)
        {
            const double pos = t / 38.0 + k * 3.7;   // rolos girando para cima
            const int d1 = ((int) pos * 7 + k * 3) % 10, d2 = ((int) pos * 7 + 7 + k * 3) % 10;
            const float frac = (float) (pos - std::floor (pos));
            drawDigit (juce::String (d1), -frac * cell.getHeight(), alpha * 0.85f);
            drawDigit (juce::String (d2), (1.0f - frac) * cell.getHeight(), alpha * 0.85f);
        }
        else
        {
            const float settle = (float) juce::jmin (1.0, (t - stopAt) / 90.0);   // pequeno tranco ao parar
            drawDigit (digits.substring (k, k + 1), (1.0f - settle) * -4.0f, alpha);
        }
    }
}

void PushButton::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (b, 5.0f);
    if (! red)
    {
        pushButtonCap (g, b.reduced (4.0f).withTrimmedBottom (2.5f), down, isMouseOver(), text);
        return;
    }
    // tampa vermelha (como o botão de gravar das mesas antigas), com brilho pulsando para chamar atenção
    if (attention > 0.0f)
    {
        g.setColour (juce::Colour (0xffff5a3c).withAlpha (0.10f + 0.30f * attention));
        g.fillRoundedRectangle (b.expanded (1.0f), 7.0f);
    }
    auto cap = b.reduced (4.0f).withTrimmedBottom (2.5f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (cap.translated (0, 2.5f), 4.0f);
    cap = cap.translated (0, down ? 2.0f : 0.0f);
    juce::ColourGradient rg (juce::Colour (down ? 0xffb2321e : (isMouseOver() ? 0xffe8533a : 0xffd8442c)), cap.getX(), cap.getY(),
                             juce::Colour (down ? 0xff5c140b : 0xff7e1a0e), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (rg);
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.fillRoundedRectangle (cap.reduced (5.0f, 0).withHeight (cap.getHeight() * 0.32f).translated (0, 2.0f), 3.0f);
    if (attention > 0.0f)
    {
        g.setColour (juce::Colour (0xffffb08a).withAlpha (0.18f * attention));
        g.fillRoundedRectangle (cap, 4.0f);
    }
    if (sheen >= 0.0f && sheen <= 1.0f)   // reflexo atravessando a tampa
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (cap, 4.0f);
        g.reduceClipRegion (clip);
        const float x = cap.getX() - 40.0f + (cap.getWidth() + 80.0f) * sheen;
        juce::Path band;
        band.startNewSubPath (x, cap.getBottom()); band.lineTo (x + 18.0f, cap.getY());
        band.lineTo (x + 34.0f, cap.getY()); band.lineTo (x + 16.0f, cap.getBottom()); band.closeSubPath();
        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.fillPath (band);
    }
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (cap, 4.0f, 1.0f);
    g.setColour (colours::cream);
    g.setFont (stencil (12.5f, 0.16f));
    g.drawText (text, cap, juce::Justification::centred);
}

//==============================================================================
// BATIDA PRINCIPAL: bumbo e caixa por colcheia
BeatGrid::BeatGrid (juce::RangedAudioParameter& kick, juce::RangedAudioParameter& snare)
    : kickAtt (kick,  [this] (float v) { masks[0] = juce::roundToInt (v); repaint(); }, nullptr),
      snareAtt (snare, [this] (float v) { masks[1] = juce::roundToInt (v); repaint(); }, nullptr)
{
    kickAtt.sendInitialUpdate();
    snareAtt.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

juce::Rectangle<float> BeatGrid::cell (int row, int col) const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromTop (13.0f);                       // números 1 e 2 e 3 e 4 e
    r.removeFromLeft (42.0f);                      // nomes BUMBO / CAIXA
    const float cw = r.getWidth() / 8.0f, ch = r.getHeight() / 2.0f;
    return juce::Rectangle<float> (r.getX() + cw * (float) col, r.getY() + ch * (float) row, cw, ch).reduced (1.5f, 2.0f);
}

bool BeatGrid::hit (juce::Point<float> p, int& row, int& col) const
{
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 8; ++c)
            if (cell (r, c).expanded (1.5f, 2.0f).contains (p)) { row = r; col = c; return true; }
    return false;
}

void BeatGrid::setCell (int row, int col, bool on)
{
    const int m = on ? (masks[row] | (1 << col)) : (masks[row] & ~(1 << col));
    if (m == masks[row]) return;
    masks[row] = m;
    (row == 0 ? kickAtt : snareAtt).setValueAsPartOfGesture ((float) m);
    repaint();
}

void BeatGrid::mouseDown (const juce::MouseEvent& e)
{
    int row, col;
    if (! hit (e.position, row, col)) return;
    dragRow = row;
    dragValue = ! ((masks[row] >> col) & 1);
    (row == 0 ? kickAtt : snareAtt).beginGesture();
    setCell (row, col, dragValue);
}

void BeatGrid::mouseDrag (const juce::MouseEvent& e)
{
    int row, col;
    if (dragRow >= 0 && hit (e.position, row, col) && row == dragRow)
        setCell (row, col, dragValue);
}

void BeatGrid::mouseUp (const juce::MouseEvent&)
{
    if (dragRow >= 0) (dragRow == 0 ? kickAtt : snareAtt).endGesture();
    dragRow = -1;
}

void BeatGrid::paint (juce::Graphics& g)
{
    static const char* counts[] = { "1", "e", "2", "e", "3", "e", "4", "e" };
    static const char* rows[]   = { "BUMBO", "CAIXA" };
    auto b = getLocalBounds().toFloat();

    for (int c = 0; c < 8; ++c)
    {
        const auto cr = cell (0, c);
        g.setColour (c % 2 == 0 ? colours::ink : colours::inkSoft.withAlpha (0.7f));
        g.setFont (stencil (c % 2 == 0 ? 10.0f : 8.5f, 0.05f));
        g.drawText (counts[c], juce::Rectangle<float> (cr.getX(), b.getY(), cr.getWidth(), 12.0f), juce::Justification::centred);
    }

    // moldura metálica comum, como a dos botões
    auto frame = juce::Rectangle<float> (cell (0, 0).getX() - 4.0f, cell (0, 0).getY() - 4.0f, 0, 0)
                     .withRightX (cell (1, 7).getRight() + 4.0f).withBottomY (cell (1, 7).getBottom() + 4.0f);
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (frame, 4.0f);

    for (int r = 0; r < 2; ++r)
    {
        const auto c0 = cell (r, 0);
        g.setColour (colours::ink);
        g.setFont (stencil (9.0f, 0.10f));
        g.drawText (rows[r], juce::Rectangle<float> (b.getX(), c0.getY(), 38.0f, c0.getHeight()), juce::Justification::centredLeft);

        for (int c = 0; c < 8; ++c)
        {
            const auto cr = cell (r, c);
            const bool on = (masks[r] >> c) & 1;
            juce::ColourGradient cg (juce::Colour (on ? 0xff3a2a14 : 0xff34302c), cr.getX(), cr.getY(),
                                     juce::Colour (on ? 0xff1c140a : 0xff141210), cr.getX(), cr.getBottom(), false);
            g.setGradientFill (cg);
            g.fillRoundedRectangle (cr, 2.5f);
            if (on)
            {
                const auto lamp = cr.reduced (cr.getWidth() * 0.22f, cr.getHeight() * 0.22f);
                g.setColour (colours::amber.withAlpha (0.35f));
                g.fillEllipse (lamp.expanded (2.5f));
                g.setColour (colours::amber);
                g.fillEllipse (lamp);
            }
            else if (c % 2 == 0)
            {
                g.setColour (colours::cream.withAlpha (0.18f));
                g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (cr.getCentre()));
            }
        }
    }
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
      styleButtons (*p.apvts.getParameter ("style"), { "ROCKERS", "ONE DROP", "BOOM BAP" }, 1),
      hatButtons (*p.apvts.getParameter ("hatPattern"), { "1/8", "1/16", "CONTRA", "TERCINA" }, 2),
      backbeatButtons (*p.apvts.getParameter ("backbeat"), { "CAIXA", "ARO", "AMBOS" }, 3, true),
      kitButtons (*p.apvts.getParameter ("kit"), { "ANOS 70", "DIGITAL" }, 2, true),
      drummerSwitch (*p.apvts.getParameter ("drummer")),
      grooveSelector (p),
      bars (*p.apvts.getParameter ("bars")),
      seed (*p.apvts.getParameter ("seed")),
      beatGrid (*p.apvts.getParameter ("kickBeats"), *p.apvts.getParameter ("snareBeats")),
      noteMap (p),
      dragZone (p)
{
    setLookAndFeel (&lnf);

    for (juce::Component* c : { (juce::Component*) &styleButtons, (juce::Component*) &hatButtons,
                                (juce::Component*) &backbeatButtons, (juce::Component*) &drummerSwitch, (juce::Component*) &grooveSelector,
                                (juce::Component*) &kitButtons, (juce::Component*) &beatGrid,
                                (juce::Component*) &bars, (juce::Component*) &seed, (juce::Component*) &variation })
        addAndMakeVisible (c);
    variation.onClick = [this] { proc.newVariation(); };
    styleButtons.onSelect = [this] (int st) { proc.applyStyleDefaults (st); };   // estilo carrega bumbo e caixa dele

    setupToggle (fill, "fill", "VIRADA NO FIM");
    setupToggle (intro, "intro", "VIRADA DE INTRO");
    setupToggle (swingAll, "swingAll", "SWING EM TUDO");
    setupToggle (sound, "sound", "SOM INTERNO");
    setupToggle (colorMidi, "colorMidi", "MIDI COLORIDO");

    setupKnob (swing, "swing", "SWING");
    setupKnob (dynamics, "dynamics", {});
    for (auto* k : { &swing, &dynamics })
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

    addAndMakeVisible (previewButton);
    addAndMakeVisible (speaker);
    previewAtt = std::make_unique<ButtonAttach> (proc.apvts, "preview", previewButton);
    addChildComponent (clearEdits);
    clearEdits.onClick = [this] { proc.clearEdits(); };

    setSize (1280, 804);   // mesma largura da 1.0: o Riddim Machine encaixa este painel na aba BATERIA
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
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Mastor Reggae Drum Machine");
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
    const bool edits = proc.hasEdits();
    if (edits != showingEdits) { showingEdits = edits; clearEdits.setVisible (edits); }
    const bool playing = proc.isHostPlaying() || proc.isPreviewing();
    {   // NOVA VARIAÇÃO: respira devagar e um reflexo passa a cada 3 s
        const double ms = juce::Time::getMillisecondCounterHiRes();
        variation.attention = 0.5f + 0.5f * std::sin ((float) (ms * 0.0042));
        const double cyc = std::fmod (ms, 3000.0) / 700.0;
        variation.sheen = cyc <= 1.0 ? (float) cyc : -1.0f;
        variation.repaint();
        repaint (styleDisplay.expanded (4));       // letreiro do estilo
        if (seed.animating()) seed.repaint();      // jackpot da semente
    }
    previewButton.pulse = playing ? 0.5f + 0.5f * std::sin ((float) juce::Time::getMillisecondCounterHiRes() * 0.0063f) : 0.0f;
    if (playing || previewButton.getToggleState()) previewButton.repaint();
    {   // cone do alto-falante acompanha o som
        const float lv = std::sqrt (juce::jlimit (0.0f, 1.0f, proc.getOutputLevel() * 1.6f));
        if (std::abs (lv - speaker.level) > 0.004f) { speaker.level = lv; speaker.repaint(); }
    }
    if (v != lastVersion)
    {
        lastVersion = v;
        pattern = proc.getPatternCopy();
        repaint();
    }
    else if (playing || isMouseOver (true))
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

    {   // ESTILO: letreiro analógico passando
        const int st = juce::jlimit (0, 2, choice ("style"));
        const auto r = styleDisplay.toFloat();
        glowText (g, r, {}, 19.0f);
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (r.reduced (5.0f, 2.0f).toNearestInt());
        const juce::String text (kStyleMarquee[st]);
        const auto font = stencil (19.0f, 0.1f);
        const float tw = juce::GlyphArrangement::getStringWidth (font, text);
        const float speed = 46.0f;   // pixels por segundo
        const float off = std::fmod ((float) (juce::Time::getMillisecondCounterHiRes() * 0.001) * speed, juce::jmax (1.0f, tw));
        g.setFont (font);
        for (float x = r.getX() + 8.0f - off; x < r.getRight(); x += tw)
        {
            const auto tr = juce::Rectangle<float> (x, r.getY(), tw, r.getHeight());
            g.setColour (colours::amber.withAlpha (0.18f));
            for (auto o : { juce::Point<float> (-1, 0), { 1, 0 }, { 0, -1 }, { 0, 1 } })
                g.drawText (text, tr.translated (o.x, o.y), juce::Justification::centredLeft, false);
            g.setColour (colours::amber);
            g.drawText (text, tr, juce::Justification::centredLeft, false);
        }
        // grade de lâmpadas do letreiro (linhas e colunas escuras por cima)
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        for (float y = r.getY() + 1.0f; y < r.getBottom(); y += 3.0f) g.fillRect (r.getX(), y, r.getWidth(), 1.0f);
        for (float x = r.getX() + 1.0f; x < r.getRight(); x += 3.0f)  g.fillRect (x, r.getY(), 1.0f, r.getHeight());
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f), 3.0f);
    }
    glowText (g, hatDisplay.toFloat(), kHatNames[juce::jlimit (0, 3, choice ("hatPattern"))], 15.0f);

    // ficha do baterista: nome no display e, gravado embaixo, a banda e a assinatura
    {
        static const char* bands[] = { "ROOTS RADICS  -  VOLCANO", "SOUL SYNDICATE  -  AGGROVATORS", "THE WAILERS",
                                       "SLY & ROBBIE  -  BLACK UHURU", "" };
        static const char* sigs[]  = { "RETO, ACENTO NO CONTRATEMPO",
                                       "FLYING CYMBAL NO CONTRATEMPO",
                                       "TERCINA QUEBRADA, CAI DO 1",
                                       "ROCKERS: SEMICOLCHEIA E SWISH",
                                       "" };
        const bool mt40 = (choice ("seed") % 2) == 1;
        bands[4] = mt40 ? "MAQUINA: CASIO MT-40  (SEMENTE IMPAR)" : "MAQUINA: OBERHEIM DMX  (SEMENTE PAR)";
        sigs[4]  = mt40 ? "PRESET ROCK DO SLENG TENG 1985" : "DANCEHALL DIGITAL ANOS 80";
        const int d = juce::jlimit (0, dubdrums::numDrummers - 1, choice ("drummer"));
        glowText (g, drummerDisplay.toFloat(), kDrummerNames[d], 17.0f);
        // embaixo do visor GROOVES: como é o groove carregado, ou a banda / assinatura do baterista
        auto info = drumMod.reduced (12, 0).withTop (grooveSelector.getBottom() + 1).withBottom (drumMod.getBottom() - 4).toFloat();
        const int gr = proc.getGroove();
        const bool showGroove = gr >= 0 && dubdrums::groovePresets[gr].drummer == d && proc.grooveIntact();
        g.setColour (showGroove ? colours::ink : colours::inkSoft);
        g.setFont (stencil (9.0f, 0.1f));
        g.drawFittedText (showGroove ? juce::String (dubdrums::groovePresets[gr].note)
                                     : juce::String (bands[d]) + "  -  " + (choice ("kit") == 1 && d != 4 ? "KIT DIGITAL" : sigs[d]),
                          info.toNearestInt(), juce::Justification::centred, 1, 0.8f);
    }
    glowText (g, dynDisplay.toFloat(), kDynNames[juce::jlimit (0, 5, choice ("dynamics"))], 17.0f);

    drawPattern (g, grid.toFloat());
}

namespace
{
    struct GridRow { int note; const char* name; };
    const GridRow kGridRows[] = {
        { dubdrums::gm::Crash, "PRATO" }, { dubdrums::gm::OpenHat, "HH ABERTO" }, { dubdrums::gm::ClosedHat, "HH FECHADO" },
        { dubdrums::gm::TomHigh, "TOM 1" }, { dubdrums::gm::TomMid, "TOM 2" }, { dubdrums::gm::TomLow, "SURDO" },
        { dubdrums::gm::Snare, "CAIXA" }, { dubdrums::gm::SideStick, "ARO" }, { dubdrums::gm::Kick, "BUMBO" } };
    constexpr int kGridNumRows = 9;

    // área das notas dentro do visor PADRÃO (o resto são os nomes e a faixa dos compassos)
    juce::Rectangle<float> gridNotesArea (juce::Rectangle<float> r)
    {
        auto area = r.reduced (10.0f, 8.0f);
        area.removeFromLeft (82.0f);
        area.removeFromTop (18.0f);
        return area;
    }
}

bool DubDrumsPanel::gridHit (juce::Point<float> p, int& row, double& beat) const
{
    const auto area = gridNotesArea (grid.toFloat());
    if (! area.contains (p)) return false;
    const double total = juce::jmax (1.0, pattern.introLengthInBeats + pattern.lengthInBeats);
    row = juce::jlimit (0, kGridNumRows - 1, (int) ((p.y - area.getY()) / (area.getHeight() / (float) kGridNumRows)));
    beat = juce::jlimit (0.0, total - 1.0e-6, (double) ((p.x - area.getX()) / area.getWidth()) * total);
    return true;
}

int DubDrumsPanel::barTagAt (juce::Point<float> p) const
{
    for (const auto& [r, bar] : barTags) if (r.contains (p)) return bar;
    return -3;
}

void DubDrumsPanel::dragBar (int bar)
{
    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (proc.suggestedFileName (bar));
    if (proc.writeMidiFile (file, bar))
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (file.getFullPathName()), false, this);
}

void DubDrumsPanel::mouseDown (const juce::MouseEvent& e)
{
    int row; double beat;
    dragging = false;
    pressedBar = barTagAt (e.position);
    barDragged = false;
    if (pressedBar != -3) return;   // faixa do compasso: arrastar leva só esse compasso
    if (! gridHit (e.position, row, beat)) return;
    const int note = kGridRows[row].note;
    dragAdd = e.mods.isPopupMenu() ? false : ! proc.noteAt (note, beat);   // botão direito sempre apaga
    proc.toggleNote (note, beat, dragAdd);
    dragging = true;
    lastRow = row;
    lastCell = (int) std::floor (beat * 4.0);
}

void DubDrumsPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (pressedBar != -3)
    {
        if (! barDragged && e.getDistanceFromDragStart() > 4) { barDragged = true; dragBar (pressedBar); }
        return;
    }
    int row; double beat;
    if (! dragging || ! gridHit (e.position, row, beat)) return;
    const int cell = (int) std::floor (beat * 4.0);
    if (row == lastRow && cell == lastCell) return;
    lastRow = row; lastCell = cell;
    proc.toggleNote (kGridRows[row].note, beat, dragAdd);   // pinta (ou apaga) as casas por onde passa
}

void DubDrumsPanel::mouseMove (const juce::MouseEvent& e)
{
    int row; double beat;
    setMouseCursor (barTagAt (e.position) != -3 ? juce::MouseCursor::DraggingHandCursor
                    : gridHit (e.position, row, beat) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    repaint (grid.expanded (4));
}

void DubDrumsPanel::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    int row; double beat;
    if (! gridHit (e.position, row, beat) || std::abs (w.deltaY) < 1.0e-6f) return;
    proc.changeVelocity (kGridRows[row].note, beat, w.deltaY > 0 ? 8 : -8);
}

void DrumsPreviewButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    const bool on = getToggleState();
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (b, 7.0f);
    auto cap = b.reduced (4.0f).withTrimmedBottom (3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (cap.translated (0, 3.0f), 5.0f);
    cap = cap.translated (0, down ? 2.0f : 0.0f);
    if (on)
    {
        g.setColour (colours::jamGreen.withAlpha (0.25f + 0.25f * pulse));
        g.fillRoundedRectangle (b.expanded (2.0f), 9.0f);
    }
    const auto top = on ? juce::Colour (0xff27c45e) : (over ? juce::Colour (0xff3b9a58) : juce::Colour (0xff2c7c45));
    const auto bot = on ? juce::Colour (0xff0b6a2c) : juce::Colour (0xff0f3f1f);
    juce::ColourGradient gr (top, cap.getX(), cap.getY(), bot, cap.getX(), cap.getBottom(), false);
    g.setGradientFill (gr);
    g.fillRoundedRectangle (cap, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.30f : 0.18f));
    g.fillRoundedRectangle (cap.reduced (6.0f, 0).withHeight (cap.getHeight() * 0.30f).translated (0, 2.0f), 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (cap, 5.0f, 1.2f);

    // lâmpada à esquerda e símbolo play / stop
    auto content = cap.reduced (10.0f, 0);
    const auto lamp = content.removeFromLeft (16.0f).withSizeKeepingCentre (14.0f, 14.0f);
    g.setColour (on ? juce::Colour (0xffd8ffb0).withAlpha (0.55f + 0.45f * pulse) : juce::Colour (0xff20301f));
    g.fillEllipse (lamp);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (lamp, 1.0f);
    content.removeFromLeft (8.0f);
    const auto icon = content.removeFromLeft (18.0f).withSizeKeepingCentre (16.0f, 18.0f);
    juce::Path sym;
    if (on) sym.addRoundedRectangle (icon.reduced (1.5f, 2.5f), 2.0f);
    else    sym.addTriangle (icon.getX() + 1.0f, icon.getY(), icon.getX() + 1.0f, icon.getBottom(), icon.getRight(), icon.getCentreY());
    g.setColour (colours::cream);
    g.fillPath (sym);
    content.removeFromLeft (8.0f);
    g.setFont (stencil (14.5f, 0.06f));
    g.drawText (on ? "PARAR" : "PREVIEW", content, juce::Justification::centredLeft);
}

void DubDrumsPanel::drawPattern (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r.expanded (3), 4.0f);
    juce::ColourGradient gl (colours::glass.brighter (0.15f), r.getX(), r.getY(), colours::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r, 3.0f);

    const auto* rows = kGridRows;
    constexpr int numRows = kGridNumRows;

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
        if (i == 0) barTags.clear();
        barTags.push_back ({ tag.expanded (0, 2.0f), bar });
        if (isMouseOver() && tag.expanded (0, 2.0f).contains (getMouseXYRelative().toFloat()))
        {
            g.setColour (col.withAlpha (0.35f));
            g.fillRoundedRectangle (tag, 2.0f);
        }
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

    // --- 5 módulos de cima: ESTILO | LOOP | BATERISTA | HI-HAT | DINÂMICA
    const int x0 = 52, my = 92, mh = 318, gap = 10;
    styleMod = { x0,                        my, 214, mh };
    loopMod  = { styleMod.getRight() + gap, my, 214, mh };
    drumMod  = { loopMod.getRight() + gap,  my, 270, mh };
    hatMod   = { drumMod.getRight() + gap,  my, 226, mh };
    dynMod   = { hatMod.getRight() + gap,   my, W - 52 - (hatMod.getRight() + gap), mh };

    {   // ESTILO: estilo, batida principal e onde cai o bumbo
        auto a = styleMod.reduced (14, 0);
        a.removeFromTop (22);
        styleDisplay = a.removeFromTop (34).reduced (2, 0);
        a.removeFromTop (18);
        styleButtons.setBounds (a.removeFromTop (96));
        a.removeFromTop (18);
        beatGrid.setBounds (a.removeFromTop (62));
        a.removeFromTop (18);
        backbeatButtons.setBounds (a.removeFromTop (38));
    }
    {   // LOOP
        auto a = loopMod.reduced (14, 0);
        a.removeFromTop (32);
        bars.setBounds (a.removeFromTop (50).reduced (2, 0));
        a.removeFromTop (12);
        fill.button.setBounds (a.removeFromTop (32).reduced (12, 0));
        a.removeFromTop (6);
        intro.button.setBounds (a.removeFromTop (32).reduced (12, 0));
        a.removeFromTop (26);
        seed.setBounds (a.removeFromTop (40).reduced (36, 0));
        a.removeFromTop (12);
        variation.setBounds (a.removeFromTop (42).reduced (8, 0));
    }
    {   // BATERISTA: display, chave seletora de 4 posições e ficha
        auto a = drumMod.reduced (14, 0);
        a.removeFromTop (22);
        drummerDisplay = a.removeFromTop (34).reduced (2, 0);
        a.removeFromTop (4);
        drummerSwitch.setBounds (a.removeFromTop (184));
        a.removeFromTop (2);
        grooveSelector.setBounds (a.removeFromTop (44));   // GROOVES: visor azul com as músicas do baterista
    }
    {   // HI-HAT: figura e swing
        auto a = hatMod.reduced (14, 0);
        a.removeFromTop (14);
        auto header = a.removeFromTop (28);
        swingAll.button.setBounds (header.removeFromRight (150));
        a.removeFromTop (4);
        hatDisplay = a.removeFromTop (34).reduced (2, 0);
        a.removeFromTop (18);
        hatButtons.setBounds (a.removeFromTop (72).expanded (6, 0));
        a.removeFromTop (4);
        swing.label.setBounds (a.removeFromTop (15));
        swing.slider.setBounds (juce::Rectangle<int> (92, 92).withCentre ({ a.getCentreX(), a.getY() + 64 }));
    }
    {   // DINÂMICA
        auto a = dynMod.reduced (14, 0);
        a.removeFromTop (22);
        dynDisplay = a.removeFromTop (34).reduced (2, 0);
        a.removeFromTop (14);
        dynamics.slider.setBounds (a.withTrimmedBottom (8).withSizeKeepingCentre (146, 146));
    }

    // --- linha do meio: ALEATORIEDADE | SOM | MIDI
    const int by = 432, bh = 158;
    randPanel  = { 52, by, 324, bh };
    soundPanel = { randPanel.getRight() + 14, by, 452, bh };
    midiPanel  = { soundPanel.getRight() + 14, by, W - 52 - (soundPanel.getRight() + 14), bh };
    {
        auto a = randPanel.reduced (10, 0);
        a.removeFromTop (16);
        a.removeFromBottom (22);
        const int kw = a.getWidth() / 4;
        for (auto* k : { &rndKick, &rndSnare, &rndHat, &rndPerc })
            placeKnob (*k, a.removeFromLeft (kw).withTrimmedTop (2));
    }
    {   // SOM: PREVIEW grande em destaque, BPM e VOLUME; embaixo som interno e kit
        auto a = soundPanel.reduced (12, 0);
        a.removeFromTop (18);
        auto top = a.removeFromTop (80);
        previewButton.setBounds (top.removeFromLeft (156).withSizeKeepingCentre (156, 62));
        top.removeFromLeft (8);
        speaker.setBounds (top.removeFromLeft (112).withSizeKeepingCentre (112, 80).translated (0, -1));   // alto-falante no meio
        top.removeFromLeft (4);
        const int kw = top.getWidth() / 2;
        placeKnob (bpm, top.removeFromLeft (kw));
        placeKnob (volume, top);
        a.removeFromTop (18);
        auto bottom = a.removeFromTop (34);
        sound.button.setBounds (bottom.removeFromLeft (126).reduced (0, 1));
        bottom.removeFromLeft (8);
        kitButtons.setBounds (bottom);
    }
    {
        auto a = midiPanel.reduced (14, 0);
        a.removeFromTop (16);
        auto top = a.removeFromTop (80);
        exportBtn.setBounds (top.removeFromLeft (124).withSizeKeepingCentre (124, 52));
        top.removeFromLeft (2);
        placeKnob (channel, top.removeFromLeft (72));
        top.removeFromLeft (4);
        auto col = top.withSizeKeepingCentre (top.getWidth(), 74);
        notesButton.setBounds (col.removeFromTop (35));
        col.removeFromTop (4);
        colorMidi.button.setBounds (col.removeFromTop (35));
        a.removeFromTop (6);
        dragZone.setBounds (a.removeFromTop (50));
    }

    grid = { 62, 628, W - 124, 158 };
    clearEdits.setBounds (grid.getRight() - 196, grid.getY() - 27, 200, 24);
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

//==============================================================================
// Chave seletora do baterista
juce::Point<float> DrummerSwitch::centre() const { return { (float) getWidth() * 0.5f, (float) getHeight() * 0.64f }; }
float DrummerSwitch::radius() const { return juce::jmin ((float) getWidth() * 0.19f, (float) getHeight() * 0.23f); }

juce::Rectangle<float> DrummerSwitch::labelArea (int i) const
{
    const auto c = centre();
    const float a = angleOf (i), lr = radius() + 42.0f;
    return juce::Rectangle<float> (56.0f, 28.0f).withCentre ({ c.x + std::sin (a) * lr, c.y - std::cos (a) * lr - 4.0f });
}

void DrummerSwitch::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < dubdrums::numDrummers; ++i)
        if (labelArea (i).expanded (3.0f).contains (e.position)) { select (i); return; }
    dragging = e.position.getDistanceFrom (centre()) < radius() * 1.25f;
    dragStartIndex = index();
}

void DrummerSwitch::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const int delta = (e.getDistanceFromDragStartX() - e.getDistanceFromDragStartY()) / 40;
    const int target = juce::jlimit (0, dubdrums::numDrummers - 1, dragStartIndex + delta);
    if (target != index()) select (target);
}

void DrummerSwitch::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const int dir = w.deltaY > 0 ? 1 : (w.deltaY < 0 ? -1 : 0);
    if (dir != 0) select (juce::jlimit (0, dubdrums::numDrummers - 1, index() + dir));
}

void DrummerSwitch::paint (juce::Graphics& g)
{
    static const char* first[] = { "STYLE", "SANTA", "CARLTON", "SLY", "DIGI" };
    static const char* last[]  = { "SCOTT", "DAVIS", "BARRETT", "DUNBAR", "KAL" };
    const auto c = centre();
    const float r = radius();
    const int sel = juce::jlimit (0, dubdrums::numDrummers - 1, index());
    const auto mouse = getMouseXYRelative().toFloat();

    // placa da chave: disco de alumínio escovado com as marcas das três posições
    const float plateR = r * 1.32f;
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillEllipse (juce::Rectangle<float> (plateR * 2, plateR * 2).withCentre (c).translated (1.5f, 3.0f));
    juce::ColourGradient al (juce::Colour (0xffe6e3dc), c.x - plateR, c.y - plateR, juce::Colour (0xff8d8a84), c.x + plateR, c.y + plateR, false);
    g.setGradientFill (al);
    g.fillEllipse (juce::Rectangle<float> (plateR * 2, plateR * 2).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.08f));
    for (int k = 0; k < 18; ++k)
    {
        const float rr = plateR * (0.40f + 0.033f * (float) k);
        g.drawEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (c), 0.6f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawEllipse (juce::Rectangle<float> (plateR * 2, plateR * 2).withCentre (c), 1.0f);

    for (int i = 0; i < dubdrums::numDrummers; ++i)
    {
        const float a = angleOf (i);
        const bool on = i == sel;
        // marca gravada na placa
        g.setColour (colours::ink.withAlpha (on ? 0.95f : 0.6f));
        g.drawLine (c.x + std::sin (a) * plateR * 0.80f, c.y - std::cos (a) * plateR * 0.80f,
                    c.x + std::sin (a) * plateR * 0.97f, c.y - std::cos (a) * plateR * 0.97f, on ? 2.6f : 1.6f);

        // lâmpada piloto e nome em cada posição
        const auto la = labelArea (i);
        const bool over = isMouseOver() && la.expanded (4.0f).contains (mouse);
        const auto lamp = juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ la.getCentreX(), la.getBottom() + 4.0f });
        if (on)
        {
            g.setColour (colours::amber.withAlpha (0.35f));
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

        g.setColour (on ? colours::ink : colours::inkSoft.withAlpha (over ? 1.0f : 0.75f));
        g.setFont (stencil (on ? 10.0f : 9.5f, 0.06f));
        g.drawText (first[i], la.withHeight (la.getHeight() * 0.5f), juce::Justification::centredBottom);
        g.drawText (last[i],  la.withTrimmedTop (la.getHeight() * 0.5f), juce::Justification::centredTop);
    }

    // knob "cabeça de galinha" de baquelite apontando para o baterista escolhido
    const float a = angleOf (sel);
    auto rot = juce::AffineTransform::rotation (a).translated (c.x, c.y);
    juce::Path head;
    head.addEllipse (-r * 0.60f, -r * 0.60f, r * 1.20f, r * 1.20f);
    juce::Path pointer;
    pointer.startNewSubPath (-r * 0.30f, -r * 0.40f);
    pointer.lineTo (-r * 0.11f, -r * 1.10f);
    pointer.quadraticTo (0.0f, -r * 1.20f, r * 0.11f, -r * 1.10f);
    pointer.lineTo (r * 0.30f, -r * 0.40f);
    pointer.closeSubPath();
    head.addPath (pointer);
    head.setUsingNonZeroWinding (true);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillPath (head, rot.translated (2.0f, 4.0f));
    juce::ColourGradient bak (juce::Colour (0xff45403a), c.x - r, c.y - r, juce::Colour (0xff060505), c.x + r * 0.6f, c.y + r, false);
    g.setGradientFill (bak);
    g.fillPath (head, rot);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.strokePath (head, juce::PathStrokeType (1.2f), rot);
    juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.18f), c.x, c.y - r, juce::Colours::transparentWhite, c.x, c.y + r * 0.2f, false);
    g.setGradientFill (gloss);
    g.fillEllipse (juce::Rectangle<float> (r * 0.8f, r * 0.8f).withCentre (c).translated (0, -r * 0.05f));

    // linha creme no ponteiro
    juce::Path line;
    line.addRoundedRectangle (-1.6f, -r * 1.04f, 3.2f, r * 0.62f, 1.6f);
    g.setColour (colours::cream);
    g.fillPath (line, rot);
    // parafuso central
    const auto sc = juce::Rectangle<float> (r * 0.16f, r * 0.16f).withCentre (c);
    juce::ColourGradient sg (juce::Colour (0xffe0ded8), sc.getX(), sc.getY(), juce::Colour (0xff5e5b55), sc.getRight(), sc.getBottom(), false);
    g.setGradientFill (sg);
    g.fillEllipse (sc);
}

//==============================================================================
// Alto-falante vintage (três desenhos de época), com desgaste: verniz gasto, pano amarelado, riscos e parafusos
namespace
{
    // pano de alto-falante: trama em diagonal, cor de tweed envelhecido
    void grilleCloth (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour base, float level)
    {
        juce::ColourGradient cg (base.brighter (0.15f), r.getCentreX(), r.getY(), base.darker (0.35f), r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRect (r);
        // o cone por trás do pano: aparece como uma sombra que "respira" com o som
        const float cr = juce::jmin (r.getWidth(), r.getHeight()) * (0.36f + 0.05f * level);
        juce::ColourGradient cone (juce::Colours::black.withAlpha (0.10f + 0.22f * level), r.getCentreX(), r.getCentreY(),
                                   juce::Colours::transparentBlack, r.getCentreX() + cr, r.getCentreY(), true);
        g.setGradientFill (cone);
        g.fillEllipse (juce::Rectangle<float> (cr * 2, cr * 2).withCentre (r.getCentre()));
        g.setColour (juce::Colours::black.withAlpha (0.20f + 0.18f * level));
        g.fillEllipse (juce::Rectangle<float> (cr * 0.42f, cr * 0.42f).withCentre (r.getCentre()));   // calota
        // trama
        g.setColour (base.brighter (0.45f).withAlpha (0.35f));
        for (float x = r.getX() - r.getHeight(); x < r.getRight(); x += 3.0f)
            g.drawLine (x, r.getBottom(), x + r.getHeight(), r.getY(), 0.6f);
        g.setColour (base.darker (0.6f).withAlpha (0.35f));
        for (float x = r.getX(); x < r.getRight() + r.getHeight(); x += 3.0f)
            g.drawLine (x, r.getBottom(), x - r.getHeight(), r.getY(), 0.6f);
        // manchas de tempo (amarelado / sujeira), sempre no mesmo lugar
        juce::Random rnd (1959);
        for (int i = 0; i < 9; ++i)
        {
            const float w = r.getWidth() * (0.15f + 0.25f * rnd.nextFloat());
            const auto spot = juce::Rectangle<float> (w, w * 0.7f).withCentre ({ r.getX() + r.getWidth() * rnd.nextFloat(), r.getY() + r.getHeight() * rnd.nextFloat() });
            g.setColour ((i % 2 ? juce::Colour (0xff6b4a12) : juce::Colours::black).withAlpha (0.07f));
            g.fillEllipse (spot);
        }
    }

    void screw (juce::Graphics& g, juce::Point<float> c, float r, float angle)
    {
        juce::ColourGradient sg (juce::Colour (0xffd8cfb8), c.x - r, c.y - r, juce::Colour (0xff4d4636), c.x + r, c.y + r, false);
        g.setGradientFill (sg);
        g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawLine (c.x - std::cos (angle) * r * 0.75f, c.y - std::sin (angle) * r * 0.75f,
                    c.x + std::cos (angle) * r * 0.75f, c.y + std::sin (angle) * r * 0.75f, 1.0f);
    }

    void scratches (juce::Graphics& g, juce::Rectangle<float> r, int seed)
    {
        juce::Random rnd (seed);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        for (int i = 0; i < 7; ++i)
        {
            const float x = r.getX() + r.getWidth() * rnd.nextFloat(), y = r.getY() + r.getHeight() * rnd.nextFloat();
            g.drawLine (x, y, x + 6.0f + 10.0f * rnd.nextFloat(), y + (rnd.nextFloat() - 0.5f) * 4.0f, 0.5f);
        }
    }
}

void VintageSpeaker::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    const float lv = juce::jlimit (0.0f, 1.0f, level);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (b.translated (1.5f, 2.5f), 7.0f);

    if (design == 0)
    {
        // 1959: gabinete de madeira de bateria eletrônica valvulada, tweed dourado, tweeter em cima do 12"
        juce::ColourGradient wood (juce::Colour (0xff7a4a22), b.getX(), b.getY(), juce::Colour (0xff3b2110), b.getRight(), b.getBottom(), false);
        g.setGradientFill (wood);
        g.fillRoundedRectangle (b, 7.0f);
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        for (float y = b.getY() + 2.0f; y < b.getBottom(); y += 2.6f)   // veio da madeira
            g.drawHorizontalLine ((int) y, b.getX() + 3.0f, b.getRight() - 3.0f);
        auto inner = b.reduced (5.0f);
        auto tweet = inner.removeFromTop (inner.getHeight() * 0.17f).withSizeKeepingCentre (inner.getWidth() * 0.40f, inner.getHeight() * 0.15f);
        inner.removeFromTop (3.0f);
        const float d = juce::jmin (inner.getWidth(), inner.getHeight());
        auto woofer = inner.withSizeKeepingCentre (d, d);
        for (auto [area, round] : { std::pair<juce::Rectangle<float>, bool> { woofer, true }, { tweet, false } })
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path hole;
            if (round) hole.addEllipse (area); else hole.addRoundedRectangle (area, area.getHeight() * 0.5f);
            g.reduceClipRegion (hole);
            grilleCloth (g, area, juce::Colour (0xffb48a42), round ? lv : lv * 0.6f);
        }
        g.setColour (juce::Colour (0xffc9a24a).withAlpha (0.9f));   // aro de latão gasto
        g.drawEllipse (woofer, 2.2f);
        g.drawRoundedRectangle (tweet, tweet.getHeight() * 0.5f, 1.6f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (woofer.expanded (1.6f), 0.8f);
        for (int k = 0; k < 4; ++k)
        {
            const float a = juce::MathConstants<float>::pi * (0.25f + 0.5f * (float) k);
            screw (g, woofer.getCentre() + juce::Point<float> (std::cos (a), std::sin (a)) * (d * 0.5f + 4.0f), 2.2f, 0.4f + (float) k);
        }
        scratches (g, b, 7);
    }
    else if (design == 1)
    {
        // anos 50: rádio de mesa de baquelite marfim, frisos cromados e o olho mágico verde
        juce::ColourGradient bak (juce::Colour (0xffeee3c6), b.getX(), b.getY(), juce::Colour (0xffb7a47a), b.getRight(), b.getBottom(), false);
        g.setGradientFill (bak);
        g.fillRoundedRectangle (b, 12.0f);
        g.setColour (juce::Colour (0xff8a6a2a).withAlpha (0.25f));   // amarelado do tempo nas bordas
        g.drawRoundedRectangle (b.reduced (1.5f), 11.0f, 3.0f);
        auto inner = b.reduced (8.0f);
        auto eye = inner.removeFromTop (16.0f).withSizeKeepingCentre (16.0f, 16.0f);
        inner.removeFromTop (3.0f);
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path hole; hole.addRoundedRectangle (inner, 6.0f);
            g.reduceClipRegion (hole);
            grilleCloth (g, inner, juce::Colour (0xff8a5a2a), lv);
        }
        // frisos cromados na frente do pano
        const int bars = 5;
        for (int i = 0; i < bars; ++i)
        {
            const float y = inner.getY() + inner.getHeight() * ((float) i + 0.5f) / (float) bars;
            auto bar = juce::Rectangle<float> (inner.getX() - 2.0f, y - 1.6f, inner.getWidth() + 4.0f, 3.2f);
            juce::ColourGradient chrome (juce::Colours::white, bar.getX(), bar.getY(), juce::Colour (0xff6d6a64), bar.getX(), bar.getBottom(), false);
            g.setGradientFill (chrome);
            g.fillRoundedRectangle (bar, 1.5f);
        }
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawRoundedRectangle (inner, 6.0f, 1.0f);
        // olho mágico (válvula 6E5): o leque verde fecha com o volume
        g.setColour (juce::Colour (0xff14180f));
        g.fillEllipse (eye.expanded (2.0f));
        juce::Path fan;
        const float open = juce::MathConstants<float>::pi * (0.9f - 0.75f * lv);
        fan.addPieSegment (eye, open * 0.5f, juce::MathConstants<float>::twoPi - open * 0.5f, 0.28f);
        g.setColour (juce::Colour (0xff6dff8a).withAlpha (0.55f + 0.4f * lv));
        g.fillPath (fan);
        g.setColour (juce::Colour (0xffc7c2b5));
        g.drawEllipse (eye.expanded (2.0f), 1.4f);
        scratches (g, b, 55);
    }
    else
    {
        // anos 60: rádio transistorizado, alumínio escovado perfurado e calota cromada
        juce::ColourGradient al (juce::Colour (0xffd9d6cf), b.getX(), b.getY(), juce::Colour (0xff8b8880), b.getRight(), b.getBottom(), false);
        g.setGradientFill (al);
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.06f));
        for (float y = b.getY() + 1.0f; y < b.getBottom(); y += 1.5f) g.drawHorizontalLine ((int) y, b.getX() + 2.0f, b.getRight() - 2.0f);
        const float d = juce::jmin (b.getWidth(), b.getHeight()) - 12.0f;
        auto disc = b.withSizeKeepingCentre (d, d);
        const auto c = disc.getCentre();
        // furos em anéis: o cone escuro mexendo atrás aparece mais forte nos furos do meio
        for (int ring = 0; ring < 6; ++ring)
        {
            const float rr = d * 0.5f * ((float) ring + 0.6f) / 6.2f;
            const int holes = ring == 0 ? 1 : ring * 8;
            for (int k = 0; k < holes; ++k)
            {
                const float a = juce::MathConstants<float>::twoPi * (float) k / (float) holes + (float) ring * 0.3f;
                const auto p = c + juce::Point<float> (std::cos (a), std::sin (a)) * (ring == 0 ? 0.0f : rr);
                const float pulse = lv * (1.0f - (float) ring / 6.0f);
                g.setColour (juce::Colour (0xff15120e).withAlpha (0.75f + 0.25f * pulse));
                g.fillEllipse (juce::Rectangle<float> (3.4f + 0.8f * pulse, 3.4f + 0.8f * pulse).withCentre (p));
            }
        }
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (disc.expanded (2.0f), 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawEllipse (disc.expanded (3.0f), 0.6f);
        for (auto p : { b.getTopLeft() + juce::Point<float> (6, 6), b.getTopRight() + juce::Point<float> (-6, 6),
                        b.getBottomLeft() + juce::Point<float> (6, -6), b.getBottomRight() + juce::Point<float> (-6, -6) })
            screw (g, p, 2.2f, p.x * 0.1f);
        scratches (g, b, 63);
    }
}

//==============================================================================
// GROOVES: visor azul de MPC (o mesmo visual dos presets do Riddim Machine)
namespace
{
    void blueLcd (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setColour (juce::Colour (0xff0a0c10));
        g.fillRoundedRectangle (r.expanded (3.0f), 5.0f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawRoundedRectangle (r.expanded (3.0f).translated (0, 1.0f), 5.0f, 1.0f);
        juce::ColourGradient gl (juce::Colour (0xff4a8cf0), r.getX(), r.getY(), juce::Colour (0xff1d4aa8), r.getX(), r.getBottom(), false);
        gl.addColour (0.55, juce::Colour (0xff2e66cc));
        g.setGradientFill (gl);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.07f));               // grade de pixels do LCD
        for (float y = r.getY() + 2.0f; y < r.getBottom() - 1.0f; y += 3.0f)
            g.drawHorizontalLine ((int) y, r.getX() + 2.0f, r.getRight() - 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    }

    void blueText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, float size, float alpha = 1.0f)
    {
        g.setFont (stencil (size, 0.06f));
        g.setColour (juce::Colours::white.withAlpha (0.22f * alpha));
        for (auto o : { juce::Point<float> (-0.8f, 0), { 0.8f, 0 }, { 0, -0.8f }, { 0, 0.8f } })
            g.drawFittedText (t, r.translated (o.x, o.y).toNearestInt(), j, 1, 0.75f);
        g.setColour (juce::Colour (0xffe8f3ff).withAlpha (alpha));
        g.drawFittedText (t, r.toNearestInt(), j, 1, 0.75f);
    }

    juce::Array<int> groovesOf (int drummer)
    {
        juce::Array<int> list;
        for (int i = 0; i < dubdrums::numGroovePresets; ++i)
            if (dubdrums::groovePresets[i].drummer == drummer) list.add (i);
        return list;
    }

    juce::String grooveLine2 (const dubdrums::GroovePreset& gp)
    {
        return juce::String (gp.artist).toUpperCase() + "  " + juce::String (gp.year) + "  -  "
             + (gp.approx ? "~" : "") + juce::String (gp.bpm) + " BPM";
    }
}

GrooveSelector::GrooveSelector (DubDrumsProcessor& p) : proc (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setRepaintsOnMouseActivity (true);
    startTimerHz (8);
}

GrooveSelector::~GrooveSelector() { stopTimer(); }

int GrooveSelector::drummer() const
{
    return juce::jlimit (0, dubdrums::numDrummers - 1, (int) proc.apvts.getRawParameterValue ("drummer")->load());
}

juce::Rectangle<float> GrooveSelector::keyRect (int side) const
{
    auto r = getLocalBounds().toFloat().reduced (1.0f, 4.0f);
    return side < 0 ? r.removeFromLeft (26.0f) : r.removeFromRight (26.0f);
}

juce::Rectangle<float> GrooveSelector::lcdRect() const
{
    return getLocalBounds().toFloat().reduced (31.0f, 4.0f);
}

void GrooveSelector::timerCallback()
{
    const juce::String key = juce::String (drummer()) + ":" + juce::String (proc.getGroove()) + ":" + (proc.grooveIntact() ? "1" : "0");
    if (key != shownKey) { shownKey = key; repaint(); if (auto* parent = getParentComponent()) parent->repaint (getBounds().withHeight (getHeight() + 22)); }
}

void GrooveSelector::paint (juce::Graphics& g)
{
    // teclas pretas ◀ ▶ com a luz azul embaixo
    for (int side : { -1, 1 })
    {
        const auto k = keyRect (side);
        const bool down = pressed == side;
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (k.translated (0, 2.0f), 3.0f);
        auto cap = k.translated (0, down ? 1.5f : 0.0f);
        juce::ColourGradient cg (juce::Colour (0xff2c2925), cap.getX(), cap.getY(), juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (juce::Colour (0xff4a8cf0).withAlpha (down ? 0.9f : 0.35f));
        g.fillRoundedRectangle (cap.withTop (cap.getBottom() - 3.0f).reduced (4.0f, 0.0f).translated (0, -1.5f), 1.0f);
        juce::Path tri;
        const auto c = cap.withTrimmedBottom (3.0f).getCentre();
        if (side < 0) tri.addTriangle (c.x - 5.0f, c.y, c.x + 4.0f, c.y - 5.5f, c.x + 4.0f, c.y + 5.5f);
        else          tri.addTriangle (c.x + 5.0f, c.y, c.x - 4.0f, c.y - 5.5f, c.x - 4.0f, c.y + 5.5f);
        g.setColour (colours::cream.withAlpha (isMouseOver() ? 0.95f : 0.75f));
        g.fillPath (tri);
    }

    const auto r = lcdRect();
    blueLcd (g, r);
    auto t = r.reduced (7.0f, 2.0f);
    const int d = drummer(), gr = proc.getGroove();
    const auto list = groovesOf (d);
    if (gr >= 0 && dubdrums::groovePresets[gr].drummer == d)
    {
        const auto& gp = dubdrums::groovePresets[gr];
        const bool intact = proc.grooveIntact();
        // título não invade o contador n/N do canto
        blueText (g, juce::String (gp.song).toUpperCase() + (intact ? "" : " *"),
                  t.removeFromTop (t.getHeight() * 0.55f).withTrimmedRight (30.0f),
                  juce::Justification::centredLeft, 13.0f);
        blueText (g, grooveLine2 (gp), t, juce::Justification::centredLeft, 9.0f, 0.8f);
        blueText (g, juce::String (list.indexOf (gr) + 1) + "/" + juce::String (list.size()), r.reduced (7.0f, 3.0f).removeFromTop (14.0f),
                  juce::Justification::topRight, 8.5f, 0.6f);
    }
    else
    {
        blueText (g, "GROOVES  -  " + juce::String (list.size()) + " MUSICAS", t.removeFromTop (t.getHeight() * 0.55f),
                  juce::Justification::centredLeft, 12.0f);
        blueText (g, "ESCOLHA COM  < >  OU CLIQUE AQUI", t, juce::Justification::centredLeft, 9.0f, 0.75f);
    }
    // brilho do vidro
    juce::ColourGradient sh (juce::Colours::white.withAlpha (0.16f), r.getX(), r.getY(),
                             juce::Colours::transparentWhite, r.getX(), r.getY() + r.getHeight() * 0.45f, false);
    g.setGradientFill (sh);
    g.fillRoundedRectangle (r, 3.0f);
}

void GrooveSelector::step (int delta)
{
    const auto list = groovesOf (drummer());
    if (list.isEmpty()) return;
    const int at = list.indexOf (proc.getGroove());
    const int next = at < 0 ? (delta > 0 ? 0 : list.size() - 1) : (at + delta + list.size()) % list.size();
    proc.loadGroove (list[next]);
    repaint();
}

void GrooveSelector::showList()
{
    juce::PopupMenu menu;
    static const char* names[] = { "STYLE SCOTT", "SANTA DAVIS", "CARLTON BARRETT", "SLY DUNBAR", "DIGIKAL (MAQUINAS)" };
    const int current = proc.getGroove();
    for (int d = 0; d < dubdrums::numDrummers; ++d)
    {
        juce::PopupMenu sub;
        for (int i : groovesOf (d))
        {
            const auto& gp = dubdrums::groovePresets[i];
            sub.addItem (i + 1, juce::String (gp.song) + "  -  " + gp.artist + " " + juce::String (gp.year)
                                    + "  (" + (gp.approx ? "~" : "") + juce::String (gp.bpm) + " BPM)", true, i == current);
        }
        menu.addSubMenu (names[d], sub, true, nullptr, current >= 0 && dubdrums::groovePresets[current].drummer == d);
    }
    juce::Component::SafePointer<GrooveSelector> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int result)
    {
        if (safe != nullptr && result > 0)
        {
            safe->proc.loadGroove (result - 1);
            safe->repaint();
        }
    });
}

void GrooveSelector::mouseDown (const juce::MouseEvent& e)
{
    if (keyRect (-1).contains (e.position))     { pressed = -1; step (-1); }
    else if (keyRect (1).contains (e.position)) { pressed = 1;  step (1); }
    else if (lcdRect().contains (e.position))   showList();
    repaint();
}

void GrooveSelector::mouseUp (const juce::MouseEvent&)
{
    pressed = 0;
    repaint();
}
