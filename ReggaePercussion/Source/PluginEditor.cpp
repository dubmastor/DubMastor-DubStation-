#include "PluginEditor.h"
#include <map>

using namespace juce;

namespace col
{
const Colour cream { 0xffe6dabb }, cream2 { 0xffd9ccac }, panel { 0x40fffaeb }, line { 0xffb8a984 }, ink { 0xff1b130d }, label { 0xff4b4034 },
             muted { 0xff8f826f }, well { 0xff15110e }, amber { 0xfff2a83a }, red { 0xffc8402f }, green { 0xff2f9a4c },
             olive { 0xff7f9a5a }, olive2 { 0xff6d8a4c }, keyHi { 0xfff4f1ea }, keyLo { 0xffcfcac0 };
}
static const uint32 barCols[8] = { 0xff2f9a4c, 0xffe0a92a, 0xffc8402f, 0xff3d7fd9, 0xff8e5bc7, 0xffe0782a, 0xff1aa596, 0xffd9468f };
Colour ReggaePercussionEditor::barColour (int b) { return Colour (barCols[b & 7]); }

static Font mono (float size, bool bold = true) { return Font (FontOptions (Font::getDefaultMonospacedFontName(), size, bold ? Font::bold : Font::plain)); }
static Font sans (float size, bool bold = true) { return Font (FontOptions (size, bold ? Font::bold : Font::plain)); }
static String U (const char* utf8) { return String::fromUTF8 (utf8); }
// maiúsculas com acento (toUpperCase depende do locale do sistema)
static String UP (const String& x)
{
    String r = x.toUpperCase();
    const char* lo[] = { "\xc3\xa1", "\xc3\xa2", "\xc3\xa3", "\xc3\xa7", "\xc3\xa9", "\xc3\xaa", "\xc3\xad", "\xc3\xb3", "\xc3\xb4", "\xc3\xba" };
    const char* hi[] = { "\xc3\x81", "\xc3\x82", "\xc3\x83", "\xc3\x87", "\xc3\x89", "\xc3\x8a", "\xc3\x8d", "\xc3\x93", "\xc3\x94", "\xc3\x9a" };
    for (int i = 0; i < 10; ++i) r = r.replace (U (lo[i]), U (hi[i]));
    return r;
}
static String soundLabel (int s) { const auto& d = mrp::sounds()[(size_t) jlimit (0, mrp::numSounds - 1, s)]; return U (d.name) + " " + U (d.sub); }
static String noteName (int n) { static const char* nn[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }; return String (nn[n % 12]) + String (n / 12 - 2); }   // C3 = 60 (Cubase / Battery)

// ================================================================== LookAndFeel
PercLookAndFeel::PercLookAndFeel()
{
    setColour (ComboBox::backgroundColourId, Colour (0xfffbf7ec));
    setColour (ComboBox::textColourId, col::ink);
    setColour (ComboBox::outlineColourId, Colour (0xff24201c));
    setColour (ComboBox::arrowColourId, col::ink);
    setColour (PopupMenu::backgroundColourId, Colour (0xff1a1714));
    setColour (PopupMenu::textColourId, Colour (0xffefe5cc));
    setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xff3a2f1c));
    setColour (PopupMenu::highlightedTextColourId, Colour (0xffffc061));
    setColour (PopupMenu::headerTextColourId, Colour (0xffc9a45c));
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxTextColourId, Colour (0xff5a4d3e));
    setColour (Label::textColourId, col::label);
    setColour (TooltipWindow::backgroundColourId, Colour (0xff1a1714));
    setColour (TooltipWindow::textColourId, Colour (0xffefe5cc));
}

void PercLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const String kind = b.getProperties()["kind"].toString();
    const bool on = b.getToggleState();
    const float lift = (kind == "go" || kind == "red" || kind == "dark" || kind == "tab") ? 4.0f : 3.0f;
    bool pressed = down;
    if (kind == "key" || kind == "tab") pressed = pressed || on;
    if (kind == "mutekey") pressed = pressed || on;   // mudo = tecla afundada
    auto body = pressed ? r.withTrimmedTop (lift) : r.withTrimmedBottom (lift);
    if (! pressed) { g.setColour (Colour (0xff1a1613)); g.fillRoundedRectangle (r.withTrimmedTop (lift), 5.0f); }
    Colour top, bot;
    if (kind == "go")        { top = Colour (0xff2f9a4c); bot = Colour (0xff17652e); }
    else if (kind == "red")  { top = Colour (0xffd4473a); bot = Colour (0xff9b2219); }
    else if (kind == "dark") { top = on ? Colour (0xff5a4a3a) : Colour (0xff3a332d); bot = Colour (0xff1b1714); }
    else if (pressed)        { top = Colour (0xffc9b58c); bot = Colour (0xffe7d3a6); }
    else                     { top = col::keyHi; bot = col::keyLo; }
    g.setGradientFill (ColourGradient (top, body.getX(), body.getY(), bot, body.getX(), body.getBottom(), false));
    g.fillRoundedRectangle (body, 5.0f);
    if (over && ! pressed) { g.setColour (Colours::white.withAlpha (0.10f)); g.fillRoundedRectangle (body, 5.0f); }
    g.setColour (Colour (0xff24201c)); g.drawRoundedRectangle (body, 5.0f, 1.6f);
    // luz (LED) das teclas com estado
    if (kind == "tab" || kind == "mutekey" || b.getProperties()["led"])
    {
        const bool lit = kind == "mutekey" ? ! on : on;
        auto led = Rectangle<float> (9.0f, 9.0f).withPosition (body.getX() + 7.0f, body.getY() + 6.0f);
        const Colour lc = b.getProperties().contains ("ledColour") ? Colour ((uint32) (int64) b.getProperties()["ledColour"]) : col::amber;
        if (lit) { g.setColour (lc.withAlpha (0.45f)); g.fillEllipse (led.expanded (3.0f)); g.setColour (lc); }
        else g.setColour (Colour (0xff3a2f27));
        g.fillEllipse (led);
    }
    const float fl = (float) b.getProperties()["flash"];
    if (fl > 0.01f)
    {
        const Colour lc = b.getProperties().contains ("ledColour") ? Colour ((uint32) (int64) b.getProperties()["ledColour"]) : col::amber;
        g.setColour (lc.withAlpha (jlimit (0.0f, 1.0f, fl))); g.drawRoundedRectangle (body.reduced (1.5f), 5.0f, 3.0f);
    }
}

void PercLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool down)
{
    const String kind = b.getProperties()["kind"].toString();
    const bool on = b.getToggleState();
    bool pressed = down || ((kind == "key" || kind == "tab" || kind == "mutekey") && on);
    auto r = b.getLocalBounds().reduced (4, 2);
    if (kind == "go" || kind == "red" || kind == "dark" || kind == "tab") r = pressed ? r.withTrimmedTop (4) : r.withTrimmedBottom (4);
    else r = pressed ? r.withTrimmedTop (3) : r.withTrimmedBottom (3);
    Colour c = (kind == "go" || kind == "red") ? Colour (0xfff7f2df) : kind == "dark" ? Colour (0xffefe6d0) : (pressed ? Colour (0xff2a1a08) : col::label);
    if (kind == "go") c = Colour (0xfff7d36a);
    if (! b.isEnabled()) c = c.withAlpha (0.35f);
    g.setColour (c);
    const String txt = b.getButtonText();
    if (txt.contains ("\n"))
    {
        auto top = r.removeFromTop (r.getHeight() * 55 / 100);
        g.setFont (sans (kind == "tab" ? 15.0f : 12.0f)); g.drawFittedText (txt.upToFirstOccurrenceOf ("\n", false, false), top, Justification::centredBottom, 1);
        g.setColour (c.withAlpha (0.7f)); g.setFont (sans (kind == "tab" ? 10.0f : 9.0f));
        g.drawFittedText (txt.fromFirstOccurrenceOf ("\n", false, false), r, Justification::centredTop, 1);
        return;
    }
    g.setFont (sans (kind == "go" ? 14.0f : 12.0f));
    g.drawFittedText (txt, r, Justification::centred, 2, 0.8f);
}

void PercLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const float d = (float) jmin (w, h) - 4.0f, cx = x + w * 0.5f, cy = y + h * 0.5f, rr = d * 0.5f;
    const bool bi = (bool) s.getProperties()["bipolar"];
    const float ang = a0 + pos * (a1 - a0), mid = (a0 + a1) * 0.5f;
    Path bg; bg.addCentredArc (cx, cy, rr - 2, rr - 2, 0, a0, a1, true);
    g.setColour (col::line); g.strokePath (bg, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    Path arc; arc.addCentredArc (cx, cy, rr - 2, rr - 2, 0, bi ? jmin (mid, ang) : a0, bi ? jmax (mid, ang) : ang, true);
    g.setColour (Colour (0xffd78a2a)); g.strokePath (arc, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    auto kr = Rectangle<float> (d * 0.66f, d * 0.66f).withCentre ({ cx, cy });
    g.setColour (Colour (0xffc8c8c8)); g.fillEllipse (kr.expanded (2.5f));
    g.setGradientFill (ColourGradient (Colour (0xff4a4a4a), kr.getX(), kr.getY(), Colour (0xff111111), kr.getRight(), kr.getBottom(), false));
    g.fillEllipse (kr);
    Path p; p.addRoundedRectangle (-1.6f, -kr.getHeight() * 0.46f, 3.2f, kr.getHeight() * 0.3f, 1.5f);
    g.setColour (Colour (0xfff4f1ea)); g.fillPath (p, AffineTransform::rotation (ang).translated (cx, cy));
}

Font PercLookAndFeel::getComboBoxFont (ComboBox&) { return sans (12.5f); }
Font PercLookAndFeel::getPopupMenuFont() { return sans (14.0f, false); }
Label* PercLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (mono (11.0f, false));
    l->setColour (Label::textColourId, Colour (0xff5a4d3e));
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    return l;
}

// ================================================================== pads
void PadComp::paint (Graphics& g)
{
    const auto& pd = mrp::pads()[(size_t) index];
    const auto& sd = mrp::sounds()[(size_t) pd.snd];
    const Colour c (sd.argb);
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setGradientFill (ColourGradient (Colour (0xff3a322b).interpolatedWith (c, flash * 0.35f), r.getX(), r.getY(), Colour (0xff1a1511), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 5.0f);
    if (flash > 0.01f) { g.setColour (c.withAlpha (flash)); g.drawRoundedRectangle (r.reduced (1.0f), 5.0f, 2.5f); }
    g.setColour (Colours::black); g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (c); g.fillRect (r.removeFromTop (5.0f).reduced (1.0f, 0));
    auto t = getLocalBounds().reduced (8, 6).withTrimmedTop (4);
    g.setColour (Colour (0xffefe6d0)); g.setFont (sans (11.5f));
    g.drawText (UP (U (sd.name)), t.removeFromTop (16), Justification::topLeft);
    g.setColour (Colour (0xffb9a684)); g.setFont (sans (10.0f, false));
    g.drawText (pd.lg < 0 ? U (sd.sub) : String (pd.lg ? "Longo" : "Curto"), t.removeFromTop (14), Justification::topLeft);
    const int n = proc.padNote[(size_t) index].load();
    g.setColour (col::amber); g.setFont (mono (10.5f));
    g.drawText (noteName (n) + " . " + String (n), t.removeFromBottom (14), Justification::bottomLeft);
    auto bar = getLocalBounds().toFloat().removeFromRight (9.0f).reduced (2.0f, 10.0f);
    g.setColour (Colour (0xff0d0b09)); g.fillRoundedRectangle (bar, 2.0f);
    g.setColour (c); g.fillRoundedRectangle (bar.withTrimmedTop (bar.getHeight() * (1.0f - flash)), 2.0f);
}

void PadComp::mouseDown (const MouseEvent& e)
{
    const float vel = jlimit (0.3f, 1.0f, 1.0f - (float) e.y / (float) jmax (1, getHeight()) * 0.7f);
    proc.playPad (index, vel);
}

// ================================================================== visor MPC
void LcdComp::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0a0c10)); g.fillRoundedRectangle (r, 4.0f);
    auto in = r.reduced (4.0f);
    g.setGradientFill (ColourGradient (Colour (0xff4a8cf0), in.getX(), in.getY(), Colour (0xff1d4aa8), in.getX(), in.getBottom(), false));
    g.fillRect (in);
    for (float y = in.getY(); y < in.getBottom(); y += 3.0f) { g.setColour (Colours::black.withAlpha (0.07f)); g.drawHorizontalLine ((int) y, in.getX(), in.getRight()); }
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.16f), in.getX(), in.getY(), Colours::transparentWhite, in.getX(), in.getCentreY(), false));
    g.fillRect (in);
    const auto& gs = mrp::grooves();
    const int gi = jlimit (0, (int) gs.size() - 1, proc.grooveIndex.load());
    int inGroup = 1; for (int i = gi - 1; i >= 0 && String (gs[(size_t) i].group) == gs[(size_t) gi].group; --i) ++inGroup;
    int group = 0; for (int i = 1; i <= gi; ++i) if (String (gs[(size_t) i].group) != gs[(size_t) i - 1].group) ++group;
    auto t = in.reduced (12.0f, 5.0f).toNearestInt();
    g.setColour (Colour (0xffe8f3ff).withAlpha (0.85f)); g.setFont (sans (10.5f));
    auto top = t.removeFromTop (14);
    g.drawText (String (gs[(size_t) gi].group).toUpperCase(), top, Justification::left);
    g.drawText (String (gi + 1).paddedLeft ('0', 2) + "/" + String ((int) gs.size()), top, Justification::right);
    auto mid = t.removeFromTop (26);
    auto bank = mid.removeFromLeft (44).reduced (0, 4);
    g.setColour (Colour (0xffe8f3ff)); g.fillRect (bank);
    g.setColour (Colour (0xff1d4aa8)); g.setFont (mono (12.0f));
    g.drawText (String::charToString ((juce_wchar) ('A' + group)) + String (inGroup).paddedLeft ('0', 2), bank, Justification::centred);
    g.setColour (Colour (0xffe8f3ff)); g.setFont (sans (19.0f));
    g.drawText (gs[(size_t) gi].name, mid.withTrimmedLeft (10), Justification::centredLeft, true);
    g.setColour (Colour (0xffe8f3ff).withAlpha (0.85f)); g.setFont (sans (10.5f));
    const bool playing = proc.playStep.load() >= 0;
    const double bpm = proc.get ("bpm");
    g.drawText (String (bpm, 1).paddedLeft ('0', 5) + " BPM", t, Justification::left);
    g.drawText ("SW " + String (roundToInt (proc.get ("swing") * 100)) + "%", t, Justification::centred);
    g.drawText (playing ? "PLAY" : "STOP", t, Justification::right);
}

// ================================================================== EQ
float EqComp::xOf (double f) const { return (float) (std::log (f / 20.0) / std::log (1000.0)) * getWidth(); }
double EqComp::fOf (float x) const { return 20.0 * std::pow (1000.0, jlimit (0.0, 1.0, (double) x / getWidth())); }

