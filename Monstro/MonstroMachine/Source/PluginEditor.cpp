#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

namespace
{
String u8 (const char* s) { return String::fromUTF8 (s); }
const char* kLeft = "\xe2\x97\x80";   // ◀
const char* kRight = "\xe2\x96\xb6";  // ▶
Font lcdFont (float h) { return Font (FontOptions (Font::getDefaultMonospacedFontName(), h, Font::bold)); }
Font uiFont (float h, bool bold = true) { return Font (FontOptions (h, bold ? Font::bold : Font::plain)); }
String two (int n) { return String (n).paddedLeft ('0', 2); }
String bar (float v, int n = 10) { const int k = jlimit (0, n, roundToInt (v * n)); return String::repeatedString (u8 ("\xe2\x96\x88"), k) + String::repeatedString (u8 ("\xe2\x96\x91"), n - k); }
String styleUpper (const String& key) { return String (mm::styleName (key.toStdString())).toUpperCase(); }
}

// ---------------------------------------------------------------- LCD
void LcdScreen::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (mmui::lcd, r.getWidth() * 0.4f, r.getHeight() * 0.3f, mmui::lcd2, r.getRight(), r.getBottom(), true));
    g.fillRoundedRectangle (r, 5);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (r.reduced (0.5f), 5, 1.5f);
    for (int i = 0; i < (int) items.size(); ++i)
    {
        const auto& it = items[(size_t) i];
        if (it.inv) { g.setColour (mmui::ink); g.fillRect (it.r); }
        else if (i == hover && it.action.isNotEmpty()) { g.setColour (mmui::ink.withAlpha (0.13f)); g.fillRect (it.r); }
        if (it.box) { g.setColour (mmui::ink.withAlpha (it.inv ? 1.0f : 0.55f)); g.drawRect (it.r, 1.0f); }
        g.setColour (it.inv ? mmui::lcd : mmui::ink);
        g.setFont (lcdFont (it.size));
        g.drawFittedText (it.text, it.r.reduced (4, 0).toNearestInt(), it.just, 1, 0.7f);
    }
}

void LcdScreen::mouseDown (const MouseEvent& e)
{
    for (const auto& it : items)
        if (it.action.isNotEmpty() && it.r.contains (e.position)) { ed.act (it.action); return; }
}

void LcdScreen::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < (int) items.size(); ++i) if (items[(size_t) i].action.isNotEmpty() && items[(size_t) i].r.contains (e.position)) h = i;
    if (h != hover) { hover = h; setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); }
}

// ---------------------------------------------------------------- pads e passos
void PadButton::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    g.setColour (Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (r.translated (0, 2), 8);
    g.setGradientFill (ColourGradient (mmui::padHi, r.getX(), r.getY(), mmui::pad, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 8);
    if (glow > 0.01f) { g.setColour (mmui::red.withAlpha (glow * 0.85f)); g.fillRoundedRectangle (r, 8); }
    g.setColour (current ? mmui::amber : Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (r.reduced (1), 8, current ? 2.5f : 1.0f);
    g.setColour (mmui::silk.withAlpha (muted ? 0.35f : 0.9f));
    g.setFont (uiFont (13));
    g.drawText ("PAD " + String (index + 1), r.reduced (8, 6).removeFromTop (16), Justification::topLeft);
    g.setFont (uiFont (12, false));
    g.drawFittedText (label, r.reduced (6, 8).toNearestInt(), Justification::centredBottom, 2, 0.8f);
    if (user) { g.setColour (mmui::amber); g.fillEllipse (r.getRight() - 14, r.getY() + 8, 6, 6); }
    if (muted) { g.setColour (mmui::red); g.setFont (uiFont (11)); g.drawText ("MUTE", r.reduced (8, 6).removeFromTop (16), Justification::topRight); }
}

void PadButton::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { ed.proc.uiPad = index; ed.act ("mute:" + String (index)); return; }
    ed.padPressed (index);
}

void StepButton::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2);
    g.setColour (value == 2 ? mmui::red : value == 1 ? mmui::amber : mmui::body2);
    g.fillRoundedRectangle (r, 4);
    g.setColour (now ? Colours::white : Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (r, 4, now ? 2.5f : 1.0f);
    g.setColour (value ? Colours::black : mmui::silk.withAlpha (step % 4 == 0 ? 0.9f : 0.5f));
    g.setFont (uiFont (12));
    g.drawText (String (step + 1), r, Justification::centred);
}

void StepButton::mouseDown (const MouseEvent& e)
{
    downAt = Time::getMillisecondCounter();
    longDone = false;
    if (e.mods.isPopupMenu() || e.mods.isShiftDown()) { longDone = true; ed.stepClicked (step, true); }
}

void StepButton::mouseUp (const MouseEvent&)
{
    if (longDone) return;
    ed.stepClicked (step, Time::getMillisecondCounter() - downAt > 420);   // segurar = acento
}

void MidiDrag::mouseDrag (const MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    const auto f = proc.tempMidi (allSeqs ? -1 : proc.seq());
    if (! proc.writeMidiFile (f, allSeqs ? -1 : proc.seq())) return;
    dragging = true;
    DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; });
}

