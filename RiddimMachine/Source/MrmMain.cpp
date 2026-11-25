#include "MrmMain.h"
#include "MrmStyle.h"
#include "../../ReggaeChordMachine/Source/HarmonyPresets.h"
#include "BpmPresets.h"
#include "../../RiddimBank/Source/RiddimBankMenu.h"

#ifndef MRM_DRUMS_VER
 #define MRM_DRUMS_VER ""
#endif
#ifndef MRM_CHORDS_VER
 #define MRM_CHORDS_VER ""
#endif
#ifndef MRM_BASS_VER
 #define MRM_BASS_VER ""
#endif

namespace mrmui
{
//==============================================================================
// PowerSwitch
PowerSwitch::PowerSwitch (juce::RangedAudioParameter& p, juce::String onT, juce::String offT)
    : param (p), att (p, [this] (float v) { on = v > 0.5f; repaint(); }, nullptr),
      onText (std::move (onT)), offText (std::move (offT))
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setRepaintsOnMouseActivity (true);
}

void PowerSwitch::mouseDown (const juce::MouseEvent&) { att.setValueAsCompleteGesture (on ? 0.0f : 1.0f); }

void PowerSwitch::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (r.translated (0, 2.0f), 5.0f);
    juce::ColourGradient body (juce::Colour (0xff3a3632).brighter (isMouseOver() ? 0.08f : 0.0f), r.getX(), r.getY(),
                               juce::Colour (0xff141210), r.getX(), r.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, 5.0f);

    // alavanca: para a direita = ON
    auto slot = juce::Rectangle<float> (r.getHeight() * 1.5f, r.getHeight() * 0.56f)
                    .withCentre ({ r.getX() + r.getHeight() * 0.95f, r.getCentreY() });
    g.setColour (juce::Colour (0xff0a0807));
    g.fillRoundedRectangle (slot, slot.getHeight() * 0.5f);
    auto knob = juce::Rectangle<float> (slot.getHeight() + 2.0f, slot.getHeight() + 2.0f)
                    .withCentre ({ on ? slot.getRight() - slot.getHeight() * 0.5f : slot.getX() + slot.getHeight() * 0.5f, slot.getCentreY() });
    juce::ColourGradient kg (juce::Colour (0xffefe9dc), knob.getX(), knob.getY(), juce::Colour (0xff8d8576), knob.getRight(), knob.getBottom(), false);
    g.setGradientFill (kg);
    g.fillEllipse (knob);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (knob, 0.8f);

    auto txt = r.withTrimmedLeft (slot.getRight() - r.getX() + 6.0f).withTrimmedRight (26.0f);
    g.setFont (stencil (juce::jmin (12.5f, r.getHeight() * 0.42f), 0.14f));
    g.setColour (on ? col::amber : col::cream.withAlpha (0.4f));
    g.drawFittedText (on ? onText : offText, txt.toNearestInt(), juce::Justification::centredLeft, 1, 0.55f);
    lamp (g, juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ r.getRight() - 14.0f, r.getCentreY() }), on, juce::Colour (0xff5cff7a));
}

//==============================================================================
// MusicianKeys
MusicianKeys::MusicianKeys (juce::RangedAudioParameter& p, juce::StringArray n, int columns)
    : param (p), att (p, [this] (float v) { selected = juce::roundToInt (v); repaint(); }, nullptr),
      names (std::move (n)), cols (juce::jmax (1, columns))
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

juce::Rectangle<float> MusicianKeys::keyRect (int i) const
{
    const int rowsN = (names.size() + cols - 1) / cols;
    const float gap = 4.0f;
    const float w = ((float) getWidth() - gap * (float) (cols - 1)) / (float) cols;
    const float h = ((float) getHeight() - gap * (float) (rowsN - 1)) / (float) rowsN;
    const int row = i / cols, c = i % cols;
    // a última linha incompleta fica centralizada
    const int inRow = juce::jmin (cols, names.size() - row * cols);
    const float offset = (float) (cols - inRow) * (w + gap) * 0.5f;
    return { offset + (float) c * (w + gap), (float) row * (h + gap), w, h };
}

int MusicianKeys::keyAt (juce::Point<float> p) const
{
    for (int i = 0; i < names.size(); ++i)
        if (keyRect (i).contains (p)) return i;
    return -1;
}

void MusicianKeys::mouseDown (const juce::MouseEvent& e)
{
    const int k = keyAt (e.position);
    if (k >= 0)
    {
        att.setValueAsCompleteGesture ((float) k);
        if (onSelect) onSelect (k);
    }
}

void MusicianKeys::paint (juce::Graphics& g)
{
    const int hover = isMouseOver() ? keyAt (getMouseXYRelative().toFloat()) : -1;
    for (int i = 0; i < names.size(); ++i)
    {
        auto r = keyRect (i);
        const bool sel = i == selected;
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r.translated (0, 2.0f), 3.0f);
        auto cap = r.translated (0, sel ? 1.5f : 0.0f);
        juce::ColourGradient cg (juce::Colour (sel ? 0xff2c2925 : 0xff3a3632).brighter (i == hover && ! sel ? 0.1f : 0.0f),
                                 cap.getX(), cap.getY(), juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (cap, 3.0f);
        if (sel)
        {
            g.setColour (col::amber.withAlpha (0.8f));
            g.fillRoundedRectangle (cap.withTop (cap.getBottom() - 3.0f).reduced (6.0f, 0.0f).translated (0, -1.5f), 1.0f);
        }
        lamp (g, juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ cap.getX() + 9.0f, cap.getCentreY() }), sel);
        g.setColour (sel ? col::amber : col::cream.withAlpha (0.6f));
        g.setFont (stencil (juce::jmin (10.5f, cap.getHeight() * 0.36f), 0.08f));
        g.drawFittedText (names[i], cap.withTrimmedLeft (16.0f).withTrimmedRight (3.0f).toNearestInt(),
                          juce::Justification::centred, 2, 0.75f);
    }
}

//==============================================================================
// BigDial
BigDial::BigDial (juce::RangedAudioParameter& p)
    : param (p), att (p, [this] (float v) { value = param.convertTo0to1 (v); repaint(); }, nullptr)
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void BigDial::mouseDown (const juce::MouseEvent&) { dragStart = value; att.beginGesture(); }
void BigDial::mouseUp (const juce::MouseEvent&)   { att.endGesture(); }
void BigDial::mouseDrag (const juce::MouseEvent& e)
{
    const float v = juce::jlimit (0.0f, 1.0f, dragStart - (float) e.getDistanceFromDragStartY() / 220.0f);
    att.setValueAsPartOfGesture (param.convertFrom0to1 (v));
}
void BigDial::mouseDoubleClick (const juce::MouseEvent&) { att.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }
void BigDial::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    att.setValueAsCompleteGesture (param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, value + w.deltaY * 0.15f)));
}

void BigDial::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float size = juce::jmin (r.getWidth(), r.getHeight());
    auto c = r.withSizeKeepingCentre (size, size);
    const float a0 = juce::MathConstants<float>::pi * 1.25f, a1 = juce::MathConstants<float>::pi * 2.75f;
    const float ang = a0 + (a1 - a0) * value;
    const auto ctr = c.getCentre();
    const float rOut = size * 0.5f - 3.0f;

    // escala: traços e o arco âmbar até o valor
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.0f;
        const float r1 = rOut, r2 = rOut - (i % 5 == 0 ? 8.0f : 5.0f);
        g.setColour (col::ink.withAlpha (0.7f));
        g.drawLine (ctr.x + std::sin (a) * r1, ctr.y - std::cos (a) * r1, ctr.x + std::sin (a) * r2, ctr.y - std::cos (a) * r2, i % 5 == 0 ? 1.8f : 1.1f);
    }
    juce::Path arc;
    arc.addCentredArc (ctr.x, ctr.y, rOut - 12.0f, rOut - 12.0f, 0.0f, a0, ang, true);
    if (value > 0.001f)
    {
        g.setColour (col::amber.withAlpha (0.9f));
        g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // botão grande cromado com saia preta
    const float kr = rOut - 18.0f;
    auto knob = juce::Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (ctr);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (knob.translated (2.0f, 4.0f));
    juce::ColourGradient skirt (juce::Colour (0xff3a3632), knob.getX(), knob.getY(), juce::Colour (0xff0e0c0a), knob.getX(), knob.getBottom(), false);
    g.setGradientFill (skirt);
    g.fillEllipse (knob);
    auto cap = knob.reduced (kr * 0.2f);
    juce::ColourGradient chrome (juce::Colour (0xfff4f2ec), cap.getX(), cap.getY(), juce::Colour (0xff5e5a52), cap.getRight(), cap.getBottom(), false);
    chrome.addColour (0.5, juce::Colour (0xffb9b5ac));
    g.setGradientFill (chrome);
    g.fillEllipse (cap);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawEllipse (cap, 1.0f);
    // ponteiro
    const float p1 = kr * 0.25f, p2 = kr * 0.92f;
    g.setColour (col::jamRed);
    g.drawLine (ctr.x + std::sin (ang) * p1, ctr.y - std::cos (ang) * p1, ctr.x + std::sin (ang) * p2, ctr.y - std::cos (ang) * p2, 4.0f);
}

//==============================================================================
// SirenFire
SirenFire::SirenFire (juce::RangedAudioParameter& p)
    : att (p, [this] (float v) { down = v > 0.5f; repaint(); }, nullptr)
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}
void SirenFire::mouseDown (const juce::MouseEvent&) { att.setValueAsCompleteGesture (1.0f); }
void SirenFire::mouseUp (const juce::MouseEvent&)   { att.setValueAsCompleteGesture (0.0f); }
void SirenFire::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float size = juce::jmin (r.getWidth(), r.getHeight()) - 6.0f;
    auto c = r.withSizeKeepingCentre (size, size);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.translated (2.0f, 4.0f));
    juce::ColourGradient bezel (juce::Colour (0xffd9d4c8), c.getX(), c.getY(), juce::Colour (0xff4a463f), c.getRight(), c.getBottom(), false);
    g.setGradientFill (bezel);
    g.fillEllipse (c);
    auto cap = c.reduced (size * 0.1f).translated (0, down ? 2.0f : 0.0f);
    const auto red = juce::Colour (0xffe8432c);
    juce::ColourGradient cg (down ? red.brighter (0.5f) : red, cap.getX(), cap.getY(), red.darker (0.8f), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillEllipse (cap);
    if (down)
    {
        g.setColour (red.withAlpha (0.35f));
        g.fillEllipse (c.expanded (3.0f));
    }
    g.setColour (juce::Colours::white.withAlpha (down ? 0.95f : 0.8f));
    g.setFont (stencil (size * 0.2f, 0.12f));
    g.drawText ("FIRE", cap, juce::Justification::centred);
}

//==============================================================================
// BpmPanel: seção BPM da aba RIDDIM
BpmPanel::BpmPanel (MrmProcessor& p)
    : proc (p), att (*p.apvts.getParameter ("bpm"), [this] (float v)
      {
          value = v;
          if (chosen >= 0 && juce::roundToInt (v) != bpmpresets::presets[chosen].bpm) chosen = -1;
          repaint();
      }, nullptr)
{
    att.sendInitialUpdate();
    entry.setInputRestrictions (3, "0123456789");
    entry.setJustification (juce::Justification::centred);
    entry.setFont (stencil (40.0f, 0.05f));
    entry.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff120e0a));
    entry.setColour (juce::TextEditor::textColourId, col::amber);
    entry.setColour (juce::TextEditor::highlightColourId, col::amber.withAlpha (0.3f));
    entry.setColour (juce::TextEditor::outlineColourId, col::amber.withAlpha (0.6f));
    entry.setColour (juce::TextEditor::focusedOutlineColourId, col::amber);
    entry.setColour (juce::CaretComponent::caretColourId, col::amber);
    entry.onReturnKey = [this] { commitEntry(); };
    entry.onFocusLost = [this] { commitEntry(); };
    entry.onEscapeKey = [this] { entry.setVisible (false); };
    addChildComponent (entry);
    startTimerHz (10);
}

