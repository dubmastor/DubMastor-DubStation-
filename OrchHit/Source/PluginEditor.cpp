#include "PluginEditor.h"

using namespace juce;

namespace col
{
const Colour bg { 0xff1d1d20 }, panel { 0xff29292d }, line { 0xff3c3c42 }, lcd { 0xffa9c46c }, lcdInk { 0xff1c2a10 },
             red { 0xffd7262e }, text { 0xffecebe6 }, mute { 0xffa19e97 }, gridBg { 0xffe9ecdc }, key { 0xfff7f6f2 };
}

static Font mono (float size, bool bold = true) { return Font (FontOptions (Font::getDefaultMonospacedFontName(), size, bold ? Font::bold : Font::plain)); }
static Font sans (float size, bool bold = false) { return Font (FontOptions (size, bold ? Font::bold : Font::plain)); }
static String u8 (const char* s) { return String::fromUTF8 (s); }

// ================================================================== LookAndFeel
HitLookAndFeel::HitLookAndFeel()
{
    setColour (PopupMenu::backgroundColourId, col::panel);
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedBackgroundColourId, col::red);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxTextColourId, col::text);
}

void HitLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const String kind = b.getProperties()["kind"].toString();
    const bool on = b.getToggleState();
    Colour fill = on ? col::red : Colour (0xff222226), edge = on ? Colour (0xffff7a7f) : col::line;
    if (kind == "tab") { fill = on ? col::red : col::panel; }
    if (over && b.isEnabled()) fill = fill.brighter (0.1f);
    if (down) fill = fill.darker (0.2f);
    g.setColour (fill); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (edge); g.drawRoundedRectangle (r, 6.0f, 1.2f);
    if (kind == "led")
    {   // LED dos efeitos
        auto led = Rectangle<float> (10, 10).withCentre ({ r.getRight() - 14.0f, r.getCentreY() });
        g.setColour (on ? Colour (0xffffe0e0) : Colour (0xff5a5750)); g.fillEllipse (led);
    }
}

void HitLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    const String kind = b.getProperties()["kind"].toString();
    g.setColour (b.getToggleState() ? Colours::white : col::text.withAlpha (b.isEnabled() ? 1.0f : 0.4f));
    g.setFont (mono (kind == "tab" ? 13.0f : 11.5f));
    auto r = b.getLocalBounds().reduced (6, 2);
    if (kind == "led") r.removeFromRight (18);
    g.drawFittedText (b.getButtonText(), r, Justification::centred, 2, 0.8f);
}

void HitLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider&)
{
    const float d = (float) jmin (w, h) - 8.0f, cx = x + w * 0.5f, cy = y + h * 0.5f;
    auto r = Rectangle<float> (d, d).withCentre ({ cx, cy });
    Path arc; arc.addCentredArc (cx, cy, d * 0.5f, d * 0.5f, 0, a0, a0 + pos * (a1 - a0), true);
    g.setColour (col::line); g.drawEllipse (r, 3.0f);
    g.setColour (col::red); g.strokePath (arc, PathStrokeType (3.0f));
    g.setColour (Colour (0xff111114)); g.fillEllipse (r.reduced (6.0f));
    const float ang = a0 + pos * (a1 - a0);
    Path p; p.addRectangle (-1.5f, -d * 0.36f, 3.0f, d * 0.22f);
    g.setColour (Colours::white); g.fillPath (p, AffineTransform::rotation (ang).translated (cx, cy));
}

Font HitLookAndFeel::getComboBoxFont (ComboBox&) { return mono (12.0f); }
Font HitLookAndFeel::getPopupMenuFont() { return sans (14.0f); }

// ================================================================== LCD
void HitLcd::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::lcd); g.fillRoundedRectangle (r, 8.0f);
    g.setColour (Colours::black.withAlpha (0.25f)); g.drawRoundedRectangle (r.reduced (1.0f), 8.0f, 2.0f);
    const int t = (int) proc.get ("timbre");
    const auto& p = moh::presets()[(size_t) jlimit (0, moh::numPresets() - 1, t)];
    const auto& s = proc.sample (t);
    auto in = r.reduced (14.0f, 10.0f);
    g.setColour (col::lcdInk);
    g.setFont (mono (20.0f)); g.drawText (u8 (p.lcd), in.removeFromTop (24.0f).toNearestInt(), Justification::centredLeft);
    g.drawText (note, Rectangle<float> (in.getRight() - 60.0f, in.getY() - 24.0f, 60.0f, 24.0f).toNearestInt(), Justification::centredRight);
    g.setFont (mono (13.0f, false));
    g.drawText (u8 (p.info) + String::fromUTF8 (" · ") + String ((int) (s.size() * 1000 / 44100)) + " ms", in.removeFromTop (20.0f).toNearestInt(), Justification::centredLeft);
    auto w = in.reduced (0, 4);
    const float mid = w.getCentreY(), hh = w.getHeight() * 0.48f;
    const int cols = (int) w.getWidth();
    for (int i = 0; i < cols && ! s.empty(); ++i)
    {
        const size_t a = (size_t) i * s.size() / (size_t) cols, b = std::max (a + 1, (size_t) (i + 1) * s.size() / (size_t) cols);
        float mn = 1, mx = -1; for (size_t k = a; k < b && k < s.size(); ++k) { mn = std::min (mn, s[k]); mx = std::max (mx, s[k]); }
        g.fillRect (w.getX() + i, mid - mx * hh, 1.0f, std::max (1.0f, (mx - mn) * hh));
    }
}

