#include "PluginEditor.h"

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

static float knobReduction (const juce::Slider& s) { return s.getName() == "root" ? 34.0f : 11.0f; }

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

    g.setColour (juce::Colour (0xff0e0c0a));                    // fundo do encaixe
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
            // frente da tecla (borda inferior arredondada, como teclado antigo)
            if (! on)
            {
                g.setColour (juce::Colour (0xffb39d6a));
                g.fillRect (r.withTop (r.getBottom() - 6.0f).reduced (1.0f, 0.0f));
            }
            if (on)
            {
                g.setColour (colours::amber.withAlpha (0.18f));
                g.fillRoundedRectangle (r, rad);
            }
            g.setColour (on ? juce::Colour (0xff6b3f05) : colours::inkSoft);
            g.setFont (stencil (11.0f, 0.02f));
            g.drawText (dub::rootName (n), r.withTrimmedBottom (10.0f).withTop (r.getBottom() - 32.0f), juce::Justification::centred);
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
            g.setColour (on ? colours::amber : colours::cream.withAlpha (0.75f));
            g.setFont (stencil (8.5f, 0.0f));
            g.drawText (dub::rootName (n), r.withTop (r.getBottom() - 24.0f).withTrimmedBottom (10.0f), juce::Justification::centred);
        }

        // lâmpada da tecla selecionada
        const auto lamp = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ r.getCentreX(), r.getBottom() - (black ? 6.0f : 9.0f) });
        if (on)
        {
            g.setColour (colours::amber.withAlpha (0.35f));
            g.fillEllipse (lamp.expanded (4.0f));
            juce::ColourGradient lit (juce::Colour (0xffffe3a0), lamp.getCentreX(), lamp.getCentreY(),
                                      colours::amber.darker (0.2f), lamp.getRight(), lamp.getBottom(), true);
            g.setGradientFill (lit);
            g.fillEllipse (lamp);
        }
    };

    for (int n : kWhite) drawKey (n);
    // sombra das teclas pretas sobre as brancas
    for (auto& bk : kBlackAfter) drawKey (bk[0]);

    // barra cromada no topo (feltro + metal)
    auto bar = b.withHeight (7.0f);
    juce::ColourGradient chrome (juce::Colour (0xffd8d6d0), bar.getX(), bar.getY(), juce::Colour (0xff5e5b55), bar.getX(), bar.getBottom(), false);
    g.setGradientFill (chrome);
    g.fillRect (bar);
    g.setColour (juce::Colour (0xff5a1414));                        // feltro vermelho
    g.fillRect (b.withY (bar.getBottom()).withHeight (2.0f));
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

void ModeSwitch::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1714));                        // moldura metálica comum
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);
    auto inner = b.reduced (4.0f);
    const auto mouse = getMouseXYRelative().toFloat();
    const bool over = isMouseOver();
    auto left = inner.removeFromLeft (inner.getWidth() * 0.5f).reduced (1.5f, 0.0f);
    auto right = inner.reduced (1.5f, 0.0f);
    pushButtonCap (g, left.withTrimmedBottom (2.5f),  index() == 0, over && left.contains (mouse),  "MAIOR");
    pushButtonCap (g, right.withTrimmedBottom (2.5f), index() == 1, over && right.contains (mouse), "MENOR");
}

