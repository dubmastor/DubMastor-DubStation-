#include "PluginEditor.h"

using namespace mmui;

static juce::Font mono (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, bold ? juce::Font::bold : juce::Font::plain));
}
static juce::Font sans (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

// ---------------- visual ----------------
MonstroLook::MonstroLook()
{
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff525457));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff5f8f44));
    setColour (juce::TextButton::textColourOffId, silk);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1c1d1f));
    setColour (juce::ComboBox::textColourId, silk);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
    setColour (juce::ComboBox::arrowColourId, amber);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c1d1f));
    setColour (juce::PopupMenu::textColourId, silk);
    setColour (juce::PopupMenu::headerTextColourId, amber);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, amber);
    setColour (juce::PopupMenu::highlightedTextColourId, ink);
    setColour (juce::Slider::textBoxTextColourId, amber);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colour (0xffb8b6b0));
}

void MonstroLook::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& c, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    auto base = b.getToggleState() ? (b.getName() == "REC" ? red : juce::Colour (0xff5f8f44)) : c;
    if (over) base = base.brighter (.08f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r.translated (0, down ? 1.f : 2.f), 5.f);
    g.setGradientFill (juce::ColourGradient (base.brighter (.15f), r.getX(), r.getY(), base.darker (.25f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r.withTrimmedBottom (2.f).translated (0, down ? 1.f : 0.f), 5.f);
}

void MonstroLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    auto k = r.withSizeKeepingCentre (d, d);
    g.setColour (juce::Colours::black.withAlpha (.5f)); g.fillEllipse (k.translated (0, 2.f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a6c70), k.getX(), k.getY(), juce::Colour (0xff2a2b2d), k.getX(), k.getBottom(), false));
    g.fillEllipse (k);
    juce::Path arc;
    arc.addCentredArc (k.getCentreX(), k.getCentreY(), d / 2 + 2, d / 2 + 2, 0, a0, a0 + (a1 - a0) * pos, true);
    g.setColour (amber); g.strokePath (arc, juce::PathStrokeType (2.5f));
    const float a = a0 + (a1 - a0) * pos;
    g.setColour (silk);
    g.drawLine (k.getCentreX(), k.getCentreY(), k.getCentreX() + std::sin (a) * d * .4f, k.getCentreY() - std::cos (a) * d * .4f, 2.5f);
}

// ---------------- pads ----------------
void PadButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.f);
    const bool f = flashing(), cur = proc.selectedPad.load() == index, user = proc.hasSample (index);
    g.setColour (juce::Colours::black); g.fillRoundedRectangle (r.translated (0, f ? 2.f : 4.f), 9.f);
    auto top = r.withTrimmedBottom (4.f).translated (0, f ? 2.f : 0.f);
    juce::Colour hi = f ? juce::Colour (0xffffd27a) : user ? juce::Colour (0xff99bbbb) : padHi;
    juce::Colour lo = f ? amber : user ? juce::Colour (0xff668888) : pad;
    g.setGradientFill (juce::ColourGradient (hi, top.getCentreX(), top.getY() + top.getHeight() * .35f, lo, top.getX(), top.getBottom(), true));
    g.fillRoundedRectangle (top, 9.f);
    if (f) { g.setColour (amber.withAlpha (.35f)); g.drawRoundedRectangle (top.expanded (2.f), 10.f, 4.f); }
    g.setColour (cur ? red : juce::Colour (0xff2a2b2d));
    g.drawRoundedRectangle (top.expanded (1.5f), 10.f, cur ? 3.f : 2.f);
    g.setColour (juce::Colour (0xff1a1a1a));
    g.setFont (sans (12.f, true));
    g.drawText (juce::String (index + 1), top.reduced (7.f), juce::Justification::topLeft);
    g.setFont (sans (11.f));
    g.drawFittedText (proc.sampleName (index), top.reduced (7.f).withTrimmedTop (top.getHeight() * .55f).toNearestInt(), juce::Justification::bottomLeft, 2);
}