// ================================================================== envelope
EnvelopeEditor::Geo EnvelopeEditor::geo() const
{
    Geo q; q.pad = 16; q.x0 = q.pad; const float iw = getWidth() - 2 * q.pad;
    q.wA = iw * .2f; q.wD = iw * .33f; q.wS = iw * .15f; q.wR = iw * .32f;
    q.xA = q.x0 + q.wA * std::sqrt (proc.get ("attack") / .3f);
    q.xD = q.xA + q.wD * std::sqrt (proc.get ("decay"));
    q.xS = q.xD + q.wS;
    q.xR = q.xS + q.wR * std::sqrt (proc.get ("release"));
    q.top = q.pad + 4; q.h = getHeight() - 2 * q.pad - 24;
    return q;
}

void EnvelopeEditor::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::lcd); g.fillRoundedRectangle (r, 8.0f);
    const auto q = geo(); const float S = proc.get ("sustain");
    g.setColour (col::lcdInk.withAlpha (0.15f));
    for (float l : { .25f, .5f, .75f }) g.drawHorizontalLine ((int) q.y (l), q.pad, getWidth() - q.pad);
    Path p; p.startNewSubPath (q.x0, q.y (0)); p.lineTo (q.xA, q.y (1)); p.lineTo (q.xD, q.y (S)); p.lineTo (q.xS, q.y (S)); p.lineTo (q.xR, q.y (0));
    Path fill (p); fill.closeSubPath();
    g.setColour (col::lcdInk.withAlpha (0.18f)); g.fillPath (fill);
    g.setColour (col::lcdInk); g.strokePath (p, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setFont (mono (13.0f, false)); g.setColour (col::lcdInk.withAlpha (0.6f));
    g.drawText (u8 ("segura a tecla"), Rectangle<float> (q.xD, q.y (S) + 4, q.xS - q.xD + 40, 18).toNearestInt(), Justification::centredLeft);
    const std::pair<Point<float>, const char*> h[3] = { { { q.xA, q.y (1) }, "A" }, { { q.xD, q.y (S) }, "D/S" }, { { q.xR, q.y (0) }, "R" } };
    for (int i = 0; i < 3; ++i)
    {
        const float rr = drag == i ? 11.0f : 8.0f;
        g.setColour (col::red); g.fillEllipse (Rectangle<float> (2 * rr, 2 * rr).withCentre (h[i].first));
        g.setColour (col::lcdInk); g.drawEllipse (Rectangle<float> (2 * rr, 2 * rr).withCentre (h[i].first), 2.0f);
        g.setFont (mono (14.0f)); g.drawText (h[i].second, Rectangle<float> (h[i].first.x + 12, std::max (2.0f, h[i].first.y - 22), 40, 18).toNearestInt(), Justification::centredLeft);
    }
    auto ms = [] (float s) { return s < 1.0f ? String (roundToInt (s * 1000)) + " ms" : String (s, 2) + " s"; };
    g.setFont (mono (14.0f)); g.setColour (col::lcdInk);
    g.drawText ("A " + ms (proc.get ("attack")) + u8 (" · D ") + ms (proc.get ("decay")) + u8 (" · S ") + String (roundToInt (S * 100)) + u8 ("% · R ") + ms (proc.get ("release")),
                getLocalBounds().removeFromBottom (24).reduced (14, 0), Justification::centredLeft);
}

void EnvelopeEditor::gesture (bool begin)
{
    const char* ids[3][2] = { { "attack", nullptr }, { "decay", "sustain" }, { "release", nullptr } };
    if (drag < 0) return;
    for (auto id : ids[drag])
        if (id != nullptr) if (auto* p = proc.apvts.getParameter (id)) { if (begin) p->beginChangeGesture(); else p->endChangeGesture(); }
}

void EnvelopeEditor::mouseDown (const MouseEvent& e)
{
    const auto q = geo(); const float S = proc.get ("sustain");
    const Point<float> h[3] = { { q.xA, q.y (1) }, { q.xD, q.y (S) }, { q.xR, q.y (0) } };
    float best = 40; drag = -1;
    for (int i = 0; i < 3; ++i) if (h[i].getDistanceFrom (e.position) < best) { best = h[i].getDistanceFrom (e.position); drag = i; }
    gesture (true); repaint();
}

