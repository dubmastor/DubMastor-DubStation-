#include "BatteryEditor.h"

#ifndef DBR_VERSION_SHORT
 #define DBR_VERSION_SHORT "1.0"
#endif

using namespace dbr;

namespace dbrui
{
//==============================================================================
// Visual da família Dub Station (mesmas cores do MRDM-12)
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
    const juce::Colour amber     { 0xffffa928 };
    const juce::Colour amberDim  { 0xff6a4510 };
    const juce::Colour glass     { 0xff17110a };
    const juce::Colour woodDark  { 0xff2e1a0d };
    const juce::Colour woodLight { 0xff6b4423 };
}

static juce::Font stencil (float size, float kerning = 0.08f)
{
    return juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (kerning);
}

// texto âmbar com brilho (display de vidro)
static void glow (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j,
                  float size, float alpha = 1.0f, float kerning = 0.12f)
{
    g.setFont (stencil (size, kerning));
    g.setColour (col::amber.withAlpha (0.16f * alpha));
    for (auto o : { juce::Point<float> (-1, 0), { 1, 0 }, { 0, -1 }, { 0, 1 } })
        g.drawText (t, r.translated (o.x, o.y), j);
    g.setColour (col::amber.withAlpha (alpha));
    g.drawText (t, r, j);
}

// vidro escuro do display (LCD estilo DMX) com moldura
static void lcdGlass (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r.expanded (4), 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawRoundedRectangle (r.expanded (4).translated (0, 1), 5.0f, 1.0f);
    juce::ColourGradient gl (col::glass.brighter (0.18f), r.getX(), r.getY(), col::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r, 3.0f);
}

static void lcdShine (juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::ColourGradient sh (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(),
                             juce::Colours::transparentWhite, r.getX(), r.getY() + r.getHeight() * 0.5f, false);
    g.setGradientFill (sh);
    g.fillRoundedRectangle (r, 3.0f);
}

// lâmpada piloto âmbar
static void lamp (juce::Graphics& g, juce::Point<float> c, bool on, float size = 10.0f)
{
    const auto r = juce::Rectangle<float> (size, size).withCentre (c);
    if (on)
    {
        g.setColour (col::amber.withAlpha (0.30f));
        g.fillEllipse (r.expanded (5.0f));
        juce::ColourGradient lit (juce::Colour (0xffffe3a0), r.getCentreX(), r.getCentreY(),
                                  col::amber.darker (0.2f), r.getRight(), r.getBottom(), true);
        g.setGradientFill (lit);
    }
    else
        g.setColour (juce::Colour (0xff4a3a22));
    g.fillEllipse (r);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (r, 1.0f);
}

// tampa de botão de pressão (escura, verde ou vermelha)
static void keyCap (juce::Graphics& g, juce::Rectangle<float> r, Key::Cap cap, bool pressed, bool over)
{
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (r.translated (0, 2.5f), 3.0f);
    auto c = r.translated (0, pressed ? 2.0f : 0.0f);
    juce::Colour top (0xff3a3632), bottom (0xff141210);
    if (cap == Key::Cap::Green) { top = col::jamGreen.brighter (0.1f); bottom = col::jamGreen.darker (0.8f); }
    if (cap == Key::Cap::Red)   { top = juce::Colour (0xffb8342a); bottom = juce::Colour (0xff4a110d); }
    if (pressed) top = top.darker (0.15f);
    juce::ColourGradient cg (top.brighter (over && ! pressed ? 0.10f : 0.0f), c.getX(), c.getY(), bottom, c.getX(), c.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (c, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (pressed ? 0.05f : (cap == Key::Cap::Dark ? 0.12f : 0.22f)));
    g.fillRoundedRectangle (c.withHeight (c.getHeight() * 0.36f).reduced (3, 1), 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (c, 3.0f, 1.0f);
}

//==============================================================================
// Fundo: madeira, metal pintado envelhecido, parafusos e plaquetas (mesma receita do MRDM-12)
static void paintWood (juce::Graphics& g, juce::Rectangle<int> wood, juce::Random& rng)
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

static void paintPlate (juce::Graphics& g, juce::Rectangle<int> plate, juce::Random& rng, bool top, bool bottom)
{
    juce::ColourGradient pg (col::plate.brighter (0.03f), (float) plate.getX(), (float) plate.getY(),
                             col::plateDark.darker (0.06f), (float) plate.getRight(), (float) plate.getBottom(), false);
    pg.addColour (0.5, col::plate.interpolatedWith (juce::Colour (0xffd9c48f), 0.35f));
    g.setGradientFill (pg);
    g.fillRect (plate);
    auto randIn = [&] (juce::Rectangle<int> r)
    { return juce::Point<float> ((float) r.getX() + rng.nextFloat() * (float) r.getWidth(), (float) r.getY() + rng.nextFloat() * (float) r.getHeight()); };

    const int area = plate.getWidth() * plate.getHeight();
    for (int i = 0; i < area / 50000 + 4; ++i)
    {
        const float rr = 60.0f + rng.nextFloat() * 220.0f;
        const auto pc = randIn (plate);
        juce::ColourGradient y (juce::Colour (0xffc9a64e).withAlpha (0.07f + rng.nextFloat() * 0.05f), pc.x, pc.y,
                                juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
        g.setGradientFill (y);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 1.4f).withCentre (pc));
    }
    for (int i = 0; i < area / 16; ++i)
    {
        const auto pt = randIn (plate);
        g.setColour ((rng.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.03f + rng.nextFloat() * 0.07f));
        g.fillRect (pt.x, pt.y, 1.0f, 1.0f);
    }
    for (int i = 0; i < area / 33000; ++i)
    {
        const float rr = 10.0f + rng.nextFloat() * 90.0f;
        const auto pc = randIn (plate);
        juce::ColourGradient stain (juce::Colour (0xff7a5a2c).withAlpha (0.05f + rng.nextFloat() * 0.07f), pc.x, pc.y,
                                    juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
        g.setGradientFill (stain);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (pc));
    }
    for (int i = 0; i < area / 7000; ++i)
    {
        const auto p0 = randIn (plate);
        const float ang = rng.nextFloat() * 3.14159f, len = 4.0f + rng.nextFloat() * 38.0f;
        g.setColour ((rng.nextFloat() < 0.6f ? juce::Colours::white : juce::Colour (0xff4a3a24)).withAlpha (0.10f + rng.nextFloat() * 0.18f));
        g.drawLine (p0.x, p0.y, p0.x + std::cos (ang) * len, p0.y + std::sin (ang) * len, 0.5f);
    }
    {
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
        if (top)    band (pf.withHeight (d),               { 0, pf.getY() },      { 0, pf.getY() + d });
        if (bottom) band (pf.withTop (pf.getBottom() - d), { 0, pf.getBottom() }, { 0, pf.getBottom() - d });
    }
}

static void screw (juce::Graphics& g, float sx, float sy, juce::Random& rng)
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

// moldura gravada + plaqueta preta com o nome da seção
static void section (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& t)
{
    auto rf = r.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawRoundedRectangle (rf.translated (1, 1), 4.0f, 1.0f);
    g.setColour (col::ink.withAlpha (0.55f));
    g.drawRoundedRectangle (rf, 4.0f, 1.2f);
    g.setFont (stencil (11.5f, 0.22f));
    const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 22.0f;
    auto tag = juce::Rectangle<float> (rf.getX() + 12, rf.getY() - 9, tw, 18);
    g.setColour (col::labelBg);
    g.fillRoundedRectangle (tag, 2.0f);
    g.setColour (col::cream);
    g.drawText (t, tag, juce::Justification::centred);
}

//==============================================================================
void Key::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    const bool pressed = down || (lit && cap == Cap::Dark && showLamp);
    keyCap (g, r, cap, pressed, isMouseOver());
    auto c = r.translated (0, pressed ? 2.0f : 0.0f);

    if (title == "<" || title == ">")                     // setas desenhadas
    {
        juce::Path tri;
        const auto cc = c.getCentre();
        const float s = juce::jmin (c.getWidth(), c.getHeight()) * 0.22f;
        if (title == "<") tri.addTriangle (cc.x - s, cc.y, cc.x + s * 0.8f, cc.y - s, cc.x + s * 0.8f, cc.y + s);
        else              tri.addTriangle (cc.x + s, cc.y, cc.x - s * 0.8f, cc.y - s, cc.x - s * 0.8f, cc.y + s);
        g.setColour (col::cream.withAlpha (isMouseOver() ? 1.0f : 0.75f));
        g.fillPath (tri);
        return;
    }

    auto text = c;
    if (showLamp)
    {
        lamp (g, { c.getX() + 17.0f, c.getCentreY() }, lit);
        text.removeFromLeft (24.0f);
    }
    const juce::Colour ink = cap == Cap::Dark ? (lit || ! showLamp ? col::cream : col::cream.withAlpha (0.6f))
                                              : (cap == Cap::Green ? col::jamGold : col::cream);
    g.setColour (ink);
    if (sub.isEmpty())
    {
        g.setFont (stencil (juce::jmin (13.5f, c.getHeight() * 0.42f), 0.16f));
        g.drawText (title, text, juce::Justification::centred);
    }
    else
    {
        g.setFont (stencil (15.0f, 0.18f));
        g.drawText (title, text.withTrimmedBottom (text.getHeight() * 0.42f), juce::Justification::centredBottom);
        g.setColour (ink.withAlpha (0.6f));
        g.setFont (stencil (9.0f, 0.24f));
        g.drawText (sub, text.withTrimmedTop (text.getHeight() * 0.62f), juce::Justification::centredTop);
    }
}

