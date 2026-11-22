#include "PluginEditor.h"

#ifndef JucePlugin_VersionString
 #define JucePlugin_VersionString "0.0.0"
#endif

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
    const juce::Colour redLamp   { 0xffff4a35 };
}

static constexpr float kStartAngle = juce::MathConstants<float>::pi * 1.2f;
static constexpr float kEndAngle   = juce::MathConstants<float>::pi * 2.8f;
static constexpr float kKnobReduction = 11.0f;

static juce::Font stencil (float size, float kerning = 0.08f)
{
    return juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (kerning);
}

static juce::Font mono (float size)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::bold));
}

static juce::String versionShort()
{
    return juce::String (JucePlugin_VersionString).upToLastOccurrenceOf (".", false, false);
}

// área circular do knob dentro do slider (mesma conta do LookAndFeel)
static juce::Rectangle<float> knobCircle (const juce::Slider& s)
{
    auto r = s.getBounds().toFloat();
    if (s.getTextBoxPosition() == juce::Slider::TextBoxBelow) r.removeFromBottom ((float) s.getTextBoxHeight());
    r = r.reduced (kKnobReduction);
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

// moldura de tela de fósforo (bisel metálico + vidro escuro); devolve a área útil
static juce::Rectangle<float> drawCrtFrame (juce::Graphics& g, juce::Rectangle<float> b)
{
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (b.translated (1.5f, 3.0f), 10.0f);
    juce::ColourGradient bezel (juce::Colour (0xffe0ded8), b.getX(), b.getY(), juce::Colour (0xff55524c), b.getRight(), b.getBottom(), false);
    g.setGradientFill (bezel);
    g.fillRoundedRectangle (b, 10.0f);
    auto inner = b.reduced (5.0f);
    g.setColour (juce::Colour (0xff0b0d0b));
    g.fillRoundedRectangle (inner, 8.0f);
    auto screen = inner.reduced (5.0f);
    juce::ColourGradient glassG (juce::Colour (0xff0f2116), screen.getCentreX(), screen.getCentreY(),
                                 juce::Colour (0xff040806), screen.getX(), screen.getY(), true);
    g.setGradientFill (glassG);
    g.fillRoundedRectangle (screen, 12.0f);
    return screen;
}

static void crtGlass (juce::Graphics& g, juce::Rectangle<float> screen)
{
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (screen.withHeight (screen.getHeight() * 0.38f).reduced (6.0f, 3.0f), 10.0f);
}

//==============================================================================
DubLookAndFeel::DubLookAndFeel()
{
    setColour (juce::Label::textColourId, colours::ink);
    setColour (juce::Slider::textBoxTextColourId, colours::inkSoft);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::BubbleComponent::backgroundColourId, colours::labelBg);
    setColour (juce::BubbleComponent::outlineColourId, colours::amber);
    setColour (juce::TooltipWindow::textColourId, colours::amber);
}

juce::Label* DubLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (stencil (11.0f, 0.04f));
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return l;
}

void DubLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float startAngle, float endAngle, juce::Slider& s)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (kKnobReduction);
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

    const float capR = radius * 0.74f;                     // corpo de baquelite
    juce::ColourGradient body (juce::Colour (0xff3d3833), c.x - capR, c.y - capR,
                               juce::Colour (0xff070605), c.x + capR * 0.6f, c.y + capR, false);
    g.setGradientFill (body);
    g.fillEllipse (circle (capR));

    juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.16f), c.x, c.y - capR,
                                juce::Colours::transparentWhite, c.x, c.y + capR * 0.2f, false);
    g.setGradientFill (gloss);
    g.fillEllipse (circle (capR * 0.86f).translated (0, -capR * 0.06f));

    auto rot = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
    juce::Path line;
    const float lw = 2.6f;
    line.addRoundedRectangle (-lw * 0.5f, -capR * 0.95f, lw, capR * 0.62f, lw * 0.5f);
    g.setColour (s.isEnabled() ? colours::cream : colours::cream.withAlpha (0.4f));
    g.fillPath (line, rot);
    juce::Path notch;
    notch.addRectangle (-1.2f, -radius * 0.99f, 2.4f, radius - capR + 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillPath (notch, rot);
}

// fader do EQ: trilho escuro rebaixado e tampa cromada com risco âmbar
void DubLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                                       juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, s);
        return;
    }
    const float cx = (float) x + (float) w * 0.5f;
    auto slot = juce::Rectangle<float> (cx - 3.0f, (float) y - 2.0f, 6.0f, (float) h + 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.fillRoundedRectangle (slot.translated (0.8f, 0.8f), 3.0f);
    g.setColour (juce::Colour (0xff0e0c0a));
    g.fillRoundedRectangle (slot, 3.0f);

    const float zeroY = (float) s.getPositionOfValue (0.0);
    g.setColour (colours::amber.withAlpha (0.75f));
    g.fillRect (cx - 1.0f, juce::jmin (zeroY, pos), 2.0f, std::abs (pos - zeroY));

    auto cap = juce::Rectangle<float> (cx - 13.0f, pos - 8.0f, 26.0f, 16.0f);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (cap.translated (1.0f, 2.5f), 2.5f);
    juce::ColourGradient chrome (juce::Colour (0xffeeece6), cap.getX(), cap.getY(), juce::Colour (0xff6a6760), cap.getX(), cap.getBottom(), false);
    chrome.addColour (0.48, juce::Colour (0xffb9b6ae));
    chrome.addColour (0.52, juce::Colour (0xff8e8b84));
    g.setGradientFill (chrome);
    g.fillRoundedRectangle (cap, 2.5f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (cap, 2.5f, 0.8f);
    g.setColour (juce::Colour (0xff2b241d));
    g.fillRect (cap.getX() + 3.0f, cap.getCentreY() - 1.0f, cap.getWidth() - 6.0f, 2.0f);
}

// botão com tampa escura e lâmpada piloto (âmbar; vermelha no BYPASS)
void DubLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.0f);
    const bool on = b.getToggleState();
    const bool red = (bool) b.getProperties().getWithDefault ("redLamp", false);
    const auto lampCol = red ? colours::redLamp : colours::amber;

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
        g.setColour (lampCol.withAlpha (0.30f));
        g.fillEllipse (lamp.expanded (5.0f));
        juce::ColourGradient lit (red ? juce::Colour (0xffffc0b0) : juce::Colour (0xffffe3a0), lamp.getCentreX(), lamp.getCentreY(),
                                  lampCol.darker (0.2f), lamp.getRight(), lamp.getBottom(), true);
        g.setGradientFill (lit);
    }
    else
        g.setColour (red ? juce::Colour (0xff4a2420) : juce::Colour (0xff4a3a22));
    g.fillEllipse (lamp);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (lamp, 1.0f);

    g.setColour (colours::cream.withAlpha (on ? 1.0f : 0.65f));
    g.setFont (stencil (r.getHeight() > 30.0f ? 13.0f : 11.0f, 0.12f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (22), juce::Justification::centred);
}

// BASS BOOST: tampa verde grande de botão de pedal; ligado = aro dourado e tampa acesa
void DubLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (5.0f);
    if (on)
    {
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (colours::jamGold.withAlpha (0.10f * (float) i));
            g.fillRoundedRectangle (r.expanded ((float) (4 - i) * 2.0f), 9.0f);
        }
    }
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (r, 6.0f);                 // moldura
    auto cap = r.reduced (4.0f).translated (0, (down || on) ? 1.5f : 0.0f);
    const auto base = on ? colours::jamGreen.brighter (0.45f) : colours::jamGreen;
    juce::ColourGradient capG (base.brighter (over ? 0.1f : 0.0f), cap.getX(), cap.getY(),
                               colours::jamGreen.darker (on ? 0.45f : 0.75f), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (capG);
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.16f : 0.22f));
    g.fillRoundedRectangle (cap.withHeight (cap.getHeight() * 0.38f).reduced (4, 2), 3.0f);
    if (on)
    {
        g.setColour (colours::jamGold.withAlpha (0.85f));
        g.drawRoundedRectangle (r.expanded (1.5f), 7.0f, 2.2f);
    }
}

void DubLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    const bool on = b.getToggleState();
    g.setColour (on ? juce::Colour (0xfffff1b0) : colours::jamGold);
    g.setFont (stencil (16.0f, 0.18f));
    g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, (down || on) ? 2 : 1), juce::Justification::centred);
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

    const auto lamp = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ cap.getX() + 14.0f, cap.getCentreY() });
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
    auto area = cap.withTrimmedLeft (22.0f).withTrimmedRight (3.0f);
    float size = 11.5f;
    while (size > 8.0f && juce::GlyphArrangement::getStringWidth (stencil (size, 0.12f), text) > area.getWidth())
        size -= 0.5f;
    g.setFont (stencil (size, 0.12f));
    g.drawText (text, area, juce::Justification::centred);
}

ChoiceKeys::ChoiceKeys (juce::RangedAudioParameter& p, juce::StringArray names, std::vector<int> ord)
    : param (p), att (p, [this] (float v) { value = v; repaint(); }, nullptr), labels (std::move (names)), order (std::move (ord))
{
    if (order.empty())
        for (int i = 0; i < labels.size(); ++i) order.push_back (i);
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setRepaintsOnMouseActivity (true);
}

void ChoiceKeys::mouseDown (const juce::MouseEvent& e)
{
    const int n = juce::jmax (1, labels.size());
    const int i = juce::jlimit (0, n - 1, (int) (e.position.x / (float) getWidth() * (float) n));
    att.setValueAsCompleteGesture ((float) order[(size_t) i]);
}

void ChoiceKeys::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1714));                         // moldura metálica comum
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);
    auto inner = b.reduced (4.0f);
    const auto mouse = getMouseXYRelative().toFloat();
    const int n = juce::jmax (1, labels.size());
    const float w = inner.getWidth() / (float) n;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (inner.getX() + w * (float) i, inner.getY(), w, inner.getHeight()).reduced (1.5f, 0.0f);
        pushButtonCap (g, r.withTrimmedBottom (2.5f), juce::roundToInt (value) == order[(size_t) i], isMouseOver() && r.contains (mouse), labels[i]);
    }
}

//==============================================================================
void TubeGlow::update (bool on, float driveDb, float level)
{
    // calor: aquecimento do filamento (ligado) + DRIVE + quanto sinal está batendo na grade
    const float driveNorm = juce::jlimit (0.0f, 1.0f, driveDb / 36.0f);
    const float target = on ? juce::jlimit (0.0f, 1.0f, 0.16f + 0.34f * driveNorm + 0.55f * juce::jmin (1.2f, level) * (0.35f + 0.65f * driveNorm))
                            : 0.04f;
    heat += (target - heat) * (target > heat ? 0.07f : 0.025f);    // inércia térmica: esquenta rápido, esfria devagar
    flicker = 0.85f * flicker + 0.15f * (rng.nextFloat() - 0.5f);
    repaint();
}