void EnvelopeEditor::mouseDrag (const MouseEvent& e)
{
    if (drag < 0) return;
    const auto q = geo(); const float x = e.position.x, y = e.position.y;
    auto sq = [] (float v) { return v * v * (v < 0 ? -1.0f : 1.0f); };
    if (drag == 0) proc.setParam ("attack", jlimit (.001f, .3f, sq ((x - q.x0) / q.wA) * .3f));
    if (drag == 1) { proc.setParam ("decay", jlimit (.01f, 1.0f, sq ((x - q.xA) / q.wD))); proc.setParam ("sustain", jlimit (0.0f, 1.0f, 1 - (y - q.top) / q.h)); }
    if (drag == 2) proc.setParam ("release", jlimit (.01f, 1.0f, sq ((x - q.xS) / q.wR)));
    repaint();
}

void EnvelopeEditor::mouseUp (const MouseEvent&) { gesture (false); drag = -1; repaint(); }

// ================================================================== miniaturas
void Thumb::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (col::panel); g.fillRoundedRectangle (r, 8.0f);
    g.setColour (selected ? col::red : col::line); g.drawRoundedRectangle (r, 8.0f, selected ? 2.5f : 1.0f);
    auto in = r.reduced (8.0f, 5.0f);
    g.setColour (col::text); g.setFont (mono (11.5f)); g.drawFittedText (title, in.removeFromTop (16).toNearestInt(), Justification::centredLeft, 1, 0.8f);
    g.setColour (col::mute); g.setFont (sans (11.0f)); g.drawFittedText (info, in.removeFromTop (15).toNearestInt(), Justification::centredLeft, 1, 0.8f);
    auto d = in.withTrimmedTop (3);
    g.setColour (col::lcd); g.fillRoundedRectangle (d, 4.0f);
    int lo = 127, hi = 0, end = 64;
    for (auto& n : notes) { lo = std::min (lo, n.note); hi = std::max (hi, n.note); end = std::max (end, n.tick + n.len); }
    const int bars = relative ? (end + 63) / 64 : ticks / 64, total = bars * 64;
    if (notes.empty()) { lo = 0; hi = 0; }
    lo -= 1; hi += 1;
    for (int b = 0; b <= bars * 4; ++b)
    {
        g.setColour (col::lcdInk.withAlpha (b % 4 == 0 ? 0.45f : 0.15f));
        g.fillRect (d.getX() + d.getWidth() * b / (bars * 4), d.getY(), b % 4 == 0 ? 1.2f : 0.8f, d.getHeight());
    }
    g.setColour (col::red);
    const float rh = d.getHeight() / (float) (hi - lo + 1);
    for (auto& n : notes)
        g.fillRect (d.getX() + d.getWidth() * n.tick / total, d.getY() + (hi - n.note) * rh, std::max (2.5f, d.getWidth() * n.len / total), std::max (2.5f, rh * 0.8f));
}

void Thumb::mouseUp (const MouseEvent& e)
{
    if (dragged) return;
    if (e.mods.isPopupMenu()) { if (onRight) onRight(); return; }
    if (e.getNumberOfClicks() == 1 && onClick) onClick();
}

// ================================================================== grade
int GridEditor::noteAt (Point<float> p) const
{
    const auto g = proc.getGrid();
    for (int i = (int) g.size() - 1; i >= 0; --i)
    {
        const auto& n = g[(size_t) i];
        Rectangle<float> r (xOf (n.tick), area().getY() + (hi - n.note) * rowH(), std::max (5.0f, xOf (n.tick + n.len) - xOf (n.tick)), rowH());
        if (r.expanded (3, 1).contains (p)) return i;
    }
    return -1;
}