//==============================================================================
void InversionSwitch::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto frame = b.withTrimmedTop (9.0f);
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (frame, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (frame.reduced (0.5f), 5.0f, 1.0f);

    const int sel = juce::jlimit (0, 3, index());
    const int sug = suggested ? juce::jlimit (0, 2, suggested()) + 1 : -1;   // índice do botão sugerido
    static const char* labels[4] = { "AUTO", "FUND", "1\xc2\xaa", "2\xc2\xaa" };
    const auto mouse = getMouseXYRelative().toFloat();

    auto inner = frame.reduced (3.0f);
    const float w = inner.getWidth() / 4.0f;
    for (int k = 0; k < 4; ++k)
    {
        auto r = juce::Rectangle<float> (inner.getX() + w * (float) k, inner.getY(), w, inner.getHeight()).reduced (1.5f, 0.0f).withTrimmedBottom (2.5f);
        const bool on = k == sel, over = isMouseOver() && r.contains (mouse);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (r.translated (0, 2.5f), 3.0f);
        auto cap = r.translated (0, on ? 2.0f : 0.0f);
        juce::ColourGradient cg (juce::Colour (on ? 0xff2c2925 : 0xff3a3632).brighter (over && ! on ? 0.08f : 0.0f), cap.getX(), cap.getY(),
                                 juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (on ? 0.04f : 0.12f));
        g.drawHorizontalLine ((int) cap.getY() + 1, cap.getX() + 3, cap.getRight() - 3);

        if (on)   // lâmpada do botão apertado
        {
            const auto lamp = juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ cap.getX() + 8.0f, cap.getCentreY() });
            g.setColour (colours::amber.withAlpha (0.35f));
            g.fillEllipse (lamp.expanded (3.5f));
            g.setColour (juce::Colour (0xffffd27a));
            g.fillEllipse (lamp);
        }

        const bool isSug = k == sug;
        g.setColour (on ? colours::cream : isSug ? colours::amber : colours::cream.withAlpha (0.5f));
        g.setFont (stencil (11.0f, 0.08f));
        g.drawText (juce::String::fromUTF8 (labels[k]), cap.withTrimmedLeft (on ? 10.0f : 0.0f), juce::Justification::centred);

        if (isSug)   // seta âmbar em cima do botão sugerido
        {
            juce::Path tri;
            const float cx = r.getCentreX(), ty = b.getY() + 1.0f;
            tri.addTriangle (cx - 5.0f, ty, cx + 5.0f, ty, cx, ty + 6.5f);
            g.setColour (colours::amber.darker (0.15f));
            g.fillPath (tri);
        }
    }
}

//==============================================================================
void ChoiceWindow::mouseDown (const juce::MouseEvent& e)
{
    const int dir = e.position.x < (float) getWidth() * 0.5f ? -1 : 1;
    select (juce::jlimit (0, numChoices() - 1, index() + dir));
}

void ChoiceWindow::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const int count = numChoices();
    const int i = juce::jlimit (0, count - 1, index());

    // moldura de metal escuro com parafusos
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (b.translated (0, 2.0f), 5.0f);
    juce::ColourGradient frame (juce::Colour (0xff3a3632), b.getX(), b.getY(), juce::Colour (0xff141210), b.getX(), b.getBottom(), false);
    g.setGradientFill (frame);
    g.fillRoundedRectangle (b, 5.0f);

    // setas nas laterais (acendem com o mouse)
    auto arrow = [&] (float cx, int dir)
    {
        const bool lit = side == dir;
        const bool possible = (dir < 0 && i > 0) || (dir > 0 && i < count - 1);
        juce::Path p;
        const float cy = b.getCentreY(), s = 6.5f;
        if (dir < 0) p.addTriangle (cx + s * 0.6f, cy - s, cx + s * 0.6f, cy + s, cx - s * 0.8f, cy);
        else         p.addTriangle (cx - s * 0.6f, cy - s, cx - s * 0.6f, cy + s, cx + s * 0.8f, cy);
        g.setColour (! possible ? colours::cream.withAlpha (0.15f) : lit ? colours::amber : colours::cream.withAlpha (0.6f));
        g.fillPath (p);
    };
    arrow (b.getX() + 16.0f, -1);
    arrow (b.getRight() - 16.0f, 1);

    // janela do contador: tambor creme (cilindro)
    auto win = b.reduced (32.0f, 7.0f);
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (win.expanded (2.0f), 3.0f);
    juce::ColourGradient drum (juce::Colour (0xff8f8264), win.getX(), win.getY(), juce::Colour (0xff8f8264), win.getX(), win.getBottom(), false);
    drum.addColour (0.25, juce::Colour (0xffefe5cc));
    drum.addColour (0.55, juce::Colour (0xfff6ecd6));
    drum.addColour (0.85, juce::Colour (0xffcbbd98));
    g.setGradientFill (drum);
    g.fillRoundedRectangle (win, 2.0f);

    const auto [big, unit] = format (i);
    g.setColour (colours::ink);
    float bigSize = 21.0f;
    g.setFont (stencil (bigSize, 0.04f));
    float nw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), big);
    g.setFont (stencil (10.0f, 0.22f));
    const float uw = unit.isEmpty() ? 0.0f : juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), unit);
    const float gap = unit.isEmpty() ? 0.0f : 8.0f;
    if (nw + gap + uw > win.getWidth() - 10.0f)           // texto longo: diminui o grande
    {
        bigSize = juce::jmax (11.0f, bigSize * (win.getWidth() - 10.0f - gap - uw) / nw);
        g.setFont (stencil (bigSize, 0.04f));
        nw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), big);
    }
    auto line = win.withSizeKeepingCentre (nw + gap + uw, win.getHeight());
    g.setFont (stencil (bigSize, 0.04f));
    g.drawText (big, line.removeFromLeft (nw), juce::Justification::centredLeft);
    line.removeFromLeft (gap);
    g.setFont (stencil (10.0f, 0.22f));
    g.drawText (unit, line, juce::Justification::centredLeft);

    // vidro com reflexo
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillRoundedRectangle (win.withHeight (win.getHeight() * 0.3f).reduced (2.0f, 1.0f), 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (win, 2.0f, 1.0f);
}

