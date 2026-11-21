#include "PluginEditor.h"
#include <tuple>

#ifndef JucePlugin_VersionString
 #define JucePlugin_VersionString "2.3.0"
#endif
static juce::String versionShort() { return juce::String (JucePlugin_VersionString).upToLastOccurrenceOf (".", false, false); }

// Visual da família DubMastor: painel de metal creme envelhecido, laterais de madeira, knobs de
// baquelite com saia de alumínio, lâmpadas piloto e displays âmbar (igual ao Big Knob e ao Dub Chords).
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
    const juce::Colour phosphor  { 0xff5dff8a };
}

static constexpr float kStartAngle = juce::MathConstants<float>::pi * 1.2f;
static constexpr float kEndAngle   = juce::MathConstants<float>::pi * 2.8f;
// chaves seletoras (estilo e timbre): curso menor, como uma chave rotativa de verdade
static constexpr float kSwStart = juce::MathConstants<float>::pi * 1.5f;
static constexpr float kSwEnd   = juce::MathConstants<float>::pi * 2.5f;

static juce::Font stencil (float size, float kerning = 0.08f)
{
    return juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (kerning);
}

static bool isSwitch (const juce::Slider& s) { return s.getName() == "big" || s.getName() == "switch"; }
static float knobReduction (const juce::Slider& s)
{
    if (s.getName() == "big")    return 30.0f;
    if (s.getName() == "switch") return 24.0f;
    return 12.0f;
}

// área circular do knob dentro do slider (mesma conta do LookAndFeel)
static juce::Rectangle<float> knobCircle (const juce::Slider& s)
{
    auto r = s.getBounds().toFloat();
    if (s.getTextBoxPosition() == juce::Slider::TextBoxBelow) r.removeFromBottom ((float) s.getTextBoxHeight());
    r = r.reduced (knobReduction (s));
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    return r.withSizeKeepingCentre (d, d);
}

static void lampAt (juce::Graphics& g, juce::Rectangle<float> lamp, bool on, juce::Colour c = colours::amber)
{
    if (on)
    {
        g.setColour (c.withAlpha (0.30f));
        g.fillEllipse (lamp.expanded (lamp.getWidth() * 0.5f));
        juce::ColourGradient lit (c.brighter (0.8f), lamp.getCentreX(), lamp.getCentreY(),
                                  c.darker (0.2f), lamp.getRight(), lamp.getBottom(), true);
        g.setGradientFill (lit);
    }
    else
        g.setColour (juce::Colour (0xff4a3a22));
    g.fillEllipse (lamp);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (lamp, 1.0f);
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

static void glassBox (juce::Graphics& g, juce::Rectangle<float> r, float brighten = 0.15f)
{
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r.expanded (3), 4.0f);
    juce::ColourGradient gl (colours::glass.brighter (brighten), r.getX(), r.getY(), colours::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r, 3.0f);
}

//==============================================================================
BassLookAndFeel::BassLookAndFeel()
{
    setColour (juce::Label::textColourId, colours::ink);
    setColour (juce::Slider::textBoxTextColourId, colours::inkSoft);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void BassLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    const bool isBig = isSwitch (s);
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
void BassLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool down)
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

    lampAt (g, juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ r.getX() + 14.0f, r.getCentreY() }), on);

    g.setColour (colours::cream.withAlpha (on ? 1.0f : 0.65f));
    g.setFont (stencil (11.0f, 0.08f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (22), juce::Justification::centred);
}

// EXPORTAR / NOVA IDEIA: tampa verde grande de botão de pedal (igual ao THROW do Big Knob)
void BassLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (r, 6.0f);
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

void BassLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setColour (colours::jamGold);
    g.setFont (stencil (14.0f, 0.16f));
    g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, down ? 2 : 1), juce::Justification::centred);
}

//==============================================================================
// Teclado mecânico
static const int kWhite[7] = { 0, 2, 4, 5, 7, 9, 11 };
static const int kBlackAfter[5][2] = { { 1, 0 }, { 3, 1 }, { 6, 3 }, { 8, 4 }, { 10, 5 } };   // nota, tecla branca à esquerda

static bool isBlack (int n) { return n == 1 || n == 3 || n == 6 || n == 8 || n == 10; }

juce::Rectangle<float> NoteKeys::keyRect (int note) const
{
    const auto b = getLocalBounds().toFloat();
    const float ww = b.getWidth() / 7.0f;
    for (int i = 0; i < 7; ++i)
        if (kWhite[i] == note) return { b.getX() + ww * (float) i, b.getY(), ww, b.getHeight() };
    for (auto& bk : kBlackAfter)
        if (bk[0] == note)
        {
            const float bw = ww * 0.62f;
            return { b.getX() + ww * (float) (bk[1] + 1) - bw * 0.5f, b.getY(), bw, b.getHeight() * 0.60f };
        }
    return {};
}

int NoteKeys::keyAt (juce::Point<float> p) const
{
    for (auto& bk : kBlackAfter)
        if (keyRect (bk[0]).contains (p)) return bk[0];
    for (int n : kWhite)
        if (keyRect (n).contains (p)) return n;
    return -1;
}

void NoteKeys::mouseDown (const juce::MouseEvent& e)
{
    const int k = keyAt (e.position);
    if (k >= 0) select (k);
}