void GridEditor::paint (Graphics& g)
{
    auto a = area();
    g.setColour (col::gridBg); g.fillRect (getLocalBounds());
    const float rh = rowH();
    for (int p = hi; p >= lo; --p)
    {
        const float y = a.getY() + (hi - p) * rh;
        const int pc = p % 12; const bool black = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
        g.setColour (black ? Colour (0xff2a2a2c) : col::key); g.fillRect (0.0f, y, 36.0f, rh - 0.5f);
        if (black) { g.setColour (Colours::black.withAlpha (0.05f)); g.fillRect (a.getX(), y, a.getWidth(), rh); }
        if (p % 12 == 0 || p == moh::kRootDefault)
        {
            g.setColour (black ? Colours::white : Colour (0xff333333)); g.setFont (mono (9.5f, false));
            g.drawText (OrchHitProcessor::noteName (p), Rectangle<float> (2, y - 1, 34, rh + 2).toNearestInt(), Justification::centredLeft);
        }
    }
    const float tw = a.getWidth() / moh::kTotalTicks;
    const int step = snap * tw >= 5 ? snap : 16;
    for (int t = 0; t <= moh::kTotalTicks; t += step)
    {
        const bool bar = t % 64 == 0, beat = t % 16 == 0;
        g.setColour (col::lcdInk.withAlpha (bar ? 0.55f : beat ? 0.25f : 0.08f));
        g.fillRect (xOf (t), a.getY(), bar ? 2.0f : 1.0f, a.getHeight());
    }
    for (int b = 0; b < 4; ++b)
    {
        // caixa no 2 e no 4
        for (int beat : { 1, 3 }) { g.setColour (col::red.withAlpha (0.5f)); g.fillRect (xOf (b * 64 + beat * 16), a.getY(), xOf (16) - xOf (0), 3.0f); }
        g.setColour (col::lcdInk.withAlpha (0.6f)); g.setFont (mono (10.0f)); g.drawText (String (b + 1), Rectangle<float> (xOf (b * 64) + 4, a.getY() + 3, 20, 12).toNearestInt(), Justification::centredLeft);
    }
    for (auto& n : proc.getGrid())
    {
        Rectangle<float> r (xOf (n.tick), a.getY() + (hi - n.note) * rh + 0.5f, std::max (6.0f, xOf (n.tick + n.len) - xOf (n.tick)), rh - 1.0f);
        g.setColour (col::red); g.fillRoundedRectangle (r, 2.0f);
        g.setColour (Colour (0xff6d0f14)); g.drawRoundedRectangle (r, 2.0f, 1.0f);
    }
    if (hover.x >= 0)
    {   // onde o MIDI arrastado vai cair
        const int t0 = jlimit (0, moh::kTotalTicks - 16, (int) (tickAt (hover.x) / 16) * 16), p = jlimit (lo, hi, pitchAt (hover.y));
        g.setColour (col::red.withAlpha (0.25f)); g.fillRect (xOf (t0), a.getY() + (hi - p) * rh, xOf (64) - xOf (0), rh);
        g.setColour (col::lcdInk); g.setFont (mono (11.0f));
        g.drawText (u8 ("compasso ") + String (t0 / 64 + 1) + ", base " + OrchHitProcessor::noteName (p), Rectangle<float> (xOf (t0) + 4, a.getY() + (hi - p) * rh - 16, 220, 14).toNearestInt(), Justification::centredLeft);
    }
    const double pb = proc.playBeat.load();
    if (pb >= 0) { g.setColour (col::lcdInk); g.fillRect (xOf (pb * 16), a.getY(), 2.0f, a.getHeight()); }
}

void GridEditor::commit (const moh::Grid& g)
{
    history.push_back (proc.getGrid());
    if (history.size() > 60) history.erase (history.begin());
    proc.setGrid (g, true);
    repaint();
}

void GridEditor::undo()
{
    if (history.empty()) return;
    proc.setGrid (history.back(), true); history.pop_back(); repaint();
}

void GridEditor::mouseDown (const MouseEvent& e)
{
    work = proc.getGrid(); moved = false; dragIdx = -1;
    if (! area().contains (e.position)) return;
    const int i = noteAt (e.position);
    if (e.mods.isPopupMenu() || erase)
    {
        if (i >= 0) { auto g = work; g.erase (g.begin() + i); commit (g); }
        return;
    }
    if (i >= 0)
    {
        history.push_back (work);
        dragIdx = i; start = work[(size_t) i];
        resizing = e.position.x >= xOf (start.tick + start.len) - 2.0f;
        return;
    }
    const int p = pitchAt (e.position.y), len = moh::lengthTicks ((int) proc.get ("noteLen"));
    if (p < lo || p > hi) return;
    const int t = jlimit (0, moh::kTotalTicks - 1, (int) (tickAt (e.position.x) / snap) * snap);
    auto g = work; g.push_back ({ t, std::min (len > 0 ? len : snap, moh::kTotalTicks - t), p });
    commit (g);
    proc.keyboardState.noteOn (1, p, 0.8f); proc.keyboardState.noteOff (1, p, 0.0f);
}

void GridEditor::mouseDrag (const MouseEvent& e)
{
    if (dragIdx < 0 || dragIdx >= (int) work.size()) return;
    const float tw = area().getWidth() / moh::kTotalTicks;
    const double dt = e.getDistanceFromDragStartX() / tw;
    auto& n = work[(size_t) dragIdx];
    if (std::abs (e.getDistanceFromDragStartX()) + std::abs (e.getDistanceFromDragStartY()) > 3) moved = true;
    if (resizing)
    {
        const int st = start.len < snap ? 1 : snap;
        n.len = jlimit (1, moh::kTotalTicks - n.tick, (int) std::round ((start.len + dt) / st) * st);
    }
    else
    {
        n.tick = jlimit (0, moh::kTotalTicks - n.len, (int) std::round ((start.tick + dt) / snap) * snap);
        const int np = jlimit (lo, hi, start.note - (int) std::round (e.getDistanceFromDragStartY() / rowH()));
        if (np != n.note) { n.note = np; proc.keyboardState.noteOn (1, np, 0.7f); proc.keyboardState.noteOff (1, np, 0.0f); }
    }
    proc.setGrid (work, true);
    repaint();
}

