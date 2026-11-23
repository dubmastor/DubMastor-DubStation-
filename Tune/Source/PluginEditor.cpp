#include "PluginEditor.h"
#include "BinaryData.h"
#include <tuple>
#include "../../RiddimMachine/Source/MrmStyle.h"

using namespace mrmui;

namespace
{
const char* const kNoteNames[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };

juce::String noteName (int midi)
{
    if (midi < 0) return "--";
    return juce::String (kNoteNames[midi % 12]) + juce::String (midi / 12 - 1);
}

juce::Font script (float h)
{
    static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor (BinaryData::KaushanScriptRegular_ttf,
                                                                             (size_t) BinaryData::KaushanScriptRegular_ttfSize);
    return juce::Font (juce::FontOptions (tf).withHeight (h));
}

// vidro escuro com borda (displays âmbar, editor de notas, EQ)
void darkGlass (juce::Graphics& g, juce::Rectangle<float> r, float corner = 4.0f)
{
    displayGlass (g, r, corner, 0.12f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (r, corner, 1.0f);
}

void lampDot (juce::Graphics& g, juce::Point<float> c, bool on, juce::Colour col)
{
    lamp (g, juce::Rectangle<float> (10.0f, 10.0f).withCentre (c), on, col);
}

// cromado com saia preta (igual aos knobs do Control Room)
void chromeKnob (juce::Graphics& g, juce::Point<float> ctr, float kr, float ang, juce::Colour pointer)
{
    auto knob = juce::Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (ctr);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (knob.translated (1.5f, 3.0f));
    juce::ColourGradient skirt (juce::Colour (0xff3a3632), knob.getX(), knob.getY(), juce::Colour (0xff0e0c0a), knob.getX(), knob.getBottom(), false);
    g.setGradientFill (skirt);
    g.fillEllipse (knob);
    auto cap = knob.reduced (kr * 0.22f);
    juce::ColourGradient chrome (juce::Colour (0xfff4f2ec), cap.getX(), cap.getY(), juce::Colour (0xff5e5a52), cap.getRight(), cap.getBottom(), false);
    chrome.addColour (0.5, juce::Colour (0xffb9b5ac));
    g.setGradientFill (chrome);
    g.fillEllipse (cap);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawEllipse (cap, 1.0f);
    const float p1 = kr * 0.2f, p2 = kr * 0.95f;
    g.setColour (pointer);
    g.drawLine (ctr.x + std::sin (ang) * p1, ctr.y - std::cos (ang) * p1, ctr.x + std::sin (ang) * p2, ctr.y - std::cos (ang) * p2, juce::jmax (2.0f, kr * 0.09f));
}

constexpr float kA0 = juce::MathConstants<float>::pi * 1.25f, kA1 = juce::MathConstants<float>::pi * 2.75f;
} // namespace

namespace mtui
{
//==============================================================================
void LampKey::paint (juce::Graphics& g)
{
    const bool on = isOn ? isOn() : false;
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const bool over = isMouseOver();
    if (green)
    {
        g.setColour (juce::Colour (0xff1a1714));
        g.fillRoundedRectangle (r, 6.0f);
        if (pulse && on)
        {
            const float a = 0.5f * (1.0f - pulsePhase);
            g.setColour (juce::Colour (0xff5cff7a).withAlpha (a * 0.6f));
            g.drawRoundedRectangle (r.expanded (2.0f + 5.0f * pulsePhase), 8.0f, 2.0f);
        }
        auto cap = r.reduced (4.0f).translated (0, pressed ? 1.5f : 0.0f);
        const juce::Colour base = col::jamGreen;
        juce::ColourGradient cg (base.brighter (over ? 0.12f : 0.0f), cap.getX(), cap.getY(), base.darker (0.75f), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (cap, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.2f));
        g.fillRoundedRectangle (cap.withHeight (cap.getHeight() * 0.38f).reduced (4, 2), 3.0f);
        lamp (g, juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ cap.getX() + 18.0f, cap.getCentreY() }), on, juce::Colour (0xff5cff7a));
        g.setColour (col::jamGold);
        g.setFont (stencil (fontSize, 0.12f));
        g.drawFittedText (text, cap.withTrimmedLeft (30.0f).toNearestInt(), juce::Justification::centred, 1, 0.7f);
        return;
    }

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (0, 2.5f), 4.0f);
    auto cap = r.translated (0, (on || pressed) ? 1.5f : 0.0f);
    juce::ColourGradient cg (juce::Colour (on ? 0xff2c2925 : 0xff3a3632).brighter (over && ! on ? 0.09f : 0.0f),
                             cap.getX(), cap.getY(), juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.04f : 0.12f));
    g.drawHorizontalLine ((int) cap.getY() + 1, cap.getX() + 4, cap.getRight() - 4);
    if (on)
    {
        g.setColour (col::amber.withAlpha (0.8f));
        g.fillRoundedRectangle (cap.withTop (cap.getBottom() - 3.0f).reduced (cap.getWidth() * 0.12f, 0.0f).translated (0, -2.0f), 1.5f);
    }
    const bool withLamp = cap.getWidth() > 70.0f;
    if (withLamp)
        lamp (g, juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ cap.getX() + 14.0f, cap.getCentreY() - (sub.isNotEmpty() ? 0.0f : 0.5f) }), on);
    auto txt = cap.withTrimmedLeft (withLamp ? 24.0f : 4.0f).withTrimmedRight (4.0f);
    g.setColour (on ? col::amber : col::cream.withAlpha (0.66f));
    g.setFont (stencil (fontSize, 0.12f));
    if (sub.isEmpty())
        g.drawFittedText (text, txt.withTrimmedBottom (2.0f).toNearestInt(), juce::Justification::centred, 1, 0.6f);
    else
    {
        g.drawFittedText (text, txt.withTrimmedBottom (txt.getHeight() * 0.42f).toNearestInt(), juce::Justification::centredBottom, 1, 0.7f);
        g.setColour (on ? col::cream : col::cream.withAlpha (0.42f));
        g.setFont (stencil (8.5f, 0.22f));
        g.drawFittedText (sub, txt.withTrimmedTop (txt.getHeight() * 0.6f).toNearestInt(), juce::Justification::centredTop, 1, 0.7f);
    }
}

//==============================================================================
Knob::Knob (juce::RangedAudioParameter& p, juce::String l, float)
    : param (p), att (p, [this] (float v) { value = param.convertTo0to1 (v); repaint(); }, nullptr), label (std::move (l))
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void Knob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const auto labelR = r.removeFromTop (14.0f);
    const auto valueR = r.removeFromBottom (15.0f);
    const float size = juce::jmin (r.getWidth(), r.getHeight());
    const auto ctr = r.getCentre();
    const float rOut = size * 0.5f - 1.0f;
    const auto ink = darkPanel ? col::cream.withAlpha (0.75f) : col::ink.withAlpha (0.7f);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = kA0 + (kA1 - kA0) * (float) i / 10.0f;
        const float r1 = rOut, r2 = rOut - (i % 5 == 0 ? 5.0f : 3.0f);
        g.setColour (ink);
        g.drawLine (ctr.x + std::sin (a) * r1, ctr.y - std::cos (a) * r1, ctr.x + std::sin (a) * r2, ctr.y - std::cos (a) * r2, 1.1f);
    }
    const float ang = kA0 + (kA1 - kA0) * value;
    chromeKnob (g, ctr, rOut - 7.0f, ang, col::ink);
    if (darkPanel)
    {
        g.setColour (col::cream);
        g.setFont (stencil (10.0f, 0.2f));
        g.drawText (label, labelR, juce::Justification::centred);
    }
    else engraved (g, label, labelR, juce::Justification::centred, 10.0f, 0.2f, col::ink);
    g.setColour (darkPanel ? col::cream.withAlpha (0.7f) : col::inkSoft);
    g.setFont (juce::Font (juce::FontOptions (11.5f)));
    g.drawText (param.getCurrentValueAsText(), valueR, juce::Justification::centred);
}

void Knob::mouseDown (const juce::MouseEvent&) { startNorm = value; att.beginGesture(); }
void Knob::mouseUp (const juce::MouseEvent&) { att.endGesture(); }
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    const float nv = juce::jlimit (0.0f, 1.0f, startNorm - (float) e.getDistanceFromDragStartY() / 180.0f);
    att.setValueAsPartOfGesture (param.convertFrom0to1 (nv));
}
void Knob::mouseDoubleClick (const juce::MouseEvent&) { att.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    att.setValueAsCompleteGesture (param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, value + w.deltaY * 0.08f)));
}

//==============================================================================
TuneKnob::TuneKnob (juce::RangedAudioParameter& p)
    : param (p), att (p, [this] (float v) { value = v; repaint(); }, nullptr)
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

static juce::Colour cleanRobotColour (float t)   // 0..1: verde -> âmbar -> vermelho
{
    const juce::Colour g (0xff3be070), a = col::amber, r (0xffd8442c);
    return t < 0.6f ? g.interpolatedWith (a, t / 0.6f) : a.interpolatedWith (r, (t - 0.6f) / 0.4f);
}

void TuneKnob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float size = juce::jmin (r.getWidth(), r.getHeight());
    const auto ctr = r.getCentre();
    const float rOut = size * 0.5f - 4.0f;
    const float t = value / 100.0f;
    const float ang = kA0 + (kA1 - kA0) * t;

    for (int i = 0; i <= 20; ++i)
    {
        const float a = kA0 + (kA1 - kA0) * (float) i / 20.0f;
        const float r1 = rOut, r2 = rOut - (i % 5 == 0 ? 11.0f : 6.0f);
        g.setColour (col::ink.withAlpha (0.75f));
        g.drawLine (ctr.x + std::sin (a) * r1, ctr.y - std::cos (a) * r1, ctr.x + std::sin (a) * r2, ctr.y - std::cos (a) * r2, i % 5 == 0 ? 2.0f : 1.2f);
    }
    // arco de luz até o valor
    const auto c = cleanRobotColour (t);
    juce::Path arc;
    arc.addCentredArc (ctr.x, ctr.y, rOut - 17.0f, rOut - 17.0f, 0.0f, kA0, juce::jmax (kA0 + 0.02f, ang), true);
    g.setColour (c.withAlpha (0.28f));
    g.strokePath (arc, juce::PathStrokeType (12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c);
    g.strokePath (arc, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // botão grande: saia preta frisada, anel cromado, centro dourado (como o Big Knob)
    const float kr = rOut - 28.0f;
    auto knob = juce::Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (ctr);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (knob.translated (3.0f, 6.0f));
    juce::ColourGradient ring (juce::Colour (0xfff4f2ec), knob.getX(), knob.getY(), juce::Colour (0xff5e5a52), knob.getRight(), knob.getBottom(), false);
    ring.addColour (0.5, juce::Colour (0xffb9b5ac));
    g.setGradientFill (ring);
    g.fillEllipse (knob);
    auto skirt = knob.reduced (kr * 0.12f);
    juce::ColourGradient sk (juce::Colour (0xff3a3632), skirt.getX(), skirt.getY(), juce::Colour (0xff0b0a09), skirt.getX(), skirt.getBottom(), false);
    g.setGradientFill (sk);
    g.fillEllipse (skirt);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    for (int i = 0; i < 48; ++i)
    {
        const float a = (float) i / 48.0f * juce::MathConstants<float>::twoPi;
        g.drawLine (ctr.x + std::sin (a) * kr * 0.72f, ctr.y - std::cos (a) * kr * 0.72f, ctr.x + std::sin (a) * kr * 0.86f, ctr.y - std::cos (a) * kr * 0.86f, 1.0f);
    }
    const float dr = kr * 0.34f;
    auto dot = juce::Rectangle<float> (dr * 2.0f, dr * 2.0f).withCentre (ctr);
    juce::ColourGradient gold (juce::Colour (0xfffff0a0), dot.getX(), dot.getY(), juce::Colour (0xff9c7a05), dot.getRight(), dot.getBottom(), false);
    gold.addColour (0.5, col::jamGold);
    g.setGradientFill (gold);
    g.fillEllipse (dot);
    g.setColour (col::cream);
    g.drawLine (ctr.x + std::sin (ang) * dr * 1.15f, ctr.y - std::cos (ang) * dr * 1.15f,
                ctr.x + std::sin (ang) * kr * 0.84f, ctr.y - std::cos (ang) * kr * 0.84f, 6.0f);

    engraved (g, "CLEAN", { ctr.x - rOut - 10.0f, ctr.y + rOut * 0.62f, 80.0f, 16.0f }, juce::Justification::centredLeft, 13.0f, 0.2f, juce::Colour (0xff0b6a2e));
    engraved (g, u8 ("ROB\xc3\x94"), { ctr.x + rOut - 70.0f, ctr.y + rOut * 0.62f, 80.0f, 16.0f }, juce::Justification::centredRight, 13.0f, 0.2f, juce::Colour (0xffb8361f));
    engraved (g, "FIRME", { ctr.x - 40.0f, ctr.y - rOut - 15.0f, 80.0f, 14.0f }, juce::Justification::centred, 10.0f, 0.2f);
}

void TuneKnob::mouseDown (const juce::MouseEvent&) { startNorm = value / 100.0f; att.beginGesture(); }
void TuneKnob::mouseUp (const juce::MouseEvent&) { att.endGesture(); }
void TuneKnob::mouseDrag (const juce::MouseEvent& e)
{
    const float nv = juce::jlimit (0.0f, 1.0f, startNorm - (float) (e.getDistanceFromDragStartY() - e.getDistanceFromDragStartX()) / 300.0f);
    att.setValueAsPartOfGesture (nv * 100.0f);
}
void TuneKnob::mouseDoubleClick (const juce::MouseEvent&) { att.setValueAsCompleteGesture (25.0f); }
void TuneKnob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    att.setValueAsCompleteGesture (juce::jlimit (0.0f, 100.0f, value + w.deltaY * 10.0f));
}