static juce::Colour heatColour (float h)
{
    struct Stop { float t; juce::uint32 c; };
    static const Stop stops[] = { { 0.00f, 0xff2a0a04 }, { 0.22f, 0xff8e2206 }, { 0.45f, 0xffff5a0e },
                                  { 0.68f, 0xffffa62e }, { 0.86f, 0xffffdc80 }, { 1.00f, 0xfffff6dc } };
    h = juce::jlimit (0.0f, 1.0f, h);
    for (int i = 1; i < 6; ++i)
        if (h <= stops[i].t)
            return juce::Colour (stops[i - 1].c).interpolatedWith (juce::Colour (stops[i].c),
                                                                    (h - stops[i - 1].t) / (stops[i].t - stops[i - 1].t));
    return juce::Colour (stops[5].c);
}

void TubeGlow::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float h = juce::jlimit (0.0f, 1.0f, heat + flicker * 0.06f * (0.3f + heat));
    const auto hot = heatColour (h);

    // soquete de baquelite com anel cromado
    auto base = b.removeFromBottom (b.getHeight() * 0.17f);
    auto socket = base.withSizeKeepingCentre (b.getWidth() * 0.78f, base.getHeight());
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (socket.translated (2.0f, 3.0f), 5.0f);
    juce::ColourGradient bak (juce::Colour (0xff3a3430), socket.getX(), 0, juce::Colour (0xff0b0908), socket.getRight(), 0, false);
    bak.addColour (0.3, juce::Colour (0xff4a423c));
    g.setGradientFill (bak);
    g.fillRoundedRectangle (socket, 5.0f);
    auto ring = socket.withHeight (6.0f).reduced (4.0f, 0.0f);
    juce::ColourGradient chrome (juce::Colour (0xffeeece6), ring.getX(), 0, juce::Colour (0xff5e5b55), ring.getRight(), 0, false);
    chrome.addColour (0.35, juce::Colour (0xffffffff));
    g.setGradientFill (chrome);
    g.fillRoundedRectangle (ring, 2.0f);

    // envelope de vidro
    auto env = b.withSizeKeepingCentre (b.getWidth() * 0.66f, b.getHeight()).withTrimmedTop (8.0f);
    juce::Path glassPath;
    glassPath.addRoundedRectangle (env.getX(), env.getY(), env.getWidth(), env.getHeight() + 4.0f,
                                   env.getWidth() * 0.5f, env.getWidth() * 0.5f, true, true, false, false);
    // bico de selagem no topo
    juce::Path tip;
    tip.addRoundedRectangle (env.getCentreX() - 4.0f, env.getY() - 8.0f, 8.0f, 12.0f, 4.0f);

    // halo de luz projetado no berço em volta da válvula (some antes das bordas)
    if (h > 0.05f)
    {
        const auto full = getLocalBounds().toFloat();
        const auto c = env.getCentre();
        const float rx = full.getWidth() * 0.5f, ry = full.getHeight() * 0.48f;
        juce::ColourGradient halo (hot.withAlpha (0.08f + 0.32f * h), c.x, c.y,
                                   hot.withAlpha (0.0f), c.x + rx, c.y, true);
        halo.addColour (0.55, hot.withAlpha ((0.08f + 0.32f * h) * (0.25f + 0.35f * h)));
        g.setGradientFill (halo);
        g.fillEllipse (juce::Rectangle<float> (rx * 2.0f, ry * 2.0f).withCentre (c));
    }

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (glassPath);

        g.setColour (juce::Colour (0xff120d0a).withAlpha (0.92f));         // interior escuro
        g.fillPath (glassPath);

        // getter prateado (espelho escuro no domo)
        auto dome = env.withHeight (env.getWidth() * 0.55f);
        juce::ColourGradient getter (juce::Colour (0xff8d8a86).withAlpha (0.75f), dome.getX(), dome.getY(),
                                     juce::Colour (0xff1c1a18).withAlpha (0.6f), dome.getRight(), dome.getBottom(), false);
        g.setGradientFill (getter);
        g.fillEllipse (dome.reduced (2.0f));

        // brilho do filamento atrás da placa
        const auto core = env.withTrimmedTop (env.getHeight() * 0.28f).withTrimmedBottom (env.getHeight() * 0.18f);
        const auto cc = core.getCentre();
        juce::ColourGradient glow (hot.withAlpha (0.25f + 0.75f * h), cc.x, cc.y,
                                   hot.withAlpha (0.0f), cc.x + core.getWidth() * (0.35f + 0.55f * h), cc.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (core.expanded (core.getWidth() * 0.4f * h, core.getHeight() * 0.15f * h));

        // fluorescência azul de válvula forçada
        if (h > 0.62f)
        {
            const float blue = (h - 0.62f) / 0.38f;
            juce::ColourGradient bl (juce::Colour (0xff6f8dff).withAlpha (0.35f * blue), cc.x, core.getY(),
                                     juce::Colour (0xff6f8dff).withAlpha (0.0f), cc.x, core.getY() - core.getHeight() * 0.5f, false);
            g.setGradientFill (bl);
            g.fillRect (core.withY (core.getY() - core.getHeight() * 0.5f).withHeight (core.getHeight() * 0.6f));
        }

        // micas (espaçadores) em cima e embaixo
        g.setColour (juce::Colour (0xffcfc7b5).withAlpha (0.55f));
        g.fillRect (core.getX() - 2.0f, core.getY() - 6.0f, core.getWidth() + 4.0f, 3.0f);
        g.fillRect (core.getX() - 2.0f, core.getBottom() + 3.0f, core.getWidth() + 4.0f, 3.0f);

        // duas placas (triodo duplo) com a fenda por onde a luz escapa
        for (int k = 0; k < 2; ++k)
        {
            auto plateR = core.withWidth (core.getWidth() * 0.40f).translated (k == 0 ? 0.0f : core.getWidth() * 0.60f, 0.0f).reduced (1.0f, 2.0f);
            juce::ColourGradient pg (juce::Colour (0xff4a4744), plateR.getX(), 0, juce::Colour (0xff1e1c1a), plateR.getRight(), 0, false);
            pg.addColour (0.45, juce::Colour (0xff6b6762));
            g.setGradientFill (pg);
            g.fillRect (plateR);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            for (float yy = plateR.getY() + 4.0f; yy < plateR.getBottom() - 2.0f; yy += 5.0f)
                g.drawHorizontalLine ((int) yy, plateR.getX() + 1.0f, plateR.getRight() - 1.0f);
            // fenda central com o filamento
            auto slit = plateR.withSizeKeepingCentre (2.5f + 2.0f * h, plateR.getHeight() * 0.72f);
            g.setColour (hot.withAlpha (0.35f + 0.65f * h));
            g.fillRoundedRectangle (slit, 1.2f);
            g.setColour (heatColour (juce::jmin (1.0f, h + 0.25f)).withAlpha (0.6f * h));
            g.fillRoundedRectangle (slit.expanded (2.0f + 3.0f * h, 1.0f), 2.0f);
        }

        // hastes de suporte
        g.setColour (juce::Colour (0xff8c8780).withAlpha (0.7f));
        for (float sx : { core.getX() + 3.0f, core.getCentreX(), core.getRight() - 3.0f })
            g.drawVerticalLine ((int) sx, core.getY() - 8.0f, env.getBottom() + 4.0f);
    }

    // vidro por cima: reflexos e contorno
    juce::ColourGradient refl (juce::Colours::white.withAlpha (0.30f), env.getX(), 0,
                               juce::Colours::white.withAlpha (0.0f), env.getX() + env.getWidth() * 0.35f, 0, false);
    g.setGradientFill (refl);
    g.fillRoundedRectangle (env.withWidth (env.getWidth() * 0.30f).reduced (3.0f, env.getWidth() * 0.3f), 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f + 0.10f * h));
    g.fillRoundedRectangle (env.withTrimmedLeft (env.getWidth() * 0.78f).reduced (2.0f, env.getWidth() * 0.45f), 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.strokePath (glassPath, juce::PathStrokeType (1.2f));
    g.setColour (juce::Colour (0xffc9c4b8).withAlpha (0.8f));
    g.fillPath (tip);
}

//==============================================================================
const EnvPresetKeys::Preset EnvPresetKeys::presets[3] = {
    { "CURTO", 0.5f,  260.0f,  80.0f, 35.0f },    // punch seco, para riddim rápido
    { "MEDIO", 1.0f,  750.0f, 220.0f, 70.0f },    // o 808 clássico
    { "LONGO", 2.0f, 2200.0f, 700.0f, 110.0f },   // boom longo que segura o compasso
};

EnvPresetKeys::EnvPresetKeys (DubBassAudioProcessor& p) : proc (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setRepaintsOnMouseActivity (true);
}

int EnvPresetKeys::current() const
{
    auto near = [this] (const char* id, float v)
    {
        return std::abs (proc.apvts.getRawParameterValue (id)->load() - v) <= juce::jmax (0.6f, v * 0.03f);
    };
    for (int i = 0; i < 3; ++i)
        if (near (ParamID::attack, presets[i].attackMs) && near (ParamID::decay, presets[i].decayMs)
            && near (ParamID::release, presets[i].releaseMs))
            return i;
    return -1;
}

void EnvPresetKeys::mouseDown (const juce::MouseEvent& e)
{
    const int i = juce::jlimit (0, 2, (int) (e.position.x / (float) getWidth() * 3.0f));
    auto set = [this] (const char* id, float v)
    {
        if (auto* prm = proc.apvts.getParameter (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
            prm->endChangeGesture();
        }
    };
    set (ParamID::attack,    presets[i].attackMs);
    set (ParamID::decay,     presets[i].decayMs);
    set (ParamID::release,   presets[i].releaseMs);
    set (ParamID::pitchTime, presets[i].pitchTimeMs);
    repaint();
}

void EnvPresetKeys::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (b, 5.0f);
    auto inner = b.reduced (3.0f);
    auto lbl = inner.removeFromLeft (34.0f);
    g.setColour (colours::amber);
    g.setFont (stencil (10.0f, 0.1f));
    g.drawText ("808", lbl, juce::Justification::centred);
    const int cur = current();
    const auto mouse = getMouseXYRelative().toFloat();
    const float w = inner.getWidth() / 3.0f;
    for (int i = 0; i < 3; ++i)
    {
        auto r = juce::Rectangle<float> (inner.getX() + w * (float) i, inner.getY(), w, inner.getHeight()).reduced (1.5f, 0.0f);
        pushButtonCap (g, r.withTrimmedBottom (2.0f), cur == i, isMouseOver() && r.contains (mouse),
                       i == 1 ? juce::String::fromUTF8 ("MÉDIO") : juce::String (presets[i].name));
    }
}