void GridEditor::mouseUp (const MouseEvent&)
{
    if (dragIdx >= 0 && ! moved && ! history.empty()) history.pop_back();
    dragIdx = -1;
}

void GridEditor::itemDropped (const SourceDetails& d)
{
    const auto p = d.localPosition.toFloat();
    hover = { -1, -1 };
    if (onDrop && area().contains (p))
        onDrop (d.description.toString(), jlimit (0, moh::kTotalTicks - 16, (int) (tickAt (p.x) / 16) * 16), jlimit (lo, hi, pitchAt (p.y)));
    repaint();
}

// ================================================================== arraste para a DAW
void MidiDragZone::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (Colour (0xff2f1b1c)); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::red); g.drawRoundedRectangle (r, 6.0f, 1.5f);
    g.setColour (col::text); g.setFont (mono (11.5f));
    g.drawFittedText (u8 ("⠿ ARRASTE O MIDI PARA A DAW"), getLocalBounds().reduced (6, 2), Justification::centred, 2);
}

void MidiDragZone::mouseDrag (const MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    const auto f = File::getSpecialLocation (File::tempDirectory).getChildFile ("MOH-88 grade " + String (roundToInt (proc.get ("bpm"))) + "bpm.mid");
    if (proc.writeMidiFile (f)) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
}

// ================================================================== editor
OrchHitEditor::OrchHitEditor (OrchHitProcessor& p)
    : AudioProcessorEditor (p), proc (p), lcd (p), env (p), gridEd (p), dragZone (p),
      keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    chip (tabSom, "SOM", [this] { setPage (0); }, "tab");
    chip (tabMidi, "MIDIs E GRADE", [this] { setPage (1); }, "tab");
    addAndMakeVisible (lcd);

    // ---- SOM
    for (int i = 0; i < moh::numPresets(); ++i)
    {
        const auto& pr = moh::presets()[(size_t) i];
        chip (timbreBtn[i], u8 (pr.lcd), [this, i] { proc.setParam ("timbre", (float) i); });
        timbreBtn[i].setTooltip (u8 (pr.info));
    }
    addAndMakeVisible (env);
    struct EP { float a, d, s, r; };
    static const EP eps[3] = { { .002f, .18f, .35f, .04f }, { .002f, .35f, .7f, .12f }, { .01f, .6f, .9f, .3f } };
    const char* epNames[3] = { "STAB CURTO", "MÉDIO", "SEGURADO" };
    for (int i = 0; i < 3; ++i)
        chip (envPreset[i], u8 (epNames[i]), [this, i] {
            const EP& e = eps[i];
            proc.setParam ("attack", e.a); proc.setParam ("decay", e.d); proc.setParam ("sustain", e.s); proc.setParam ("release", e.r); env.repaint(); });
    knob (vol, "vol", "VOLUME"); knob (rev, "reverb", "REVERB PLATE"); knob (hand, "hand", u8 ("MÃO DIREITA"));
    chip (handsBtn, u8 ("DUAS MÃOS"), nullptr, "led"); chip (echoBtn, "TAPE ECHO", nullptr, "led"); chip (satBtn, "SATURADOR", nullptr, "led");
    for (auto* b : { &handsBtn, &echoBtn, &satBtn }) b->setClickingTogglesState (true);
    handsAtt = std::make_unique<BA> (proc.apvts, "twoHands", handsBtn);
    echoAtt = std::make_unique<BA> (proc.apvts, "echoOn", echoBtn);
    satAtt = std::make_unique<BA> (proc.apvts, "satOn", satBtn);

    // ---- MIDIs E GRADE
    for (int i = 0; i < 5; ++i)
    {
        auto& t = grooveThumb[i];
        const auto& G = moh::grooves()[(size_t) i];
        t.title = u8 (G.name); t.info = u8 (G.info); t.notes = moh::grooveGrid (i); t.relative = false;
        t.onClick = [this, i] {
            gridEd.commit (moh::grooveGrid (i, std::max (1, moh::lengthTicks ((int) proc.get ("noteLen")))));
            proc.loadGroove (i);
            if (proc.get ("preview") < 0.5f && proc.get ("machineOn") < 0.5f) proc.setParam ("preview", 1.0f);   // como no celular: já começa a tocar
        };
        addAndMakeVisible (t);
    }
    rebuildClips();
    chip (previewBtn, "PREVIEW", nullptr); chip (dawBtn, "TOCA COM A DAW", nullptr); chip (drumsBtn, "BATERIA", nullptr);
    for (auto* b : { &previewBtn, &dawBtn, &drumsBtn }) b->setClickingTogglesState (true);
    previewAtt = std::make_unique<BA> (proc.apvts, "preview", previewBtn);
    dawAtt = std::make_unique<BA> (proc.apvts, "machineOn", dawBtn);
    drumsAtt = std::make_unique<BA> (proc.apvts, "drums", drumsBtn);
    knob (bpm, "bpm", "BPM");
    bpm.s.setSliderStyle (Slider::LinearBar); bpm.s.setColour (Slider::trackColourId, col::red.withAlpha (0.6f));
    bpm.s.setColour (Slider::backgroundColourId, Colour (0xff222226)); bpm.s.setTextValueSuffix (" BPM");
    bpm.s.setTextBoxStyle (Slider::TextBoxLeft, false, 150, 32);
    chip (drawBtn, u8 ("✎ DESENHAR"), [this] { gridEd.erase = false; });
    chip (eraseBtn, "APAGAR", [this] { gridEd.erase = true; });
    chip (undoBtn, "DESFAZER", [this] { gridEd.undo(); });
    chip (clearBtn, "LIMPAR", [this] { gridEd.commit ({}); });
    chip (saveBtn, "SALVAR COMO MIDI", [this] {
        auto g = proc.getGrid(); if (g.empty()) return;
        std::vector<moh::GridNote> rel; for (auto n : g) rel.push_back ({ n.tick, n.len, n.note - moh::kRootDefault });
        if (proc.userClips.size() >= 6) proc.userClips.erase (proc.userClips.begin());
        proc.userClips.push_back ({ "MEU MIDI " + String ((int) proc.userClips.size() + 1), rel });
        rebuildClips(); resized(); });
    const int snaps[3] = { 4, 2, 1 }; const char* snapNames[3] = { "1/16", "1/32", "1/64" };
    for (int i = 0; i < 3; ++i) chip (snapBtn[i], snapNames[i], [this, i, snaps] { gridEd.snap = snaps[i]; gridEd.repaint(); });
    addAndMakeVisible (gridEd);
    gridEd.onDrop = [this] (const String& desc, int t0, int root) {
        const int i = desc.fromFirstOccurrenceOf (":", false, false).getIntValue();
        if (desc.startsWith ("clip:") && i >= 0 && i < (int) moh::clips().size()) placeClipAt (moh::clips()[(size_t) i].notes, t0, root);
        if (desc.startsWith ("user:") && i >= 0 && i < (int) proc.userClips.size()) placeClipAt (proc.userClips[(size_t) i].notes, t0, root);
    };
    addAndMakeVisible (dragZone);

    // ---- sempre embaixo: TAMANHO DA NOTA e teclado
    for (int i = 0; i < moh::kNumLengths; ++i) chip (lenBtn[i], moh::lengthName (i), [this, i] { proc.setParam ("noteLen", (float) i); });
    keyboard.setAvailableRange (24, 96);
    keyboard.setOctaveForMiddleC (3);   // nomes do Cubase: C3 = dó central
    keyboard.setLowestVisibleKey (36);
    keyboard.setColour (MidiKeyboardComponent::keyDownOverlayColourId, col::red);
    keyboard.setColour (MidiKeyboardComponent::mouseOverKeyOverlayColourId, col::red.withAlpha (0.3f));
    addAndMakeVisible (keyboard);

    setSize (1000, 704);
    setPage (0);
    startTimerHz (30);
}