void NoteKeys::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const int active = index();

    g.setColour (juce::Colour (0xff0e0c0a));
    g.fillRoundedRectangle (b, 3.0f);

    auto drawKey = [&] (int n)
    {
        const bool black = isBlack (n), on = n == active, over = n == hover;
        auto r = keyRect (n).reduced (black ? 0.0f : 1.0f, 0.0f);
        r = r.withTrimmedTop (on ? 3.0f : 0.0f);               // tecla afundada
        const float rad = 3.0f;

        if (! black)
        {
            juce::ColourGradient iv (juce::Colour (on ? 0xffe9d9ae : 0xfff6ecd2).brighter (over && ! on ? 0.04f : 0.0f), r.getX(), r.getY(),
                                     juce::Colour (on ? 0xffc2ad78 : 0xffd9c69a), r.getX(), r.getBottom(), false);
            g.setGradientFill (iv);
            g.fillRoundedRectangle (r, rad);
            g.setColour (juce::Colour (0xff3d3124).withAlpha (0.8f));
            g.drawRoundedRectangle (r, rad, 1.0f);
            if (! on)
            {
                g.setColour (juce::Colour (0xffb39d6a));
                g.fillRect (r.withTop (r.getBottom() - 5.0f).reduced (1.0f, 0.0f));
            }
            else
            {
                g.setColour (colours::amber.withAlpha (0.18f));
                g.fillRoundedRectangle (r, rad);
            }
            g.setColour (on ? juce::Colour (0xff6b3f05) : colours::inkSoft);
            g.setFont (stencil (9.5f, 0.0f));
            g.drawText (mbm::rootName (n), r.withTrimmedBottom (9.0f).withTop (r.getBottom() - 28.0f), juce::Justification::centred);
        }
        else
        {
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillRoundedRectangle (r.translated (1.5f, 2.0f), rad);
            juce::ColourGradient bk (juce::Colour (on ? 0xff2a2520 : 0xff3d3833).brighter (over && ! on ? 0.12f : 0.0f), r.getX(), r.getY(),
                                     juce::Colour (0xff070605), r.getRight(), r.getBottom(), false);
            g.setGradientFill (bk);
            g.fillRoundedRectangle (r, rad);
            g.setColour (juce::Colours::white.withAlpha (on ? 0.05f : 0.14f));
            g.fillRoundedRectangle (r.reduced (3.0f, 2.0f).withHeight (r.getHeight() * 0.55f), 2.0f);
        }

        if (on)
            lampAt (g, juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ r.getCentreX(), r.getBottom() - (black ? 6.0f : 8.0f) }), true);
    };

    for (int n : kWhite) drawKey (n);
    for (auto& bk : kBlackAfter) drawKey (bk[0]);

    auto bar = b.withHeight (6.0f);                                   // barra cromada + feltro
    juce::ColourGradient chrome (juce::Colour (0xffd8d6d0), bar.getX(), bar.getY(), juce::Colour (0xff5e5b55), bar.getX(), bar.getBottom(), false);
    g.setGradientFill (chrome);
    g.fillRect (bar);
    g.setColour (juce::Colour (0xff5a1414));
    g.fillRect (b.withY (bar.getBottom()).withHeight (2.0f));
}

//==============================================================================
void QualitySwitch::mouseDown (const juce::MouseEvent& e)
{
    const int i = juce::jlimit (0, mbm::NumQualities - 1, (int) (e.position.x / (float) getWidth() * (float) mbm::NumQualities));
    select (i);
}

void QualitySwitch::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);

    static const char* labels[mbm::NumQualities] = { "MAIOR", "MENOR", "7", "m7" };
    auto inner = b.reduced (3.0f);
    const float w = inner.getWidth() / (float) mbm::NumQualities;
    const auto mouse = getMouseXYRelative().toFloat();
    for (int i = 0; i < mbm::NumQualities; ++i)
    {
        auto r = juce::Rectangle<float> (inner.getX() + w * (float) i, inner.getY(), w, inner.getHeight()).reduced (1.5f, 0.0f).withTrimmedBottom (2.5f);
        const bool on = index() == i, over = isMouseOver() && r.contains (mouse);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (r.translated (0, 2.5f), 3.0f);
        auto cap = r.translated (0, on ? 2.0f : 0.0f);
        juce::ColourGradient cg (juce::Colour (on ? 0xff2c2925 : 0xff3a3632).brighter (over && ! on ? 0.08f : 0.0f), cap.getX(), cap.getY(),
                                 juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (on ? 0.04f : 0.12f));
        g.drawHorizontalLine ((int) cap.getY() + 1, cap.getX() + 3, cap.getRight() - 3);
        lampAt (g, juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ cap.getCentreX(), cap.getY() + 8.0f }), on);
        g.setColour (on ? colours::cream : colours::cream.withAlpha (0.5f));
        g.setFont (stencil (i < 2 ? 9.0f : 11.0f, 0.08f));
        g.drawText (labels[i], cap.withTrimmedTop (13.0f), juce::Justification::centred);
    }
}

//==============================================================================
int CounterWindow::minIndex() const { return kind == Idea ? 1 : 0; }
int CounterWindow::maxIndex() const { return kind == Idea ? 999 : mbm::numDurations - 1; }

void CounterWindow::mouseDown (const juce::MouseEvent& e)
{
    const int dir = e.position.x < (float) getWidth() * 0.5f ? -1 : 1;
    const int step = (kind == Idea && e.mods.isShiftDown()) ? 10 : 1;
    select (juce::jlimit (minIndex(), maxIndex(), index() + dir * step));
}

void CounterWindow::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const int i = juce::jlimit (minIndex(), maxIndex(), index());

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (b.translated (0, 2.0f), 5.0f);
    juce::ColourGradient frame (juce::Colour (0xff3a3632), b.getX(), b.getY(), juce::Colour (0xff141210), b.getX(), b.getBottom(), false);
    g.setGradientFill (frame);
    g.fillRoundedRectangle (b, 5.0f);

    auto arrow = [&] (float cx, int dir)
    {
        const bool lit = side == dir;
        const bool possible = (dir < 0 && i > minIndex()) || (dir > 0 && i < maxIndex());
        juce::Path p;
        const float cy = b.getCentreY(), s = 6.0f;
        if (dir < 0) p.addTriangle (cx + s * 0.6f, cy - s, cx + s * 0.6f, cy + s, cx - s * 0.8f, cy);
        else         p.addTriangle (cx - s * 0.6f, cy - s, cx - s * 0.6f, cy + s, cx + s * 0.8f, cy);
        g.setColour (! possible ? colours::cream.withAlpha (0.15f) : lit ? colours::amber : colours::cream.withAlpha (0.6f));
        g.fillPath (p);
    };
    arrow (b.getX() + 14.0f, -1);
    arrow (b.getRight() - 14.0f, 1);

    // janela do contador: tambor creme
    auto win = b.reduced (28.0f, 6.0f);
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (win.expanded (2.0f), 3.0f);
    juce::ColourGradient drum (juce::Colour (0xff8f8264), win.getX(), win.getY(), juce::Colour (0xff8f8264), win.getX(), win.getBottom(), false);
    drum.addColour (0.25, juce::Colour (0xffefe5cc));
    drum.addColour (0.55, juce::Colour (0xfff6ecd6));
    drum.addColour (0.85, juce::Colour (0xffcbbd98));
    g.setGradientFill (drum);
    g.fillRoundedRectangle (win, 2.0f);

    juce::String num, unit;
    if (kind == Idea)
    {
        num = juce::String (i).paddedLeft ('0', 3);
        unit = "IDEIA";
    }
    else
    {
        static const char* nums[mbm::numDurations] = { "1/2", "1", "2", "3", "4", "8" };
        num = nums[i];
        unit = i <= 1 ? "COMP." : "COMPS.";
    }
    const float big = kind == Idea ? 22.0f : 19.0f;
    g.setColour (colours::ink);
    g.setFont (stencil (big, 0.06f));
    const float nw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), num);
    g.setFont (stencil (9.5f, 0.2f));
    const float uw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), unit);
    auto line = win.withSizeKeepingCentre (nw + 7.0f + uw, win.getHeight());
    if (kind == Idea)
    {
        g.drawText (unit, line.removeFromLeft (uw), juce::Justification::centredLeft);
        line.removeFromLeft (7.0f);
        g.setFont (stencil (big, 0.06f));
        g.drawText (num, line, juce::Justification::centredLeft);
    }
    else
    {
        g.setFont (stencil (big, 0.06f));
        g.drawText (num, line.removeFromLeft (nw), juce::Justification::centredLeft);
        line.removeFromLeft (7.0f);
        g.setFont (stencil (9.5f, 0.2f));
        g.drawText (unit, line, juce::Justification::centredLeft);
    }

    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillRoundedRectangle (win.withHeight (win.getHeight() * 0.3f).reduced (2.0f, 1.0f), 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (win, 2.0f, 1.0f);
}