//==============================================================================
void PreviewButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    const bool on = getToggleState();
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (r, 6.0f);                                   // moldura
    auto cap = r.reduced (4.0f).translated (0, (down || on) ? 1.5f : 0.0f);
    const juce::Colour base = on ? colours::jamGold.darker (0.15f) : colours::jamGreen;
    juce::ColourGradient cg (base.brighter (over ? 0.12f : 0.0f), cap.getX(), cap.getY(), base.darker (0.75f), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.12f : 0.22f));
    g.fillRoundedRectangle (cap.withHeight (cap.getHeight() * 0.38f).reduced (4, 2), 3.0f);

    // lâmpada piloto
    const auto lamp = juce::Rectangle<float> (11.0f, 11.0f).withCentre ({ cap.getX() + 18.0f, cap.getCentreY() });
    if (on)
    {
        g.setColour (colours::amber.withAlpha (0.4f));
        g.fillEllipse (lamp.expanded (6.0f));
        juce::ColourGradient lit (juce::Colour (0xffffe3a0), lamp.getCentreX(), lamp.getCentreY(),
                                  colours::amber.darker (0.2f), lamp.getRight(), lamp.getBottom(), true);
        g.setGradientFill (lit);
    }
    else
        g.setColour (juce::Colour (0xff3a2d18));
    g.fillEllipse (lamp);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (lamp, 1.0f);

    // símbolo play / stop + texto
    auto txt = cap.withTrimmedLeft (30.0f);
    const auto ic = juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ txt.getX() + 7.0f, cap.getCentreY() });
    juce::Path sym;
    if (on) sym.addRectangle (ic.reduced (1.5f));
    else    sym.addTriangle (ic.getX() + 1.0f, ic.getY(), ic.getX() + 1.0f, ic.getBottom(), ic.getRight(), ic.getCentreY());
    const juce::Colour ink = on ? juce::Colour (0xff2e2108) : colours::jamGold;
    g.setColour (ink);
    g.fillPath (sym);
    g.setFont (stencil (13.0f, 0.1f));
    g.drawText (on ? "PARAR" : "PREVIEW", txt.withTrimmedLeft (16.0f), juce::Justification::centred);
}

//==============================================================================
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

void MidiDragZone::mouseDrag (const juce::MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("MRCM-1240 riddim.mid");
    if (proc.writeMidiFile (file))
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (file.getFullPathName()), false, this);
}