void BpmPanel::setBpm (float v)
{
    att.setValueAsCompleteGesture (juce::jlimit (50.0f, 180.0f, std::round (v)));
}

void BpmPanel::choosePreset (int i)
{
    if (i < 0 || i >= bpmpresets::numPresets) return;
    setBpm ((float) bpmpresets::presets[i].bpm);
    chosen = i;
    repaint();
}

void BpmPanel::tap (double ms)
{
    if (! taps.empty() && ms - taps.back() > 2000.0) taps.clear();
    taps.push_back (ms);
    if (taps.size() > 6) taps.erase (taps.begin());
    tapFlash = ms;
    if (taps.size() >= 2)
    {
        const double avg = (taps.back() - taps.front()) / (double) (taps.size() - 1);
        if (avg > 0.0) { chosen = -1; setBpm ((float) (60000.0 / avg)); }
    }
    repaint();
}

void BpmPanel::openEntry()
{
    entry.setBounds (display.reduced (10, 16));
    entry.setText (juce::String (juce::roundToInt (value)), false);
    entry.setVisible (true);
    entry.grabKeyboardFocus();
    entry.selectAll();
}

void BpmPanel::commitEntry()
{
    if (! entry.isVisible()) return;
    entry.setVisible (false);
    const int v = entry.getText().getIntValue();
    if (v > 0) { chosen = -1; setBpm ((float) v); }
}

void BpmPanel::timerCallback()
{
    const bool daw = proc.isDawPlaying();
    const double b = proc.getDawBpm();
    if (daw != dawShown || (daw && std::abs (b - dawBpm) > 0.01)) { dawShown = daw; dawBpm = b; repaint(); }
    if (tapFlash > 0.0 && juce::Time::getMillisecondCounterHiRes() - tapFlash > 150.0) { tapFlash = 0.0; repaint(); }
}

void BpmPanel::resized()
{
    display = { 0, 0, 196, 90 };
    const int sx = 208;
    steps[0] = { sx, 0, 54, 42 };       // -1
    steps[1] = { sx + 60, 0, 54, 42 };  // +1
    steps[2] = { sx, 48, 54, 42 };      // -5
    steps[3] = { sx + 60, 48, 54, 42 }; // +5
    tapKey = { sx + 126, 0, 84, 90 };
    // PRESET DE BPM no visor azul (igual ao PRESET do Dub Battery Reggae): ◀ visor ▶
    stepL = { 436, 0, 40, 32 };
    stepR = { getWidth() - 40, 0, 40, 32 };
    presetLcd = { 486, 2, getWidth() - 40 - 486 - 10, 28 };
}

juce::Rectangle<int> BpmPanel::keyArea (int i) const
{
    const int x0 = 436, w = getWidth() - x0, cols = 12, gap = 5, top = 40;
    const int kw = (w - gap * (cols - 1)) / cols, kh = (getHeight() - top - gap) / 2;
    return { x0 + (i % cols) * (kw + gap), top + (i / cols) * (kh + gap), kw, kh };
}

void BpmPanel::stepPreset (int delta)
{
    int i = chosen;
    if (i < 0)
    {   // sem preset aceso: começa pelo mais perto do BPM atual
        int best = 0;
        for (int k = 1; k < bpmpresets::numPresets; ++k)
            if (std::abs (bpmpresets::presets[k].bpm - value) < std::abs (bpmpresets::presets[best].bpm - value)) best = k;
        i = delta > 0 ? best - 1 : best + 1;
    }
    choosePreset ((i + delta + bpmpresets::numPresets) % bpmpresets::numPresets);
}

static void bpmKey (juce::Graphics& g, juce::Rectangle<float> r, bool lit, bool down)
{
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (0, 2.0f), 3.0f);
    auto cap = r.translated (0, down ? 1.5f : 0.0f);
    juce::ColourGradient cg (juce::Colour (0xff2c2925), cap.getX(), cap.getY(), juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 3.0f);
    g.setColour (lit ? col::amber : col::amber.withAlpha (0.18f));
    g.fillRoundedRectangle (cap.withTop (cap.getBottom() - 3.0f).reduced (6.0f, 0.0f).translated (0, -1.5f), 1.0f);
}

void BpmPanel::paint (juce::Graphics& g)
{
    // visor grande
    auto d = display.toFloat();
    displayGlass (g, d.reduced (2.0f), 4.0f);
    g.setFont (stencil (9.0f, 0.3f));
    glowText (g, dawShown ? "BPM DA DAW" : "BPM", d.reduced (8.0f, 4.0f).withHeight (14.0f), juce::Justification::centredLeft, col::amber, 0.7f);
    if (! entry.isVisible())
    {
        g.setFont (stencil (46.0f, 0.05f));
        const juce::String t = dawShown ? juce::String (dawBpm, 1) : juce::String (juce::roundToInt (value));
        glowText (g, t, d.reduced (6.0f, 12.0f).translated (0, 4.0f), juce::Justification::centred);
    }
    g.setFont (stencil (7.5f, 0.2f));
    glowText (g, dawShown ? juce::String ("A DAW MANDA NO ANDAMENTO") : juce::String::fromUTF8 ("CLIQUE E DIGITE  \xc2\xb7  ARRASTE"), d.reduced (8.0f, 4.0f).withTrimmedTop (d.getHeight() - 20.0f),
              juce::Justification::centred, col::amber, 0.5f);

    const char* stepNames[] = { "-1", "+1", "-5", "+5" };
    for (int i = 0; i < 4; ++i)
    {
        auto r = steps[i].toFloat();
        bpmKey (g, r, false, pressed == 100 + i);
        g.setColour (col::cream);
        g.setFont (stencil (16.0f, 0.05f));
        g.drawText (stepNames[i], r.translated (0, pressed == 100 + i ? 1.5f : 0.0f).withTrimmedBottom (4.0f), juce::Justification::centred);
    }
    {
        auto r = tapKey.toFloat();
        const bool flash = tapFlash > 0.0;
        bpmKey (g, r, flash, pressed == 200);
        lamp (g, juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ r.getCentreX(), r.getY() + 16.0f }), flash);
        g.setColour (col::cream);
        g.setFont (stencil (19.0f, 0.12f));
        g.drawText ("TAP", r.withTrimmedTop (24.0f).withHeight (30.0f), juce::Justification::centred);
        g.setFont (stencil (7.0f, 0.15f));
        g.setColour (col::cream.withAlpha (0.6f));
        g.drawText (juce::String::fromUTF8 ("BATA NO TEMPO"), r.withTrimmedTop (56.0f).withHeight (14.0f), juce::Justification::centred);
    }

    // PRESET DE BPM: ◀ visor azul ▶ e as 24 teclas por época e estilo
    mpc::key (g, stepL.toFloat(), juce::String::fromUTF8 ("\xe2\x97\x80"), pressed == 300, 12.0f);
    mpc::key (g, stepR.toFloat(), juce::String::fromUTF8 ("\xe2\x96\xb6"), pressed == 301, 12.0f);
    {
        auto l = presetLcd.toFloat();
        mpc::lcd (g, l);
        auto t = l.reduced (10.0f, 2.0f);
        mpc::text (g, "BPM", t, juce::Justification::centredLeft, 9.0f, 0.7f, 0.25f);
        if (chosen >= 0)
        {
            const auto& pr = bpmpresets::presets[chosen];
            mpc::text (g, juce::String (chosen + 1).paddedLeft ('0', 2) + "  " + juce::String (pr.style) + u8 ("  \xc2\xb7  ")
                          + juce::String (pr.name).toUpperCase(), t.withTrimmedLeft (40.0f), juce::Justification::centredLeft, 13.0f);
            mpc::text (g, juce::String (pr.bpm) + " BPM", t, juce::Justification::centredRight, 14.0f);
        }
        else
        {
            mpc::text (g, u8 ("PRESET DE BPM  \xc2\xb7  \xe2\x97\x80 \xe2\x96\xb6 ESCOLHE O ESTILO"), t.withTrimmedLeft (40.0f), juce::Justification::centredLeft, 12.0f, 0.75f);
            mpc::text (g, juce::String (juce::roundToInt (value)) + " BPM", t, juce::Justification::centredRight, 14.0f);
        }
        mpc::shine (g, l);
    }
    for (int i = 0; i < bpmpresets::numPresets; ++i)
    {
        const auto& pr = bpmpresets::presets[i];
        auto r = keyArea (i).toFloat();
        const bool lit = chosen == i;
        bpmKey (g, r, lit, pressed == i);
        auto t = r.translated (0, pressed == i ? 1.5f : 0.0f).reduced (2.0f, 2.0f);
        g.setColour (lit ? col::amber : col::cream.withAlpha (0.75f));
        g.setFont (stencil (6.5f, 0.02f));
        juce::String shortStyle (pr.style);   // tecla pequena: sem REGGAE, ROCK e UK
        shortStyle = shortStyle.replace ("EARLY REGGAE", "EARLY").replace ("LOVERS ROCK", "LOVERS").replace (" UK", "");
        g.drawFittedText (shortStyle, t.withHeight (9.0f).toNearestInt(), juce::Justification::centred, 1, 0.55f);
        g.setColour (lit ? col::amber : col::cream);
        g.setFont (stencil (12.5f, 0.03f));
        g.drawText (juce::String (pr.bpm), t.withTrimmedTop (9.0f).withTrimmedBottom (3.0f), juce::Justification::centred);
    }
}

void BpmPanel::mouseDown (const juce::MouseEvent& e)
{
    const auto pt = e.getPosition();
    pressed = -1;
    dragging = false;
    if (display.contains (pt)) { dragStart = value; return; }
    for (int i = 0; i < 4; ++i)
        if (steps[i].contains (pt))
        {
            const float delta[] = { -1.0f, 1.0f, -5.0f, 5.0f };
            pressed = 100 + i; chosen = -1;
            setBpm (value + delta[i]);
            repaint();
            return;
        }
    if (tapKey.contains (pt)) { pressed = 200; tap (juce::Time::getMillisecondCounterHiRes()); return; }
    if (stepL.contains (pt)) { pressed = 300; stepPreset (-1); return; }
    if (stepR.contains (pt) || presetLcd.contains (pt)) { pressed = 301; stepPreset (1); return; }
    for (int i = 0; i < bpmpresets::numPresets; ++i)
        if (keyArea (i).contains (pt)) { pressed = i; choosePreset (i); return; }
}

void BpmPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (! display.contains (e.getMouseDownPosition()) || std::abs (e.getDistanceFromDragStartY()) < 3) return;
    if (! dragging) { dragging = true; att.beginGesture(); }
    chosen = -1;
    att.setValueAsPartOfGesture (juce::jlimit (50.0f, 180.0f, std::round (dragStart - (float) e.getDistanceFromDragStartY() * 0.35f)));
}

void BpmPanel::mouseUp (const juce::MouseEvent& e)
{
    if (dragging) att.endGesture();
    else if (display.contains (e.getMouseDownPosition()) && display.contains (e.getPosition())) openEntry();
    dragging = false;
    pressed = -1;
    repaint();
}

void BpmPanel::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (! display.contains (e.getPosition())) return;
    const int step = w.deltaY > 0 ? 1 : w.deltaY < 0 ? -1 : 0;
    if (step != 0) { chosen = -1; setBpm (value + (float) step); }
}

//==============================================================================
// RiddimPresetPanel: BANCO ÚNICO DE RIDDIMS no visor azul de MPC (estilo > música), carrega na hora
static const char* const kKits[] = { "ROCKERS", "ONE DROP", "BOOM BAP", "DEMBOW" };