//==============================================================================
juce::Rectangle<float> InterlockKeys::keyRect (int i) const
{
    auto b = getLocalBounds().toFloat().reduced (4.0f);
    const int n = juce::jmax (1, labels.size());
    if (vertical)
    {
        const float h = b.getHeight() / (float) n;
        return { b.getX(), b.getY() + h * (float) i, b.getWidth(), h };
    }
    const float w = b.getWidth() / (float) n;
    return { b.getX() + w * (float) i, b.getY(), w, b.getHeight() };
}

void InterlockKeys::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < labels.size(); ++i)
        if (keyRect (i).contains (e.position)) { select (i); return; }
}

void InterlockKeys::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.35f));                // moldura de metal escuro
    g.fillRoundedRectangle (b.translated (0, 2.0f), 5.0f);
    juce::ColourGradient frame (juce::Colour (0xff3a3632), b.getX(), b.getY(), juce::Colour (0xff141210), b.getX(), b.getBottom(), false);
    g.setGradientFill (frame);
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);

    const auto mouse = getMouseXYRelative().toFloat();
    for (int i = 0; i < labels.size(); ++i)
    {
        const bool on = index() == i;
        const bool over = isMouseOver() && keyRect (i).contains (mouse);
        auto r = keyRect (i).reduced (vertical ? 0.0f : 1.5f, vertical ? 1.2f : 0.0f).withTrimmedBottom (vertical ? 1.5f : 2.5f);
        // vão entre as teclas
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0, 2.0f), 3.0f);
        // tecla de baquelite: afunda quando travada
        auto cap = r.translated (0, on ? 1.8f : 0.0f);
        juce::ColourGradient cg (juce::Colour (on ? 0xff2a2622 : 0xff433e38).brighter (over && ! on ? 0.10f : 0.0f), cap.getX(), cap.getY(),
                                 juce::Colour (0xff12100e), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (on ? 0.04f : 0.14f));
        g.drawHorizontalLine ((int) cap.getY() + 1, cap.getX() + 3, cap.getRight() - 3);
        if (on)
        {
            g.setColour (colours::amber.withAlpha (0.10f));
            g.fillRoundedRectangle (cap, 3.0f);
        }

        if (vertical)
        {
            lampAt (g, juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ cap.getX() + 15.0f, cap.getCentreY() }), on);
            g.setColour (on ? colours::cream : colours::cream.withAlpha (0.55f));
            g.setFont (stencil (11.0f, 0.14f));
            g.drawText (labels[i], cap.withTrimmedLeft (30.0f).withTrimmedRight (8.0f), juce::Justification::centredLeft);
            g.setColour (colours::cream.withAlpha (0.25f));
            g.setFont (stencil (8.0f, 0.1f));
            g.drawText (juce::String (i + 1), cap.withTrimmedRight (10.0f), juce::Justification::centredRight);
        }
        else
        {
            lampAt (g, juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ cap.getCentreX(), cap.getY() + 9.0f }), on);
            g.setColour (on ? colours::cream : colours::cream.withAlpha (0.55f));
            g.setFont (stencil (labels[i].length() > 7 ? 9.0f : 10.0f, 0.1f));
            g.drawText (labels[i], cap.withTrimmedTop (15.0f), juce::Justification::centred);
        }
    }
}

//==============================================================================
void PlayButton::paint (juce::Graphics& g)
{
    const bool on = index() != 0;
    const bool over = isMouseOver();
    auto r = getLocalBounds().toFloat().reduced (3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r.translated (0, 3.0f), 8.0f);
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

    auto cap = r.reduced (6.0f).translated (0, on ? 2.0f : 0.0f);
    const auto base = on ? colours::jamGreen.brighter (0.25f) : colours::jamGreen;
    juce::ColourGradient cg (base.brighter (over ? 0.12f : 0.0f), cap.getX(), cap.getY(),
                             colours::jamGreen.darker (0.8f), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.10f : 0.22f));
    g.fillRoundedRectangle (cap.withHeight (cap.getHeight() * 0.36f).reduced (6, 3), 4.0f);
    if (on)
    {
        g.setColour (colours::jamGold.withAlpha (0.5f));
        g.drawRoundedRectangle (r.expanded (2.0f), 10.0f, 3.0f);
    }

    // ícone + texto
    const float iconS = 16.0f;
    auto content = cap.withSizeKeepingCentre (cap.getWidth(), cap.getHeight());
    g.setFont (stencil (17.0f, 0.2f));
    const juce::String text = on ? "STOP" : "PLAY";
    const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text);
    const float total = iconS + 12.0f + tw;
    const float x0 = content.getCentreX() - total * 0.5f, cy = content.getCentreY() + 1.0f;
    juce::Path icon;
    if (on) icon.addRoundedRectangle (x0, cy - iconS * 0.45f, iconS * 0.9f, iconS * 0.9f, 2.0f);
    else    icon.addTriangle (x0, cy - iconS * 0.55f, x0, cy + iconS * 0.55f, x0 + iconS, cy);
    g.setColour (colours::jamGold);
    g.fillPath (icon);
    g.drawText (text, juce::Rectangle<float> (x0 + iconS + 12.0f, cy - 12.0f, tw + 4.0f, 24.0f), juce::Justification::centredLeft);

    lampAt (g, juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ cap.getRight() - 14.0f, cap.getY() + 13.0f }), on);
}

//==============================================================================
void MidiDragZone::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    glassBox (g, r.reduced (3.0f), isMouseOver() ? 0.30f : 0.15f);

    juce::Path outline, dashed;
    outline.addRoundedRectangle (r.reduced (6.0f), 3.0f);
    const float dashes[] = { 5.0f, 4.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, outline, dashes, 2);
    g.setColour (colours::amber.withAlpha (isMouseOver() ? 0.9f : 0.55f));
    g.fillPath (dashed);

    g.setColour (colours::amber);
    g.setFont (stencil (12.5f, 0.14f));
    g.drawText ("ARRASTE O MIDI", r.withTrimmedBottom (r.getHeight() * 0.42f).withTrimmedTop (4), juce::Justification::centredBottom);
    g.setColour (colours::amber.withAlpha (0.6f));
    g.setFont (stencil (9.0f, 0.18f));
    g.drawText ("PARA A TIMELINE DA DAW", r.withTrimmedTop (r.getHeight() * 0.58f), juce::Justification::centredTop);
}