//==============================================================================
void OzoneMeter::update()
{
    auto ballistic = [] (float& lvl, float& hold, int& holdT, float now)
    {
        lvl = now > lvl ? now : juce::jmax (now, lvl - 1.4f);       // ~42 dB/s de queda
        if (now >= hold) { hold = now; holdT = 45; }
        else if (--holdT <= 0) hold = juce::jmax (lvl, hold - 1.0f);
    };
    for (int c = 0; c < 2; ++c)
    {
        ballistic (in[c],  inHold[c],  inHoldT[c],  proc.inPeakDb[(size_t) c].load());
        ballistic (out[c], outHold[c], outHoldT[c], proc.outPeakDb[(size_t) c].load());
    }
    const float g = juce::jmin (0.0f, proc.gainReductionDb.load());
    gr = g < gr ? g : gr + (g - gr) * 0.25f;
    if (gr <= grHold) { grHold = gr; grHoldT = 45; }
    else if (--grHoldT <= 0) grHold = juce::jmin (0.0f, grHold + 0.3f);
    repaint();
}

void OzoneMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const bool bypass = proc.apvts.getRawParameterValue (ParamID::maxBypass)->load() > 0.5f;
    const float ceiling = proc.apvts.getRawParameterValue (ParamID::maxCeiling)->load();

    // visor de vidro rebaixado
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (b.translated (1.5f, 3.0f), 6.0f);
    juce::ColourGradient bg (juce::Colour (0xff0d1318), b.getX(), b.getY(), juce::Colour (0xff05080a), b.getX(), b.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (b, 6.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8e8e8), b.getX(), b.getY(), juce::Colour (0xff4c4c4c), b.getRight(), b.getBottom(), false));
    g.drawRoundedRectangle (b, 6.0f, 2.0f);

    auto inner = b.reduced (8.0f, 8.0f);
    auto readout = inner.removeFromTop (30.0f);
    auto labels = inner.removeFromBottom (14.0f);
    inner.removeFromBottom (4.0f);
    inner.removeFromTop (6.0f);

    const float minDb = -48.0f;
    auto frac = [&] (float d) { return juce::jlimit (0.0f, 1.0f, (d - minDb) / (0.0f - minDb)); };
    auto yOf  = [&] (float d) { return inner.getBottom() - frac (d) * inner.getHeight(); };

    // colunas: IN L, IN R | escala | GR | OUT L, OUT R
    const float scaleW = 20.0f, gap = 3.0f, grW = 14.0f;
    const float barW = (inner.getWidth() - scaleW - grW - gap * 6.0f) / 4.0f;
    float x = inner.getX();
    const float xInL = x;            x += barW + gap;
    const float xInR = x;            x += barW + gap * 2.0f;
    const float xScale = x;          x += scaleW + gap;
    const float xGr = x;             x += grW + gap * 2.0f;
    const float xOutL = x;           x += barW + gap;
    const float xOutR = x;

    // escala em dB
    g.setFont (mono (8.5f));
    for (float d : { 0.0f, -3.0f, -6.0f, -9.0f, -12.0f, -18.0f, -24.0f, -30.0f, -36.0f, -48.0f })
    {
        const float y = yOf (d);
        g.setColour (juce::Colour (0xff8fb6c8).withAlpha (0.18f));
        g.drawHorizontalLine ((int) y, inner.getX(), inner.getRight());
        g.setColour (d > -6.0f ? juce::Colour (0xffff8a7a) : juce::Colour (0xffa9c4d1));
        g.drawText (juce::String ((int) d), juce::Rectangle<float> (xScale, y - 5.0f, scaleW, 10.0f), juce::Justification::centred);
    }

    // barra de nível (gradiente contínuo com fatias finas, estilo Ozone)
    auto drawBar = [&] (float bx, float lvl, float hold, bool dim)
    {
        auto track = juce::Rectangle<float> (bx, inner.getY(), barW, inner.getHeight());
        g.setColour (juce::Colour (0xff0a1a22));
        g.fillRect (track);
        const float top = yOf (lvl);
        if (lvl > minDb)
        {
            juce::ColourGradient cg (juce::Colour (0xff1aa3c9), 0, inner.getBottom(), juce::Colour (0xffff3b2f), 0, inner.getY(), false);
            cg.addColour (frac (-18.0f), juce::Colour (0xff35d6ff));
            cg.addColour (frac (-6.0f), juce::Colour (0xffd8f6ff));
            cg.addColour (frac (-1.0f), juce::Colour (0xffffd36a));
            g.setGradientFill (cg);
            g.setOpacity (dim ? 0.35f : 1.0f);
            g.fillRect (track.withTop (top));
            g.setOpacity (1.0f);
        }
        g.setColour (juce::Colours::black.withAlpha (0.45f));     // fatias digitais
        for (float yy = inner.getBottom() - 3.0f; yy > inner.getY(); yy -= 3.0f)
            g.drawHorizontalLine ((int) yy, bx, bx + barW);
        if (hold > minDb)
        {
            g.setColour (hold > -1.0f ? juce::Colour (0xffff5a4a) : juce::Colour (0xffe8fbff));
            g.fillRect (bx, yOf (hold) - 1.0f, barW, 2.0f);
        }
    };
    drawBar (xInL, in[0], inHold[0], false);
    drawBar (xInR, in[1], inHold[1], false);
    drawBar (xOutL, out[0], outHold[0], bypass);
    drawBar (xOutR, out[1], outHold[1], bypass);

    // redução de ganho: desce do topo em âmbar (0..-12 dB ocupando a altura toda)
    {
        auto track = juce::Rectangle<float> (xGr, inner.getY(), grW, inner.getHeight());
        g.setColour (juce::Colour (0xff1c140a));
        g.fillRect (track);
        const float f = juce::jlimit (0.0f, 1.0f, -gr / 12.0f);
        juce::ColourGradient ag (juce::Colour (0xffffc04a), 0, inner.getY(), juce::Colour (0xffff7a1a), 0, inner.getBottom(), false);
        g.setGradientFill (ag);
        g.fillRect (track.withHeight (inner.getHeight() * f));
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        for (float yy = inner.getY() + 3.0f; yy < inner.getBottom(); yy += 3.0f)
            g.drawHorizontalLine ((int) yy, xGr, xGr + grW);
        const float hf = juce::jlimit (0.0f, 1.0f, -grHold / 12.0f);
        if (hf > 0.0f)
        {
            g.setColour (juce::Colour (0xffffe3a0));
            g.fillRect (xGr, inner.getY() + inner.getHeight() * hf - 1.0f, grW, 2.0f);
        }
    }

    // marca do teto (ceiling) sobre as barras de saída
    if (! bypass)
    {
        const float y = yOf (ceiling);
        g.setColour (colours::amber);
        g.drawLine (xOutL - 2.0f, y, xOutR + barW + 2.0f, y, 1.4f);
        juce::Path tri;
        tri.addTriangle (xOutR + barW + 2.0f, y, xOutR + barW + 8.0f, y - 4.0f, xOutR + barW + 8.0f, y + 4.0f);
        g.fillPath (tri);
    }

    // rótulos
    g.setFont (stencil (9.0f, 0.1f));
    g.setColour (juce::Colour (0xffa9c4d1));
    g.drawText ("IN",  juce::Rectangle<float> (xInL, labels.getY(), barW * 2.0f + gap, labels.getHeight()), juce::Justification::centred);
    g.drawText ("GR",  juce::Rectangle<float> (xGr - 4.0f, labels.getY(), grW + 8.0f, labels.getHeight()), juce::Justification::centred);
    g.drawText ("OUT", juce::Rectangle<float> (xOutL, labels.getY(), barW * 2.0f + gap, labels.getHeight()), juce::Justification::centred);

    // leituras numéricas
    auto num = [] (float d) { return d <= -99.0f ? juce::String ("-inf") : juce::String (d, 1); };
    auto box = [&] (float bx, float bw, const juce::String& t, juce::Colour c)
    {
        auto r = juce::Rectangle<float> (bx, readout.getY(), bw, readout.getHeight());
        g.setColour (juce::Colour (0xff02090d));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colour (0xff2b4655));
        g.drawRoundedRectangle (r, 3.0f, 0.8f);
        g.setColour (c);
        g.setFont (mono (10.0f));
        g.drawText (t, r, juce::Justification::centred);
    };
    const float pIn = juce::jmax (inHold[0], inHold[1]), pOut = juce::jmax (outHold[0], outHold[1]);
    box (xInL, barW * 2.0f + gap, num (pIn), pIn > -1.0f ? juce::Colour (0xffff7a6a) : juce::Colour (0xff9fe9ff));
    box (xGr - 6.0f, grW + 12.0f, juce::String (grHold, 1), colours::amber);
    box (xOutL, barW * 2.0f + gap, num (pOut), juce::Colour (0xff9fe9ff));

    if (bypass)
    {
        g.setColour (juce::Colour (0xccff4a35));
        g.setFont (stencil (12.0f, 0.3f));
        g.drawText ("BYPASS", inner.withSizeKeepingCentre (inner.getWidth(), 20.0f), juce::Justification::centred);
    }
}

//==============================================================================
void CrtScope::update()
{
    int n;
    while ((n = proc.scopeFifo.pull (temp.data(), (int) temp.size())) > 0)
        for (int i = 0; i < n; ++i)
        {
            ring[(size_t) writePos] = temp[(size_t) i];
            writePos = (writePos + 1) % ringSize;
        }

    // gatilho na subida pelo zero para a onda ficar parada
    int start = (writePos - displayLen - 1 + ringSize * 2) % ringSize;
    for (int k = 0; k < 2048; ++k)
    {
        const int i = (start - k + ringSize) % ringSize, prev = (i - 1 + ringSize) % ringSize;
        if (ring[(size_t) prev] < 0.0f && ring[(size_t) i] >= 0.0f) { start = i; break; }
    }

    prev2 = prev1; prev1 = trace;
    const int cols = 200;
    trace.assign ((size_t) cols * 2, 0.0f);
    for (int c = 0; c < cols; ++c)
    {
        float lo = 1e9f, hi = -1e9f;
        const int a = c * displayLen / cols, e = (c + 1) * displayLen / cols;
        for (int i = a; i < e; ++i)
        {
            const float v = ring[(size_t) ((start + i) % ringSize)];
            lo = juce::jmin (lo, v); hi = juce::jmax (hi, v);
        }
        trace[(size_t) c * 2] = lo; trace[(size_t) c * 2 + 1] = hi;
    }
    repaint();
}