void PadButton::mouseDown (const juce::MouseEvent& e)
{
    const float vel = juce::jlimit (.35f, 1.f, 1.f - (float) e.position.y / (float) getHeight() * .5f + .25f);
    proc.hitPad (index, vel);
    flash();
    if (auto* p = getParentComponent()) p->repaint();
}

// ---------------- passos ----------------
int StepRow::stepAt (juce::Point<int> p) const
{
    return juce::jlimit (0, mm::kSteps - 1, p.x * mm::kSteps / juce::jmax (1, getWidth()));
}

void StepRow::paint (juce::Graphics& g)
{
    const int seq = (int) proc.apvts.getRawParameterValue ("seq")->load();
    const int pad = proc.selectedPad.load(), now = proc.currentStep();
    const float w = getWidth() / (float) mm::kSteps;
    for (int s = 0; s < mm::kSteps; ++s)
    {
        auto r = juce::Rectangle<float> (s * w, 0, w, (float) getHeight()).reduced (2.f);
        const int v = proc.getStep (seq, pad, s);
        g.setColour (v == 2 ? juce::Colour (0xffff7a1a) : v ? amber : (s % 4 == 0 ? juce::Colour (0xff5a5c60) : juce::Colour (0xff4b4d50)));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (s == now ? juce::Colours::white : juce::Colours::black);
        g.drawRoundedRectangle (r, 4.f, s == now ? 2.f : 1.f);
        if (s % 4 == 0)
        {
            g.setColour (juce::Colour (0xff1a1a1a));
            g.setFont (sans (10.f, true));
            g.drawText (juce::String (s / 4 + 1), r.reduced (3.f), juce::Justification::topLeft);
        }
    }
}

void StepRow::mouseDown (const juce::MouseEvent& e)
{
    downStep = stepAt (e.getPosition());
    downAt = juce::Time::getMillisecondCounter();
    if (e.mods.isPopupMenu() || e.mods.isShiftDown())
    {
        const int seq = (int) proc.apvts.getRawParameterValue ("seq")->load();
        proc.pushUndo();
        proc.setStep (seq, proc.selectedPad.load(), downStep, 2);
        downStep = -1;
        repaint();
    }
}

void StepRow::mouseUp (const juce::MouseEvent&)
{
    if (downStep < 0) return;
    const int seq = (int) proc.apvts.getRawParameterValue ("seq")->load(), pad = proc.selectedPad.load();
    proc.pushUndo();
    const bool longPress = juce::Time::getMillisecondCounter() - downAt > 420;   // toque segura = acento
    const int v = proc.getStep (seq, pad, downStep);
    proc.setStep (seq, pad, downStep, longPress ? 2 : (v ? 0 : 1));
    if (! longPress && ! v && ! proc.isRunning()) proc.hitPad (pad, .8f);
    downStep = -1;
    repaint();
}