void MidiDragZone::mouseDrag (const juce::MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    const auto s = proc.getSettings();
    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile (juce::String ("Bass Machine ") + mbm::styleName (s.style) + " " + juce::String (s.seed) + ".mid");
    if (proc.writeMidiFile (file))
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (file.getFullPathName()), false, this);
}

//==============================================================================
void BassRoll::update()
{
    slots = proc.getSlots();
    events = mbm::buildLoop (slots, proc.getSettings(), loopBeats);
    repaint();
}

void BassRoll::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    glassBox (g, r);
    if (loopBeats <= 0.0 || events.empty()) return;

    auto area = r.reduced (8.0f, 6.0f);
    auto header = area.removeFromTop (16.0f);
    auto keys = area.removeFromLeft (30.0f);

    int lo = 127, hi = 0;
    for (const auto& e : events) { lo = std::min (lo, e.note); hi = std::max (hi, e.note); }
    lo -= 2; hi += 2;
    if (hi - lo < 14) { const int mid = (hi + lo) / 2; lo = mid - 7; hi = mid + 7; }
    const float rowH = area.getHeight() / (float) (hi - lo + 1);
    auto yOf = [&] (int n) { return area.getBottom() - (float) (n - lo + 1) * rowH; };
    auto xOf = [&] (double beat) { return area.getX() + (float) (beat / loopBeats) * area.getWidth(); };

    // faixas de oitava/semitom e nomes das notas Cs e Es à esquerda
    for (int n = lo; n <= hi; ++n)
    {
        const bool blackKey = isBlack (n % 12);
        if (blackKey)
        {
            g.setColour (juce::Colours::black.withAlpha (0.22f));
            g.fillRect (area.getX(), yOf (n), area.getWidth(), rowH);
        }
        if (n % 12 == 0 || n % 12 == 4 || n % 12 == 9)
        {
            g.setColour (colours::amber.withAlpha (0.45f));
            g.setFont (stencil (8.5f, 0.0f));
            g.drawText (juce::MidiMessage::getMidiNoteName (n, true, true, 4), keys.withY (yOf (n) - 4.0f).withHeight (rowH + 8.0f),
                        juce::Justification::centredLeft);
        }
    }

    // acordes: faixas e rótulos
    double x = 0;
    for (int i = 0; i < (int) slots.size(); ++i)
    {
        const auto& s = slots[(size_t) i];
        if (s.beats <= 0) continue;
        const float x0 = xOf (x), x1 = xOf (x + s.beats);
        g.setColour (colours::amber.withAlpha ((i % 2) ? 0.03f : 0.07f));
        g.fillRect (x0, area.getY(), x1 - x0, area.getHeight());
        g.setColour (colours::amber.withAlpha (0.45f));
        g.drawVerticalLine ((int) x0, header.getY(), area.getBottom());
        g.setColour (colours::amber);
        g.setFont (stencil (11.0f, 0.08f));
        g.drawText (juce::String (mbm::chordLabel (s)), juce::Rectangle<float> (x0 + 5.0f, header.getY(), x1 - x0 - 6.0f, header.getHeight()),
                    juce::Justification::centredLeft);
        x += s.beats;
    }
    // compassos e tempos
    for (double b = 1; b < loopBeats; b += 1)
    {
        const bool bar = std::fmod (b, 4.0) < 1e-6;
        g.setColour (colours::amber.withAlpha (bar ? 0.16f : 0.06f));
        g.drawVerticalLine ((int) xOf (b), area.getY(), area.getBottom());
    }

    const double pos = proc.getPlayPositionInLoop();
    const int sounding = pos >= 0.0 ? proc.getSoundingNote() : -1;

    for (const auto& e : events)
    {
        auto nr = juce::Rectangle<float> (xOf (e.start), yOf (e.note), juce::jmax (3.0f, xOf (e.start + e.length) - xOf (e.start)), rowH)
                      .reduced (0.0f, rowH > 6.0f ? 1.0f : 0.0f);
        const bool now = pos >= 0.0 && pos >= e.start && pos < e.start + e.length && e.note == sounding;
        const float a = 0.45f + 0.55f * (float) e.velocity / 127.0f;
        const auto c = e.fill ? colours::jamGold : colours::amber;
        if (now)
        {
            g.setColour (c.withAlpha (0.35f));
            g.fillRoundedRectangle (nr.expanded (3.0f), 3.0f);
        }
        g.setColour ((now ? juce::Colour (0xffffe3a0) : c).withAlpha (now ? 1.0f : a));
        g.fillRoundedRectangle (nr, 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (nr, 2.0f, 0.6f);
        if (nr.getWidth() > 22.0f && rowH >= 7.0f)
        {
            g.setColour (juce::Colour (0xff2a1a05));
            g.setFont (stencil (juce::jmin (9.0f, rowH + 1.0f), 0.0f));
            g.drawText (mbm::rootName (e.note), nr.withTrimmedLeft (3.0f), juce::Justification::centredLeft);
        }
    }

    if (pos >= 0.0)
    {
        const float px = xOf (pos);
        g.setColour (colours::amber.withAlpha (0.3f));
        g.fillRect (px - 3.0f, header.getY(), 6.0f, area.getBottom() - header.getY());
        g.setColour (juce::Colour (0xffffe3a0));
        g.fillRect (px - 1.0f, header.getY(), 2.0f, area.getBottom() - header.getY());
    }

    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.4f), 3.0f);
}

//==============================================================================
// Osciloscópio de tubo verde (mesmo do Mastor Fone)
static juce::Rectangle<float> drawCrtFrame (juce::Graphics& g, juce::Rectangle<float> b, const juce::String& labelText, juce::Path& tube)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (b.reduced (2.0f).translated (1.5f, 3.0f), 10.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2a2a), 0, b.getY(), juce::Colour (0xff0b0b0b), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b.reduced (2.0f), 10.0f);
    auto label = b.removeFromBottom (18.0f);
    auto screen = b.reduced (12.0f, 9.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8e8e8), screen.getX(), screen.getY(), juce::Colour (0xff4c4c4c),
                                             screen.getRight(), screen.getBottom(), false));
    g.drawRoundedRectangle (screen.expanded (3.0f), 14.0f, 2.5f);
    tube.clear();
    tube.addRoundedRectangle (screen, 12.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0f2a17), screen.getCentreX(), screen.getCentreY(),
                                             juce::Colour (0xff030a05), screen.getX(), screen.getY(), true));
    g.fillPath (tube);

    g.setColour (colours::cream.withAlpha (0.85f));
    g.setFont (stencil (9.5f, 0.12f));
    g.drawText (labelText, label.withTrimmedLeft (14.0f).translated (0, -2.0f), juce::Justification::centredLeft);
    const auto lp = juce::Point<float> (label.getRight() - 18.0f, label.getCentreY() - 3.0f);
    g.setColour (colours::phosphor);
    g.fillEllipse (lp.x - 3.5f, lp.y - 3.5f, 7.0f, 7.0f);
    return screen;
}