//==============================================================================
void ChoiceRow::bind (juce::RangedAudioParameter& p)
{
    att = std::make_unique<juce::ParameterAttachment> (p, [this] (float v) { value = v; repaint(); }, nullptr);
    att->sendInitialUpdate();
}

juce::Rectangle<float> ChoiceRow::cell (int i) const
{
    const int rows = (labels.size() + cols - 1) / cols;
    const float w = (float) getWidth() / (float) cols, h = (float) getHeight() / (float) rows;
    return { (float) (i % cols) * w, (float) (i / cols) * h, w, h };
}

void ChoiceRow::paint (juce::Graphics& g)
{
    const int sel = juce::roundToInt (value);
    for (int i = 0; i < labels.size(); ++i)
    {
        auto r = cell (i).reduced (3.0f);
        const bool on = toggle ? sel == 1 : sel == i;
        keyCap (g, r, Key::Cap::Dark, on, isMouseOver() && cell (i).contains (getMouseXYRelative().toFloat()));
        auto c = r.translated (0, on ? 2.0f : 0.0f);
        lamp (g, { c.getX() + 16.0f, c.getCentreY() }, on, 9.0f);
        g.setColour (on ? col::cream : col::cream.withAlpha (0.55f));
        g.setFont (stencil (11.5f, 0.14f));
        g.drawText (labels[i], c.withTrimmedLeft (22.0f), juce::Justification::centred);
    }
}

void ChoiceRow::mouseDown (const juce::MouseEvent& e)
{
    if (att == nullptr) return;
    if (toggle) { att->setValueAsCompleteGesture (value > 0.5f ? 0.0f : 1.0f); return; }
    for (int i = 0; i < labels.size(); ++i)
        if (cell (i).contains (e.position)) { att->setValueAsCompleteGesture ((float) i); return; }
}

//==============================================================================
KnobBox::KnobBox (juce::String captionIn, std::function<juce::String (float)> fmt)
    : caption (std::move (captionIn)), format (std::move (fmt))
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    slider.setLookAndFeel (&lnf);
    slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible (slider);
}

KnobBox::~KnobBox() { slider.setLookAndFeel (nullptr); }

void KnobBox::bind (juce::AudioProcessorValueTreeState& s, const juce::String& id)
{
    att.reset();
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    repaint();
}

void KnobBox::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (16);
    r.removeFromBottom (26);
    slider.setBounds (r);
}

void KnobBox::paint (juce::Graphics& g)
{
    g.setColour (col::ink);
    g.setFont (stencil (11.0f, 0.2f));
    g.drawText (caption, getLocalBounds().removeFromTop (14), juce::Justification::centred);

    // escala gravada em volta do knob
    const auto c = slider.getBounds().toFloat().getCentre();
    const float rad = juce::jmin (slider.getWidth(), slider.getHeight()) * 0.5f - 13.0f + 6.0f;
    g.setColour (col::ink.withAlpha (0.55f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = juce::MathConstants<float>::pi * (1.2f + 1.6f * (float) i / 10.0f);
        const float r1 = rad + 1.0f, r2 = rad + (i % 5 == 0 ? 6.0f : 4.0f);
        g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, 1.1f);
    }

    auto v = getLocalBounds().removeFromBottom (22).toFloat().reduced (8.0f, 2.0f);
    lcdGlass (g, v.reduced (0, 1));
    glow (g, format ? format ((float) slider.getValue()) : juce::String (slider.getValue(), 1), v, juce::Justification::centred, 11.0f);
    lcdShine (g, v.reduced (0, 1));
}

//==============================================================================
Pad::Pad (BatteryProcessor& p, int i) : proc (p), index (i)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setRepaintsOnMouseActivity (true);
}