RiddimPresetPanel::RiddimPresetPanel (MrmProcessor& p) : proc (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    startTimerHz (8);
}

int RiddimPresetPanel::current() const { return proc.currentBankRiddim(); }   // projeto antigo: o preset do MRCM vira a música do banco

int RiddimPresetPanel::styleOf (int entry) const
{
    return entry >= 0 && entry < riddimbank::numEntries ? riddimbank::entries[entry].style : -1;
}

juce::Array<int> RiddimPresetPanel::songsOf (int style) const
{
    juce::Array<int> out;   // ordem do menu MÚSICAS A–Z, só as que já têm harmonia
    for (int k = 0; k < riddimbank::numEntries; ++k)
    {
        const int i = riddimbank::songOrder[k];
        if (riddimbank::entries[i].style == style && riddimbank::usable (i)) out.add (i);
    }
    return out;
}

void RiddimPresetPanel::stepStyle (int delta)
{
    const int cur = styleOf (current());
    for (int tries = 0; tries < riddimbank::numStyles; ++tries)
    {
        styleSel = ((cur >= 0 && tries == 0 ? cur : styleSel) + delta + riddimbank::numStyles) % riddimbank::numStyles;
        const auto songs = songsOf (styleSel);
        if (! songs.isEmpty()) { proc.applyBankRiddim (songs[0]); break; }
    }
    repaint();
}

void RiddimPresetPanel::stepSong (int delta)
{
    const int cur = current();
    const int st = cur >= 0 ? styleOf (cur) : styleSel;
    const auto songs = songsOf (st);
    if (songs.isEmpty()) return;
    const int pos = songs.indexOf (cur);
    const int n = songs.size();
    const int next = pos < 0 ? (delta > 0 ? 0 : n - 1) : (pos + delta + n) % n;
    proc.applyBankRiddim (songs[next]);
    repaint();
}

void RiddimPresetPanel::showList()
{
    juce::PopupMenu menu;
    riddimbank::addMenus (menu, current());   // ESTILOS · RIDDIMS A–Z · MÚSICAS A–Z (o mesmo menu de todas as machines)
    juce::Component::SafePointer<RiddimPresetPanel> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        if (r == riddimbank::toggleCompleteId) { riddimbank::onlyComplete() = ! riddimbank::onlyComplete(); safe->showList(); return; }
        if (const int e = riddimbank::entryFromMenuResult (r); e >= 0) { safe->proc.applyBankRiddim (e); safe->repaint(); }
    });
}

void RiddimPresetPanel::timerCallback()
{
    const int c = current();
    if (c != shown) { shown = c; if (c >= 0) styleSel = styleOf (c); repaint(); }
    else if (const int ph = riddimbank::creditPhase(); ph != shownPhase) { shownPhase = ph; if (c >= 0 && *riddimbank::entries[c].credits) repaint (lcd); }
}

void RiddimPresetPanel::resized()
{
    // ESTILO ◀ ▶ em cima, MÚSICA ◀ ▶ embaixo, o visor no meio e a LISTA na direita
    keys[0] = { 0, 0, 44, 34 };   keys[1] = { 50, 0, 44, 34 };
    keys[2] = { 0, 40, 44, 34 };  keys[3] = { 50, 40, 44, 34 };
    listKey = { getWidth() - 112, 0, 112, 74 };
    lcd = { 184, 2, getWidth() - 184 - 112 - 14, 70 };
}

void RiddimPresetPanel::paint (juce::Graphics& g)
{
    const juce::String L = juce::String::fromUTF8 ("\xe2\x97\x80"), R = juce::String::fromUTF8 ("\xe2\x96\xb6");
    for (int i = 0; i < 4; ++i)
        mpc::key (g, keys[i].toFloat(), i % 2 == 0 ? L : R, pressed == i);
    engraved (g, "ESTILO", juce::Rectangle<float> (100.0f, 0.0f, 76.0f, 34.0f), juce::Justification::centredLeft, 10.0f, 0.25f, col::ink);
    engraved (g, u8 ("M\xc3\x9aSICA"), juce::Rectangle<float> (100.0f, 40.0f, 76.0f, 34.0f), juce::Justification::centredLeft, 10.0f, 0.25f, col::ink);

    auto l = lcd.toFloat();
    mpc::lcd (g, l);
    auto t = l.reduced (14.0f, 6.0f);
    const int cur = current();
    const int st = cur >= 0 ? styleOf (cur) : styleSel;
    const auto songs = songsOf (st);
    mpc::text (g, juce::String (riddimbank::styles[st]) + "  " + riddimbank::styleYears[st], t.withHeight (16.0f), juce::Justification::centredLeft, 13.0f, 0.85f, 0.25f);
    if (cur >= 0)
    {
        const auto& e = riddimbank::entries[cur];
        const auto count = juce::String (songs.indexOf (cur) + 1).paddedLeft ('0', 2) + "/" + juce::String (songs.size()).paddedLeft ('0', 2);
        mpc::text (g, count, t.withHeight (16.0f), juce::Justification::centredRight, 12.0f, 0.8f);
        {   // luzes H B D: acesa = a parte veio da gravação original
            static const juce::uint32 ledCol[3] = { 0xffe5484d, 0xfff5c542, 0xff3fb950 };
            static const char* const ledTxt[3] = { "H", "B", "D" };
            auto row = t.withHeight (16.0f).withTrimmedRight (70.0f).removeFromRight (78.0f);
            for (int k = 0; k < 3; ++k)
            {
                auto cell = row.removeFromLeft (26.0f);
                const bool on = (e.orig >> k) & 1;
                const auto dot = cell.removeFromLeft (10.0f).withSizeKeepingCentre (8.0f, 8.0f);
                g.setColour (juce::Colour (ledCol[k]).withAlpha (on ? 1.0f : 0.18f));
                g.fillEllipse (dot);
                if (on) { g.setColour (juce::Colour (ledCol[k]).withAlpha (0.35f)); g.drawEllipse (dot.expanded (1.5f), 1.0f); }
                mpc::text (g, ledTxt[k], cell.withTrimmedLeft (2.0f), juce::Justification::centredLeft, 11.0f, on ? 0.95f : 0.4f);
            }
        }
        mpc::text (g, (u8 (e.name) + (e.year > 0 ? " " + juce::String (e.year) : juce::String())).toUpperCase(),
                   t.withTrimmedTop (17.0f).withHeight (22.0f), juce::Justification::centredLeft, 17.0f, 1.0f, 0.04f);
        if (riddimbank::showCredits (e))
            mpc::text (g, riddimbank::creditsText (e), t.withTrimmedTop (41.0f), juce::Justification::centredLeft, 10.5f, 0.85f, 0.06f);
        else
        mpc::text (g, riddimbank::chordsText (e) + (e.harmony == 1 ? "~" : "") + u8 ("  \xc2\xb7  ")
                      + (e.bpmMeasured ? juce::String (e.bpm) + " BPM ORIGINAL" : "~" + juce::String (e.bpm) + " BPM")
                      + u8 ("  \xc2\xb7  ") + kKits[juce::jlimit (0, 3, e.kit)] + u8 ("  \xc2\xb7  ")
                      + juce::String (dubdrums::drummerName ((dubdrums::Drummer) e.drummer)).toUpperCase() + " + " + juce::String (mbm::styleName (e.bassStyle)).toUpperCase(),
                   t.withTrimmedTop (41.0f), juce::Justification::centredLeft, 10.5f, 0.85f, 0.06f);
    }
    else
    {
        mpc::text (g, u8 ("ESCOLHA UM RIDDIM  \xc2\xb7  \xe2\x97\x80 \xe2\x96\xb6"), t.withTrimmedTop (17.0f).withHeight (22.0f), juce::Justification::centredLeft, 16.0f, 0.8f);
        mpc::text (g, u8 ("BANCO \xc3\x9aNICO: ") + juce::String (riddimbank::numEntries) + u8 (" M\xc3\x9aSICAS \xc2\xb7 ") + juce::String (riddimbank::numFamilies)
                      + u8 (" RIDDIMS \xc2\xb7 HARMONIA, BPM E OS M\xc3\x9aSICOS CERTOS"), t.withTrimmedTop (41.0f), juce::Justification::centredLeft, 10.5f, 0.7f);
    }
    mpc::shine (g, l);

    auto lk = listKey.toFloat();
    mpc::key (g, lk, "", pressed == 4);
    g.setColour (col::cream);
    g.setFont (stencil (14.0f, 0.12f));
    g.drawText ("LISTA", lk.withTrimmedBottom (34.0f).translated (0, pressed == 4 ? 1.5f : 0.0f), juce::Justification::centredBottom);
    g.setFont (stencil (7.5f, 0.12f));
    g.setColour (col::cream.withAlpha (0.6f));
    g.drawText (u8 ("BANCO \xc3\x9aNICO \xe2\x96\xbe"), lk.withTrimmedTop (44.0f).withHeight (14.0f).translated (0, pressed == 4 ? 1.5f : 0.0f), juce::Justification::centred);
}

void RiddimPresetPanel::mouseDown (const juce::MouseEvent& e)
{
    const auto pt = e.getPosition();
    pressed = -1;
    for (int i = 0; i < 4; ++i)
        if (keys[i].contains (pt))
        {
            pressed = i;
            if (i < 2) stepStyle (i == 0 ? -1 : 1); else stepSong (i == 2 ? -1 : 1);
            repaint();
            return;
        }
    if (listKey.contains (pt) || lcd.contains (pt)) { pressed = 4; repaint(); showList(); }
}

void RiddimPresetPanel::mouseUp (const juce::MouseEvent&) { pressed = -1; repaint(); }

//==============================================================================
// NewLineKey
void NewLineKey::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const float a = dimmed ? 0.4f : 1.0f;
    g.setColour (juce::Colours::black.withAlpha (0.45f * a));
    g.fillRoundedRectangle (r.translated (0, 2.0f), 4.0f);
    auto cap = r.translated (0, down ? 1.5f : 0.0f).withTrimmedBottom (2.0f);
    juce::ColourGradient cg (juce::Colour (0xff2c2925), cap.getX(), cap.getY(), juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (col::amber.withAlpha ((down ? 0.9f : 0.45f) * a));
    g.fillRoundedRectangle (cap.withTop (cap.getBottom() - 3.0f).reduced (8.0f, 0.0f).translated (0, -1.5f), 1.0f);
    g.setColour (col::amber.withAlpha (a));
    g.setFont (stencil (24.0f, 0.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x86\xbb"), cap.withHeight (cap.getHeight() * 0.5f).translated (0, 4.0f), juce::Justification::centred);
    g.setColour (col::cream.withAlpha (a));
    g.setFont (stencil (10.0f, 0.12f));
    g.drawText ("NOVA LINHA", cap.withTrimmedTop (cap.getHeight() * 0.52f).withHeight (14.0f), juce::Justification::centred);
    g.setFont (stencil (7.0f, 0.1f));
    g.setColour (col::cream.withAlpha (0.55f * a));
    g.drawText (machine == MrmProcessor::Drums ? "OUTRA BATERIA" : "OUTRO BAIXO", cap.withTrimmedTop (cap.getHeight() * 0.52f + 14.0f).withHeight (12.0f), juce::Justification::centred);
}

void NewLineKey::mouseUp (const juce::MouseEvent& e)
{
    down = false;
    repaint();
    if (getLocalBounds().contains (e.getPosition())) proc.newLine (machine);
}

//==============================================================================
// DragZone
void DragZone::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const float a = dimmed ? 0.35f : 1.0f;
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r, 5.0f);
    juce::ColourGradient gl (col::glass.brighter (isMouseOver() ? 0.30f : 0.15f), r.getX(), r.getY(),
                             col::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r.reduced (3.0f), 4.0f);

    juce::Path outline, dashed;
    outline.addRoundedRectangle (r.reduced (6.0f), 3.0f);
    const float dashes[] = { 5.0f, 4.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, outline, dashes, 2);
    g.setColour (col::amber.withAlpha ((isMouseOver() ? 0.95f : 0.6f) * a));
    g.fillPath (dashed);

    // seta para baixo (para a timeline)
    const float ax = r.getCentreX(), ay = r.getY() + r.getHeight() * 0.22f;
    juce::Path arrow;
    arrow.addArrow ({ ax, ay - 7.0f, ax, ay + 8.0f }, 2.2f, 9.0f, 6.0f);
    g.setColour (col::amber.withAlpha (0.8f * a));
    g.fillPath (arrow);

    auto text = r.withTrimmedTop (r.getHeight() * 0.38f).reduced (6.0f, 4.0f);
    g.setColour (col::amber.withAlpha (a));
    g.setFont (stencil (juce::jmin (13.0f, r.getHeight() * 0.2f), 0.12f));
    g.drawFittedText (title, text.removeFromTop (text.getHeight() * 0.55f).toNearestInt(), juce::Justification::centredBottom, 2, 0.8f);
    g.setColour (col::amber.withAlpha (0.6f * a));
    g.setFont (stencil (8.5f, 0.18f));
    g.drawFittedText (sub, text.toNearestInt(), juce::Justification::centredTop, 2, 0.8f);
}