static void drawCrtGlass (juce::Graphics& g, juce::Rectangle<float> screen)
{
    // vinheta do tubo e reflexo curvo do vidro
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, screen.getCentreX(), screen.getCentreY(),
                                             juce::Colours::black.withAlpha (0.55f), screen.getX(), screen.getY(), true));
    g.fillRect (screen);
    juce::Path refl;
    refl.addEllipse (screen.getX() - screen.getWidth() * 0.1f, screen.getY() - screen.getHeight() * 0.65f,
                     screen.getWidth() * 0.9f, screen.getHeight() * 1.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), screen.getX(), screen.getY(),
                                             juce::Colours::transparentWhite, screen.getX(), screen.getCentreY(), false));
    g.fillPath (refl);
}

void CrtScope::update()
{
    const int N = (int) buf.size(), show = 1536;
    proc.copyScope (buf.data(), N);
    // gatilho: subida pelo zero (com histerese) para a onda ficar parada na tela
    int start = N - show;
    for (int i = N - show - 1; i > 1; --i)
        if (buf[(size_t) i - 1] < -0.01f && buf[(size_t) i] >= 0.0f) { start = i; break; }
    start = juce::jmin (start, N - show);

    prev2 = prev1; prev1 = trace;
    const int cols = 200;
    trace.assign ((size_t) cols * 2, 0.0f);
    for (int c = 0; c < cols; ++c)
    {
        float lo = 1e9f, hi = -1e9f;
        const int a = start + c * show / cols, b = start + (c + 1) * show / cols;
        for (int i = a; i < b; ++i) { lo = juce::jmin (lo, buf[(size_t) i]); hi = juce::jmax (hi, buf[(size_t) i]); }
        trace[(size_t) c * 2] = lo; trace[(size_t) c * 2 + 1] = hi;
    }
    repaint();
}

void CrtScope::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    juce::Path tube;
    const auto screen = drawCrtFrame (g, b, juce::String::fromUTF8 ("OSCILOSCÓPIO  ·  BAIXO"), tube);

    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (tube);

    g.setColour (colours::phosphor.withAlpha (0.13f));                      // retícula
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine (juce::roundToInt (screen.getX() + screen.getWidth() * (float) i / 8.0f), screen.getY(), screen.getBottom());
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine (juce::roundToInt (screen.getY() + screen.getHeight() * (float) i / 4.0f), screen.getX(), screen.getRight());
    g.setColour (colours::phosphor.withAlpha (0.22f));
    g.drawHorizontalLine (juce::roundToInt (screen.getCentreY()), screen.getX(), screen.getRight());

    // traço com persistência do fósforo (quadros anteriores mais apagados)
    auto drawTrace = [&] (const std::vector<float>& t, float alpha, bool glow)
    {
        if (t.size() < 4) return;
        const int cols = (int) t.size() / 2;
        const float cy = screen.getCentreY(), amp = screen.getHeight() * 0.45f;
        auto yOf = [&] (float v) { return cy - amp * juce::jlimit (-1.0f, 1.0f, v * 0.95f); };
        juce::Path p;
        for (int c = 0; c < cols; ++c)
        {
            const float x = screen.getX() + 4.0f + (screen.getWidth() - 8.0f) * (float) c / (float) (cols - 1);
            const float mid = 0.5f * (t[(size_t) c * 2] + t[(size_t) c * 2 + 1]);
            if (c == 0) p.startNewSubPath (x, yOf (mid)); else p.lineTo (x, yOf (mid));
            g.setColour (colours::phosphor.withAlpha (0.35f * alpha));
            g.drawLine (x, yOf (t[(size_t) c * 2]), x, yOf (t[(size_t) c * 2 + 1]), 1.0f);
        }
        if (glow)
        {
            g.setColour (colours::phosphor.withAlpha (0.18f * alpha));
            g.strokePath (p, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (colours::phosphor.withAlpha (alpha));
        g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    drawTrace (prev2, 0.18f, false);
    drawTrace (prev1, 0.35f, false);
    drawTrace (trace, 1.0f, true);

    drawCrtGlass (g, screen);
}

//==============================================================================
BassMachinePanel::BassMachinePanel (BassMachineProcessor& p) : proc (p), dragZone (p), roll (p), scope (p)
{
    setLookAndFeel (&lnf);

    for (int i = 0; i < BassMachineProcessor::numSlots; ++i)
    {
        auto& m = mods[(size_t) i];
        const auto n = juce::String (i);
        m.keys = std::make_unique<NoteKeys>      (*proc.apvts.getParameter ("root" + n));
        m.qual = std::make_unique<QualitySwitch> (*proc.apvts.getParameter ("qual" + n));
        m.dur  = std::make_unique<CounterWindow> (*proc.apvts.getParameter ("dur" + n), CounterWindow::Duration);
        addAndMakeVisible (*m.keys);
        addAndMakeVisible (*m.qual);
        addAndMakeVisible (*m.dur);
        if (i > 0) setupToggle (m.on, "on" + n, "ON");
    }

    auto percent = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };

    {
        juce::StringArray names;
        for (int s = 0; s < mbm::NumStyles; ++s) names.add (mbm::styleName (s));
        styleKeys = std::make_unique<InterlockKeys> (*proc.apvts.getParameter ("style"), names, true);
        addAndMakeVisible (*styleKeys);
        juce::StringArray drums, kmodes;
        for (int d = 0; d < mbm::NumDrumStyles; ++d) drums.add (mbm::drumStyleName (d));
        kmodes.add ("PIANO M1"); kmodes.add (juce::String::fromUTF8 ("ÓRGÃO B3")); kmodes.add ("AMBOS");
        drumKeys = std::make_unique<InterlockKeys> (*proc.apvts.getParameter ("drumStyle"), drums, false);
        keysKeys = std::make_unique<InterlockKeys> (*proc.apvts.getParameter ("keysMode"), kmodes, false);
        addAndMakeVisible (*drumKeys);
        addAndMakeVisible (*keysKeys);
    }
    setupKnob (density,   "density",   "DENSIDADE");
    setupKnob (variation, "variation", "VARIACAO");
    setupKnob (swing,     "swing",     "SWING");
    setupKnob (base,      "base",      "REGISTRO");
    base.slider.textFromValueFunction = [] (double v) { return juce::MidiMessage::getMidiNoteName ((int) v, true, true, 4); };
    setupKnob (velocity,  "velocity",  "VELOCITY");
    setupKnob (gate,      "gate",      "SUSTENTA");
    for (auto* k : { &density, &variation, &swing, &velocity, &gate })
        k->slider.textFromValueFunction = percent;

    idea = std::make_unique<CounterWindow> (*proc.apvts.getParameter ("seed"), CounterWindow::Idea);
    addAndMakeVisible (*idea);
    newIdeaBtn.onClick = [this] { proc.newIdea(); };
    addAndMakeVisible (newIdeaBtn);

    play = std::make_unique<PlayButton> (*proc.apvts.getParameter ("preview"));
    addAndMakeVisible (*play);
    setupKnob (bpm, "bpm", "BPM");

    {
        juce::StringArray tnames;
        for (int t = 0; t < mbm::NumTimbres; ++t) tnames.add (mbm::timbre (t).name);
        timbreKeys = std::make_unique<InterlockKeys> (*proc.apvts.getParameter ("timbre"), tnames, false);
        addAndMakeVisible (*timbreKeys);
    }
    setupKnob (inGain, "inGain", "GANHO");
    inGain.slider.textFromValueFunction = [] (double v) { return (v > 0.05 ? "+" : "") + juce::String (v, 1) + " dB"; };
    inGain.slider.setDoubleClickReturnValue (true, 0.0);
    setupKnob (tone,   "tone",   "TONE");
    setupKnob (drive,  "drive",  "DRIVE");
    setupKnob (volume, "volume", "VOLUME");
    for (auto* k : { &tone, &drive, &volume })
        k->slider.textFromValueFunction = percent;
    setupToggle (sound, "sound", "SOM INTERNO");
    setupToggle (drumsOn, "drumsOn", "BATERIA");
    setupToggle (keysOn, "keysOn", "TECLADO");
    setupKnob (drumVol, "drumVol", "VOLUME");
    setupKnob (keysVol, "keysVol", "VOLUME");
    for (auto* k : { &drumVol, &keysVol })
        k->slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };

    setupKnob (reps, "exportReps", "REPETIR");
    reps.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    exportBtn.onClick = [this] { exportMidi(); };
    addAndMakeVisible (exportBtn);
    addAndMakeVisible (dragZone);
    addAndMakeVisible (roll);
    addAndMakeVisible (scope);

    for (auto* k : { &density, &variation, &swing, &base, &velocity, &gate, &bpm, &inGain, &tone, &drive, &volume, &drumVol, &keysVol })
        k->slider.updateText();

    setSize (kW, kH);
    timerCallback();
    startTimerHz (30);
}