// ---------------- LCD ----------------
void Lcd::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff111111)); g.fillRoundedRectangle (r, 10.f);
    auto s = r.reduced (10.f);
    g.setGradientFill (juce::ColourGradient (lcd, s.getX() + s.getWidth() * .4f, s.getY() + s.getHeight() * .3f, lcd2, s.getRight(), s.getBottom(), true));
    g.fillRoundedRectangle (s, 4.f);
    auto t = s.reduced (10.f, 6.f);
    const auto& k = mm::kits()[(size_t) proc.apvts.getRawParameterValue ("kit")->load()];
    const int seq = (int) proc.apvts.getRawParameterValue ("seq")->load(), pad = proc.selectedPad.load(), step = proc.currentStep();
    const float lh = t.getHeight() / 7.f;
    auto line = [&] (int i, const juce::String& left, const juce::String& right, float size = 17.f, bool inv = false)
    {
        auto row = juce::Rectangle<float> (t.getX(), t.getY() + lh * (float) i, t.getWidth(), lh * (size > 20 ? 1.6f : 1.f));
        g.setFont (mono (size, size > 20));
        if (inv)
        {
            const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), left) + 8.f;
            g.setColour (lcdInk); g.fillRect (row.withWidth (w));
            g.setColour (lcd); g.drawText (left, row.withWidth (w), juce::Justification::centred);
        }
        else { g.setColour (lcdInk); g.drawText (left, row, juce::Justification::centredLeft); }
        g.setColour (lcdInk); g.drawText (right, row, juce::Justification::centredRight);
    };
    const juce::String status = proc.isRunning() ? (proc.recording ? "REC" : "PLAY") : "STOP";
    line (0, status, "SEQ:" + juce::String::charToString ((juce::juce_wchar) ('A' + seq)) + "  BEAT " + juce::String (step < 0 ? 1 : step / 4 + 1), 17.f, true);
    line (1, U (k.name).toUpperCase(), {}, 26.f);
    const float vol = proc.apvts.getRawParameterValue ("vol")->load();
    juce::String bar; for (int i = 0; i < 10; ++i) bar << (i < juce::roundToInt (vol * 10) ? "#" : ".");
    line (3, "VOL." + juce::String (k.vol) + " " + U (mm::styleName (proc.forcedStyle >= 0 ? proc.forcedStyle.load() : k.style)).toUpperCase(),
          juce::String (juce::roundToInt (proc.shownBpm())) + " BPM SW" + juce::String ((int) proc.apvts.getRawParameterValue ("swing")->load()) + "%");
    const int tu = (int) proc.apvts.getRawParameterValue ("tune" + juce::String (pad))->load();
    line (4, "PAD" + juce::String (pad + 1).paddedLeft ('0', 2) + " " + proc.sampleName (pad).substring (0, 16),
          "T" + juce::String (tu > 0 ? "+" : "") + juce::String (tu) + " D" + juce::String ((int) proc.apvts.getRawParameterValue ("dec" + juce::String (pad))->load()) + "%");
    line (5, "MST " + bar, juce::String ("SAT:") + (proc.apvts.getRawParameterValue ("satOn")->load() > .5f ? "ON " : "BYP")
                              + " ECHO:" + (proc.apvts.getRawParameterValue ("echoOn")->load() > .5f ? "ON" : "BYP"));
    // mini grade do pad
    const float y = t.getY() + lh * 6.2f, cw = t.getWidth() / (float) mm::kSteps;
    for (int st = 0; st < mm::kSteps; ++st)
    {
        auto c = juce::Rectangle<float> (t.getX() + st * cw + 1, y, cw - 2, lh * .45f);
        g.setColour (proc.getStep (seq, pad, st) ? lcdInk : lcdInk.withAlpha (.15f));
        g.fillRect (c);
        if (st == step) { g.setColour (lcdInk); g.drawRect (c.expanded (1.f), 1.f); }
    }
}

// ---------------- logos ----------------
void LogoSlot::setImageData (const juce::String& b64)
{
    img = {};
    if (b64.isEmpty()) return;
    juce::MemoryOutputStream mo;
    if (juce::Base64::convertFromBase64 (mo, b64))
        img = juce::ImageFileFormat::loadFrom (mo.getData(), mo.getDataSize());
    repaint();
}

void LogoSlot::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.f);
    if (img.isValid()) { g.drawImage (img, r, juce::RectanglePlacement::centred); return; }
    g.setColour (juce::Colours::black.withAlpha (.15f)); g.fillRoundedRectangle (r, 8.f);
    juce::Path p; p.addRoundedRectangle (r, 8.f);
    juce::Path dashed; const float dl[] = { 4.f, 3.f };
    juce::PathStrokeType (1.f).createDashedStroke (dashed, p, dl, 2);
    g.setColour (juce::Colour (0xff777777)); g.fillPath (dashed);
    g.setColour (juce::Colour (0xffaaaaaa)); g.setFont (sans (11.f));
    g.drawFittedText (text, getLocalBounds().reduced (4), juce::Justification::centred, 2);
}