//==============================================================================
DubChordsPanel::DubChordsPanel (DubChordsProcessor& p) : proc (p), dragZone (p)
{
    setLookAndFeel (&lnf);

    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
    {
        auto& m = mods[(size_t) i];
        const auto n = juce::String (i);
        m.keys = std::make_unique<NoteKeys>       (*proc.apvts.getParameter ("root" + n));
        m.mode = std::make_unique<ModeSwitch>     (*proc.apvts.getParameter ("qual" + n));
        m.inv  = std::make_unique<InversionSwitch> (*proc.apvts.getParameter ("inv" + n),
                                                    [this, i] { return voicings.suggested.empty() ? 0 : voicings.suggested[(size_t) i]; });
        m.dur  = std::make_unique<ChoiceWindow>   (*proc.apvts.getParameter ("dur" + n), [] (int d) -> std::pair<juce::String, juce::String>
                 { return { dub::durationShort (d), d <= 1 ? "COMPASSO" : "COMPASSOS" }; });
        addAndMakeVisible (*m.keys);
        addAndMakeVisible (*m.mode);
        addAndMakeVisible (*m.inv);
        addAndMakeVisible (*m.dur);
        if (i > 0) setupToggle (m.on, "on" + n, "ON");
    }



    setupKnob (center,   "center",   "REGISTRO");
    setupKnob (velocity, "velocity", "VELOCITY");
    setupKnob (swing,    "swing",    "SWING");
    center.slider.textFromValueFunction = [] (double v) { return juce::MidiMessage::getMidiNoteName ((int) v, true, true, 4); };
    center.slider.updateText();
    for (auto* k : { &velocity, &swing })
    {
        k->slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
        k->slider.updateText();
    }

    addAndMakeVisible (previewBtn);
    previewAtt = std::make_unique<ButtonAttach> (proc.apvts, "preview", previewBtn);
    setupToggle (bubble, "bubble", "BUBBLE");
    setupToggle (sound,  "sound",  "SOM INTERNO");
    setupKnob (bpm,      "bpm",      "BPM");
    setupKnob (pianoLvl, "pianoLvl", "PIANO M1");
    setupKnob (organLvl, "organLvl", "HAMMOND");
    for (auto* k : { &pianoLvl, &organLvl })
    {
        k->slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
        k->slider.updateText();
    }

    setupKnob (reps, "exportReps", "REPETIR");
    reps.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    exportBtn.onClick = [this] { exportMidi(); };
    addAndMakeVisible (exportBtn);
    presetBtn.onClick = [this] { showPresets(); };
    presetBtn.setTooltip (juce::String::fromUTF8 ("Harmonias clássicas por estilo: roots, rockers, rub a dub, lovers rock, dancehall digital e new roots"));
    addAndMakeVisible (presetBtn);
    addAndMakeVisible (dragZone);

    voicings = proc.getVoicings();
    setSize (1240, 846);
    timerCallback();
    startTimerHz (30);
}

DubChordsPanel::~DubChordsPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DubChordsPanel::setupKnob (Knob& k, const juce::String& id, const juce::String& name)
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

void DubChordsPanel::setupToggle (Toggle& t, const juce::String& id, const juce::String& text)
{
    t.button.setButtonText (text);
    addAndMakeVisible (t.button);
    t.att = std::make_unique<ButtonAttach> (proc.apvts, id, t.button);
}

void DubChordsPanel::placeKnob (Knob& k, juce::Rectangle<int> r)
{
    k.label.setBounds (r.removeFromTop (15));
    k.slider.setBounds (r);
}

bool DubChordsPanel::slotEnabled (int i) const
{
    return i == 0 || proc.apvts.getRawParameterValue ("on" + juce::String (i))->load() > 0.5f;
}

void DubChordsPanel::exportMidi()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Mastor Reggae Chord Machine");
    dir.createDirectory();
    chooser = std::make_unique<juce::FileChooser> ("Salvar riddim em MIDI", dir.getChildFile ("riddim.mid"), "*.mid");
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
                                                                          "Mastor Reggae Chord Machine", juce::String::fromUTF8 ("N\xc3\xa3o foi poss\xc3\xadvel salvar o MIDI."));
                          });
}

void DubChordsPanel::timerCallback()
{
    voicings = proc.getVoicings();
    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
    {
        const bool on = slotEnabled (i);
        if (on != lastOn[(size_t) i] || ! isShowing())
        {
            lastOn[(size_t) i] = on;
            auto& m = mods[(size_t) i];
            for (juce::Component* c : { (juce::Component*) m.keys.get(), (juce::Component*) m.mode.get(),
                                        (juce::Component*) m.inv.get(), (juce::Component*) m.dur.get() })
                c->setAlpha (on ? 1.0f : 0.38f);
        }
    }
    repaint();
}