//==============================================================================
static const int kWhite[7] = { 0, 2, 4, 5, 7, 9, 11 };
static const int kBlack[5] = { 1, 3, 6, 8, 10 };
static const float kBlackPos[5] = { 1.0f, 2.0f, 4.0f, 5.0f, 6.0f };   // fronteira entre teclas brancas

int KeyPiano::keyAt (juce::Point<float> p) const
{
    const float w = (float) getWidth() / 7.0f, bh = (float) getHeight() * 0.6f, bw = w * 0.58f;
    if (p.y < bh)
        for (int i = 0; i < 5; ++i)
            if (std::abs (p.x - kBlackPos[i] * w) < bw * 0.5f) return kBlack[i];
    return kWhite[juce::jlimit (0, 6, (int) (p.x / w))];
}

void KeyPiano::mouseDown (const juce::MouseEvent& e) { proc.chooseKey (keyAt (e.position)); }

void KeyPiano::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float w = r.getWidth() / 7.0f, bh = r.getHeight() * 0.6f, bw = w * 0.58f;
    g.setColour (col::labelBg);
    g.fillRect (r.withHeight (3.0f));
    for (int i = 0; i < 7; ++i)
    {
        auto k = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY() + 3.0f, w, r.getHeight() - 3.0f).reduced (0.6f, 0);
        const bool on = shown == kWhite[i];
        juce::ColourGradient kg (on ? juce::Colour (0xfff3d9a0) : juce::Colour (0xfff3ecd8), k.getX(), k.getY(),
                                 on ? juce::Colour (0xffe7b860) : juce::Colour (0xffdccfae), k.getX(), k.getBottom(), false);
        g.setGradientFill (kg);
        g.fillRoundedRectangle (k, 2.0f);
        g.setColour (col::ink.withAlpha (0.5f));
        g.drawRoundedRectangle (k, 2.0f, 0.8f);
        if (on)
        {
            g.setColour (col::amber.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (14, 14).withCentre ({ k.getCentreX(), k.getBottom() - 26.0f }));
            g.setColour (col::amber);
            g.fillEllipse (juce::Rectangle<float> (8, 8).withCentre ({ k.getCentreX(), k.getBottom() - 26.0f }));
        }
        g.setColour (col::inkSoft);
        g.setFont (stencil (10.0f, 0.05f));
        g.drawText (kNoteNames[kWhite[i]], k.withTop (k.getBottom() - 16.0f), juce::Justification::centred);
    }
    for (int i = 0; i < 5; ++i)
    {
        auto k = juce::Rectangle<float> (r.getX() + kBlackPos[i] * w - bw * 0.5f, r.getY() + 3.0f, bw, bh);
        const bool on = shown == kBlack[i];
        juce::ColourGradient kg (on ? juce::Colour (0xff7a4a10) : juce::Colour (0xff2a2520), k.getX(), k.getY(),
                                 on ? juce::Colour (0xff3a2206) : juce::Colour (0xff0d0b09), k.getX(), k.getBottom(), false);
        g.setGradientFill (kg);
        g.fillRoundedRectangle (k, 2.0f);
        g.setColour (on ? col::amber : col::cream.withAlpha (0.6f));
        g.setFont (stencil (8.5f, 0.0f));
        g.drawText (kNoteNames[kBlack[i]], k.withTop (k.getBottom() - 14.0f), juce::Justification::centred);
    }
}

//==============================================================================
void CentsMeter::set (float cents, bool active)
{
    on = active;
    target = active ? juce::jlimit (-50.0f, 50.0f, cents) : 0.0f;
    // ponteiro com massa (como um VU de verdade)
    const float acc = (target - needle) * 0.35f - vel * 0.45f;
    vel += acc; needle += vel;
    repaint();
}

void CentsMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::labelBg);
    g.fillRoundedRectangle (r, 6.0f);
    auto face = r.reduced (6.0f);
    juce::ColourGradient fg (juce::Colour (0xfff7e9c0), face.getCentreX(), face.getBottom() + 30.0f, juce::Colour (0xffd9c07a), face.getCentreX(), face.getY(), true);
    g.setGradientFill (fg);
    g.fillRoundedRectangle (face, 3.0f);
    const auto pivot = juce::Point<float> (face.getCentreX(), face.getBottom() + 8.0f);
    const float R = face.getHeight() * 0.92f, swing = 0.78f;
    for (int i = -10; i <= 10; ++i)
    {
        const float a = swing * (float) i / 10.0f;
        const bool major = i % 5 == 0;
        g.setColour (std::abs (i) >= 6 ? juce::Colour (0xffc8301e) : col::ink);
        g.drawLine (pivot.x + std::sin (a) * R, pivot.y - std::cos (a) * R, pivot.x + std::sin (a) * (R - (major ? 10.0f : 6.0f)),
                    pivot.y - std::cos (a) * (R - (major ? 10.0f : 6.0f)), major ? 1.6f : 1.0f);
    }
    g.setColour (col::ink);
    g.setFont (stencil (9.5f, 0.05f));
    for (int i : { -50, -25, 0, 25, 50 })
    {
        const float a = swing * (float) i / 50.0f;
        g.drawText (i > 0 ? "+" + juce::String (i) : juce::String (i),
                    juce::Rectangle<float> (40, 12).withCentre ({ pivot.x + std::sin (a) * (R + 7.0f), pivot.y - std::cos (a) * (R + 7.0f) }), juce::Justification::centred);
    }
    g.setFont (stencil (13.0f, 0.2f));
    g.drawText ("CENTS", face.withTrimmedTop (face.getHeight() * 0.45f).withHeight (18.0f), juce::Justification::centred);
    const float a = swing * needle / 50.0f;
    g.setColour (on ? juce::Colour (0xff111111) : juce::Colour (0x66111111));
    g.drawLine (pivot.x, pivot.y, pivot.x + std::sin (a) * (R + 2.0f), pivot.y - std::cos (a) * (R + 2.0f), 1.8f);
    g.setColour (col::labelBg);
    g.fillRect (juce::Rectangle<float> (face.getWidth() * 0.34f, 9.0f).withCentre ({ face.getCentreX(), face.getBottom() - 3.0f }));
}

//==============================================================================
void ScaleKeyboard::set (uint16_t m, float s, int t, bool b)
{
    const bool lit = s > 0.0f && ! b;
    glow += ((lit ? 1.0f : 0.0f) - glow) * 0.35f;
    if (m != mask || std::abs (s - sung) > 0.02f || t != target || b != byp || glow > 0.01f)
    {
        mask = m; sung = s; target = t; byp = b;
        repaint();
    }
}

void ScaleKeyboard::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::labelBg);
    g.fillRoundedRectangle (r, 4.0f);
    r = r.reduced (4.0f);
    int whites = 0;
    for (int m = kLow; m <= kHigh; ++m) if (! juce::MidiMessage::isMidiNoteBlack (m)) ++whites;
    const float ww = r.getWidth() / (float) whites;
    const int sungNote = sung > 0.0f ? juce::roundToInt (sung) : -1;
    auto inScale = [this] (int m) { return ((mask >> (m % 12)) & 1) != 0; };

    // teclas brancas
    int wi = 0;
    std::vector<std::pair<int, float>> blacks;
    for (int m = kLow; m <= kHigh; ++m)
    {
        if (juce::MidiMessage::isMidiNoteBlack (m)) { blacks.push_back ({ m, r.getX() + ww * (float) wi }); continue; }
        auto k = juce::Rectangle<float> (r.getX() + ww * (float) wi, r.getY(), ww, r.getHeight()).reduced (0.6f, 0);
        ++wi;
        const bool on = inScale (m);
        const bool isT = ! byp && m == target, isS = m == sungNote;
        juce::Colour top = on ? juce::Colour (0xfff3ecd8) : juce::Colour (0xff8f877a);
        juce::Colour bot = on ? juce::Colour (0xffdccfae) : juce::Colour (0xff6f675c);
        if (isT) { top = juce::Colour (0xfff3d9a0).interpolatedWith (top, 1.0f - glow); bot = juce::Colour (0xffe7b860).interpolatedWith (bot, 1.0f - glow); }
        juce::ColourGradient kg (top, k.getX(), k.getY(), bot, k.getX(), k.getBottom(), false);
        g.setGradientFill (kg);
        g.fillRoundedRectangle (k, 2.0f);
        g.setColour (col::ink.withAlpha (0.5f));
        g.drawRoundedRectangle (k, 2.0f, 0.8f);
        if (! on)
        {
            g.setColour (col::ink.withAlpha (0.35f));
            g.drawLine (k.getX() + 4.0f, k.getBottom() - 24.0f, k.getRight() - 4.0f, k.getBottom() - 12.0f, 1.2f);
        }
        if (isT)
        {
            g.setColour (col::amber.withAlpha (0.4f * glow));
            g.fillEllipse (juce::Rectangle<float> (16, 16).withCentre ({ k.getCentreX(), k.getBottom() - 14.0f }));
            g.setColour (col::amber.withAlpha (glow));
            g.fillEllipse (juce::Rectangle<float> (9, 9).withCentre ({ k.getCentreX(), k.getBottom() - 14.0f }));
        }
        if (isS && ! isT)
        {
            g.setColour (juce::Colour (0xffff5a3c));
            g.drawEllipse (juce::Rectangle<float> (11, 11).withCentre ({ k.getCentreX(), k.getBottom() - 14.0f }), 2.0f);
        }
        if (m % 12 == 0)
        {
            g.setColour (col::inkSoft);
            g.setFont (stencil (8.5f, 0.0f));
            g.drawText ("C" + juce::String (m / 12 - 1), k.withTop (k.getBottom() - 11.0f), juce::Justification::centred);
        }
    }
    // teclas pretas
    for (auto [m, x] : blacks)
    {
        auto k = juce::Rectangle<float> (x - ww * 0.3f, r.getY(), ww * 0.6f, r.getHeight() * 0.6f);
        const bool on = inScale (m);
        const bool isT = ! byp && m == target, isS = m == sungNote;
        juce::Colour top = on ? juce::Colour (0xff2a2520) : juce::Colour (0xff8a837a);
        if (isT) top = juce::Colour (0xffb06a10).interpolatedWith (top, 1.0f - glow);
        juce::ColourGradient kg (top, k.getX(), k.getY(), on ? juce::Colour (0xff0d0b09) : juce::Colour (0xff6a645c), k.getX(), k.getBottom(), false);
        g.setGradientFill (kg);
        g.fillRoundedRectangle (k, 2.0f);
        if (isT)
        {
            g.setColour (col::amber.withAlpha (glow));
            g.fillEllipse (juce::Rectangle<float> (7, 7).withCentre ({ k.getCentreX(), k.getBottom() - 9.0f }));
        }
        if (isS && ! isT)
        {
            g.setColour (juce::Colour (0xffff5a3c));
            g.drawEllipse (juce::Rectangle<float> (9, 9).withCentre ({ k.getCentreX(), k.getBottom() - 9.0f }), 1.8f);
        }
    }
}