// ---------------------------------------------------------------- editor
MonstroEditor::MonstroEditor (MonstroProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    logoAcademia = ImageCache::getFromMemory (BinaryData::academia_png, BinaryData::academia_pngSize);
    logoMonstro = ImageCache::getFromMemory (BinaryData::monstro_jpg, BinaryData::monstro_jpgSize);

    addAndMakeVisible (screen);
    for (int i = 0; i < 6; ++i)
    {
        auto* b = fkeys.add (new TextButton());
        addAndMakeVisible (b);
        b->onClick = [this, i] { if (i < (int) fkeyActions.size()) act (fkeyActions[(size_t) i]); };
    }
    const char* modeNames[6] { "MAIN", "PROGRAM", "TIMBRE", "MIXER", "EFFECTS", "SEQUENCE" };
    const char* modeKeys[6] { "main", "kit", "timbre", "mixer", "fx", "seq" };
    for (int i = 0; i < 6; ++i)
    {
        auto* b = modes.add (new TextButton (modeNames[i]));
        addAndMakeVisible (b);
        const String key (modeKeys[i]);
        b->onClick = [this, key] { act ("go:" + key); };
    }
    for (int i = 0; i < mm::numSeqs; ++i)
    {
        auto* b = seqBtns.add (new TextButton ("SEQ " + String::charToString ((juce_wchar) ('A' + i))));
        addAndMakeVisible (b);
        b->onClick = [this, i] { act ("pat:" + String (i)); };
    }
    // pads na ordem da MPC: 13–16 em cima, 1–4 embaixo
    for (int i = 0; i < mm::numPads; ++i) addAndMakeVisible (pads.add (new PadButton (*this, i)));
    for (int s = 0; s < mm::numSteps; ++s) addAndMakeVisible (steps.add (new StepButton (*this, s)));

    addBtn (dPrev, "dprev"); addBtn (dNext, "dnext"); addBtn (undoB, "undo"); addBtn (tapB, "tap");
    addBtn (folderB, "folder"); addBtn (sampleB, "sample"); addBtn (factoryB, "factory");
    addBtn (recB, "rec"); addBtn (playB, "play"); addBtn (stopB, "stop"); addBtn (dawB, "daw");
    factoryB.setButtonText (u8 ("KIT DE F\xc3\x81" "BRICA"));
    dPrev.setButtonText (u8 ("DATA \xe2\x97\x80")); dNext.setButtonText (u8 ("DATA \xe2\x96\xb6"));
    addAndMakeVisible (midiSeq); addAndMakeVisible (midiAll);
    midiSeq.onClick = midiAll.onClick = [this] { toast ("Arraste o botao MIDI para a trilha da DAW"); };
    midiSeq.setTooltip (u8 ("Arraste para a DAW: o SEQ que está tocando")); midiAll.setTooltip ("Arraste para a DAW: SEQ A, B, C e D (4 compassos)");

    auto setupSlider = [this] (Slider& s, Label& l, const String& name, const String& id, std::unique_ptr<SliderParameterAttachment>& a)
    {
        s.setSliderStyle (Slider::LinearHorizontal);
        s.setTextBoxStyle (Slider::TextBoxRight, false, 44, 20);
        s.setColour (Slider::thumbColourId, mmui::amber);
        s.setColour (Slider::trackColourId, mmui::red.withAlpha (0.7f));
        s.setColour (Slider::backgroundColourId, mmui::body2);
        s.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        addAndMakeVisible (s);
        l.setText (name, dontSendNotification);
        l.setFont (uiFont (12)); l.setColour (Label::textColourId, mmui::silk);
        addAndMakeVisible (l);
        a = std::make_unique<SliderParameterAttachment> (*proc.apvts.getParameter (id), s);
    };
    setupSlider (bpmS, bpmL, "BPM", "bpm", bpmA);
    setupSlider (swingS, swingL, "SWING", "swing", swingA);
    setupSlider (masterS, masterL, "MASTER", "master", masterA);
    masterS.textFromValueFunction = [] (double v) { return String (roundToInt (v * 100)) + "%"; };
    masterS.updateText();

    stepLbl.setFont (uiFont (13)); stepLbl.setColour (Label::textColourId, mmui::silk);
    addAndMakeVisible (stepLbl);
    toastL.setFont (uiFont (14)); toastL.setJustificationType (Justification::centred);
    toastL.setColour (Label::backgroundColourId, mmui::ink.withAlpha (0.92f)); toastL.setColour (Label::textColourId, mmui::lcd);
    addChildComponent (toastL);

    setSize (1060, 716);
    buildScreen();
    refresh();
    startTimerHz (30);
}

MonstroEditor::~MonstroEditor() { stopTimer(); }

Button& MonstroEditor::addBtn (TextButton& b, const String& action)
{
    addAndMakeVisible (b);
    b.onClick = [this, action] { act (action); };
    return b;
}