//==============================================================================
void DubChordsPanel::drawKnobScale (juce::Graphics& g, juce::Slider& s, int ticks, const juce::StringArray& labels)
{
    const auto circ = knobCircle (s);
    const auto c = circ.getCentre();
    const float r = circ.getWidth() * 0.5f;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = kStartAngle + (kEndAngle - kStartAngle) * (float) i / (float) juce::jmax (1, ticks - 1);
        const float r1 = r + 4.0f, r2 = r + (labels.isEmpty() ? 9.0f : 11.0f);
        g.setColour (colours::ink.withAlpha (0.85f));
        g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1,
                    c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, labels.isEmpty() ? 1.1f : 1.6f);
        if (i < labels.size())
        {
            g.setFont (stencil (labels.size() > 6 ? 11.5f : 9.5f, 0.02f));
            const float lr = r + (labels.size() > 6 ? 24.0f : 20.0f);
            g.drawText (labels[i], juce::Rectangle<float> (44, 14).withCentre ({ c.x + std::sin (a) * lr, c.y - std::cos (a) * lr }),
                        juce::Justification::centred);
        }
    }
}

void DubChordsPanel::renderBackground()
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

    // --- faixa de console anos 70 (mostarda do MRCM-1240) no alto da placa creme; letras do cabeçalho em creme
    {
        const auto band = plate.withHeight (80).toFloat();
        const juce::Colour bc (0xffb08a3a);
        g.setColour (bc);
        g.fillRect (band);
        for (int i = 0; i < 900; ++i)   // tinta velha: pontinhos e riscos
        {
            const float x = band.getX() + rng.nextFloat() * band.getWidth(), y = band.getY() + rng.nextFloat() * band.getHeight();
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.03f + rng.nextFloat() * 0.05f));
            g.fillRect (x, y, 1.0f + rng.nextFloat() * 2.0f, 1.0f);
        }
        g.setColour (bc.darker (0.6f));
        g.fillRect (band.getX(), band.getBottom() - 3.0f, band.getWidth(), 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRect (band.getX(), band.getBottom(), band.getWidth(), 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.fillRect (band.getX(), band.getBottom() + 1.0f, band.getWidth(), 3.0f);
    }

    // --- cabeçalho
    g.setColour (juce::Colour (0xfff3e6c8));
    // padrão da família: versão no título (vem do CMake: project(DubChords VERSION x.y.z) -> "x.y"),
    // e a placa de latão com a sigla MRCM-1240
    juce::String ver (JucePlugin_VersionString);
    if (ver.containsChar ('.') && ver.fromFirstOccurrenceOf (".", false, false).containsChar ('.'))
        ver = ver.upToLastOccurrenceOf (".", false, false);
    const juce::String title ("MASTOR REGGAE CHORD MACHINE " + ver);
    {   // nome comprido: diminui a fonte se o título + a placa de latão não couberem antes das faixas da bandeira
        float fs = 30.0f;
        const float maxW = (float) plate.getWidth() - 330.0f - 32.0f - 26.0f - 260.0f;
        while (fs > 22.0f && juce::GlyphArrangement::getStringWidth (stencil (fs, 0.22f), title) > maxW)
            fs -= 1.0f;
        g.setFont (stencil (fs, 0.22f));
    }
    g.drawText (title, plate.getX() + 32, 20, 760, 34, juce::Justification::left);
    const float titleRight = (float) plate.getX() + 32.0f + juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title);

    {   // placa de latão com a sigla da máquina
        g.setFont (stencil (15.0f, 0.34f));
        const juce::String t ("MRCM-1240");
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
    g.setColour (juce::Colour (0xfff3e6c8).withAlpha (0.75f));
    g.setFont (stencil (10.5f, 0.25f));
    g.drawText (juce::String::fromUTF8 ("MRCM-1240  \xc2\xb7  RIDDIM CHORD GENERATOR  \xc2\xb7  DUBMASTOR"), plate.getX() + 34, 52, 600, 14, juce::Justification::left);


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
    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
        section (mods[(size_t) i].area, "ACORDE " + juce::String (i + 1));
    section (rhythmPanel, "RITMO");
    section (soundPanel,  "SOM");
    section (midiPanel,   "MIDI");
    section (loopRow.expanded (10, 14).withTrimmedBottom (2), "LOOP");

    // --- escalas
    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
    {
        auto& md = mods[(size_t) i];
        // rótulos gravados acima de cada controle
        g.setColour (colours::inkSoft);
        g.setFont (stencil (9.0f, 0.3f));
        for (auto [c, t] : { std::pair<juce::Component*, const char*> { md.keys.get(), "T\xc3\x94NICA" },
                             { md.mode.get(), "MAIOR  /  MENOR" }, { md.inv.get(), "INVERS\xc3\x83O" }, { md.dur.get(), "DURA\xc3\x87\xc3\x83O" } })
            g.drawText (juce::String::fromUTF8 (t), c->getBounds().translated (0, -15).withHeight (12), juce::Justification::centred);
        // trilho escuro onde o teclado está encaixado
        auto rail = md.keys->getBounds().toFloat().expanded (6.0f, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (rail.translated (1, 1), 5.0f, 1.0f);
        g.setColour (colours::ink.withAlpha (0.6f));
        g.drawRoundedRectangle (rail, 5.0f, 1.2f);
        g.setColour (colours::inkSoft);
        if (i == 0)
        {
            g.setFont (stencil (9.0f, 0.2f));
            g.drawText ("SEMPRE LIGADO", md.on.button.getBounds().toFloat().withX ((float) md.area.getRight() - 130.0f).withWidth (116.0f),
                        juce::Justification::centredRight);
        }
    }
    for (auto* k : { &center, &velocity, &swing, &bpm, &pianoLvl, &organLvl })
        drawKnobScale (g, k->slider, 11);
    g.setColour (colours::inkSoft);
    g.setFont (stencil (9.0f, 0.3f));
    g.drawText (juce::String::fromUTF8 ("PADR\xc3\x83O DO SKANK"), patternArea.translated (0, -15).withHeight (12), juce::Justification::centred);
    {   // skank sempre no contratempo: janela fixa no lugar do antigo seletor
        auto b = patternArea.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (b.translated (0, 2.0f), 5.0f);
        juce::ColourGradient frame (juce::Colour (0xff3a3632), b.getX(), b.getY(), juce::Colour (0xff141210), b.getX(), b.getBottom(), false);
        g.setGradientFill (frame);
        g.fillRoundedRectangle (b, 5.0f);
        auto win = b.reduced (14.0f, 7.0f);
        g.setColour (juce::Colour (0xff0c0907));
        g.fillRoundedRectangle (win.expanded (2.0f), 3.0f);
        juce::ColourGradient drum (juce::Colour (0xff8f8264), win.getX(), win.getY(), juce::Colour (0xff8f8264), win.getX(), win.getBottom(), false);
        drum.addColour (0.25, juce::Colour (0xffefe5cc));
        drum.addColour (0.55, juce::Colour (0xfff6ecd6));
        drum.addColour (0.85, juce::Colour (0xffcbbd98));
        g.setGradientFill (drum);
        g.fillRoundedRectangle (win, 2.0f);
        g.setColour (colours::ink);
        g.setFont (stencil (17.0f, 0.06f));
        g.drawText ("CONTRATEMPO", win, juce::Justification::centred);
    }
    {   // notas sempre 1/32: plaqueta gravada no lugar do antigo seletor
        auto r = patternArea.translated (0, patternArea.getHeight() + 30).withHeight (44).toFloat();
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (r.translated (1, 1), 4.0f, 1.0f);
        g.setColour (colours::ink.withAlpha (0.55f));
        g.drawRoundedRectangle (r, 4.0f, 1.2f);
        g.setColour (colours::inkSoft);
        g.setFont (stencil (9.0f, 0.3f));
        g.drawText (juce::String::fromUTF8 ("DURA\xc3\x87\xc3\x83O DAS NOTAS"), r.withHeight (r.getHeight() * 0.45f).translated (0, 4), juce::Justification::centred);
        g.setColour (colours::ink);
        g.setFont (stencil (14.0f, 0.12f));
        g.drawText ("1/32  FUSA", r.withTrimmedTop (r.getHeight() * 0.42f), juce::Justification::centred);
    }
    drawKnobScale (g, reps.slider, 4, { "1x", "2x", "4x", "8x" });
}

void DubChordsPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::woodDark);
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    const auto slots = proc.getSlots();
    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
    {
        const auto& s = slots[(size_t) i];
        const bool on = s.beats > 0;
        const auto& m = mods[(size_t) i];
        juce::String t = "OFF";
        if (on)
        {
            const double bars = s.beats / 4.0;
            juce::ignoreUnused (bars);
            t = juce::String (dub::rootName (s.root)) + (s.quality == dub::Min ? " MENOR" : " MAIOR");
            if ((size_t) i < voicings.used.size())
                t << juce::String::fromUTF8 ("  \xc2\xb7  ") << juce::String::fromUTF8 (dub::inversionName (voicings.used[(size_t) i]));
        }
        glowText (g, m.display.toFloat(), t, 17.0f, on ? 1.0f : 0.35f);
    }


    drawTimeline (g, timeline.toFloat());
}

