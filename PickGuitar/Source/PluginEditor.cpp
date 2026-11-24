#include "PluginEditor.h"

using namespace juce;

// maiúsculas com os acentos do português (o toUpperCase do JUCE só troca o ASCII)
static String upper (const String& s)
{
    String r = s.toUpperCase();
    const char* lo[] = { "á", "à", "â", "ã", "é", "ê", "í", "ó", "ô", "õ", "ú", "ç" };
    const char* up[] = { "Á", "À", "Â", "Ã", "É", "Ê", "Í", "Ó", "Ô", "Õ", "Ú", "Ç" };
    for (int i = 0; i < 12; ++i) r = r.replace (String::fromUTF8 (lo[i]), String::fromUTF8 (up[i]));
    return r;
}

//==============================================================================
// visual
PickLookAndFeel::PickLookAndFeel()
{
    setColour (ComboBox::backgroundColourId, pg::dark);
    setColour (ComboBox::textColourId, pg::text);
    setColour (ComboBox::outlineColourId, pg::line);
    setColour (ComboBox::arrowColourId, pg::amber);
    setColour (PopupMenu::backgroundColourId, Colour (0xff2a1d14));
    setColour (PopupMenu::textColourId, pg::text);
    setColour (PopupMenu::highlightedBackgroundColourId, pg::amber);
    setColour (PopupMenu::highlightedTextColourId, pg::ink);
    setColour (Label::textColourId, pg::cream);
    setColour (Slider::textBoxTextColourId, pg::cream);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::rotarySliderFillColourId, pg::amber);
    setColour (TextButton::textColourOffId, pg::text);
    setColour (TextButton::textColourOnId, pg::ink);
}

void PickLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const float th = jmax (3.0f, r * 0.13f);
    Path bg, val;
    bg.addCentredArc (c.x, c.y, r - th * 0.5f, r - th * 0.5f, 0.0f, a0, a1, true);
    g.setColour (pg::dark);
    g.strokePath (bg, PathStrokeType (th, PathStrokeType::curved, PathStrokeType::rounded));
    if (pos > 0.002f)
    {
        val.addCentredArc (c.x, c.y, r - th * 0.5f, r - th * 0.5f, 0.0f, a0, ang, true);
        g.setColour (s.findColour (Slider::rotarySliderFillColourId));
        g.strokePath (val, PathStrokeType (th, PathStrokeType::curved, PathStrokeType::rounded));
    }
    const float kr = r - th - 3.0f;
    ColourGradient grad (Colour (0xff6b5440), c.x - kr * 0.3f, c.y - kr * 0.4f, Colour (0xff140e09), c.x + kr, c.y + kr, true);
    grad.addColour (0.6, Colour (0xff2e2218));
    g.setGradientFill (grad);
    g.fillEllipse (Rectangle<float> (kr * 2, kr * 2).withCentre (c));
    g.setColour (Colours::black);
    g.drawEllipse (Rectangle<float> (kr * 2, kr * 2).withCentre (c), 1.5f);
    g.setColour (pg::cream);
    const Point<float> tip (c.x + std::sin (ang) * kr * 0.8f, c.y - std::cos (ang) * kr * 0.8f);
    g.drawLine (Line<float> (c, tip), jmax (2.5f, kr * 0.16f));
}

void PickLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider::SliderStyle, Slider&)
{
    const float cy = (float) y + (float) h * 0.5f;
    auto track = Rectangle<float> ((float) x, cy - 3.0f, (float) w, 6.0f);
    g.setColour (pg::dark); g.fillRoundedRectangle (track, 3.0f);
    g.setColour (pg::blue); g.fillRoundedRectangle (track.withRight (pos), 3.0f);
    g.setColour (pg::cream); g.fillEllipse (Rectangle<float> (16.0f, 16.0f).withCentre ({ pos, cy }));
    g.setColour (Colours::black); g.drawEllipse (Rectangle<float> (16.0f, 16.0f).withCentre ({ pos, cy }), 1.0f);
}

void PickLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    Colour top (0xff4b3725), bot (0xff33251a);
    if (on)
    {
        const auto col = Colour ((uint32) (int64) b.getProperties().getWithDefault ("on", (int64) pg::amber.getARGB()));
        top = col.brighter (0.25f); bot = col.darker (0.15f);
    }
    if (down) { top = top.darker (0.2f); bot = bot.darker (0.2f); }
    g.setGradientFill (ColourGradient (top, 0, r.getY(), bot, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (over ? pg::amber : Colours::black);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

void PickLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    const bool on = b.getToggleState();
    const bool darkText = on && (bool) b.getProperties().getWithDefault ("darkText", true);
    g.setColour (darkText ? pg::ink : (on ? Colours::white : pg::text));
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 2), Justification::centred, 2);
}

Label* PickLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (FontOptions (Font::getDefaultMonospacedFontName(), 11.5f, Font::plain));
    l->setColour (Label::textColourId, pg::cream);
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    return l;
}

//==============================================================================
Knob::Knob (PickGuitarProcessor& p, const String& id, String label, Colour arc) : name (std::move (label))
{
    slider.setSliderStyle (Slider::RotaryVerticalDrag);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setTextBoxStyle (Slider::TextBoxBelow, true, 90, 16);
    slider.setColour (Slider::rotarySliderFillColourId, arc);
    slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    addAndMakeVisible (slider);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id, slider);
    slider.setDoubleClickReturnValue (true, (double) p.apvts.getParameter (id)->convertFrom0to1 (p.apvts.getParameter (id)->getDefaultValue()));
}
void Knob::resized() { slider.setBounds (getLocalBounds().withTrimmedBottom (14)); }
void Knob::paint (Graphics& g)
{
    g.setColour (pg::muted);
    g.setFont (FontOptions (10.5f, Font::bold));
    g.drawFittedText (name, getLocalBounds().removeFromBottom (14), Justification::centred, 1);
}

//==============================================================================
Chips::Chips (PickGuitarProcessor& p, const String& id, StringArray labels, Colour on)
    : attachment (*p.apvts.getParameter (id), [this] (float v) { refresh (roundToInt (v)); }, nullptr)
{
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new TextButton (labels[i]));
        b->getProperties().set ("on", (int64) on.getARGB());
        b->onClick = [this, i] { attachment.setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    attachment.sendInitialUpdate();
}
void Chips::refresh (int v)
{
    for (int i = 0; i < buttons.size(); ++i) buttons[i]->setToggleState (i == v, dontSendNotification);
    if (onChange) onChange (v);
}
void Chips::resized()
{
    auto r = getLocalBounds();
    const int w = (r.getWidth() - 6 * (buttons.size() - 1)) / jmax (1, buttons.size());
    for (auto* b : buttons) { b->setBounds (r.removeFromLeft (w)); r.removeFromLeft (6); }
}

