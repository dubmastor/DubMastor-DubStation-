#include "PluginEditor.h"
#include "RiddimBankMenu.h"

using namespace juce;

namespace col
{
const Colour bg { 0xff121212 }, panel { 0xff1f1f1f }, line { 0xff333333 }, cream { 0xffe9dfc6 }, cream2 { 0xffd6c8a6 },
             ink { 0xff2b241a }, ink2 { 0xff6b5c44 }, green { 0xff7fbf6a }, lcd { 0xff8fc06c }, amber { 0xfff6a531 },
             clay { 0xffc2741f }, red { 0xffb3261e }, wood { 0xff1f150d }, gold { 0xfff6c94d };
const Colour chord[4] = { Colour (0xffffa928), Colour (0xffff6f4f), Colour (0xff5fae4f), Colour (0xff3f8fd9) };
}

static Font mono (float size, bool bold = true) { return Font (FontOptions (Font::getDefaultMonospacedFontName(), size, bold ? Font::bold : Font::plain)); }
static Font sans (float size, bool bold = false) { return Font (FontOptions (size, bold ? Font::bold : Font::plain)); }

// ================================================================== LookAndFeel
ClavLookAndFeel::ClavLookAndFeel()
{
    setColour (ComboBox::backgroundColourId, Colour (0xff0d0a07));
    setColour (ComboBox::textColourId, col::amber);
    setColour (ComboBox::outlineColourId, Colour (0xff3a2a18));
    setColour (ComboBox::arrowColourId, col::amber);
    setColour (PopupMenu::backgroundColourId, Colour (0xff1a1714));
    setColour (PopupMenu::textColourId, Colour (0xffefe5cc));
    setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xff3a2f1c));
    setColour (PopupMenu::highlightedTextColourId, Colour (0xffffc061));
    setColour (PopupMenu::headerTextColourId, Colour (0xffc9a45c));
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
}

void ClavLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const String kind = b.getProperties()["kind"].toString();
    const bool on = b.getToggleState();
    Colour fill, edge;
    if (kind == "big")        { fill = Colour (0xff1a8a3c); edge = Colour (0xff0a4a1c); }
    else if (kind == "light") { fill = on ? col::ink : Colour (0xffefe6cf); edge = on ? col::ink : Colour (0xffb9a98a); }
    else if (kind == "tab")   { fill = on ? col::green : Colour (0xff1d1d1d); edge = on ? Colour (0xffa5d98f) : Colour (0xff3a3a3a); }
    else                      { fill = on ? col::green : Colour (0xff1c1c1c); edge = on ? Colour (0xffa5d98f) : Colour (0xff3c3c3c); }
    if (kind == "big")
    {
        ColourGradient gr (Colour (0xff22a447), r.getX(), r.getY(), Colour (0xff0d6e2a), r.getX(), r.getBottom(), false);
        g.setGradientFill (gr);
    }
    else g.setColour (fill);
    if (over && b.isEnabled()) g.setColour (kind == "big" ? Colour (0xff2bb653) : fill.brighter (0.12f));
    if (down) g.setColour (fill.darker (0.2f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (r, 4.0f, 1.2f);
}

void ClavLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    const String kind = b.getProperties()["kind"].toString();
    const bool on = b.getToggleState();
    Colour c = kind == "big" ? Colour (0xfff6e27a) : kind == "light" ? (on ? col::gold : Colour (0xff4a3f2e))
             : (on ? Colour (0xff10200b) : Colour (0xffe6e6e6));
    if (! b.isEnabled()) c = c.withAlpha (0.35f);
    g.setColour (c);
    g.setFont (mono (kind == "big" ? 13.0f : 11.5f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 2), Justification::centred, 2, 0.8f);
}

void ClavLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider&)
{
    const float d = (float) jmin (w, h) - 6.0f, cx = x + w * 0.5f, cy = y + h * 0.5f;
    auto r = Rectangle<float> (d, d).withCentre ({ cx, cy });
    g.setColour (Colour (0xff9a9a9a)); g.fillEllipse (r);
    ColourGradient kn (Colour (0xff3a3a3a), r.getX(), r.getY(), Colour (0xff0d0d0d), r.getRight(), r.getBottom(), false);
    g.setGradientFill (kn); g.fillEllipse (r.reduced (3.0f));
    const float ang = a0 + pos * (a1 - a0);
    Path p; p.addRectangle (-1.5f, -d * 0.42f, 3.0f, d * 0.32f);
    g.setColour (Colours::white); g.fillPath (p, AffineTransform::rotation (ang).translated (cx, cy));
}

void ClavLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider::SliderStyle, Slider&)
{
    const float cy = y + h * 0.5f;
    g.setColour (Colour (0xff1b1610)); g.fillRoundedRectangle ((float) x, cy - 3.0f, (float) w, 6.0f, 3.0f);
    g.setColour (col::clay); g.fillRoundedRectangle ((float) x, cy - 3.0f, pos - x, 6.0f, 3.0f);
    auto th = Rectangle<float> (14.0f, 20.0f).withCentre ({ pos, cy });
    ColourGradient gr (Colour (0xffefe7d6), th.getX(), th.getY(), Colour (0xffb5aa94), th.getX(), th.getBottom(), false);
    g.setGradientFill (gr); g.fillRoundedRectangle (th, 3.0f);
    g.setColour (Colour (0xff5a5040)); g.drawRoundedRectangle (th, 3.0f, 1.0f);
}

void ClavLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat();
    auto box = r.removeFromLeft (r.getHeight()).reduced (3.0f);
    g.setColour (b.getToggleState() ? col::green : Colour (0xff2a2a2a));
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (Colour (0xff555555)); g.drawRoundedRectangle (box, 3.0f, 1.0f);
    g.setColour (b.findColour (ToggleButton::textColourId));
    g.setFont (mono (11.0f));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().withTrimmedLeft (4), Justification::centredLeft, 1);
}

Font ClavLookAndFeel::getComboBoxFont (ComboBox&) { return mono (11.5f); }
Font ClavLookAndFeel::getPopupMenuFont() { return sans (14.0f); }