void DubChordsPanel::drawTimeline (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r.expanded (3), 4.0f);
    juce::ColourGradient gl (colours::glass.brighter (0.15f), r.getX(), r.getY(), colours::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r, 3.0f);

    const auto slots = proc.getSlots();
    double total = 0;
    for (const auto& s : slots) total += s.beats;
    if (total <= 0) return;

    // compassos
    g.setColour (colours::amber.withAlpha (0.12f));
    for (double b = 4; b < total; b += 4)
        g.drawVerticalLine ((int) (r.getX() + (float) (b / total) * r.getWidth()), r.getY() + 4, r.getBottom() - 4);

    double x = 0;
    g.setFont (stencil (13.0f, 0.1f));
    for (const auto& s : slots)
    {
        if (s.beats <= 0) continue;
        const float x0 = r.getX() + (float) (x / total) * r.getWidth();
        const float w  = (float) (s.beats / total) * r.getWidth();
        auto seg = juce::Rectangle<float> (x0, r.getY(), w, r.getHeight()).reduced (3.0f, 5.0f);
        g.setColour (colours::amber.withAlpha (0.10f));
        g.fillRoundedRectangle (seg, 2.0f);
        g.setColour (colours::amber.withAlpha (0.55f));
        g.drawRoundedRectangle (seg, 2.0f, 0.8f);
        g.setColour (colours::amber);
        g.drawText (juce::String (dub::chordLabel (s)), seg, juce::Justification::centred);
        x += s.beats;
    }

    const double pos = proc.getPlayPositionInLoop();
    if (pos >= 0.0)
    {
        const float px = r.getX() + (float) (pos / total) * r.getWidth();
        g.setColour (colours::amber.withAlpha (0.3f));
        g.fillRect (px - 3.0f, r.getY() + 2.0f, 6.0f, r.getHeight() - 4.0f);
        g.setColour (juce::Colour (0xffffe3a0));
        g.fillRect (px - 1.0f, r.getY() + 2.0f, 2.0f, r.getHeight() - 4.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f), 3.0f);
}