void EqComp::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff120c08)); g.fillRoundedRectangle (r, 6.0f);
    const float W = r.getWidth(), H = r.getHeight();
    auto yOf = [H] (double db) { return (float) (0.5 - db / 36.0) * H; };
    g.setColour (Colour (0xff2a1d14));
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 }) g.drawVerticalLine ((int) xOf (f), 0, H);
    for (double db : { -12.0, -6.0, 6.0, 12.0 }) g.drawHorizontalLine ((int) yOf (db), 0, W);
    g.setColour (Colour (0xff5a4430)); g.drawHorizontalLine ((int) yOf (0), 0, W);
    g.setColour (Colour (0xffb9a684)); g.setFont (mono (10.0f, false));
    g.drawText ("100", (int) xOf (100) + 3, (int) H - 14, 30, 12, Justification::left);
    g.drawText ("1k", (int) xOf (1000) + 3, (int) H - 14, 30, 12, Justification::left);
    g.drawText ("10k", (int) xOf (10000) + 3, (int) H - 14, 30, 12, Justification::left);
    const bool on = proc.get ("eqOn") > 0.5f;
    const double sr = 48000.0;
    mrp::Biquad hp, bl, lp;
    hp.set (mrp::Biquad::HP, proc.get ("hpf"), 0.707f, (float) sr); bl.set (mrp::Biquad::PEAK, proc.get ("bellF"), 1.0f, (float) sr, proc.get ("bellG")); lp.set (mrp::Biquad::LP, proc.get ("lpf"), 0.707f, (float) sr);
    Path p, fill;
    for (int i = 0; i <= 200; ++i)
    {
        const float x = W * i / 200.0f; const double f = fOf (x);
        const double m = on ? hp.mag (f, sr) * bl.mag (f, sr) * lp.mag (f, sr) : 1.0;
        const float y = yOf (jmax (-18.0, 20.0 * std::log10 (jmax (1e-6, m))));
        if (i == 0) { p.startNewSubPath (x, y); fill.startNewSubPath (x, H); fill.lineTo (x, y); } else { p.lineTo (x, y); fill.lineTo (x, y); }
    }
    fill.lineTo (W, H); fill.closeSubPath();
    g.setGradientFill (ColourGradient (on ? Colour (0x48f2a541) : Colour (0x33777777), 0, 0, Colours::transparentBlack, 0, H, false)); g.fillPath (fill);
    g.setColour (on ? Colour (0xfff2a541) : Colour (0xff6b5a48)); g.strokePath (p, PathStrokeType (2.5f));
    const float px[3] = { xOf (proc.get ("hpf")), xOf (proc.get ("bellF")), xOf (proc.get ("lpf")) };
    const float py[3] = { yOf (0), yOf (proc.get ("bellG")), yOf (0) };
    const char* lb[3] = { "HPF", "BELL", "LPF" };
    for (int k = 0; k < 3; ++k)
    {
        g.setColour (k == 1 ? col::red : Colour (0xffefe2c4)); g.fillEllipse (px[k] - 7, py[k] - 7, 14, 14);
        g.setColour (Colours::black); g.drawEllipse (px[k] - 7, py[k] - 7, 14, 14, 1.4f);
        g.setColour (Colour (0xffefe2c4)); g.setFont (mono (10.0f));
        g.drawText (lb[k], (int) jlimit (2.0f, W - 34, px[k] + 9), (int) jmax (2.0f, py[k] - 19), 34, 12, Justification::left);
    }
}

void EqComp::mouseDown (const MouseEvent& e)
{
    const float H = (float) getHeight();
    auto yOf = [H] (double db) { return (float) (0.5 - db / 36.0) * H; };
    const float px[3] = { xOf (proc.get ("hpf")), xOf (proc.get ("bellF")), xOf (proc.get ("lpf")) };
    const float py[3] = { yOf (0), yOf (proc.get ("bellG")), yOf (0) };
    float best = 1e9f; dragging = -1;
    for (int k = 0; k < 3; ++k) { const float d = std::hypot (px[k] - e.x, py[k] - e.y); if (d < best) { best = d; dragging = k; } }
    if (best > 60) dragging = -1;
    mouseDrag (e);
}

void EqComp::mouseDrag (const MouseEvent& e)
{
    if (dragging < 0) return;
    const double f = fOf ((float) e.x);
    if (dragging == 0) proc.setParam ("hpf", (float) jlimit (20.0, 2000.0, jmin (f, (double) proc.get ("lpf") * 0.8)));
    if (dragging == 2) proc.setParam ("lpf", (float) jlimit (1000.0, 20000.0, jmax (f, (double) proc.get ("hpf") * 1.25)));
    if (dragging == 1)
    {
        proc.setParam ("bellF", (float) jlimit (200.0, 12000.0, f));
        proc.setParam ("bellG", jlimit (-12.0f, 12.0f, (0.5f - (float) e.y / (float) getHeight()) * 36.0f));
    }
    repaint();
}

// ================================================================== arrastar MIDI
void DragZone::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    if (big)
    {
        g.setColour (Colour (0xff15110e)); g.fillRoundedRectangle (r, 8.0f);
        auto in = r.reduced (6.0f);
        g.setGradientFill (ColourGradient (Colour (0xffffd27a), in.getX(), in.getY(), Colour (0xffc8702a), in.getX(), in.getBottom(), false));
        g.fillRoundedRectangle (in, 7.0f);
        g.setColour (Colours::black); g.drawRoundedRectangle (in, 7.0f, 2.0f);
        auto ic = in.withWidth (in.getHeight()).reduced (8.0f);
        g.setColour (Colour (0xff1b130d)); g.fillRoundedRectangle (ic, 6.0f);
        g.setColour (col::amber); g.setFont (sans (22.0f)); g.drawText (String (CharPointer_UTF8 ("\xe2\x87\xa3")), ic, Justification::centred);
        auto tx = in.withTrimmedLeft (in.getHeight() + 6).toNearestInt();
        g.setColour (Colour (0xff1b130d)); g.setFont (sans (19.0f));
        g.drawText (title, tx.removeFromTop (tx.getHeight() / 2 + 4), Justification::bottomLeft);
        g.setFont (sans (11.0f, false)); g.drawText (sub, tx, Justification::topLeft, true);
        return;
    }
    g.setColour (Colours::black); g.fillRoundedRectangle (r.withTrimmedTop (3.0f), 5.0f);
    auto body = r.withTrimmedBottom (3.0f);
    g.setColour (colour); g.fillRoundedRectangle (body, 5.0f);
    g.setColour (Colours::black); g.drawRoundedRectangle (body, 5.0f, 1.6f);
    g.setColour (colour.getPerceivedBrightness() > 0.6f ? Colour (0xff1b130d) : Colours::white);
    g.setFont (sans (11.5f));
    if (sub.isNotEmpty())
    {
        auto t = body.toNearestInt();
        g.drawText (title, t.removeFromTop (t.getHeight() * 55 / 100), Justification::centredBottom);
        g.setFont (mono (9.0f, false)); g.drawText (sub, t, Justification::centredTop);
    }
    else g.drawFittedText (title, body.toNearestInt().reduced (4, 0), Justification::centred, 2);
}

void DragZone::mouseDrag (const MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    auto f = proc.tempMidi (trackIx, barIx);
    if (proc.writeMidiFile (f, trackIx, barIx)) DragAndDropContainer::performExternalDragDropOfFiles (StringArray (f.getFullPathName()), false, this);
}

// ================================================================== grade
Rectangle<float> GridComp::cell (int row, int step) const
{
    const float x0 = (float) nameW, w = (getWidth() - x0) / 16.0f, h = (getHeight() - headH) / (float) mrp::numTracks;
    return { x0 + step * w + (step % 4 == 0 && step ? 3.0f : 0.0f) + 1.5f, headH + row * h + 3.0f, w - 4.5f, h - 6.0f };
}