OrchHitEditor::~OrchHitEditor() { stopTimer(); setLookAndFeel (nullptr); }

void OrchHitEditor::chip (TextButton& b, const String& text, std::function<void()> click, const String& kind)
{
    b.setButtonText (text);
    if (kind.isNotEmpty()) b.getProperties().set ("kind", kind);
    if (click) b.onClick = std::move (click);
    addAndMakeVisible (b);
}

void OrchHitEditor::knob (Knob& k, const String& id, const String& label)
{
    k.label = label;
    k.s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    k.att = std::make_unique<SA> (proc.apvts, id, k.s);
    addAndMakeVisible (k.s);
}

void OrchHitEditor::rebuildClips()
{
    for (auto* t : clipThumbs) removeChildComponent (t);
    clipThumbs.clear();
    auto add = [this] (const String& title, const String& info, const std::vector<moh::GridNote>& notes, const String& desc, int userIdx)
    {
        auto* t = clipThumbs.add (new Thumb());
        t->title = title; t->info = info; t->notes = notes;
        t->onClick = [this, notes] { placeClipAt (notes, 0, moh::kRootDefault); };
        t->onDouble = [this, notes] { moh::Grid g; moh::placeClip (g, notes, 0, moh::kRootDefault); gridEd.commit (g); };   // EDITAR: só ele na grade
        t->onDrag = [this, desc, t] (const MouseEvent&) { if (! isDragAndDropActive()) startDragging (desc, t); };
        if (userIdx >= 0) t->onRight = [this, userIdx] { proc.userClips.erase (proc.userClips.begin() + userIdx); rebuildClips(); resized(); };
        addAndMakeVisible (t);
    };
    for (int i = 0; i < (int) moh::clips().size(); ++i)
        add (u8 (moh::clips()[(size_t) i].name), u8 (moh::clips()[(size_t) i].info), moh::clips()[(size_t) i].notes, "clip:" + String (i), -1);
    for (int i = 0; i < (int) proc.userClips.size(); ++i)
        add (proc.userClips[(size_t) i].name, String ((int) proc.userClips[(size_t) i].notes.size()) + u8 (" notas · botão direito apaga"), proc.userClips[(size_t) i].notes, "user:" + String (i), i);
    setPage (page);
}