void RockerButton::paintButton (Graphics& g, bool, bool)
{
    auto r = getLocalBounds().toFloat();
    auto label = r.removeFromBottom (14.0f);
    auto body = r.reduced (2.0f);
    g.setColour (Colour (0xff1a120a)); g.fillRoundedRectangle (body, 5.0f);
    auto face = body.reduced (2.0f);
    g.setColour (colour); g.fillRoundedRectangle (face, 4.0f);
    // balancim: a metade apertada fica na sombra
    auto shade = getToggleState() ? face.withTrimmedTop (face.getHeight() * 0.68f) : face.withTrimmedBottom (face.getHeight() * 0.68f);
    g.setColour (Colours::black.withAlpha (0.3f)); g.fillRect (shade);
    g.setColour (col::ink); g.setFont (mono (9.5f));
    g.drawText (getButtonText(), label, Justification::centred);
}

// ================================================================== visor MIDI
void LineRoll::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const bool big = style < 0;
    const auto line = big ? proc.getCurrentLine() : proc.getSuggestion (style);
    g.setColour (big ? Colour (0xff17110a) : Colour (0xffefe6cf)); g.fillRoundedRectangle (r, 3.0f);
    beats = big ? line.bars * 4.0 : std::min (8.0, line.bars * 4.0);
    area = big ? r.reduced (6.0f).withTrimmedLeft (26.0f).withTrimmedTop (14.0f) : r.reduced (1.0f);
    auto xOf = [&] (double b) { return area.getX() + (float) (b / beats) * area.getWidth(); };
    // caixa no 2 e no 4
    for (double b = 1; b < beats; b += 2)
    {
        g.setColour (big ? Colour (0xffe5484d) : Colour (0xffb3261e).withAlpha (0.08f));
        if (big) g.fillRect (xOf (b), area.getY() - 3.0f, xOf (b + 1) - xOf (b), 2.0f);
        else g.fillRect (xOf (b), area.getY(), xOf (b + 1) - xOf (b), area.getHeight());
    }
    for (int i = 1; i < (int) (beats * 4); ++i)
    {
        const bool bar = i % 16 == 0, bt = i % 4 == 0;
        g.setColour (big ? col::amber.withAlpha (bar ? 0.28f : (bt ? 0.1f : 0.035f)) : (bar ? Colour (0xff8a7a5c) : bt ? Colour (0xffc4b493) : Colour (0xffddd0b2)));
        g.fillRect (xOf (i / 4.0), area.getY(), 1.0f, area.getHeight());
    }
    lo = 127; hi = 0;
    for (auto& e : line.events) { lo = std::min (lo, e.note); hi = std::max (hi, e.note); }
    if (hi < lo) { lo = (int) proc.get ("center") - 8; hi = lo + 24; }
    lo -= 2; hi += 2; if (hi - lo < 18) { const int m = (hi + lo) / 2; lo = m - 9; hi = m + 9; }
    const float rh = area.getHeight() / (float) (hi - lo + 1);
    auto yOf = [&] (int n) { return area.getBottom() - (float) (n - lo + 1) * rh; };
    if (big)
    {
        for (int n = lo; n <= hi; ++n)
            if (n % 12 == 0) { g.setColour (col::amber.withAlpha (0.55f)); g.setFont (mono (9.0f, false));
                               g.drawText (ReggaeClavinetProcessor::noteName (n), Rectangle<float> (r.getX() + 3, yOf (n) - 3, 24, rh + 6), Justification::centredLeft); }
        // nomes dos acordes
        const auto cs = proc.getChords();
        int prev = -1;
        for (int b = 0; b < (int) beats; ++b)
        {
            const auto ca = mrcl::chordAt (cs, b);
            if (ca.k != prev)
            {
                g.setColour (col::chord[ca.c.slot & 3]); g.fillRect (xOf (b), r.getY() + 2, 1.0f, r.getHeight() - 4);
                g.setFont (mono (10.0f));
                g.drawText (String (mrcl::rootName (ca.c.root)) + (ca.c.minor ? "m" : ""), Rectangle<float> (xOf (b) + 4, r.getY() + 2, 60, 12), Justification::centredLeft);
                prev = ca.k;
            }
        }
    }
    const double pb = proc.playBeat.load();
    for (auto& e : line.events)
    {
        if (e.start >= beats) continue;
        const bool now = big && pb >= 0 && pb >= e.start - 0.01 && pb < e.start + std::max (e.length, 0.25);
        g.setColour (col::chord[e.slot & 3].withAlpha (now ? 1.0f : 0.5f + 0.5f * e.vel / 127.0f));
        g.fillRect (xOf (e.start) + 0.5f, yOf (e.note), std::max (2.5f, xOf (e.start + e.length) - xOf (e.start) - 1.0f), std::max (2.0f, rh - 0.5f));
    }
    if (big && pb >= 0) { g.setColour (Colours::white); g.fillRect (xOf (std::fmod (pb, beats)), area.getY(), 1.0f, area.getHeight()); }
    if (big && proc.hasEdits()) { g.setColour (Colour (0xffffc061)); g.setFont (mono (10.0f)); g.drawText ("MIDI EDITADO", area.reduced (4).toNearestInt(), Justification::bottomLeft); }
}

void LineRoll::mouseDown (const MouseEvent& ev)
{
    if (style >= 0) { proc.setParam ("use", (float) style); proc.clearEdits(); return; }
    if (! area.contains (ev.position) || hi <= lo) return;
    const double beat = (ev.position.x - area.getX()) / area.getWidth() * beats;
    const float rh = area.getHeight() / (float) (hi - lo + 1);
    const int note = lo + (int) std::floor ((area.getBottom() - ev.position.y) / rh);
    auto line = proc.getCurrentLine();
    auto& evs = line.events;
    auto it = std::find_if (evs.begin(), evs.end(), [&] (const mrcl::Event& e)
        { return beat >= e.start - 0.06 && beat <= e.start + std::max (e.length, 0.12) && std::abs (e.note - note) <= 1; });
    if (it != evs.end()) evs.erase (it);
    else
    {
        const double st = std::floor (beat * 4.0) / 4.0;
        const auto ca = mrcl::chordAt (proc.getChords(), st);
        const auto p = mrcl::poolNotes (mrcl::pcs (ca.c), note - 6, note + 6);
        if (p.empty()) return;
        int best = p[0]; for (int n : p) if (std::abs (n - note) < std::abs (best - note)) best = n;
        evs.push_back ({ st, std::max (proc.get ("nota") > 0.5f ? 0.125 : 0.0625, 0.12), best, (int) std::lround (0.85 * proc.get ("velocity") * 127), ca.c.slot });
        std::stable_sort (evs.begin(), evs.end(), [] (const mrcl::Event& a, const mrcl::Event& b) { return a.start < b.start; });
    }
    proc.setEdits (line);
    repaint();
}