void DragZone::mouseDrag (const juce::MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("MRM-0420");
    dir.createDirectory();
    auto file = dir.getChildFile (machine < 0 ? proc.suggestedRiddimName() : proc.suggestedMachineName (machine));
    const bool ok = machine < 0 ? proc.writeRiddimFile (file) : proc.writeMachineFile (machine, file);
    if (ok)
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (file.getFullPathName()), false, this);
}

//==============================================================================
// Lane
juce::Rectangle<float> Lane::rollArea() const
{
    return getLocalBounds().toFloat().reduced (5.0f, 4.0f).withTrimmedLeft (52.0f).withTrimmedTop (12.0f);
}

juce::Rectangle<float> Lane::rowArea (int row, int numRows) const
{
    auto a = rollArea();
    const float h = a.getHeight() / (float) numRows;
    return { a.getX(), a.getY() + h * (float) row, a.getWidth(), h };
}

double Lane::beatAtX (float x, double total) const
{
    auto a = rollArea();
    return (double) ((x - a.getX()) / a.getWidth()) * total;
}

float Lane::xOfBeat (double beat, double total) const
{
    auto a = rollArea();
    return a.getX() + (float) (beat / juce::jmax (0.001, total)) * a.getWidth();
}

void Lane::paintFrame (juce::Graphics& g, double total, double intro, double playBeat, const juce::StringArray& rowNames)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    displayGlass (g, r, 4.0f, 0.10f);
    auto a = rollArea();
    if (total <= 0.0) return;

    // intro sombreada
    if (intro > 0.0)
    {
        g.setColour (col::amber.withAlpha (0.06f));
        g.fillRect (a.withRight (xOfBeat (intro, total)));
        g.setColour (col::amber.withAlpha (0.55f));
        g.setFont (stencil (8.0f, 0.15f));
        g.drawText ("INTRO", juce::Rectangle<float> (a.getX(), r.getY() + 2.0f, xOfBeat (intro, total) - a.getX(), 10.0f),
                    juce::Justification::centred);
    }

    // linhas: semicolcheia fraca, tempo médio, compasso forte com o número
    const int steps = (int) std::round (total * 4.0);
    for (int s = 0; s <= steps; ++s)
    {
        const float x = xOfBeat (s * 0.25, total);
        const bool bar = s % 16 == 0, beat = s % 4 == 0;
        g.setColour (col::amber.withAlpha (bar ? 0.38f : beat ? 0.16f : 0.05f));
        g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
        if (bar && s < steps && s * 0.25 >= intro - 1e-9)
        {
            g.setColour (col::amber.withAlpha (0.5f));
            g.setFont (stencil (8.0f, 0.1f));
            g.drawText (juce::String ((int) std::round ((s * 0.25 - intro) / 4.0) + 1), juce::Rectangle<float> (x + 2.0f, r.getY() + 2.0f, 20.0f, 10.0f),
                        juce::Justification::centredLeft);
        }
    }
    // nomes das linhas
    g.setFont (stencil (8.0f, 0.12f));
    for (int i = 0; i < rowNames.size(); ++i)
    {
        auto row = rowArea (i, rowNames.size());
        g.setColour (col::amber.withAlpha (0.06f + (i % 2) * 0.03f));
        g.fillRect (row.withLeft (r.getX() + 4.0f).withWidth (a.getX() - r.getX() - 7.0f).reduced (0, 0.5f));
        g.setColour (col::amber.withAlpha (0.75f));
        g.drawFittedText (rowNames[i], juce::Rectangle<float> (r.getX() + 5.0f, row.getY(), a.getX() - r.getX() - 9.0f, row.getHeight()).toNearestInt(),
                          juce::Justification::centredRight, 1, 0.6f);
    }
    // cursor
    if (playBeat >= 0.0 && playBeat < total)
    {
        const float x = xOfBeat (playBeat, total);
        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.drawLine (x, a.getY() - 2.0f, x, a.getBottom(), 1.4f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.35f), 4.0f);
}

//==============================================================================
// DrumLane
namespace
{
    const int kDrumRows[] = { dubdrums::gm::OpenHat, dubdrums::gm::ClosedHat, dubdrums::gm::SideStick, dubdrums::gm::Snare, dubdrums::gm::Kick };
    constexpr int kNumDrumRows = 5;
    juce::StringArray drumRowNames() { return { "HH ABERTO", "HI-HAT", "ARO", "CAIXA", "BUMBO" }; }
}

void DrumLane::update()
{
    auto& d = proc.getDrums();
    if (d.getPatternVersion() != version)
    {
        version = d.getPatternVersion();
        pattern = d.getPatternCopy();
    }
    repaint();
}

bool DrumLane::hit (juce::Point<float> p, int& row, double& beat) const
{
    auto a = rollArea();
    const double total = pattern.introLengthInBeats + pattern.lengthInBeats;
    if (! a.contains (p) || total <= 0.0) return false;
    row = juce::jlimit (0, kNumDrumRows - 1, (int) ((p.y - a.getY()) / (a.getHeight() / (float) kNumDrumRows)));
    beat = std::floor (beatAtX (p.x, total) * 4.0) / 4.0 + 0.001;   // semicolcheia clicada
    return beat >= 0.0 && beat < total;
}

void DrumLane::mouseDown (const juce::MouseEvent& e)
{
    int row; double beat;
    if (! hit (e.position, row, beat)) return;
    auto& d = proc.getDrums();
    dragAdd = ! d.noteAt (kDrumRows[row], beat);
    d.toggleNote (kDrumRows[row], beat, dragAdd);
    lastRow = row; lastBeat = beat;
    update();
}

void DrumLane::mouseDrag (const juce::MouseEvent& e)
{
    int row; double beat;
    if (! hit (e.position, row, beat)) return;
    if (row == lastRow && std::abs (beat - lastBeat) < 0.01) return;
    proc.getDrums().toggleNote (kDrumRows[row], beat, dragAdd);
    lastRow = row; lastBeat = beat;
    update();
}

void DrumLane::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    int row; double beat;
    if (! hit (e.position, row, beat) || std::abs (w.deltaY) < 1.0e-4f) return;
    proc.getDrums().changeVelocity (kDrumRows[row], beat, w.deltaY > 0 ? 8 : -8);
    update();
}

void DrumLane::paint (juce::Graphics& g)
{
    const double intro = pattern.introLengthInBeats, total = intro + pattern.lengthInBeats;
    paintFrame (g, total, intro, proc.getDrums().getPlayheadBeat(), drumRowNames());
    {
        // estilo e hi-hat da bateria (a ALAVANCA sorteia os dois): sempre à vista
        auto& d = proc.getDrums().apvts;
        const char* hats[] = { "HI-HAT 1/8", "HI-HAT 1/16", "HI-HAT CONTRATEMPO", "HI-HAT TERCINAS", "HI-HAT FLYING CYMBAL", "HI-HAT ABRE NO 1" };
        const auto style = (dubdrums::Style) juce::roundToInt (d.getRawParameterValue ("style")->load());
        const int hat = juce::jlimit (0, 5, juce::roundToInt (d.getRawParameterValue ("hatPattern")->load()));
        g.setColour (col::amber.withAlpha (0.85f));
        g.setFont (stencil (8.0f, 0.15f));
        juce::ignoreUnused (style);   // o estilo fica nas teclas ESTILO DE REGGAE
        g.drawText (hats[hat],
                    getLocalBounds().toFloat().reduced (8.0f, 3.0f).withHeight (10.0f), juce::Justification::centredRight);
    }
    if (total <= 0.0) return;
    const float cellW = xOfBeat (0.25, total) - xOfBeat (0.0, total);
    auto draw = [&] (const dubdrums::NoteEvent& e, double at)
    {
        for (int row = 0; row < kNumDrumRows; ++row)
            if (kDrumRows[row] == e.note || (row == 1 && e.note == dubdrums::gm::PedalHat))
            {
                auto ra = rowArea (row, kNumDrumRows);
                const float v = (float) e.velocity / 127.0f;
                auto cell = juce::Rectangle<float> (xOfBeat (at, total) + 0.5f, ra.getY() + 1.0f, juce::jmax (2.0f, cellW - 1.0f), ra.getHeight() - 2.0f);
                g.setColour (col::amber.withAlpha (0.25f + 0.75f * v));
                g.fillRoundedRectangle (cell, 1.5f);
            }
    };
    for (const auto& e : pattern.intro)  draw (e, e.beat);
    for (const auto& e : pattern.events) draw (e, intro + e.beat);
}

//==============================================================================
// ChordLane
void ChordLane::update()
{
    auto& c = proc.getChords();
    slots = c.getSlots();
    events = c.getCurrentLoop (loopBeats);
    repaint();
}

bool ChordLane::hit (juce::Point<float> p, int& row, double& beat) const
{
    auto a = rollArea();
    if (! a.contains (p) || loopBeats <= 0.0) return false;
    row = p.y < a.getCentreY() ? 0 : 1;
    beat = std::floor (beatAtX (p.x, loopBeats) * 4.0) / 4.0;
    return beat >= 0.0 && beat < loopBeats;
}

void ChordLane::mouseDown (const juce::MouseEvent& e)
{
    int row; double step;
    if (! hit (e.position, row, step)) return;
    const int ch = row == 0 ? dub::ChSkank : dub::ChBubble;
    auto inCell = [&] (const dub::NoteEvent& ev) { return ev.channel == ch && ev.start >= step - 1.0e-6 && ev.start < step + 0.25 - 1.0e-6; };

    auto ev = events;
    const auto before = ev.size();
    ev.erase (std::remove_if (ev.begin(), ev.end(), inCell), ev.end());
    if (ev.size() == before)
    {
        // acorde que soa nesse tempo
        int slot = -1; double pos = 0.0;
        for (int i = 0; i < (int) slots.size(); ++i)
        {
            if (slots[(size_t) i].beats <= 0) continue;
            if (step >= pos - 1.0e-9 && step < pos + slots[(size_t) i].beats) { slot = i; break; }
            pos += slots[(size_t) i].beats;
        }
        if (slot < 0) return;

        // copia o ataque mais próximo do mesmo acorde (mesmas notas, tamanho e força)
        double bestStart = -1.0, bestDist = 1.0e9;
        int bestSlot = -1;
        for (const auto& x : events)
        {
            if (x.channel != ch) continue;
            const double d = std::abs (x.start - step) + (x.slot == slot ? 0.0 : 1000.0);
            if (d < bestDist) { bestDist = d; bestStart = x.start; bestSlot = x.slot; }
        }
        if (bestStart < 0.0) return;   // sem modelo (ex.: bubble desligado)
        int shift = 0;
        if (bestSlot != slot && bestSlot >= 0)
        {
            shift = ((slots[(size_t) slot].root - slots[(size_t) bestSlot].root) % 12 + 12) % 12;
            if (shift > 6) shift -= 12;
        }
        for (const auto& x : events)
            if (x.channel == ch && std::abs (x.start - bestStart) < 1.0e-6)
            {
                auto n = x;
                n.start = step;
                n.slot = slot;
                n.note = juce::jlimit (0, 127, x.note + shift);
                ev.push_back (n);
            }
    }
    proc.getChords().setEdits (std::move (ev), loopBeats);
    update();
}

