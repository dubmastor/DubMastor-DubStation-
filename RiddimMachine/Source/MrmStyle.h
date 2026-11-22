// Visual DubStation do MRM-0420: painel creme envelhecido, laterais de madeira, plaquetas pretas,
// displays âmbar e lâmpadas. Funções de desenho usadas pelo cabeçalho e pela aba RIDDIM.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace mrmui
{
namespace col
{
    const juce::Colour plate     { 0xffe4d8bc };
    const juce::Colour plateDark { 0xffc9b994 };
    const juce::Colour ink       { 0xff2b241d };
    const juce::Colour inkSoft   { 0xff5c5144 };
    const juce::Colour labelBg   { 0xff1a1714 };
    const juce::Colour cream     { 0xffefe5cc };
    const juce::Colour jamGreen  { 0xff0b8a3c };
    const juce::Colour jamGold   { 0xffeabd12 };
    const juce::Colour jamRed    { 0xffd8442c };
    const juce::Colour amber     { 0xffffa928 };
    const juce::Colour glass     { 0xff17110a };
    const juce::Colour woodDark  { 0xff2e1a0d };
    const juce::Colour woodLight { 0xff6b4423 };
}

inline juce::Font stencil (float size, float kerning = 0.08f)
{
    return juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (kerning);
}

inline juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

inline float textWidth (const juce::Font& f, const juce::String& t) { return juce::GlyphArrangement::getStringWidth (f, t); }

inline void lamp (juce::Graphics& g, juce::Rectangle<float> r, bool on, juce::Colour litColour = col::amber)
{
    if (on)
    {
        g.setColour (litColour.withAlpha (0.32f));
        g.fillEllipse (r.expanded (5.0f));
        juce::ColourGradient lit (litColour.brighter (0.9f), r.getCentreX(), r.getCentreY(),
                                  litColour.darker (0.2f), r.getRight(), r.getBottom(), true);
        g.setGradientFill (lit);
    }
    else
        g.setColour (juce::Colour (0xff4a3a22));
    g.fillEllipse (r);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (r, 1.0f);
}

// vidro escuro de display (âmbar)
inline void displayGlass (juce::Graphics& g, juce::Rectangle<float> r, float corner = 3.0f, float lift = 0.15f)
{
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r.expanded (2.0f), corner + 1.0f);
    juce::ColourGradient gl (col::glass.brighter (lift), r.getX(), r.getY(), col::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r, corner);
}

inline void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j,
                      juce::Colour c = col::amber, float alpha = 1.0f)
{
    g.setColour (c.withAlpha (0.18f * alpha));
    for (auto o : { juce::Point<float> (-1, 0), { 1, 0 }, { 0, -1 }, { 0, 1 } })
        g.drawText (t, r.translated (o.x, o.y), j);
    g.setColour (c.withAlpha (alpha));
    g.drawText (t, r, j);
}

inline void paintWood (juce::Graphics& g, juce::Rectangle<int> wood, juce::Random& rng)
{
    const float wx = (float) wood.getX(), ww = (float) wood.getWidth();
    juce::ColourGradient wg (col::woodLight.darker (0.1f), wx, 0, col::woodDark, wx + ww, 0, false);
    wg.addColour (0.35, col::woodLight.darker (0.25f));
    wg.addColour (0.7,  col::woodLight.darker (0.45f));
    g.setGradientFill (wg);
    g.fillRect (wood);
    for (int i = 0; i < 40; ++i)
    {
        juce::Path grain;
        const float xx = wx + rng.nextFloat() * ww, ph = rng.nextFloat() * 6.0f, amp = 0.8f + rng.nextFloat() * 2.2f;
        grain.startNewSubPath (xx, (float) wood.getY());
        for (int yy = wood.getY(); yy <= wood.getBottom(); yy += 14)
            grain.lineTo (xx + std::sin ((float) yy * 0.011f + ph) * amp, (float) yy);
        g.setColour ((i % 3 ? juce::Colours::black : col::woodLight.brighter (0.3f)).withAlpha (0.10f + rng.nextFloat() * 0.16f));
        g.strokePath (grain, juce::PathStrokeType (0.5f + rng.nextFloat() * 1.5f));
    }
    juce::ColourGradient edge (juce::Colours::white.withAlpha (0.10f), wx, 0, juce::Colours::transparentWhite, wx + 4.0f, 0, false);
    g.setGradientFill (edge);
    g.fillRect (wood.withWidth (4));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRect (wood, 1);
}