void MonstroEditor::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff46484b), 0, 0, mmui::body2, 0, r.getBottom(), false));
    g.fillAll();
    g.setColour (Colour (0xff6a6c70)); g.drawHorizontalLine (1, 0, r.getRight());
    // cabeçalho: Academia de Beats, nome e o Monstro.zip
    if (logoAcademia.isValid())
        g.drawImageWithin (logoAcademia, 20, 10, 210, 60, RectanglePlacement::xLeft | RectanglePlacement::yMid);
    if (logoMonstro.isValid())
    {
        Path clip; clip.addRoundedRectangle (getWidth() - 84.0f, 8.0f, 64.0f, 64.0f, 10.0f);
        Graphics::ScopedSaveState s (g); g.reduceClipRegion (clip);
        g.drawImageWithin (logoMonstro, getWidth() - 84, 8, 64, 64, RectanglePlacement::stretchToFit);
    }
    g.setColour (mmui::silk);
    g.setFont (uiFont (30));
    g.drawText ("MONSTRO MACHINE", 250, 12, 520, 34, Justification::centredLeft);
    g.setFont (uiFont (14, false));
    g.setColour (mmui::silk.withAlpha (0.7f));
    g.drawText ("MM-2500  " + String (JucePlugin_VersionString).upToLastOccurrenceOf (".", false, false)
                + u8 ("  ·  timbres do Monstro.zip · Academia de Beats"), 252, 46, 600, 20, Justification::centredLeft);
    // LED do nível de saída
    const float lv = jlimit (0.0f, 1.0f, proc.outLevel.load());
    g.setColour (mmui::red.withAlpha (0.25f + 0.75f * lv)); g.fillEllipse ((float) getWidth() - 112, 34, 12, 12);
    // moldura da seção da direita e silkscreen
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (592, 82, 456, 452, 10);
    g.setColour (mmui::silk.withAlpha (0.55f)); g.setFont (uiFont (11));
    g.drawText ("SOFT KEYS", 20, 372, 120, 14, Justification::left);
    g.drawText ("MODE", 20, 430, 120, 14, Justification::left);
}

void MonstroEditor::resized()
{
    screen.setBounds (20, 88, 560, 280);
    const int fw = (560 - 5 * 6) / 6;
    for (int i = 0; i < 6; ++i) fkeys[i]->setBounds (20 + i * (fw + 6), 386, fw, 40);
    for (int i = 0; i < 6; ++i) modes[i]->setBounds (20 + i * (fw + 6), 444, fw, 32);
    dPrev.setBounds (20, 488, 130, 44); dNext.setBounds (156, 488, 130, 44); undoB.setBounds (300, 488, 132, 44); tapB.setBounds (440, 488, 140, 44);
    folderB.setBounds (20, 542, 184, 36); sampleB.setBounds (208, 542, 184, 36); factoryB.setBounds (396, 542, 184, 36);
    recB.setBounds (20, 588, 90, 40); playB.setBounds (116, 588, 120, 40); stopB.setBounds (242, 588, 110, 40); dawB.setBounds (358, 588, 222, 40);

    const int ps = 104, gap = 8, px = 600, py = 90;
    for (int i = 0; i < mm::numPads; ++i)
    {
        const int row = 3 - i / 4, col = i % 4;
        pads[i]->setBounds (px + col * (ps + gap), py + row * (ps + gap), ps, ps);
    }
    for (int i = 0; i < mm::numSeqs; ++i) seqBtns[i]->setBounds (600 + i * 66, 544, 60, 34);
    midiSeq.setBounds (868, 544, 86, 34); midiAll.setBounds (958, 544, 86, 34);
    bpmL.setBounds (600, 588, 50, 18); bpmS.setBounds (596, 604, 150, 24);
    swingL.setBounds (752, 588, 60, 18); swingS.setBounds (748, 604, 146, 24);
    masterL.setBounds (900, 588, 70, 18); masterS.setBounds (896, 604, 150, 24);

    stepLbl.setBounds (20, 636, 900, 18);
    const int sw = (1020 - 15 * 4) / 16;
    for (int s = 0; s < mm::numSteps; ++s) steps[s]->setBounds (20 + s * (sw + 4), 656, sw, 48);
    toastL.setBounds (60, 330, 480, 30);
}