void ChordLane::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    int row; double step;
    if (! hit (e.position, row, step) || std::abs (w.deltaY) < 1.0e-4f) return;
    const int ch = row == 0 ? dub::ChSkank : dub::ChBubble;
    auto ev = events;
    bool changed = false;
    for (auto& x : ev)
        if (x.channel == ch && x.start >= step - 1.0e-6 && x.start < step + 0.25 - 1.0e-6)
        {
            x.velocity = juce::jlimit (1, 127, x.velocity + (w.deltaY > 0 ? 8 : -8));
            changed = true;
        }
    if (changed) { proc.getChords().setEdits (std::move (ev), loopBeats); update(); }
}

void ChordLane::paint (juce::Graphics& g)
{
    paintFrame (g, loopBeats, 0.0, proc.getChords().getPlayPositionInLoop(), { "SKANK", "BUBBLE" });
    if (loopBeats <= 0.0) return;

    // nomes dos acordes na régua
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    double pos = 0.0;
    g.setFont (stencil (8.5f, 0.1f));
    for (const auto& s : slots)
    {
        if (s.beats <= 0) continue;
        const float x0 = xOfBeat (pos, loopBeats), x1 = xOfBeat (pos + s.beats, loopBeats);
        g.setColour (col::amber.withAlpha (0.9f));
        g.drawText (juce::String (dub::chordLabel (s)), juce::Rectangle<float> (x0 + 14.0f, r.getY() + 2.0f, juce::jmax (10.0f, x1 - x0 - 16.0f), 10.0f),
                    juce::Justification::centredLeft);
        pos += s.beats;
    }

    // ataques: um bloco por ataque, com as notas do acorde riscadas dentro
    const float stepW = xOfBeat (0.25, loopBeats) - xOfBeat (0.0, loopBeats);
    for (int row = 0; row < 2; ++row)
    {
        const int ch = row == 0 ? dub::ChSkank : dub::ChBubble;
        auto ra = rowArea (row, 2);
        int lo = 127, hi = 0;
        for (const auto& e : events) if (e.channel == ch) { lo = juce::jmin (lo, e.note); hi = juce::jmax (hi, e.note); }
        for (const auto& e : events)
        {
            if (e.channel != ch) continue;
            const float x = xOfBeat (e.start, loopBeats);
            const float w = juce::jmax (2.5f, juce::jmin (stepW * 0.9f, xOfBeat (e.start + e.length, loopBeats) - x + 2.0f));
            auto cell = juce::Rectangle<float> (x + 0.5f, ra.getY() + 1.5f, w, ra.getHeight() - 3.0f);
            g.setColour (col::amber.withAlpha (0.10f + 0.25f * (float) e.velocity / 127.0f));
            g.fillRoundedRectangle (cell, 1.5f);
            const float t = hi > lo ? (float) (e.note - lo) / (float) (hi - lo) : 0.5f;
            const float y = cell.getBottom() - 2.0f - t * (cell.getHeight() - 4.0f);
            g.setColour (col::amber.withAlpha (0.45f + 0.55f * (float) e.velocity / 127.0f));
            g.fillRect (cell.getX(), y - 1.0f, w, 2.0f);
        }
    }
}

//==============================================================================
// JackpotReels
JackpotReels::JackpotReels (juce::RangedAudioParameter& p)
    : param (p), att (p, [this] (float v) { value = juce::roundToInt (v); repaint(); }, nullptr)
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

bool JackpotReels::animating() const { return juce::Time::getMillisecondCounterHiRes() - animStart < 1400.0; }

void JackpotReels::mouseDown (const juce::MouseEvent&) { dragStart = value; att.beginGesture(); }
void JackpotReels::mouseUp (const juce::MouseEvent&)   { att.endGesture(); }
void JackpotReels::mouseDrag (const juce::MouseEvent& e)
{
    att.setValueAsPartOfGesture ((float) juce::jlimit (1, 999, dragStart - e.getDistanceFromDragStartY() / 3));
}
void JackpotReels::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const int step = w.deltaY > 0 ? 1 : w.deltaY < 0 ? -1 : 0;
    if (step != 0) att.setValueAsCompleteGesture ((float) juce::jlimit (1, 999, value + step));
}

void JackpotReels::paint (juce::Graphics& g)
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (shown < 0) shown = value;
    if (value != shown)
    {
        if (! isMouseButtonDown()) animStart = now;   // arrastando a semente na mão: sem animação
        shown = value;
    }
    const double t = now - animStart;

    auto r = getLocalBounds().toFloat();
    // moldura cromada
    juce::ColourGradient chrome (juce::Colour (0xfff2f0ea), r.getX(), r.getY(), juce::Colour (0xff6e6a62), r.getX(), r.getBottom(), false);
    chrome.addColour (0.5, juce::Colour (0xffb9b5ac));
    g.setGradientFill (chrome);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

    auto inner = r.reduced (8.0f);
    g.setColour (juce::Colour (0xff120d09));
    g.fillRoundedRectangle (inner, 5.0f);

    const auto digits = juce::String (value).paddedLeft ('0', 3);
    const float gap = 6.0f;
    const float cw = (inner.getWidth() - 2.0f * gap - 12.0f) / 3.0f;
    for (int k = 0; k < 3; ++k)
    {
        auto cell = juce::Rectangle<float> (inner.getX() + 6.0f + (float) k * (cw + gap), inner.getY() + 6.0f, cw, inner.getHeight() - 12.0f);
        // rolo cilíndrico: claro no meio, escuro nas bordas
        juce::ColourGradient drum (juce::Colour (0xff2a1f14), cell.getX(), cell.getY(), juce::Colour (0xff2a1f14), cell.getX(), cell.getBottom(), false);
        drum.addColour (0.5, juce::Colour (0xff4a3826));
        g.setGradientFill (drum);
        g.fillRoundedRectangle (cell, 3.0f);

        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (cell.toNearestInt());
        g.setFont (stencil (cell.getHeight() * 0.62f, 0.0f));
        auto drawDigit = [&] (const juce::String& d, float dy, float a)
        {
            glowText (g, d, cell.translated (0, dy), juce::Justification::centred, col::amber, a);
        };
        const double stopAt = 520.0 + 280.0 * k;
        if (t < stopAt)
        {
            const double pos = t / 34.0 + k * 3.7;          // rolos girando para cima
            const int d1 = ((int) pos * 7 + k * 3) % 10, d2 = ((int) pos * 7 + 7 + k * 3) % 10;
            const float frac = (float) (pos - std::floor (pos));
            drawDigit (juce::String (d1), -frac * cell.getHeight(), 0.85f);
            drawDigit (juce::String (d2), (1.0f - frac) * cell.getHeight(), 0.85f);
        }
        else
        {
            const float settle = (float) juce::jmin (1.0, (t - stopAt) / 110.0);   // tranco ao parar
            drawDigit (digits.substring (k, k + 1), (1.0f - settle) * -6.0f, 1.0f);
        }
        // sombra cilíndrica por cima dos números
        juce::ColourGradient shade (juce::Colours::black.withAlpha (0.65f), cell.getX(), cell.getY(), juce::Colours::transparentBlack, cell.getX(), cell.getCentreY(), false);
        g.setGradientFill (shade);
        g.fillRect (cell.withHeight (cell.getHeight() * 0.5f));
        juce::ColourGradient shade2 (juce::Colours::transparentBlack, cell.getX(), cell.getCentreY(), juce::Colours::black.withAlpha (0.65f), cell.getX(), cell.getBottom(), false);
        g.setGradientFill (shade2);
        g.fillRect (cell.withTop (cell.getCentreY()));
    }
    // linha de pagamento vermelha
    g.setColour (col::jamRed.withAlpha (0.7f));
    g.drawLine (inner.getX() + 2.0f, inner.getCentreY(), inner.getRight() - 2.0f, inner.getCentreY(), 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRoundedRectangle (inner.withHeight (inner.getHeight() * 0.3f), 5.0f);
}