//==============================================================================
NoteEditor::NoteEditor (MastorTuneProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

juce::Rectangle<float> NoteEditor::rollArea() const { return getLocalBounds().toFloat().reduced (6.0f).withTrimmedLeft (34.0f); }
float NoteEditor::xOf (double ppq) const { auto a = rollArea(); return a.getX() + (float) ((ppq - viewStart) / viewBeats) * a.getWidth(); }
float NoteEditor::yOf (float midi) const { auto a = rollArea(); return a.getBottom() - (midi - lowNote + 0.5f) / (highNote - lowNote + 1.0f) * a.getHeight(); }

void NoteEditor::refresh (bool changed)
{
    if (follow && proc.isHostPlaying())
    {
        const double ppq = proc.getPlayPpq();
        if (ppq < viewStart + viewBeats * 0.1 || ppq > viewStart + viewBeats * 0.8)
            viewStart = std::floor ((ppq - viewBeats * 0.2) / 4.0) * 4.0;
    }
    if (changed || follow)
    {
        // faixa de notas: as que aparecem na janela, com folga
        float lo = 200.0f, hi = -1.0f;
        for (auto& n : proc.getNotes().getNotes())
            if (n.end >= viewStart && n.start <= viewStart + viewBeats)
            { lo = juce::jmin (lo, (float) n.shown()); hi = juce::jmax (hi, (float) n.shown()); }
        if (hi < 0) { lo = 52; hi = 67; }
        float mid = 0.5f * (lo + hi), span = juce::jmax (14.0f, hi - lo + 6.0f);
        lowNote = std::floor (mid - span * 0.5f); highNote = lowNote + std::ceil (span);
    }
    repaint();
}

void NoteEditor::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    darkGlass (g, r, 4.0f);
    const auto a = rollArea();
    const auto st = proc.getStatus();
    const uint16_t mask = mt::scaleMask (st.keyRoot, st.scale);
    const float rowH = a.getHeight() / (highNote - lowNote + 1.0f);

    // linhas das notas (as da escala um pouco mais claras) e nomes
    for (int m = (int) lowNote; m <= (int) highNote; ++m)
    {
        const float y = yOf ((float) m);
        const bool inScale = (mask >> (m % 12)) & 1;
        if (inScale)
        {
            g.setColour (col::amber.withAlpha (0.06f));
            g.fillRect (a.getX(), y - rowH * 0.5f, a.getWidth(), rowH);
        }
        g.setColour (col::amber.withAlpha (0.10f));
        g.drawHorizontalLine ((int) (y + rowH * 0.5f), a.getX(), a.getRight());
        if (rowH >= 7.0f && (inScale || rowH > 10.0f))
        {
            g.setColour (col::amber.withAlpha (inScale ? 0.85f : 0.4f));
            g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), juce::jmin (10.0f, rowH), juce::Font::plain)));
            g.drawText (noteName (m), juce::Rectangle<float> (r.getX() + 4.0f, y - rowH * 0.5f, 34.0f, rowH), juce::Justification::centredRight);
        }
    }
    // compassos
    for (double b = std::ceil (viewStart / 4.0) * 4.0; b <= viewStart + viewBeats; b += 4.0)
    {
        const float x = xOf (b);
        g.setColour (col::amber.withAlpha (0.22f));
        g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
        g.setColour (col::amber.withAlpha (0.5f));
        g.setFont (juce::Font (juce::FontOptions (9.0f)));
        g.drawText (juce::String ((int) (b / 4.0) + 1), juce::Rectangle<float> (x + 2.0f, a.getY(), 30.0f, 10.0f), juce::Justification::left);
    }

    g.saveState();
    g.reduceClipRegion (a.toNearestInt());
    const auto& notes = proc.getNotes().getNotes();
    for (auto& n : notes)
    {
        if (n.end < viewStart || n.start > viewStart + viewBeats) continue;
        const float x0 = xOf (n.start), x1 = xOf (n.end);
        const float y = yOf ((float) n.shown());
        auto box = juce::Rectangle<float> (x0, y - rowH * 0.42f, juce::jmax (3.0f, x1 - x0 - 1.0f), rowH * 0.84f);
        const bool doubt = n.doubt && n.edit < 0;
        if (doubt)
        {
            const float pulse = 0.65f + 0.35f * std::sin (blink * 6.0f);
            g.setColour (juce::Colour (0xffff5a3c).withAlpha (0.35f * pulse));
            g.fillRoundedRectangle (box.expanded (2.0f), 3.0f);
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (box.toNearestInt());
            g.setColour (juce::Colour (0xffd8442c));
            g.fillRect (box);
            g.setColour (juce::Colour (0xffff7a5c).withAlpha (pulse));
            for (float sx = box.getX() - box.getHeight(); sx < box.getRight(); sx += 10.0f)
                g.drawLine (sx, box.getBottom(), sx + box.getHeight(), box.getY(), 4.0f);
        }
        else
        {
            const juce::Colour c = n.edit >= 0 ? juce::Colour (0xff3be070) : col::amber;
            g.setColour (c.withAlpha (0.3f));
            g.fillRoundedRectangle (box.expanded (2.0f), 3.0f);
            juce::ColourGradient ng (c.brighter (0.4f), box.getX(), box.getY(), c.darker (0.3f), box.getX(), box.getBottom(), false);
            g.setGradientFill (ng);
            g.fillRoundedRectangle (box, 2.0f);
        }
        if (n.id == selected)
        {
            g.setColour (col::cream);
            g.drawRoundedRectangle (box.expanded (3.0f), 3.0f, 2.0f);
        }
        // o que foi cantado (linha fina)
        if (n.edit < 0 && std::abs (n.sung - (float) n.shown()) > 0.08f)
        {
            g.setColour (col::cream.withAlpha (0.45f));
            const float ys = yOf (n.sung);
            g.drawLine (x0, ys, x1, ys, 1.0f);
        }
    }
    if (proc.isHostPlaying())
    {
        const float x = xOf (proc.getPlayPpq());
        g.setColour (col::cream.withAlpha (0.9f));
        g.drawLine (x, a.getY(), x, a.getBottom(), 1.5f);
    }
    g.restoreState();

    if (notes.empty())
    {
        g.setColour (col::amber.withAlpha (0.75f));
        g.setFont (stencil (12.0f, 0.12f));
        g.drawFittedText (u8 ("D\xc3\x83\x92 PLAY NA DAW: AS NOTAS CANTADAS APARECEM AQUI"), a.toNearestInt(), juce::Justification::centred, 2);
    }

    // escolha da nota certa
    if (selected >= 0 && ! popup.isEmpty())
    {
        g.setColour (juce::Colour (0xf0100c08));
        g.fillRoundedRectangle (popup, 5.0f);
        g.setColour (col::amber);
        g.drawRoundedRectangle (popup, 5.0f, 1.4f);
        g.setFont (stencil (10.5f, 0.12f));
        g.drawText (u8 ("QUAL \xc3\x89 A NOTA CERTA?"), popup.withHeight (22.0f).reduced (8.0f, 0), juce::Justification::centredLeft);
        for (auto& c : choices)
        {
            const SungNote* cur = nullptr;
            for (auto& n : notes) if (n.id == selected) cur = &n;
            const bool on = cur != nullptr && ((c.midi < 0 && cur->edit < 0) || (c.midi >= 0 && cur->shown() == c.midi && (cur->edit >= 0 || ! cur->doubt)));
            g.setColour (juce::Colour (on ? 0xff2c2925 : 0xff3a3632));
            g.fillRoundedRectangle (c.r, 3.0f);
            lamp (g, juce::Rectangle<float> (8, 8).withCentre ({ c.r.getX() + 11.0f, c.r.getCentreY() }), on);
            g.setColour (on ? col::amber : col::cream.withAlpha (0.8f));
            g.setFont (stencil (11.5f, 0.1f));
            g.drawText (c.midi < 0 ? juce::String ("AUTO") : noteName (c.midi), c.r.withTrimmedLeft (20.0f), juce::Justification::centredLeft);
        }
    }
}

void NoteEditor::mouseDown (const juce::MouseEvent& e)
{
    dragStartView = viewStart;
    // clique numa opção do popup
    if (selected >= 0)
        for (auto& c : choices)
            if (c.r.contains (e.position))
            {
                proc.getNotes().setEdit (selected, c.midi);
                selected = -1; choices.clear(); popup = {};
                proc.notesChanged = true;
                repaint();
                return;
            }
    selected = -1; choices.clear(); popup = {};
    const auto a = rollArea();
    const float rowH = a.getHeight() / (highNote - lowNote + 1.0f);
    for (auto& n : proc.getNotes().getNotes())
    {
        const float x0 = xOf (n.start), x1 = xOf (n.end), y = yOf ((float) n.shown());
        if (juce::Rectangle<float> (x0, y - rowH * 0.6f, juce::jmax (6.0f, x1 - x0), rowH * 1.2f).contains (e.position))
        {
            selected = n.id;
            // opções: as notas da escala em volta do que foi cantado, e AUTO
            const auto st = proc.getStatus();
            const uint16_t mask = mt::scaleMask (st.keyRoot, st.scale);
            std::vector<int> opts;
            int below = -1, above = -1;
            for (int k = (int) std::floor (n.sung); k > n.sung - 13; --k) if ((mask >> (((k % 12) + 12) % 12)) & 1) { below = k; break; }
            for (int k = (int) std::floor (n.sung) + 1; k < n.sung + 13; ++k) if ((mask >> (((k % 12) + 12) % 12)) & 1) { above = k; break; }
            int below2 = -1, above2 = -1;
            if (below >= 0) for (int k = below - 1; k > below - 13; --k) if ((mask >> (((k % 12) + 12) % 12)) & 1) { below2 = k; break; }
            if (above >= 0) for (int k = above + 1; k < above + 13; ++k) if ((mask >> (((k % 12) + 12) % 12)) & 1) { above2 = k; break; }
            for (int m : { above2, above, below, below2 }) if (m >= 0) opts.push_back (m);
            opts.push_back (-1);
            const float pw = 120.0f, ph = 26.0f + 24.0f * (float) opts.size();
            float px = juce::jmin (x1 + 8.0f, a.getRight() - pw - 4.0f);
            if (px < x0 && x0 - pw - 8.0f > a.getX()) px = x0 - pw - 8.0f;
            const float py = juce::jlimit (a.getY() + 2.0f, a.getBottom() - ph - 2.0f, y - ph * 0.5f);
            popup = { px, py, pw, ph };
            for (size_t i = 0; i < opts.size(); ++i)
                choices.push_back ({ juce::Rectangle<float> (px + 6.0f, py + 24.0f + 24.0f * (float) i, pw - 12.0f, 21.0f), opts[i] });
            break;
        }
    }
    repaint();
}

void NoteEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (selected >= 0) return;
    const auto a = rollArea();
    viewStart = juce::jmax (0.0, dragStartView - (double) e.getDistanceFromDragStartX() / a.getWidth() * viewBeats);
    follow = false;
    repaint();
}

void NoteEditor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
        viewBeats = juce::jlimit (4.0, 64.0, viewBeats * (w.deltaY > 0 ? 0.8 : 1.25));
    else
    {
        viewStart = juce::jmax (0.0, viewStart - (double) (w.deltaY + w.deltaX) * viewBeats * 0.25);
        follow = viewStart <= 0.0 ? follow : false;
    }
    if (proc.isHostPlaying() && ! follow && w.deltaY == 0 && w.deltaX == 0) follow = true;
    refresh (true);
}

//==============================================================================
void MelodyDrag::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour (0xff1a1410));
    g.fillRoundedRectangle (r, 4.0f);
    juce::Path border;
    border.addRoundedRectangle (r.reduced (3.0f), 3.0f);
    juce::Path dashed;
    const float dash[] = { 7.0f, 5.0f };
    juce::PathStrokeType (2.0f).createDashedStroke (dashed, border, dash, 2);
    g.setColour (col::amber.withAlpha (isMouseOver() ? 1.0f : 0.8f));
    g.fillPath (dashed);
    juce::Path arrow;
    const float cx = r.getCentreX(), cy = r.getCentreY() - 26.0f;
    arrow.addArrow ({ cx, cy - 12.0f, cx, cy + 12.0f }, 2.4f, 10.0f, 7.0f);
    g.fillPath (arrow);
    g.setFont (stencil (12.5f, 0.12f));
    g.drawFittedText ("ARRASTE A\nMELODIA", r.withTrimmedTop (r.getHeight() * 0.5f - 6.0f).withHeight (34.0f).toNearestInt(), juce::Justification::centred, 2);
    g.setColour (col::amber.withAlpha (0.65f));
    g.setFont (stencil (8.0f, 0.12f));
    g.drawFittedText ("MIDI PARA A DAW OU O MRM", r.withTrimmedTop (r.getHeight() * 0.5f + 32.0f).withHeight (14.0f).toNearestInt(), juce::Justification::centred, 1);
}

void MelodyDrag::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true;
    const auto mf = proc.getNotes().toMidi (proc.getHostBpm());
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Mastor Tune - melodia.mid");
    f.deleteFile();
    juce::FileOutputStream os (f);
    if (os.openedOk())
    {
        mf.writeTo (os);
        os.flush();
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
}

//==============================================================================
EqGraph::EqGraph (MastorTuneProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::CrosshairCursor); }

mt::EqSettings EqGraph::settings() const
{
    mt::EqSettings s;
    s.hp = proc.getP ("eqHp"); s.lp = proc.getP ("eqLp");
    s.f[0] = proc.getP ("eq1f"); s.g[0] = proc.getP ("eq1g");
    s.f[1] = proc.getP ("eq2f"); s.g[1] = proc.getP ("eq2g");
    s.f[2] = proc.getP ("eq3f"); s.g[2] = proc.getP ("eq3g");
    return s;
}
float EqGraph::xOfHz (float hz) const { return 4.0f + std::log10 (hz / 20.0f) / 3.0f * ((float) getWidth() - 8.0f); }
float EqGraph::hzOfX (float x) const { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - 4.0f) / ((float) getWidth() - 8.0f))); }
float EqGraph::yOfDb (float db) const { return (float) getHeight() * 0.5f - db / 15.0f * ((float) getHeight() * 0.5f - 8.0f); }

juce::Point<float> EqGraph::nodePos (int i) const
{
    const auto s = settings();
    if (i == 0) return { xOfHz (s.hp), yOfDb (0) };
    if (i == 4) return { xOfHz (s.lp), yOfDb (0) };
    return { xOfHz (s.f[i - 1]), yOfDb (s.g[i - 1]) };
}

void EqGraph::setParam (const char* id, float v)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void EqGraph::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    darkGlass (g, r, 4.0f);
    const auto s = settings();
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        g.setColour (col::amber.withAlpha (0.13f));
        g.drawVerticalLine ((int) xOfHz (f), r.getY() + 2, r.getBottom() - 2);
    }
    g.setFont (juce::Font (juce::FontOptions (9.5f)));
    for (auto [f, t] : { std::pair<float, const char*> { 100.0f, "100" }, { 1000.0f, "1k" }, { 10000.0f, "10k" } })
    {
        g.setColour (col::cream.withAlpha (0.55f));
        g.drawText (t, juce::Rectangle<float> (xOfHz (f) + 3.0f, r.getBottom() - 14.0f, 30.0f, 12.0f), juce::Justification::left);
    }
    for (int db : { -9, 0, 9 })
    {
        g.setColour (col::amber.withAlpha (db == 0 ? 0.3f : 0.1f));
        g.drawHorizontalLine ((int) yOfDb ((float) db), r.getX() + 2, r.getRight() - 2);
        g.setColour (col::cream.withAlpha (0.5f));
        g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (6.0f, yOfDb ((float) db) - 12.0f, 30.0f, 11.0f), juce::Justification::left);
    }
    juce::Path curve, fill;
    for (int x = 2; x < getWidth() - 2; x += 2)
    {
        const float y = juce::jlimit (2.0f, (float) getHeight() - 2.0f, yOfDb (mt::VoiceEQ::responseDb (s, hzOfX ((float) x))));
        if (x == 2) { curve.startNewSubPath ((float) x, y); fill.startNewSubPath ((float) x, r.getBottom()); }
        curve.lineTo ((float) x, y); fill.lineTo ((float) x, y);
    }
    fill.lineTo ((float) getWidth() - 2.0f, r.getBottom()); fill.closeSubPath();
    const bool on = proc.getP ("eqOn") > 0.5f;
    g.setColour (col::amber.withAlpha (on ? 0.16f : 0.07f));
    g.fillPath (fill);
    g.setColour (col::amber.withAlpha (on ? 1.0f : 0.45f));
    g.strokePath (curve, juce::PathStrokeType (2.6f));

    const char* labs[] = { "HP", "1", "2", "3", "LP" };
    for (int i = 0; i < 5; ++i)
    {
        const auto p = nodePos (i);
        const bool filt = i == 0 || i == 4;
        const juce::Colour c = filt ? juce::Colour (0xff3be070) : col::amber;
        const float rad = i == active ? 13.0f : 11.0f;
        g.setColour (c.withAlpha (0.35f));
        g.fillEllipse (juce::Rectangle<float> (rad * 2.6f, rad * 2.6f).withCentre (p));
        juce::ColourGradient ng (c.brighter (0.6f), p.x - rad, p.y - rad, c.darker (0.4f), p.x + rad, p.y + rad, false);
        g.setGradientFill (ng);
        g.fillEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (p));
        g.setColour (col::cream);
        g.drawEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (p), 1.6f);
        g.setColour (col::labelBg);
        g.setFont (stencil (9.5f, 0.0f));
        g.drawText (labs[i], juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (p), juce::Justification::centred);
    }
}

juce::String EqGraph::readout() const
{
    const auto s = settings();
    auto fmt = [] (float f) { return f >= 1000.0f ? juce::String (f / 1000.0f, f >= 10000.0f ? 0 : 1) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; };
    if (active == 0) return s.hp > 21.0f ? "HP  " + fmt (s.hp) + u8 ("  \xc2\xb7  24 dB/OIT") : "HP  DESLIGADO (ARRASTE PARA A DIREITA)";
    if (active == 4) return s.lp < 19500.0f ? "LP  " + fmt (s.lp) + u8 ("  \xc2\xb7  24 dB/OIT") : "LP  DESLIGADO (ARRASTE PARA A ESQUERDA)";
    const int i = active - 1;
    return "PONTO " + juce::String (active) + "  " + fmt (s.f[i]) + u8 ("  \xc2\xb7  ") + (s.g[i] >= 0 ? "+" : "") + juce::String (s.g[i], 1) + " dB";
}

void EqGraph::mouseDown (const juce::MouseEvent& e)
{
    drag = -1;
    float best = 22.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float d = nodePos (i).getDistanceFrom (e.position);
        if (d < best) { best = d; drag = i; }
    }
    if (drag >= 0)
    {
        active = drag;
        static const char* ids[5][2] = { { "eqHp", nullptr }, { "eq1f", "eq1g" }, { "eq2f", "eq2g" }, { "eq3f", "eq3g" }, { "eqLp", nullptr } };
        for (auto* id : ids[drag]) if (id) if (auto* p = proc.apvts.getParameter (id)) p->beginChangeGesture();
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }
    repaint();
}

void EqGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    const float hz = hzOfX (e.position.x);
    const auto s = settings();
    if (drag == 0) setParam ("eqHp", juce::jlimit (20.0f, juce::jmin (2000.0f, s.lp / 1.5f), hz));
    else if (drag == 4) setParam ("eqLp", juce::jlimit (juce::jmax (1000.0f, s.hp * 1.5f), 20000.0f, hz));
    else
    {
        const juce::String n (drag);
        const float db = juce::jlimit (-12.0f, 12.0f, std::round ((((float) getHeight() * 0.5f - e.position.y) / ((float) getHeight() * 0.5f - 8.0f) * 15.0f) * 2.0f) / 2.0f);
        setParam (("eq" + n + "f").toRawUTF8(), juce::jlimit (40.0f, 16000.0f, hz));
        setParam (("eq" + n + "g").toRawUTF8(), db);
    }
    repaint();
}

void EqGraph::mouseUp (const juce::MouseEvent&)
{
    if (drag >= 0)
    {
        static const char* ids[5][2] = { { "eqHp", nullptr }, { "eq1f", "eq1g" }, { "eq2f", "eq2g" }, { "eq3f", "eq3g" }, { "eqLp", nullptr } };
        for (auto* id : ids[drag]) if (id) if (auto* p = proc.apvts.getParameter (id)) p->endChangeGesture();
    }
    drag = -1;
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

void EqGraph::mouseDoubleClick (const juce::MouseEvent&)
{
    // duplo clique: o ponto volta ao padrão
    if (active == 0) setParam ("eqHp", 20.0f);
    else if (active == 4) setParam ("eqLp", 20000.0f);
    else setParam (("eq" + juce::String (active) + "g").toRawUTF8(), 0.0f);
    repaint();
}

void EqGraph::applyPreset (int i)
{
    // FLAT, HPF, LPF, HPF+LPF (os mesmos do Graphic EQ do Tape Echo)
    const float hp = (i == 1 || i == 3) ? 120.0f : 20.0f, lp = (i == 2 || i == 3) ? 6000.0f : 20000.0f;
    const float f[3] = { 250.0f, 1000.0f, 4000.0f };
    for (auto* id : { "eqHp", "eqLp", "eq1f", "eq1g", "eq2f", "eq2g", "eq3f", "eq3g" })
        if (auto* p = proc.apvts.getParameter (id)) p->beginChangeGesture();
    setParam ("eqHp", hp); setParam ("eqLp", lp);
    for (int k = 0; k < 3; ++k) { const juce::String n (k + 1); setParam (("eq" + n + "f").toRawUTF8(), f[k]); setParam (("eq" + n + "g").toRawUTF8(), 0.0f); }
    for (auto* id : { "eqHp", "eqLp", "eq1f", "eq1g", "eq2f", "eq2g", "eq3f", "eq3g" })
        if (auto* p = proc.apvts.getParameter (id)) p->endChangeGesture();
    repaint();
}

//==============================================================================
static void tapeReel (juce::Graphics& g, juce::Point<float> c, float rad, float ang, bool dark)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (c).translated (1.5f, 3.0f));
    juce::ColourGradient rg (juce::Colour (0xfff4f2ec), c.x - rad, c.y - rad, juce::Colour (0xff8d8a84), c.x + rad, c.y + rad, false);
    g.setGradientFill (rg);
    g.fillEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (c));
    g.setColour (dark ? juce::Colour (0xff2a2622) : juce::Colour (0xff6b4423));
    g.drawEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (c), rad * 0.12f);
    for (int k = 0; k < 3; ++k)
    {
        const float a = ang + (float) k * juce::MathConstants<float>::twoPi / 3.0f;
        g.setColour (juce::Colour (0xff2a2622));
        g.fillEllipse (juce::Rectangle<float> (rad * 0.36f, rad * 0.36f).withCentre ({ c.x + std::sin (a) * rad * 0.48f, c.y - std::cos (a) * rad * 0.48f }));
    }
    g.setColour (juce::Colour (0xff555555));
    g.fillEllipse (juce::Rectangle<float> (rad * 0.22f, rad * 0.22f).withCentre (c));
}