//==============================================================================
Bypass::Bypass (PickGuitarProcessor& p, const String& id, String onText) : onLabel (std::move (onText))
{
    button.setClickingTogglesState (true);
    button.onStateChange = [this] { update(); };
    addAndMakeVisible (button);
    att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id, button);
    update();
}
void Bypass::update()
{
    const bool on = button.getToggleState();
    button.setButtonText (on ? onLabel + " LIGADO" : "BYPASS");
    repaint();
}
void Bypass::resized() { button.setBounds (getLocalBounds().withTrimmedLeft (26)); }
void Bypass::paint (Graphics& g)
{
    auto l = Rectangle<float> (16, 16).withCentre ({ 9.0f, (float) getHeight() * 0.5f });
    if (button.getToggleState())
    {
        g.setGradientFill (ColourGradient (Colour (0xffffd38a), l.getX(), l.getY(), pg::amber, l.getRight(), l.getBottom(), true));
        g.fillEllipse (l);
        g.setColour (pg::amber.withAlpha (0.35f)); g.fillEllipse (l.expanded (3));
    }
    else { g.setColour (Colour (0xff3a1d12)); g.fillEllipse (l); }
    g.setColour (Colours::black); g.drawEllipse (l, 1.0f);
}

//==============================================================================
Combo::Combo (PickGuitarProcessor& p, const String& id, String label) : name (std::move (label))
{
    if (auto* c = dynamic_cast<AudioParameterChoice*> (p.apvts.getParameter (id))) box.addItemList (c->choices, 1);
    addAndMakeVisible (box);
    att = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, id, box);
}
void Combo::resized() { box.setBounds (getLocalBounds().withTrimmedTop (name.isEmpty() ? 0 : 15)); }
void Combo::paint (Graphics& g)
{
    if (name.isEmpty()) return;
    g.setColour (pg::muted);
    g.setFont (FontOptions (11.0f, Font::bold));
    g.drawText (name, getLocalBounds().removeFromTop (14), Justification::centredLeft);
}

//==============================================================================
// grade
Rectangle<float> GridView::cellRect (int i) const
{
    const float gap = 4.0f, beatGap = 8.0f;
    const float w = ((float) getWidth() - 15 * gap - 3 * beatGap) / 16.0f;
    const float x = (float) i * (w + gap) + (float) (i / 4) * beatGap;
    return { x, 18.0f, w, (float) getHeight() - 18.0f };
}
void GridView::paint (Graphics& g)
{
    const int cur = proc.engine.curStep.load();
    for (int i = 0; i < mpg::kSteps; ++i)
    {
        auto r = cellRect (i);
        const int b = i % 4, beat = i / 4 + 1;
        g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 12.0f, Font::bold));
        g.setColour (b == 0 ? ((beat == 2 || beat == 4) ? Colour (0xffff9a86) : pg::cream) : pg::muted);
        g.drawText (b == 0 ? String (beat) : String (b == 1 ? "e" : b == 2 ? "&" : "a"), r.withY (0).withHeight (16), Justification::centred);
        const char c = proc.cell (i);
        Colour top (0xff1a120c), bot (0xff1a120c), txt = pg::muted;
        if (c == 'x') { top = Colour (0xff7a5430); bot = Colour (0xff4b3420); txt = pg::cream; }
        if (c == 'X') { top = Colour (0xfff2b25a); bot = Colour (0xffc47a22); txt = pg::ink; }
        if (c == 'o') { top = Colour (0xff6fbf78); bot = Colour (0xff3b7f45); txt = Colours::white; }
        if (c == 'r') { top = pg::blue; bot = Colour (0xff1d4aa8); txt = Colour (0xffe8f3ff); }
        g.setGradientFill (ColourGradient (top, 0, r.getY(), bot, 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (i == cur ? pg::cream : Colours::black);
        g.drawRoundedRectangle (r, 5.0f, i == cur ? 2.5f : 1.0f);
        g.setColour (txt);
        if (c == 'r')
        {
            for (int k = 0; k < 6; ++k)
                g.fillEllipse (Rectangle<float> (4, 4).withCentre ({ r.getCentreX() + (k < 3 ? -4.0f : 4.0f), r.getCentreY() + (float) (k % 3 - 1) * 7.0f }));
        }
        else if (c != '.')
        {
            g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 14.0f, Font::bold));
            g.drawText (String::charToString ((juce_wchar) c), r, Justification::centred);
        }
    }
}
void GridView::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < mpg::kSteps; ++i)
        if (cellRect (i).contains (e.position)) { proc.cycleCell (i); repaint(); return; }
}

//==============================================================================
void NeckView::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff4a2e18), 0, 0, Colour (0xff2a1a0f), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 5.0f);
    const int frets = 16;
    const float fw = b.getWidth() / (float) frets, H = b.getHeight();
    g.setColour (Colour (0xffbfb39a));
    for (int i = 0; i <= frets; ++i) g.drawLine ((float) i * fw, 0, (float) i * fw, H, 2.0f);
    g.setColour (Colour (0xffe9dfc8));
    for (int f : { 3, 5, 7, 9, 15 }) g.fillEllipse (Rectangle<float> (6, 6).withCentre ({ ((float) f - 0.5f) * fw, H * 0.5f }));
    g.fillEllipse (Rectangle<float> (6, 6).withCentre ({ 11.5f * fw, H / 3 }));
    g.fillEllipse (Rectangle<float> (6, 6).withCentre ({ 11.5f * fw, 2 * H / 3 }));
    const int open[6] = { 40, 45, 50, 55, 59, 64 };
    auto sy = [&] (int i) { return H - 5.0f - (float) i * (H - 10.0f) / 5.0f; };
    for (int i = 0; i < 6; ++i) { g.setColour (Colour (0xffd8cfb8)); g.drawLine (0, sy (i), b.getWidth(), sy (i), 2.2f - (float) i * 0.25f); }
    const double now = Time::getMillisecondCounterHiRes();
    for (auto& h : hits)
    {
        const float a = 1.0f - (float) ((now - h.t) / 500.0);
        if (a <= 0) continue;
        int si = 0;
        for (int i = 5; i >= 0; --i) if (h.midi >= open[i] && h.midi - open[i] <= frets - 1) { si = i; break; }
        const int fret = jlimit (0, frets - 1, h.midi - open[si]);
        auto c = Rectangle<float> (16, 16).withCentre ({ ((float) fret + 0.5f) * fw, sy (si) });
        g.setColour ((h.open ? Colour (0xff6fbf78) : pg::amber).withAlpha (a));
        g.fillEllipse (c);
        g.setColour (pg::ink.withAlpha (a));
        g.setFont (FontOptions (10.0f, Font::bold));
        g.drawText (mpg::noteName (h.midi), c, Justification::centred);
    }
}