//==============================================================================
// JackpotButton
void JackpotButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const float pulse = 0.5f + 0.5f * (float) std::sin (phase * 5.0);

    // caixa preta com lâmpadas de cassino em volta
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (0, 3.0f), 12.0f);
    juce::ColourGradient box (juce::Colour (0xff2b2622), r.getX(), r.getY(), juce::Colour (0xff0e0c0a), r.getX(), r.getBottom(), false);
    g.setGradientFill (box);
    g.fillRoundedRectangle (r, 12.0f);

    juce::Path ring;
    ring.addRoundedRectangle (r.reduced (8.0f), 8.0f);
    const int bulbs = 26;
    const int chase = (int) (phase * (spinning ? 22.0 : 9.0));
    for (int i = 0; i < bulbs; ++i)
    {
        const auto p = ring.getPointAlongPath (ring.getLength() * (float) i / (float) bulbs);
        const bool lit = ((i + chase) % 3) == 0;
        const auto bulb = juce::Rectangle<float> (7.0f, 7.0f).withCentre (p);
        if (lit)
        {
            g.setColour (col::jamGold.withAlpha (0.35f));
            g.fillEllipse (bulb.expanded (4.0f));
        }
        juce::ColourGradient bg (lit ? juce::Colour (0xfffff3c0) : juce::Colour (0xff6a5326), bulb.getCentreX(), bulb.getCentreY(),
                                 lit ? col::jamGold : juce::Colour (0xff3a2d16), bulb.getRight(), bulb.getBottom(), true);
        g.setGradientFill (bg);
        g.fillEllipse (bulb);
    }

    // tampa vermelha grande (botão de fliperama), com brilho pulsando
    auto cap = r.reduced (20.0f, 18.0f);
    g.setColour (col::jamRed.withAlpha (0.18f + 0.30f * pulse));
    g.fillRoundedRectangle (cap.expanded (5.0f), cap.getHeight() * 0.5f);
    g.setColour (juce::Colour (0xff1a0806));
    g.fillRoundedRectangle (cap.translated (0, 4.0f), cap.getHeight() * 0.5f);
    cap = cap.translated (0, down ? 3.0f : 0.0f);
    const bool over = isMouseOver();
    juce::ColourGradient rg (juce::Colour (down ? 0xffb2321e : over ? 0xffff6a4a : 0xffee4a30), cap.getCentreX(), cap.getY(),
                             juce::Colour (down ? 0xff5c140b : 0xff8e1c0f), cap.getCentreX(), cap.getBottom(), false);
    g.setGradientFill (rg);
    g.fillRoundedRectangle (cap, cap.getHeight() * 0.5f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillRoundedRectangle (cap.reduced (cap.getHeight() * 0.25f, 3.0f).withHeight (cap.getHeight() * 0.3f), cap.getHeight() * 0.15f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (cap, cap.getHeight() * 0.5f, 1.2f);

    auto txt = cap.reduced (10.0f, 6.0f);
    g.setFont (stencil (juce::jmin (25.0f, txt.getHeight() * 0.42f, txt.getWidth() * 0.115f), 0.08f));
    g.setColour (juce::Colour (0xff4a0d05));
    g.drawText ("NOVO RIDDIM", txt.withTrimmedBottom (txt.getHeight() * 0.3f).translated (0, 1.5f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xfffff3d6));
    g.drawText ("NOVO RIDDIM", txt.withTrimmedBottom (txt.getHeight() * 0.3f), juce::Justification::centred, false);
    g.setFont (stencil (9.5f, 0.3f));
    g.setColour (col::jamGold.withAlpha (0.95f));
    g.drawText (spinning ? "GIRANDO..." : "NOVA SEMENTE", txt.withTrimmedTop (txt.getHeight() * 0.68f), juce::Justification::centred);
}

//==============================================================================
// JackpotLever
void JackpotLever::mouseDown (const juce::MouseEvent&)
{
    if (state == State::Going || state == State::Returning) return;
    state = State::Dragging;
    dragStartPull = pull;
    fired = false;
}

void JackpotLever::mouseDrag (const juce::MouseEvent& e)
{
    if (state != State::Dragging) return;
    pull = juce::jlimit (0.0f, 1.0f, dragStartPull + (float) e.getDistanceFromDragStartY() / ((float) getHeight() * 0.6f));
    if (pull >= 0.98f && ! fired) { fired = true; if (onPull) onPull(); }
    repaint();
}

void JackpotLever::mouseUp (const juce::MouseEvent&)
{
    if (state != State::Dragging) return;
    state = fired ? State::Returning : State::Going;   // clique (ou puxada curta): a alavanca desce sozinha
}

void JackpotLever::tick (double dt)
{
    const float before = pull;
    if (state == State::Going)
    {
        pull = juce::jmin (1.0f, pull + (float) dt * 6.0f);
        if (pull >= 1.0f) { if (! fired && onPull) onPull(); fired = true; state = State::Returning; }
    }
    else if (state == State::Returning)
    {
        pull = juce::jmax (0.0f, pull - (float) dt * 3.2f);
        if (pull <= 0.0f) state = State::Idle;
    }
    if (pull != before) repaint();
}

void JackpotLever::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float cx = r.getCentreX();
    // caixa da alavanca (cromada) presa no lado da máquina
    auto base = juce::Rectangle<float> (juce::jmin (r.getWidth() - 10.0f, 76.0f), 44.0f).withCentre ({ cx, r.getBottom() - 26.0f });
    juce::ColourGradient chrome (juce::Colour (0xfff2f0ea), base.getX(), base.getY(), juce::Colour (0xff5e5a52), base.getX(), base.getBottom(), false);
    chrome.addColour (0.5, juce::Colour (0xffbab6ad));
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (base.translated (1.5f, 3.0f), 8.0f);
    g.setGradientFill (chrome);
    g.fillRoundedRectangle (base, 8.0f);
    auto slot = juce::Rectangle<float> (10.0f, base.getHeight() - 12.0f).withCentre (base.getCentre());
    g.setColour (juce::Colour (0xff141210));
    g.fillRoundedRectangle (slot, 4.0f);

    // haste: gira na frente da caixa (em cima -> para a frente/baixo)
    const float pivotY = base.getY() + 10.0f;
    const float L = pivotY - r.getY() - 20.0f;
    const float ballY = pivotY - L + pull * (L + 18.0f);
    const float ballR = 17.0f + pull * 5.0f;   // a bola chega mais perto quando desce
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawLine (cx + 2.0f, pivotY + 2.0f, cx + 2.0f, ballY + 2.0f, 6.0f);
    juce::ColourGradient rod (juce::Colour (0xff8d897f), cx - 3.0f, 0, juce::Colour (0xfff5f3ee), cx + 1.0f, 0, false);
    rod.addColour (1.0, juce::Colour (0xff6e6a62));
    g.setGradientFill (rod);
    g.fillRect (juce::Rectangle<float> (cx - 3.0f, juce::jmin (pivotY, ballY), 6.0f, std::abs (ballY - pivotY)));
    g.setColour (juce::Colour (0xff3a3632));
    g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ cx, pivotY }));

    // bola vermelha brilhante
    auto ball = juce::Rectangle<float> (ballR * 2.0f, ballR * 2.0f).withCentre ({ cx, ballY });
    g.setColour (col::jamRed.withAlpha (isMouseOver() ? 0.45f : 0.25f));
    g.fillEllipse (ball.expanded (5.0f));
    juce::ColourGradient bg (juce::Colour (0xffff8a6a), ball.getX() + ballR * 0.6f, ball.getY() + ballR * 0.5f,
                             juce::Colour (0xff7e1a0e), ball.getRight(), ball.getBottom(), true);
    g.setGradientFill (bg);
    g.fillEllipse (ball);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.fillEllipse (juce::Rectangle<float> (ballR * 0.7f, ballR * 0.45f).withCentre ({ ball.getX() + ballR * 0.75f, ball.getY() + ballR * 0.55f }));
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawEllipse (ball, 1.0f);
}

//==============================================================================
// GrilleSpeaker
void GrilleSpeaker::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);

    // gabinete de madeira com cantos arredondados
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (2.0f, 4.0f), 14.0f);
    juce::ColourGradient wood (juce::Colour (0xff7a4a22), r.getX(), r.getY(), juce::Colour (0xff2e170a), r.getRight(), r.getBottom(), false);
    wood.addColour (0.45, juce::Colour (0xff5a3316));
    g.setGradientFill (wood);
    g.fillRoundedRectangle (r, 14.0f);
    juce::Random rng (59);
    for (int i = 0; i < 26; ++i)
    {
        const float y = r.getY() + rng.nextFloat() * r.getHeight();
        g.setColour (juce::Colours::black.withAlpha (0.08f + rng.nextFloat() * 0.1f));
        g.drawLine (r.getX() + 6.0f, y, r.getRight() - 6.0f, y + (rng.nextFloat() - 0.5f) * 6.0f, 0.6f + rng.nextFloat());
    }
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (r.reduced (1.5f), 13.0f, 1.0f);

    // moldura cromada da grade
    auto frame = r.reduced (14.0f, 12.0f).withTrimmedBottom (22.0f);
    juce::ColourGradient chrome (juce::Colour (0xfff2f0ea), frame.getX(), frame.getY(), juce::Colour (0xff5e5a52), frame.getX(), frame.getBottom(), false);
    chrome.addColour (0.5, juce::Colour (0xffbab6ad));
    g.setGradientFill (chrome);
    g.fillRoundedRectangle (frame, 8.0f);
    auto hole = frame.reduced (4.0f);

    // cone atrás da grade, mexendo com o som
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (hole.toNearestInt());
        g.setColour (juce::Colour (0xff0d0b09));
        g.fillRect (hole);
        const float push = juce::jlimit (0.0f, 1.0f, level);
        const auto c = hole.getCentre();
        const float rad = juce::jmin (hole.getWidth(), hole.getHeight()) * 0.48f;
        juce::ColourGradient cone (juce::Colour (0xff5a4c3c).brighter (0.35f * push), c.x, c.y, juce::Colour (0xff0f0d0b), c.x + rad, c.y, true);
        g.setGradientFill (cone);
        g.fillEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (c));
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        for (float k : { 0.82f, 0.62f, 0.45f })
            g.drawEllipse (juce::Rectangle<float> (rad * 2.0f * k, rad * 2.0f * k).withCentre (c), 1.0f);
        const float dome = rad * (0.24f + 0.05f * push);
        juce::ColourGradient dg (juce::Colour (0xff6a5d4c).brighter (0.4f * push), c.x - dome * 0.3f, c.y - dome * 0.4f,
                                 juce::Colour (0xff1c1915), c.x + dome, c.y + dome, true);
        g.setGradientFill (dg);
        g.fillEllipse (juce::Rectangle<float> (dome * 2.0f, dome * 2.0f).withCentre (c));
    }

    // grade de metal perfurada (desenhada uma vez; os furos deixam ver o cone)
    if (! grille.isValid() || grille.getWidth() != juce::roundToInt (hole.getWidth() * 2.0f))
    {
        grille = juce::Image (juce::Image::ARGB, juce::roundToInt (hole.getWidth() * 2.0f), juce::roundToInt (hole.getHeight() * 2.0f), true);
        juce::Graphics gg (grille);
        gg.addTransform (juce::AffineTransform::scale (2.0f));
        const float w = hole.getWidth(), h = hole.getHeight();
        juce::Path sheet;
        sheet.addRectangle (0.0f, 0.0f, w, h);
        juce::Path holes;
        const float pitch = 6.4f, hr = 2.2f;
        int row = 0;
        for (float y = pitch * 0.6f; y < h - 1.0f; y += pitch * 0.866f, ++row)
            for (float x = (row % 2 ? pitch * 0.5f : 0.0f) + pitch * 0.6f; x < w - 1.0f; x += pitch)
                holes.addEllipse (x - hr, y - hr, hr * 2.0f, hr * 2.0f);
        sheet.addPath (holes);
        sheet.setUsingNonZeroWinding (false);
        juce::ColourGradient metal (juce::Colour (0xffd9d6cf), 0, 0, juce::Colour (0xff7d7870), 0, h, false);
        metal.addColour (0.35, juce::Colour (0xffc3bfb6));
        metal.addColour (0.7,  juce::Colour (0xff9e998f));
        gg.setGradientFill (metal);
        gg.fillPath (sheet);
        gg.setColour (juce::Colours::black.withAlpha (0.35f));   // sombra dentro de cada furo
        gg.strokePath (holes, juce::PathStrokeType (0.5f), juce::AffineTransform::translation (0.0f, 0.4f));
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.25f), 0, 0, juce::Colours::transparentWhite, w * 0.6f, h, false);
        gg.setGradientFill (sheen);
        gg.fillPath (sheet);
    }
    g.drawImage (grille, hole);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawRoundedRectangle (frame, 8.0f, 1.0f);

    // plaquinha DUBSTATION embaixo
    auto badge = juce::Rectangle<float> (120.0f, 15.0f).withCentre ({ r.getCentreX(), r.getBottom() - 15.0f });
    juce::ColourGradient bb (juce::Colour (0xffe8d390), badge.getX(), badge.getY(), juce::Colour (0xff86692a), badge.getX(), badge.getBottom(), false);
    g.setGradientFill (bb);
    g.fillRoundedRectangle (badge, 3.0f);
    g.setColour (juce::Colour (0xff2a1d06));
    g.setFont (stencil (9.5f, 0.32f));
    g.drawText ("DUBSTATION", badge, juce::Justification::centred);
}