void CrtScope::paint (juce::Graphics& g)
{
    const auto screen = drawCrtFrame (g, getLocalBounds().toFloat());
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (screen.toNearestInt());
        g.setColour (colours::phosphor.withAlpha (0.12f));
        for (int i = 1; i < 8; ++i)
            g.drawVerticalLine (juce::roundToInt (screen.getX() + screen.getWidth() * (float) i / 8.0f), screen.getY(), screen.getBottom());
        for (int i = 1; i < 4; ++i)
            g.drawHorizontalLine (juce::roundToInt (screen.getY() + screen.getHeight() * (float) i / 4.0f), screen.getX(), screen.getRight());
        g.setColour (colours::phosphor.withAlpha (0.22f));
        g.drawHorizontalLine (juce::roundToInt (screen.getCentreY()), screen.getX(), screen.getRight());

        auto drawTrace = [&] (const std::vector<float>& t, float alpha, bool glow)
        {
            if (t.size() < 4) return;
            const int cols = (int) t.size() / 2;
            const float cy = screen.getCentreY(), amp = screen.getHeight() * 0.44f;
            juce::Path p;
            for (int c = 0; c < cols; ++c)
            {
                const float x = screen.getX() + 4.0f + (screen.getWidth() - 8.0f) * (float) c / (float) (cols - 1);
                const float y = cy - amp * juce::jlimit (-1.0f, 1.0f, 0.5f * (t[(size_t) c * 2] + t[(size_t) c * 2 + 1]));
                if (c == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            if (glow)
            {
                g.setColour (colours::phosphor.withAlpha (0.16f * alpha));
                g.strokePath (p, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            g.setColour (colours::phosphor.withAlpha (alpha));
            g.strokePath (p, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        drawTrace (prev2, 0.15f, false);
        drawTrace (prev1, 0.30f, false);
        drawTrace (trace, 1.0f, true);
    }
    crtGlass (g, screen);
}

//==============================================================================
void TapeReels::update (bool on, float driveDb, float level)
{
    // motor: parte e para com inércia; a fita corre de cima para baixo e volta ao fim do rolo
    const float target = on ? 0.16f : 0.0f;
    speed += (target - speed) * (target > speed ? 0.05f : 0.035f);
    angle += speed;
    if (angle > juce::MathConstants<float>::twoPi * 1000.0f) angle = 0.0f;
    progress += speed * 0.00035f;
    if (progress > 1.0f) progress = 0.0f;

    const float driveNorm = juce::jlimit (0.0f, 1.0f, driveDb / 36.0f);
    const float g = on ? juce::jlimit (0.0f, 1.0f, juce::jmin (1.2f, level) * (0.4f + 0.6f * driveNorm)) : 0.0f;
    glow += (g - glow) * (g > glow ? 0.35f : 0.08f);
    repaint();
}

static void drawReel (juce::Graphics& g, juce::Point<float> c, float r, float pack, float angle)
{
    // sombra
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c).translated (1.5f, 2.5f));

    // bolo de fita marrom (visto através do flange)
    const float pr = r * pack;
    juce::ColourGradient tape (juce::Colour (0xff5a3a22), c.x, c.y, juce::Colour (0xff23150c), c.x + pr, c.y, true);
    g.setGradientFill (tape);
    g.fillEllipse (juce::Rectangle<float> (pr * 2.0f, pr * 2.0f).withCentre (c));
    g.setColour (juce::Colour (0xff7a5634).withAlpha (0.35f));
    for (float rr = r * 0.3f; rr < pr; rr += 2.2f)
        g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (c), 0.5f);

    // flange de alumínio com três janelas que giram
    juce::Path flange;
    flange.addEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
    for (int k = 0; k < 3; ++k)
    {
        const float a0 = angle + juce::MathConstants<float>::twoPi * (float) k / 3.0f;
        juce::Path win;
        win.addPieSegment (juce::Rectangle<float> (r * 1.74f, r * 1.74f).withCentre (c), a0 + 0.22f, a0 + 1.72f, 0.36f);
        flange.addPath (win);
    }
    flange.setUsingNonZeroWinding (false);
    juce::ColourGradient al (juce::Colour (0xffe6e4de), c.x - r, c.y - r, juce::Colour (0xff6b6862), c.x + r, c.y + r, false);
    al.addColour (0.45, juce::Colour (0xffbab7b0));
    g.setGradientFill (al);
    g.fillPath (flange);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.strokePath (flange, juce::PathStrokeType (0.8f));

    // reflexo fixo (a luz não gira com o rolo)
    juce::ColourGradient sh (juce::Colours::white.withAlpha (0.28f), c.x - r * 0.6f, c.y - r * 0.8f,
                             juce::Colours::transparentWhite, c.x, c.y, true);
    g.setGradientFill (sh);
    g.fillEllipse (juce::Rectangle<float> (r * 1.6f, r * 1.0f).withCentre ({ c.x - r * 0.25f, c.y - r * 0.45f }));

    // cubo com três dentes
    const float hr = r * 0.2f;
    g.setColour (juce::Colour (0xff1a1714));
    g.fillEllipse (juce::Rectangle<float> (hr * 2.0f, hr * 2.0f).withCentre (c));
    g.setColour (juce::Colour (0xffd8d6d0));
    for (int k = 0; k < 3; ++k)
    {
        const float a = angle + juce::MathConstants<float>::twoPi * (float) k / 3.0f;
        juce::Path tooth;
        tooth.addRectangle (-1.3f, -hr, 2.6f, hr * 0.7f);
        g.fillPath (tooth, juce::AffineTransform::rotation (a).translated (c.x, c.y));
    }
}

void TapeReels::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    const float r = juce::jmin (b.getWidth() * 0.37f, b.getHeight() * 0.23f);
    const float cx = b.getX() + r + 3.0f;
    const juce::Point<float> top (cx, b.getY() + r + 4.0f), bot (cx, b.getBottom() - r - 4.0f);

    // carretel de cima esvazia, o de baixo enche
    const float packTop = 0.45f + 0.45f * (1.0f - progress), packBot = 0.45f + 0.45f * progress;

    // caminho da fita: sai do rolo de cima, passa pelas guias e pela cabeça, desce para o de baixo
    const auto head = juce::Rectangle<float> (16.0f, 30.0f).withCentre ({ b.getRight() - 9.0f, b.getCentreY() });
    const juce::Point<float> guideT (b.getRight() - 6.0f, top.y - r * 0.2f), guideB (b.getRight() - 6.0f, bot.y + r * 0.2f);
    juce::Path tp;
    tp.startNewSubPath (top.x + r * packTop, top.y);
    tp.lineTo (guideT);
    tp.lineTo (head.getX() - 1.0f, head.getY() - 4.0f);
    tp.lineTo (head.getX() - 1.0f, head.getBottom() + 4.0f);
    tp.lineTo (guideB);
    tp.lineTo (bot.x + r * packBot, bot.y);
    g.setColour (juce::Colour (0xff3d2716));
    g.strokePath (tp, juce::PathStrokeType (2.6f));
    g.setColour (juce::Colour (0xff8a6040).withAlpha (0.5f));
    g.strokePath (tp, juce::PathStrokeType (0.8f));

    // guias cromadas
    for (auto gp : { guideT, guideB })
    {
        juce::ColourGradient cg (juce::Colour (0xfff0eee8), gp.x - 4, gp.y - 4, juce::Colour (0xff5e5b55), gp.x + 4, gp.y + 4, false);
        g.setGradientFill (cg);
        g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (gp));
    }

    // bloco da cabeça de gravação; a fenda brilha com a saturação
    juce::ColourGradient hg (juce::Colour (0xffd8d6d0), head.getX(), head.getY(), juce::Colour (0xff4a4844), head.getRight(), head.getBottom(), false);
    g.setGradientFill (hg);
    g.fillRoundedRectangle (head, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (head, 3.0f, 0.8f);
    const auto gap = juce::Rectangle<float> (2.0f, head.getHeight() - 8.0f).withCentre ({ head.getX() + 3.0f, head.getCentreY() });
    if (glow > 0.02f)
    {
        g.setColour (colours::amber.withAlpha (0.35f * glow));
        g.fillEllipse (gap.expanded (6.0f + 6.0f * glow, 4.0f));
    }
    g.setColour (colours::amber.interpolatedWith (juce::Colour (0xfffff1c8), glow).withAlpha (0.25f + 0.75f * glow));
    g.fillRect (gap);

    drawReel (g, top, r, packTop, -angle);
    drawReel (g, bot, r, packBot, -angle * (packTop / juce::jmax (0.3f, packBot)));

    // lâmpada REC
    const auto lamp = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ head.getCentreX(), head.getY() - 14.0f });
    if (speed > 0.02f)
    {
        g.setColour (colours::redLamp.withAlpha (0.35f));
        g.fillEllipse (lamp.expanded (4.0f));
        g.setColour (colours::redLamp);
    }
    else
        g.setColour (juce::Colour (0xff4a2420));
    g.fillEllipse (lamp);
}

//==============================================================================
EnvelopeEditor::EnvelopeEditor (DubBassAudioProcessor& p)
    : proc (p),
      pa (p.apvts.getParameter (ParamID::attack)),  pd (p.apvts.getParameter (ParamID::decay)),
      ps (p.apvts.getParameter (ParamID::sustain)), pr (p.apvts.getParameter (ParamID::release))
{
    setRepaintsOnMouseActivity (true);
}

bool EnvelopeEditor::is808() const
{
    return juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::mode)->load()) == 1;
}

EnvelopeEditor::Geo EnvelopeEditor::geometry() const
{
    Geo q;
    q.plot = getLocalBounds().toFloat().reduced (10.0f).reduced (14.0f, 0.0f).withTrimmedTop (16.0f).withTrimmedBottom (24.0f);
    q.segW = q.plot.getWidth() / 3.5f;              // ataque, decay e release até 1 segmento; sustain fixo 0.5
    q.x0 = q.plot.getX();
    q.xA = q.x0 + juce::jmax (0.015f, pa->getValue()) * q.segW;
    q.xD = q.xA + juce::jmax (0.015f, pd->getValue()) * q.segW;
    q.xS = q.xD + 0.5f * q.segW;
    q.xR = q.xS + juce::jmax (0.015f, pr->getValue()) * q.segW;
    q.yTop = q.plot.getY();
    q.yBot = q.plot.getBottom();
    q.ySus = q.yBot - (is808() ? 0.0f : ps->getValue()) * (q.yBot - q.yTop);
    return q;
}