void Pad::paint (juce::Graphics& g)
{
    auto well = getLocalBounds().toFloat().reduced (2.0f);
    // poço do pad na carcaça
    g.setColour (juce::Colour (0xff15120f));
    g.fillRoundedRectangle (well, 7.0f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (well.translated (0, 1.2f), 7.0f, 1.0f);

    // LED de seleção (fileira em cima do pad)
    auto pad = well.reduced (5.0f);
    if (flash > 0.01f || selected)
    {
        const float a = juce::jmax (flash, selected ? 0.35f : 0.0f);
        g.setColour (col::amber.withAlpha (0.55f * a));
        g.drawRoundedRectangle (well.reduced (1.5f), 6.5f, 2.5f);
    }

    // borracha (cinza quente de MPC), levemente côncava; afunda quando toca
    const float press = isMouseButtonDown() ? 1.5f : 0.0f;
    pad = pad.translated (0, press);
    juce::ColourGradient rubber (juce::Colour (0xffc4beb1), pad.getX(), pad.getY(),
                                 juce::Colour (0xff8c867a), pad.getX(), pad.getBottom(), false);
    rubber.addColour (0.5, juce::Colour (0xffaba597));
    g.setGradientFill (rubber);
    g.fillRoundedRectangle (pad, 5.0f);
    juce::ColourGradient dish (juce::Colours::black.withAlpha (0.10f), pad.getCentreX(), pad.getCentreY(),
                               juce::Colours::transparentBlack, pad.getX(), pad.getY(), true);
    g.setGradientFill (dish);
    g.fillRoundedRectangle (pad, 5.0f);
    juce::Random rng ((juce::int64) index * 7919 + 3);
    for (int i = 0; i < 260; ++i)                       // textura da borracha
    {
        g.setColour ((rng.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.05f + rng.nextFloat() * 0.05f));
        g.fillRect (pad.getX() + rng.nextFloat() * pad.getWidth(), pad.getY() + rng.nextFloat() * pad.getHeight(), 1.0f, 1.0f);
    }

    // luz âmbar de dentro da borracha (SP-404)
    if (flash > 0.01f)
    {
        juce::ColourGradient lit (juce::Colour (0xffffd27a).withAlpha (0.85f * flash), pad.getCentreX(), pad.getCentreY(),
                                  col::amber.withAlpha (0.35f * flash), pad.getX(), pad.getY(), true);
        g.setGradientFill (lit);
        g.fillRoundedRectangle (pad, 5.0f);
    }
    if (dropHover)
    {
        g.setColour (col::jamGreen.withAlpha (0.45f));
        g.fillRoundedRectangle (pad, 5.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawLine (pad.getX() + 6, pad.getY() + 1.2f, pad.getRight() - 6, pad.getY() + 1.2f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawRoundedRectangle (pad, 5.0f, 1.0f);

    // serigrafia
    auto txt = pad.reduced (8.0f, 5.0f);
    const juce::Colour ink = col::ink.withAlpha (0.82f);
    g.setColour (ink);
    g.setFont (stencil (10.0f, 0.12f));
    g.drawText (juce::String (index + 1).paddedLeft ('0', 2), txt, juce::Justification::topLeft);
    g.setColour (ink.withAlpha (0.6f));
    g.drawText (noteName (firstNote + index) + " " + juce::String (firstNote + index), txt, juce::Justification::topRight);
    g.setColour (ink);
    g.setFont (stencil (12.5f, 0.12f));
    g.drawText (pads[index].name, txt, juce::Justification::centredBottom);
}

void Pad::mouseDown (const juce::MouseEvent& e)
{
    if (onSelect) onSelect (index);
    const float y = juce::jlimit (0.0f, 1.0f, e.position.y / (float) juce::jmax (1, getHeight()));
    proc.triggerPad (index, juce::jlimit (30, 127, juce::roundToInt (127.0f - y * 90.0f)));
    repaint();
}

void Pad::mouseUp (const juce::MouseEvent&)
{
    proc.releasePad (index);
    repaint();
}

static bool isAudioFile (const juce::String& f)
{
    return juce::File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3");
}

bool Pad::isInterestedInFileDrag (const juce::StringArray& files) { return files.size() > 0 && isAudioFile (files[0]); }

void Pad::filesDropped (const juce::StringArray& files, int, int)
{
    dropHover = false;
    if (files.size() > 0 && proc.loadSampleFile (index, juce::File (files[0])))
    {
        if (onSelect) onSelect (index);
        if (onLoaded) onLoaded (index);
    }
    repaint();
}

//==============================================================================
SampleVisor::SampleVisor (BatteryProcessor& p) : proc (p) {}

void SampleVisor::setPad (int p) { pad = p; lastSample = nullptr; repaint(); }

juce::Rectangle<float> SampleVisor::waveArea() const
{
    return getLocalBounds().toFloat().reduced (14.0f, 0.0f).withTrimmedTop (34.0f).withTrimmedBottom (30.0f);
}

float SampleVisor::xOf (float f) const   { auto a = waveArea(); return a.getX() + f * a.getWidth(); }
float SampleVisor::fracOf (float x) const { auto a = waveArea(); return juce::jlimit (0.0f, 1.0f, (x - a.getX()) / a.getWidth()); }

void SampleVisor::tick()
{
    auto s = proc.getSample (pad);
    const float play = proc.getPlayPosition (pad);
    const float st = proc.padValue (pad, P::start), en = proc.padValue (pad, P::end);
    if (s.get() != lastSample || std::abs (play - lastPlay) > 1.0e-4f || st != lastStart || en != lastEnd)
    {
        lastSample = s.get(); lastPlay = play; lastStart = st; lastEnd = en;
        repaint();
    }
}

void SampleVisor::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    lcdGlass (g, r);
    auto s = proc.getSample (pad);
    const float st = proc.padValue (pad, P::start), en = proc.padValue (pad, P::end);

    // linha de cima: pad, nome, nota / arquivo e duração
    auto top = r.reduced (14.0f, 0.0f).removeFromTop (30.0f).withTrimmedTop (6.0f);
    glow (g, "PAD " + juce::String (pad + 1).paddedLeft ('0', 2) + "   " + pads[pad].name + "   "
                 + noteName (firstNote + pad) + " " + juce::String (firstNote + pad),
          top, juce::Justification::centredLeft, 14.0f);
    if (s != nullptr)
        glow (g, s->name + "   " + juce::String (juce::roundToInt (s->lengthMs())) + " MS", top, juce::Justification::centredRight, 12.0f, 0.75f);

    // grade do LCD
    auto wa = waveArea();
    g.setColour (col::amber.withAlpha (0.06f));
    for (int i = 1; i < 16; ++i) g.drawVerticalLine (juce::roundToInt (wa.getX() + wa.getWidth() * (float) i / 16.0f), wa.getY(), wa.getBottom());
    g.drawHorizontalLine (juce::roundToInt (wa.getCentreY()), wa.getX(), wa.getRight());

    // forma de onda (colunas âmbar; fora da região tocada fica apagada)
    if (s != nullptr && ! s->peaks.empty())
    {
        const int cols = juce::roundToInt (wa.getWidth() / 3.0f);
        const float bw = wa.getWidth() / (float) cols;
        for (int c = 0; c < cols; ++c)
        {
            const float f0 = (float) c / (float) cols, f1 = (float) (c + 1) / (float) cols;
            const int a = (int) (f0 * (float) s->peaks.size()), b = juce::jmax (a + 1, (int) (f1 * (float) s->peaks.size()));
            float m = 0.0f;
            for (int k = a; k < b && k < (int) s->peaks.size(); ++k) m = juce::jmax (m, s->peaks[(size_t) k]);
            const float h = juce::jmax (1.0f, m * wa.getHeight() * 0.48f);
            const bool in = f1 > st && f0 < en;
            g.setColour (in ? col::amber.withAlpha (0.9f) : col::amberDim.withAlpha (0.7f));
            g.fillRect (wa.getX() + (float) c * bw + 0.5f, wa.getCentreY() - h, bw - 1.0f, h * 2.0f);
        }
    }
    else
        glow (g, "SEM SAMPLE - ARRASTE UM WAV AQUI", wa, juce::Justification::centred, 13.0f, 0.6f);

    // sombra fora da região, marcadores INÍCIO e FIM
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRect (juce::Rectangle<float> (wa.getX(), wa.getY(), xOf (st) - wa.getX(), wa.getHeight()));
    g.fillRect (juce::Rectangle<float> (xOf (en), wa.getY(), wa.getRight() - xOf (en), wa.getHeight()));
    auto marker = [&] (float f, const juce::String& t, bool left, bool hot)
    {
        const float x = xOf (f);
        g.setColour (juce::Colour (0xffffe3a0).withAlpha (hot ? 1.0f : 0.85f));
        g.fillRect (x - (hot ? 1.0f : 0.5f), wa.getY() - 4.0f, hot ? 2.0f : 1.2f, wa.getHeight() + 8.0f);
        juce::Path flag;
        const float fy = wa.getY() - 4.0f;
        if (left) flag.addTriangle (x, fy, x + 12.0f, fy, x, fy + 12.0f);
        else      flag.addTriangle (x, fy, x - 12.0f, fy, x, fy + 12.0f);
        g.fillPath (flag);
        g.setFont (stencil (9.0f, 0.1f));
        g.drawText (t, juce::Rectangle<float> (left ? x + 3.0f : x - 23.0f, wa.getBottom() - 12.0f, 20.0f, 12.0f), juce::Justification::centred);
    };
    marker (st, "I", true, hover == 1 || dragging == 1);
    marker (en, "F", false, hover == 2 || dragging == 2);

    // posição tocando
    const float play = proc.getPlayPosition (pad);
    if (play >= 0.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillRect (xOf (play) - 0.75f, wa.getY(), 1.5f, wa.getHeight());
    }

    // linha de baixo: valores
    auto bottom = r.reduced (14.0f, 0.0f).removeFromBottom (26.0f).withTrimmedBottom (6.0f);
    const double len = s != nullptr ? s->lengthMs() : 0.0;
    juce::String info = "INICIO " + juce::String (juce::roundToInt (st * len)) + " MS     FIM "
                      + juce::String (juce::roundToInt (en * len)) + " MS     TOCA " + juce::String (juce::roundToInt ((en - st) * len)) + " MS";
    glow (g, info, bottom, juce::Justification::centredLeft, 11.0f, 0.8f);
    glow (g, "ARRASTE  I  E  F    -    DUPLO CLIQUE: SAMPLE INTEIRO", bottom, juce::Justification::centredRight, 9.5f, 0.5f);
    lcdShine (g, r);
}

void SampleVisor::mouseMove (const juce::MouseEvent& e)
{
    const float x = e.position.x;
    const float ds = std::abs (x - xOf (proc.padValue (pad, P::start))), de = std::abs (x - xOf (proc.padValue (pad, P::end)));
    const int h = juce::jmin (ds, de) < 10.0f ? (ds <= de ? 1 : 2) : 0;
    setMouseCursor (h != 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
    if (h != hover) { hover = h; repaint(); }
}

void SampleVisor::mouseDown (const juce::MouseEvent& e)
{
    const float f = fracOf (e.position.x);
    const float st = proc.padValue (pad, P::start), en = proc.padValue (pad, P::end);
    dragging = std::abs (f - st) <= std::abs (f - en) ? 1 : 2;   // pega o marcador mais perto
    auto& p = proc.padParam (pad, dragging == 1 ? P::start : P::end);
    p.beginChangeGesture();
    mouseDrag (e);
}

void SampleVisor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == 0) return;
    float f = fracOf (e.position.x);
    if (dragging == 1) f = juce::jmin (f, proc.padValue (pad, P::end) - 0.005f);
    else               f = juce::jmax (f, proc.padValue (pad, P::start) + 0.005f);
    auto& p = proc.padParam (pad, dragging == 1 ? P::start : P::end);
    p.setValueNotifyingHost (p.convertTo0to1 (juce::jlimit (0.0f, 1.0f, f)));
    repaint();
}

void SampleVisor::mouseUp (const juce::MouseEvent&)
{
    if (dragging != 0)
    {
        proc.padParam (pad, dragging == 1 ? P::start : P::end).endChangeGesture();
        proc.triggerPad (pad, 110);                // ouve o corte na hora
        juce::Timer::callAfterDelay (300, [this] { proc.releasePad (pad); });
    }
    dragging = 0;
    repaint();
}

void SampleVisor::mouseDoubleClick (const juce::MouseEvent&)
{
    for (auto [prm, v] : { std::pair<P, float> { P::start, 0.0f }, { P::end, 1.0f } })
    {
        auto& p = proc.padParam (pad, prm);
        p.beginChangeGesture(); p.setValueNotifyingHost (p.convertTo0to1 (v)); p.endChangeGesture();
    }
    repaint();
}

bool SampleVisor::isInterestedInFileDrag (const juce::StringArray& files) { return files.size() > 0 && isAudioFile (files[0]); }

void SampleVisor::filesDropped (const juce::StringArray& files, int, int)
{
    if (files.size() > 0 && proc.loadSampleFile (pad, juce::File (files[0])) && onLoaded) onLoaded();
    repaint();
}

//==============================================================================
EnvelopeView::EnvelopeView (BatteryProcessor& p) : proc (p) {}

void EnvelopeView::setPad (int p) { pad = p; repaint(); }

void EnvelopeView::tick()
{
    juce::String key;
    for (auto p : { P::attack, P::hold, P::decay, P::sustain, P::release, P::mode })
        key << juce::String (val (p), 3) << ";";
    if (key != lastKey) { lastKey = key; repaint(); }
}

// largura de cada trecho: raiz do tempo (os curtos ficam visíveis, os longos não estouram)
EnvelopeView::Shape EnvelopeView::shape() const
{
    Shape s;
    s.area = getLocalBounds().toFloat().reduced (16.0f, 0.0f).withTrimmedTop (34.0f).withTrimmedBottom (30.0f);
    const auto a = s.area;
    const float w = a.getWidth();
    auto seg = [&] (P p, float maxMs, float share) { return std::sqrt (juce::jlimit (0.0f, 1.0f, val (p) / maxMs)) * w * share; };
    const float y0 = a.getBottom(), y1 = a.getY();
    const float ys = y0 - val (P::sustain) * (y0 - y1);
    s.a = { a.getX() + 4.0f + seg (P::attack, 2000.0f, 0.18f), y1 };     // folga mínima: A e H nunca se encavalam
    s.h = { s.a.x + 14.0f + seg (P::hold, 2000.0f, 0.16f), y1 };
    s.d = { s.h.x + juce::jmax (4.0f, seg (P::decay, 5000.0f, 0.26f)), ys };
    s.s = { s.d.x + w * 0.14f, ys };
    s.r = { s.s.x + juce::jmax (4.0f, seg (P::release, 5000.0f, 0.24f)), y0 };
    return s;
}

static juce::String msText (float ms)
{
    return ms >= 1000.0f ? juce::String (ms / 1000.0f, 2) + " S" : juce::String (juce::roundToInt (ms)) + " MS";
}

void EnvelopeView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    lcdGlass (g, r);
    const auto s = shape();
    const bool oneShot = juce::roundToInt (val (P::mode)) == 0;

    auto top = r.reduced (16.0f, 0.0f).removeFromTop (30.0f).withTrimmedTop (6.0f);
    glow (g, "AHDSR   PAD " + juce::String (pad + 1).paddedLeft ('0', 2), top, juce::Justification::centredLeft, 13.0f);
    glow (g, oneShot ? "ONE SHOT" : "GATE", top, juce::Justification::centredRight, 11.0f, 0.75f);

    g.setColour (col::amber.withAlpha (0.06f));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine (juce::roundToInt (s.area.getY() + s.area.getHeight() * (float) i / 4.0f), s.area.getX(), s.area.getRight());

    juce::Path fill, line;
    const juce::Point<float> origin (s.area.getX(), s.area.getBottom());
    line.startNewSubPath (origin);
    line.lineTo (s.a);
    line.lineTo (s.h);
    line.quadraticTo ({ s.h.x + (s.d.x - s.h.x) * 0.15f, s.d.y }, s.d);   // curva exponencial do decay
    line.lineTo (s.s);
    fill = line;
    juce::Path rel;
    rel.startNewSubPath (s.s);
    rel.quadraticTo ({ s.s.x + (s.r.x - s.s.x) * 0.15f, s.r.y }, s.r);
    fill.quadraticTo ({ s.s.x + (s.r.x - s.s.x) * 0.15f, s.r.y }, s.r);
    fill.closeSubPath();

    g.setColour (col::amber.withAlpha (0.13f));
    g.fillPath (fill);
    g.setColour (col::amber.withAlpha (0.25f));
    g.strokePath (line, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (col::amber);
    g.strokePath (line, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (oneShot)   // no ONE SHOT o release só vale no choke: tracejado
    {
        juce::Path dashed;
        const float dashes[] = { 4.0f, 4.0f };
        juce::PathStrokeType (1.6f).createDashedStroke (dashed, rel, dashes, 2);
        g.setColour (col::amber.withAlpha (0.55f));
        g.fillPath (dashed);
    }
    else
    {
        g.setColour (col::amber);
        g.strokePath (rel, juce::PathStrokeType (2.0f));
    }

    // pontos arrastáveis
    const juce::Point<float> pts[] = { s.a, s.h, s.d, s.r };
    const char* names[] = { "A", "H", "D", "R" };
    for (int i = 0; i < 4; ++i)
    {
        const bool hot = hover == i + 1 || dragging == i + 1;
        const auto c = pts[i];
        g.setColour (juce::Colours::black);
        g.fillEllipse (juce::Rectangle<float> (13, 13).withCentre (c));
        g.setColour (hot ? juce::Colour (0xffffe3a0) : col::amber);
        g.fillEllipse (juce::Rectangle<float> (hot ? 11.0f : 9.0f, hot ? 11.0f : 9.0f).withCentre (c));
        g.setFont (stencil (9.5f, 0.1f));
        auto lab = juce::Rectangle<float> (20, 12).withCentre ({ c.x, c.y < s.area.getCentreY() ? c.y + 14.0f : c.y - 14.0f });
        if (i == 3) lab = lab.translated (-10.0f, -2.0f);
        g.setColour (col::amber.withAlpha (0.85f));
        g.drawText (names[i], lab, juce::Justification::centred);
    }
    g.setFont (stencil (9.5f, 0.1f));
    g.setColour (col::amber.withAlpha (0.85f));
    g.drawText ("S", juce::Rectangle<float> (20, 12).withCentre ({ (s.d.x + s.s.x) * 0.5f, s.s.y < s.area.getY() + 14.0f ? s.s.y + 13.0f : s.s.y - 10.0f }),
                juce::Justification::centred);   // sustain no alto: a letra vai para baixo da linha

    auto bottom = r.reduced (16.0f, 0.0f).removeFromBottom (26.0f).withTrimmedBottom (6.0f);
    glow (g, "A " + msText (val (P::attack)) + "   H " + msText (val (P::hold)) + "   D " + msText (val (P::decay))
                 + "   S " + juce::String (juce::roundToInt (val (P::sustain) * 100.0f)) + "%   R " + msText (val (P::release)),
          bottom, juce::Justification::centredLeft, 10.0f, 0.8f, 0.06f);
    lcdShine (g, r);
}

void EnvelopeView::setVal (P p, float plain)
{
    auto& prm = proc.padParam (pad, p);
    prm.setValueNotifyingHost (prm.convertTo0to1 (prm.getNormalisableRange().snapToLegalValue (plain)));
}

void EnvelopeView::mouseMove (const juce::MouseEvent& e)
{
    const auto s = shape();
    const juce::Point<float> pts[] = { s.a, s.h, s.d, s.r };
    int h = 0;
    float best = 14.0f;
    for (int i = 0; i < 4; ++i)
        if (pts[i].getDistanceFrom (e.position) < best) { best = pts[i].getDistanceFrom (e.position); h = i + 1; }
    if (h == 0 && std::abs (e.position.y - s.s.y) < 8.0f && e.position.x > s.d.x && e.position.x < s.s.x) h = 3;   // patamar do sustain
    setMouseCursor (h != 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    if (h != hover) { hover = h; repaint(); }
}

void EnvelopeView::mouseDown (const juce::MouseEvent& e)
{
    mouseMove (e);
    dragging = hover;
    if (dragging == 0) return;
    const P which[] = { P::attack, P::hold, P::decay, P::release };
    prm (which[dragging - 1]).beginChangeGesture();
    if (dragging == 3) prm (P::sustain).beginChangeGesture();
}

void EnvelopeView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == 0) return;
    const auto s = shape();
    const float w = s.area.getWidth();
    auto fromX = [&] (float dx, float maxMs, float share)
    {
        const float f = juce::jlimit (0.0f, 1.0f, dx / (w * share));
        return f * f * maxMs;
    };
    switch (dragging)
    {
        case 1: setVal (P::attack,  fromX (e.position.x - s.area.getX() - 4.0f, 2000.0f, 0.18f)); break;
        case 2: setVal (P::hold,    fromX (e.position.x - s.a.x - 14.0f, 2000.0f, 0.16f)); break;
        case 3:
            setVal (P::decay,   fromX (e.position.x - s.h.x, 5000.0f, 0.26f));
            setVal (P::sustain, juce::jlimit (0.0f, 1.0f, (s.area.getBottom() - e.position.y) / s.area.getHeight()));
            break;
        case 4: setVal (P::release, fromX (e.position.x - s.s.x, 5000.0f, 0.24f)); break;
        default: break;
    }
    repaint();
}

void EnvelopeView::mouseUp (const juce::MouseEvent&)
{
    if (dragging != 0)
    {
        const P which[] = { P::attack, P::hold, P::decay, P::release };
        prm (which[dragging - 1]).endChangeGesture();
        if (dragging == 3) prm (P::sustain).endChangeGesture();
        proc.triggerPad (pad, 110);
        juce::Timer::callAfterDelay (350, [this] { proc.releasePad (pad); });
    }
    dragging = 0;
}

void EnvelopeView::mouseDoubleClick (const juce::MouseEvent&)
{
    for (auto p : { P::attack, P::hold, P::decay, P::sustain, P::release })
    {
        auto& x = prm (p);
        x.beginChangeGesture(); x.setValueNotifyingHost (x.getDefaultValue()); x.endChangeGesture();
    }
    repaint();
}

//==============================================================================
void LedMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r, 4.0f);
    constexpr int segs = 16;
    const float db = juce::Decibels::gainToDecibels (level, -48.0f);
    const int on = juce::roundToInt (juce::jmap (db, -48.0f, 0.0f, 0.0f, (float) segs));
    auto in = r.reduced (5.0f, 6.0f);
    const float h = in.getHeight() / (float) segs;
    for (int i = 0; i < segs; ++i)
    {
        auto led = juce::Rectangle<float> (in.getX(), in.getBottom() - (float) (i + 1) * h, in.getWidth(), h).reduced (0, 1.2f);
        const juce::Colour c = i >= segs - 2 ? juce::Colour (0xffe0402a) : (i >= segs - 5 ? col::jamGold : col::jamGreen.brighter (0.3f));
        g.setColour (i < on ? c : c.withAlpha (0.13f));
        g.fillRoundedRectangle (led, 1.5f);
    }
}