// ---------------------------------------------------------------- telas
void MonstroEditor::buildScreen()
{
    auto& it = screen.items;
    it.clear();
    fkeyActions.clear();
    const float W = (float) screen.getWidth(), pad = 10;
    float y = 8;
    auto add = [&] (Rectangle<float> r, const String& t, const String& a = {}, bool inv = false, float size = 18, Justification j = Justification::centredLeft, bool box = false)
    { LcdScreen::Item x; x.r = r; x.text = t; x.action = a; x.inv = inv; x.size = size; x.just = j; x.box = box; it.push_back (x); };
    auto header = [&] (const String& l, const String& r)
    {
        const float lw = (float) l.length() * 11.0f + 14;
        add ({ pad, y, lw, 22 }, l, {}, true);
        add ({ pad + lw, y, W - 2 * pad - lw, 22 }, r, {}, false, 16, Justification::centredRight);
        y += 28;
    };
    auto row = [&] (const String& l, const String& r, const String& al = {}, const String& ar = {})
    {
        add ({ pad, y, (W - 2 * pad) * 0.62f, 24 }, l, al);
        add ({ pad + (W - 2 * pad) * 0.62f, y, (W - 2 * pad) * 0.38f, 24 }, r, ar, false, 18, Justification::centredRight);
        y += 26;
    };
    auto fam = [&] (const String& centre, const String& aL, const String& aC, const String& aR, float size)
    {
        add ({ pad, y, 36, 40 }, u8 (kLeft), aL, false, 22, Justification::centred, true);
        add ({ pad + 42, y, W - 2 * pad - 84, 40 }, centre, aC, false, size, Justification::centred);
        add ({ W - pad - 36, y, 36, 40 }, u8 (kRight), aR, false, 22, Justification::centred, true);
        y += 46;
    };
    auto fk = [&] (std::initializer_list<std::pair<const char*, const char*>> keys)
    {
        int i = 0;
        for (auto& [l, a] : keys) { fkeys[i]->setButtonText ("F" + String (i + 1) + "  " + u8 (l)); fkeyActions.push_back (a); ++i; }
    };

    const auto& k = mm::kits()[(size_t) proc.kit()];
    const int cur = proc.uiPad;
    const String scr = proc.uiScreen;
    const bool playing = proc.playStep.load() >= 0;
    const String seqName = String::charToString ((juce_wchar) ('A' + proc.seq()));
    auto tuneTxt = [&] (int p) { const int t = (int) proc.get (MonstroProcessor::pid ("tune", p)); return "T" + String (t > 0 ? "+" : "") + String (t); };
    auto decTxt = [&] (int p) { return "D" + String ((int) proc.get (MonstroProcessor::pid ("dec", p))) + "%"; };

    if (scr == "kit")
    {
        int n = 0; for (auto& x : mm::kits()) n += x.vol == proc.uiKitVol ? 1 : 0;
        header ("PROGRAM", String (n) + u8 (" PRODUTORES NO VOL.") + String (proc.uiKitVol) + u8 (" · \xe2\x98\x85 PASTA"));
        const float tw = (W - 2 * pad - 18) / 4;
        for (int v = 1; v <= 4; ++v) add ({ pad + (v - 1) * (tw + 6), y, tw, 24 }, "VOL." + String (v), "kvol:" + String (v), v == proc.uiKitVol, 18, Justification::centred, true);
        y += 30;
        const float cw = (W - 2 * pad - 8) / 2;
        int i = 0;
        for (int ki = 0; ki < mm::numKits(); ++ki)
        {
            const auto& x = mm::kits()[(size_t) ki];
            if (x.vol != proc.uiKitVol) continue;
            const float cx = pad + (i % 2) * (cw + 8), cy = y + (i / 2) * 24.0f;
            add ({ cx, cy, cw, 23 }, u8 (x.name).toUpperCase() + (proc.hasLib (ki) ? u8 (" \xe2\x98\x85") : String()) + "  " + String (proc.kitCountOf (ki)) + " KITS", "load:" + String (ki), ki == proc.kit(), 16);
            ++i;
        }
        y += ((i + 1) / 2) * 24.0f + 6;
        const int nk = proc.kitCount();
        const float kw = (W - 2 * pad - 7 * 4) / 8;
        for (int j = 0; j < nk; ++j)
            add ({ pad + j * (kw + 4), y, kw, 26 }, "KIT " + String (j + 1), "setpk:" + String (j), j == proc.pk(), 15, Justification::centred, true);
        fk ({ { "VOL.1", "kvol:1" }, { "VOL.2", "kvol:2" }, { "KIT \xe2\x97\x80", "pk-1" }, { "KIT \xe2\x96\xb6", "pk+1" }, { "PROD \xe2\x97\x80", "kit-1" }, { "PROD \xe2\x96\xb6", "kit+1" } });
    }
    else if (scr == "timbre")
    {
        const int f = mm::padFam()[(size_t) cur], n = proc.varCount (cur), v = proc.variation (cur);
        header ("TIMBRE", proc.userKitName().isNotEmpty() && proc.hasUserFiles (cur) ? "ARQUIVOS DA PASTA" : u8 (k.name).toUpperCase() + u8 (" · MESMO PRODUTOR"));
        const float tw = (W - 2 * pad - 5 * 4) / 6;
        for (int i = 0; i < mm::numFams; ++i)
            add ({ pad + (i % 6) * (tw + 4), y + (i / 6) * 25.0f, tw, 23 }, mm::famNames()[(size_t) i], "fam:" + String (i), i == f, 15, Justification::centred, true);
        y += 54;
        fam ("PAD" + two (cur + 1) + " " + proc.timbreName (cur).substring (0, 18), "var-1", {}, "var+1", 26);
        String padsOf;
        for (int p = 0; p < mm::numPads; ++p) if (mm::padFam()[(size_t) p] == f) padsOf << (padsOf.isEmpty() ? "" : "/") << (p + 1);
        row (String (v + 1) + " DE " + String (n) + " " + mm::famNames()[(size_t) f] + (padsOf.containsChar ('/') ? u8 (" · PADS ") + padsOf : String()), tuneTxt (cur) + " " + decTxt (cur));
        // grade de 8 × 3 com a página do timbre atual
        const int per = 24, page = v / per, first = page * per;
        const float gw = (W - 2 * pad - 2 * 34 - 7 * 3) / 8;
        add ({ pad, y, 30, 74 }, u8 ("\xc2\xab"), "vpage-", false, 20, Justification::centred, true);
        for (int i = 0; i < per && first + i < n; ++i)
            add ({ pad + 34 + (i % 8) * (gw + 3), y + (i / 8) * 25.0f, gw, 23 }, two (first + i + 1), "setv:" + String (first + i), first + i == v, 16, Justification::centred, true);
        add ({ W - pad - 30, y, 30, 74 }, u8 ("\xc2\xbb"), "vpage+", false, 20, Justification::centred, true);
        fk ({ { "TIMB \xe2\x97\x80", "var-1" }, { "TIMB \xe2\x96\xb6", "var+1" }, { "PAD \xe2\x97\x80", "pad-1" }, { "PAD \xe2\x96\xb6", "pad+1" }, { "SORTEAR", "rndv" }, { "ORIGINAL", "setv:0" } });
    }
    else if (scr == "mixer")
    {
        header ("MIXER", u8 ("TOQUE NO PAD · CLIQUE EM M PARA MUTAR"));
        const float cw = (W - 2 * pad - 8) / 2;
        for (int p = 0; p < mm::numPads; ++p)
        {
            const float cx = pad + (p / 8) * (cw + 8), cy = y + (p % 8) * 27.0f;
            const bool m = proc.get (MonstroProcessor::pid ("mute", p)) > 0.5f;
            add ({ cx, cy, cw - 34, 25 }, two (p + 1) + " " + proc.timbreName (p).substring (0, 11) + "  " + decTxt (p) + " " + tuneTxt (p), "pad:" + String (p), p == cur, 15);
            add ({ cx + cw - 30, cy, 30, 25 }, "M", "mute:" + String (p), m, 16, Justification::centred, true);
        }
        fk ({ { "TUNE \xe2\x88\x92", "tune-" }, { "TUNE +", "tune+" }, { "DEC \xe2\x88\x92", "dec-" }, { "DEC +", "dec+" }, { "VOL \xe2\x88\x92", "pvol-" }, { "VOL +", "pvol+" } });
    }
    else if (scr == "fx")
    {
        header ("EFFECTS", "MASTER");
        const bool sat = proc.get ("satOn") > 0.5f, echo = proc.get ("echoOn") > 0.5f;
        row ("SATURADOR", sat ? "ON" : "BYPASS", "sat", "sat");
        row ("TAPE ECHO", echo ? "ON" : "BYPASS", "echo", "echo");
        row ("DRIVE " + bar (proc.get ("drive")) + "  " + String (roundToInt (proc.get ("drive") * 100)) + "%", u8 ("\xe2\x88\x92   +"));
        add ({ W - pad - 80, y - 26, 38, 24 }, u8 ("\xe2\x88\x92"), "drv-", false, 18, Justification::centred, true);
        add ({ W - pad - 38, y - 26, 38, 24 }, "+", "drv+", false, 18, Justification::centred, true);
        row ("ECO " + String ((int) proc.get ("echoSteps")) + " PASSOS", {});
        add ({ W - pad - 80, y - 26, 38, 24 }, u8 ("\xe2\x88\x92"), "et-", false, 18, Justification::centred, true);
        add ({ W - pad - 38, y - 26, 38, 24 }, "+", "et+", false, 18, Justification::centred, true);
        row ("FEEDBACK " + bar (proc.get ("echoFb") / 0.85f) + "  " + String (roundToInt (proc.get ("echoFb") * 100)) + "%", {});
        add ({ W - pad - 80, y - 26, 38, 24 }, u8 ("\xe2\x88\x92"), "fb-", false, 18, Justification::centred, true);
        add ({ W - pad - 38, y - 26, 38, 24 }, "+", "fb+", false, 18, Justification::centred, true);
        row ("MIX " + bar (proc.get ("echoMix")) + "  " + String (roundToInt (proc.get ("echoMix") * 100)) + "%", {});
        add ({ W - pad - 80, y - 26, 38, 24 }, u8 ("\xe2\x88\x92"), "mix-", false, 18, Justification::centred, true);
        add ({ W - pad - 38, y - 26, 38, 24 }, "+", "mix+", false, 18, Justification::centred, true);
        row ("MASTER " + bar (proc.get ("master")) + "  " + String (roundToInt (proc.get ("master") * 100)) + "%", {});
        add ({ W - pad - 80, y - 26, 38, 24 }, u8 ("\xe2\x88\x92"), "mst-", false, 18, Justification::centred, true);
        add ({ W - pad - 38, y - 26, 38, 24 }, "+", "mst+", false, 18, Justification::centred, true);
        row (u8 ("Padrão de fábrica: saturador e eco em bypass"), {});
        fk ({ { "SAT", "sat" }, { "ECHO", "echo" }, { "TEMPO \xe2\x88\x92", "et-" }, { "TEMPO +", "et+" }, { "FB \xe2\x88\x92", "fb-" }, { "FB +", "fb+" } });
    }
    else if (scr == "seq")
    {
        header ("SEQUENCE", String ((int) proc.get ("bpm")) + u8 (" BPM · SW") + String ((int) proc.get ("swing")) + "%");
        const float cw = (W - 2 * pad - 8) / 2;
        for (int s = 0; s < mm::numSeqs; ++s)
            add ({ pad + (s % 2) * (cw + 8), y + (s / 2) * 30.0f, cw, 28 }, "SEQ " + String::charToString ((juce_wchar) ('A' + s)) + "   " + String (proc.notesIn (s)) + " NOTAS",
                 "pat:" + String (s), s == proc.seq(), 18);
        y += 66;
        const float tw = (W - 2 * pad - 5 * 4) / 6;
        add ({ pad, y, tw, 26 }, u8 (k.name).toUpperCase().substring (0, 10), "prodgroove", proc.styleIsProducer(), 13, Justification::centred, true);
        for (int i = 0; i < 5; ++i)
        {
            const String key (mm::styleKeys()[(size_t) i]);
            add ({ pad + (i + 1) * (tw + 4), y, tw, 26 }, styleUpper (key).upToFirstOccurrenceOf (" ", false, false), "style:" + key, ! proc.styleIsProducer() && key == proc.style(), 13, Justification::centred, true);
        }
        y += 34;
        row (proc.styleIsProducer() ? u8 (mm::prof (proc.kit()).d).substring (0, 52) : (proc.style() == "reggae" ? u8 ("Reggae: caixa/aro no 2 e no 4") : u8 ("Groove de outro estilo")), {});
        row ("TOCA COM A DAW", proc.get ("machineOn") > 0.5f ? "SIM" : u8 ("NÃO"), "daw", "daw");
        fk ({ { "BPM \xe2\x88\x92", "bpm-" }, { "BPM +", "bpm+" }, { "SWING \xe2\x88\x92", "sw-" }, { "SWING +", "sw+" }, { "UNDO", "undo" }, { "LIMPAR", "clear" } });
    }
    else
    {
        header (proc.recording ? "REC" : playing ? "PLAY" : "STOP", "SEQ:" + seqName + " BEAT " + String (playing ? proc.playStep.load() / 4 + 1 : 1));
        fam ((proc.hasLib (proc.kit()) ? "USER: " : "") + u8 (k.name).toUpperCase(), "kit-1", "go:kit", "kit+1", 28);
        fam ("KIT " + String (proc.pk() + 1) + "/" + String (proc.kitCount()) + (proc.userKitName().isNotEmpty() ? " " + proc.userKitName().toUpperCase().substring (0, 14) : String()), "pk-1", "go:kit", "pk+1", 20);
        row ("VOL." + String (k.vol) + " " + styleUpper (proc.style()), String ((int) proc.get ("bpm")) + " BPM SW" + String ((int) proc.get ("swing")) + "%");
        fam ("PAD" + two (cur + 1) + " " + proc.timbreName (cur).substring (0, 16) + " /" + String (proc.varCount (cur)), "var-1", "go:timbre", "var+1", 22);
        row ("MST " + bar (proc.get ("master")), String ("SAT:") + (proc.get ("satOn") > 0.5f ? "ON" : "BYP") + " ECHO:" + (proc.get ("echoOn") > 0.5f ? "ON" : "BYP"), {}, "go:fx");
        const float mw = (W - 2 * pad - 15 * 3) / 16;
        const int ps = proc.playStep.load();
        for (int s = 0; s < mm::numSteps; ++s)
        {
            const int v = proc.getStep (proc.seq(), cur, s);
            add ({ pad + s * (mw + 3), y + 4, mw, 18 }, v == 2 ? ">" : "", {}, v != 0, 12, Justification::centred, s == ps || s % 4 == 0);
        }
        fk ({ { "PROD \xe2\x97\x80", "kit-1" }, { "PROD \xe2\x96\xb6", "kit+1" }, { "KIT \xe2\x97\x80", "pk-1" }, { "KIT \xe2\x96\xb6", "pk+1" }, { "TIMB \xe2\x97\x80", "var-1" }, { "TIMB \xe2\x96\xb6", "var+1" } });
    }
    for (int i = 0; i < modes.size(); ++i)
    {
        static const char* keys[6] { "main", "kit", "timbre", "mixer", "fx", "seq" };
        const bool on = scr == keys[i];
        modes[i]->setColour (TextButton::buttonColourId, on ? mmui::red.darker (0.3f) : mmui::body2);
        modes[i]->setColour (TextButton::textColourOffId, on ? Colours::white : mmui::silk.withAlpha (0.8f));
    }
    screen.repaint();
}