void OrchHitEditor::placeClipAt (const std::vector<moh::GridNote>& notes, int t0, int root)
{
    auto g = proc.getGrid();
    moh::placeClip (g, notes, t0, root, GridEditor::lo, GridEditor::hi);
    gridEd.commit (g);
}

void OrchHitEditor::setPage (int p)
{
    page = p;
    tabSom.setToggleState (p == 0, dontSendNotification); tabMidi.setToggleState (p == 1, dontSendNotification);
    std::vector<Component*> som { &env, &vol.s, &rev.s, &hand.s, &handsBtn, &echoBtn, &satBtn };
    for (auto& b : timbreBtn) som.push_back (&b);
    for (auto& b : envPreset) som.push_back (&b);
    std::vector<Component*> midi { &previewBtn, &dawBtn, &drumsBtn, &bpm.s, &drawBtn, &eraseBtn, &undoBtn, &clearBtn, &saveBtn, &gridEd, &dragZone };
    for (auto& t : grooveThumb) midi.push_back (&t);
    for (auto* t : clipThumbs) midi.push_back (t);
    for (auto& b : snapBtn) midi.push_back (&b);
    for (auto* c : som) c->setVisible (p == 0);
    for (auto* c : midi) c->setVisible (p == 1);
    lcd.setVisible (p == 0);
    repaint();
}

void OrchHitEditor::paint (Graphics& g)
{
    g.fillAll (col::bg);
    auto head = getLocalBounds().removeFromTop (54);
    g.setColour (col::panel); g.fillRect (head);
    g.setColour (col::text); g.setFont (sans (24.0f, true)); g.drawText ("Mastor Orch Hit", 16, 6, 300, 28, Justification::centredLeft);
    g.setColour (col::mute); g.setFont (mono (10.5f, false));
    g.drawText (u8 ("ORCHESTRA HIT ANOS 80 · DUB STATION · MOH-88 v") + JucePlugin_VersionString, 16, 32, 420, 16, Justification::centredLeft);
    auto plate = Rectangle<float> (892, 12, 92, 30);
    g.setColour (col::red); g.fillRoundedRectangle (plate, 6.0f);
    g.setColour (Colours::white); g.setFont (mono (15.0f)); g.drawText ("MOH-88", plate.toNearestInt(), Justification::centred);

    auto label = [&] (const String& s, int x, int y, int w = 600) { g.setColour (col::mute); g.setFont (mono (11.0f)); g.drawText (s, x, y, w, 14, Justification::centredLeft); };
    if (page == 0)
    {
        label ("TIMBRES", timbreArea.getX(), timbreArea.getY() - 16);
        label (u8 ("ENVELOPE · ARRASTE OS PONTOS (ATAQUE, DECAIMENTO/SUSTENTAÇÃO, RELEASE)"), envArea.getX(), envArea.getY() - 16, 620);
        label ("EFEITOS", fxArea.getX(), fxArea.getY() - 16);
        g.setColour (col::text); g.setFont (mono (10.5f));
        for (auto* k : { &vol, &rev, &hand })
            g.drawText (k->label, k->s.getBounds().withY (k->s.getBottom()).withHeight (14), Justification::centred);
        g.setColour (col::mute); g.setFont (sans (12.0f));
        g.drawFittedText (u8 ("Cada tecla toca o acorde do hit (Lá + Mi em várias oitavas; o RAGGA MENOR tem a terça) transposto, "
                              "como nos samplers dos anos 80. DUAS MÃOS soma a mesma nota uma oitava acima. Tape Echo e Saturador começam desligados."),
                          fxArea.withTrimmedTop (fxArea.getHeight() - 70), Justification::topLeft, 5);
    }
    else
    {
        label (u8 ("GROOVES DE REGGAE · OS POUCOS ATAQUES DESSE TIMBRE (RÉ – RÉ – SI♭ – DÓ, CAIXA NO 2 E NO 4)"), grooveArea.getX(), grooveArea.getY() - 16, 900);
        label (u8 ("MIDIs · ARRASTE PARA A GRADE (CLIQUE = INSERE NO COMPASSO 1 · DUPLO CLIQUE = EDITA SÓ ELE)"), clipArea.getX(), clipArea.getY() - 16, 900);
        g.setColour (col::mute); g.setFont (mono (11.0f));
        g.drawText ("GRADE", snapBtn[0].getX() - 52, snapBtn[0].getY(), 48, snapBtn[0].getHeight(), Justification::centredRight);
    }
    // TAMANHO DA NOTA (sempre visível)
    g.setColour (col::mute); g.setFont (mono (11.0f));
    g.drawText ("TAMANHO DA NOTA", 16, lenBtn[0].getY(), 150, lenBtn[0].getHeight(), Justification::centredLeft);
}