// metal creme envelhecido (manchas, poeira, riscos); topGrime = sujeira na borda de cima
inline void paintPlate (juce::Graphics& g, juce::Rectangle<int> plate, juce::Random& rng, bool topGrime, bool bottomGrime)
{
    juce::ColourGradient pg (col::plate.brighter (0.03f), (float) plate.getX(), (float) plate.getY(),
                             col::plateDark.darker (0.02f), (float) plate.getRight(), (float) plate.getBottom() + 300.0f, false);
    pg.addColour (0.5, col::plate.interpolatedWith (juce::Colour (0xffd9c48f), 0.35f));
    g.setGradientFill (pg);
    g.fillRect (plate);

    auto randIn = [&] (juce::Rectangle<int> r)
    { return juce::Point<float> ((float) r.getX() + rng.nextFloat() * (float) r.getWidth(), (float) r.getY() + rng.nextFloat() * (float) r.getHeight()); };
    const float area = (float) plate.getWidth() * (float) plate.getHeight() / (1224.0f * 846.0f);

    for (int i = 0; i < (int) (14 * area) + 4; ++i)
    {
        const float rr = 60.0f + rng.nextFloat() * 200.0f;
        const auto pc = randIn (plate);
        juce::ColourGradient y (juce::Colour (0xffc9a64e).withAlpha (0.07f + rng.nextFloat() * 0.05f), pc.x, pc.y,
                                juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
        g.setGradientFill (y);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 1.4f).withCentre (pc));
    }
    for (int i = 0; i < (int) (60000 * area); ++i)
    {
        const auto pt = randIn (plate);
        g.setColour ((rng.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.03f + rng.nextFloat() * 0.07f));
        g.fillRect (pt.x, pt.y, 1.0f, 1.0f);
    }
    for (int i = 0; i < (int) (30 * area) + 4; ++i)
    {
        const float rr = 10.0f + rng.nextFloat() * 70.0f;
        const auto pc = randIn (plate);
        juce::ColourGradient stain (juce::Colour (0xff7a5a2c).withAlpha (0.05f + rng.nextFloat() * 0.06f), pc.x, pc.y,
                                    juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
        g.setGradientFill (stain);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (pc));
    }
    for (int i = 0; i < (int) (150 * area) + 10; ++i)
    {
        const auto p0 = randIn (plate);
        const float ang = rng.nextFloat() * 3.14159f, len = 4.0f + rng.nextFloat() * 38.0f;
        g.setColour ((rng.nextFloat() < 0.6f ? juce::Colours::white : juce::Colour (0xff4a3a24)).withAlpha (0.10f + rng.nextFloat() * 0.18f));
        g.drawLine (p0.x, p0.y, p0.x + std::cos (ang) * len, p0.y + std::sin (ang) * len, 0.5f);
    }
    const auto pf = plate.toFloat();
    const float d = 34.0f;
    const juce::Colour grime (0xff3d2b14);
    auto band = [&] (juce::Rectangle<float> r, juce::Point<float> from, juce::Point<float> to)
    {
        juce::ColourGradient eg (grime.withAlpha (0.30f), from.x, from.y, grime.withAlpha (0.0f), to.x, to.y, false);
        g.setGradientFill (eg);
        g.fillRect (r);
    };
    band (pf.withWidth (d),                { pf.getX(), 0 },     { pf.getX() + d, 0 });
    band (pf.withLeft (pf.getRight() - d), { pf.getRight(), 0 }, { pf.getRight() - d, 0 });
    if (topGrime)    band (pf.withHeight (d), { 0, pf.getY() }, { 0, pf.getY() + d });
    if (bottomGrime) band (pf.withTop (pf.getBottom() - d), { 0, pf.getBottom() }, { 0, pf.getBottom() - d });
}

inline void screw (juce::Graphics& g, float sx, float sy, juce::Random& rng)
{
    const auto r = juce::Rectangle<float> (11, 11).withCentre ({ sx, sy });
    juce::ColourGradient sg (juce::Colour (0xffe0ded8), r.getX(), r.getY(), juce::Colour (0xff5e5b55), r.getRight(), r.getBottom(), false);
    g.setGradientFill (sg);
    g.fillEllipse (r);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (r, 0.8f);
    const float a = rng.nextFloat() * 3.14f;
    g.drawLine (sx - std::cos (a) * 4.0f, sy - std::sin (a) * 4.0f, sx + std::cos (a) * 4.0f, sy + std::sin (a) * 4.0f, 1.4f);
}