void DubChordsPanel::resized()
{
    const int W = getWidth(), H = getHeight();
    woodL = { 0, 0, 28, H };
    woodR = { W - 28, 0, 28, H };
    plate = { 28, 0, W - 56, H };

    // --- 4 módulos de acorde
    const int x0 = 52, modW = 270, gap = (W - 104 - modW * DubChordsProcessor::numSlots) / (DubChordsProcessor::numSlots - 1);
    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
    {
        auto& m = mods[(size_t) i];
        m.area = { x0 + i * (modW + gap), 92, modW, 440 };
        auto a = m.area.reduced (14, 0);
        a.removeFromTop (16);
        auto header = a.removeFromTop (28);
        m.on.button.setBounds (header.removeFromRight (84));
        a.removeFromTop (6);
        m.display = a.removeFromTop (36).reduced (4, 0);
        a.removeFromTop (4);
        a.removeFromTop (26);
        m.keys->setBounds (a.removeFromTop (124).reduced (6, 0));
        a.removeFromTop (30);
        m.mode->setBounds (a.removeFromTop (40));
        a.removeFromTop (20);
        m.inv->setBounds (a.removeFromTop (44));
        a.removeFromTop (26);
        m.dur->setBounds (a.removeFromTop (52).reduced (20, 0));
    }

    // --- linha de baixo
    const int by = 556, bh = 200;
    rhythmPanel = { 52, by, 430, bh };
    soundPanel  = { 496, by, 430, bh };
    midiPanel   = { 940, by, W - 52 - 940, bh };

    {   // RITMO: contadores à esquerda, knobs à direita
        auto a = rhythmPanel.reduced (14, 0);
        a.removeFromTop (34);
        auto left = a.removeFromLeft (190);
        patternArea = left.removeFromTop (52);
        a.removeFromLeft (8);
        const int kw = a.getWidth() / 3;
        for (auto* k : { &center, &velocity, &swing })
            placeKnob (*k, a.removeFromLeft (kw).withTrimmedBottom (24).withTrimmedTop (14));
    }
    {   // SOM: PREVIEW + chaves à esquerda, BPM e níveis dos timbres à direita
        auto a = soundPanel.reduced (12, 0);
        a.removeFromTop (18);
        auto c1 = a.removeFromLeft (150);
        previewBtn.setBounds (c1.removeFromTop (58));
        c1.removeFromTop (16);
        for (auto* t : { &bubble, &sound })
        {
            t->button.setBounds (c1.removeFromTop (38).reduced (0, 1));
            c1.removeFromTop (12);
        }
        a.removeFromLeft (10);
        const int kw = a.getWidth() / 3;
        for (auto* k : { &bpm, &pianoLvl, &organLvl })
            placeKnob (*k, a.removeFromLeft (kw).withTrimmedBottom (24).withTrimmedTop (14));
    }
    {   // MIDI
        auto a = midiPanel.reduced (14, 0);
        a.removeFromTop (22);
        auto top = a.removeFromTop (92);
        exportBtn.setBounds (top.removeFromLeft (140).withSizeKeepingCentre (140, 56));
        placeKnob (reps, top);
        a.removeFromTop (14);
        dragZone.setBounds (a.removeFromTop (60));
    }

    loopRow  = { 62, 788, W - 124, 34 };
    presetBtn.setBounds (loopRow.withWidth (150));
    timeline = loopRow.withTrimmedLeft (162);
    renderBackground();
}