//==============================================================================
Header::Header (BatteryProcessor& p, std::function<void (int)> onTab, std::function<void()> onPresetLoaded)
    : proc (p), presetLoaded (std::move (onPresetLoaded))
{
    for (auto* c : { &battery, &drumsTab, &play, &prev, &next, &save })
        addAndMakeVisible (c);
    prev.showLamp = next.showLamp = false;
    battery.onClick  = [onTab] { onTab (0); };
    drumsTab.onClick = [onTab] { onTab (1); };
    play.onClick = [this] { proc.setGroovePlaying (! proc.isGroovePlaying()); refresh(); };
    prev.onClick = [this] { stepPreset (-1); };
    next.onClick = [this] { stepPreset (1); };
    save.onClick = [this] { askSave(); };
    setSize (BatteryRoot::baseW, height);
}

void Header::refresh()
{
    play.setLit (proc.isGroovePlaying());
    repaint (presetDisplay.expanded (6));
}

void Header::stepPreset (int delta)
{
    const int n = proc.getPresetNames().size();
    if (n <= 0) return;
    proc.loadPreset ((proc.getPresetIndex() + delta + n) % n);
    refresh();
    if (presetLoaded) presetLoaded();
}

void Header::askSave()
{
    auto* w = new juce::AlertWindow ("GRAVAR PRESET", "Nome do preset (fica em Documentos / Dub Battery Reggae / Presets):",
                                     juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.getPresetName());
    w->addButton ("GRAVAR", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("CANCELAR", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Header> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result == 1 && safe != nullptr)
        {
            if (! safe->proc.savePreset (w->getTextEditorContents ("name")))
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "DUB BATTERY REGGAE",
                                                        u8"N\xc3\xa3o foi poss\xc3\xadvel gravar o preset.");
            safe->refresh();
        }
    }), true);
}