// ================================================================== arraste do MIDI
void MidiDragZone::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (Colour (0xff1c140c)); g.fillRoundedRectangle (r, 5.0f);
    Path outline, dashed; outline.addRoundedRectangle (r.reduced (3.0f), 4.0f);
    const float dashes[] = { 5.0f, 4.0f };
    PathStrokeType (1.4f).createDashedStroke (dashed, outline, dashes, 2);
    g.setColour (col::clay.withAlpha (isMouseOver() ? 1.0f : 0.75f)); g.fillPath (dashed);
    g.setColour (col::amber); g.setFont (mono (12.5f));
    g.drawText ("ARRASTE O MIDI", r.withTrimmedBottom (r.getHeight() * 0.45f), Justification::centredBottom);
    g.setColour (col::amber.withAlpha (0.65f)); g.setFont (mono (9.0f, false));
    g.drawText ("PARA A TIMELINE DA DAW", r.withTrimmedTop (r.getHeight() * 0.58f), Justification::centredTop);
}

void MidiDragZone::mouseDrag (const MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    const auto& m = mrcl::musicians()[(size_t) jlimit (0, 6, (int) proc.get ("musician"))];
    auto f = File::getSpecialLocation (File::tempDirectory)
                 .getChildFile (String ("MRCL-71 ") + m.name + " " + mrcl::styleName ((int) proc.get ("use")) + ".mid");
    if (proc.writeMidiFile (f)) DragAndDropContainer::performExternalDragDropOfFiles (StringArray (f.getFullPathName()), false, this);
}

// ================================================================== editor
ReggaeClavinetEditor::ReggaeClavinetEditor (ReggaeClavinetProcessor& p)
    : AudioProcessorEditor (p), proc (p), keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard), roll (p), dragZone (p)
{
    setLookAndFeel (&lnf);
    auto& st = proc.apvts;

    // cabeçalho
    chip (tabInst, "INSTRUMENTO", [this] { setPage (0); }); tabInst.getProperties().set ("kind", "tab");
    chip (tabMach, "CLAVINET MACHINE", [this] { setPage (1); }); tabMach.getProperties().set ("kind", "tab");
    previewBtn.setClickingTogglesState (true); previewBtn.getProperties().set ("kind", "big");
    addAndMakeVisible (previewBtn); previewAtt = std::make_unique<BA> (st, "preview", previewBtn);
    dawBtn.setColour (ToggleButton::textColourId, Colour (0xffd0d0d0)); addAndMakeVisible (dawBtn); dawAtt = std::make_unique<BA> (st, "machineOn", dawBtn);
    dawBtn.setTooltip (String::fromUTF8 ("A linha da CLAVINET MACHINE toca junto com o play da DAW"));
    knob (bpm, "bpm", "BPM", false);
    for (int i = 0; i < 6; ++i)
        chip (octBtn[i], "C" + String (i - 1), [this, i] { proc.setParam ("center", (float) (12 * (i - 1 + 2) + 5)); proc.clearEdits(); });

    // INSTRUMENTO
    const char* fl[4] = { "BRILL.", "TREBLE", "MEDIUM", "SOFT" };
    const Colour fc[4] = { Colour (0xffb3261e), Colour (0xffe0a92b), Colour (0xffe9e2cf), Colour (0xff4a3628) };
    const char* fid[4] = { "fBrill", "fTreble", "fMedium", "fSoft" };
    for (int i = 0; i < 4; ++i) { filt[i] = std::make_unique<RockerButton> (fl[i], fc[i]); addAndMakeVisible (*filt[i]); filtAtt[i] = std::make_unique<BA> (st, fid[i], *filt[i]); }
    const char* pl[2] = { "C / D", "A / B" }; const char* pid[2] = { "pickCD", "pickAB" };
    for (int i = 0; i < 2; ++i) { pick[i] = std::make_unique<RockerButton> (pl[i], Colour (0xff2f2f2f)); addAndMakeVisible (*pick[i]); pickAtt[i] = std::make_unique<BA> (st, pid[i], *pick[i]); }
    for (int i = 0; i < 6; ++i) chip (timbreBtn[i], timbreName (i), [this, i] { proc.applyTimbre (i); });
    knob (bright, "bright", "BRILHO", false); knob (decay, "decay", "DECAY", false); knob (mute, "mute", String::fromUTF8 ("MUTE (L\xc3\x83)"), false); knob (vol, "vol", "VOLUME", false);
    knob (wahSens, "wahSens", "SENS."); knob (wahRange, "wahRange", "FAIXA");
    knob (phRate, "phRate", "RATE"); knob (phDepth, "phDepth", "DEPTH");
    knob (echoTime, "echoTime", "TEMPO"); knob (echoFb, "echoFb", "FEEDBACK"); knob (echoMix, "echoMix", "ECHO VOL");
    knob (reverb, "reverb", "REVERB"); knob (master, "master", "MASTER");
    for (auto [b, a, id, t] : { std::tuple { &wahBtn, &wahAtt, "wahOn", "WAH" }, std::tuple { &phBtn, &phAtt, "phOn", "PHASER" },
                                std::tuple { &echoBtn, &echoAtt, "echoOn", "ECHO" }, std::tuple { &maxBtn, &maxAtt, "maxOn", "MAX" } })
    {
        b->setButtonText (t); b->setClickingTogglesState (true); addAndMakeVisible (*b);
        *a = std::make_unique<BA> (st, id, *b);
    }
    keyboard.setKeyWidth (24.0f); keyboard.setScrollButtonsVisible (false);
    keyboard.setColour (MidiKeyboardComponent::whiteNoteColourId, Colour (0xfff2ead6));
    keyboard.setColour (MidiKeyboardComponent::keyDownOverlayColourId, Colour (0xffe8c27a));
    keyboard.setColour (MidiKeyboardComponent::mouseOverKeyOverlayColourId, Colour (0x33c2741f));
    keyboard.setOctaveForMiddleC (3);   // nomes do Cubase: C3 = dó central
    addAndMakeVisible (keyboard);

    // CLAVINET MACHINE
    chip (srcBtn[0], "ACORDES DO PLUGIN", [this] { proc.setParam ("src", 0.0f); }, true);
    chip (srcBtn[1], "MIDI DA TRILHA", [this] { proc.setParam ("src", 1.0f); }, true);
    srcBtn[1].setTooltip (String::fromUTF8 ("Segue os acordes do MIDI que chega nesta trilha (o skank do MRP-80, do MRM-0420 ou de outra trilha)"));
    bankBtn.setButtonText (String::fromUTF8 ("BANCO DE RIDDIMS \xe2\x96\xbe")); bankBtn.getProperties().set ("kind", "light");
    bankBtn.onClick = [this] { showBank(); }; addAndMakeVisible (bankBtn);
    for (int i = 0; i < 4; ++i)
    {
        const String n (i + 1);
        for (int r = 0; r < 12; ++r) rootBox[i].addItem (mrcl::rootName (r), r + 1);
        for (int d = 0; d < 6; ++d) durBox[i].addItem (mrcl::durName (d), d + 1);
        addAndMakeVisible (rootBox[i]); addAndMakeVisible (durBox[i]);
        rootAtt[i] = std::make_unique<CA> (st, "root" + n, rootBox[i]);
        durAtt[i] = std::make_unique<CA> (st, "dur" + n, durBox[i]);
        chip (majBtn[i], "MAIOR", [this, n] { proc.setParam ("minor" + n, 0.0f); });
        chip (minBtn[i], "menor", [this, n] { proc.setParam ("minor" + n, 1.0f); });
        if (i > 0)
        {
            onBtn[i].setButtonText ("LIGADO"); onBtn[i].setColour (ToggleButton::textColourId, col::amber);
            addAndMakeVisible (onBtn[i]); onAtt[i] = std::make_unique<BA> (st, "on" + n, onBtn[i]);
        }
    }
    for (int i = 0; i < 7; ++i)
        chip (musBtn[i], mrcl::musicians()[(size_t) i].name, [this, i]
        {
            proc.setParam ("musician", (float) i);
            proc.setParam ("swing", (float) mrcl::musicians()[(size_t) i].swing);
            proc.applyTimbre (mrcl::musicians()[(size_t) i].timbre + 1);   // +1: o M1 é o primeiro timbre
        }, true);
    for (int i = 0; i < 4; ++i) chip (fillBtn[i], mrcl::fillName (i), [this, i] { proc.setParam ("fill", (float) i); }, true);
    chip (everyBtn[0], "A CADA 4", [this] { proc.setParam ("fillEvery", 0.0f); }, true);
    chip (everyBtn[1], "A CADA 8", [this] { proc.setParam ("fillEvery", 1.0f); }, true);
    chip (notaBtn[0], "1/64", [this] { proc.setParam ("nota", 0.0f); }, true);
    chip (notaBtn[1], "1/32", [this] { proc.setParam ("nota", 1.0f); }, true);
    knob (center, "center", "REGISTRO", false); knob (density, "density", "DENSIDADE", false);
    knob (swing, "swing", "SWING", false); knob (velocity, "velocity", "VELOCITY", false);
    center.s.textFromValueFunction = [] (double v) { return ReggaeClavinetProcessor::noteName ((int) v); }; center.s.updateText();
    for (int i = 0; i < 3; ++i)
    {
        mini[i] = std::make_unique<LineRoll> (p); mini[i]->style = i; addAndMakeVisible (*mini[i]);
        chip (ouvirBtn[i], String::fromUTF8 ("\xe2\x96\xb6 OUVIR"), [this, i] { proc.setParam ("use", (float) i); proc.clearEdits(); proc.setParam ("preview", 1.0f); }, true);
        chip (usarBtn[i], String::fromUTF8 ("\xe2\x9c\x93 USAR"), [this, i] { proc.setParam ("use", (float) i); proc.clearEdits(); }, true);
    }
    chip (newBtn, String::fromUTF8 ("\xe2\x9f\xb3 GERAR NOVAS"), [this] { proc.newIdeas(); }); newBtn.getProperties().set ("kind", "big");
    chip (exportBtn, "EXPORTAR", [this] { exportMidi(); }); exportBtn.getProperties().set ("kind", "big");
    chip (undoBtn, String::fromUTF8 ("VOLTAR \xc3\x80 SUGEST\xc3\x83O"), [this] { proc.clearEdits(); }, true);
    repsBox.addItemList ({ "x1", "x2", "x4" }, 1); addAndMakeVisible (repsBox); repsAtt = std::make_unique<CA> (st, "reps", repsBox);
    addAndMakeVisible (roll); addAndMakeVisible (dragZone);

    setSize (1100, 820);
    setPage (1);
    startTimerHz (25);
}