//==============================================================================
Rectangle<float> PicaPauMap::cellRect (int row, int col) const
{
    const float lx = 22.0f, gap = 2.0f;
    const float w = ((float) getWidth() - lx - 15 * gap - 3.0f) / 16.0f;
    const float h = ((float) getHeight() - 7 * 3.0f) / 8.0f;
    return { lx + (float) col * (w + gap) + (col >= 8 ? 3.0f : 0.0f), (float) row * (h + 3.0f), w, h };
}
void PicaPauMap::paint (Graphics& g)
{
    const auto p = proc.makeParams();
    std::array<bool, mpg::kCycle> autoSet {};
    mpg::autoPicaPau (p.grid, p.dose, autoSet);
    const int cur = proc.engine.curCycle.load();
    g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 10.0f, Font::plain));
    for (int row = 0; row < 8; ++row)
    {
        auto lr = cellRect (row, 0);
        g.setColour (Colour (0xff7a6a50));
        g.drawText (String (row + 1), Rectangle<float> (0, lr.getY(), 18, lr.getHeight()), Justification::centredRight);
        for (int col = 0; col < 16; ++col)
        {
            const int cyc = row * 16 + col;
            const char v = p.grid[(size_t) col];
            auto r = cellRect (row, col);
            const bool pp = mpg::isPicaPau (p, autoSet, cyc);
            Colour c (0xff1f1610);
            if (v != '.') c = pp ? pg::blue : (v == 'o' ? Colour (0xff3b7f45) : Colour (0xff6b4a2b));
            if (pp) { g.setColour (pg::blue.withAlpha (0.35f)); g.fillRoundedRectangle (r.expanded (2), 3.0f); }
            g.setColour (c);
            g.fillRoundedRectangle (pp ? r : r.reduced (0, 2), 2.0f);
            if (proc.edit (cyc) != 0 && v != '.' && v != 'o') { g.setColour (Colour (0xffe8f3ff)); g.drawRoundedRectangle (r.reduced (1), 2.0f, 1.6f); }
            if (cyc == cur) { g.setColour (pg::cream); g.drawRoundedRectangle (r.expanded (1), 2.0f, 1.6f); }
        }
    }
}
String PicaPauMap::info() const
{
    const auto p = proc.makeParams();
    std::array<bool, mpg::kCycle> autoSet {};
    mpg::autoPicaPau (p.grid, p.dose, autoSet);
    int n = 0;
    for (int c = 0; c < mpg::kCycle; ++c) n += mpg::isPicaPau (p, autoSet, c);
    const int ed = proc.numEdits();
    String s = n ? String (n) + " pica-pau em 8 compassos" + String::fromUTF8 (" · ≈ 1 a cada ") + String (8.0 / n, 1).replace (".0", "") + " compasso(s)"
                 : String ("Nenhum pica-pau.");
    if (ed) s << String::fromUTF8 (" · ") << ed << String::fromUTF8 (" edição(ões) sua(s)");
    return s;
}
void PicaPauMap::mouseDown (const MouseEvent& e)
{
    for (int row = 0; row < 8; ++row)
        for (int col = 0; col < 16; ++col)
            if (cellRect (row, col).expanded (1).contains (e.position))
            {
                const char v = proc.cell (col);
                if (v == 'x' || v == 'X' || v == 'r') { proc.toggleEdit (row * 16 + col); repaint(); if (auto* p = getParentComponent()) p->repaint(); }
                return;
            }
}