// ---------------------------------------------------------------- ações (as mesmas da versão web)
String MonstroEditor::folderToast (int kits) const
{
    if (kits <= 0) return "Nenhum som reconhecido na pasta";
    if (proc.lastLoadProducers > 1) return String (proc.lastLoadProducers) + u8 (" produtores · ") + String (kits) + " kits carregados";
    return u8 (mm::kits()[(size_t) proc.kit()].name) + ": " + String (kits) + (kits > 1 ? " kits carregados" : " kit carregado");
}

void MonstroEditor::nudge (const String& id, float d, float lo, float hi) { proc.setParam (id, jlimit (lo, hi, proc.get (id) + d)); }

void MonstroEditor::act (const String& a)
{
    const String c = a.upToFirstOccurrenceOf (":", false, false), v = a.fromFirstOccurrenceOf (":", false, false);
    auto& p = proc;
    const int cur = p.uiPad;
    auto tap = [&] (int pad) { p.uiPad = pad; p.playPad (pad, 0.85f); };

    if (a == "kit-1" || a == "kit+1") { p.selectKit (p.kit() + (a == "kit+1" ? 1 : -1)); toast (u8 (mm::kits()[(size_t) p.kit()].name)); }
    else if (a == "pk-1" || a == "pk+1") { p.selectPk (p.pk() + (a == "pk+1" ? 1 : -1)); toast ("KIT " + String (p.pk() + 1) + (p.userKitName().isNotEmpty() ? u8 (" \xc2\xb7 ") + p.userKitName() : String())); }
    else if (a == "prodgroove") { p.pushUndo(); p.loadProducerGroove(); toast ("Groove do " + u8 (mm::kits()[(size_t) p.kit()].name)); }
    else if (a == "var-1" || a == "var+1") p.setVariation (cur, p.variation (cur) + (a == "var+1" ? 1 : -1));
    else if (a == "vpage-" || a == "vpage+") p.setVariation (cur, p.variation (cur) + (a == "vpage+" ? 24 : -24));
    else if (a == "rndv") p.setVariation (cur, Random::getSystemRandom().nextInt (jmax (1, p.varCount (cur))));
    else if (a == "pad-1" || a == "pad+1") tap ((cur + (a == "pad+1" ? 1 : 15)) % 16);
    else if (a == "mst-" || a == "mst+") nudge ("master", a == "mst+" ? 0.1f : -0.1f, 0, 1);
    else if (a == "tune-" || a == "tune+") { nudge (MonstroProcessor::pid ("tune", cur), a == "tune+" ? 1.0f : -1.0f, -12, 12); tap (cur); }
    else if (a == "dec-" || a == "dec+") { nudge (MonstroProcessor::pid ("dec", cur), a == "dec+" ? 10.0f : -10.0f, 25, 200); tap (cur); }
    else if (a == "pvol-" || a == "pvol+") { nudge (MonstroProcessor::pid ("vol", cur), a == "pvol+" ? 0.1f : -0.1f, 0, 1); tap (cur); }
    else if (a == "sat") p.setParam ("satOn", p.get ("satOn") > 0.5f ? 0.0f : 1.0f);
    else if (a == "echo") p.setParam ("echoOn", p.get ("echoOn") > 0.5f ? 0.0f : 1.0f);
    else if (a == "drv-" || a == "drv+") nudge ("drive", a == "drv+" ? 0.05f : -0.05f, 0, 1);
    else if (a == "et-" || a == "et+") nudge ("echoSteps", a == "et+" ? 1.0f : -1.0f, 1, 8);
    else if (a == "fb-" || a == "fb+") nudge ("echoFb", a == "fb+" ? 0.05f : -0.05f, 0, 0.85f);
    else if (a == "mix-" || a == "mix+") nudge ("echoMix", a == "mix+" ? 0.05f : -0.05f, 0, 1);
    else if (a == "bpm-" || a == "bpm+") nudge ("bpm", a == "bpm+" ? 1.0f : -1.0f, 60, 180);
    else if (a == "sw-" || a == "sw+") nudge ("swing", a == "sw+" ? 1.0f : -1.0f, 50, 75);
    else if (a == "undo") toast (p.undo() ? "Desfeito" : "Nada para desfazer");
    else if (a == "clear") { p.pushUndo(); p.clearSeq (p.seq()); toast ("SEQ " + String::charToString ((juce_wchar) ('A' + p.seq())) + " limpa"); }
    else if (a == "dprev" || a == "dnext")
    {
        const bool nx = a == "dnext";
        const String s = p.uiScreen;
        act (s == "kit" ? (nx ? "kit+1" : "kit-1") : s == "mixer" ? (nx ? "pad+1" : "pad-1") : s == "seq" ? (nx ? "bpm+" : "bpm-") : s == "fx" ? (nx ? "fb+" : "fb-") : (nx ? "var+1" : "var-1"));
        return;
    }
    else if (a == "tap")
    {
        const double now = Time::getMillisecondCounterHiRes();
        for (int i = taps.size(); --i >= 0;) if (now - taps[i] > 2500) taps.remove (i);
        taps.add (now);
        if (taps.size() > 1) p.setParam ("bpm", (float) jlimit (60, 180, roundToInt (60000.0 / ((taps.getLast() - taps[0]) / (taps.size() - 1)))));
    }
    else if (a == "play") { p.setParam ("preview", 1.0f); }
    else if (a == "stop") { p.setParam ("preview", 0.0f); p.recording = false; }
    else if (a == "rec") { p.recording = ! p.recording.load(); if (p.recording) p.setParam ("preview", 1.0f); }
    else if (a == "daw") p.setParam ("machineOn", p.get ("machineOn") > 0.5f ? 0.0f : 1.0f);
    else if (a == "factory") { p.forgetFolders(); toast (u8 ("Kits de fábrica: ") + u8 (mm::kits()[(size_t) p.kit()].name)); }
    else if (a == "folder")
    {
        chooser = std::make_unique<FileChooser> ("Pasta do kit (o Monstro.zip descompactado ou outra pasta de samples)", File(), "*");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories, [this] (const FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (! f.isDirectory()) return;
            const int n = proc.loadFolder (f);
            toast (folderToast (n));
            buildScreen(); refresh();
        });
        return;
    }
    else if (a == "sample")
    {
        chooser = std::make_unique<FileChooser> ("Som para o PAD " + String (cur + 1), File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f.existsAsFile() && proc.loadSample (proc.uiPad, f)) { proc.playPad (proc.uiPad, 0.9f); toast (f.getFileName() + " no PAD " + String (proc.uiPad + 1)); }
            buildScreen(); refresh();
        });
        return;
    }
    else if (c == "go") { p.uiScreen = v; if (v == "kit") p.uiKitVol = mm::kits()[(size_t) p.kit()].vol; }
    else if (c == "kvol") { p.uiKitVol = v.getIntValue(); p.uiScreen = "kit"; }
    else if (c == "load") { p.selectKit (v.getIntValue()); toast (u8 (mm::kits()[(size_t) p.kit()].name) + " carregado"); }
    else if (c == "setpk") p.selectPk (v.getIntValue());
    else if (c == "setv") p.setVariation (cur, v.getIntValue());
    else if (c == "fam")
    {
        const int f = v.getIntValue();
        int next = -1, first = -1;
        for (int i = 0; i < mm::numPads; ++i)
            if (mm::padFam()[(size_t) i] == f) { if (first < 0) first = i; if (i > cur && next < 0) next = i; }
        tap (next >= 0 && mm::padFam()[(size_t) cur] == f ? next : first);
    }
    else if (c == "pad") tap (v.getIntValue());
    else if (c == "mute") { const String id = MonstroProcessor::pid ("mute", v.getIntValue()); p.setParam (id, p.get (id) > 0.5f ? 0.0f : 1.0f); }
    else if (c == "pat") p.setParam ("seq", (float) v.getIntValue());
    else if (c == "style") { p.pushUndo(); p.loadStyle (v); toast ("Groove: " + String (mm::styleName (v.toStdString()))); }
    buildScreen();
    refresh();
}