ReggaeClavinetEditor::~ReggaeClavinetEditor() { stopTimer(); setLookAndFeel (nullptr); }

void ReggaeClavinetEditor::knob (Knob& k, const String& id, const String& label, bool rotary)
{
    k.label = label;
    k.s.setSliderStyle (rotary ? Slider::RotaryHorizontalVerticalDrag : Slider::LinearHorizontal);
    k.s.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    k.s.setTextBoxStyle (rotary ? Slider::TextBoxBelow : Slider::TextBoxRight, false, rotary ? 70 : 52, 16);
    k.s.setColour (Slider::textBoxTextColourId, rotary ? Colour (0xff9a9a9a) : (id == "bpm" ? Colour (0xffe8e8e8) : col::ink));
    k.s.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    k.s.setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    addAndMakeVisible (k.s);
    k.att = std::make_unique<SA> (proc.apvts, id, k.s);
    if (id != "center" && id != "bpm" && id != "echoTime" && id != "phRate")
    {
        k.s.textFromValueFunction = [] (double v) { return String (roundToInt (v * 100.0)) + "%"; };
        k.s.updateText();
    }
}

void ReggaeClavinetEditor::chip (TextButton& b, const String& text, std::function<void()> click, bool light)
{
    b.setButtonText (text);
    b.onClick = std::move (click);
    if (light) b.getProperties().set ("kind", "light");
    addAndMakeVisible (b);
}

std::vector<Component*> ReggaeClavinetEditor::pageComponents (int p)
{
    std::vector<Component*> v;
    if (p == 0)
    {
        for (auto& f : filt) v.push_back (f.get());
        for (auto& f : pick) v.push_back (f.get());
        for (auto& b : timbreBtn) v.push_back (&b);
        for (auto* k : { &bright, &decay, &mute, &vol, &wahSens, &wahRange, &phRate, &phDepth, &echoTime, &echoFb, &echoMix, &reverb, &master }) v.push_back (&k->s);
        for (auto* b : { &wahBtn, &phBtn, &echoBtn, &maxBtn }) v.push_back (b);
        v.push_back (&keyboard);
    }
    else
    {
        for (auto& b : srcBtn) v.push_back (&b);
        v.push_back (&bankBtn);
        for (int i = 0; i < 4; ++i) { v.push_back (&rootBox[i]); v.push_back (&durBox[i]); v.push_back (&majBtn[i]); v.push_back (&minBtn[i]); if (i > 0) v.push_back (&onBtn[i]); }
        for (auto& b : musBtn) v.push_back (&b);
        for (auto& b : fillBtn) v.push_back (&b);
        for (auto& b : everyBtn) v.push_back (&b);
        for (auto& b : notaBtn) v.push_back (&b);
        for (auto* k : { &center, &density, &swing, &velocity }) v.push_back (&k->s);
        for (int i = 0; i < 3; ++i) { v.push_back (mini[i].get()); v.push_back (&ouvirBtn[i]); v.push_back (&usarBtn[i]); }
        for (auto* c : std::initializer_list<Component*> { &newBtn, &exportBtn, &undoBtn, &repsBox, &roll, &dragZone }) v.push_back (c);
    }
    return v;
}