// ---------------- MIDI ----------------
juce::File MidiDrag::writeTemp()
{
    const int seq = (int) proc.apvts.getRawParameterValue ("seq")->load();
    const auto& k = mm::kits()[(size_t) proc.apvts.getRawParameterValue ("kit")->load()];
    auto bytes = mm::patternToMidi (proc.seqGrid (seq), proc.shownBpm());
    auto name = "Monstro_" + U (k.name).retainCharacters ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")
              + "_SEQ" + juce::String::charToString ((juce::juce_wchar) ('A' + seq)) + "_" + juce::String (juce::roundToInt (proc.shownBpm())) + "bpm.mid";
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("MonstroMachine").getChildFile (name);
    f.getParentDirectory().createDirectory();
    f.replaceWithData (bytes.data(), bytes.size());
    return f;
}

void MidiDrag::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.f);
    g.setColour (juce::Colours::black); g.fillRoundedRectangle (r.translated (0, 2.f), 5.f);
    g.setColour (juce::Colour (0xff3f6f8f)); g.fillRoundedRectangle (r.withTrimmedBottom (2.f), 5.f);
    g.setColour (juce::Colours::white); g.setFont (sans (13.f, true));
    g.drawText (U ("⇩ ARRASTAR MIDI"), r, juce::Justification::centred);
}

void MidiDrag::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true;
    auto f = writeTemp();
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; });
}