void OrchHitEditor::resized()
{
    tabSom.setBounds (600, 12, 120, 30); tabMidi.setBounds (726, 12, 156, 30);
    const int top = 76;
    lcd.setBounds (16, top, 460, 136);
    {
        timbreArea = { 492, top, 492, 136 };
        for (int i = 0; i < moh::numPresets(); ++i)
            timbreBtn[i].setBounds (492 + (i % 4) * 124, top + (i / 4) * 70, 118, 62);
        envArea = { 16, 244, 620, 296 };
        env.setBounds (16, 244, 620, 250);
        for (int i = 0; i < 3; ++i) envPreset[i].setBounds (16 + i * 130, 502, 122, 36);
        fxArea = { 652, 244, 332, 296 };
        vol.s.setBounds (660, 244, 96, 80); rev.s.setBounds (770, 244, 96, 80); hand.s.setBounds (880, 244, 96, 80);
        handsBtn.setBounds (652, 352, 332, 36); echoBtn.setBounds (652, 394, 162, 36); satBtn.setBounds (822, 394, 162, 36);
    }
    // MIDIs E GRADE (os grooves e MIDIs ocupam o lugar do LCD e dos timbres)
    grooveArea = { 16, top, 968, 66 };
    for (int i = 0; i < 5; ++i) grooveThumb[i].setBounds (16 + i * 196, top, 188, 66);
    clipArea = { 16, 162, 968, 62 };
    const int nc = clipThumbs.size(), cw = nc > 0 ? (968 - (nc - 1) * 6) / nc : 0;
    for (int i = 0; i < nc; ++i) clipThumbs[i]->setBounds (16 + i * (cw + 6), 162, cw, 62);
    previewBtn.setBounds (16, 232, 96, 32); dawBtn.setBounds (118, 232, 140, 32); drumsBtn.setBounds (264, 232, 96, 32);
    bpm.s.setBounds (366, 232, 150, 32);
    dragZone.setBounds (744, 232, 240, 32);
    drawBtn.setBounds (16, 270, 110, 30); eraseBtn.setBounds (132, 270, 86, 30); undoBtn.setBounds (224, 270, 96, 30);
    clearBtn.setBounds (326, 270, 80, 30); saveBtn.setBounds (412, 270, 160, 30);
    for (int i = 0; i < 3; ++i) snapBtn[i].setBounds (812 + i * 58, 270, 52, 30);
    gridArea = { 16, 306, 968, 236 };
    gridEd.setBounds (gridArea);
    // embaixo
    for (int i = 0; i < moh::kNumLengths; ++i) lenBtn[i].setBounds (170 + i * 74, 552, 68, 30);
    keyboard.setBounds (16, 590, 968, 104);
    keyboard.setKeyWidth (968.0f / 29.0f);
    keyboard.setLowestVisibleKey (36);   // C1 (nomes do Cubase)
    // na aba MIDIs o LCD dá lugar aos grooves
    lcd.setVisible (page == 0);
}

void OrchHitEditor::timerCallback()
{
    const int t = (int) proc.get ("timbre"), len = (int) proc.get ("noteLen"), gr = (int) proc.get ("groove");
    for (int i = 0; i < moh::numPresets(); ++i) timbreBtn[i].setToggleState (i == t, dontSendNotification);
    for (int i = 0; i < moh::kNumLengths; ++i) lenBtn[i].setToggleState (i == len, dontSendNotification);
    for (int i = 0; i < 5; ++i)
    {
        const bool sel = i == gr && ! proc.gridIsCustom();
        if (grooveThumb[i].selected != sel) { grooveThumb[i].selected = sel; grooveThumb[i].repaint(); }
    }
    const int snaps[3] = { 4, 2, 1 };
    for (int i = 0; i < 3; ++i) snapBtn[i].setToggleState (gridEd.snap == snaps[i], dontSendNotification);
    drawBtn.setToggleState (! gridEd.erase, dontSendNotification); eraseBtn.setToggleState (gridEd.erase, dontSendNotification);
    if (t != lastTimbre) { lastTimbre = t; lcd.repaint(); }
    // a última tecla tocada no LCD
    for (int n = 127; n >= 0; --n) if (proc.keyboardState.isNoteOnForChannels (0xffff, n)) { lcd.note = OrchHitProcessor::noteName (n); lcd.repaint(); break; }
    if (page == 0) env.repaint();
    if (page == 1) gridEd.repaint();
    lcd.setVisible (page == 0);
}