//==============================================================================
// EQ
float EqGraph::xOf (float f) const { return std::log (f / 20.0f) / std::log (1000.0f) * (float) getWidth(); }
float EqGraph::fOf (float x) const { return 20.0f * std::pow (1000.0f, jlimit (0.0f, 1.0f, x / (float) getWidth())); }
float EqGraph::yOf (float db) const { return (float) getHeight() * 0.5f - db / 15.0f * ((float) getHeight() * 0.5f - 14.0f); }
float EqGraph::dbOf (float y) const { return -(y - (float) getHeight() * 0.5f) / ((float) getHeight() * 0.5f - 14.0f) * 15.0f; }
int EqGraph::hitNode (Point<float> p) const
{
    int best = -1; float bd = 22.0f;
    for (int i = 0; i < mpg::kEqBands; ++i)
    {
        const float d = p.getDistanceFrom ({ xOf (proc.get ("eqF" + String (i + 1))), yOf (proc.get ("eqG" + String (i + 1))) });
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
void EqGraph::setBand (int i, float f, float g, float q, bool)
{
    const String n (i + 1);
    for (auto [id, v] : { std::pair<String, float> { "eqF" + n, f }, { "eqG" + n, g }, { "eqQ" + n, q } })
        if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}
void EqGraph::updateReadout (int i)
{
    if (i < 0) return;
    const String n (i + 1);
    const float f = proc.get ("eqF" + n), g = proc.get ("eqG" + n), q = proc.get ("eqQ" + n);
    const bool hl = mpg::eqType (i) == mpg::EqHP || mpg::eqType (i) == mpg::EqLP;
    readout = String::fromUTF8 (mpg::eqName (i)) + String::fromUTF8 (" · ") + (f < 1000 ? String (roundToInt (f)) + " Hz" : String (f / 1000.0f, 2) + " kHz")
            + String::fromUTF8 (" · ") + (hl ? String::fromUTF8 ("ressonância ") + String (g, 1) + " dB"
                                             : (g > 0 ? "+" : "") + String (g, 1) + " dB" + String::fromUTF8 (" · Q ") + String (q, 2));
}
void EqGraph::mouseDown (const MouseEvent& e)
{
    drag = sel = hitNode (e.position);
    if (drag >= 0)
        for (auto k : { "eqF", "eqG" }) if (auto* p = proc.apvts.getParameter (k + String (drag + 1))) p->beginChangeGesture();
    updateReadout (sel); repaint();
}
void EqGraph::mouseDrag (const MouseEvent& e)
{
    if (drag < 0) return;
    const bool hl = mpg::eqType (drag) == mpg::EqHP || mpg::eqType (drag) == mpg::EqLP;
    const float f = (float) roundToInt (fOf (e.position.x));
    const float g = std::round (jlimit (hl ? 0.0f : -15.0f, 15.0f, dbOf (e.position.y)) * 2.0f) / 2.0f;
    setBand (drag, f, g, proc.get ("eqQ" + String (drag + 1)), true);
    updateReadout (drag); repaint();
}
void EqGraph::mouseUp (const MouseEvent&)
{
    if (drag >= 0)
        for (auto k : { "eqF", "eqG" }) if (auto* p = proc.apvts.getParameter (k + String (drag + 1))) p->endChangeGesture();
    drag = -1;
}
void EqGraph::mouseDoubleClick (const MouseEvent& e)
{
    const int i = hitNode (e.position);
    if (i < 0) return;
    const auto& d = mpg::kEqFlat[(size_t) i];
    setBand (i, d.f, d.g, d.q, false);
    sel = i; updateReadout (i); repaint();
}
void EqGraph::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    int i = hitNode (e.position);
    if (i < 0) i = sel;
    if (i < 0 || mpg::eqType (i) != mpg::EqPK) return;
    const float q = jlimit (0.3f, 8.0f, proc.get ("eqQ" + String (i + 1)) * (w.deltaY > 0 ? 1.1f : 0.91f));
    if (auto* p = proc.apvts.getParameter ("eqQ" + String (i + 1))) p->setValueNotifyingHost (p->convertTo0to1 (q));
    sel = i; updateReadout (i); repaint();
}
void EqGraph::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (pg::dark); g.fillRoundedRectangle (b, 6.0f);
    g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 11.0f, Font::plain));
    for (int f : { 50, 100, 200, 500, 1000, 2000, 5000, 10000 })
    {
        const float x = xOf ((float) f);
        g.setColour (Colour (0xff2c2017)); g.drawVerticalLine ((int) x, 0, b.getBottom());
        g.setColour (Colour (0xff7a6a50)); g.drawText (f < 1000 ? String (f) : String (f / 1000) + "k", (int) x + 3, getHeight() - 16, 40, 14, Justification::left);
    }
    for (int d : { -12, -6, 0, 6, 12 })
    {
        const float y = yOf ((float) d);
        g.setColour (d ? Colour (0xff2c2017) : Colour (0xff4a3828)); g.drawHorizontalLine ((int) y, 0, b.getRight());
        g.setColour (Colour (0xff7a6a50)); g.drawText ((d > 0 ? "+" : "") + String (d), 4, (int) y - 15, 30, 14, Justification::left);
    }
    const auto p = proc.makeParams();
    Path curve;
    for (int x = 0; x <= getWidth(); x += 3)
    {
        const float db = jlimit (-24.0f, 24.0f, mpg::responseDb (p, fOf ((float) x)));
        if (x == 0) curve.startNewSubPath ((float) x, yOf (db)); else curve.lineTo ((float) x, yOf (db));
    }
    Path fill (curve);
    fill.lineTo (b.getRight(), yOf (0)); fill.lineTo (0, yOf (0)); fill.closeSubPath();
    g.setColour ((p.eqOn ? pg::amber : Colour (0xff786446)).withAlpha (0.12f)); g.fillPath (fill);
    g.setColour (p.eqOn ? pg::amber : Colour (0xff6a5a44)); g.strokePath (curve, PathStrokeType (3.0f));
    for (int i = 0; i < mpg::kEqBands; ++i)
    {
        const auto c = Point<float> (xOf (p.eq[(size_t) i].f), yOf (p.eq[(size_t) i].g));
        const float r = i == sel ? 11.0f : 9.0f;
        g.setColour (i == sel ? Colour (0xffffd27a) : (i == 0 || i == 6 ? pg::red : pg::cream));
        g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c));
        g.setColour (Colours::black); g.drawEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c), 2.0f);
        g.setColour (pg::ink); g.setFont (FontOptions (10.0f, Font::bold));
        g.drawText (i == 0 ? "H" : i == 6 ? "L" : String (i), Rectangle<float> (r * 2, r * 2).withCentre (c), Justification::centred);
    }
}

//==============================================================================
void ScopeView::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff06120a)); g.fillRoundedRectangle (b, 8.0f);
    g.setColour (Colour (0xff0a0c08)); g.drawRoundedRectangle (b.reduced (1.5f), 8.0f, 3.0f);
    g.setColour (Colour (0x2e50a05a));
    for (int i = 1; i < 10; ++i) g.drawVerticalLine (roundToInt (b.getWidth() * (float) i / 10.0f), 4, b.getBottom() - 4);
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine (roundToInt (b.getHeight() * (float) i / 4.0f), 4, b.getRight() - 4);
    constexpr int N = PickGuitarProcessor::kScope, show = 900;
    const int w = proc.scopeWrite.load();
    std::array<float, N> s;
    for (int i = 0; i < N; ++i) s[(size_t) i] = proc.scope[(size_t) ((w + i) % N)];
    int s0 = N - show - 400;
    for (int i = s0; i < N - show; ++i) if (s[(size_t) (i - 1)] < 0 && s[(size_t) i] >= 0) { s0 = i; break; }
    Path p;
    for (int i = 0; i < show; ++i)
    {
        const float x = b.getX() + 6 + (b.getWidth() - 12) * (float) i / (float) (show - 1);
        const float y = b.getCentreY() - jlimit (-1.0f, 1.0f, s[(size_t) (s0 + i)] * 1.6f) * b.getHeight() * 0.42f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (Colour (0x5578ff78)); g.strokePath (p, PathStrokeType (5.0f));
    g.setColour (Colour (0xff8fff8f)); g.strokePath (p, PathStrokeType (1.8f));
}

//==============================================================================
void PresetLcd::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0a0c10)); g.fillRoundedRectangle (b, 5.0f);
    auto in = b.reduced (4.0f);
    g.setGradientFill (ColourGradient (Colour (0xff4a8cf0), 0, in.getY(), Colour (0xff1d4aa8), 0, in.getBottom(), false));
    g.fillRoundedRectangle (in, 3.0f);
    g.setColour (Colours::black.withAlpha (0.07f));
    for (float y = in.getY(); y < in.getBottom(); y += 3.0f) g.drawHorizontalLine ((int) y, in.getX(), in.getRight());
    const int i = proc.getCurrentProgram();
    const auto& P = mpg::presets()[(size_t) i];
    auto t = in.reduced (12.0f, 2.0f);
    g.setColour (Colour (0xffe8f3ff).withAlpha (0.85f));
    g.setFont (FontOptions (11.0f, Font::bold));
    auto top = t.removeFromTop (14);
    g.drawText ("PRESET  " + upper (String::fromUTF8 (mpg::eraName (P.era))), top, Justification::left);
    g.drawText (String (i + 1) + "/" + String ((int) mpg::presets().size()), top, Justification::right);
    g.setColour (Colour (0xffe8f3ff));
    g.setFont (FontOptions (19.0f, Font::bold));
    g.drawText (upper (String::fromUTF8 (P.name)), t.removeFromTop (21), Justification::left);
    g.setFont (FontOptions (10.5f, Font::bold));
    g.setColour (Colour (0xffe8f3ff).withAlpha (0.85f));
    g.drawText (upper (String::fromUTF8 (P.role)), t, Justification::left);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.16f), 0, in.getY(), Colours::transparentWhite, 0, in.getCentreY(), false));
    g.fillRoundedRectangle (in, 3.0f);
}