// ---------------- editor ----------------
MonstroEditor::MonstroEditor (MonstroProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&look);
    addAndMakeVisible (lcd);
    for (auto* l : { &logoA, &logoM }) addAndMakeVisible (l);
    logoA.setImageData (proc.logoA); logoM.setImageData (proc.logoM);
    logoA.onClick = [this] { chooseLogo (true); };
    logoM.onClick = [this] { chooseLogo (false); };

    const char* fk[6] = { "VOL -", "VOL +", "KIT <", "KIT >", "ESTILO", "LIMPAR" };
    for (int i = 0; i < 6; ++i) { fkeys[i].setButtonText (fk[i]); addAndMakeVisible (fkeys[i]); }
    fkeys[0].onClick = [this] { setParam ("vol", juce::jmax (0.f, param ("vol") - .1f)); };
    fkeys[1].onClick = [this] { setParam ("vol", juce::jmin (1.f, param ("vol") + .1f)); };
    fkeys[2].onClick = [this] { proc.selectKit ((int) param ("kit") - 1); };
    fkeys[3].onClick = [this] { proc.selectKit ((int) param ("kit") + 1); };
    fkeys[4].onClick = [this] {
        const auto& k = mm::kits()[(size_t) param ("kit")];
        const int cur = proc.forcedStyle >= 0 ? proc.forcedStyle.load() : (int) k.style;
        proc.forcedStyle = (cur + 1) % mm::kStyles;
        proc.loadStyle (proc.forcedStyle);
    };
    fkeys[5].onClick = [this] { proc.clearSeq ((int) param ("seq")); };

    int lastVol = 0;
    for (int i = 0; i < (int) mm::kits().size(); ++i)
    {
        const auto& k = mm::kits()[(size_t) i];
        if (k.vol != lastVol) { kitBox.addSectionHeading ("Monstro.zip Vol. " + juce::String (k.vol)); lastVol = k.vol; }
        kitBox.addItem (U (k.name), i + 1);
    }
    kitBox.onChange = [this] { const int id = kitBox.getSelectedId(); if (id > 0 && id - 1 != (int) param ("kit")) proc.selectKit (id - 1); };
    addAndMakeVisible (kitBox);

    recB.setName ("REC");
    for (auto* b : { &recB, &playB, &stopB, &tapB, &undoB }) addAndMakeVisible (b);
    recB.onClick = [this] { proc.recording = ! proc.recording; if (proc.recording) proc.internalPlay = true; };
    playB.onClick = [this] { proc.internalPlay = true; };
    stopB.onClick = [this] { proc.internalPlay = false; proc.recording = false; };
    undoB.onClick = [this] { proc.undo(); repaint(); };
    tapB.onClick = [this] {
        const double now = juce::Time::getMillisecondCounterHiRes();
        while (! taps.isEmpty() && now - taps.getFirst() > 2500) taps.remove (0);
        taps.add (now);
        if (taps.size() > 1)
            setParam ("bpm", (float) juce::jlimit (60.0, 180.0, std::round (60000.0 / ((taps.getLast() - taps.getFirst()) / (taps.size() - 1)))));
    };

    auto knob = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (sans (11.f));
        addAndMakeVisible (s); addAndMakeVisible (l);
    };
    knob (bpmS, bpmL, "BPM (sem DAW)"); knob (swingS, swingL, "SWING"); knob (tuneS, tuneL, "PAD TUNE"); knob (decS, decL, "PAD DECAY");
    knob (satS, satL, "DRIVE"); knob (echoTS, echoTL, "TEMPO"); knob (echoFS, echoFL, "FEEDBACK"); knob (echoMS, echoML, "MIX");
    bpmA = std::make_unique<SA> (proc.apvts, "bpm", bpmS);
    swingA = std::make_unique<SA> (proc.apvts, "swing", swingS);
    satA = std::make_unique<SA> (proc.apvts, "satDrive", satS);
    echoTA = std::make_unique<SA> (proc.apvts, "echoTime", echoTS);
    echoFA = std::make_unique<SA> (proc.apvts, "echoFb", echoFS);
    echoMA = std::make_unique<SA> (proc.apvts, "echoMix", echoMS);
    for (auto* b : { &satB, &echoB }) { b->setClickingTogglesState (true); addAndMakeVisible (b); }
    satBA = std::make_unique<BA> (proc.apvts, "satOn", satB);
    echoBA = std::make_unique<BA> (proc.apvts, "echoOn", echoB);

    for (int i = 0; i < 4; ++i)
    {
        seqB[i].setButtonText ("SEQ " + juce::String::charToString ((juce::juce_wchar) ('A' + i)));
        seqB[i].onClick = [this, i] { setParam ("seq", (float) i); };
        addAndMakeVisible (seqB[i]);
    }
    for (auto* b : { &folderB, &sampleB, &clearSmpB, &saveMidiB }) addAndMakeVisible (b);
    folderB.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> (U ("Escolha a pasta descompactada do kit"), juce::File(), "");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [this] (const juce::FileChooser& c) { auto d = c.getResult(); if (d.isDirectory()) proc.loadFolder (d); repaint(); });
    };
    sampleB.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> (U ("Sample para o pad ") + juce::String (proc.selectedPad.load() + 1), juce::File(), "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& c) { auto f = c.getResult(); if (f.existsAsFile()) proc.loadSampleFile (proc.selectedPad.load(), f); repaint(); });
    };
    clearSmpB.onClick = [this] { proc.clearSamples(); repaint(); };
    saveMidiB.onClick = [this] {
        auto tmp = midiDrag.writeTemp();
        chooser = std::make_unique<juce::FileChooser> ("Salvar MIDI", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (tmp.getFileName()), "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                              [tmp] (const juce::FileChooser& c) { auto f = c.getResult(); if (f != juce::File()) tmp.copyFileTo (f.withFileExtension ("mid")); });
    };

    // ordem MPC: pad 13–16 em cima, 1–4 embaixo
    const int order[16] = { 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3 };
    for (int i : order) addAndMakeVisible (pads.add (new PadButton (proc, i)));
    addAndMakeVisible (steps);
    addAndMakeVisible (midiDrag);

    setResizable (true, true);
    setResizeLimits (900, 720, 1600, 1260);
    getConstrainer()->setFixedAspectRatio (1000.0 / 800.0);
    setSize (1000, 800);
    startTimerHz (30);
}

MonstroEditor::~MonstroEditor() { setLookAndFeel (nullptr); }

void MonstroEditor::setParam (const juce::String& id, float value)
{
    if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (value));
}
float MonstroEditor::param (const juce::String& id) const { return proc.apvts.getRawParameterValue (id)->load(); }