void Header::resized()
{
    modeSection   = { 52, 86, 520, 46 };
    battery.setBounds  (60, 88, 170, 44);
    drumsTab.setBounds (236, 88, 170, 44);
    play.setBounds     (420, 88, 146, 44);
    presetSection = { 600, 86, 628, 46 };
    prev.setBounds (608, 88, 44, 44);
    presetDisplay = { 662, 92, 360, 34 };
    next.setBounds (1030, 88, 44, 44);
    save.setBounds (1084, 88, 136, 44);
    background = {};
}

void Header::renderBackground()
{
    const float scale = 2.0f;
    background = juce::Image (juce::Image::ARGB, getWidth() * 2, getHeight() * 2, true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    juce::Random rng (2026);
    const int W = getWidth(), H = getHeight();
    paintWood (g, { 0, 0, 28, H }, rng);
    paintWood (g, { W - 28, 0, 28, H }, rng);
    const juce::Rectangle<int> plate (28, 0, W - 56, H);
    paintPlate (g, plate, rng, true, false);
    screw (g, (float) plate.getX() + 12.0f, 12.0f, rng);
    screw (g, (float) plate.getRight() - 12.0f, 12.0f, rng);

    // título (padrão da casa: versão logo depois do nome)
    g.setColour (col::ink);
    const juce::String title ("DUB BATTERY REGGAE " DBR_VERSION_SHORT);
    g.setFont (stencil (30.0f, 0.22f));
    g.drawText (title, plate.getX() + 32, 20, 700, 34, juce::Justification::left);
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
        g.setColour (juce::Colour (0xff3d2c0c).withAlpha (0.55f));
        g.drawRoundedRectangle (pl, 3.0f, 0.8f);
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
    g.setColour (col::inkSoft);
    g.setFont (stencil (10.5f, 0.25f));
    g.drawText ("16 PAD REGGAE DRUM SAMPLER", plate.getX() + 34, 52, 500, 14, juce::Justification::left);

    {   // faixas da bandeira da Jamaica, pintadas à mão e envelhecidas
        const float sx = (float) plate.getRight() - 330.0f, sw = 290.0f, sh = 7.0f;
        const juce::Colour faded[] = {
            col::jamGreen.interpolatedWith (juce::Colour (0xff6f7a4a), 0.35f),
            col::jamGold.interpolatedWith (juce::Colour (0xffb08a3a), 0.40f),
            juce::Colour (0xff2b2621) };
        for (int k = 0; k < 3; ++k)
        {
            const float y = 30.0f + 9.0f * (float) k;
            juce::Path band;
            band.startNewSubPath (sx, y + rng.nextFloat() * 0.8f);
            for (float x = sx; x <= sx + sw; x += 6.0f) band.lineTo (x, y + (rng.nextFloat() - 0.3f) * 0.9f);
            for (float x = sx + sw; x >= sx; x -= 6.0f) band.lineTo (x, y + sh + (rng.nextFloat() - 0.7f) * 0.9f);
            band.closeSubPath();
            juce::ColourGradient fade (faded[k].withAlpha (0.92f), sx, y, faded[k].withAlpha (0.70f), sx + sw, y, false);
            fade.addColour (0.3, faded[k].brighter (0.12f).withAlpha (0.85f));
            g.setGradientFill (fade);
            g.fillPath (band);
        }
        for (int i = 0; i < 700; ++i)
        {
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colour (0xff3d2b14)).withAlpha (0.08f + rng.nextFloat() * 0.10f));
            g.fillRect (sx + rng.nextFloat() * sw, 30.0f + rng.nextFloat() * 25.0f, 1.0f, 1.0f);
        }
    }

    section (g, modeSection.expanded (2, 4), "MODO");
    section (g, presetSection.expanded (2, 4), "PRESET");
}