void GridComp::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const Colour bc = ReggaePercussionEditor::barColour (ed.editBar);
    g.setColour (Colour (0xff17120f)); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (bc); g.drawRoundedRectangle (r.reduced (1.5f), 6.0f, 3.0f);
    const int ps = proc.playStep.load();
    const int playBar = ps >= 0 ? ps / 16 : -1, pst = ps >= 0 ? ps % 16 : -1;
    for (int s = 0; s < 16; ++s)
    {
        auto c = cell (0, s); c.setY (4.0f); c.setHeight (16.0f);
        const bool now = playBar == ed.editBar && pst == s;
        if (now) { g.setColour (col::amber); g.fillRoundedRectangle (c, 3.0f); }
        g.setColour (now ? col::ink : mrp::isSnareStep (s) ? Colour (0xffff9a86) : mrp::isSkankStep (s) ? col::amber : (s % 4 == 0 ? Colour (0xffe8dcc4) : col::muted));
        g.setFont (mono (11.0f)); g.drawText (mrp::countName (s), c, Justification::centred);
        auto ref = c.withY (21.0f).withHeight (4.0f);
        g.setColour (mrp::isSnareStep (s) ? col::red : mrp::isSkankStep (s) ? Colour (0xff8a6a22) : Colour (0xff2a1d14));
        g.fillRoundedRectangle (ref, 1.5f);
    }
    for (int t = 0; t < mrp::numTracks; ++t)
    {
        const int snd = proc.soundOf (t);
        const Colour c (mrp::sounds()[(size_t) snd].argb);
        const bool on = proc.isAudible (t);
        auto row = cell (t, 0);
        // nome da peça (pisca no ritmo)
        auto nameR = Rectangle<float> (8.0f, row.getY(), 104.0f, row.getHeight());
        g.setColour (c.withAlpha (0.25f + 0.75f * ed.rowFlash[t])); g.fillEllipse (nameR.getX(), nameR.getCentreY() - 4, 8, 8);
        g.setColour (Colour (0xffefe2c4).withAlpha (on ? 1.0f : 0.45f)); g.setFont (sans (12.0f));
        g.drawText (U (mrp::sounds()[(size_t) snd].name), nameR.withTrimmedLeft (13).toNearestInt(), Justification::centredLeft, true);
        for (int s = 0; s < 16; ++s)
        {
            auto ce = cell (t, s);
            const int v = proc.getStep (t, ed.editBar * 16 + s);
            if (v == 2) g.setGradientFill (ColourGradient (Colour (0xfff2b25a), ce.getX(), ce.getY(), Colour (0xffc47a22), ce.getX(), ce.getBottom(), false));
            else if (v == 1) g.setGradientFill (ColourGradient (Colour (0xff7a5430), ce.getX(), ce.getY(), Colour (0xff4b3420), ce.getX(), ce.getBottom(), false));
            else g.setColour ((s / 4) % 2 ? Colour (0xff20160f) : Colour (0xff1a120c));
            g.setOpacity (on ? 1.0f : 0.35f);
            g.fillRoundedRectangle (ce, 4.0f);
            g.setOpacity (1.0f);
            const bool now = playBar == ed.editBar && pst == s;
            g.setColour (now ? Colour (0xffefe2c4) : Colours::black); g.drawRoundedRectangle (ce, 4.0f, now ? 2.0f : 1.0f);
        }
    }
}

void GridComp::mouseDown (const MouseEvent& e)
{
    for (int t = 0; t < mrp::numTracks; ++t)
        for (int s = 0; s < 16; ++s)
            if (cell (t, s).contains (e.position))
            {
                const int nv = (proc.getStep (t, ed.editBar * 16 + s) + 1) % 3;
                if (ed.editAll) for (int b = 0; b < 8; ++b) proc.setStep (t, b * 16 + s, nv);
                else proc.setStep (t, ed.editBar * 16 + s, nv);
                repaint();
                return;
            }
}

// ================================================================== canal de uma peça
StripComp::StripComp (ReggaePercussionProcessor& p, int track) : proc (p), t (track), drag (p, track, -1)
{
    auto prep = [this] (TextButton& b, const String& kind) { b.getProperties().set ("kind", kind); addAndMakeVisible (b); };
    mute.setClickingTogglesState (true); prep (mute, "mutekey");
    muteA = std::make_unique<BA> (proc.apvts, ReggaePercussionProcessor::tid ("mute", t), mute);
    mute.onStateChange = [this] { mute.setButtonText (mute.getToggleState() ? "MUDO" : "TOCANDO"); };
    mute.setButtonText (mute.getToggleState() ? "MUDO" : "TOCANDO");
    prep (prev, "key"); prep (swap, "key"); prep (next, "key"); prep (load, "key"); prep (unload, "key");
    prev.onClick = [this] { proc.cycleSound (t, -1); };
    next.onClick = [this] { proc.cycleSound (t, 1); };
    swap.onClick = [this] { ReggaePercussionEditor::swapMenu (proc, t, &swap); };
    prev.setTooltip (U ("Pe\xc3\xa7" "a anterior")); next.setTooltip (U ("Pr\xc3\xb3xima pe\xc3\xa7" "a")); swap.setTooltip (U ("Escolher a pe\xc3\xa7" "a na lista"));
    load.setTooltip (U ("Trocar a s\xc3\xadntese por um sample seu (wav, aif, mp3)")); unload.setTooltip (U ("Voltar para a s\xc3\xadntese"));
    load.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> (U ("Sample para a pe\xc3\xa7" "a"), File::getSpecialLocation (File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f.existsAsFile()) { proc.loadSample (t, f); repaint(); }
        });
    };
    unload.onClick = [this] { proc.clearSample (t); repaint(); };
    for (auto* s : { &dec, &vol, &pan })
    {
        s->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (Slider::TextBoxBelow, true, 56, 16);
        s->setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        addAndMakeVisible (*s);
    }
    pan.getProperties().set ("bipolar", true);
    decA = std::make_unique<SA> (proc.apvts, ReggaePercussionProcessor::tid ("dec", t), dec);
    volA = std::make_unique<SA> (proc.apvts, ReggaePercussionProcessor::tid ("vol", t), vol);
    panA = std::make_unique<SA> (proc.apvts, ReggaePercussionProcessor::tid ("pan", t), pan);
    dec.textFromValueFunction = vol.textFromValueFunction = [] (double v) { return String (roundToInt (v * 100)) + "%"; };
    pan.textFromValueFunction = [] (double v) { const int x = roundToInt (v * 100); return std::abs (x) < 3 ? String ("C") : (x < 0 ? "L" : "R") + String (std::abs (x)); };
    dec.updateText(); vol.updateText(); pan.updateText();
    drag.setTooltip (U ("Arraste para a DAW o MIDI s\xc3\xb3 desta pe\xc3\xa7" "a"));
    addAndMakeVisible (drag);
    refresh();
}

void StripComp::refresh()
{
    const int s = proc.soundOf (t);
    if (s == shownSnd) return;
    shownSnd = s;
    const auto& d = mrp::sounds()[(size_t) s];
    const int n1 = proc.padNote[(size_t) mrp::padOf (s, false)].load(), n2 = proc.padNote[(size_t) mrp::padOf (s, true)].load();
    swap.setButtonText (String (CharPointer_UTF8 ("\xe2\x87\x84 TROCAR\nnota ")) + String (n1) + (d.note2 >= 0 ? "/" + String (n2) : String()));
    drag.title = U ("\xe2\x87\xa3 MIDI s\xc3\xb3 do ") + U (d.name);
    mute.getProperties().set ("ledColour", (int64) d.argb);
    drag.colour = Colour (0xfff4f1ea);
    repaint(); drag.repaint();
}

void StripComp::paint (Graphics& g)
{
    const auto& d = mrp::sounds()[(size_t) proc.soundOf (t)];
    const Colour c (d.argb);
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (Colour (0xfff3ead3).interpolatedWith (c, 0.22f * flash + (flash > 0.01f ? 0.18f : 0.0f) * flash));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (flash > 0.01f ? c.withAlpha (jmin (1.0f, 0.4f + flash)) : col::line);
    g.drawRoundedRectangle (r, 8.0f, flash > 0.01f ? 3.5f : 1.0f);
    g.setColour (c); g.fillRect (r.removeFromTop (4.0f).reduced (6.0f, 0));
    auto head = getLocalBounds().reduced (10, 8).removeFromTop (20);
    g.setColour (c.withAlpha (0.5f + 0.5f * flash)); g.fillEllipse ((float) head.getX(), (float) head.getCentreY() - 4, 8, 8);
    g.setColour (col::ink); g.setFont (sans (12.0f));
    g.drawText (UP (U (d.name)) + U ("  \xc2\xb7  ") + UP (U (d.sub)), head.withTrimmedLeft (14), Justification::centredLeft, true);
    auto src = getLocalBounds().reduced (10, 0);
    g.setColour (Colour (0xff6b5a48)); g.setFont (mono (9.5f, false));
    const String sn = proc.sampleName (t);
    g.drawText ("FONTE: " + (sn.isEmpty() ? U ("S\xc3\x8dNTESE") : sn), src.withY (136).withHeight (14), Justification::centredLeft, true);
    g.setColour (col::label); g.setFont (sans (9.5f));
    const char* names[3] = { "DURA\xc3\x87\xc3\x83O", "VOLUME", "PAN" };
    for (int k = 0; k < 3; ++k)
    {
        auto* s = k == 0 ? &dec : k == 1 ? &vol : &pan;
        g.drawText (U (names[k]), s->getX(), s->getY() - 13, s->getWidth(), 12, Justification::centred);
    }
}