void SatView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    darkGlass (g, r, 4.0f);
    const bool on = proc.getP ("satOn") > 0.5f;
    const int mode = (int) proc.getP ("satMode");
    const auto c = r.getCentre();
    if (mode == 0)
    {
        const float rad = juce::jmin (r.getHeight() * 0.34f, r.getWidth() * 0.18f);
        g.setColour (juce::Colour (0xff3a2614));
        g.drawLine (c.x - rad * 1.4f, c.y + rad * 0.95f, c.x + rad * 1.4f, c.y + rad * 0.95f, 3.0f);
        tapeReel (g, { c.x - rad * 1.4f, c.y }, rad, on ? phase * 2.0f : 0.3f, false);
        tapeReel (g, { c.x + rad * 1.4f, c.y }, rad, on ? phase * 2.0f + 1.0f : 1.3f, false);
    }
    else if (mode == 1)
    {
        const float glow = on ? 0.55f + 0.45f * std::sin (phase * 2.6f) * 0.5f + 0.2f : 0.0f;
        auto tube = juce::Rectangle<float> (r.getHeight() * 0.4f, r.getHeight() * 0.66f).withCentre ({ c.x, c.y - 6.0f });
        if (on)
        {
            g.setColour (juce::Colour (0xffffb300).withAlpha (0.25f * glow));
            g.fillEllipse (tube.expanded (16.0f));
        }
        juce::ColourGradient tg (juce::Colour (on ? 0xffc25a00 : 0xff3a2a1a).withMultipliedBrightness (on ? 0.7f + 0.5f * glow : 1.0f),
                                 tube.getCentreX(), tube.getCentreY(), juce::Colour (0xff2a1404), tube.getRight(), tube.getY(), true);
        g.setGradientFill (tg);
        g.fillRoundedRectangle (tube, tube.getWidth() * 0.45f);
        g.setColour (col::cream.withAlpha (0.55f));
        g.drawRoundedRectangle (tube, tube.getWidth() * 0.45f, 1.6f);
        g.setColour (juce::Colour (0xff8d8a84));
        g.fillRoundedRectangle (tube.withY (tube.getBottom() - 2.0f).withHeight (14.0f).expanded (6.0f, 0), 2.0f);
    }
    else
    {
        const float rad = r.getHeight() * 0.42f * (on ? 1.0f + 0.025f * std::sin (phase * 40.0f) : 1.0f);
        auto sp = juce::Rectangle<float> (rad * 2, rad * 2).withCentre (c);
        g.setColour (juce::Colour (0xff6b4423)); g.fillEllipse (sp.expanded (4.0f));
        juce::ColourGradient cone (juce::Colour (0xff3a3226), c.x, c.y, juce::Colour (0xff111111), c.x + rad, c.y, true);
        g.setGradientFill (cone); g.fillEllipse (sp);
        g.setColour (juce::Colour (0xff4a4030)); g.drawEllipse (sp.reduced (rad * 0.35f), 2.0f);
        g.setColour (juce::Colour (0xff111111)); g.fillEllipse (sp.reduced (rad * 0.8f));
    }
    if (! on)
    {
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r, 4.0f);
    }
}

void EchoView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    darkGlass (g, r, 4.0f);
    const bool on = proc.getP ("echoOn") > 0.5f;
    const float rad = r.getHeight() * 0.32f;
    const auto l = juce::Point<float> (r.getX() + rad + 26.0f, r.getCentreY()), rr = juce::Point<float> (r.getRight() - rad - 26.0f, r.getCentreY());
    juce::Path loop;
    loop.addRoundedRectangle (juce::Rectangle<float> (l.x, l.y - rad * 0.75f, rr.x - l.x, rad * 1.5f), rad * 0.75f);
    g.setColour (juce::Colour (0xff6b4423));
    g.strokePath (loop, juce::PathStrokeType (5.0f));
    for (int k = 0; k < 3; ++k)
    {
        const auto hp = juce::Rectangle<float> (16, 16).withCentre ({ l.x + (rr.x - l.x) * (0.35f + 0.15f * (float) k), r.getCentreY() + rad * 0.75f + 2.0f });
        juce::ColourGradient hg (juce::Colour (0xffe9e6df), hp.getX(), hp.getY(), juce::Colour (0xff6f6c66), hp.getRight(), hp.getBottom(), false);
        g.setGradientFill (hg);
        g.fillRoundedRectangle (hp, 2.0f);
    }
    tapeReel (g, l, rad, on ? phase * 3.0f : 0.2f, true);
    tapeReel (g, rr, rad, on ? phase * 3.0f + 0.7f : 1.1f, true);
    g.setColour (col::amber.withAlpha (on ? 0.9f : 0.4f));
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain)));
    const bool sync = proc.getP ("echoSync") > 0.5f;
    const juce::String t = sync ? MastorTuneProcessor::divisionNames()[(int) proc.getP ("echoDiv")]
                                : juce::String (juce::roundToInt (proc.getP ("echoMs"))) + " ms";
    g.drawText ((on ? "MTE  " : "OFF  ") + t, r.reduced (8.0f, 4.0f), juce::Justification::topLeft);
    if (! on)
    {
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (r, 4.0f);
    }
}

void MaxMeters::update()
{
    const auto st = proc.getStatus();
    for (int c = 0; c < 2; ++c)
    {
        const float db = juce::Decibels::gainToDecibels (st.outPeak[c], -60.0f);
        out[c] = db > out[c] ? db : out[c] - 1.2f;
    }
    inLvl = st.inDb > inLvl ? st.inDb : inLvl - 1.2f;
    gr = st.grDb < gr ? st.grDb : gr + 0.6f;
    if (gr > 0) gr = 0;
    repaint();
}

void MaxMeters::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    darkGlass (g, r, 4.0f);
    auto area = r.reduced (8.0f).withTrimmedBottom (14.0f);
    const auto scaleCol = area.removeFromLeft (24.0f);
    auto ydb = [&] (float db) { return area.getY() + (-juce::jlimit (-30.0f, 0.0f, db) / 30.0f) * area.getHeight(); };
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    for (int db : { 0, -3, -6, -10, -15, -20, -30 })
    {
        g.setColour (col::cream.withAlpha (0.45f));
        g.drawText (juce::String (db), juce::Rectangle<float> (scaleCol.getX(), ydb ((float) db) - 6.0f, 22.0f, 12.0f), juce::Justification::centredRight);
        g.setColour (col::cream.withAlpha (0.07f));
        g.drawHorizontalLine ((int) ydb ((float) db), area.getX(), area.getRight());
    }
    const float vals[5] = { inLvl, inLvl, gr, out[0], out[1] };
    const char* labs[5] = { "IN", "", "GR", "OUT", "" };
    const float bw = area.getWidth() / 5.0f;
    for (int i = 0; i < 5; ++i)
    {
        auto bar = juce::Rectangle<float> (area.getX() + bw * (float) i + 4.0f, area.getY(), bw - 8.0f, area.getHeight());
        g.setColour (juce::Colour (0xff1b2418));
        g.fillRect (bar);
        if (i == 2)
        {
            const float h = juce::jlimit (0.0f, 1.0f, -gr / 30.0f) * bar.getHeight();
            juce::ColourGradient gg (juce::Colour (0xffff5a3c), bar.getX(), bar.getY(), col::amber, bar.getX(), bar.getY() + bar.getHeight() * 0.5f, false);
            g.setGradientFill (gg);
            g.fillRect (bar.withHeight (h));
        }
        else
        {
            const float top = ydb (vals[i]);
            juce::ColourGradient mg (juce::Colour (0xffff5a3c), bar.getX(), area.getY(), juce::Colour (0xff3be070), bar.getX(), area.getBottom(), false);
            mg.addColour (0.25, col::jamGold);
            g.setGradientFill (mg);
            if (vals[i] > -30.0f) g.fillRect (bar.withTop (top));
        }
        g.setColour (juce::Colour (0xff0e0c0a));
        for (float y = bar.getY(); y < bar.getBottom(); y += 4.0f) g.fillRect (bar.getX(), y, bar.getWidth(), 1.5f);
        g.setColour (col::cream.withAlpha (0.6f));
        g.setFont (stencil (8.5f, 0.1f));
        if (labs[i][0]) g.drawText (labs[i], juce::Rectangle<float> (bar.getX() - 6.0f, r.getBottom() - 16.0f, bar.getWidth() * (i == 2 ? 1.0f : 2.0f) + 12.0f, 12.0f), juce::Justification::centred);
    }
}

BigKnobView::BigKnobView (juce::RangedAudioParameter& p)
    : param (p), att (p, [this] (float v) { value = v; repaint(); }, nullptr)
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}
void BigKnobView::mouseDown (const juce::MouseEvent&) { startNorm = value; att.beginGesture(); }
void BigKnobView::mouseUp (const juce::MouseEvent&) { att.endGesture(); }
void BigKnobView::mouseDrag (const juce::MouseEvent& e) { att.setValueAsPartOfGesture (juce::jlimit (0.0f, 1.0f, startNorm - (float) e.getDistanceFromDragStartY() / 220.0f)); }

void BigKnobView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float size = juce::jmin (r.getWidth(), r.getHeight());
    const auto ctr = r.getCentre();
    const float rOut = size * 0.5f - 4.0f;
    const int steps = 12;
    const int sel = juce::roundToInt (value * (float) (steps - 1));
    const float ang = kA0 + (kA1 - kA0) * (float) sel / (float) (steps - 1);
    static const char* labs[12] = { "OFF", "70", "100", "150", "250", "500", "1k", "2k", "3k", "5k", "7.5k", "10k" };
    for (int i = 0; i < steps; ++i)
    {
        const float a = kA0 + (kA1 - kA0) * (float) i / (float) (steps - 1);
        g.setColour (i == sel ? col::amber.darker (0.3f) : col::ink.withAlpha (0.75f));
        g.drawLine (ctr.x + std::sin (a) * (rOut - 18.0f), ctr.y - std::cos (a) * (rOut - 18.0f), ctr.x + std::sin (a) * (rOut - 24.0f), ctr.y - std::cos (a) * (rOut - 24.0f), 1.6f);
        g.setFont (stencil (8.5f, 0.0f));
        g.drawText (labs[i], juce::Rectangle<float> (30, 12).withCentre ({ ctr.x + std::sin (a) * (rOut - 6.0f), ctr.y - std::cos (a) * (rOut - 6.0f) }), juce::Justification::centred);
    }
    const float kr = rOut - 30.0f;
    auto ringR = juce::Rectangle<float> (kr * 2, kr * 2).withCentre (ctr);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (ringR.translated (2.0f, 5.0f));
    juce::ColourGradient ring (juce::Colour (0xfff4f2ec), ringR.getX(), ringR.getY(), juce::Colour (0xff5e5a52), ringR.getRight(), ringR.getBottom(), false);
    ring.addColour (0.5, juce::Colour (0xffb9b5ac));
    g.setGradientFill (ring); g.fillEllipse (ringR);
    auto sk = ringR.reduced (kr * 0.14f);
    juce::ColourGradient skg (juce::Colour (0xff3a3632), sk.getX(), sk.getY(), juce::Colour (0xff0b0a09), sk.getX(), sk.getBottom(), false);
    g.setGradientFill (skg); g.fillEllipse (sk);
    auto dot = sk.reduced (kr * 0.42f);
    juce::ColourGradient gold (juce::Colour (0xfffff0a0), dot.getX(), dot.getY(), juce::Colour (0xff9c7a05), dot.getRight(), dot.getBottom(), false);
    gold.addColour (0.5, col::jamGold);
    g.setGradientFill (gold); g.fillEllipse (dot);
    g.setColour (col::cream);
    g.drawLine (ctr.x + std::sin (ang) * kr * 0.48f, ctr.y - std::cos (ang) * kr * 0.48f, ctr.x + std::sin (ang) * kr * 0.84f, ctr.y - std::cos (ang) * kr * 0.84f, 4.0f);
}