//==============================================================================
PickGuitarEditor::PickGuitarEditor (PickGuitarProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      key (p, "key", "TOM"), scale (p, "scale", "ESCALA"), master (p, "master", "MASTER"),
      lcd (p), scope (p),
      mode (p, "mode", { "FOLLOW", "GROOVE", "COUNTER" }),
      src (p, "src", "ENTRADA DO BAIXO"), line (p, "line", "LINHA GUIA"), oct (p, "oct", "OITAVA DA PALHETA"), shape (p, "shape", "DESENHO DO COUNTER"),
      grid (p),
      gate (p, "gate", String::fromUTF8 ("DURAÇÃO")), human (p, "human", "HUMANO"), swing (p, "swing", "SWING"), acc (p, "acc", "ACENTO"),
      art (p, "art", { String::fromUTF8 ("MANTÉM"), "PICA-PAU" }), rep (p, "rep", { "3 TERCINAS", "2", "4" }, pg::blue),
      ppSwing (p, "ppSwing", "SWING PICA-PAU", pg::blue), map (p),
      pickup (p, "pickup", { String::fromUTF8 ("CAPTADOR BRAÇO"), "CAPTADOR MEIO", "CAPTADOR PONTE" }),
      mute (p, "mute", "ABAFADO"), pos (p, "pos", String::fromUTF8 ("POSIÇÃO PALHETA")), atk (p, "atk", "ATAQUE"),
      drive (p, "drive", "DRIVE"), tone (p, "tone", "TOM"), body (p, "body", "CORPO"),
      eqOn (p, "eqOn", "EQ"), eq (p), bigKnob (p, "bk", ""),
      satOn (p, "satOn", "SATURADOR"), satType (p, "satType", { "FITA", String::fromUTF8 ("VÁLVULA"), "TRANSISTOR" }, Colour (0xffffb48a)),
      satDrive (p, "satDrive", "DRIVE", Colour (0xffffb48a)), satMix (p, "satMix", "MIX", Colour (0xffffb48a)), satOut (p, "satOut", String::fromUTF8 ("SAÍDA"), Colour (0xffffb48a)),
      echoOn (p, "echoOn", "ECHO"), echoSync (p, "echoSync", "TEMPO"),
      echoFb (p, "echoFb", String::fromUTF8 ("REPETIÇÕES"), Colour (0xfff6efd9)), echoMix (p, "echoMix", "MIX ECO", Colour (0xfff6efd9)), spring (p, "spring", "MOLA", Colour (0xfff6efd9)),
      keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    wood = skin::makeWood (26, kH);

    // cabeçalho
    for (auto* c : std::initializer_list<Component*> { &key, &scale, &vu, &master, &power, &eraPrev, &eraNext, &musPrev, &musNext, &lcd, &play, &bpm, &guide, &scope })
        addAndMakeVisible (c);
    vu.setColours (pg::cream, pg::ink, Colour (0xff1a120c));
    power.setButtonText ("POWER"); power.setClickingTogglesState (true);
    power.getProperties().set ("on", (int64) pg::red.getARGB()); power.getProperties().set ("darkText", false);
    powerAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "power", power);
    eraPrev.setButtonText (String::fromUTF8 ("◀")); eraNext.setButtonText (String::fromUTF8 ("▶"));
    musPrev.setButtonText (String::fromUTF8 ("◀")); musNext.setButtonText (String::fromUTF8 ("▶"));
    auto eraJump = [this] (int d)
    {
        const int n = (int) mpg::presets().size();
        const int cur = mpg::presets()[(size_t) proc.getCurrentProgram()].era;
        const int ne = (cur + d + mpg::kNumEras) % mpg::kNumEras;
        for (int i = 0; i < n; ++i) if (mpg::presets()[(size_t) i].era == ne) { proc.applyMusician (i); break; }
    };
    eraPrev.onClick = [eraJump] { eraJump (-1); };
    eraNext.onClick = [eraJump] { eraJump (1); };
    musPrev.onClick = [this] { const int n = getNumMus(); proc.applyMusician ((proc.getCurrentProgram() + n - 1) % n); };
    musNext.onClick = [this] { proc.applyMusician ((proc.getCurrentProgram() + 1) % getNumMus()); };
    play.setButtonText (String::fromUTF8 ("▶ PLAY")); play.setClickingTogglesState (true);
    play.getProperties().set ("on", (int64) pg::green.getARGB()); play.getProperties().set ("darkText", false);
    playAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "play", play);
    bpm.setSliderStyle (Slider::IncDecButtons);
    bpm.setTextBoxStyle (Slider::TextBoxLeft, false, 56, 28);
    bpm.setTextValueSuffix (" BPM");
    bpmAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "bpm", bpm);
    bpm.setColour (Slider::textBoxOutlineColourId, pg::line);
    guide.setButtonText ("BAIXO GUIA"); guide.setClickingTogglesState (true);
    guideAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "guideBass", guide);

    const char* tabNames[] = { "LINHA", "TIMBRE", "EQUALIZADOR", "SATURADOR + TAPE ECHO", "GUITARRISTAS" };
    for (int i = 0; i < 5; ++i)
    {
        auto* t = tabs.add (new TextButton (String::fromUTF8 (tabNames[i])));
        t->onClick = [this, i] { setPage (i); };
        addAndMakeVisible (t);
    }

    // LINHA
    addChildComponent (pLinha);
    for (auto* c : std::initializer_list<Component*> { &mode, &src, &line, &oct, &shape, &grid, &gClear, &gFill, &gReset, &neck,
                                                       &gate, &human, &swing, &acc, &art, &rep, &dose, &ppSwing, &map, &ppClear })
        pLinha.addAndMakeVisible (c);
    gClear.setButtonText ("LIMPAR GRADE"); gFill.setButtonText ("TODAS AS NOTAS"); gReset.setButtonText ("GRADE DO GUITARRISTA");
    gClear.onClick = [this] { proc.setGrid ("................"); };
    gFill.onClick = [this] { proc.setGrid ("XxxxXxxxXxxxXxxx"); };
    gReset.onClick = [this] { proc.setGrid (mpg::presets()[(size_t) proc.getCurrentProgram()].grid); };
    dose.setSliderStyle (Slider::LinearHorizontal);
    dose.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
    doseAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "dose", dose);
    dose.setDoubleClickReturnValue (true, 5.0);
    ppClear.setButtonText (String::fromUTF8 ("DESFAZER EDIÇÕES DO MAPA"));
    ppClear.onClick = [this] { proc.clearEdits(); map.repaint(); pLinha.repaint(); };

    // TIMBRE
    addChildComponent (pTimbre);
    for (auto* c : std::initializer_list<Component*> { &pickup, &mute, &pos, &atk, &drive, &tone, &body }) pTimbre.addAndMakeVisible (c);

    // EQ
    addChildComponent (pEq);
    pEq.addAndMakeVisible (eqOn);
    pEq.addAndMakeVisible (eq);
    pEq.addAndMakeVisible (bigKnob);
    bigKnob.slider.setTextBoxStyle (Slider::TextBoxBelow, true, 140, 18);
    for (int i = 0; i < (int) mpg::eqPresets().size(); ++i)
    {
        auto* b = eqPresets.add (new TextButton (String::fromUTF8 (mpg::eqPresets()[(size_t) i].name)));
        b->onClick = [this, i] { proc.applyEqPreset (i); eq.repaint(); };
        pEq.addAndMakeVisible (b);
    }

    // SATURADOR + TAPE ECHO
    addChildComponent (pFx);
    for (auto* c : std::initializer_list<Component*> { &satOn, &satType, &satDrive, &satMix, &satOut, &echoOn, &echoSync, &echoFb, &echoMix, &spring, &throwBtn })
        pFx.addAndMakeVisible (c);
    throwBtn.setButtonText ("SEGURE: DUB THROW");
    throwBtn.getProperties().set ("on", (int64) pg::red.getARGB());
    throwBtn.onStateChange = [this] { proc.throwOn.store (throwBtn.isDown()); };

    // GUITARRISTAS
    addChildComponent (pMus);
    for (int i = 0; i < getNumMus(); ++i)
    {
        auto* b = musButtons.add (new TextButton (String::fromUTF8 (mpg::presets()[(size_t) i].name).toUpperCase()));
        b->getProperties().set ("on", (int64) pg::blue.getARGB()); b->getProperties().set ("darkText", false);
        b->onClick = [this, i] { proc.applyMusician (i); };
        pMus.addAndMakeVisible (b);
    }

    addAndMakeVisible (keyboard);
    keyboard.setAvailableRange (24, 71);
    keyboard.setLowestVisibleKey (24);
    keyboard.setColour (MidiKeyboardComponent::keyDownOverlayColourId, pg::amber);
    keyboard.setColour (MidiKeyboardComponent::mouseOverKeyOverlayColourId, pg::amber.withAlpha (0.3f));

    setSize (kW, kH);
    setPage (0);
    startTimerHz (30);
}