// seção com moldura gravada e plaqueta preta com o nome (igual aos painéis das máquinas)
inline void section (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& t)
{
    auto rf = r.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawRoundedRectangle (rf.translated (1, 1), 4.0f, 1.0f);
    g.setColour (col::ink.withAlpha (0.55f));
    g.drawRoundedRectangle (rf, 4.0f, 1.2f);
    g.setFont (stencil (11.5f, 0.22f));
    const float tw = textWidth (g.getCurrentFont(), t) + 22.0f;
    auto tag = juce::Rectangle<float> (rf.getX() + 12, rf.getY() - 9, tw, 18);
    g.setColour (col::labelBg);
    g.fillRoundedRectangle (tag, 2.0f);
    g.setColour (col::cream);
    g.drawText (t, tag, juce::Justification::centred);
}

// texto gravado no painel (sombra clara embaixo)
inline void engraved (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j,
                      float size, float kerning = 0.25f, juce::Colour c = col::inkSoft)
{
    g.setFont (stencil (size, kerning));
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.drawText (t, r.translated (0.6f, 0.8f), j);
    g.setColour (c);
    g.drawText (t, r, j);
}

// placa de latão escovado com o texto gravado
inline void brassPlate (juce::Graphics& g, juce::Rectangle<float> pl, const juce::String& t, float fontSize, juce::Random& rng, bool rivets = true)
{
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.fillRoundedRectangle (pl.translated (2.0f, 3.5f), 4.0f);
    juce::ColourGradient brass (juce::Colour (0xffe8d390), pl.getX(), pl.getY(), juce::Colour (0xff86692a), pl.getX(), pl.getBottom(), false);
    brass.addColour (0.45, juce::Colour (0xffcaab5c));
    brass.addColour (0.55, juce::Colour (0xffb39446));
    g.setGradientFill (brass);
    g.fillRoundedRectangle (pl, 4.0f);
    for (int i = 0; i < (int) (pl.getWidth() * 0.4f); ++i)
    {
        const float yy = pl.getY() + 1.0f + rng.nextFloat() * (pl.getHeight() - 2.0f);
        const float x0 = pl.getX() + rng.nextFloat() * pl.getWidth() * 0.5f;
        g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colour (0xff5a4316)).withAlpha (0.06f + rng.nextFloat() * 0.08f));
        g.drawHorizontalLine ((int) yy, x0, juce::jmin (pl.getRight() - 1.0f, x0 + 20.0f + rng.nextFloat() * pl.getWidth() * 0.6f));
    }
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.drawLine (pl.getX() + 3, pl.getY() + 1, pl.getRight() - 3, pl.getY() + 1, 1.0f);
    g.setColour (juce::Colour (0xff3d2c0c).withAlpha (0.7f));
    g.drawLine (pl.getX() + 3, pl.getBottom() - 1, pl.getRight() - 3, pl.getBottom() - 1, 1.2f);
    g.drawRoundedRectangle (pl, 4.0f, 0.9f);
    auto inner = rivets ? pl.reduced (30.0f, 7.0f) : pl.reduced (6.0f, 4.0f);
    if (rivets)
    {
        g.setColour (juce::Colour (0xff4a3712).withAlpha (0.6f));
        g.drawRoundedRectangle (inner, 2.5f, 1.0f);
        for (float rx : { pl.getX() + 15.0f, pl.getRight() - 15.0f })
        {
            const auto rv = juce::Rectangle<float> (11, 11).withCentre ({ rx, pl.getCentreY() });
            juce::ColourGradient rg (juce::Colour (0xfff0dfa6), rv.getX(), rv.getY(), juce::Colour (0xff6b5220), rv.getRight(), rv.getBottom(), false);
            g.setGradientFill (rg);
            g.fillEllipse (rv);
            g.setColour (juce::Colour (0xff3d2c0c).withAlpha (0.6f));
            g.drawLine (rv.getX() + 2.5f, rv.getCentreY(), rv.getRight() - 2.5f, rv.getCentreY(), 1.0f);
        }
    }
    g.setFont (stencil (fontSize, 0.3f));
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawText (t, inner.translated (1.0f, 1.2f), juce::Justification::centred);
    g.setColour (juce::Colour (0xff2a1d06));
    g.drawText (t, inner, juce::Justification::centred);
}
} // namespace mrmui