int EnvelopeEditor::hitTest (juce::Point<float> m) const
{
    const auto q = geometry();
    const juce::Point<float> pts[] = { { q.xA, q.yTop }, { q.xD, q.ySus }, { q.xR, q.yBot } };
    int best = -1;
    float bestD = 18.0f;
    for (int i = 0; i < 3; ++i)
        if (const float d = pts[i].getDistanceFrom (m); d < bestD) { bestD = d; best = i; }
    return best;
}

void EnvelopeEditor::paint (juce::Graphics& g)
{
    const auto screen = drawCrtFrame (g, getLocalBounds().toFloat());
    const auto q = geometry();
    const bool e808 = is808();
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (screen.toNearestInt());

        g.setColour (colours::phosphor.withAlpha (0.10f));
        for (int i = 1; i < 7; ++i)
            g.drawVerticalLine (juce::roundToInt (screen.getX() + screen.getWidth() * (float) i / 7.0f), screen.getY(), screen.getBottom());
        for (int i = 1; i < 4; ++i)
            g.drawHorizontalLine (juce::roundToInt (q.yTop + (q.yBot - q.yTop) * (float) i / 4.0f), screen.getX(), screen.getRight());

        // curva: ataque reto, decay e release exponenciais (como no som)
        juce::Path env;
        env.startNewSubPath (q.x0, q.yBot);
        env.lineTo (q.xA, q.yTop);
        env.cubicTo (q.xA + (q.xD - q.xA) * 0.15f, q.ySus - (q.ySus - q.yTop) * 0.15f,
                     q.xA + (q.xD - q.xA) * 0.35f, q.ySus, q.xD, q.ySus);
        env.lineTo (q.xS, q.ySus);
        env.cubicTo (q.xS + (q.xR - q.xS) * 0.15f, q.yBot - (q.yBot - q.ySus) * 0.15f,
                     q.xS + (q.xR - q.xS) * 0.35f, q.yBot, q.xR, q.yBot);
        env.lineTo (screen.getRight(), q.yBot);

        juce::Path fill (env);
        fill.lineTo (screen.getRight(), q.yBot);
        fill.lineTo (q.x0, q.yBot);
        fill.closeSubPath();
        g.setColour (colours::phosphor.withAlpha (0.10f));
        g.fillPath (fill);
        g.setColour (colours::phosphor.withAlpha (0.18f));
        g.strokePath (env, juce::PathStrokeType (6.0f));
        g.setColour (colours::phosphor);
        g.strokePath (env, juce::PathStrokeType (1.8f));

        // marca do note-off
        g.setColour (colours::phosphor.withAlpha (0.35f));
        for (float y = q.yTop; y < q.yBot; y += 6.0f)
            g.drawVerticalLine ((int) q.xS, y, y + 3.0f);

        // pontos arrastáveis
        const juce::Point<float> pts[] = { { q.xA, q.yTop }, { q.xD, q.ySus }, { q.xR, q.yBot } };
        const char* names[] = { "A", e808 ? "D" : "D/S", "R" };
        for (int i = 0; i < 3; ++i)
        {
            const bool hot = i == dragging || (dragging < 0 && i == hover);
            const auto c = pts[i];
            g.setColour (colours::amber.withAlpha (hot ? 0.4f : 0.22f));
            g.fillEllipse (juce::Rectangle<float> (hot ? 22.0f : 16.0f, hot ? 22.0f : 16.0f).withCentre (c));
            g.setColour (colours::amber);
            g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (c));
            g.setFont (stencil (9.0f, 0.1f));
            g.drawText (names[i], juce::Rectangle<float> (30.0f, 12.0f).withCentre ({ c.x, i == 0 ? c.y - 12.0f : c.y - 13.0f }).translated (i == 2 ? -6.0f : 0.0f, i == 0 ? 2.0f : 0.0f),
                        juce::Justification::centred);
        }

        // leitura dos valores
        g.setFont (mono (9.5f));
        g.setColour (colours::phosphor.withAlpha (0.85f));
        const juce::String txt = "A " + pa->getCurrentValueAsText() + "   D " + pd->getCurrentValueAsText()
                               + (e808 ? juce::String ("   S --") : "   S " + ps->getCurrentValueAsText())
                               + "   R " + pr->getCurrentValueAsText();
        g.drawText (txt, screen.withTop (screen.getBottom() - 18.0f), juce::Justification::centred);
        if (e808)
        {
            g.setColour (colours::amber.withAlpha (0.8f));
            g.drawText (juce::String::fromUTF8 ("808: decai até zero"), screen.reduced (8.0f, 3.0f).withHeight (12.0f), juce::Justification::topRight);
        }
    }
    crtGlass (g, screen);
}

void EnvelopeEditor::mouseMove (const juce::MouseEvent& e)
{
    const int h = hitTest (e.position);
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void EnvelopeEditor::mouseExit (const juce::MouseEvent&) { hover = -1; repaint(); }

void EnvelopeEditor::mouseDown (const juce::MouseEvent& e)
{
    dragging = hitTest (e.position);
    if (dragging == 0) pa->beginChangeGesture();
    if (dragging == 1) { pd->beginChangeGesture(); if (! is808()) ps->beginChangeGesture(); }
    if (dragging == 2) pr->beginChangeGesture();
}

void EnvelopeEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0) return;
    const auto q = geometry();
    const auto m = e.position;
    auto set = [] (juce::RangedAudioParameter* p, float v) { p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, v)); };
    if (dragging == 0) set (pa, (m.x - q.x0) / q.segW);
    if (dragging == 1)
    {
        set (pd, (m.x - q.xA) / q.segW);
        if (! is808()) set (ps, (q.yBot - m.y) / (q.yBot - q.yTop));
    }
    if (dragging == 2) set (pr, (m.x - q.xS) / q.segW);
    repaint();
}

void EnvelopeEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragging == 0) pa->endChangeGesture();
    if (dragging == 1) { pd->endChangeGesture(); if (! is808()) ps->endChangeGesture(); }
    if (dragging == 2) pr->endChangeGesture();
    dragging = -1;
    repaint();
}

void EnvelopeEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    // duplo clique no ponto volta ao padrão
    auto reset = [] (juce::RangedAudioParameter* p) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture(); };
    switch (hitTest (e.position))
    {
        case 0: reset (pa); break;
        case 1: reset (pd); reset (ps); break;
        case 2: reset (pr); break;
        default: break;
    }
    repaint();
}

//==============================================================================
float CrtEqCurve::xFor (double f) const
{
    const auto s = screenArea();
    return s.getX() + s.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0));
}

float CrtEqCurve::yFor (float db) const
{
    const auto s = screenArea();
    return juce::jmap (juce::jlimit (-15.0f, 15.0f, db), -15.0f, 15.0f, s.getBottom() - 4.0f, s.getY() + 4.0f);
}

int CrtEqCurve::nearestBand (float x) const
{
    int best = 0;
    float bestD = 1.0e9f;
    for (int i = 0; i < DubBassAudioProcessor::numEqBands; ++i)
        if (const float d = std::abs (xFor (DubBassAudioProcessor::eqFrequencies[(size_t) i]) - x); d < bestD) { bestD = d; best = i; }
    return best;
}

void CrtEqCurve::mouseMove (const juce::MouseEvent& e)  { hover = nearestBand (e.position.x); repaint(); }
void CrtEqCurve::mouseExit (const juce::MouseEvent&)    { hover = -1; repaint(); }

void CrtEqCurve::mouseDown (const juce::MouseEvent& e)
{
    dragging = nearestBand (e.position.x);
    if (auto* p = proc.apvts.getParameter (DubBassAudioProcessor::eqParamID (dragging)))
        p->beginChangeGesture();
    mouseDrag (e);
}

void CrtEqCurve::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0) return;
    const auto s = screenArea();
    const float db = juce::jmap (e.position.y, s.getBottom() - 4.0f, s.getY() + 4.0f, -15.0f, 15.0f);
    if (auto* p = proc.apvts.getParameter (DubBassAudioProcessor::eqParamID (dragging)))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (-12.0f, 12.0f, db)));
    repaint();
}

void CrtEqCurve::mouseUp (const juce::MouseEvent&)
{
    if (dragging >= 0)
        if (auto* p = proc.apvts.getParameter (DubBassAudioProcessor::eqParamID (dragging)))
            p->endChangeGesture();
    dragging = -1;
}

void CrtEqCurve::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (auto* p = proc.apvts.getParameter (DubBassAudioProcessor::eqParamID (nearestBand (e.position.x))))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (0.0f));
        p->endChangeGesture();
    }
}