BassMachinePanel::~BassMachinePanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BassMachinePanel::setupKnob (Knob& k, const juce::String& id, const juce::String& name, const juce::String& sliderName)
{
    k.slider.setName (sliderName);
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (kStartAngle, kEndAngle, true);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 80, 15);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxTextColourId, colours::inkSoft);
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<SliderAttach> (proc.apvts, id, k.slider);
    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (stencil (10.5f, 0.12f));
    k.label.setColour (juce::Label::textColourId, colours::ink);
    addAndMakeVisible (k.label);
}

void BassMachinePanel::setupToggle (Toggle& t, const juce::String& id, const juce::String& text)
{
    t.button.setButtonText (text);
    addAndMakeVisible (t.button);
    t.att = std::make_unique<ButtonAttach> (proc.apvts, id, t.button);
}

void BassMachinePanel::placeKnob (Knob& k, juce::Rectangle<int> r)
{
    k.label.setBounds (r.removeFromTop (14));
    k.slider.setBounds (r);
}

bool BassMachinePanel::slotEnabled (int i) const
{
    return i == 0 || proc.apvts.getRawParameterValue ("on" + juce::String (i))->load() > 0.5f;
}

void BassMachinePanel::exportMidi()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Bass Machine");
    dir.createDirectory();
    const auto s = proc.getSettings();
    const auto name = juce::String ("baixo ") + juce::String (mbm::styleName (s.style)).toLowerCase() + " " + juce::String (s.seed) + ".mid";
    chooser = std::make_unique<juce::FileChooser> ("Salvar linha de baixo em MIDI", dir.getChildFile (name), "*.mid");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              if (! f.hasFileExtension ("mid")) f = f.withFileExtension ("mid");
                              if (! proc.writeMidiFile (f))
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                                          "Bass Machine", "Nao foi possivel salvar o MIDI.");
                          });
}

void BassMachinePanel::timerCallback()
{
    for (int i = 0; i < BassMachineProcessor::numSlots; ++i)
    {
        const bool on = slotEnabled (i);
        if (on != lastOn[(size_t) i] || ! isShowing())
        {
            lastOn[(size_t) i] = on;
            auto& m = mods[(size_t) i];
            for (juce::Component* c : { (juce::Component*) m.keys.get(), (juce::Component*) m.qual.get(), (juce::Component*) m.dur.get() })
                c->setAlpha (on ? 1.0f : 0.38f);
        }
    }
    roll.update();
    scope.update();
    repaint();
}

//==============================================================================
void BassMachinePanel::drawKnobScale (juce::Graphics& g, juce::Slider& s, int ticks, const juce::StringArray& labels, float labelW)
{
    const auto circ = knobCircle (s);
    const auto c = circ.getCentre();
    const float r = circ.getWidth() * 0.5f;
    const float a0 = isSwitch (s) ? kSwStart : kStartAngle, a1 = isSwitch (s) ? kSwEnd : kEndAngle;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / (float) juce::jmax (1, ticks - 1);
        const float r1 = r + 4.0f, r2 = r + (labels.isEmpty() ? 8.0f : 11.0f);
        g.setColour (colours::ink.withAlpha (0.85f));
        g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1,
                    c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, labels.isEmpty() ? 1.1f : 1.6f);
        if (i < labels.size())
        {
            g.setFont (stencil (9.0f, 0.06f));
            const float lr = r + 16.0f + labelW * 0.32f * std::abs (std::sin (a));
            g.drawText (labels[i], juce::Rectangle<float> (labelW, 13).withCentre ({ c.x + std::sin (a) * lr, c.y - std::cos (a) * lr }),
                        juce::Justification::centred);
        }
    }
}