void Header::paint (juce::Graphics& g)
{
    if (background.isNull()) renderBackground();
    g.drawImage (background, getLocalBounds().toFloat());

    auto d = presetDisplay.toFloat();
    lcdGlass (g, d);
    const int idx = proc.getPresetIndex();
    const bool user = idx >= proc.getNumFactoryPresets();
    glow (g, juce::String (idx + 1).paddedLeft ('0', 2), d.withTrimmedLeft (12.0f), juce::Justification::centredLeft, 15.0f, 0.65f);
    glow (g, proc.getPresetName(), d, juce::Justification::centred, 16.0f);
    glow (g, user ? "USUARIO" : "FABRICA", d.withTrimmedRight (12.0f), juce::Justification::centredRight, 9.0f, 0.55f);
    lcdShine (g, d);
}

//==============================================================================
static juce::String fmtDb (float v)   { return v <= -35.95f ? "-INF" : (v > 0.0f ? "+" : "") + juce::String (v, 1) + " DB"; }
static juce::String fmtPan (float v)  { const int p = juce::roundToInt (std::abs (v) * 100.0f); return p == 0 ? "C" : (v < 0 ? "L " : "R ") + juce::String (p); }
static juce::String fmtTune (float v) { return (v > 0.0f ? "+" : "") + juce::String (v, 2) + " ST"; }
static juce::String fmtHz (float v)   { return v >= 19500.0f ? "ABERTO" : (v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " KHZ" : juce::String (juce::roundToInt (v)) + " HZ"); }
static juce::String fmtPct (float v)  { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }

BatteryPanel::BatteryPanel (BatteryProcessor& p)
    : proc (p), visor (p), envelope (p),
      level ("VOLUME", fmtDb), pan ("PAN", fmtPan), tune ("AFINACAO", fmtTune), cutoff ("FILTRO", fmtHz),
      velocity ("VELOCITY", fmtPct), masterLevel ("VOLUME", fmtDb), tape ("FITA", fmtPct)
{
    for (int i = 0; i < numPads; ++i)
    {
        auto pad = std::make_unique<Pad> (proc, i);
        pad->onSelect = [this] (int k) { selectPad (k); };
        pad->onLoaded = [this] (int) { refreshAll(); };
        addAndMakeVisible (*pad);
        pads.push_back (std::move (pad));
    }
    addAndMakeVisible (visor);
    addAndMakeVisible (envelope);
    visor.onLoaded = [this] { refreshAll(); };
    for (auto* c : std::initializer_list<juce::Component*> { &prevSample, &nextSample, &load, &listen, &reverse, &mode, &choke,
                                                            &level, &pan, &tune, &cutoff, &velocity, &masterLevel, &tape, &meter })
        addAndMakeVisible (c);
    reverse.toggle = true;
    for (auto* k : { &prevSample, &nextSample, &load, &listen }) k->showLamp = false;
    prevSample.onClick = [this] { stepSample (-1); };
    nextSample.onClick = [this] { stepSample (1); };
    load.onClick = [this] { chooseFile(); };
    listen.onClick = [this]
    {
        proc.triggerPad (proc.selectedPad, 110);
        juce::Timer::callAfterDelay (400, [this] { proc.releasePad (proc.selectedPad); });
    };
    masterLevel.bind (proc.apvts, "masterLevel");
    tape.bind (proc.apvts, "tape");

    setSize (width, height);
    selectPad (proc.selectedPad);
    startTimerHz (30);
}

BatteryPanel::~BatteryPanel() { stopTimer(); }

void BatteryPanel::selectPad (int pad)
{
    pad = juce::jlimit (0, numPads - 1, pad);
    proc.selectedPad = pad;
    for (int i = 0; i < numPads; ++i)
    {
        pads[(size_t) i]->selected = i == pad;
        pads[(size_t) i]->repaint();
    }
    visor.setPad (pad);
    envelope.setPad (pad);
    level.bind    (proc.apvts, pid (pad, P::level));
    pan.bind      (proc.apvts, pid (pad, P::pan));
    tune.bind     (proc.apvts, pid (pad, P::tune));
    cutoff.bind   (proc.apvts, pid (pad, P::cutoff));
    velocity.bind (proc.apvts, pid (pad, P::velocity));
    reverse.bind (proc.padParam (pad, P::reverse));
    mode.bind    (proc.padParam (pad, P::mode));
    choke.bind   (proc.padParam (pad, P::choke));
    repaint (padDisplay.expanded (6));
}

void BatteryPanel::refreshAll()
{
    selectPad (proc.selectedPad);
    repaint();
}

void BatteryPanel::stepSample (int delta)
{
    const int n = BatteryProcessor::numFactorySamples();
    int cur = proc.factoryIndexOf (proc.selectedPad);
    if (cur < 0) cur = delta > 0 ? -1 : 0;
    proc.setFactorySample (proc.selectedPad, (cur + delta + n) % n);
    refreshAll();
    listen.onClick();
}

void BatteryPanel::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Carregar sample no pad " + juce::String (proc.selectedPad + 1),
                                                   juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                   "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile() && proc.loadSampleFile (proc.selectedPad, f))
                                  refreshAll();
                          });
}

void BatteryPanel::timerCallback()
{
    for (int i = 0; i < numPads; ++i)
    {
        auto& pad = *pads[(size_t) i];
        const auto hits = proc.getHitCount (i);
        float f = pad.flash;
        if (hits != pad.lastHit) { pad.lastHit = hits; f = 0.45f + 0.55f * proc.getHitVelocity (i); }
        else f *= 0.86f;
        if (f < 0.01f) f = 0.0f;
        if (f != pad.flash) { pad.flash = f; pad.repaint(); }
    }
    visor.tick();
    envelope.tick();
    meter.level = proc.getOutputLevel();
    meter.repaint();
}

void BatteryPanel::resized()
{
    const int W = getWidth(), H = getHeight();
    woodL = { 0, 0, 28, H };
    woodR = { W - 28, 0, 28, H };
    plate = { 28, 0, W - 56, H };

    sampleSec = { 52, 26, 776, 250 };
    envSec    = { 848, 26, 380, 250 };
    padSec    = { 52, 302, 560, 400 };
    padCtlSec = { 632, 302, 596, 230 };
    chokeSec  = { 632, 554, 280, 148 };
    masterSec = { 932, 554, 296, 148 };

    visor.setBounds (sampleSec.getX() + 14, sampleSec.getY() + 18, sampleSec.getWidth() - 28, 176);
    {
        auto row = juce::Rectangle<int> (sampleSec.getX() + 10, sampleSec.getY() + 204, sampleSec.getWidth() - 20, 40);
        prevSample.setBounds (row.removeFromLeft (118));
        nextSample.setBounds (row.removeFromLeft (118));
        row.removeFromLeft (14);
        load.setBounds (row.removeFromLeft (130));
        listen.setBounds (row.removeFromLeft (104));
        row.removeFromLeft (14);
        reverse.setBounds (row.removeFromLeft (150));
    }
    envelope.setBounds (envSec.getX() + 14, envSec.getY() + 18, envSec.getWidth() - 28, 176);
    mode.setBounds (envSec.getX() + 10, envSec.getY() + 204, envSec.getWidth() - 20, 40);

    {   // 16 pads: pad 1 embaixo à esquerda (MPC)
        auto area = padSec.reduced (16, 18);
        const int gap = 12;
        const int pw = (area.getWidth() - 3 * gap) / 4, ph = (area.getHeight() - 3 * gap) / 4;
        for (int i = 0; i < numPads; ++i)
        {
            const int col = i % 4, row = 3 - i / 4;
            pads[(size_t) i]->setBounds (area.getX() + col * (pw + gap), area.getY() + row * (ph + gap), pw, ph);
        }
    }

    padDisplay = { padCtlSec.getX() + 16, padCtlSec.getY() + 20, padCtlSec.getWidth() - 32, 32 };
    {
        auto row = juce::Rectangle<int> (padCtlSec.getX() + 8, padCtlSec.getY() + 62, padCtlSec.getWidth() - 16, 160);
        const int w = row.getWidth() / 5;
        for (auto* k : { &level, &pan, &tune, &cutoff, &velocity })
            k->setBounds (row.removeFromLeft (w).reduced (2, 0));
    }
    choke.setBounds (chokeSec.reduced (12, 0).withTrimmedTop (22).withHeight (88));
    masterLevel.setBounds (masterSec.getX() + 10, masterSec.getY() + 12, 104, 130);
    tape.setBounds (masterSec.getX() + 116, masterSec.getY() + 12, 104, 130);
    meter.setBounds (masterSec.getRight() - 58, masterSec.getY() + 18, 40, 118);
    background = {};
}