void CrtEqCurve::paint (juce::Graphics& g)
{
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    const auto screen = drawCrtFrame (g, getLocalBounds().toFloat());


    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (screen.toNearestInt());

    g.setColour (colours::phosphor.withAlpha (0.12f));
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
        g.drawVerticalLine ((int) xFor (f), screen.getY(), screen.getBottom());
    for (float db : { -12.0f, -6.0f, 6.0f, 12.0f })
        g.drawHorizontalLine ((int) yFor (db), screen.getX(), screen.getRight());
    g.setColour (colours::phosphor.withAlpha (0.25f));
    g.drawHorizontalLine ((int) yFor (0.0f), screen.getX(), screen.getRight());

    const double sr    = proc.getSampleRateSafe();
    const bool eqOn    = proc.apvts.getRawParameterValue (ParamID::eqOn)->load() > 0.5f;
    const bool boostOn = proc.apvts.getRawParameterValue (ParamID::boost)->load() > 0.5f;

    std::vector<Coeffs::Ptr> filters;
    if (eqOn)
        for (int i = 0; i < DubBassAudioProcessor::numEqBands; ++i)
        {
            const float gainDb = proc.apvts.getRawParameterValue (DubBassAudioProcessor::eqParamID (i))->load();
            if (std::abs (gainDb) > 0.01f)
                filters.push_back (Coeffs::makePeakFilter (sr, std::min (DubBassAudioProcessor::eqFrequencies[(size_t) i], (float) (sr * 0.45)),
                                                           DubBassAudioProcessor::eqQ, juce::Decibels::decibelsToGain (gainDb)));
        }
    if (boostOn)
        filters.push_back (Coeffs::makePeakFilter (sr, DubBassAudioProcessor::boostFreq, DubBassAudioProcessor::boostQ,
                                                   juce::Decibels::decibelsToGain (DubBassAudioProcessor::boostGainDb)));

    juce::Path curve;
    const int steps = juce::jmax (2, (int) screen.getWidth());
    for (int i = 0; i <= steps; ++i)
    {
        const double f = 20.0 * std::pow (1000.0, (double) i / steps);
        double mag = 1.0;
        for (auto& c : filters) mag *= c->getMagnitudeForFrequency (f, sr);
        const float x = screen.getX() + screen.getWidth() * (float) i / (float) steps;
        const float y = yFor ((float) juce::Decibels::gainToDecibels (mag));
        if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
    }
    const float alpha = eqOn || boostOn ? 1.0f : 0.35f;
    juce::Path fill (curve);
    fill.lineTo (screen.getRight(), yFor (0.0f));
    fill.lineTo (screen.getX(), yFor (0.0f));
    fill.closeSubPath();
    g.setColour (colours::phosphor.withAlpha (0.10f * alpha));
    g.fillPath (fill);
    g.setColour (colours::phosphor.withAlpha (0.16f * alpha));
    g.strokePath (curve, juce::PathStrokeType (6.0f));
    g.setColour (colours::phosphor.withAlpha (alpha));
    g.strokePath (curve, juce::PathStrokeType (1.7f));

    g.setFont (mono (9.0f));
    g.setColour (colours::phosphor.withAlpha (0.55f));
    g.drawText ("100", juce::Rectangle<float> (xFor (100.0) + 3.0f, screen.getBottom() - 14.0f, 30.0f, 12.0f), juce::Justification::left);
    g.drawText ("1k",  juce::Rectangle<float> (xFor (1000.0) + 3.0f, screen.getBottom() - 14.0f, 30.0f, 12.0f), juce::Justification::left);
    g.drawText ("10k", juce::Rectangle<float> (xFor (10000.0) + 3.0f, screen.getBottom() - 14.0f, 30.0f, 12.0f), juce::Justification::left);
    if (boostOn)
    {
        g.setColour (colours::amber);
        g.drawText ("BOOST +4 dB @ 60 Hz", screen.reduced (8.0f, 4.0f), juce::Justification::topRight);
    }

    // pontos das 10 bandas (arrastáveis)
    for (int i = 0; i < DubBassAudioProcessor::numEqBands; ++i)
    {
        const float gainDb = proc.apvts.getRawParameterValue (DubBassAudioProcessor::eqParamID (i))->load();
        const juce::Point<float> c (xFor (DubBassAudioProcessor::eqFrequencies[(size_t) i]), yFor (gainDb));
        const bool hot = i == dragging || (dragging < 0 && i == hover);
        if (hot)
        {
            g.setColour (colours::amber.withAlpha (0.3f));
            g.fillEllipse (juce::Rectangle<float> (18.0f, 18.0f).withCentre (c));
            const float hz = DubBassAudioProcessor::eqFrequencies[(size_t) i];
            g.setColour (colours::amber);
            g.setFont (mono (9.0f));
            g.drawText ((hz < 1000.0f ? juce::String (juce::roundToInt (hz)) + " Hz " : juce::String (hz / 1000.0f, 0) + " kHz ")
                            + (gainDb > 0.0f ? "+" : "") + juce::String (gainDb, 1) + " dB",
                        screen.reduced (8.0f, 4.0f), juce::Justification::topLeft);
        }
        g.setColour (hot ? colours::amber : colours::phosphor.withAlpha (std::abs (gainDb) > 0.05f ? 0.95f : 0.45f));
        g.fillEllipse (juce::Rectangle<float> (hot ? 8.0f : 6.0f, hot ? 8.0f : 6.0f).withCentre (c));
    }
}

//==============================================================================
DubKeyboard::DubKeyboard (juce::MidiKeyboardState& st)
    : juce::MidiKeyboardComponent (st, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setAvailableRange (24, 60);          // C1..C4: a região do baixo (22 teclas brancas)
    setOctaveForMiddleC (4);             // nota 60 = C4
    setScrollButtonsVisible (false);
    setWantsKeyboardFocus (false);
    setMidiChannel (1);
    setVelocity (0.8f, true);            // mais embaixo na tecla = mais forte
    setBlackNoteLengthProportion (0.62f);
    setBlackNoteWidthProportion (0.62f);
    setColour (whiteNoteColourId, juce::Colour (0xff0e0c0a));   // fundo do encaixe
    setColour (keySeparatorLineColourId, juce::Colours::transparentBlack);
    setColour (shadowColourId, juce::Colours::transparentBlack);
}

static void keyLamp (juce::Graphics& g, juce::Point<float> c)
{
    const auto lamp = juce::Rectangle<float> (7.0f, 7.0f).withCentre (c);
    g.setColour (colours::amber.withAlpha (0.35f));
    g.fillEllipse (lamp.expanded (4.0f));
    juce::ColourGradient lit (juce::Colour (0xffffe3a0), lamp.getCentreX(), lamp.getCentreY(),
                              colours::amber.darker (0.2f), lamp.getRight(), lamp.getBottom(), true);
    g.setGradientFill (lit);
    g.fillEllipse (lamp);
}

void DubKeyboard::drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                                 juce::Colour, juce::Colour)
{
    auto r = area.reduced (1.0f, 0.0f).withTrimmedTop (isDown ? 3.0f : 0.0f);   // tecla afundada
    const float rad = 3.0f;
    juce::ColourGradient iv (juce::Colour (isDown ? 0xffe9d9ae : 0xfff6ecd2).brighter (isOver && ! isDown ? 0.04f : 0.0f), r.getX(), r.getY(),
                             juce::Colour (isDown ? 0xffc2ad78 : 0xffd9c69a), r.getX(), r.getBottom(), false);
    g.setGradientFill (iv);
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colour (0xff3d3124).withAlpha (0.8f));
    g.drawRoundedRectangle (r, rad, 1.0f);
    if (! isDown)
    {
        g.setColour (juce::Colour (0xffb39d6a));                                  // frente da tecla
        g.fillRect (r.withTop (r.getBottom() - 6.0f).reduced (1.0f, 0.0f));
    }
    else
    {
        g.setColour (colours::amber.withAlpha (0.20f));
        g.fillRoundedRectangle (r, rad);
        keyLamp (g, { r.getCentreX(), r.getBottom() - 10.0f });
    }
    if (note % 12 == 0)                                                           // C1, C2, C3, C4
    {
        g.setColour (isDown ? juce::Colour (0xff6b3f05) : colours::inkSoft);
        g.setFont (stencil (10.0f, 0.02f));
        g.drawText (juce::MidiMessage::getMidiNoteName (note, true, true, 4),
                    r.withTop (r.getBottom() - (isDown ? 34.0f : 26.0f)).withHeight (14.0f), juce::Justification::centred);
    }
}

void DubKeyboard::drawBlackNote (int, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver, juce::Colour)
{
    auto r = area.withTrimmedTop (isDown ? 2.0f : 0.0f);
    const float rad = 3.0f;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (1.5f, 2.0f), rad);
    juce::ColourGradient bk (juce::Colour (isDown ? 0xff2a2520 : 0xff3d3833).brighter (isOver && ! isDown ? 0.12f : 0.0f), r.getX(), r.getY(),
                             juce::Colour (0xff070605), r.getRight(), r.getBottom(), false);
    g.setGradientFill (bk);
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (isDown ? 0.05f : 0.14f));
    g.fillRoundedRectangle (r.reduced (3.0f, 2.0f).withHeight (r.getHeight() * 0.55f), 2.0f);
    if (isDown)
        keyLamp (g, { r.getCentreX(), r.getBottom() - 8.0f });
}

void DubKeyboard::paint (juce::Graphics& g)
{
    MidiKeyboardComponent::paint (g);
    auto b = getLocalBounds().toFloat();
    auto bar = b.withHeight (7.0f);                                               // barra cromada
    juce::ColourGradient chrome (juce::Colour (0xffd8d6d0), bar.getX(), bar.getY(), juce::Colour (0xff5e5b55), bar.getX(), bar.getBottom(), false);
    g.setGradientFill (chrome);
    g.fillRect (bar);
    g.setColour (juce::Colour (0xff5a1414));                                      // feltro vermelho
    g.fillRect (b.withY (bar.getBottom()).withHeight (2.0f));
}

//==============================================================================
DubBassPanel::DubBassPanel (DubBassAudioProcessor& p)
    : proc (p),
      modeKeys (*p.apvts.getParameter (ParamID::mode), { "BASS", "SUPER TONE", "808" }, { 0, 2, 1 }),
      envEditor (p),
      env808Presets (p),
      drvTypeKeys (*p.apvts.getParameter (ParamID::drvType), { "TUBE", "SOFT", "FUZZ", "FITA" }),
      eqCurve (p), scope (p), keyboard (p.keyboardState), meter (p)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (modeKeys);
    setupKnob (harm2,     ParamID::harm2,     "HARM 2");
    setupKnob (harm3,     ParamID::harm3,     "HARM 3");
    setupKnob (glide,     ParamID::glide,     "GLIDE");
    setupKnob (pitchAmt,  ParamID::pitchAmt,  "808 PITCH");
    setupKnob (pitchTime, ParamID::pitchTime, "808 TEMPO");

    addAndMakeVisible (envEditor);
    addChildComponent (env808Presets);
    setupKnob (cutoff,  ParamID::cutoff,  "CUTOFF");
    setupKnob (reso,    ParamID::reso,    "RESSON.");

    setupToggle (drvOn, ParamID::drvOn, "ON");
    addAndMakeVisible (drvTypeKeys);
    setupKnob (drive,   ParamID::drive,   "DRIVE");
    setupKnob (drvTone, ParamID::drvTone, "TOM");
    setupKnob (drvMix,  ParamID::drvMix,  "MIX");
    addAndMakeVisible (tube);
    addChildComponent (reels);

    setupToggle (eqOn, ParamID::eqOn, "ON");
    addAndMakeVisible (eqCurve);
    for (int b = 0; b < DubBassAudioProcessor::numEqBands; ++b)
    {
        auto& f = eqFaders[(size_t) b];
        f.setSliderStyle (juce::Slider::LinearVertical);
        f.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        f.setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (f);
        eqAtts[(size_t) b] = std::make_unique<SliderAttach> (proc.apvts, DubBassAudioProcessor::eqParamID (b), f);
        f.setDoubleClickReturnValue (true, 0.0);
    }

    setupToggle (revOn, ParamID::revOn, "ON");
    setupKnob (revSize,   ParamID::revSize,   "TAMANHO");
    setupKnob (revDamp,   ParamID::revDamp,   "DAMP");
    setupKnob (revWidth,  ParamID::revWidth,  "ABERTURA");
    setupKnob (revLowCut, ParamID::revLowCut, "LOW CUT");
    setupKnob (revMix,    ParamID::revMix,    "MIX");

    boostBtn.setClickingTogglesState (true);
    boostBtn.setTooltip ("60 Hz +4 dB, antes do maximizer");
    addAndMakeVisible (boostBtn);
    boostAtt = std::make_unique<ButtonAttach> (proc.apvts, ParamID::boost, boostBtn);

    setupKnob (output, ParamID::output, "VOLUME");
    addAndMakeVisible (scope);
    addAndMakeVisible (keyboard);

    addAndMakeVisible (meter);
    setupKnob (maxGain,    ParamID::maxGain,    "GANHO",   true);
    setupKnob (maxCeiling, ParamID::maxCeiling, "TETO",    true);
    setupKnob (maxRelease, ParamID::maxRelease, "RELEASE", true);
    setupToggle (maxBypass, ParamID::maxBypass, "BYPASS");
    maxBypass.button.getProperties().set ("redLamp", true);

    setSize (1300, 980);
    timerCallback();
    startTimerHz (30);
}