void StripComp::resized()
{
    auto r = getLocalBounds().reduced (10, 8);
    r.removeFromTop (24);
    mute.setBounds (r.removeFromTop (30).withWidth (jmin (r.getWidth(), 130)));
    r.removeFromTop (4);
    auto row = r.removeFromTop (38);
    prev.setBounds (row.removeFromLeft (28)); next.setBounds (row.removeFromRight (28));
    swap.setBounds (row.reduced (3, 0));
    r.removeFromTop (4);
    auto sr = r.removeFromTop (26);
    unload.setBounds (sr.removeFromRight (30)); load.setBounds (sr.withTrimmedRight (4));
    r.removeFromTop (16);
    drag.setBounds (r.removeFromTop (28));
    r.removeFromTop (14);
    const int w = r.getWidth() / 3;
    dec.setBounds (r.removeFromLeft (w).withHeight (62)); vol.setBounds (r.removeFromLeft (w).withHeight (62)); pan.setBounds (r.withHeight (62));
}

// ================================================================== editor
void ReggaePercussionEditor::swapMenu (ReggaePercussionProcessor& proc, int track, Component* target)
{
    PopupMenu m;
    const int cur = proc.soundOf (track);
    for (int i = 0; i < mrp::numSounds; ++i)
    {
        m.addItem (i + 1, soundLabel (i) + "   (nota " + String (proc.padNote[(size_t) mrp::padOf (i, false)].load()) + ")", true, i == cur);
    }
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (target), [&proc, track] (int r)
    {
        if (r > 0) { proc.setParam (ReggaePercussionProcessor::tid ("snd", track), (float) (r - 1)); proc.playPad (mrp::padOf (r - 1, true), 0.95f); }
    });
}

void ReggaePercussionEditor::knob (Slider& s, const String& id, std::unique_ptr<SA>& att)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::TextBoxBelow, true, 64, 16);
    s.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    att = std::make_unique<SA> (proc.apvts, id, s);
    addAndMakeVisible (s);
}

void ReggaePercussionEditor::toggle (TextButton& b, const String& text, const String& id, std::unique_ptr<BA>& att, const String& kind)
{
    b.setButtonText (text); b.setClickingTogglesState (true);
    b.getProperties().set ("kind", kind); b.getProperties().set ("led", true);
    att = std::make_unique<BA> (proc.apvts, id, b);
    addAndMakeVisible (b);
}

void ReggaePercussionEditor::button (TextButton& b, const String& text, std::function<void()> click, const String& kind)
{
    b.setButtonText (text); b.getProperties().set ("kind", kind); b.onClick = std::move (click);
    addAndMakeVisible (b);
}