//==============================================================================
// PRESETS: um submenu por estilo de reggae, com a década e os acordes de cada preset
void DubChordsPanel::showPresets()
{
    juce::PopupMenu menu;
    menu.addSectionHeader (juce::String::fromUTF8 ("HARMONIAS CLÁSSICAS DO REGGAE"));
    // estilo de reggae > música (abre passando o mouse ou clicando)
    for (int f = 0; f < dubpresets::numFolders; ++f)
    {
        juce::PopupMenu sub;
        int count = 0;
        bool ticked = false;
        for (int i : dubpresets::menuOrder)
        {
            const auto& pr = dubpresets::presets[i];
            if (juce::String (pr.folder) != dubpresets::folders[f]) continue;
            juce::StringArray ch;
            for (int k = 0; k < pr.numChords; ++k)
                ch.add (juce::String (dub::rootName (pr.chords[k].root)) + (pr.chords[k].minor ? "m" : ""));
            const bool tick = proc.getLastPreset() == i;
            ticked = ticked || tick; ++count;
            sub.addItem (i + 1, juce::String::fromUTF8 (pr.name) + "   (" + pr.era + ")   " + ch.joinIntoString (" - "), true, tick);
        }
        menu.addSubMenu (juce::String (dubpresets::folders[f]) + "   (" + juce::String (count) + ")", sub, true, nullptr, ticked);
    }
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn),
                        [this] (int r) { if (r > 0) { proc.applyPreset (r - 1); repaint(); } });
}