void BassMachinePanel::renderBackground()
{
    if (getWidth() <= 0) return;
    const float scale = 2.0f;   // 2x para telas retina
    background = juce::Image (juce::Image::ARGB, (int) (getWidth() * scale), (int) (getHeight() * scale), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    juce::Random rng (1976);

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
    const juce::String title ("MASTOR BASS MACHINE " + versionShort());
    g.drawText (title, plate.getX() + 32, 20, 760, 34, juce::Justification::left);
    const float titleRight = (float) plate.getX() + 32.0f + juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title);

    {   // placa de latão "DUB STATION" (a versão fica no título)
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
        g.setColour (juce::Colour (0xff3d2c0c).withAlpha (0.55f));
        g.drawRoundedRectangle (pl, 3.0f, 0.8f);
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
    g.drawText (juce::String::fromUTF8 ("MBM-710  \xc2\xb7  GERADOR DE REGGAE BASS LINES  \xc2\xb7  DUBMASTOR"), plate.getX() + 34, 52, 700, 14, juce::Justification::left);

    {   // faixas da bandeira da Jamaica, pintadas à mão e envelhecidas
        const float sx = (float) plate.getRight() - 330.0f, sw = 290.0f, sh = 7.0f;
        const juce::Colour faded[] = {
            colours::jamGreen.interpolatedWith (juce::Colour (0xff6f7a4a), 0.35f),
            colours::jamGold.interpolatedWith (juce::Colour (0xffb08a3a), 0.40f),
            juce::Colour (0xff2b2621) };
        for (int k = 0; k < 3; ++k)
        {
            const float y = 30.0f + 9.0f * (float) k;
            juce::Path bandP;
            bandP.startNewSubPath (sx, y + rng.nextFloat() * 0.8f);
            for (float x = sx; x <= sx + sw; x += 6.0f) bandP.lineTo (x, y + (rng.nextFloat() - 0.3f) * 0.9f);
            for (float x = sx + sw; x >= sx; x -= 6.0f) bandP.lineTo (x, y + sh + (rng.nextFloat() - 0.7f) * 0.9f);
            bandP.closeSubPath();
            juce::ColourGradient fade (faded[k].withAlpha (0.92f), sx, y, faded[k].withAlpha (0.70f), sx + sw, y, false);
            fade.addColour (0.3, faded[k].brighter (0.12f).withAlpha (0.85f));
            g.setGradientFill (fade);
            g.fillPath (bandP);
        }
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
    section (stylePanel,   "ESTILO");
    section (harmonyPanel, "HARMONIA");
    section (linePanel,    "LINHA DE BAIXO");
    section (previewPanel, "PREVIEW");
    section (timbrePanel,  juce::String::fromUTF8 ("TIMBRE  \xc2\xb7  DUB BASS"));
    section (scopePanel,   juce::String::fromUTF8 ("OSCILOSCÓPIO"));
    section (midiPanel,    "MIDI");
    section (drumsPanel,   "BATERIA DE ACOMPANHAMENTO");
    section (keysPanel,    juce::String::fromUTF8 ("TECLADO  \xc2\xb7  MASTOR ROOTS KEYS"));

    // módulos de acorde: moldura rebaixada + número gravado
    for (int i = 0; i < BassMachineProcessor::numSlots; ++i)
    {
        const auto& md = mods[(size_t) i];
        auto a = md.area.toFloat();
        g.setColour (colours::ink.withAlpha (0.07f));
        g.fillRoundedRectangle (a, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawRoundedRectangle (a.translated (1, 1), 4.0f, 0.8f);
        g.setColour (colours::ink.withAlpha (0.35f));
        g.drawRoundedRectangle (a, 4.0f, 1.0f);

        auto badge = juce::Rectangle<float> (a.getX() + 8.0f, a.getY() + 9.0f, 22.0f, 22.0f);
        g.setColour (colours::labelBg);
        g.fillEllipse (badge);
        g.setColour (colours::cream);
        g.setFont (stencil (12.0f, 0.0f));
        g.drawText (juce::String (i + 1), badge, juce::Justification::centred);
        g.setColour (colours::inkSoft);
        g.setFont (stencil (9.0f, 0.25f));
        g.drawText ("ACORDE", juce::Rectangle<float> (badge.getRight() + 6.0f, badge.getY(), 70.0f, badge.getHeight()), juce::Justification::centredLeft);
        if (i == 0)
        {
            g.setFont (stencil (8.0f, 0.18f));
            g.drawText ("SEMPRE LIGADO", md.on.button.getBounds().toFloat().withX (a.getRight() - 96.0f).withWidth (88.0f),
                        juce::Justification::centredRight);
        }
        g.setFont (stencil (8.5f, 0.3f));
        g.drawText ("TONICA", md.keys->getBounds().translated (0, -13).withHeight (11), juce::Justification::centred);
        g.drawText ("TIPO", md.qual->getBounds().translated (0, -13).withHeight (11), juce::Justification::centred);
        g.drawText ("DURACAO", md.dur->getBounds().translated (0, -13).withHeight (11), juce::Justification::centred);
    }

    // escalas


    for (auto* k : { &density, &variation, &swing, &base, &velocity, &gate, &bpm, &inGain, &tone, &drive, &volume, &drumVol, &keysVol })
        drawKnobScale (g, k->slider, 11);
    g.setColour (colours::inkSoft);
    g.setFont (stencil (8.5f, 0.12f));
    g.drawFittedText (juce::String::fromUTF8 ("SÓ NO PREVIEW: O MIDI ARRASTADO É SÓ O BAIXO"),
                      drumsPanel.withTrimmedLeft (14).withWidth (150).withTrimmedTop (64).withHeight (30), juce::Justification::centredLeft, 2);
    g.drawFittedText (juce::String::fromUTF8 ("SKANK DO PIANO M1 E BUBBLE DO HAMMOND SEGUINDO A HARMONIA"),
                      keysPanel.withTrimmedLeft (14).withWidth (150).withTrimmedTop (64).withHeight (30), juce::Justification::centredLeft, 2);
    drawKnobScale (g, reps.slider, 4, { "1x", "2x", "4x", "8x" }, 22.0f);

    // dicas gravadas
    g.setColour (colours::inkSoft);
    g.setFont (stencil (8.5f, 0.12f));
    g.drawFittedText (juce::String::fromUTF8 ("CLIQUE NAS SETAS PARA VOLTAR A UMA IDEIA  \xc2\xb7  SHIFT = DE 10 EM 10"), ideaHint, juce::Justification::centred, 2);
}

void BassMachinePanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::woodDark);
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    const auto slots = proc.getSlots();
    for (int i = 0; i < BassMachineProcessor::numSlots; ++i)
    {
        const auto& s = slots[(size_t) i];
        const bool on = s.beats > 0;
        juce::String t = "OFF";
        if (on) t = juce::String (mbm::rootName (s.root)) + " " + mbm::qualityLongName (s.quality);
        glowText (g, mods[(size_t) i].display.toFloat(), t, 15.0f, on ? 1.0f : 0.35f);
    }

    const int st = (int) proc.apvts.getRawParameterValue ("style")->load();
    {
        auto r = styleDisplay.toFloat();
        glassBox (g, r);
        g.setColour (colours::amber);
        g.setFont (stencil (17.0f, 0.14f));
        g.drawText (mbm::styleName (st), r.withTrimmedBottom (r.getHeight() * 0.42f), juce::Justification::centredBottom);
        g.setColour (colours::amber.withAlpha (0.65f));
        g.setFont (stencil (9.5f, 0.2f));
        g.drawText (mbm::styleTagline (st), r.withTrimmedTop (r.getHeight() * 0.6f), juce::Justification::centredTop);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f), 3.0f);
    }

    const int tb = (int) proc.apvts.getRawParameterValue ("timbre")->load();
    {
        auto r = timbreDisplay.toFloat();
        glassBox (g, r);
        const float on = proc.apvts.getRawParameterValue ("sound")->load() > 0.5f ? 1.0f : 0.35f;
        g.setColour (colours::amber.withAlpha (on));
        g.setFont (stencil (15.0f, 0.14f));
        g.drawText (mbm::timbre (tb).name, r.withTrimmedBottom (r.getHeight() * 0.42f), juce::Justification::centredBottom);
        g.setColour (colours::amber.withAlpha (0.6f * on));
        g.setFont (stencil (8.5f, 0.16f));
        g.drawText (mbm::timbre (tb).tagline, r.withTrimmedTop (r.getHeight() * 0.6f), juce::Justification::centredTop);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f), 3.0f);
    }
}