ReggaePercussionEditor::ReggaePercussionEditor (ReggaePercussionProcessor& p)
    : AudioProcessorEditor (p), proc (p), lcd (p), eq (p), dragAll (p, -1, -1), grid (p, *this)
{
    setLookAndFeel (&lnf);

    // cabeçalho e transporte
    toggle (playBtn, "PLAY", "preview", playA, "go");
    playBtn.getProperties().set ("led", false);
    playBtn.onStateChange = [this] { playBtn.setButtonText (playBtn.getToggleState() ? "STOP" : "PLAY"); playBtn.getProperties().set ("kind", playBtn.getToggleState() ? "red" : "go"); };
    toggle (dawBtn, "TOCA COM\nA DAW", "machineOn", dawA, "key");
    dawBtn.setTooltip (U ("Ligado: a PERC MACHINE toca junto com o play da DAW, no andamento dela"));
    button (prevBtn, "<", [this] { proc.loadGroove ((proc.grooveIndex.load() - 1 + (int) mrp::grooves().size()) % (int) mrp::grooves().size()); });
    button (nextBtn, ">", [this] { proc.loadGroove ((proc.grooveIndex.load() + 1) % (int) mrp::grooves().size()); });
    addAndMakeVisible (lcd);
    {
        String grp; int id = 1;
        for (auto& gv : mrp::grooves())
        {
            if (grp != gv.group) { grp = gv.group; grooveBox.addSectionHeading (grp); }
            grooveBox.addItem (gv.name, id++);
        }
        grooveBox.onChange = [this] { if (grooveBox.getSelectedId() > 0) proc.loadGroove (grooveBox.getSelectedId() - 1); };
        addAndMakeVisible (grooveBox);
    }
    button (verBtn, "v" + String (JucePlugin_VersionString).upToLastOccurrenceOf (".", false, false), [] {}, "dark");
    verBtn.setTooltip ("Mastor Reggae Percussion MRP-77 · Dub Station");
    button (tabPads, U ("PADS\nsons \xc2\xb7 pe\xc3\xa7" "as \xc2\xb7 efeitos"), [this] { setPage (0); }, "tab");
    button (tabMach, U ("PERC MACHINE\ngroove \xc2\xb7 compassos \xc2\xb7 MIDI"), [this] { setPage (1); }, "tab");

    // PADS
    for (int i = 0; i < mrp::numPads; ++i) { padComps.push_back (std::make_unique<PadComp> (proc, i)); addAndMakeVisible (*padComps.back()); }
    addAndMakeVisible (eq);
    eqPreset.addItemList ({ "Flat", "LPF", "HPF", "Banda", "Dub telefone", "Brilho" }, 1);
    eqPreset.setTextWhenNothingSelected ("Presets");
    eqPreset.onChange = [this]
    {
        static const float P[6][4] = { { 30, 2500, 0, 18000 }, { 30, 2500, 0, 3500 }, { 900, 2500, 0, 18000 }, { 700, 2500, 3, 5000 }, { 1200, 2200, 6, 3800 }, { 150, 9000, 5, 18000 } };
        const int i = eqPreset.getSelectedId() - 1; if (i < 0) return;
        proc.setParam ("hpf", P[i][0]); proc.setParam ("bellF", P[i][1]); proc.setParam ("bellG", P[i][2]); proc.setParam ("lpf", P[i][3]);
        eq.repaint();
    };
    addAndMakeVisible (eqPreset);
    toggle (eqBtn, "EQ", "eqOn", eqA);
    toggle (satBtn, "SATURADOR", "satOn", satA, "dark");
    toggle (echoBtn, "TAPE ECHO", "echoOn", echoA, "dark");
    knob (big, "big", bigA); knob (drive, "drive", driveA); knob (echoFb, "echoFb", echoFbA); knob (echoMix, "echoMix", echoMixA);
    big.setTextBoxStyle (Slider::TextBoxBelow, true, 60, 16);
    for (auto* s : { &big, &drive, &echoFb, &echoMix }) { s->textFromValueFunction = [] (double v) { return String (roundToInt (v * 100)) + "%"; }; s->updateText(); }
    echoTime.addItemList ({ "1/8 pontuada", "1/8", "1/4", "1/4 pontuada" }, 1);
    echoTimeA = std::make_unique<CA> (proc.apvts, "echoTime", echoTime);
    addAndMakeVisible (echoTime);
    for (int t = 0; t < mrp::numTracks; ++t) { strips.push_back (std::make_unique<StripComp> (proc, t)); addAndMakeVisible (*strips.back()); }

    // PERC MACHINE
    dragAll.big = true; dragAll.title = "ARRASTE O MIDI PARA A DAW";
    dragAll.setTooltip ("Segure e solte na timeline da DAW");
    addAndMakeVisible (dragAll);
    for (int b = 0; b < 8; ++b)
    {
        chips.push_back (std::make_unique<DragZone> (proc, -1, b));
        chips.back()->title = "C" + String (b + 1); chips.back()->colour = barColour (b);
        chips.back()->setTooltip (U ("Arraste para a DAW s\xc3\xb3 o compasso ") + String (b + 1) + ", na cor dele");
        addAndMakeVisible (*chips.back());
    }
    button (exportBtn, String (CharPointer_UTF8 ("\xe2\xac\x87 BAIXAR MIDI")), [this] { exportMidi(); }, "go");
    toggle (colorBtn, "MIDI COLORIDO", "midiColor", colorA);
    colorBtn.setTooltip ("Cada compasso num canal (1, 2, 3...): na DAW cada compasso aparece numa cor");
    toggle (swingMidiBtn, "SWING NO MIDI", "midiSwing", swingMidiA);
    for (int c = 1; c <= 16; ++c) chBox.addItem ("Canal " + String (c), c);
    chBox.setSelectedId ((int) proc.get ("midiCh"), dontSendNotification);
    chBox.onChange = [this] { proc.setParam ("midiCh", (float) chBox.getSelectedId()); };
    addAndMakeVisible (chBox);
    knob (swingK, "swing", swingA); knob (humanK, "human", humanA); knob (bpmK, "bpm", bpmA);
    swingK.textFromValueFunction = humanK.textFromValueFunction = [] (double v) { return String (roundToInt (v * 100)) + "%"; };
    bpmK.textFromValueFunction = [] (double v) { return String (roundToInt (v)); };
    swingK.updateText(); humanK.updateText(); bpmK.updateText();
    toggle (guideBtn, "GUIA: CAIXA\n+ PALHETA", "guide", guideA);
    button (clearBtn, "LIMPAR", [this] { proc.clearPattern(); grid.repaint(); });
    button (barsDown, "<", [this] { proc.setParam ("bars", (float) jmax (0, (int) proc.get ("bars") - 1)); });
    button (barsUp, ">", [this] { proc.setParam ("bars", (float) jmin (3, (int) proc.get ("bars") + 1)); });
    for (int b = 0; b < 8; ++b)
    {
        barSel[b].setButtonText (String (b + 1));
        barSel[b].getProperties().set ("kind", "key"); barSel[b].getProperties().set ("led", true); barSel[b].getProperties().set ("ledColour", (int64) barCols[b]);
        barSel[b].onClick = [this, b] { editBar = b; refreshBars(); grid.repaint(); };
        addAndMakeVisible (barSel[b]);
    }
    button (editAllBtn, "TODOS OS COMPASSOS", [this] { editAll = ! editAll; editAllBtn.setToggleState (editAll, dontSendNotification); editAllBtn.setButtonText (editAll ? U ("TODOS OS COMPASSOS") : U ("S\xc3\x93 ESTE COMPASSO")); });
    editAllBtn.setToggleState (true, dontSendNotification);
    button (followBtn, "SEGUIR O PLAY", [this] { follow = ! follow; followBtn.setToggleState (follow, dontSendNotification); });
    followBtn.setToggleState (true, dontSendNotification);
    for (int t = 0; t < mrp::numTracks; ++t)
    {
        muteKey[t].setClickingTogglesState (true); muteKey[t].getProperties().set ("kind", "mutekey");
        muteKeyA[t] = std::make_unique<BA> (proc.apvts, ReggaePercussionProcessor::tid ("mute", t), muteKey[t]);
        addAndMakeVisible (muteKey[t]);
        button (rowPrev[t], "<", [this, t] { proc.cycleSound (t, -1); }, "dark");
        button (rowSwap[t], String (CharPointer_UTF8 ("\xe2\x86\x94")), [this, t] { swapMenu (proc, t, &rowSwap[t]); }, "dark");
        button (rowNext[t], ">", [this, t] { proc.cycleSound (t, 1); }, "dark");
        rowPrev[t].setTooltip (U ("Pe\xc3\xa7" "a anterior")); rowNext[t].setTooltip (U ("Pr\xc3\xb3xima pe\xc3\xa7" "a")); rowSwap[t].setTooltip (U ("Escolher a pe\xc3\xa7" "a na lista"));
        rowM[t].setButtonText ("M"); rowM[t].setClickingTogglesState (true); rowM[t].getProperties().set ("kind", "dark");
        rowS[t].setButtonText ("S"); rowS[t].setClickingTogglesState (true); rowS[t].getProperties().set ("kind", "dark");
        rowMA[t] = std::make_unique<BA> (proc.apvts, ReggaePercussionProcessor::tid ("mute", t), rowM[t]);
        rowSA[t] = std::make_unique<BA> (proc.apvts, ReggaePercussionProcessor::tid ("solo", t), rowS[t]);
        rowM[t].setTooltip ("Mudo"); rowS[t].setTooltip ("Solo");
        addAndMakeVisible (rowM[t]); addAndMakeVisible (rowS[t]);
    }
    addAndMakeVisible (grid);
    for (int t = 0; t < mrp::numTracks; ++t) for (auto* c : { (Component*) &rowPrev[t], (Component*) &rowSwap[t], (Component*) &rowNext[t], (Component*) &rowM[t], (Component*) &rowS[t] }) c->toFront (false);
    button (resetNotesBtn, U ("NOTAS DE F\xc3\x81" "BRICA"), [this] { proc.resetNotes(); refreshNotes(); });
    for (int i = 0; i < mrp::numPads; ++i)
    {
        const auto& pd = mrp::pads()[(size_t) i];
        noteLbl[i].setText (pd.lg < 0 ? soundLabel (pd.snd) : U (mrp::sounds()[(size_t) pd.snd].name) + (pd.lg ? " Longo" : " Curto"), dontSendNotification);
        noteLbl[i].setFont (sans (11.0f, false));
        addAndMakeVisible (noteLbl[i]);
        noteSl[i].setSliderStyle (Slider::IncDecButtons);
        noteSl[i].setIncDecButtonsMode (Slider::incDecButtonsDraggable_Vertical);
        noteSl[i].setTextBoxStyle (Slider::TextBoxLeft, false, 34, 16);
        noteSl[i].setRange (0, 127, 1);
        noteSl[i].setValue (proc.padNote[(size_t) i].load(), dontSendNotification);
        noteSl[i].onValueChange = [this, i] { proc.padNote[(size_t) i] = (int) noteSl[i].getValue(); refreshNotes(); for (auto& s : strips) s->refresh(); };
        addAndMakeVisible (noteSl[i]);
    }

    setSize (1180, 820);
    setPage (0);
    refreshNotes();
    startTimerHz (30);
}

ReggaePercussionEditor::~ReggaePercussionEditor() { stopTimer(); setLookAndFeel (nullptr); }

std::vector<Component*> ReggaePercussionEditor::page0()
{
    std::vector<Component*> v { &eq, &eqPreset, &eqBtn, &satBtn, &echoBtn, &big, &drive, &echoFb, &echoMix, &echoTime };
    for (auto& p : padComps) v.push_back (p.get());
    for (auto& s : strips) v.push_back (s.get());
    return v;
}

std::vector<Component*> ReggaePercussionEditor::page1()
{
    std::vector<Component*> v { &dragAll, &exportBtn, &colorBtn, &swingMidiBtn, &chBox, &swingK, &humanK, &bpmK, &guideBtn, &clearBtn,
                                &barsDown, &barsUp, &editAllBtn, &followBtn, &grid, &resetNotesBtn };
    for (auto& c : chips) v.push_back (c.get());
    for (int b = 0; b < 8; ++b) v.push_back (&barSel[b]);
    for (int t = 0; t < mrp::numTracks; ++t) for (auto* c : { (Component*) &muteKey[t], (Component*) &rowPrev[t], (Component*) &rowSwap[t], (Component*) &rowNext[t], (Component*) &rowM[t], (Component*) &rowS[t] }) v.push_back (c);
    for (int i = 0; i < mrp::numPads; ++i) { v.push_back (&noteLbl[i]); v.push_back (&noteSl[i]); }
    return v;
}

void ReggaePercussionEditor::setPage (int p)
{
    page = p;
    for (auto* c : page0()) c->setVisible (p == 0);
    for (auto* c : page1()) c->setVisible (p == 1);
    tabPads.setToggleState (p == 0, dontSendNotification);
    tabMach.setToggleState (p == 1, dontSendNotification);
    shownBars = -1;
    if (p == 1) refreshBars();
    repaint();
}

void ReggaePercussionEditor::refreshBars()
{
    const int bars = proc.numBars();
    editBar = jlimit (0, bars - 1, editBar);
    for (int b = 0; b < 8; ++b)
    {
        barSel[b].setVisible (page == 1 && b < bars);
        barSel[b].setToggleState (b == editBar, dontSendNotification);
        chips[(size_t) b]->setVisible (page == 1 && b < bars);
        chips[(size_t) b]->sub = proc.get ("midiColor") > 0.5f ? "canal " + String (b + 1) : "canal " + String ((int) proc.get ("midiCh"));
        chips[(size_t) b]->repaint();
    }
    shownBars = bars;
    repaint();
}