PickGuitarEditor::~PickGuitarEditor() { stopTimer(); setLookAndFeel (nullptr); }

int PickGuitarEditor::getNumMus() { return (int) mpg::presets().size(); }

void PickGuitarEditor::setPage (int pg_)
{
    page = jlimit (0, 4, pg_);
    Component* pages[] = { &pLinha, &pTimbre, &pEq, &pFx, &pMus };
    for (int i = 0; i < 5; ++i) { pages[i]->setVisible (i == page); tabs[i]->setToggleState (i == page, dontSendNotification); }
    repaint();
}

void PickGuitarEditor::timerCallback()
{
    vu.update (proc.meter.load(), 1.0f / 30.0f);
    const int pc = proc.engine.pickCount.load();
    if (pc != lastPickCount) { lastPickCount = pc; neck.hit (proc.engine.lastPick.load(), proc.engine.lastPickOpen.load() != 0); }
    scope.repaint();
    lcd.repaint();
    repaint (0, 152, kW, 48);   // luzes da contagem
    for (int i = 0; i < musButtons.size(); ++i) musButtons[i]->setToggleState (i == proc.getCurrentProgram(), dontSendNotification);
    if (page == 0) { grid.repaint(); neck.repaint(); map.repaint(); pLinha.repaint (map.getRight() - 400, map.getBottom(), 420, 40); repaint (pLinha.getBounds()); }
    if (page == 2) eq.repaint();
}