void BassMachinePanel::resized()
{
    const int W = getWidth(), H = getHeight();
    woodL = { 0, 0, 28, H };
    woodR = { W - 28, 0, 28, H };
    plate = { 28, 0, W - 56, H };

    // --- linha 1: ESTILO (botoeira) | HARMONIA
    stylePanel   = { 52, 92, 300, 362 };
    harmonyPanel = { 366, 92, W - 52 - 366, 362 };
    {
        auto a = stylePanel.reduced (14, 0);
        a.removeFromTop (18);
        styleDisplay = a.removeFromTop (42).reduced (4, 0);
        a.removeFromTop (8);
        styleKeys->setBounds (a.removeFromTop (204));
        a.removeFromTop (6);
        const int kw = a.getWidth() / 4;
        for (auto* k : { &density, &variation, &swing, &base })
            placeKnob (*k, a.removeFromLeft (kw).withTrimmedBottom (8));
    }
    {
        auto a = harmonyPanel.reduced (12, 0);
        a.removeFromTop (18);
        a.removeFromBottom (12);
        const int gap = 10, mw = (a.getWidth() - gap * (BassMachineProcessor::numSlots - 1)) / BassMachineProcessor::numSlots;
        for (int i = 0; i < BassMachineProcessor::numSlots; ++i)
        {
            auto& m = mods[(size_t) i];
            m.area = { a.getX() + i * (mw + gap), a.getY(), mw, a.getHeight() };
            auto r = m.area.reduced (8, 0);
            r.removeFromTop (6);
            auto header = r.removeFromTop (28);
            m.on.button.setBounds (header.removeFromRight (62));
            r.removeFromTop (8);
            m.display = r.removeFromTop (32).reduced (3, 0);
            r.removeFromTop (22);
            m.keys->setBounds (r.removeFromTop (110).reduced (2, 0));
            r.removeFromTop (22);
            m.qual->setBounds (r.removeFromTop (40));
            r.removeFromTop (22);
            m.dur->setBounds (r.removeFromTop (44).reduced (4, 0));
        }
    }

    // --- linha 2: LINHA DE BAIXO (ideia | visor | toque)
    linePanel = { 52, 476, W - 104, 172 };
    {
        auto a = linePanel.reduced (14, 0);
        a.removeFromTop (18);
        a.removeFromBottom (10);
        auto left = a.removeFromLeft (190);
        idea->setBounds (left.removeFromTop (46));
        left.removeFromTop (8);
        newIdeaBtn.setBounds (left.removeFromTop (54));
        ideaHint = left.reduced (0, 2);
        a.removeFromLeft (12);
        auto right = a.removeFromRight (170);
        const int kw = right.getWidth() / 2;
        placeKnob (velocity, right.removeFromLeft (kw));
        placeKnob (gate, right);
        a.removeFromRight (10);
        roll.setBounds (a);
    }

    // --- linha 3: PREVIEW | TIMBRE | OSCILOSCÓPIO | MIDI
    const int y3 = 670, h3 = 168;
    previewPanel = { 52, y3, 214, h3 };
    timbrePanel  = { 280, y3, 470, h3 };
    scopePanel   = { 764, y3, 226, h3 };
    midiPanel    = { 1004, y3, W - 52 - 1004, h3 };
    {
        auto a = previewPanel.reduced (12, 0);
        a.removeFromTop (20);
        a.removeFromBottom (10);
        play->setBounds (a.removeFromLeft (118).withSizeKeepingCentre (118, 84));
        a.removeFromLeft (4);
        placeKnob (bpm, a.withTrimmedTop (10).withTrimmedBottom (10));
    }
    {
        auto a = timbrePanel.reduced (12, 0);
        a.removeFromTop (18);
        a.removeFromBottom (8);
        auto left = a.removeFromLeft (164);
        timbreDisplay = left.removeFromTop (38).reduced (2, 0);
        left.removeFromTop (14);
        timbreKeys->setBounds (left.removeFromTop (60));
        a.removeFromLeft (12);
        auto knobs = a.removeFromTop (96);
        const int kw = knobs.getWidth() / 4;
        for (auto* k : { &inGain, &tone, &drive, &volume })
            placeKnob (*k, knobs.removeFromLeft (kw));
        a.removeFromTop (8);
        sound.button.setBounds (a.removeFromTop (32).reduced (24, 0));
    }
    scope.setBounds (scopePanel.reduced (8, 0).withTrimmedTop (16).withTrimmedBottom (6));
    {
        auto a = midiPanel.reduced (12, 0);
        a.removeFromTop (18);
        auto top = a.removeFromTop (82);
        exportBtn.setBounds (top.removeFromLeft (106).withSizeKeepingCentre (106, 50));
        reps.slider.setBounds (top);
        reps.label.setBounds (top.removeFromTop (0));
        reps.label.setVisible (false);
        a.removeFromTop (4);
        dragZone.setBounds (a.removeFromTop (54));
    }

    // --- linha 4: ACOMPANHAMENTO (bateria | teclado do Roots Keys)
    const int y4 = 862, h4 = 116;
    drumsPanel = { 52, y4, 560, h4 };
    keysPanel  = { 626, y4, W - 52 - 626, h4 };
    for (auto [panel, onBtn, sel, vol] : { std::tuple<juce::Rectangle<int>, Toggle*, InterlockKeys*, Knob*> { drumsPanel, &drumsOn, drumKeys.get(), &drumVol },
                                           { keysPanel, &keysOn, keysKeys.get(), &keysVol } })
    {
        auto a = panel.reduced (14, 0);
        a.removeFromTop (18);
        a.removeFromBottom (10);
        auto left = a.removeFromLeft (150);
        onBtn->button.setBounds (left.removeFromTop (36));
        placeKnob (*vol, a.removeFromRight (84));
        a.removeFromRight (10);
        sel->setBounds (a.withSizeKeepingCentre (a.getWidth(), 56));
    }

    renderBackground();
}