void MonstroEditor::chooseLogo (bool academia)
{
    chooser = std::make_unique<juce::FileChooser> (academia ? "Logo da Academia de Beats" : "Logo do Monstro.zip", juce::File(), "*.png;*.jpg;*.jpeg;*.gif");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, academia] (const juce::FileChooser& c)
        {
            auto f = c.getResult();
            juce::MemoryBlock mb;
            if (! f.existsAsFile() || f.getSize() > 4 * 1024 * 1024 || ! f.loadFileAsData (mb)) return;
            const auto b64 = juce::Base64::toBase64 (mb.getData(), mb.getSize());
            (academia ? proc.logoA : proc.logoM) = b64;
            (academia ? logoA : logoM).setImageData (b64);
        });
}

void MonstroEditor::bindPadSliders()
{
    const int p = proc.selectedPad.load();
    if (p == boundPad) return;
    boundPad = p;
    tuneA.reset(); decA.reset();
    tuneA = std::make_unique<SA> (proc.apvts, "tune" + juce::String (p), tuneS);
    decA = std::make_unique<SA> (proc.apvts, "dec" + juce::String (p), decS);
}

void MonstroEditor::timerCallback()
{
    bindPadSliders();
    const int k = (int) param ("kit");
    if (k != shownKit) { shownKit = k; kitBox.setSelectedId (k + 1, juce::dontSendNotification); repaint (infoArea); }
    const int seq = (int) param ("seq");
    if (seq != shownSeq) { shownSeq = seq; for (int i = 0; i < 4; ++i) seqB[i].setToggleState (i == seq, juce::dontSendNotification); }
    playB.setToggleState (proc.isRunning(), juce::dontSendNotification);
    recB.setToggleState (proc.recording.load(), juce::dontSendNotification);
    for (auto* pb : pads)
    {
        const auto h = proc.padHits (pb->index);
        if (h != pb->seen) { pb->seen = h; pb->flash(); }
        const bool f = pb->flashing();
        if (f || pb->wasFlashing) pb->repaint();
        pb->wasFlashing = f;
    }
    lcd.repaint(); steps.repaint();
}

void MonstroEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff141414));
    const float sc = getWidth() / 1000.f;
    auto body = juce::Rectangle<float> (10, 10, 980, 585).transformedBy (juce::AffineTransform::scale (sc));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff46484b), 0, body.getY(), body2, 0, body.getBottom(), false));
    g.fillRoundedRectangle (body, 16.f);
    g.setColour (juce::Colours::black); g.drawRoundedRectangle (body, 16.f, 1.f);

    g.setColour (silk); g.setFont (sans (26.f * sc, true));
    g.drawText ("MONSTRO MACHINE", juce::Rectangle<float> (26, 18, 360, 34).transformedBy (juce::AffineTransform::scale (sc)), juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffb8b6b0)); g.setFont (sans (13.f * sc));
    g.drawText (U ("MM-2500 · DubMastor · timbres Monstro.zip · v") + JucePlugin_VersionString,
                juce::Rectangle<float> (290, 18, 360, 34).transformedBy (juce::AffineTransform::scale (sc)), juce::Justification::centredLeft);

    // pads: base de borracha
    g.setColour (juce::Colour (0xff1f2022));
    g.fillRoundedRectangle (juce::Rectangle<float> (530, 104, 450, 392).transformedBy (juce::AffineTransform::scale (sc)), 12.f);

    // ficha do produtor
    const auto& k = mm::kits()[(size_t) param ("kit")];
    auto info = infoArea.toFloat();
    g.setColour (paper); g.fillRoundedRectangle (info, 14.f);
    auto a = info.reduced (20.f * sc, 14.f * sc);
    auto left = a.removeFromLeft (a.getWidth() * .58f);
    g.setColour (ink); g.setFont (sans (13.f * sc));
    g.drawText ("Monstro.zip Vol. " + juce::String (k.vol) + U ("  ·  ") + U (mm::styleName (k.style)) + U (" · ") + juce::String (k.bpm) + " BPM",
                left.removeFromTop (18.f * sc), juce::Justification::centredLeft);
    g.setFont (sans (34.f * sc, true));
    g.drawText (U (k.name), left.removeFromTop (40.f * sc), juce::Justification::centredLeft);
    g.setFont (sans (14.f * sc));
    juce::String text = U (k.bio) + "\n\n";
    text << (juce::String (k.credits).isEmpty() ? U ("Créditos não divulgados nas páginas públicas do pack.") : U ("Trabalhou com: ") + U (k.credits));
    g.drawFittedText (text, left.toNearestInt(), juce::Justification::topLeft, 8, 1.f);
    a.removeFromLeft (16.f * sc);
    g.setFont (sans (13.f * sc, true));
    g.drawText (U ("Conteúdo do kit"), a.removeFromTop (20.f * sc), juce::Justification::centredLeft);
    g.setFont (sans (13.f * sc));
    const juce::String contents = U (k.contents);
    juce::String list = contents.isEmpty() ? U ("A lista de conteúdo deste kit não está publicada.") : contents.replace ("; ", "\n");
    list << "\n\n" << (k.style == mm::reggae ? U ("Groove na contagem de reggae: caixa/aro no 2 e no 4.\n") : juce::String())
         << U ("Saturador e Tape Echo começam em bypass. Pads nas notas 36–51.");
    g.setColour (juce::Colour (0xff333333));
    g.drawFittedText (list, a.toNearestInt(), juce::Justification::topLeft, 14, 1.f);
}