void PickGuitarEditor::paint (Graphics& g)
{
    g.fillAll (pg::chassis);
    g.drawImageAt (wood, 0, 0);
    // cabeçalho
    auto head = Rectangle<int> (26, 0, kW - 26, 84);
    g.setGradientFill (ColourGradient (Colour (0xff3b2a1c), 0, 0, Colour (0xff291d13), 0, 84, false));
    g.fillRect (head);
    g.setColour (Colours::black); g.drawHorizontalLine (84, 26, (float) kW);
    g.setGradientFill (ColourGradient (Colour (0xffffd27a), 0, 14, Colour (0xffc8702a), 0, 46, false));
    g.setFont (FontOptions (30.0f, Font::bold));
    g.drawText ("MASTOR PICK GUITAR", 44, 12, 400, 34, Justification::left);
    g.setColour (pg::muted);
    g.setFont (FontOptions (11.0f, Font::bold));
    g.drawText (String::fromUTF8 ("DUB STATION  ·  PICK GUITAR  ·  MPG-11  ·  v") + JucePlugin_VersionString, 46, 46, 420, 14, Justification::left);
    const Colour st[3] = { pg::red, pg::gold, pg::green };
    for (int i = 0; i < 3; ++i) { g.setColour (st[i]); g.fillRect (46 + i * 40, 64, 40, 6); }
    // visor
    g.setGradientFill (ColourGradient (Colour (0xff241a12), 0, 84, Colour (0xff1a120c), 0, 152, false));
    g.fillRect (26, 85, kW - 26, 67);
    g.setColour (pg::cream); g.setFont (FontOptions (10.5f, Font::bold));
    g.drawText ("ESTILO", 150, 96, 80, 20, Justification::left);
    g.drawText ("GUITARRISTA", 150, 122, 100, 20, Justification::left);
    // transporte
    g.setColour (Colour (0xff20160f)); g.fillRect (26, 152, kW - 26, 48);
    g.setColour (Colours::black); g.drawHorizontalLine (152, 26, (float) kW); g.drawHorizontalLine (200, 26, (float) kW);
    const int cur = proc.engine.curStep.load();
    for (int i = 0; i < 4; ++i)
    {
        auto c = Rectangle<float> (26, 26).withCentre ({ 352.0f + (float) i * 34.0f, 176.0f });
        const bool now = cur >= 0 && cur / 4 == i, sn = i == 1 || i == 3;
        g.setColour (now ? pg::amber : Colour (0xff2a1d14)); g.fillEllipse (c);
        g.setColour (sn ? pg::red : Colours::black); g.drawEllipse (c, 1.2f);
        g.setColour (now ? pg::ink : (sn ? Colour (0xffff9a86) : pg::muted));
        g.setFont (FontOptions (12.0f, Font::bold)); g.drawText (String (i + 1), c, Justification::centred);
    }
    g.setColour (pg::muted); g.setFont (FontOptions (12.0f));
    g.drawText (String::fromUTF8 ("Caixa no 2 e no 4 · segue o andamento da DAW quando ela toca; sem a DAW, use o PLAY"), 500, 160, 470, 32, Justification::left);
    // abas e painel
    g.setColour (Colour (0xff1a120c)); g.fillRect (26, 290, kW - 26, 40);
    auto pa = pageArea();
    g.setGradientFill (ColourGradient (pg::plate, 0, (float) pa.getY(), pg::plate2, 0, (float) pa.getBottom(), false));
    g.fillRect (pa);
    // grupos de cada página
    auto group = [&] (Rectangle<int> r, String title, Colour bg = Colours::black.withAlpha (0.18f), Colour tc = pg::amber)
    {
        r = r.translated (pa.getX(), pa.getY());
        g.setColour (bg); g.fillRoundedRectangle (r.toFloat(), 8.0f);
        g.setColour (pg::line); g.drawRoundedRectangle (r.toFloat(), 8.0f, 1.0f);
        g.setColour (tc); g.setFont (FontOptions (11.0f, Font::bold));
        g.drawText (title, r.getX() + 12, r.getY() + 8, r.getWidth() - 24, 14, Justification::left);
    };
    auto hint = [&] (int x, int y, int w, String s)
    { g.setColour (pg::muted); g.setFont (FontOptions (12.5f)); g.drawFittedText (s, pa.getX() + x, pa.getY() + y, w, 34, Justification::topLeft, 2); };
    if (page == 0)
    {
        group ({ 16, 196, 330, 190 }, "TOQUE");
        group ({ 356, 196, 722, 190 }, "NOTAS");
        g.setColour (pg::muted); g.setFont (FontOptions (12.0f, Font::bold));
        g.drawText ("Dose do pica-pau  " + String (proc.get ("dose"), 1) + "%", pa.getX() + 368, pa.getY() + 290, 220, 16, Justification::left);
        g.setColour (Colour (0xff9cc0ff)); g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 11.0f, Font::plain));
        g.drawFittedText (map.info(), pa.getX() + 600, pa.getY() + 362, 472, 16, Justification::right, 1, 0.9f);
    }
    if (page == 1)
    {
        hint (16, 62, 1060, String::fromUTF8 ("Corda sintetizada (Karplus-Strong). O ABAFADO é a mão na ponte: quanto mais alto, mais curta e escura a nota. "
                                              "Arraste os knobs para cima ou para baixo; duplo clique volta ao padrão de fábrica."));
        group ({ 16, 104, 300, 150 }, "CORDA E PALHETA");
        group ({ 330, 104, 300, 150 }, "AMP");
    }
    if (page == 2)
    {
        auto r = Rectangle<int> (870, 56, 208, 300).translated (pa.getX(), pa.getY());
        g.setGradientFill (ColourGradient (Colour (0xff2a1d14), 0, (float) r.getY(), Colour (0xff1a120c), 0, (float) r.getBottom(), false));
        g.fillRoundedRectangle (r.toFloat(), 10.0f);
        g.setColour (Colours::black); g.drawRoundedRectangle (r.toFloat(), 10.0f, 1.0f);
        g.setColour (pg::amber); g.setFont (FontOptions (15.0f, Font::bold));
        g.drawText ("MASTOR BIG KNOB", r.getX(), r.getY() + 12, r.getWidth(), 18, Justification::centred);
        g.setColour (pg::muted); g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 11.0f, Font::plain));
        g.drawText (String::fromUTF8 ("◀ LPF"), r.getX() + 12, r.getBottom() - 26, 80, 16, Justification::left);
        g.drawText (String::fromUTF8 ("HPF ▶"), r.getRight() - 92, r.getBottom() - 26, 80, 16, Justification::right);
        g.setColour (pg::cream); g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 12.0f, Font::plain));
        g.drawText (eq.readout.isEmpty() ? String::fromUTF8 ("Arraste os pontos: frequência para os lados, ganho para cima e para baixo (HPF/LPF: ressonância). Rodinha = Q.") : eq.readout,
                    pa.getX() + 16, pa.getY() + 362, 840, 18, Justification::left);
    }
    if (page == 3)
    {
        hint (16, 4, 1060, String::fromUTF8 ("Padrão de fábrica: saturador e Tape Echo começam em bypass. A ordem é amp → EQ → Big Knob → saturador → eco e mola."));
        auto sat = Rectangle<int> (16, 28, 1062, 150).translated (pa.getX(), pa.getY());
        g.setGradientFill (ColourGradient (Colour (0xff6b2f22), 0, (float) sat.getY(), Colour (0xff4a1e15), 0, (float) sat.getBottom(), false));
        g.fillRoundedRectangle (sat.toFloat(), 10.0f);
        g.setColour (Colours::black); g.drawRoundedRectangle (sat.toFloat(), 10.0f, 2.0f);
        g.setColour (Colour (0xfff6e4d0)); g.setFont (FontOptions (22.0f, Font::bold));
        g.drawText ("MASTOR SATURADOR", sat.getX() + 16, sat.getY() + 12, 400, 26, Justification::left);
        g.setColour (Colour (0xffe2bfa8)); g.setFont (FontOptions (10.0f, Font::bold));
        g.drawText (String::fromUTF8 ("FITA · VÁLVULA · TRANSISTOR"), sat.getX() + 16, sat.getY() + 38, 400, 14, Justification::left);
        auto ec = Rectangle<int> (16, 190, 1062, 196).translated (pa.getX(), pa.getY());
        g.setGradientFill (ColourGradient (Colour (0xff5d6b4f), 0, (float) ec.getY(), Colour (0xff46523b), 0, (float) ec.getBottom(), false));
        g.fillRoundedRectangle (ec.toFloat(), 10.0f);
        g.setColour (Colours::black); g.drawRoundedRectangle (ec.toFloat(), 10.0f, 2.0f);
        g.setColour (Colour (0xfff6efd9)); g.setFont (FontOptions (22.0f, Font::bold));
        g.drawText ("MASTOR TAPE ECHO", ec.getX() + 16, ec.getY() + 12, 400, 26, Justification::left);
        g.setColour (Colour (0xffd7d0b8)); g.setFont (FontOptions (10.0f, Font::bold));
        g.drawText (String::fromUTF8 ("SLAPBACK · FITA · MOLA"), ec.getX() + 16, ec.getY() + 38, 400, 14, Justification::left);
        // rolos e visor do tempo
        const double bpmNow = proc.get ("bpm");
        const int ms = roundToInt (mpg::echoSeconds ((int) proc.get ("echoSync"), bpmNow) * 1000.0);
        auto lcdR = Rectangle<float> (ec.getRight() - 230.0f, ec.getY() + 16.0f, 120.0f, 32.0f);
        for (float cx : { lcdR.getX() - 40.0f, lcdR.getRight() + 40.0f })
        {
            auto rr = Rectangle<float> (58, 58).withCentre ({ cx, lcdR.getCentreY() });
            g.setColour (Colour (0xff2a2a2a)); g.fillEllipse (rr);
            g.setColour (Colours::black); g.drawEllipse (rr, 2.0f);
            g.setColour (Colour (0xff777777)); g.fillEllipse (rr.reduced (24));
        }
        g.setColour (Colour (0xff0e1a0e)); g.fillRoundedRectangle (lcdR, 4.0f);
        g.setColour (Colour (0xff8fff8f)); g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 15.0f, Font::plain));
        g.drawText (String (ms) + " ms", lcdR, Justification::centred);
        g.setColour (Colour (0xffe4ddc6)); g.setFont (FontOptions (12.0f));
        g.drawText (String::fromUTF8 ("Segure o THROW para mandar tudo pro eco (o tempo do visor usa o BPM sem DAW; na DAW segue o andamento dela)."),
                    ec.getX() + 420, ec.getBottom() - 40, 630, 30, Justification::left);
    }
    if (page == 4)
    {
        hint (16, 8, 1060, String::fromUTF8 ("Cada preset carrega modo, grade, desenho do counter, timbre e eco no espírito de um guitarrista. "
                                             "São padrões originais inspirados no jeito de tocar de cada um, não transcrições nem gravações."));
        for (int e = 0; e < mpg::kNumEras; ++e)
        {
            g.setColour (pg::muted); g.setFont (FontOptions (11.0f, Font::bold));
            g.drawText (upper (String::fromUTF8 (mpg::eraName (e))), pa.getX() + 16 + e * 356, pa.getY() + 52, 340, 14, Justification::left);
        }
        const auto& P = mpg::presets()[(size_t) proc.getCurrentProgram()];
        group ({ 16, 250, 1062, 130 }, upper (String::fromUTF8 (P.name)));
        g.setColour (pg::cream); g.setFont (FontOptions (15.0f));
        g.drawFittedText (String::fromUTF8 (P.card), pa.getX() + 30, pa.getY() + 280, 1030, 80, Justification::topLeft, 3);
    }
    // barra do teclado
    g.setColour (Colour (0xff1a120c)); g.fillRect (26, 724, kW - 26, 22);
    g.setColour (Colours::black); g.drawHorizontalLine (724, 26, (float) kW);
    g.setColour (pg::muted); g.setFont (FontOptions (12.0f));
    const int ln = proc.engine.latchNote.load();
    g.drawText (String::fromUTF8 ("ENTRADA DO BAIXO — toque aqui, no teclado MIDI ou mande o MIDI do Mastor Reggae Bass MRB-69 / MRM-0420")
                + (ln >= 0 ? String::fromUTF8 ("  ·  nota: ") + mpg::noteName (ln) + String (ln / 12 - 1) : String()),
                40, 726, kW - 60, 18, Justification::left);
}