//==============================================================================
// MainPage
MainPage::MainPage (MrmProcessor& p)
    : proc (p), drumLane (p), chordLane (p), bassRoll (p.getBass()), bassLane (p, bassRoll),
      reels (*p.apvts.getParameter ("seed")),
      riddimDrag (p, -1, "ARRASTE O RIDDIM", u8 ("TUDO ALINHADO")), bpmPanel (p), presetPanel (p)
{
    setLookAndFeel (&lnf);
    auto& c = proc.getChords();

    // --- harmonia: os controles do MRCM-1240 (valem também para o baixo do MRBM-69)
    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
    {
        auto& s = strips[(size_t) i];
        const juce::String n (i);
        s.keys = std::make_unique<NoteKeys> (*c.apvts.getParameter ("root" + n));
        s.mode = std::make_unique<ModeSwitch> (*c.apvts.getParameter ("qual" + n));
        s.inv  = std::make_unique<InversionSwitch> (*c.apvts.getParameter ("inv" + n),
                                                    [this, i] { return voicings.suggested.empty() ? 0 : voicings.suggested[(size_t) i]; });
        s.dur  = std::make_unique<ChoiceWindow> (*c.apvts.getParameter ("dur" + n), [] (int d) -> std::pair<juce::String, juce::String>
                 { return { dub::durationShort (d), d <= 1 ? "COMPASSO" : "COMPASSOS" }; });
        for (juce::Component* comp : { (juce::Component*) s.keys.get(), (juce::Component*) s.mode.get(),
                                       (juce::Component*) s.inv.get(), (juce::Component*) s.dur.get() })
            addAndMakeVisible (comp);
        if (i > 0)
        {
            s.on.setButtonText ("ON");
            addAndMakeVisible (s.on);
            s.onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (c.apvts, "on" + n, s.on);
        }
    }
    voicings = c.getVoicings();

    // --- máquinas
    auto& d = proc.getDrums();
    auto& b = proc.getBass();
    rows[MrmProcessor::Drums].musicians  = std::make_unique<MusicianKeys> (*d.apvts.getParameter ("drummer"),
        juce::StringArray { "STYLE SCOTT", "SANTA DAVIS", "CARLTON BARRETT", "SLY DUNBAR", "DIGIKAL" }, 3);
    // harmonia: o skank é sempre no contratempo (sem seletor; a janela fixa é desenhada no paint)
    // bateria: o estilo de reggae (com o bumbo e a caixa dele, como no MRDM-12)
    drumStyle = std::make_unique<MusicianKeys> (*d.apvts.getParameter ("style"), juce::StringArray { "ROCKERS", "ONE DROP", "BOOM BAP", "DEMBOW" }, 4);
    drumStyle->onSelect = [this] (int st) { proc.getDrums().applyStyleDefaults (st); };
    addAndMakeVisible (*drumStyle);
    {
        juce::StringArray bassists;
        for (int s = 0; s < mbm::NumStyles; ++s) bassists.add (mbm::styleName (s));
        rows[MrmProcessor::Bass].musicians = std::make_unique<MusicianKeys> (*b.apvts.getParameter ("style"), bassists, 4);
    }
    // baixo: o timbre do MRBM-69 (os do MRDBI-07), com o SUPER TONE do Sleng Teng
    bassTimbre = std::make_unique<MusicianKeys> (*b.apvts.getParameter ("timbre"), juce::StringArray { "ROOTS", "SUPER TONE", "808" }, 3);
    addAndMakeVisible (*bassTimbre);
    rows[MrmProcessor::Drums].lane  = &drumLane;
    rows[MrmProcessor::Chords].lane = &chordLane;
    rows[MrmProcessor::Bass].lane   = &bassLane;
    const char* dragNames[] = { "ARRASTE A BATERIA", "ARRASTE A HARMONIA", "ARRASTE O BAIXO" };
    for (int m = 0; m < MrmProcessor::NumMachines; ++m)
    {
        auto& row = rows[(size_t) m];
        row.power = std::make_unique<PowerSwitch> (*proc.apvts.getParameter (MrmProcessor::machineOnId (m)), "LIGADA", "DESLIGADA");
        row.drag  = std::make_unique<DragZone> (proc, m, dragNames[m], "SO ESTA MAQUINA");
        addAndMakeVisible (*row.power);
        if (row.musicians) addAndMakeVisible (*row.musicians);
        addAndMakeVisible (*row.lane);
        addAndMakeVisible (*row.drag);
        addChildComponent (row.undo);
        row.undo.onClick = [this, m]
        {
            if (m == MrmProcessor::Drums)       proc.getDrums().clearEdits();
            else if (m == MrmProcessor::Chords) proc.getChords().clearEdits();
            else                                 proc.getBass().clearEdits();
        };
    }

    // --- JACKPOT
    addAndMakeVisible (reels);
    addAndMakeVisible (jackpot);
    jackpot.onClick = [this] { proc.newRiddim(); };
    addAndMakeVisible (lever);
    lever.onPull = [this] { proc.randomRiddim(); };
    presets.onClick = [this] { showPresets(); };
    addAndMakeVisible (presets);

    // --- RIDDIM
    length = std::make_unique<ChoiceWindow> (*proc.apvts.getParameter ("riddimBars"), [] (int i) -> std::pair<juce::String, juce::String>
    {
        return { juce::String (MrmProcessor::lengthBars (i)), "COMP." };
    });
    addAndMakeVisible (*length);
    addAndMakeVisible (riddimDrag);
    exportBtn.onClick = [this] { exportRiddim(); };
    addAndMakeVisible (exportBtn);
    // --- BIG KNOB · CONTROL ROOM
    auto& k = proc.getKnob().apvts;
    bigKnob = std::make_unique<BigDial> (*k.getParameter ("knob"));
    addAndMakeVisible (*bigKnob);
    filterMode = std::make_unique<MusicianKeys> (*k.getParameter ("mode"), juce::StringArray { "HIGH PASS", "LOW PASS" }, 2);
    addAndMakeVisible (*filterMode);
    knobBypass = std::make_unique<PowerSwitch> (*k.getParameter ("bypass"), "BYPASS", "BYPASS");
    addAndMakeVisible (*knobBypass);
    // --- DUB SIREN (a sirene do Mastor Big Knob)
    // 2.7: o FIRE e o HOLD são do MRM (a sirene passa pelo Mastor Tape Echo MTE-12); timbre e nível continuam no Big Knob
    sirenFire = std::make_unique<SirenFire> (*proc.apvts.getParameter ("sirenFire"));
    sirenHold = std::make_unique<PowerSwitch> (*proc.apvts.getParameter ("sirenHold"), "HOLD", "HOLD");
    sirenEcho = std::make_unique<PowerSwitch> (*proc.apvts.getParameter ("sirEchoOn"), "TAPE ECHO", "TAPE ECHO");
    addAndMakeVisible (*sirenEcho);
    sirenWave = std::make_unique<MusicianKeys> (*k.getParameter ("sirWave"),
                    juce::StringArray { "SENO", juce::String::fromUTF8 ("TRI\xc3\x82NG."), "QUADRADA", "SOBE", "DESCE" }, 5);
    const char* sirenIds[] = { "sirPitch", "sirRate", "sirDepth", "sirEchoRate", "sirEchoInt", "sirLevel" };
    for (size_t i = 0; i < sirenDials.size(); ++i)
    {
        auto& st = (i == 3 || i == 4) ? proc.apvts : k;   // REPEAT RATE e INTENSITY: do Tape Echo da sirene
        sirenDials[i] = std::make_unique<BigDial> (*st.getParameter (sirenIds[i]));
        addAndMakeVisible (*sirenDials[i]);
    }
    for (juce::Component* c : { (juce::Component*) sirenFire.get(), (juce::Component*) sirenHold.get(), (juce::Component*) sirenWave.get() })
        addAndMakeVisible (*c);
    muteDrums  = std::make_unique<MuteKey> (*proc.apvts.getParameter (MrmProcessor::muteId (MrmProcessor::Drums)),  "DRUMS",  "MRD-12",    juce::Colour (0xff2fbf55));
    muteBass   = std::make_unique<MuteKey> (*proc.apvts.getParameter (MrmProcessor::muteId (MrmProcessor::Bass)),   "BASS",   "MRB-69",    col::jamGold);
    muteRiddim = std::make_unique<MuteKey> (*proc.apvts.getParameter (MrmProcessor::muteId (MrmProcessor::Chords)), "RIDDIM", "MRK-80",    juce::Colour (0xffe8432c));
    for (auto* m : { muteDrums.get(), muteBass.get(), muteRiddim.get() }) addAndMakeVisible (*m);
    fxPower = std::make_unique<PowerSwitch> (*proc.apvts.getParameter ("fxOn"), "CONTROL ROOM", "CONTROL ROOM");
    echoPower = std::make_unique<PowerSwitch> (*proc.getKnob().apvts.getParameter ("echoOn"), "TAPE ECHO", "TAPE ECHO");
    addAndMakeVisible (*fxPower);
    addAndMakeVisible (*echoPower);
    addAndMakeVisible (speaker);
    addAndMakeVisible (bpmPanel);
    addAndMakeVisible (presetPanel);
    newDrums = std::make_unique<NewLineKey> (proc, MrmProcessor::Drums);
    newBass  = std::make_unique<NewLineKey> (proc, MrmProcessor::Bass);
    addAndMakeVisible (*newDrums);
    addAndMakeVisible (*newBass);

    setSize (kW, kH);
    startTimerHz (30);
}

MainPage::~MainPage()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MainPage::exportRiddim()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("MRM-0420");
    dir.createDirectory();
    chooser = std::make_unique<juce::FileChooser> ("Salvar o riddim em MIDI", dir.getChildFile (proc.suggestedRiddimName()), "*.mid");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              if (! f.hasFileExtension ("mid")) f = f.withFileExtension ("mid");
                              if (! proc.writeRiddimFile (f))
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "MRM-0420",
                                                                          u8 ("N\xc3\xa3o foi poss\xc3\xadvel salvar o MIDI."));
                          });
}

void MainPage::showPresets()
{
    juce::PopupMenu menu;
    riddimbank::addMenus (menu, proc.currentBankRiddim());   // BANCO ÚNICO: o mesmo menu de todas as machines
    juce::Component::SafePointer<MainPage> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presets),
                        [safe] (int r)
                        {
                            if (safe == nullptr || r == 0) return;
                            if (r == riddimbank::toggleCompleteId) { riddimbank::onlyComplete() = ! riddimbank::onlyComplete(); safe->showPresets(); return; }
                            if (const int e = riddimbank::entryFromMenuResult (r); e >= 0) safe->proc.applyBankRiddim (e);
                        });
}

void MainPage::timerCallback()
{
    // lanes e visores
    for (auto& row : rows) if (row.lane != nullptr && isVisible()) row.lane->update();
    proc.getBass().dropStaleEdits();
    voicings = proc.getChords().getVoicings();
    repaint (harmonySection);

    const bool edits[MrmProcessor::NumMachines] = { proc.getDrums().hasEdits(), proc.getChords().hasEdits(), proc.getBass().hasEdits() };
    for (int m = 0; m < MrmProcessor::NumMachines; ++m)
    {
        auto& row = rows[(size_t) m];
        row.undo.setVisible (edits[m]);
        const bool dim = ! proc.isMachineOn (m);
        if (row.drag->dimmed != dim) { row.drag->dimmed = dim; row.drag->repaint(); }
        if (auto* nk = m == MrmProcessor::Drums ? newDrums.get() : m == MrmProcessor::Bass ? newBass.get() : nullptr)
            if (nk->dimmed != dim) { nk->dimmed = dim; nk->repaint(); }
        row.lane->setAlpha (dim ? 0.45f : 1.0f);
    }

    // JACKPOT: luzes sempre correndo; rolos girando depois do NOVO RIDDIM; alavanca com mola
    lever.tick (1.0 / 30.0);
    jackpot.phase += 1.0 / 30.0;
    jackpot.spinning = reels.animating();
    jackpot.repaint();
    reels.repaint();

    speaker.level = speaker.level * 0.6f + proc.getOutputLevel() * 0.4f;
    speaker.repaint();
}