DubBassPanel::~DubBassPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DubBassPanel::setupKnob (Knob& k, const juce::String& id, const juce::String& name, bool dark)
{
    k.dark = dark;
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (kStartAngle, kEndAngle, true);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 90, 16);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxTextColourId, dark ? colours::amber : colours::inkSoft);
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<SliderAttach> (proc.apvts, id, k.slider);
    if (auto* prm = proc.apvts.getParameter (id))
        k.slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (stencil (11.0f, 0.12f));
    k.label.setColour (juce::Label::textColourId, dark ? colours::cream : colours::ink);
    k.label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k.label);
}

void DubBassPanel::setupToggle (Toggle& t, const juce::String& id, const juce::String& text)
{
    t.button.setButtonText (text);
    addAndMakeVisible (t.button);
    t.att = std::make_unique<ButtonAttach> (proc.apvts, id, t.button);
}

void DubBassPanel::placeKnob (Knob& k, juce::Rectangle<int> r)
{
    k.label.setBounds (r.removeFromTop (15));
    k.slider.setBounds (r);
}

void DubBassPanel::timerCallback()
{
    scope.update();
    meter.update();
    eqCurve.repaint();
    {
        const bool  on    = proc.apvts.getRawParameterValue (ParamID::drvOn)->load() > 0.5f;
        const float drvDb = proc.apvts.getRawParameterValue (ParamID::drive)->load();
        const bool  fita  = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::drvType)->load()) == 3;
        if (tube.isVisible() == fita)  { tube.setVisible (! fita); reels.setVisible (fita); repaint (tubeArea.expanded (10, 22)); }
        if (fita) reels.update (on, drvDb, proc.driveLevel.load());
        else      tube.update (on, drvDb, proc.driveLevel.load());
    }
    envEditor.repaint();
    {
        const bool show808 = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::mode)->load()) == 1;
        if (env808Presets.isVisible() != show808) env808Presets.setVisible (show808);
        if (show808) env808Presets.repaint();
    }

    // knobs do 808 só valem no modo 808; sustain não existe no 808
    const int  mode  = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::mode)->load());
    const bool is808 = mode == 1;
    for (auto* k : { &harm2, &harm3 })      // o Super Tone tem forma de onda própria
    {
        k->slider.setEnabled (mode != 2);
        k->slider.setAlpha (mode == 2 ? 0.4f : 1.0f);
    }
    for (auto* k : { &pitchAmt, &pitchTime })
    {
        k->slider.setEnabled (is808);
        k->slider.setAlpha (is808 ? 1.0f : 0.4f);
    }

    const bool maxOn = proc.apvts.getRawParameterValue (ParamID::maxBypass)->load() < 0.5f;
    for (auto* k : { &maxGain, &maxCeiling, &maxRelease })
        k->slider.setAlpha (maxOn ? 1.0f : 0.45f);

    for (auto [t, ks] : { std::pair<Toggle*, std::vector<Knob*>> { &drvOn, { &drive, &drvTone, &drvMix } },
                          { &revOn, { &revSize, &revDamp, &revWidth, &revLowCut, &revMix } } })
        for (auto* k : ks)
            k->slider.setAlpha (t->button.getToggleState() ? 1.0f : 0.55f);

    repaint (noteDisplay.expanded (6));
}

//==============================================================================
void DubBassPanel::drawKnobScale (juce::Graphics& g, Knob& k, int ticks)
{
    const auto circ = knobCircle (k.slider);
    const auto c = circ.getCentre();
    const float r = circ.getWidth() * 0.5f;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = kStartAngle + (kEndAngle - kStartAngle) * (float) i / (float) juce::jmax (1, ticks - 1);
        const float r1 = r + 3.5f, r2 = r + 8.0f;
        g.setColour ((k.dark ? colours::cream : colours::ink).withAlpha (0.85f));
        g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1,
                    c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, (i == 0 || i == ticks - 1) ? 1.6f : 1.0f);
    }
}