void PickGuitarEditor::resized()
{
    // cabeçalho
    key.setBounds (480, 18, 70, 42);
    scale.setBounds (560, 18, 170, 42);
    vu.setBounds (800, 8, 140, 68);
    master.setBounds (950, 2, 80, 80);
    power.setBounds (1046, 18, 60, 46);
    eraPrev.setBounds (44, 94, 44, 24); eraNext.setBounds (94, 94, 44, 24);
    musPrev.setBounds (44, 122, 44, 24); musNext.setBounds (94, 122, 44, 24);
    lcd.setBounds (260, 90, kW - 276, 58);
    play.setBounds (44, 160, 110, 32);
    bpm.setBounds (162, 160, 150, 32);
    guide.setBounds (980, 160, 124, 32);
    scope.setBounds (44, 206, kW - 60, 80);
    for (int i = 0; i < tabs.size(); ++i) tabs[i]->setBounds (40 + i * 190, 296, 182, 30);
    layoutPages();
    keyboard.setBounds (26, 746, kW - 26, kH - 746);
    keyboard.setKeyWidth ((float) (kW - 26) / 28.0f);
}

void PickGuitarEditor::layoutPages()
{
    const auto pa = pageArea();
    for (auto* c : std::initializer_list<Component*> { &pLinha, &pTimbre, &pEq, &pFx, &pMus }) c->setBounds (pa);

    // LINHA
    mode.setBounds (16, 18, 300, 36);
    src.setBounds (326, 10, 250, 44);
    line.setBounds (586, 10, 180, 44);
    oct.setBounds (776, 10, 140, 44);
    shape.setBounds (926, 10, 152, 44);
    grid.setBounds (16, 62, 1062, 70);
    gClear.setBounds (16, 140, 130, 28); gFill.setBounds (152, 140, 140, 28); gReset.setBounds (298, 140, 190, 28);
    neck.setBounds (500, 138, 578, 50);
    const int ky = 226;
    gate.setBounds (24, ky, 76, 96); human.setBounds (104, ky, 76, 96); swing.setBounds (184, ky, 76, 96); acc.setBounds (264, ky, 76, 96);
    art.setBounds (368, 224, 220, 30);
    rep.setBounds (368, 260, 220, 24);
    dose.setBounds (368, 306, 220, 22);
    ppClear.setBounds (368, 338, 220, 28);
    ppSwing.setBounds (598, 226, 80, 96);
    map.setBounds (688, 214, 380, 144);

    // TIMBRE
    pickup.setBounds (16, 16, 600, 38);
    mute.setBounds (32, 134, 86, 110); pos.setBounds (122, 134, 86, 110); atk.setBounds (212, 134, 86, 110);
    drive.setBounds (346, 134, 86, 110); tone.setBounds (436, 134, 86, 110); body.setBounds (526, 134, 86, 110);

    // EQ
    eqOn.setBounds (16, 10, 210, 34);
    for (int i = 0; i < eqPresets.size(); ++i) eqPresets[i]->setBounds (240 + i * 138, 10, 130, 34);
    eq.setBounds (16, 56, 840, 300);
    bigKnob.setBounds (890, 96, 168, 220);

    // SATURADOR + TAPE ECHO
    satOn.setBounds (850, 40, 228, 36);
    satType.setBounds (32, 104, 360, 34);
    satDrive.setBounds (430, 74, 86, 100); satMix.setBounds (520, 74, 86, 100); satOut.setBounds (610, 74, 86, 100);
    echoSync.setBounds (32, 262, 180, 44);
    echoFb.setBounds (240, 246, 86, 100); echoMix.setBounds (330, 246, 86, 100); spring.setBounds (420, 246, 86, 100);
    echoOn.setBounds (32, 336, 228, 36);
    throwBtn.setBounds (530, 258, 240, 40);

    // GUITARRISTAS
    int col[3] = { 0, 0, 0 };
    for (int i = 0; i < musButtons.size(); ++i)
    {
        const int e = mpg::presets()[(size_t) i].era;
        musButtons[i]->setBounds (16 + e * 356, 70 + col[e] * 50, 340, 42);
        ++col[e];
    }
}