//==============================================================================
Panel::Panel (MastorTuneProcessor& p) : proc (p)
{
    setSize (kW, kH);
    auto& ap = proc.apvts;

    // --- áreas
    secTabs  = { 50, 104, 1100, 56 };
    secHarm  = { 50, 186, 1100, 112 };
    secKey   = { 50, 318, 330, 300 };
    secKnob  = { 396, 318, 434, 300 };
    secMeter = { 846, 318, 304, 300 };
    secNotes = { 50, 642, 1100, 268 };
    secEq    = { 50, 186, 560, 330 };
    secSat   = { 626, 186, 254, 330 };
    secBk    = { 50, 540, 380, 370 };
    secEcho  = { 446, 540, 434, 370 };
    secMax   = { 896, 186, 254, 724 };

    // --- abas
    tabTune = &addKey (u8 ("AFINA\xc3\x87\xc3\x83O"), [this] { showTab (0); }, [this] { return tab == 0; }, u8 ("TOM  \xc2\xb7  CLEAN / ROB\xc3\x94  \xc2\xb7  NOTAS"));
    tabTune->setBounds (64, 112, 520, 42); tabTune->fontSize = 14.5f;
    tabProc = &addKey ("PROCESSAMENTO", [this] { showTab (1); }, [this] { return tab == 1; }, u8 ("EQ  \xc2\xb7  SATURADOR  \xc2\xb7  BIG KNOB  \xc2\xb7  TAPE ECHO  \xc2\xb7  MAXIMIZER"));
    tabProc->setBounds (616, 112, 520, 42); tabProc->fontSize = 14.5f;

    // --- HARMONIA
    mrmGlass    = { 64.0f, 204.0f, 520.0f, 40.0f };
    riddimGlass = { 596.0f, 204.0f, 250.0f, 40.0f };
    infoBox     = { 64.0f, 254.0f, 1072.0f, 34.0f };
    listenKey = &addKey ("ESCUTAR RIDDIM", [this] { setParam ("listen", param ("listen") > 0.5f ? 0.0f : 1.0f); }, [this] { return param ("listen") > 0.5f; });
    listenKey->green = true; listenKey->pulse = true; listenKey->fontSize = 15.0f;
    listenKey->setBounds (858, 200, 278, 48);
    tunePage.push_back (listenKey);

    // --- TOM / ESCALA
    keyGlass = { 64.0f, 334.0f, 226.0f, 30.0f };
    autoKey = &addKey ("AUTO", [this] { proc.setAuto (param ("autoKey") < 0.5f); }, [this] { return param ("autoKey") > 0.5f; });
    autoKey->setBounds (296, 330, 72, 36); autoKey->fontSize = 11.5f;
    tunePage.push_back (autoKey);
    piano = new KeyPiano (proc); owned.emplace_back (piano);
    piano->setBounds (64, 382, 304, 80); addAndMakeVisible (piano); tunePage.push_back (piano);
    for (int i = 0; i < 2; ++i)
    {
        auto& k = addKey (i == 0 ? "MAIOR" : "MENOR", [this, i] { proc.chooseScale (i); }, [this, i] { return st.scale == i; });
        k.setBounds (64 + i * 154, 468, 150, 38); k.fontSize = 13.5f;
        tunePage.push_back (&k);
    }
    for (int i = 2; i < mt::NumScales; ++i)
    {
        const int j = i - 2;
        static const char* upper[] = { "", "", "D\xc3\x93RICO", "MIXOL\xc3\x8d" "DIO", "FR\xc3\x8dGIO", "L\xc3\x8d" "DIO", "L\xc3\x93" "CRIO",
                                       "HARM. MENOR", "PENTA MAIOR", "PENTA MENOR", "BLUES", "CROM\xc3\x81TICA" };
        auto& k = addKey (juce::String::fromUTF8 (upper[i]), [this, i] { proc.chooseScale (i); }, [this, i] { return st.scale == i; });
        k.setBounds (64 + (j % 4) * 76, 528 + (j / 4) * 28, 76, 27); k.fontSize = 9.0f;
        tunePage.push_back (&k);
    }

    // --- KNOB CLEAN / ROBÔ
    tuneKnob = new TuneKnob (*ap.getParameter ("amount")); owned.emplace_back (tuneKnob);
    tuneKnob->setBounds (473, 330, 280, 240); addAndMakeVisible (tuneKnob); tunePage.push_back (tuneKnob);
    knobGlass = { 503.0f, 572.0f, 220.0f, 36.0f };

    // --- MEDIDOR / VOZ
    meter = new CentsMeter(); owned.emplace_back (meter);
    meter->setBounds (862, 334, 272, 100); addAndMakeVisible (meter); tunePage.push_back (meter);
    for (int i = 0; i < 3; ++i)
    {
        auto& k = addChoice ("voice", i, i == 0 ? "GRAVE" : i == 1 ? u8 ("M\xc3\x89" "DIA") : "AGUDA");
        k.setBounds (862 + i * 91, 506, 88, 32); k.fontSize = 10.5f;
        tunePage.push_back (&k);
    }
    for (auto [id, lab, x] : { std::tuple<const char*, const char*, int> { "mix", "MIX", 880 }, { "out", "SA\xc3\x8d" "DA", 1040 } })
    {
        auto* kn = new Knob (*ap.getParameter (id), juce::String::fromUTF8 (lab)); owned.emplace_back (kn);
        kn->setBounds (x, 542, 80, 72); addAndMakeVisible (kn); tunePage.push_back (kn);
    }

    // --- EDITOR DE NOTAS
    doubtGlass = { 64.0f, 660.0f, 280.0f, 30.0f };
    noteEditor = new NoteEditor (proc); owned.emplace_back (noteEditor);
    noteEditor->setBounds (64, 700, 900, 198); addAndMakeVisible (noteEditor); tunePage.push_back (noteEditor);
    melody = new MelodyDrag (proc); owned.emplace_back (melody);
    melody->setBounds (976, 700, 160, 198); addAndMakeVisible (melody); tunePage.push_back (melody);
    reKey = &addKey ("REANALISAR", [this] { proc.reanalyse(); }, [] { return false; });
    reKey->setBounds (976, 656, 160, 36); reKey->fontSize = 11.0f;
    tunePage.push_back (reKey);

    // --- EQ
    eqGraph = new EqGraph (proc); owned.emplace_back (eqGraph);
    eqGraph->setBounds (64, 246, 532, 222); addAndMakeVisible (eqGraph); procPage.push_back (eqGraph);
    eqRead = { 64.0f, 476.0f, 532.0f, 28.0f };
    const char* presets[] = { "FLAT", "HPF", "LPF", "HPF+LPF" };
    for (int i = 0; i < 4; ++i)
    {
        auto& k = addKey (presets[i], [this, i] { eqGraph->applyPreset (i); }, [this, i]
        {
            const float hp = param ("eqHp"), lp = param ("eqLp");
            bool flatBells = true;
            for (auto* id : { "eq1g", "eq2g", "eq3g" }) flatBells = flatBells && std::abs (param (id)) < 0.01f;
            const bool h = hp > 21.0f && std::abs (hp - 120.0f) < 1.0f, l = lp < 19500.0f && std::abs (lp - 6000.0f) < 5.0f;
            const bool noH = hp <= 21.0f, noL = lp >= 19500.0f;
            if (! flatBells) return false;
            return (i == 0 && noH && noL) || (i == 1 && h && noL) || (i == 2 && noH && l) || (i == 3 && h && l);
        });
        k.setBounds (64 + i * 92, 202, 88, 34); k.fontSize = 11.0f;
        procPage.push_back (&k);
        eqPresetKeys.push_back (&k);
    }
    { auto& k = addToggle ("eqOn", "LIGADO", "DESLIGADO"); k.setBounds (440, 202, 156, 34); procPage.push_back (&k); }

    // --- SATURADOR
    satView = new SatView (proc); owned.emplace_back (satView);
    satView->setBounds (640, 204, 226, 120); addAndMakeVisible (satView); procPage.push_back (satView);
    for (int i = 0; i < 3; ++i)
    {
        auto& k = addChoice ("satMode", i, i == 0 ? "FITA" : i == 1 ? u8 ("V\xc3\x81LVULA") : "SOUND SYSTEM");
        k.setBounds (640, 332 + i * 32, 140, 30); k.fontSize = 10.5f;
        procPage.push_back (&k);
    }
    { auto* kn = new Knob (*ap.getParameter ("satDrive"), "DRIVE"); owned.emplace_back (kn); kn->setBounds (786, 330, 82, 96); addAndMakeVisible (kn); procPage.push_back (kn); }
    { auto& k = addToggle ("satOn", "LIGADO", "DESLIGADO"); k.setBounds (640, 466, 226, 36); procPage.push_back (&k); }

    // --- BIG KNOB
    { auto* bk = new BigKnobView (*ap.getParameter ("bkKnob")); owned.emplace_back (bk); bk->setBounds (62, 560, 230, 230); addAndMakeVisible (bk); procPage.push_back (bk); }
    for (int i = 0; i < 2; ++i)
    {
        auto& k = addChoice ("bkMode", i, i == 0 ? "HIGH PASS" : "LOW PASS");
        k.setBounds (300, 570 + i * 40, 118, 34); k.fontSize = 10.5f;
        procPage.push_back (&k);
    }
    { auto* kn = new Knob (*ap.getParameter ("bkRes"), "RESONANCE"); owned.emplace_back (kn); kn->setBounds (310, 660, 100, 100); addAndMakeVisible (kn); procPage.push_back (kn); }
    { auto& k = addToggle ("bkOn", "LIGADO", "DESLIGADO"); k.setBounds (64, 862, 352, 36); procPage.push_back (&k); }

    // --- TAPE ECHO
    echoView = new EchoView (proc); owned.emplace_back (echoView);
    echoView->setBounds (460, 560, 406, 116); addAndMakeVisible (echoView); procPage.push_back (echoView);
    {
        const char* ids[] = { "echoDiv", "echoMs", "echoFb", "echoMix" };
        const char* labs[] = { "TEMPO (SYNC)", "TEMPO (MS)", "FEEDBACK", "MIX" };
        for (int i = 0; i < 4; ++i)
        {
            auto* kn = new Knob (*ap.getParameter (ids[i]), labs[i]); owned.emplace_back (kn);
            kn->setBounds (460 + i * 102, 690, 100, 110); addAndMakeVisible (kn); procPage.push_back (kn);
        }
    }
    { auto& k = addToggle ("echoSync", "SYNC NO BPM", "TEMPO LIVRE"); k.setBounds (460, 818, 200, 36); procPage.push_back (&k); }
    { auto& k = addToggle ("echoOn", "LIGADO", "DESLIGADO"); k.setBounds (460, 862, 406, 36); procPage.push_back (&k); }

    // --- MAXIMIZER
    maxRead = { 910.0f, 204.0f, 226.0f, 92.0f };
    maxMeters = new MaxMeters (proc); owned.emplace_back (maxMeters);
    maxMeters->setBounds (910, 304, 226, 404); addAndMakeVisible (maxMeters); procPage.push_back (maxMeters);
    {
        auto* t = new Knob (*ap.getParameter ("maxThresh"), "THRESHOLD"); owned.emplace_back (t);
        t->setBounds (910, 714, 110, 96); addAndMakeVisible (t); procPage.push_back (t);
        auto* c = new Knob (*ap.getParameter ("maxCeil"), "CEILING"); owned.emplace_back (c);
        c->setBounds (1026, 714, 110, 96); addAndMakeVisible (c); procPage.push_back (c);
    }
    for (int i = 0; i < 3; ++i)
    {
        auto& k = addChoice ("maxMode", i, i == 0 ? "TRANSP." : i == 1 ? "PUNCH" : "VINTAGE");
        k.setBounds (910 + i * 76, 818, 74, 34); k.fontSize = 10.0f;
        procPage.push_back (&k);
    }
    { auto& k = addToggle ("maxOn", "LIGADO", "DESLIGADO"); k.setBounds (910, 862, 226, 36); procPage.push_back (&k); }

    // --- TECLADO + BYPASS DO AFINADOR (nas duas abas)
    secKeys = { 50, 930, 1100, 104 };
    bypassKey = &addKey ("BYPASS DO AFINADOR", [this] { setParam ("tuneBypass", param ("tuneBypass") > 0.5f ? 0.0f : 1.0f); },
                         [this] { return param ("tuneBypass") > 0.5f; }, u8 ("S\xc3\x93 A VOZ ORIGINAL"));
    bypassKey->setBounds (64, 948, 170, 74); bypassKey->fontSize = 12.0f;
    keyboard = new ScaleKeyboard(); owned.emplace_back (keyboard);
    keyboard->setBounds (246, 946, 890, 78); addAndMakeVisible (keyboard);

    showTab (0);
    startTimerHz (30);
}