void ReggaeClavinetEditor::setPage (int p)
{
    page = jlimit (0, 1, p);
    for (int k = 0; k < 2; ++k) for (auto* c : pageComponents (k)) c->setVisible (k == page);
    tabInst.setToggleState (page == 0, dontSendNotification);
    tabMach.setToggleState (page == 1, dontSendNotification);
    repaint();
}

void ReggaeClavinetEditor::showBank()
{
    PopupMenu menu;
    riddimbank::addMenus (menu, proc.getBankEntry());
    Component::SafePointer<ReggaeClavinetEditor> safe (this);
    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&bankBtn), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        if (r == riddimbank::toggleCompleteId) { riddimbank::onlyComplete() = ! riddimbank::onlyComplete(); safe->showBank(); return; }
        if (const int e = riddimbank::entryFromMenuResult (r); e >= 0) safe->proc.applyBankEntry (e);
    });
}

void ReggaeClavinetEditor::exportMidi()
{
    auto dir = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Mastor Reggae Clavinet");
    dir.createDirectory();
    chooser = std::make_unique<FileChooser> ("Salvar a linha em MIDI", dir.getChildFile ("clavinet.mid"), "*.mid");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                          [this] (const FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == File()) return;
                              if (! f.hasFileExtension ("mid")) f = f.withFileExtension ("mid");
                              proc.writeMidiFile (f);
                          });
}

void ReggaeClavinetEditor::timerCallback()
{
    const int oct = (int) std::floor ((proc.get ("center") - 5) / 12.0) - 2;
    for (int i = 0; i < 6; ++i) octBtn[i].setToggleState (i - 1 == oct, dontSendNotification);
    const int tb = (int) proc.get ("timbre");
    for (int i = 0; i < 6; ++i) timbreBtn[i].setToggleState (i == tb, dontSendNotification);
    const int src = (int) proc.get ("src");
    for (int i = 0; i < 2; ++i) srcBtn[i].setToggleState (i == src, dontSendNotification);
    for (int i = 0; i < 4; ++i)
    {
        const bool mi = proc.get ("minor" + String (i + 1)) > 0.5f;
        majBtn[i].setToggleState (! mi, dontSendNotification); minBtn[i].setToggleState (mi, dontSendNotification);
        const bool on = i == 0 || proc.get ("on" + String (i + 1)) > 0.5f;
        for (Component* c : { (Component*) &rootBox[i], (Component*) &durBox[i], (Component*) &majBtn[i], (Component*) &minBtn[i] }) c->setAlpha (on ? 1.0f : 0.4f);
    }
    const int mu = (int) proc.get ("musician"), fi = (int) proc.get ("fill"), use = (int) proc.get ("use");
    for (int i = 0; i < 7; ++i) musBtn[i].setToggleState (i == mu, dontSendNotification);
    for (int i = 0; i < 4; ++i) fillBtn[i].setToggleState (i == fi, dontSendNotification);
    everyBtn[0].setToggleState (proc.get ("fillEvery") < 0.5f, dontSendNotification); everyBtn[1].setToggleState (proc.get ("fillEvery") > 0.5f, dontSendNotification);
    notaBtn[0].setToggleState (proc.get ("nota") < 0.5f, dontSendNotification); notaBtn[1].setToggleState (proc.get ("nota") > 0.5f, dontSendNotification);
    for (int i = 0; i < 3; ++i)
    {
        usarBtn[i].setToggleState (i == use, dontSendNotification);
        usarBtn[i].setButtonText (i == use ? String::fromUTF8 ("\xe2\x97\x8f EM USO") : String::fromUTF8 ("\xe2\x9c\x93 USAR"));
    }
    previewBtn.setButtonText (previewBtn.getToggleState() ? String::fromUTF8 ("\xe2\x96\xa0 PARAR") : String::fromUTF8 ("\xe2\x96\xb6 PREVIEW"));
    undoBtn.setEnabled (proc.hasEdits());
    const int lo = 12 * (oct + 2);
    if (keyboard.getLowestVisibleKey() != lo) { keyboard.setAvailableRange (std::max (0, lo), std::min (127, lo + 36)); keyboard.setLowestVisibleKey (lo); }
    repaint();
}