void BatteryPanel::renderBackground()
{
    background = juce::Image (juce::Image::ARGB, getWidth() * 2, getHeight() * 2, true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (2.0f));
    juce::Random rng (1976);
    paintWood (g, woodL, rng);
    paintWood (g, woodR, rng);
    paintPlate (g, plate, rng, false, true);
    juce::ColourGradient vig (juce::Colours::transparentBlack, (float) plate.getCentreX(), (float) plate.getCentreY() - 200.0f,
                              juce::Colours::black.withAlpha (0.22f), (float) plate.getX(), (float) plate.getBottom(), true);
    g.setGradientFill (vig);
    g.fillRect (plate);
    screw (g, (float) plate.getX() + 12.0f, (float) plate.getBottom() - 12.0f, rng);
    screw (g, (float) plate.getRight() - 12.0f, (float) plate.getBottom() - 12.0f, rng);

    section (g, sampleSec, "SAMPLE");
    section (g, envSec, "ENVELOPE");
    section (g, padSec, "PADS");
    section (g, padCtlSec, "PAD");
    section (g, chokeSec, "CHOKE");
    section (g, masterSec, "MASTER");

    g.setColour (col::inkSoft);
    g.setFont (stencil (9.0f, 0.28f));
    g.drawText ("CLIQUE TOCA E ESCOLHE  -  EM CIMA = MAIS FORTE  -  SOLTE UM WAV NO PAD",
                padSec.getX(), padSec.getBottom() + 8, padSec.getWidth(), 12, juce::Justification::centred);
    g.drawText ("MIDI: NOTAS C1 (36) A D#2 (51)  -  MESMO MAPA DO MRDM-12",
                padCtlSec.getX(), chokeSec.getBottom() + 8, masterSec.getRight() - padCtlSec.getX(), 12, juce::Justification::centred);
    g.drawText ("O PAD CORTA OS OUTROS DO MESMO GRUPO", chokeSec.getX(), chokeSec.getBottom() - 26, chokeSec.getWidth(), 12,
                juce::Justification::centred);
    g.drawText ("SAIDA", meter.getX() - 6, meter.getBottom() + 2, meter.getWidth() + 12, 10, juce::Justification::centred);
}

void BatteryPanel::paint (juce::Graphics& g)
{
    if (background.isNull()) renderBackground();
    g.drawImage (background, getLocalBounds().toFloat());

    // display do pad escolhido
    const int pad = proc.selectedPad;
    auto d = padDisplay.toFloat();
    lcdGlass (g, d);
    glow (g, "PAD " + juce::String (pad + 1).paddedLeft ('0', 2) + "  " + dbr::pads[pad].name, d.withTrimmedLeft (14.0f),
          juce::Justification::centredLeft, 15.0f);
    juce::String right = "NOTA " + noteName (firstNote + pad) + " " + juce::String (firstNote + pad);
    if (auto s = proc.getSample (pad)) right = s->name + "   " + right;
    glow (g, right, d.withTrimmedRight (14.0f), juce::Justification::centredRight, 11.0f, 0.7f);
    lcdShine (g, d);
}
} // namespace dbrui

//==============================================================================
namespace
{
    constexpr int kCropDrums = 74;   // o MRDM-12 aparece a partir da altura das plaquetas dos módulos
}

BatteryRoot::BatteryRoot (BatteryProcessor& p)
    : proc (p), batteryPanel (p), drumsPanel (p.getDrums()),
      header (p, [this] (int t) { showTab (t); }, [this] { batteryPanel.refreshAll(); })
{
    addChildComponent (batteryPanel);
    addChildComponent (drumsPanel);
    addAndMakeVisible (header);      // por cima: cobre o título do MRDM-12
    setSize (baseW, baseH);
    showTab (proc.currentTab);
    header.refresh();
}

void BatteryRoot::paintOverChildren (juce::Graphics& g)
{
    const float y = (float) dbrui::Header::height;
    juce::ColourGradient sh (juce::Colours::black.withAlpha (0.25f), 0.0f, y, juce::Colours::transparentBlack, 0.0f, y + 10.0f, false);
    g.setGradientFill (sh);
    g.fillRect (28.0f, y, (float) getWidth() - 56.0f, 10.0f);
}

void BatteryRoot::showTab (int t)
{
    t = juce::jlimit (0, 1, t);
    proc.currentTab = t;
    batteryPanel.setVisible (t == 0);
    drumsPanel.setVisible (t == 1);
    header.setTab (t);
}

void BatteryRoot::resized()
{
    header.setBounds (0, 0, baseW, dbrui::Header::height);
    batteryPanel.setBounds (0, dbrui::Header::height, baseW, dbrui::BatteryPanel::height);
    const float sx = (float) baseW / (float) drumsPanel.getWidth();
    const float sy = (float) dbrui::BatteryPanel::height / (float) (drumsPanel.getHeight() - kCropDrums);
    drumsPanel.setTransform (juce::AffineTransform::scale (sx, sy)
                                 .translated (0.0f, (float) dbrui::Header::height - (float) kCropDrums * sy));
}

//==============================================================================
BatteryEditor::BatteryEditor (BatteryProcessor& p) : juce::AudioProcessorEditor (p), root (p)
{
    addAndMakeVisible (root);
    const int w = root.getWidth(), h = root.getHeight();
    setResizable (true, true);
    setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);

    // abre num tamanho que caiba na tela (notebooks com 768/900 px de altura)
    double s = 1.0;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        s = juce::jlimit (0.55, 1.0, ((double) d->userArea.getHeight() - 90.0) / (double) h);
    setSize (juce::roundToInt (w * s), juce::roundToInt (h * s));
}

void BatteryEditor::resized()
{
    root.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) root.getWidth()));
}