void MonstroEditor::resized()
{
    const float sc = getWidth() / 1000.f;
    auto R = [sc] (float x, float y, float w, float h) { return juce::Rectangle<float> (x, y, w, h).transformedBy (juce::AffineTransform::scale (sc)).toNearestInt(); };

    logoA.setBounds (R (690, 16, 140, 46));
    logoM.setBounds (R (840, 16, 140, 46));

    lcd.setBounds (R (26, 70, 490, 190));
    for (int i = 0; i < 6; ++i) fkeys[i].setBounds (R (26 + i * 82.f, 266, 78, 32));
    kitBox.setBounds (R (26, 306, 490, 32));
    juce::TextButton* tr[5] = { &recB, &playB, &stopB, &tapB, &undoB };
    for (int i = 0; i < 5; ++i) tr[i]->setBounds (R (26 + i * 98.f, 346, 94, 40));

    juce::Slider* ks[4] = { &bpmS, &swingS, &tuneS, &decS };
    juce::Label* kl[4] = { &bpmL, &swingL, &tuneL, &decL };
    for (int i = 0; i < 4; ++i) { kl[i]->setBounds (R (26 + i * 122.f, 394, 118, 16)); ks[i]->setBounds (R (26 + i * 122.f, 410, 118, 74)); }

    satB.setBounds (R (26, 494, 118, 28));
    satL.setBounds (R (148, 494, 60, 14)); satS.setBounds (R (148, 508, 60, 78));
    echoB.setBounds (R (230, 494, 118, 28));
    juce::Slider* es[3] = { &echoTS, &echoFS, &echoMS };
    juce::Label* el[3] = { &echoTL, &echoFL, &echoML };
    for (int i = 0; i < 3; ++i) { el[i]->setBounds (R (352 + i * 56.f, 494, 56, 14)); es[i]->setBounds (R (352 + i * 56.f, 508, 56, 78)); }

    for (int i = 0; i < 4; ++i) seqB[i].setBounds (R (530 + i * 64.f, 70, 60, 28));
    for (int i = 0; i < 16; ++i) pads[i]->setBounds (R (536 + (i % 4) * (438.f / 4), 110 + (float) (i / 4) * 95.f, 438.f / 4, 92));
    steps.setBounds (R (530, 500, 450, 40));
    midiDrag.setBounds (R (530, 548, 140, 32));
    saveMidiB.setBounds (R (676, 548, 96, 32));
    sampleB.setBounds (R (778, 548, 106, 32));
    clearSmpB.setBounds (R (888, 548, 92, 32));
    folderB.setBounds (R (790, 70, 190, 28));

    infoArea = R (10, 605, 980, 185);
}