void DubBassPanel::renderBackground()
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
        const juce::Point<float> kc (wx + ww * (0.35f + rng.nextFloat() * 0.3f), (float) getHeight() * (0.25f + rng.nextFloat() * 0.5f));
        for (int k = 5; k >= 1; --k)
        {
            g.setColour (colours::woodDark.darker (0.4f).withAlpha (0.18f));
            g.drawEllipse (juce::Rectangle<float> ((float) k * 2.6f, (float) k * 5.5f).withCentre (kc), 0.9f);
        }
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (juce::Rectangle<float> (3.0f, 6.0f).withCentre (kc));
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

    // --- cabeçalho: nome completo + versão; a fonte encolhe até caber antes da plaqueta
    const float stripesX = (float) plate.getRight() - 250.0f;
    g.setFont (stencil (15.0f, 0.34f));
    const juce::String plateText ("DUB STATION");
    const float plateW = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), plateText) + 64.0f;
    const juce::String title ("MASTOR REGGAE DUB BASS INSTRUMENT MRDBI-07 " + versionShort());
    const float maxW = stripesX - 24.0f - plateW - 26.0f - ((float) plate.getX() + 32.0f);
    float fs = 30.0f;
    while (fs > 16.0f && juce::GlyphArrangement::getStringWidth (stencil (fs, 0.14f), title) > maxW)
        fs -= 0.5f;
    g.setColour (colours::ink);
    g.setFont (stencil (fs, 0.14f));
    g.drawText (title, plate.getX() + 32, 20, (int) maxW + 4, 34, juce::Justification::left);
    const float titleRight = (float) plate.getX() + 32.0f + juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title);

    {   // placa de latão "DUB STATION"
        g.setFont (stencil (15.0f, 0.34f));
        const juce::String t (plateText);
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
        g.setColour (juce::Colour (0xff4a3712).withAlpha (0.55f));
        g.drawRoundedRectangle (pl.reduced (24.0f, 5.0f), 2.0f, 0.8f);
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
    g.drawText (juce::String::fromUTF8 ("REGGAE DUB BASS  ·  BASS / SUPER TONE / 808  ·  DUBMASTOR"), plate.getX() + 34, 52, 600, 14, juce::Justification::left);

    {   // faixas da bandeira da Jamaica, pintadas à mão e envelhecidas
        const float sx = (float) plate.getRight() - 250.0f, sw = 210.0f, sh = 7.0f;
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
    section (oscPanel,   "OSCILADOR");
    section (envPanel,   "ENVELOPE / FILTRO");
    section (scopePanel, juce::String::fromUTF8 ("OSCILOSCÓPIO"));
    section (drvPanel,   juce::String::fromUTF8 ("VÁLVULA / SATURAÇÃO"));
    section (eqPanel,    juce::String::fromUTF8 ("EQ GRÁFICO"));
    section (revPanel,   "REVERB");
    section (boostPanel, "BASS BOOST");
    section (outPanel,   juce::String::fromUTF8 ("SAÍDA"));
    section (keysPanel,  "TECLADO  C1 - C4");

    // trilho escuro onde o teclado está encaixado
    {
        auto rail = keyboard.getBounds().toFloat().expanded (6.0f, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (rail.translated (1, 1), 5.0f, 1.0f);
        g.setColour (juce::Colour (0xff0e0c0a));
        g.fillRoundedRectangle (rail, 5.0f);
        g.setColour (colours::ink.withAlpha (0.6f));
        g.drawRoundedRectangle (rail, 5.0f, 1.2f);
    }

    // linha divisória do envelope / filtro
    {
        const float y = (float) cutoff.label.getY() - 9.0f;
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawHorizontalLine ((int) y + 1, (float) envPanel.getX() + 16.0f, (float) envPanel.getRight() - 16.0f);
        g.setColour (colours::ink.withAlpha (0.35f));
        g.drawHorizontalLine ((int) y, (float) envPanel.getX() + 16.0f, (float) envPanel.getRight() - 16.0f);
    }

    // berço da válvula: janela escura rebaixada
    {
        auto w = tubeArea.toFloat().expanded (8.0f, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (w.translated (1, 1), 6.0f, 1.0f);
        juce::ColourGradient cg (juce::Colour (0xff221d18), w.getX(), w.getY(), juce::Colour (0xff0b0908), w.getX(), w.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (w, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (w, 6.0f, 1.2f);
    }

    // escalas gravadas nos faders do EQ
    {
        g.setFont (stencil (9.5f, 0.02f));
        for (int b = 0; b < DubBassAudioProcessor::numEqBands; ++b)
        {
            const auto& f = eqFaders[(size_t) b];
            const auto fb = f.getBounds().toFloat();
            for (float db : { 12.0f, 6.0f, 0.0f, -6.0f, -12.0f })
            {
                const float y = fb.getY() + (float) f.getPositionOfValue (db);
                const bool zero = juce::exactlyEqual (db, 0.0f);
                const float len = zero ? 9.0f : 5.0f;
                g.setColour (colours::ink.withAlpha (zero ? 0.85f : 0.5f));
                g.drawHorizontalLine ((int) y, fb.getCentreX() - 15.0f - len, fb.getCentreX() - 15.0f);
                g.drawHorizontalLine ((int) y, fb.getCentreX() + 15.0f, fb.getCentreX() + 15.0f + len);
            }
            const float hz = DubBassAudioProcessor::eqFrequencies[(size_t) b];
            g.setColour (colours::ink);
            g.drawText (hz < 1000.0f ? juce::String (juce::roundToInt (hz)) : juce::String (juce::roundToInt (hz / 1000.0f)) + "k",
                        juce::Rectangle<float> (fb.getX() - 6.0f, fb.getBottom() + 4.0f, fb.getWidth() + 12.0f, 12.0f), juce::Justification::centred);
        }
        g.setColour (colours::inkSoft);
        g.setFont (stencil (8.5f, 0.1f));
        const auto f0 = eqFaders[0].getBounds().toFloat();
        g.drawText ("+12", juce::Rectangle<float> (f0.getX() - 30.0f, f0.getY() + (float) eqFaders[0].getPositionOfValue (12.0) - 6.0f, 22.0f, 12.0f), juce::Justification::right);
        g.drawText ("0",   juce::Rectangle<float> (f0.getX() - 30.0f, f0.getY() + (float) eqFaders[0].getPositionOfValue (0.0) - 6.0f, 22.0f, 12.0f), juce::Justification::right);
        g.drawText ("-12", juce::Rectangle<float> (f0.getX() - 30.0f, f0.getY() + (float) eqFaders[0].getPositionOfValue (-12.0) - 6.0f, 22.0f, 12.0f), juce::Justification::right);
    }

    // legenda do bass boost
    {
        auto cap = boostPanel.toFloat().withTop ((float) boostBtn.getBottom() + 4.0f).withTrimmedBottom (8.0f);
        g.setColour (colours::ink);
        g.setFont (stencil (12.0f, 0.12f));
        g.drawText ("60 Hz  +4 dB", cap.removeFromTop (18.0f), juce::Justification::centred);
        g.setColour (colours::inkSoft);
        g.setFont (stencil (9.0f, 0.25f));
        g.drawText (juce::String::fromUTF8 ("ANTES DO MAXIMIZER"), cap.removeFromTop (14.0f), juce::Justification::centred);
    }

    // --- MAXIMIZER: painel preto embutido na lateral direita (como no Mastor Fone)
    {
        auto mp = maxPanel.toFloat();
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (mp.translated (1, 1), 8.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (mp.translated (0, 3), 8.0f);
        juce::ColourGradient dk (juce::Colour (0xff24211d), mp.getX(), mp.getY(), juce::Colour (0xff0f0d0b), mp.getX(), mp.getBottom(), false);
        g.setGradientFill (dk);
        g.fillRoundedRectangle (mp, 8.0f);
        for (int i = 0; i < 6000; ++i)
        {
            g.setColour ((rng.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.02f + rng.nextFloat() * 0.03f));
            g.fillRect (mp.getX() + rng.nextFloat() * mp.getWidth(), mp.getY() + rng.nextFloat() * mp.getHeight(), 1.0f, 1.0f);
        }
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (mp, 8.0f, 1.5f);
        g.setColour (colours::amber);
        g.setFont (stencil (14.0f, 0.18f));
        g.drawText ("MAXIMIZER", mp.withHeight (40.0f), juce::Justification::centred);
        g.setColour (colours::cream.withAlpha (0.5f));
        g.setFont (stencil (8.5f, 0.3f));
        g.drawText ("LOOKAHEAD  1.5 ms", mp.withY (mp.getY() + 30.0f).withHeight (12.0f), juce::Justification::centred);
    }

    // --- escalas dos knobs
    for (auto* k : { &harm2, &harm3, &glide, &pitchAmt, &pitchTime, &cutoff, &reso,
                     &drive, &drvTone, &drvMix, &revSize, &revDamp, &revWidth, &revLowCut, &revMix, &output,
                     &maxGain, &maxCeiling, &maxRelease })
        drawKnobScale (g, *k, 11);
}

void DubBassPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::woodDark);
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    // display âmbar com a nota tocando
    const float note = proc.currentNote.load();
    juce::String t = "--";
    if (note >= 0.0f)
    {
        const int n = juce::roundToInt (note);
        const double hz = 440.0 * std::pow (2.0, ((double) note - 69.0) / 12.0);
        t = juce::MidiMessage::getMidiNoteName (n, true, true, 4) + juce::String::fromUTF8 ("  ·  ") + juce::String (hz, 1) + " Hz";
    }
    glowText (g, noteDisplay.toFloat(), t, 17.0f, note >= 0.0f ? 1.0f : 0.35f);

    // etiqueta embaixo do berço: válvula ou fita
    g.setColour (colours::inkSoft);
    g.setFont (stencil (9.0f, 0.3f));
    g.drawText (reels.isVisible() ? juce::String::fromUTF8 ("FITA  ¼\"  15 ips") : juce::String ("12AX7"),
                tubeArea.toFloat().expanded (8.0f, 6.0f).withY ((float) tubeArea.getBottom() + 9.0f).withHeight (12.0f), juce::Justification::centred);
}

void DubBassPanel::resized()
{
    const int W = getWidth(), H = getHeight();
    woodL = { 0, 0, 28, H };
    woodR = { W - 28, 0, 28, H };
    plate = { 28, 0, W - 56, H };

    // lateral direita: MAXIMIZER com VU digital estilo Ozone
    maxPanel = { W - 28 - 24 - 178, 96, 178, H - 96 - 26 };

    const int x0 = 52;
    oscPanel   = { x0,  104, 330, 248 };
    envPanel   = { 396, 104, 356, 248 };
    scopePanel = { 766, 104, maxPanel.getX() - 14 - 766, 248 };

    drvPanel   = { x0,  378, 440, 232 };
    eqPanel    = { 506, 378, maxPanel.getX() - 14 - 506, 232 };

    revPanel   = { x0,  636, 470, 176 };
    boostPanel = { 536, 636, 206, revPanel.getHeight() };
    outPanel   = { 756, 636, maxPanel.getX() - 14 - 756, revPanel.getHeight() };

    auto rowOf = [this] (juce::Rectangle<int> r, std::initializer_list<Knob*> ks)
    {
        const int w = r.getWidth() / (int) ks.size();
        for (auto* k : ks) placeKnob (*k, r.removeFromLeft (w));
    };

    {   // OSCILADOR
        auto a = oscPanel.reduced (12, 0);
        a.removeFromTop (18);
        modeKeys.setBounds (a.removeFromTop (38).reduced (14, 0));
        a.removeFromTop (6);
        a.removeFromBottom (6);
        auto r1 = a.removeFromTop (a.getHeight() / 2);
        rowOf (r1, { &harm2, &harm3, &glide });
        rowOf (a.withSizeKeepingCentre (a.getWidth() * 2 / 3, a.getHeight()), { &pitchAmt, &pitchTime });
    }
    {   // ENVELOPE / FILTRO
        auto a = envPanel.reduced (10, 0);
        a.removeFromTop (16);
        a.removeFromBottom (6);
        env808Presets.setBounds (a.removeFromTop (28).removeFromRight (250).reduced (4, 1));
        a.removeFromTop (2);
        envEditor.setBounds (a.removeFromTop (112).reduced (4, 0));
        a.removeFromTop (10);
        rowOf (a.withSizeKeepingCentre (a.getWidth() / 2, a.getHeight()), { &cutoff, &reso });
    }
    scope.setBounds (scopePanel.reduced (12, 0).withTrimmedTop (18).withTrimmedBottom (12));

    {   // VÁLVULA / SATURAÇÃO
        drvOn.button.setBounds (drvPanel.getRight() - 96, drvPanel.getY() + 12, 84, 28);
        auto a = drvPanel.reduced (14, 0);
        a.removeFromTop (16);
        a.removeFromBottom (18);
        tubeArea = a.removeFromLeft (112).withTrimmedTop (4).reduced (6, 0);
        tube.setBounds (tubeArea);
        reels.setBounds (tubeArea);
        a.removeFromLeft (16);
        a.removeFromTop (30);
        drvTypeKeys.setBounds (a.removeFromTop (38));
        a.removeFromTop (6);
        rowOf (a, { &drive, &drvTone, &drvMix });
    }
    {   // EQ GRÁFICO
        eqOn.button.setBounds (eqPanel.getRight() - 96, eqPanel.getY() + 12, 84, 28);
        auto a = eqPanel.reduced (14, 0);
        a.removeFromTop (44);
        eqCurve.setBounds (a.removeFromLeft (juce::jmin (220, a.getWidth() / 2 - 20)).withTrimmedBottom (14));
        a.removeFromLeft (36);
        faderArea = a.withTrimmedBottom (24).withTrimmedTop (6);
        const int w = faderArea.getWidth() / DubBassAudioProcessor::numEqBands;
        auto fa = faderArea;
        for (auto& f : eqFaders)
            f.setBounds (fa.removeFromLeft (w).withSizeKeepingCentre (30, faderArea.getHeight()));
    }
    {   // REVERB
        revOn.button.setBounds (revPanel.getRight() - 96, revPanel.getY() + 12, 84, 28);
        auto a = revPanel.reduced (10, 0);
        a.removeFromTop (40);
        a.removeFromBottom (6);
        rowOf (a, { &revSize, &revDamp, &revWidth, &revLowCut, &revMix });
    }
    {   // BASS BOOST
        auto a = boostPanel.reduced (16, 0);
        a.removeFromTop (30);
        boostBtn.setBounds (a.removeFromTop (78));
    }
    {   // SAÍDA
        auto a = outPanel.reduced (14, 0);
        a.removeFromTop (22);
        a.removeFromBottom (8);
        auto left = a.removeFromLeft (a.getWidth() - 110);
        noteDisplay = left.withSizeKeepingCentre (left.getWidth() - 8, 40).translated (0, -6);
        placeKnob (output, a);
    }
    {   // TECLADO
        keysPanel = { x0, 836, maxPanel.getX() - 14 - x0, H - 836 - 26 };
        keyboard.setBounds (keysPanel.reduced (22, 0).withTrimmedTop (20).withTrimmedBottom (14));
        keyboard.setKeyWidth ((float) keyboard.getWidth() / 22.0f);
    }
    {   // MAXIMIZER
        auto a = maxPanel.reduced (10, 0);
        a.removeFromTop (48);
        maxBypass.button.setBounds (a.removeFromBottom (54).withSizeKeepingCentre (a.getWidth() - 10, 36));
        auto knobs = a.removeFromBottom (206);
        auto top = knobs.removeFromTop (100);
        knobs.removeFromTop (8);
        placeKnob (maxGain, top.removeFromLeft (top.getWidth() / 2));
        placeKnob (maxCeiling, top);
        placeKnob (maxRelease, knobs.withSizeKeepingCentre (knobs.getWidth() / 2 + 10, knobs.getHeight()));
        a.removeFromBottom (8);
        meter.setBounds (a);
    }

    renderBackground();
}