void MonstroEditor::padPressed (int pad)
{
    proc.uiPad = pad;
    proc.playPad (pad, 0.9f);
    if (proc.recording && proc.playStep.load() >= 0)
    {
        proc.pushUndo();
        proc.setStep (proc.seq(), pad, proc.playStep.load(), 1);
    }
    buildScreen();
    refresh();
}

void MonstroEditor::stepClicked (int s, bool accent)
{
    proc.pushUndo();
    const int sq = proc.seq(), cur = proc.uiPad, v = proc.getStep (sq, cur, s);
    const int nv = accent ? 2 : (v ? 0 : 1);
    proc.setStep (sq, cur, s, nv);
    if (nv && proc.playStep.load() < 0) proc.playPad (cur, nv == 2 ? 1.0f : 0.75f);
    buildScreen();
    refresh();
}

void MonstroEditor::filesDropped (const StringArray& files, int x, int y)
{
    const File f (files[0]);
    if (f.isDirectory()) { const int n = proc.loadFolder (f); toast (folderToast (n)); }
    else
    {
        int target = proc.uiPad;
        for (int i = 0; i < pads.size(); ++i) if (pads[i]->getBounds().contains (x, y)) target = i;
        if (proc.loadSample (target, f)) { proc.uiPad = target; proc.playPad (target, 0.9f); toast (f.getFileName() + " no PAD " + String (target + 1)); }
        else toast ("Formato de audio nao reconhecido");
    }
    buildScreen(); refresh();
}