void ReggaePercussionEditor::refreshNotes()
{
    std::map<int, int> used;
    for (auto& n : proc.padNote) used[n.load()]++;
    for (int i = 0; i < mrp::numPads; ++i)
    {
        const int n = proc.padNote[(size_t) i].load();
        noteSl[i].setValue (n, dontSendNotification);
        const bool bad = mrp::isDrumNote (n) || used[n] > 1;
        noteLbl[i].setColour (Label::textColourId, bad ? col::red : col::ink);
        noteLbl[i].setTooltip (mrp::isDrumNote (n) ? U ("Conflita com o MRD-12 (notas 36 a 51)") : used[n] > 1 ? U ("Nota repetida") : noteName (n));
    }
    for (auto& p : padComps) p->repaint();
}

void ReggaePercussionEditor::exportMidi()
{
    auto dir = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Mastor Reggae Percussion");
    dir.createDirectory();
    chooser = std::make_unique<FileChooser> ("Salvar o groove em MIDI", dir.getChildFile (proc.tempMidi().getFileName()), "*.mid");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                          [this] (const FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == File()) return;
                              if (! f.hasFileExtension ("mid")) f = f.withFileExtension ("mid");
                              proc.writeMidiFile (f);
                          });
}

void ReggaePercussionEditor::timerCallback()
{
    // luzes no ritmo: pads, canais, teclas de mudo e nomes da grade
    for (int i = 0; i < mrp::numPads; ++i)
    {
        const uint32_t h = proc.padHits[(size_t) i].load();
        auto& pc = *padComps[(size_t) i];
        if (h != lastPad[i]) { lastPad[i] = h; pc.flash = 1.0f; if (page == 0) pc.repaint(); }
        else if (pc.flash > 0) { pc.flash = jmax (0.0f, pc.flash - 0.22f); if (page == 0) pc.repaint(); }
    }
    for (int t = 0; t < mrp::numTracks; ++t)
    {
        const uint32_t h = proc.trackHits[(size_t) t].load();
        auto& st = *strips[(size_t) t];
        const float v = jlimit (0.35f, 1.0f, proc.trackVel[(size_t) t].load());
        if (h != lastTrack[t]) { lastTrack[t] = h; st.flash = v; rowFlash[t] = v; keyFlash[t] = v; }
        else { st.flash = jmax (0.0f, st.flash - 0.2f); rowFlash[t] = jmax (0.0f, rowFlash[t] - 0.2f); keyFlash[t] = jmax (0.0f, keyFlash[t] - 0.2f); }
        st.refresh();
        if (page == 0) st.repaint();
        const auto& d = mrp::sounds()[(size_t) proc.soundOf (t)];
        const String txt = UP (U (d.name)) + "\n" + (proc.get (ReggaePercussionProcessor::tid ("mute", t)) > 0.5f ? "MUDO" : proc.isAudible (t) ? "TOCANDO" : "FORA DO SOLO");
        if (muteKey[t].getButtonText() != txt) muteKey[t].setButtonText (txt);
        muteKey[t].getProperties().set ("ledColour", (int64) d.argb);
        muteKey[t].getProperties().set ("flash", keyFlash[t]);
        if (page == 1) muteKey[t].repaint();
    }
    // compasso tocando: a grade acompanha (SEGUIR O PLAY)
    const int ps = proc.playStep.load();
    if (follow && ps >= 0 && ps / 16 != editBar && ps / 16 < proc.numBars()) { editBar = ps / 16; refreshBars(); }
    if (proc.numBars() != shownBars && page == 1) refreshBars();
    if (ps != shownStep) { shownStep = ps; lcd.repaint(); repaint (0, 130, getWidth(), 28); if (page == 1) grid.repaint(); }
    else if (page == 1) grid.repaint();
    if (proc.grooveIndex.load() != shownGroove)
    {
        shownGroove = proc.grooveIndex.load();
        grooveBox.setSelectedId (shownGroove + 1, dontSendNotification);
        lcd.repaint(); grid.repaint();
    }
    dragAll.sub = String (proc.numBars()) + " compasso(s) . " + String (roundToInt (proc.get ("bpm"))) + " BPM no preview . "
                + (proc.get ("midiColor") > 0.5f ? "MIDI colorido: canal 1 a " + String (proc.numBars()) : "canal " + String ((int) proc.get ("midiCh")));
    if (page == 1) dragAll.repaint();
    if (page == 0) eq.repaint();
}