// ================================================================== desenho
void ReggaeClavinetEditor::paint (Graphics& g)
{
    g.fillAll (col::bg);
    // cabeçalho
    g.setColour (Colour (0xff8a8a8a)); g.setFont (sans (14.0f).italicised()); g.drawText ("DubMastor", 20, 8, 200, 18, Justification::left);
    g.setColour (Colour (0xfff0f0f0)); g.setFont (sans (27.0f)); g.drawText ("REGGAE CLAVINET", 20, 24, 300, 32, Justification::left);
    g.setColour (Colour (0xff777777)); g.setFont (mono (10.0f, false));
    g.drawText (String::fromUTF8 ("CLAVINET SYNTHESIS  \xc2\xb7  MRCL-71  \xc2\xb7  v") + JucePlugin_VersionString, 20, 56, 300, 14, Justification::left);
    {   // placa dourada DUB STATION
        auto pl = Rectangle<float> (650, 12, 200, 46);
        ColourGradient gr (Colour (0xfff3d27a), pl.getX(), pl.getY(), Colour (0xffc99a33), pl.getX(), pl.getBottom(), false);
        g.setGradientFill (gr); g.fillRoundedRectangle (pl, 9.0f);
        g.setColour (Colour (0xff8a6516)); g.drawRoundedRectangle (pl, 9.0f, 2.0f);
        g.setColour (Colour (0xff3b2a08)); g.setFont (sans (19.0f, true).withExtraKerningFactor (0.15f)); g.drawText ("DUB STATION", pl, Justification::centred);
    }
    g.setColour (Colour (0xff3a3a3a)); g.fillRect (0, 112, getWidth(), 3);
    g.setColour (Colour (0xffd8c9a3)); g.setFont (mono (10.0f)); g.drawText ("OITAVA", 444, 76, 56, 30, Justification::centredLeft);
    g.drawText ("BPM", 160, 76, 34, 30, Justification::centredLeft);
    {   // status
        String stt = proc.playBeat.load() >= 0 ? "TOCANDO" : "PARADO";
        if (proc.get ("src") > 0.5f && proc.get ("machineOn") > 0.5f)
            stt << "  \xc2\xb7  MIDI: " << (proc.midiChordRoot.load() >= 0 ? String (mrcl::rootName (proc.midiChordRoot.load())) + (proc.midiChordMinor.load() ? "m" : "") : String ("aguardando acordes"));
        g.setColour (Colour (0xff9fe07f)); g.setFont (mono (11.0f, false)); g.drawFittedText (String::fromUTF8 (stt.toRawUTF8()), 870, 74, 214, 30, Justification::centredLeft, 2);
    }

    if (page == 0)
    {
        // tampa do clavinet
        auto lid = Rectangle<float> (16, 124, 1068, 318);
        g.setGradientFill (ColourGradient (Colour (0xff2a1d14), 0, lid.getY(), Colour (0xff140d08), 0, lid.getBottom(), false));
        g.fillRoundedRectangle (lid, 10.0f);
        auto top = Rectangle<float> (30, 136, 1040, 178), fad = Rectangle<float> (30, 324, 1040, 106);
        for (auto r : { top, fad })
        {
            g.setGradientFill (ColourGradient (col::cream, 0, r.getY(), col::cream2, 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, 6.0f); g.setColour (Colour (0xff6b5a3c)); g.drawRoundedRectangle (r, 6.0f, 2.0f);
        }
        auto badge = Rectangle<float> (50, 190, 140, 42);
        g.setGradientFill (ColourGradient (Colour (0xffcfcfcf), 0, badge.getY(), Colour (0xff8d8d8d), 0, badge.getBottom(), false));
        g.fillRoundedRectangle (badge, 4.0f); g.setColour (Colour (0xff4a4a4a)); g.drawRoundedRectangle (badge, 4.0f, 2.0f);
        g.setColour (Colour (0xff222222)); g.setFont (sans (18.0f, true).withExtraKerningFactor (0.2f)); g.drawText ("MASTOR", badge, Justification::centred);
        g.setColour (Colour (0xff4a3f2e)); g.setFont (sans (11.0f).withExtraKerningFactor (0.3f)); g.drawText (String::fromUTF8 ("CLAVINET \xc2\xb7 D-71"), 40, 238, 160, 16, Justification::centred);
        g.setColour (Colour (0xff5a4d38)); g.setFont (mono (10.0f)); g.drawText ("FILTRO", 214, 158, 100, 14, Justification::left); g.drawText ("CAPTADOR", 446, 158, 100, 14, Justification::left);
        // LCD
        auto lcd = Rectangle<float> (580, 150, 476, 80);
        g.setColour (Colour (0xff0c0c0c)); g.fillRoundedRectangle (lcd, 6.0f);
        g.setColour (col::lcd); g.fillRoundedRectangle (lcd.reduced (7.0f), 3.0f);
        g.setColour (Colour (0xff13210c)); g.setFont (mono (19.0f));
        const int tb = (int) proc.get ("timbre");
        g.drawText ("P0" + String (tb + 1) + " Clav: " + timbreLcdName (tb), lcd.reduced (18, 10).withTrimmedBottom (26), Justification::centredLeft);
        g.setFont (mono (13.0f, false));
        g.drawText ("BRI " + String (roundToInt (proc.get ("bright") * 100)) + "  MUTE " + String (roundToInt (proc.get ("mute") * 100))
                    + "  WAH " + (proc.get ("wahOn") > 0.5f ? "ON" : "OFF") + "  VOL " + String (roundToInt (proc.get ("vol") * 100)),
                    lcd.reduced (18, 10).withTrimmedTop (32), Justification::centredLeft);
        g.setColour (col::ink); g.setFont (mono (11.0f));
        const Knob* fk[4] = { &bright, &decay, &mute, &vol };
        for (int i = 0; i < 4; ++i) g.drawText (fk[i]->label, fk[i]->s.getBounds().translated (0, -20).withHeight (16), Justification::centredLeft);
        // módulos de efeito
        const char* tt[4] = { "AUTO-WAH", "PHASER", "TAPE ECHO", "MAXIMIZER" };
        const char* sub[4] = { "ENVELOPE", "4 EST\xc3\x81GIOS", "MTE-12 \xc2\xb7 COME\xc3\x87" "A DESLIGADO", "" };
        for (int i = 0; i < 4; ++i)
        {
            auto r = Rectangle<float> (16.0f + i * 270.0f, 456, 258, 176);
            g.setColour (col::panel); g.fillRoundedRectangle (r, 8.0f); g.setColour (col::line); g.drawRoundedRectangle (r, 8.0f, 1.0f);
            g.setColour (Colour (0xffeeeeee)); g.setFont (mono (12.5f)); g.drawText (tt[i], r.reduced (12, 8).withHeight (18), Justification::centredLeft);
            g.setColour (Colour (0xff777777)); g.setFont (mono (9.0f, false)); g.drawText (String::fromUTF8 (sub[i]), r.reduced (12, 8).withTrimmedTop (18).withHeight (14), Justification::centredLeft);
            g.setColour (col::line); g.fillRect (r.getX() + 12, r.getY() + 44, r.getWidth() - 24, 1.0f);
        }
        g.setColour (Colour (0xffd0d0d0)); g.setFont (mono (10.5f));
        for (auto* k : { &wahSens, &wahRange, &phRate, &phDepth, &echoTime, &echoFb, &echoMix, &reverb, &master })
            g.drawText (k->label, k->s.getBounds().translated (0, -16).withHeight (14), Justification::centred);
        // teclado: faixa de feltro
        g.setColour (Colour (0xff1a120c)); g.fillRoundedRectangle (16, 646, 1068, 164, 8.0f);
        g.setColour (Colour (0xff8a2a1e)); g.fillRoundedRectangle (26, 652, 1048, 6, 3.0f);
        return;
    }

    // ---------------- CLAVINET MACHINE
    auto mach = Rectangle<float> (16, 124, 1068, 686);
    g.setColour (Colour (0xff5a3a1f)); g.fillRoundedRectangle (mach, 8.0f);
    auto in = mach.reduced (5.0f);
    g.setGradientFill (ColourGradient (Colour (0xffe8dcc0), 0, in.getY(), Colour (0xffd4c4a0), 0, in.getBottom(), false));
    g.fillRoundedRectangle (in, 5.0f);
    auto band = in.withHeight (44.0f);
    g.setGradientFill (ColourGradient (Colour (0xffd7aa45), 0, band.getY(), Colour (0xffc4922d), 0, band.getBottom(), false));
    g.fillRect (band);
    g.setColour (Colour (0xfffff8e6)); g.setFont (sans (19.0f).withExtraKerningFactor (0.25f));
    g.drawText ("CLAVINET MACHINE  -  MASTOR REGGAE CLAVINET " + String (JucePlugin_VersionString).upToLastOccurrenceOf (".", false, false), band.reduced (16, 0), Justification::centredLeft);
    g.setFont (mono (12.0f)); g.setColour (Colour (0xff3b2a08));
    g.drawText ("MRCL-71", band.reduced (16, 0), Justification::centredRight);

    auto section = [&] (Rectangle<int> r, const String& t)
    {
        g.setColour (Colours::white.withAlpha (0.18f)); g.fillRoundedRectangle (r.toFloat(), 6.0f);
        g.setColour (Colour (0xffb9a98a)); g.drawRoundedRectangle (r.toFloat(), 6.0f, 1.0f);
        g.setFont (mono (11.0f).withExtraKerningFactor (0.2f));
        const float tw = GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 20.0f;
        auto tag = Rectangle<float> ((float) r.getX() + 12, (float) r.getY() - 9, tw, 18);
        g.setColour (col::ink); g.fillRoundedRectangle (tag, 3.0f);
        g.setColour (Colour (0xfff2ead6)); g.drawText (String::fromUTF8 (t.toRawUTF8()), tag, Justification::centred);
    };
    section (escuta, "ESCUTA"); section (musico, String::fromUTF8 ("M\xc3\x9aSICO")); section (sugest, String::fromUTF8 ("SUGEST\xc3\x95" "ES DE LINHA")); section (midiSec, String::fromUTF8 ("MIDI \xc2\xb7 EDIT\xc3\x81VEL"));

    // acordes
    const auto cs = proc.getChords();
    for (int i = 0; i < 4; ++i)
    {
        auto r = cardArea[i].toFloat();
        const bool on = i == 0 || proc.get ("on" + String (i + 1)) > 0.5f;
        g.setColour (Colour (0xff1c140c).withAlpha (on ? 1.0f : 0.45f)); g.fillRoundedRectangle (r, 4.0f);
        g.setColour (col::chord[i]); g.fillRect (r.withHeight (3.0f));
        g.setColour (col::amber.withAlpha (on ? 1.0f : 0.4f)); g.setFont (sans (16.0f).withExtraKerningFactor (0.08f));
        g.drawText (String (mrcl::rootName ((int) proc.get ("root" + String (i + 1)))) + (proc.get ("minor" + String (i + 1)) > 0.5f ? " MENOR" : " MAIOR"),
                    r.withHeight (28).translated (0, 4), Justification::centred);
    }
    // contagem: caixa no 2 e no 4
    const double pb = proc.playBeat.load();
    const int cur = pb >= 0 ? (int) std::floor (pb) % 4 : -1;
    g.setFont (mono (10.0f)); g.setColour (col::ink2); g.drawText ("CONTAGEM", beatsArea.withWidth (90), Justification::centredLeft);
    for (int b = 0; b < 4; ++b)
    {
        auto c = Rectangle<float> ((float) beatsArea.getX() + 92 + b * 42.0f, (float) beatsArea.getY() + 1, 32, 32);
        g.setColour (b % 2 ? col::red : Colour (0xffefe6cf)); g.fillEllipse (c);
        g.setColour (Colour (0xff8a7a5c)); g.drawEllipse (c, 1.0f);
        if (b == cur) { g.setColour (col::gold); g.drawEllipse (c.expanded (2.0f), 3.0f); }
        g.setColour (b % 2 ? Colours::white : col::ink); g.setFont (mono (14.0f)); g.drawText (String (b + 1), c, Justification::centred);
    }
    g.setColour (col::ink2); g.setFont (mono (10.5f, false));
    g.drawText (String::fromUTF8 ("caixa no 2 e no 4 \xc2\xb7 o clavinet responde ao skank do piano, n\xc3\xa3o dobra"),
                beatsArea.withTrimmedLeft (270), Justification::centredLeft);
    // músico
    const auto& m = mrcl::musicians()[(size_t) jlimit (0, 6, (int) proc.get ("musician"))];
    g.setColour (col::ink); g.setFont (sans (12.5f, true)); g.drawText (String::fromUTF8 (m.who), whoArea.withHeight (16), Justification::topLeft);
    g.setColour (col::ink2); g.setFont (sans (12.0f)); g.drawFittedText (String::fromUTF8 (m.desc), whoArea.withTrimmedTop (18), Justification::topLeft, 3);
    g.setFont (mono (10.0f)); g.setColour (col::ink2);
    g.drawText ("FILL (FIM DA FRASE)", fillBtn[0].getBounds().translated (0, -16).withWidth (200).withHeight (14), Justification::left);
    g.setColour (col::ink); g.setFont (mono (11.0f));
    for (auto* k : { &center, &density, &swing, &velocity }) g.drawText (k->label, k->s.getBounds().withX (musico.getX() + 12).withWidth (90), Justification::centredLeft);
    // sugestões
    for (int i = 0; i < 3; ++i)
    {
        auto r = mini[i]->getBounds();
        const bool on = i == (int) proc.get ("use");
        auto row = Rectangle<int> (sugest.getX() + 10, r.getY() - 24, sugest.getWidth() - 20, r.getHeight() + 30).toFloat();
        if (on) { g.setColour (col::clay.withAlpha (0.14f)); g.fillRoundedRectangle (row, 5.0f); g.setColour (col::clay); g.drawRoundedRectangle (row, 5.0f, 1.0f); }
        auto L = Rectangle<float> (row.getX() + 8, r.getY() + 4.0f, 36, 36);
        g.setColour (col::ink); g.fillRoundedRectangle (L, 4.0f);
        g.setColour (col::gold); g.setFont (sans (20.0f, true)); g.drawText (String::charToString ((juce_wchar) ('A' + i)), L, Justification::centred);
        g.setColour (col::ink); g.setFont (sans (14.0f, true)); g.drawText (mrcl::styleName (i), (int) row.getX() + 56, r.getY() - 22, 200, 16, Justification::left);
        g.setColour (col::ink2); g.setFont (mono (9.5f, false)); g.drawText (String::fromUTF8 (mrcl::styleDesc (i)), (int) row.getX() + 230, r.getY() - 21, 300, 14, Justification::left);
    }
    g.setColour (col::ink2); g.setFont (mono (10.0f, false));
    g.drawText (String::fromUTF8 ("Toque numa nota para apagar \xc2\xb7 toque no vazio para p\xc3\xb4r uma nota do acorde (grade de 1/16). No MIDI, acorde 1..4 = canal 1..4."),
                roll.getBounds().translated (0, -18).withHeight (14), Justification::centredLeft);
}

void ReggaeClavinetEditor::resized()
{
    // cabeçalho
    tabInst.setBounds (330, 18, 150, 34); tabMach.setBounds (488, 18, 150, 34);
    dawBtn.setBounds (870, 20, 210, 30);
    previewBtn.setBounds (20, 76, 130, 30);
    bpm.s.setBounds (194, 76, 240, 30);
    for (int i = 0; i < 6; ++i) octBtn[i].setBounds (500 + i * 60, 76, 54, 30);

    // INSTRUMENTO
    for (int i = 0; i < 4; ++i) filt[i]->setBounds (210 + i * 52, 176, 48, 78);
    for (int i = 0; i < 2; ++i) pick[i]->setBounds (442 + i * 52, 176, 48, 78);
    for (int i = 0; i < 6; ++i) timbreBtn[i].setBounds (580 + i * 80, 246, 74, 32);
    Knob* fk[4] = { &bright, &decay, &mute, &vol };
    for (int i = 0; i < 4; ++i) fk[i]->s.setBounds (48 + i * 256, 376, 236, 32);
    auto fxKnobs = [&] (int mod, std::initializer_list<Knob*> ks)
    {
        int x = 16 + mod * 270 + 14;
        const int w = (258 - 28) / 3;
        for (auto* k : ks) { k->s.setBounds (x, 524, w, 96); x += w; }
    };
    fxKnobs (0, { &wahSens, &wahRange }); fxKnobs (1, { &phRate, &phDepth }); fxKnobs (2, { &echoTime, &echoFb, &echoMix }); fxKnobs (3, { &reverb, &master });
    wahBtn.setBounds (16 + 258 - 84, 464, 72, 26); phBtn.setBounds (286 + 258 - 84, 464, 72, 26);
    echoBtn.setBounds (556 + 258 - 84, 464, 72, 26); maxBtn.setBounds (826 + 258 - 84, 464, 72, 26);
    keyboard.setBounds (26, 662, 1048, 140);
    keyboard.setKeyWidth (1048.0f / 22.0f);   // 3 oitavas (22 teclas brancas) a partir do dó da chave OITAVA

    // CLAVINET MACHINE
    escuta = { 34, 184, 1032, 190 };
    srcBtn[0].setBounds (escuta.getX() + 14, escuta.getY() + 16, 170, 28);
    srcBtn[1].setBounds (escuta.getX() + 190, escuta.getY() + 16, 150, 28);
    bankBtn.setBounds (escuta.getX() + 350, escuta.getY() + 16, escuta.getWidth() - 364, 28);
    for (int i = 0; i < 4; ++i)
    {
        cardArea[i] = { escuta.getX() + 14 + i * 253, escuta.getY() + 52, 245, 92 };
        auto r = cardArea[i].reduced (8, 0);
        rootBox[i].setBounds (r.getX(), r.getY() + 32, 70, 24);
        majBtn[i].setBounds (r.getX() + 76, r.getY() + 32, 70, 24);
        minBtn[i].setBounds (r.getX() + 150, r.getY() + 32, 79, 24);
        durBox[i].setBounds (r.getX() + (i > 0 ? 0 : 0), r.getY() + 62, i > 0 ? 110 : 229, 24);
        if (i > 0) onBtn[i].setBounds (r.getX() + 120, r.getY() + 62, 109, 24);
    }
    beatsArea = { escuta.getX() + 14, escuta.getY() + 150, escuta.getWidth() - 28, 34 };

    musico = { 34, 392, 320, 404 };
    for (int i = 0; i < 7; ++i) musBtn[i].setBounds (musico.getX() + 12 + (i % 2) * 150, musico.getY() + 16 + (i / 2) * 34, 144, 30);
    whoArea = { musico.getX() + 12, musico.getY() + 156, musico.getWidth() - 24, 60 };
    for (int i = 0; i < 4; ++i) fillBtn[i].setBounds (musico.getX() + 12 + i * 74, musico.getY() + 236, 70, 26);
    for (int i = 0; i < 2; ++i) everyBtn[i].setBounds (musico.getX() + 12 + i * 98, musico.getY() + 268, 92, 26);
    for (int i = 0; i < 2; ++i) notaBtn[i].setBounds (musico.getX() + 216 + i * 48, musico.getY() + 268, 44, 26);
    int y = musico.getY() + 304;
    for (auto* k : { &center, &density, &swing, &velocity }) { k->s.setBounds (musico.getX() + 104, y, musico.getWidth() - 116, 24); y += 24; }

    sugest = { 366, 392, 700, 222 };
    for (int i = 0; i < 3; ++i)
    {
        const int ry = sugest.getY() + 34 + i * 62;
        mini[i]->setBounds (sugest.getX() + 66, ry, 440, 36);
        ouvirBtn[i].setBounds (sugest.getX() + 516, ry + 4, 82, 28);
        usarBtn[i].setBounds (sugest.getX() + 604, ry + 4, 82, 28);
    }
    newBtn.setBounds (sugest.getRight() - 196, sugest.getY() - 2, 186, 0);   // (fica no rodapé da seção MIDI)

    midiSec = { 366, 628, 700, 168 };
    roll.setBounds (midiSec.getX() + 12, midiSec.getY() + 34, midiSec.getWidth() - 24, 86);
    const int by = midiSec.getBottom() - 40;
    dragZone.setBounds (midiSec.getX() + 12, by, 190, 34);
    exportBtn.setBounds (midiSec.getX() + 208, by, 110, 34);
    newBtn.setBounds (midiSec.getX() + 324, by, 150, 34);
    repsBox.setBounds (midiSec.getX() + 480, by + 5, 60, 24);
    undoBtn.setBounds (midiSec.getX() + 546, by, 142, 34);
}