Panel::~Panel() { stopTimer(); }

LampKey& Panel::addKey (const juce::String& text, std::function<void()> click, std::function<bool()> on, const juce::String& sub)
{
    keys.push_back (std::make_unique<LampKey> (text, sub));
    auto& k = *keys.back();
    k.onClick = std::move (click);
    k.isOn = std::move (on);
    addAndMakeVisible (k);
    return k;
}

void Panel::setParam (const char* id, float v)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

LampKey& Panel::addToggle (const char* id, const juce::String& onText, const juce::String& offText)
{
    auto& k = addKey (offText, [this, id] { setParam (id, param (id) > 0.5f ? 0.0f : 1.0f); }, [this, id] { return param (id) > 0.5f; });
    auto* kp = &k;
    k.fontSize = 11.5f;
    // o texto acompanha o estado (atualizado no timer)
    k.getProperties().set ("onText", onText);
    k.getProperties().set ("offText", offText);
    k.getProperties().set ("param", juce::String (id));
    juce::ignoreUnused (kp);
    return k;
}

LampKey& Panel::addChoice (const char* id, int index, const juce::String& text)
{
    return addKey (text, [this, id, index] { setParam (id, (float) index); }, [this, id, index] { return juce::roundToInt (param (id)) == index; });
}

void Panel::showTab (int t)
{
    tab = t;
    for (auto* c : tunePage) c->setVisible (t == 0);
    for (auto* c : procPage) c->setVisible (t == 1);
    background = {};
    repaint();
}

void Panel::resized() {}

void Panel::renderBackground()
{
    const float scale = 2.0f;
    background = juce::Image (juce::Image::ARGB, (int) (kW * scale), (int) (kH * scale), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    juce::Random rng (420);
    const juce::Rectangle<int> woodL (0, 0, 26, kH), woodR (kW - 26, 0, 26, kH), plate (26, 0, kW - 52, kH);
    paintWood (g, woodL, rng);
    paintWood (g, woodR, rng);
    paintPlate (g, plate, rng, false, true);

    {   // faixa verde-petróleo com o nome (a mesma do MRM-0420)
        const auto band = plate.withHeight (82).toFloat();
        const juce::Colour bc (0xff3d5a55);
        g.setColour (bc);
        g.fillRect (band);
        for (int i = 0; i < 1000; ++i)
        {
            const float x = band.getX() + rng.nextFloat() * band.getWidth(), y = band.getY() + rng.nextFloat() * band.getHeight();
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.03f + rng.nextFloat() * 0.05f));
            g.fillRect (x, y, 1.0f + rng.nextFloat() * 2.0f, 1.0f);
        }
        g.setColour (bc.darker (0.6f));
        g.fillRect (band.getX(), band.getBottom() - 3.0f, band.getWidth(), 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRect (band.getX(), band.getBottom(), band.getWidth(), 1.0f);
    }
    for (auto [x, y] : { std::pair<float, float> { 38.0f, 94.0f }, { kW - 38.0f, 94.0f }, { 38.0f, kH - 12.0f }, { kW - 38.0f, kH - 12.0f } })
        screw (g, x, y, rng);

    juce::String ver (JucePlugin_VersionString);
    if (ver.containsChar ('.') && ver.fromFirstOccurrenceOf (".", false, false).containsChar ('.'))
        ver = ver.upToLastOccurrenceOf (".", false, false);
    const juce::String title ("MASTOR TUNE " + ver);
    g.setFont (stencil (34.0f, 0.22f));
    const float titleW = textWidth (g.getCurrentFont(), title);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawText (title, juce::Rectangle<float> (60.0f, 14.0f, titleW + 10.0f, 42.0f).translated (0.8f, 1.0f), juce::Justification::left);
    g.setColour (juce::Colour (0xfff3e6c8));
    g.drawText (title, juce::Rectangle<float> (60.0f, 14.0f, titleW + 10.0f, 42.0f), juce::Justification::left);
    engraved (g, u8 ("AUTO-TUNE PARA REGGAE  \xc2\xb7  HARMONIA DO RIDDIM"),
              juce::Rectangle<float> (62.0f, 56.0f, 700.0f, 13.0f), juce::Justification::left, 10.0f, 0.25f, juce::Colour (0xfff3e6c8).withAlpha (0.75f));
    {
        g.setFont (stencil (25.0f, 0.3f));
        const float tw = textWidth (g.getCurrentFont(), "MT-0420");
        brassPlate (g, juce::Rectangle<float> (60.0f + titleW + 30.0f, 14.0f, tw + 92.0f, 50.0f), "MT-0420", 25.0f, rng);
    }
    {
        const float sw = 220.0f, sx = (float) plate.getRight() - 30.0f - sw, sh = 6.0f;
        const juce::Colour faded[] = { col::jamGreen.interpolatedWith (juce::Colour (0xff6f7a4a), 0.35f),
                                       col::jamGold.interpolatedWith (juce::Colour (0xffb08a3a), 0.40f), juce::Colour (0xff2b2621) };
        for (int k = 0; k < 3; ++k)
        {
            const float y = 18.0f + 8.0f * (float) k;
            juce::Path band;
            band.startNewSubPath (sx, y + rng.nextFloat() * 0.8f);
            for (float x = sx; x <= sx + sw; x += 6.0f) band.lineTo (x, y + (rng.nextFloat() - 0.3f) * 0.9f);
            for (float x = sx + sw; x >= sx; x -= 6.0f) band.lineTo (x, y + sh + (rng.nextFloat() - 0.7f) * 0.9f);
            band.closeSubPath();
            juce::ColourGradient fade (faded[k].withAlpha (0.92f), sx, y, faded[k].withAlpha (0.70f), sx + sw, y, false);
            g.setGradientFill (fade);
            g.fillPath (band);
        }
        engraved (g, "DUBSTATION", juce::Rectangle<float> (sx, 46.0f, sw, 22.0f), juce::Justification::centredRight, 19.0f, 0.42f, juce::Colour (0xfff3e6c8));
    }

    section (g, secTabs, "ABAS");
    if (tab == 0)
    {
        section (g, secHarm, u8 ("HARMONIA  \xc2\xb7  MRM \xe2\x86\x92 MIDI \xe2\x86\x92 KEY SCOUT \xe2\x86\x92 SIDECHAIN \xe2\x86\x92 MANUAL"));
        section (g, secKey, u8 ("TOM  \xc2\xb7  ESCALA"));
        section (g, secKnob, u8 ("AFINA\xc3\x87\xc3\x83O  \xc2\xb7  CLEAN \xe2\x86\x94 ROB\xc3\x94"));
        section (g, secMeter, u8 ("MEDIDOR  \xc2\xb7  VOZ"));
        section (g, secNotes, u8 ("EDITOR DE NOTAS  \xc2\xb7  TOQUE NUMA NOTA PARA CORRIGIR"));
        engraved (g, "OUTRAS ESCALAS", { 64.0f, 512.0f, 304.0f, 12.0f }, juce::Justification::centred, 9.0f, 0.22f);
        engraved (g, "TIPO DE VOZ", { 862.0f, 488.0f, 272.0f, 12.0f }, juce::Justification::centred, 9.0f, 0.22f);
        engraved (g, u8 ("\xc3\x82MBAR = AFINADA  \xc2\xb7  VERMELHA LISTRADA = D\xc3\x9aVIDA  \xc2\xb7  VERDE = CORRIGIDA POR VOC\xc3\x8a"),
                  { 356.0f, 668.0f, 610.0f, 14.0f }, juce::Justification::centredLeft, 9.0f, 0.12f);
    }
    else
    {
        // EQ em painel escuro (como o Graphic EQ do Tape Echo)
        auto eqp = secEq.toFloat();
        juce::ColourGradient dk (juce::Colour (0xff2a2520), eqp.getX(), eqp.getY(), juce::Colour (0xff1a1714), eqp.getX(), eqp.getBottom(), false);
        g.setGradientFill (dk);
        g.fillRoundedRectangle (eqp, 4.0f);
        section (g, secEq, u8 ("EQ  \xc2\xb7  ARRASTE OS PONTOS"));
        section (g, secSat, "SATURADOR");
        section (g, secBk, "BIG KNOB");
        section (g, secEcho, u8 ("TAPE ECHO  \xc2\xb7  MTE"));
        section (g, secMax, "MAXIMIZER");
        engraved (g, u8 ("CADEIA:  AFINA\xc3\x87\xc3\x83O \xe2\x86\x92 EQ \xe2\x86\x92 SATURADOR \xe2\x86\x92 BIG KNOB \xe2\x86\x92 TAPE ECHO \xe2\x86\x92 MAXIMIZER"),
                  { 64.0f, 520.0f, 820.0f, 14.0f }, juce::Justification::centredLeft, 9.0f, 0.16f);
    }
    section (g, secKeys, u8 ("TECLADO  \xc2\xb7  NOTA CANTADA ACESA  \xc2\xb7  NOTAS FORA DA ESCALA APAGADAS"));
    // assinatura
    engraved (g, u8 ("v") + ver + u8 ("  \xc2\xb7  GR\xc3\x81TIS  \xc2\xb7  VST3  \xc2\xb7  AU  \xc2\xb7  AAX  \xc2\xb7  STANDALONE"),
              { 64.0f, 1046.0f, 600.0f, 14.0f }, juce::Justification::centredLeft, 9.0f, 0.2f);
    g.setFont (script (34.0f));
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText ("DubMastor", juce::Rectangle<float> (900.0f, 1034.0f, 236.0f, 44.0f).translated (1.0f, 1.5f), juce::Justification::centredRight);
    g.setColour (col::ink);
    g.drawText ("DubMastor", juce::Rectangle<float> (900.0f, 1034.0f, 236.0f, 44.0f), juce::Justification::centredRight);
    g.setColour (col::jamGold);
    g.drawLine (980.0f, 1074.0f, 1136.0f, 1070.0f, 2.5f);
}