void ReggaePercussionEditor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff0e0a07));
    // laterais de madeira
    for (auto side : { Rectangle<int> (0, 0, 14, getHeight()), Rectangle<int> (getWidth() - 14, 0, 14, getHeight()) })
    {
        g.setGradientFill (ColourGradient (Colour (0xff2a160b), (float) side.getX(), 0, Colour (0xff5b341b), (float) side.getRight(), 0, false));
        g.fillRect (side);
    }
    auto unit = getLocalBounds().reduced (14, 0);
    g.setGradientFill (ColourGradient (Colour (0xffece2c8), (float) unit.getX(), 0, Colour (0xffcdbf9c), (float) unit.getRight(), (float) unit.getBottom(), true));
    g.fillRect (unit);
    // cabeçalho oliva
    auto head = unit.withHeight (52);
    g.setGradientFill (ColourGradient (col::olive, 0, 0, col::olive2, 0, 52, false)); g.fillRect (head);
    g.setColour (Colour (0xff4d6534)); g.fillRect (head.withTop (49).withHeight (3));
    g.setColour (Colour (0xfff1ecdc)); g.setFont (sans (20.0f));
    g.drawText ("M A S T O R   R E G G A E   P E R C U S S I O N", head.withTrimmedLeft (18).withHeight (32), Justification::bottomLeft);
    g.setColour (Colour (0xffe3e4cf)); g.setFont (sans (9.5f));
    g.drawText (U ("PERCUSS\xc3\x83O DE REGGAE  \xc2\xb7  DUB STATION  \xc2\xb7  CONTAGEM COM A CAIXA NO 2 E NO 4"), head.withTrimmedLeft (18).withTop (34).withHeight (12), Justification::left);
    const Colour stripes[3] = { col::red, Colour (0xffe6b83a), col::green };
    for (int k = 0; k < 3; ++k) { g.setColour (stripes[k]); g.fillRect (unit.getX() + 600 + k * 34, 22, 30, 5); }
    auto plate = Rectangle<float> ((float) unit.getX() + 720, 12, 100, 28);
    g.setGradientFill (ColourGradient (Colour (0xffe9c86a), plate.getX(), plate.getY(), Colour (0xffb38a2e), plate.getX(), plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 4.0f); g.setColour (Colour (0xff7a5a18)); g.drawRoundedRectangle (plate, 4.0f, 1.2f);
    g.setColour (Colour (0xff2b1f0c)); g.setFont (sans (14.0f)); g.drawText ("MRP-77", plate, Justification::centred);

    // contagem
    const int ps = proc.playStep.load();
    for (int s = 0; s < 16; ++s)
    {
        auto c = Rectangle<float> (316.0f + s * 22.0f + (s / 4) * 6.0f, 134.0f, 19.0f, 19.0f);
        const bool now = ps >= 0 && ps % 16 == s;
        g.setColour (now ? col::amber : Colour (0xfffbf7ec)); g.fillEllipse (c);
        g.setColour (mrp::isSnareStep (s) ? Colour (0xffb8392b) : Colour (0xff24201c)); g.drawEllipse (c, 1.2f);
        g.setColour (mrp::isSnareStep (s) && ! now ? Colour (0xffb8392b) : col::label); g.setFont (mono (10.0f));
        g.drawText (mrp::countName (s), c, Justification::centred);
    }

    // títulos das seções (etiquetas pretas)
    auto tag = [&g] (int x, int y, const String& t)
    {
        g.setFont (sans (11.0f));
        const int w = GlyphArrangement::getStringWidthInt (g.getCurrentFont(), t) + 26;
        g.setColour (Colour (0xff151110)); g.fillRoundedRectangle ((float) x, (float) y, (float) w, 20, 3);
        g.setColour (Colour (0xffefe6d0)); g.drawText (t, x, y, w, 20, Justification::centred);
    };
    auto box = [&g] (Rectangle<int> r) { g.setColour (Colour (0x30fffaeb)); g.fillRoundedRectangle (r.toFloat(), 8); g.setColour (col::line); g.drawRoundedRectangle (r.toFloat(), 8, 1.0f); };
    if (page == 0)
    {
        box ({ 20, 218, 612, 290 }); tag (32, 208, "P A D S");
        box ({ 642, 218, 518, 290 }); tag (654, 208, "E F E I T O S");
        g.setColour (Colour (0xff15110e)); g.fillRoundedRectangle (28.0f, 234.0f, 596.0f, 266.0f, 6.0f);
        // saturador e tape echo
        g.setGradientFill (ColourGradient (Colour (0xff5a2a1e), 0, 418, Colour (0xff3e1c13), 0, 500, false)); g.fillRoundedRectangle (652.0f, 418.0f, 226.0f, 82.0f, 8.0f);
        g.setGradientFill (ColourGradient (Colour (0xff5d6b4f), 0, 418, Colour (0xff46523b), 0, 500, false)); g.fillRoundedRectangle (886.0f, 418.0f, 266.0f, 82.0f, 8.0f);
        g.setColour (Colours::black); g.drawRoundedRectangle (652.0f, 418.0f, 226.0f, 82.0f, 8.0f, 2.0f); g.drawRoundedRectangle (886.0f, 418.0f, 266.0f, 82.0f, 8.0f, 2.0f);
        g.setColour (Colour (0xfff6efd9)); g.setFont (sans (10.0f));
        g.drawText ("DRIVE", 760, 486, 60, 12, Justification::centred);
        g.drawText ("FEEDBACK", 1010, 486, 70, 12, Justification::centred); g.drawText ("MIX", 1082, 486, 60, 12, Justification::centred);
        g.setColour (Colour (0xff15110e)); g.fillRoundedRectangle (1018.0f, 228.0f, 134.0f, 182.0f, 8.0f);
        g.setColour (col::amber); g.setFont (sans (11.0f)); g.drawText ("MASTOR BIG KNOB", 1018, 234, 134, 14, Justification::centred);
        g.setColour (Colour (0xffd9cfbd)); g.setFont (sans (10.0f, false)); g.drawText (U ("sa\xc3\xad" "da"), 1018, 392, 134, 12, Justification::centred);
        box ({ 20, 528, 1140, 282 }); tag (32, 518, "C A N A I S");
    }
    else
    {
        box ({ 20, 218, 1140, 92 }); tag (32, 208, "M I D I");
        box ({ 20, 330, 860, 480 }); tag (32, 320, "G R O O V E");
        box ({ 890, 330, 270, 480 }); tag (902, 320, "E S Q U E M A   M I D I");
        g.setColour (col::label); g.setFont (sans (9.5f));
        g.drawText (U ("BAIXAR OU ARRASTAR S\xc3\x93 UM COMPASSO, NA COR DELE"), 860, 226, 296, 12, Justification::left);
        g.drawText ("SWING", 30, 410, 60, 12, Justification::centred); g.drawText ("HUMANIZAR", 92, 410, 70, 12, Justification::centred); g.drawText ("BPM PREVIEW", 164, 410, 70, 12, Justification::centred);
        g.drawText ("COMPASSOS", 372, 342, 120, 12, Justification::left);
        g.drawText ("COMPASSO NA GRADE", 500, 342, 200, 12, Justification::left);
        // número de compassos (visor âmbar)
        auto nb = Rectangle<float> (404.0f, 358.0f, 44.0f, 30.0f);
        g.setColour (Colour (0xff1d1915)); g.fillRoundedRectangle (nb, 4.0f);
        g.setColour (col::amber); g.setFont (mono (18.0f)); g.drawText (String (proc.numBars()), nb, Justification::centred);
        g.setColour (col::label); g.setFont (sans (10.5f, false));
        g.drawFittedText (U ("O MRD-12 (bateria) usa as notas 36 a 51: o MRP-77 fica fora dessa faixa. Ajuste a nota de cada pe\xc3\xa7" "a para o seu kit (vermelho = conflito)."),
                          Rectangle<int> (900, 342, 252, 40), Justification::topLeft, 3);
    }
}

void ReggaePercussionEditor::resized()
{
    // transporte
    playBtn.setBounds (30, 64, 100, 50);
    dawBtn.setBounds (140, 64, 120, 50);
    prevBtn.setBounds (272, 64, 36, 52);
    lcd.setBounds (312, 58, 420, 72);
    nextBtn.setBounds (736, 64, 36, 52);
    grooveBox.setBounds (784, 74, 236, 30);
    verBtn.setBounds (1036, 72, 120, 34);
    tabPads.setBounds (30, 160, 556, 46);
    tabMach.setBounds (594, 160, 556, 46);

    // PADS
    for (int i = 0; i < mrp::numPads; ++i)
    {
        const int c = i % 6, r = i / 6;
        padComps[(size_t) i]->setBounds (34 + c * 98, 240 + r * 64, 94, 60);
    }
    eqBtn.setBounds (652, 230, 64, 28);
    eqPreset.setBounds (724, 230, 130, 26);
    eq.setBounds (652, 264, 358, 146);
    big.setBounds (1028, 250, 114, 140);
    satBtn.setBounds (660, 426, 110, 34);
    drive.setBounds (772, 424, 40, 62);
    drive.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
    echoBtn.setBounds (894, 426, 110, 34);
    echoTime.setBounds (894, 466, 110, 24);
    echoFb.setBounds (1020, 424, 40, 62); echoMix.setBounds (1090, 424, 40, 62);
    echoFb.setTextBoxStyle (Slider::NoTextBox, true, 0, 0); echoMix.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
    for (int t = 0; t < mrp::numTracks; ++t) strips[(size_t) t]->setBounds (30 + t * 188, 538, 184, 266);

    // PERC MACHINE
    dragAll.setBounds (28, 230, 520, 70);
    exportBtn.setBounds (556, 234, 150, 58);
    colorBtn.setBounds (714, 234, 140, 28);
    swingMidiBtn.setBounds (714, 266, 140, 28);
    chBox.setBounds (860, 268, 100, 26);
    for (int b = 0; b < 8; ++b) chips[(size_t) b]->setBounds (966 + (b % 4) * 48, 240 + (b / 4) * 30, 44, 28);
    swingK.setBounds (30, 340, 60, 70); humanK.setBounds (96, 340, 60, 70); bpmK.setBounds (166, 340, 60, 70);
    guideBtn.setBounds (238, 340, 124, 40);
    clearBtn.setBounds (238, 384, 124, 26);
    barsDown.setBounds (372, 356, 30, 34); barsUp.setBounds (450, 356, 30, 34);
    for (int b = 0; b < 8; ++b) barSel[b].setBounds (500 + b * 40, 356, 36, 34);
    editAllBtn.setBounds (500, 394, 160, 24);
    followBtn.setBounds (666, 394, 120, 24);
    for (int t = 0; t < mrp::numTracks; ++t) muteKey[t].setBounds (30 + t * 141, 424, 136, 44);
    grid.setBounds (30, 476, 842, 326);
    const float rowH = (326.0f - GridComp::headH) / (float) mrp::numTracks;
    for (int t = 0; t < mrp::numTracks; ++t)
    {
        const int y = 476 + GridComp::headH + (int) (t * rowH + rowH * 0.5f) - 12;
        rowPrev[t].setBounds (30 + 112, y, 22, 24); rowSwap[t].setBounds (30 + 136, y, 24, 24); rowNext[t].setBounds (30 + 162, y, 22, 24);
        rowM[t].setBounds (30 + 186, y, 22, 24); rowS[t].setBounds (30 + 210, y, 22, 24);
    }
    resetNotesBtn.setBounds (900, 384, 150, 24);
    for (int i = 0; i < mrp::numPads; ++i)
    {
        noteLbl[i].setBounds (900, 414 + i * 16, 160, 16);
        noteSl[i].setBounds (1062, 414 + i * 16, 90, 16);
    }
}