void MonstroEditor::toast (const String& msg)
{
    toastL.setText (msg, dontSendNotification);
    toastL.setVisible (true);
    toastL.toFront (false);
    toastUntil = Time::getMillisecondCounter() + 1800;
}

void MonstroEditor::refresh()
{
    const int sq = proc.seq(), cur = proc.uiPad, ps = proc.playStep.load();
    for (int i = 0; i < pads.size(); ++i)
    {
        auto* b = pads[i];
        b->label = proc.timbreName (i);
        b->current = i == cur;
        b->user = proc.hasSample (i);
        b->muted = proc.get (MonstroProcessor::pid ("mute", i)) > 0.5f;
        b->repaint();
    }
    for (int s = 0; s < steps.size(); ++s) { steps[s]->value = proc.getStep (sq, cur, s); steps[s]->now = s == ps; steps[s]->repaint(); }
    for (int i = 0; i < seqBtns.size(); ++i) seqBtns[i]->setColour (TextButton::buttonColourId, i == sq ? mmui::amber.darker (0.2f) : mmui::body2);
    stepLbl.setText ("PAD " + String (cur + 1) + u8 (" · ") + proc.timbreName (cur) + u8 (" · SEQ ") + String::charToString ((juce_wchar) ('A' + sq))
                     + u8 ("  —  clique liga/desliga, segure (ou botão direito) = acento"), dontSendNotification);
    const bool prev = proc.get ("preview") > 0.5f, daw = proc.get ("machineOn") > 0.5f;
    playB.setColour (TextButton::buttonColourId, prev ? Colour (0xff2f8f3a) : mmui::body2);
    recB.setColour (TextButton::buttonColourId, proc.recording ? mmui::red : mmui::body2);
    dawB.setColour (TextButton::buttonColourId, daw ? Colour (0xff2f6f8f) : mmui::body2);
    dawB.setButtonText (daw ? "TOCA COM A DAW: SIM" : u8 ("TOCA COM A DAW: NÃO"));
}

void MonstroEditor::timerCallback()
{
    bool any = false;
    for (int i = 0; i < mm::numPads; ++i)
    {
        const auto h = proc.padHits[(size_t) i].load();
        if (h != lastHits[(size_t) i]) { lastHits[(size_t) i] = h; pads[i]->glow = 1.0f; any = true; }
        else if (pads[i]->glow > 0) { pads[i]->glow = jmax (0.0f, pads[i]->glow - 0.15f); pads[i]->repaint(); }
    }
    // a tela só é refeita quando algo visível muda (passo, parâmetros, kit)
    String key;
    key << proc.playStep.load() << "|" << proc.kit() << "|" << proc.pk() << "|" << proc.seq() << "|" << proc.uiPad << "|" << (int) proc.recording.load();
    for (auto* prm : proc.getParameters()) key << "," << roundToInt (prm->getValue() * 1000);
    if (key != lastKey || any) { lastKey = key; buildScreen(); refresh(); }
    if (toastL.isVisible() && Time::getMillisecondCounter() > toastUntil) toastL.setVisible (false);
    repaint (getWidth() - 116, 30, 20, 20);
}