void MainPage::resized()
{
    woodL = { 0, 0, 28, kH };
    woodR = { kW - 28, 0, 28, kH };
    plate = { 28, 0, kW - 56, kH };

    harmonySection = { 52, 22, 1176, 272 };
    machineSection = { 52, 316, 1176, 330 };
    knobSection    = { 52, 668, 1176, 130 };
    sirenSection   = { 52, 820, 1176, 122 };
    presetBox      = { 52, 964, 1176, 100 };
    bpmBox         = { 52, 1086, 1176, 136 };
    constexpr int D = 414;   // o que vem depois da sirene, dos PRESETS e do BPM desce
    jackpotSection = { 52, 820 + D, 520, 162 };
    riddimSection  = { 592, 820 + D, 386, 162 };
    speakerArea    = { 992, 808 + D, 236, 186 };

    // harmonia: 4 acordes
    const int sw = (1176 - 24 - 36) / 4;
    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
    {
        auto& s = strips[(size_t) i];
        const int x = 64 + i * (sw + 12), y = 38;
        s.area = { x, y, sw, 250 };
        s.display = { x, y, sw - (i > 0 ? 74 : 0), 30 };
        if (i > 0) s.on.setBounds (x + sw - 68, y + 2, 68, 26);
        s.keys->setBounds (x, y + 40, sw, 92);
        s.mode->setBounds (x, y + 140, sw, 30);
        s.inv->setBounds (x, y + 178, sw, 30);
        s.dur->setBounds (x + 24, y + 216, sw - 48, 32);
    }

    // máquinas
    for (int m = 0; m < MrmProcessor::NumMachines; ++m)
    {
        auto& row = rows[(size_t) m];
        const int slot = m == MrmProcessor::Drums ? 0 : m == MrmProcessor::Bass ? 1 : 2;   // bateria, baixo, riddim
        const int y = 330 + slot * 104;
        row.area = { 64, y, 1152, 96 };
        row.plateArea = { 64, y, 160, 96 };
        row.power->setBounds (70, y + 42, 148, 28);
        row.undo.setBounds (70, y + 74, 148, 18);
        if (row.musicians)
            row.musicians->setBounds (m != MrmProcessor::Chords ? juce::Rectangle<int> (234, y + 46, 290, 48) : juce::Rectangle<int> (234, y + 16, 290, 78));
        const bool newKey = m != MrmProcessor::Chords;   // NOVA LINHA na bateria e no baixo
        row.lane->setBounds (534, y, newKey ? 446 : 550, 96);
        if (newKey) (m == MrmProcessor::Drums ? newDrums : newBass)->setBounds (986, y + 4, 100, 88);
        row.drag->setBounds (1094, y + 4, 122, 88);
    }

    drumStyle->setBounds (234, 330 + 14, 290, 26);
    bassTimbre->setBounds (234, 330 + 1 * 104 + 14, 290, 26);

    // big knob · control room
    bigKnob->setBounds (70, 684, 112, 108);
    filterMode->setBounds (196, 690, 220, 34);
    knobBypass->setBounds (196, 742, 220, 40);
    fxPower->setBounds (436, 692, 186, 40);
    echoPower->setBounds (436, 742, 186, 40);
    muteDrums->setBounds (644, 688, 184, 98);
    muteBass->setBounds (838, 688, 184, 98);
    muteRiddim->setBounds (1032, 688, 184, 98);

    // jackpot
    // dub siren
    sirenFire->setBounds (70, 836, 112, 100);
    sirenHold->setBounds (196, 842, 150, 36);
    sirenEcho->setBounds (356, 842, 220, 36);
    sirenWave->setBounds (196, 892, 380, 40);
    for (size_t i = 0; i < sirenDials.size(); ++i)
        sirenDials[i]->setBounds (600 + (int) i * 103, 840, 84, 76);

    reels.setBounds (64, 688 + 152 + D, 156, 126);
    jackpot.setBounds (226, 682 + 152 + D, 196, 138);
    lever.setBounds (432, 664 + 152 + D, 96, 158);
    presets.setBounds (1074, 8, 154, 28);

    // riddim
    length->setBounds (606, 838 + D, 170, 40);
    exportBtn.setBounds (786, 838 + D, 178, 40);
    riddimDrag.setBounds (606, 886 + D, 358, 84);

    presetPanel.setBounds (64, 980, 1152, 76);
    bpmPanel.setBounds (64, 1104, 1152, 110);
    speaker.setBounds (speakerArea);
    renderBackground();
}

void MainPage::paint (juce::Graphics& g)
{
    g.fillAll (col::woodDark);
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    // nome de cada acorde no display âmbar
    const auto slots = proc.getChords().getSlots();
    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
    {
        const auto& s = slots[(size_t) i];
        const auto& st = strips[(size_t) i];
        const bool on = s.beats > 0;
        auto d = st.display.toFloat();
        displayGlass (g, d.reduced (2.0f), 3.0f);
        juce::String t = "OFF";
        if (on)
        {
            t = juce::String (dub::rootName (s.root)) + (s.quality == dub::Min ? " MENOR" : " MAIOR");
            if ((size_t) i < voicings.used.size())
                t << u8 ("  \xc2\xb7  ") << u8 (dub::inversionName (voicings.used[(size_t) i]));
        }
        g.setFont (stencil (14.5f, 0.12f));
        glowText (g, t, d.reduced (6.0f, 2.0f), juce::Justification::centred, col::amber, on ? 1.0f : 0.35f);
    }
}

void MainPage::renderBackground()
{
    const float scale = 2.0f;
    background = juce::Image (juce::Image::ARGB, (int) (kW * scale), (int) (kH * scale), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    juce::Random rng (1240);

    paintWood (g, woodL, rng);
    paintWood (g, woodR, rng);
    paintPlate (g, plate, rng, false, true);
    for (float sx : { (float) plate.getX() + 12.0f, (float) plate.getRight() - 12.0f })
        screw (g, sx, (float) kH - 12.0f, rng);

    section (g, harmonySection, u8 ("HARMONIA  \xc2\xb7  VALE PARA O SKANK E PARA O BAIXO"));
    section (g, machineSection, u8 ("M\xc3\x81QUINAS"));
    section (g, jackpotSection, u8 ("JACKPOT  \xc2\xb7  SEMENTE  \xc2\xb7  ALAVANCA"));
    {
        // placa da alavanca: o que ela sorteia
        auto plate = juce::Rectangle<float> (526.0f, 1254.0f, 40.0f, 126.0f);   // 840 + 414 (abaixo da sirene, dos PRESETS e do BPM)
        g.setColour (juce::Colour (0xff1c1915));
        g.fillRoundedRectangle (plate, 5.0f);
        g.setColour (col::jamGold.withAlpha (0.8f));
        g.drawRoundedRectangle (plate.reduced (2.0f), 4.0f, 1.0f);
        const char* words[] = { "PUXE", "", "HAR", "MO", "NIA", "", "BATE", "RIA", "", "TUDO" };
        g.setFont (stencil (8.5f, 0.12f));
        for (int i = 0; i < 10; ++i)
        {
            g.setColour (i == 0 || i == 9 ? col::jamRed.brighter (0.4f) : col::jamGold);
            g.drawText (words[i], plate.withHeight (11.0f).translated (0.0f, 8.0f + i * 11.0f), juce::Justification::centred);
        }
    }
    section (g, riddimSection, "RIDDIM");
    section (g, knobSection, u8 ("BIG KNOB  \xc2\xb7  CONTROL ROOM  \xc2\xb7  MASTER"));
    engraved (g, u8 ("FILTRO  \xc2\xb7  DUPLO CLIQUE NO BOT\xc3\x83O ABRE"), juce::Rectangle<float> (196.0f, 677.0f, 220.0f, 11.0f), juce::Justification::centred, 7.5f, 0.2f);
    engraved (g, u8 ("BYPASS: DESLIGA O BIG KNOB INTEIRO"), juce::Rectangle<float> (196.0f, 727.0f, 220.0f, 11.0f), juce::Justification::centred, 6.5f, 0.15f);
    engraved (g, u8 ("TUDO NA ABA CONTROL ROOM"), juce::Rectangle<float> (196.0f, 784.0f, 220.0f, 11.0f), juce::Justification::centred, 7.5f, 0.2f);

    section (g, presetBox, u8 ("PRESETS  \xc2\xb7  BANCO \xc3\x9aNICO DE RIDDIMS  \xc2\xb7  ESTILO, RIDDIM E M\xc3\x9aSICA A-Z  \xc2\xb7  H B D = PARTES DA GRAVA\xc3\x87\xc3\x83O"));
    section (g, bpmBox, u8 ("BPM  \xc2\xb7  DIGITE, BATA NO TEMPO OU ESCOLHA O PRESET DE BPM"));
    section (g, sirenSection, u8 ("DUB SIREN  \xc2\xb7  COM O MASTOR TAPE ECHO MTE-12"));
    engraved (g, u8 ("SEGURE O FIRE  \xc2\xb7  O ECO CONTINUA DEPOIS"), juce::Rectangle<float> (196.0f, 882.0f, 380.0f, 9.0f), juce::Justification::centredLeft, 7.0f, 0.2f);
    engraved (g, "FORMA DA ONDA", juce::Rectangle<float> (360.0f, 848.0f, 216.0f, 12.0f), juce::Justification::centredRight, 8.5f, 0.3f);
    const char* sirenNames[] = { "PITCH", "RATE", "DEPTH", "ECO: REPEAT", "ECO: INTENS.", "LEVEL" };
    for (int i = 0; i < 6; ++i)
        engraved (g, sirenNames[i], juce::Rectangle<float> (600.0f + i * 103.0f - 10.0f, 918.0f, 104.0f, 13.0f), juce::Justification::centred, 9.0f, 0.3f);

    // plaquetas de cada máquina
    const char* names[]  = { "BATERIA", "RIDDIM", "BAIXO" };
    const juce::String models[] = { juce::String ("MRD-12 ") + MRM_DRUMS_VER, juce::String ("MRK-80 ") + MRM_CHORDS_VER,
                                    juce::String ("MRB-69 ") + MRM_BASS_VER };
    const char* players[] = { "ESTILO DE REGGAE  \xc2\xb7  BATERISTA", "PADR\xc3\x83O DO SKANK", "TIMBRE  \xc2\xb7  BAIXISTA" };
    for (int m = 0; m < MrmProcessor::NumMachines; ++m)
    {
        const auto& row = rows[(size_t) m];
        auto pa = row.plateArea.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.08f));
        g.fillRoundedRectangle (pa, 4.0f);
        g.setColour (col::ink.withAlpha (0.35f));
        g.drawRoundedRectangle (pa, 4.0f, 1.0f);
        engraved (g, names[m], pa.withHeight (24.0f).translated (0, 4.0f), juce::Justification::centred, 17.0f, 0.2f, col::ink);
        engraved (g, models[m].trim(), pa.withTrimmedTop (26.0f).withHeight (13.0f), juce::Justification::centred, 9.0f, 0.3f);
        engraved (g, u8 (players[m]), juce::Rectangle<float> (234.0f, (float) row.area.getY() + 1.0f, 290.0f, 13.0f),
                  juce::Justification::centred, 9.0f, 0.3f);
        if (! row.musicians)
        {
            // CONTRATEMPO: uma tecla só, sempre acesa
            auto cap = juce::Rectangle<float> (234.0f, (float) row.area.getY() + 26.0f, 290.0f, 40.0f).reduced (40.0f, 0.0f);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillRoundedRectangle (cap.translated (0, 2.0f), 3.0f);
            cap = cap.translated (0, 1.5f);
            juce::ColourGradient cg (juce::Colour (0xff2c2925), cap.getX(), cap.getY(), juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
            g.setGradientFill (cg);
            g.fillRoundedRectangle (cap, 3.0f);
            g.setColour (col::amber.withAlpha (0.8f));
            g.fillRoundedRectangle (cap.withTop (cap.getBottom() - 3.0f).reduced (6.0f, 0.0f).translated (0, -1.5f), 1.0f);
            lamp (g, juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ cap.getX() + 14.0f, cap.getCentreY() }), true);
            g.setColour (col::amber);
            g.setFont (stencil (12.0f, 0.12f));
            g.drawText ("CONTRATEMPO", cap.withTrimmedLeft (22.0f), juce::Justification::centred);
        }
        if (m < MrmProcessor::NumMachines - 1)
        {
            g.setColour (col::ink.withAlpha (0.15f));
            g.drawHorizontalLine (row.area.getBottom() + 4, 64.0f, 1216.0f);
        }
    }

    // gravações da seção RIDDIM
    engraved (g, "ARRASTE PARA A TIMELINE DA DAW", juce::Rectangle<float> (606.0f, 1386.0f, 358.0f, 10.0f), juce::Justification::centred, 7.0f, 0.2f);
}
} // namespace mrmui