void Panel::paint (juce::Graphics& g)
{
    if (! background.isValid()) renderBackground();
    g.drawImage (background, getLocalBounds().toFloat());
    auto mono = [] (float h) { return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::bold)); };

    if (tab == 0)
    {
        // --- MRM
        darkGlass (g, mrmGlass);
        lampDot (g, { mrmGlass.getX() + 16.0f, mrmGlass.getCentreY() }, st.mrmOpen, juce::Colour (0xff5cff7a));
        g.setFont (stencil (12.0f, 0.16f));
        g.setColour (st.mrmOpen ? col::amber : col::amber.withAlpha (0.35f));
        g.drawText (st.mrmOpen ? (st.mrmPlaying ? u8 ("LINK MRM-0420  \xc2\xb7  TOCANDO") : u8 ("LINK MRM-0420  \xc2\xb7  PARADO")) : juce::String ("MRM-0420 FECHADO"),
                    mrmGlass.withTrimmedLeft (30.0f), juce::Justification::centredLeft);
        juce::String chord = "--";
        if (st.chordRoot >= 0) chord = juce::String (kNoteNames[st.chordRoot]) + (st.chordMinor ? "m" : "");
        else if (st.source == MastorTuneProcessor::SrcMidi) chord = "MIDI";
        const char* srcNames[] = { "MANUAL", "RIDDIM", "MIDI", "MRM", "KEY SCOUT" };
        glowText (g, u8 ("ACORDE ") + chord + u8 ("  \xc2\xb7  ") + srcNames[juce::jlimit (0, 4, st.source)],
                  mrmGlass.withTrimmedRight (12.0f), juce::Justification::centredRight);
        // --- Key Scout (master) ou riddim pela entrada lateral
        darkGlass (g, riddimGlass);
        juce::String rtxt; bool lit = false;
        if (st.scoutOpen)
        {
            lit = st.scoutSignal || st.scoutKey >= 0;
            rtxt = st.scoutKey >= 0 ? "KEY SCOUT: " + juce::String (kNoteNames[st.scoutKey]) + (st.scoutMinor ? " MENOR" : " MAIOR")
                                    : juce::String ("KEY SCOUT: OUVINDO");
        }
        else
        {
            lit = st.riddimSignal;
            rtxt = param ("listen") < 0.5f ? "RIDDIM: PAUSADO" : lit ? "RIDDIM: OUVINDO" : st.riddimConnected ? "RIDDIM: SEM SOM" : "SEM KEY SCOUT / SIDECHAIN";
        }
        lampDot (g, { riddimGlass.getX() + 16.0f, riddimGlass.getCentreY() }, lit, st.scoutOpen ? juce::Colour (0xff5cff7a) : col::amber);
        g.setFont (stencil (11.0f, 0.12f));
        g.setColour (col::amber.withAlpha (lit ? 1.0f : 0.6f));
        g.drawFittedText (rtxt, riddimGlass.withTrimmedLeft (30.0f).withTrimmedRight (6.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
        // --- aviso da harmonia automática
        g.setColour (juce::Colour (0x141a1714));
        g.fillRoundedRectangle (infoBox, 4.0f);
        {
            juce::Path border; border.addRoundedRectangle (infoBox, 4.0f);
            juce::Path d; const float dash[] = { 5.0f, 4.0f };
            juce::PathStrokeType (1.0f).createDashedStroke (d, border, dash, 2);
            g.setColour (col::ink.withAlpha (0.45f)); g.fillPath (d);
        }
        const auto ic = juce::Rectangle<float> (22.0f, 22.0f).withCentre ({ infoBox.getX() + 18.0f, infoBox.getCentreY() });
        g.setColour (col::labelBg); g.fillEllipse (ic);
        g.setColour (col::amber); g.setFont (stencil (15.0f, 0.0f));
        g.drawText ("i", ic, juce::Justification::centred);
        g.setColour (col::ink);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawFittedText (u8 ("HARMONIA AUTOM\xc3\x81TICA: o Mastor Tune escuta o riddim e encontra sozinho o tom, a escala e os acordes. "
                              "Coloque o Mastor Key Scout no MASTER (sem sidechain) ou abra o MRM-0420; sem nenhum dos dois, escolha o tom \xc3\xa0 m\xc3\xa3o."),
                          infoBox.withTrimmedLeft (38.0f).withTrimmedRight (8.0f).toNearestInt(), juce::Justification::centredLeft, 2, 0.9f);

        // --- tom
        darkGlass (g, keyGlass);
        g.setFont (stencil (15.0f, 0.2f));
        static const char* upperNames[] = { "MAIOR", "MENOR", "D\xc3\x93RICO", "MIXOL\xc3\x8d" "DIO", "FR\xc3\x8dGIO", "L\xc3\x8d" "DIO", "L\xc3\x93" "CRIO",
                                            "HARM. MENOR", "PENTA MAIOR", "PENTA MENOR", "BLUES", "CROM\xc3\x81TICA" };
        glowText (g, juce::String (kNoteNames[st.keyRoot]) + " " + juce::String::fromUTF8 (upperNames[juce::jlimit (0, 11, st.scale)]),
                  keyGlass, juce::Justification::centred);
        engraved (g, st.keyAuto ? u8 ("\xe2\x97\x8f AUTO: ACHADO NO RIDDIM / MRM \xc2\xb7 TOQUE NUMA TECLA PARA MUDAR") : u8 ("MANUAL \xc2\xb7 TOQUE EM AUTO PARA DETECTAR"),
                  { 64.0f, 367.0f, 304.0f, 11.0f }, juce::Justification::centred, 8.0f, 0.08f);

        // --- knob
        darkGlass (g, knobGlass);
        const float v = tuneKnob->getValue();
        g.setFont (mono (22.0f));
        g.setColour (col::amber);
        g.drawText (juce::String (juce::roundToInt (v)), knobGlass.reduced (12.0f, 0), juce::Justification::centredLeft);
        g.setFont (stencil (11.0f, 0.14f));
        glowText (g, v < 35 ? u8 ("INVIS\xc3\x8dVEL") : v < 70 ? "FIRME" : "HARD-TUNE", knobGlass.reduced (12.0f, 0), juce::Justification::centredRight);

        // --- nota cantada
        const auto nr = juce::Rectangle<float> (862.0f, 444.0f, 272.0f, 38.0f);
        darkGlass (g, nr);
        const bool sing = st.sungMidi > 0.0f;
        g.setFont (mono (22.0f));
        g.setColour (col::amber.withAlpha (sing ? 1.0f : 0.35f));
        g.drawText (sing ? noteName (juce::roundToInt (st.sungMidi)) : "--", nr.reduced (12.0f, 0), juce::Justification::centredLeft);
        g.setFont (mono (13.0f));
        const float dev = sing && st.target >= 0 ? (st.sungMidi - (float) st.target) * 100.0f : 0.0f;
        g.drawText (sing && st.target >= 0 ? (u8 ("\xe2\x86\x92 ") + noteName (st.target) + "  " + (dev >= 0 ? "+" : "") + juce::String (juce::roundToInt (dev)) + " ct") : juce::String ("SEM VOZ"),
                    nr.reduced (12.0f, 0), juce::Justification::centredRight);

        // --- dúvidas
        darkGlass (g, doubtGlass);
        const int pend = proc.getNotes().pendingDoubts();
        lampDot (g, { doubtGlass.getX() + 15.0f, doubtGlass.getCentreY() }, pend > 0, juce::Colour (0xffff5a3c));
        g.setFont (stencil (11.0f, 0.12f));
        g.setColour (pend > 0 ? juce::Colour (0xffff7a5c) : col::amber.withAlpha (0.8f));
        g.drawText (pend == 0 ? juce::String (u8 ("TUDO REVISADO")) : juce::String (pend) + (pend == 1 ? u8 (" NOTA EM D\xc3\x9aVIDA") : u8 (" NOTAS EM D\xc3\x9aVIDA")),
                    doubtGlass.withTrimmedLeft (28.0f), juce::Justification::centredLeft);
    }
    else
    {
        darkGlass (g, eqRead);
        g.setFont (mono (13.0f));
        g.setColour (col::amber);
        g.drawText (eqGraph->readout(), eqRead.reduced (10.0f, 0), juce::Justification::centredLeft);

        darkGlass (g, maxRead);
        const bool on = param ("maxOn") > 0.5f;
        const juce::String rows[4][2] = {
            { "PUSH", on ? "+" + juce::String (-param ("maxThresh"), 1) + " dB" : "+0.0 dB" },
            { "GR", juce::String (st.grDb, 1) + " dB" },
            { "PEAK", st.outPeak[0] > 0.0001f ? juce::String (juce::Decibels::gainToDecibels (juce::jmax (st.outPeak[0], st.outPeak[1])), 1) : "-inf" },
            { "LUFS-S", st.lufs > -69.0f ? juce::String (st.lufs, 1) : "--" } };
        for (int i = 0; i < 4; ++i)
        {
            auto row = juce::Rectangle<float> (maxRead.getX() + 10.0f, maxRead.getY() + 8.0f + 19.0f * (float) i, maxRead.getWidth() - 20.0f, 18.0f);
            g.setFont (stencil (9.0f, 0.2f));
            g.setColour (col::cream.withAlpha (0.6f));
            g.drawText (rows[i][0], row, juce::Justification::centredLeft);
            g.setFont (mono (13.0f));
            g.setColour (i < 2 ? col::amber : juce::Colour (0xff5cff7a));
            g.drawText (rows[i][1], row, juce::Justification::centredRight);
        }
    }
}

void Panel::paintOverChildren (juce::Graphics& g)
{
    // módulos desligados ficam apagados (a tecla LIGADO/DESLIGADO continua acesa por cima)
    if (tab != 1) return;
    const std::pair<const char*, juce::Rectangle<int>> mods[] = { { "eqOn", secEq }, { "satOn", secSat }, { "bkOn", secBk }, { "echoOn", secEcho }, { "maxOn", secMax } };
    for (auto& [id, r] : mods)
        if (param (id) < 0.5f)
        {
            g.setColour (juce::Colour (0xffe4d8bc).withAlpha (0.28f));
            g.fillRoundedRectangle (r.toFloat().reduced (2.0f).withTrimmedBottom (48.0f).withTrimmedTop (12.0f), 4.0f);
        }
}

void Panel::timerCallback()
{
    st = proc.getStatus();
    anim += 1.0f / 30.0f;
    for (auto& k : keys)
    {
        if (k->getProperties().contains ("param"))
        {
            const bool on = param (k->getProperties()["param"].toString().toRawUTF8()) > 0.5f;
            k->setText (k->getProperties()[on ? "onText" : "offText"].toString());
        }
        if (k->pulse) k->pulsePhase = std::fmod (anim * (float) (proc.getHostBpm() / 60.0), 1.0f);
        if (k->isVisible()) k->repaint();
    }
    if (piano != nullptr && piano->shown != st.keyRoot) { piano->shown = st.keyRoot; piano->repaint(); }
    if (keyboard != nullptr) keyboard->set (mt::scaleMask (st.keyRoot, st.scale), st.sungMidi, st.target, st.tuneBypass);
    if (tab == 0)
    {
        const float cents = st.sungMidi > 0 && st.target >= 0 ? (st.sungMidi - (float) st.target) * 100.0f : 0.0f;
        meter->set (cents, st.sungMidi > 0);
        noteEditor->blink = anim;
        noteEditor->refresh (proc.notesChanged.exchange (false));
        for (auto r : { mrmGlass, riddimGlass, keyGlass, knobGlass, doubtGlass, juce::Rectangle<float> (862.0f, 444.0f, 272.0f, 38.0f), juce::Rectangle<float> (64.0f, 365.0f, 304.0f, 15.0f) })
            repaint (r.toNearestInt().expanded (2));
    }
    else
    {
        satView->phase = anim; satView->repaint();
        echoView->phase = anim; echoView->repaint();
        maxMeters->update();
        eqGraph->repaint();
        repaint (eqRead.toNearestInt().expanded (2));
        repaint (maxRead.toNearestInt().expanded (2));
        repaint (secEq); repaint (secSat); repaint (secBk); repaint (secEcho); repaint (secMax);
    }
}
} // namespace mtui
