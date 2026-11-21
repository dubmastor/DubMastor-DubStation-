#include "DubChordsPage.h"

using namespace vintage;

namespace dcui
{
Theme Theme::make (int skin)
{
    Theme t;
    t.piano = (skin == 0);
    if (t.piano)
    {
        t.panelFill = juce::Colours::white.withAlpha (0.03f);
        t.panelLine = juce::Colour (0xff3a3a3e);
        t.ink = plasticText;            t.mutedInk = plasticMuted;
        t.accent = vintage::lcdBg;      t.accentInk = vintage::lcdInk;
        t.gridBg = juce::Colour (0xff0a0c0a);
        t.gridLine = juce::Colour (0xff223022);
        t.noteFill = vintage::lcdBg;          t.bassFill = juce::Colour (0xffe2b956);
        t.lcdBg = vintage::lcdBg;       t.lcdInk = vintage::lcdInk;
    }
    else
    {
        t.panelFill = ivory;
        t.panelLine = brownInk.withAlpha (0.25f);
        t.ink = brownInk;               t.mutedInk = brownInk.withAlpha (0.6f);
        t.accent = keyOrange;           t.accentInk = console;
        t.gridBg = console;
        t.gridLine = juce::Colour (0xff3a2e22);
        t.noteFill = ivory;             t.bassFill = juce::Colour (0xffd9a23a);
        t.lcdBg = knobBlack;            t.lcdInk = ivory;
    }
    return t;
}

static void drawGrip (juce::Graphics& g, juce::Point<float> c, juce::Colour col)
{
    g.setColour (col);
    for (int r = -1; r <= 1; ++r)
        for (int k = 0; k < 2; ++k)
            g.fillEllipse (c.x - 3.5f + (float) k * 5.0f, c.y - 1.5f + (float) r * 5.0f, 3.0f, 3.0f);
}

//==============================================================================  arrastar MIDI
void MidiDragChip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool over = isMouseOver();
    g.setColour (theme.lcdBg.withAlpha (theme.piano ? (over ? 0.22f : 0.12f) : (over ? 1.0f : 0.92f)));
    g.fillRoundedRectangle (r, 4.0f);
    juce::Path outline, dashed;
    outline.addRoundedRectangle (r.reduced (2.0f), 3.0f);
    const float dashes[] = { 5.0f, 3.5f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, outline, dashes, 2);
    const auto ink = theme.piano ? theme.accent : theme.accent.brighter (0.15f);
    g.setColour (ink.withAlpha (over ? 1.0f : 0.75f));
    g.fillPath (dashed);
    drawGrip (g, { r.getX() + 14.0f, r.getCentreY() }, ink);
    g.setColour (theme.piano ? theme.ink : ivory);
    g.setFont (VintageLookAndFeel::mono (11.0f));
    g.drawText (label, r.withTrimmedLeft (22.0f), juce::Justification::centred);
}

void MidiDragChip::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 4) return;
    dragging = true;
    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (stem + ".mid");
    if (proc.writeDcMidiFile (file, part, slot))
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (file.getFullPathName()), false, this);
}

//==============================================================================  grade
void PianoRollGrid::refresh()
{
    juce::String k;
    for (int i = 0; i < RootsKeysProcessor::dcSlots; ++i)
    {
        const auto s = proc.getDcSlot (i);
        k << s.root << ':' << s.quality << ':' << s.beats << ':' << (int) proc.isDcSlotOn (i) << '|';
    }
    const auto st = proc.getDcSettings (proc.apvts.getRawParameterValue ("dcBass")->load() > 0.5f);
    k << st.pattern << '|' << st.centerNote << '|' << st.velocity << '|' << st.gate << '|' << st.swing << '|' << (int) st.bass;
    bool changed = false;
    if (k != key)
    {
        key = k;
        slots = proc.getDcSlots();
        events = dub::buildLoop (slots, st, loopBeats);
        changed = true;
    }
    const double pp = proc.dcPlayPos.load();
    if (changed || std::abs (pp - playPos) > 0.005)
    {
        playPos = pp;
        repaint();
    }
}

void PianoRollGrid::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (theme.gridBg);
    g.fillRoundedRectangle (r, 5.0f);
    auto area = r.reduced (6.0f);
    auto band = area.removeFromTop (18.0f);
    area.removeFromTop (3.0f);

    if (events.empty() || loopBeats <= 0.0)
    {
        g.setColour (theme.piano ? plasticMuted : ivoryDark);
        g.setFont (VintageLookAndFeel::mono (12.0f));
        g.drawText ("LIGUE PELO MENOS UM ACORDE", area, juce::Justification::centred);
        return;
    }

    int lo = 127, hi = 0;
    for (const auto& e : events) { lo = std::min (lo, e.note); hi = std::max (hi, e.note); }
    lo -= 2; hi += 2;
    const int rows = hi - lo + 1;
    const float rowH = area.getHeight() / (float) rows;
    auto xAt = [&] (double beat) { return area.getX() + (float) (beat / loopBeats) * area.getWidth(); };
    auto yAt = [&] (int note) { return area.getBottom() - (float) (note - lo + 1) * rowH; };

    // linhas das notas C (referência de oitava)
    for (int nn = lo; nn <= hi; ++nn)
        if (nn % 12 == 0)
        {
            g.setColour (theme.gridLine.withAlpha (0.6f));
            g.fillRect (area.getX(), yAt (nn), area.getWidth(), rowH);
        }

    // tempos e compassos
    for (int b = 0; b <= (int) loopBeats; ++b)
    {
        const bool bar = (b % 4 == 0);
        g.setColour (theme.gridLine.brighter (bar ? 0.6f : 0.0f).withAlpha (bar ? 1.0f : 0.7f));
        g.drawVerticalLine ((int) xAt (b), area.getY(), area.getBottom());
        if (bar && b < (int) loopBeats)
        {
            g.setColour (theme.piano ? plasticMuted : ivoryDark.withAlpha (0.7f));
            g.setFont (VintageLookAndFeel::mono (9.0f, false));
            g.drawText (juce::String (b / 4 + 1), juce::Rectangle<float> (xAt (b) + 3.0f, area.getY() + 1.0f, 24.0f, 10.0f),
                        juce::Justification::centredLeft);
        }
    }

    // faixa com o nome dos acordes
    double pos = 0;
    for (const auto& s : slots)
    {
        auto seg = juce::Rectangle<float> (xAt (pos), band.getY(), xAt (pos + s.beats) - xAt (pos), band.getHeight()).reduced (1.0f, 0.0f);
        g.setColour (theme.accent.withAlpha (theme.piano ? 0.85f : 0.9f));
        g.fillRoundedRectangle (seg, 3.0f);
        g.setColour (theme.accentInk);
        g.setFont (VintageLookAndFeel::mono (11.0f));
        g.drawText (juce::String (dub::chordLabel (s)), seg, juce::Justification::centred);
        pos += s.beats;
    }

    // notas
    for (const auto& e : events)
    {
        auto nr = juce::Rectangle<float> (xAt (e.start), yAt (e.note), std::max (2.5f, xAt (e.start + e.length) - xAt (e.start)), rowH);
        const auto col = (e.channel == 2 ? theme.bassFill : theme.noteFill).withAlpha (0.55f + 0.45f * (float) e.velocity / 127.0f);
        g.setColour (col);
        g.fillRoundedRectangle (nr.reduced (0.5f, std::min (1.0f, rowH * 0.15f)), 1.5f);
    }

    // cursor de reprodução
    if (playPos >= 0.0)
    {
        const float x = xAt (playPos);
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawLine (x, band.getY(), x, area.getBottom(), 1.5f);
    }

    // dica de arrastar
    g.setColour ((theme.piano ? plasticMuted : ivoryDark).withAlpha (0.8f));
    g.setFont (VintageLookAndFeel::mono (9.0f, false));
    auto hint = area.removeFromBottom (12.0f).removeFromRight (230.0f);
    g.drawText ("ARRASTE A GRADE PARA A DAW", hint, juce::Justification::centredRight);
    drawGrip (g, { hint.getRight() - 160.0f, hint.getCentreY() }, (theme.piano ? plasticMuted : ivoryDark).withAlpha (0.8f));
}

void PianoRollGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 4) return;
    dragging = true;
    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Roots Keys riddim.mid");
    if (proc.writeDcMidiFile (file, 0))
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (file.getFullPathName()), false, this);
}

//==============================================================================  cartão de acorde
ChordCard::ChordCard (RootsKeysProcessor& p, int i)
    : proc (p), index (i),
      drag (p, 0, i, "ARRASTE O ACORDE", "Roots Keys acorde " + juce::String (i + 1))
{
    auto& s = proc.apvts;
    const auto n = juce::String (i);
    for (int k = 0; k < 12; ++k)
    {
        notes[k].setButtonText (dub::rootName (k));
        notes[k].getProperties().set ("style", "chip");
        notes[k].onClick = [this, k] { if (rootAtt) rootAtt->setValueAsCompleteGesture ((float) k); };
        addAndMakeVisible (notes[k]);
    }
    rootAtt = std::make_unique<juce::ParameterAttachment> (*s.getParameter ("dcRoot" + n),
                                                           [this] (float v) { root = juce::roundToInt (v); refresh(); });
    rootAtt->sendInitialUpdate();

    qualBox.addItemList ({ "Maior", "menor", "7", "m7", "maj7", "m6", "sus2", "sus4", "dim", "m9" }, 1);
    durBox.addItemList ({ "1/2 comp.", "1 comp.", "2 comp.", "3 comp.", "4 comp.", "8 comp." }, 1);
    addAndMakeVisible (qualBox);
    addAndMakeVisible (durBox);
    qualAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "dcQual" + n, qualBox);
    durAtt  = std::make_unique<APVTS::ComboBoxAttachment> (s, "dcDur" + n, durBox);
    if (i > 0)
    {
        addAndMakeVisible (onButton);
        onAtt = std::make_unique<APVTS::ButtonAttachment> (s, "dcAct" + n, onButton);
    }
    addAndMakeVisible (drag);
}

ChordCard::~ChordCard() = default;

void ChordCard::setTheme (const Theme& t)
{
    theme = t;
    onButton.getProperties().set ("style", t.piano ? "led" : "rocker");
    drag.setTheme (t);
    repaint();
}

void ChordCard::refresh()
{
    for (int k = 0; k < 12; ++k)
        notes[k].setToggleState (k == root, juce::dontSendNotification);
    const auto s = proc.getDcSlot (index);
    const auto lbl = juce::String (dub::chordLabel (s));
    const bool on = proc.isDcSlotOn (index);
    if (lbl != shownLabel || on != shownOn)
    {
        shownLabel = lbl;
        shownOn = on;
        repaint();
    }
}

void ChordCard::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    if (! theme.piano)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (r.translated (0, 2.0f), 5.0f);
    }
    g.setColour (theme.panelFill);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (theme.panelLine);
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);

    g.setColour (theme.ink);
    g.setFont (VintageLookAndFeel::mono (11.0f));
    g.drawText ("ACORDE " + juce::String (index + 1), 12, 8, 100, 14, juce::Justification::centredLeft);

    auto lcdR = juce::Rectangle<float> (10.0f, 30.0f, r.getWidth() - 20.0f, 40.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (lcdR, 3.0f);
    g.setColour (theme.lcdBg);
    g.fillRoundedRectangle (lcdR.reduced (2.5f), 2.0f);
    g.setColour (theme.lcdInk);
    g.setFont (VintageLookAndFeel::mono (22.0f));
    g.drawText (shownLabel, lcdR.reduced (10.0f, 0.0f), juce::Justification::centredLeft);
    g.setFont (VintageLookAndFeel::mono (10.0f, false));
    g.drawText (dub::durationName ((int) proc.apvts.getRawParameterValue ("dcDur" + juce::String (index))->load()),
                lcdR.reduced (8.0f, 0.0f), juce::Justification::centredRight);
}

void ChordCard::paintOverChildren (juce::Graphics& g)
{
    if (shownOn) return;
    auto r = getLocalBounds().toFloat().withTrimmedTop (28.0f);
    g.setColour ((theme.piano ? juce::Colour (0xff141416) : ivory).withAlpha (0.72f));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (theme.ink.withAlpha (0.8f));
    g.setFont (VintageLookAndFeel::mono (12.0f));
    g.drawText ("DESLIGADO", r, juce::Justification::centred);
}

void ChordCard::resized()
{
    const int w = getWidth();
    onButton.setBounds (w - 70, 4, 62, 22);
    const int bw = (w - 20) / 6;
    for (int k = 0; k < 12; ++k)
        notes[k].setBounds (10 + (k % 6) * bw, 76 + (k / 6) * 22, bw - 2, 20);
    qualBox.setBounds (10, 124, (w - 20) / 2 - 2, 24);
    durBox.setBounds (10 + (w - 20) / 2 + 2, 124, (w - 20) / 2 - 2, 24);
    drag.setBounds (10, 156, w - 20, 38);
}
} // namespace dcui

//==============================================================================  página
DubChordsPage::DubChordsPage (RootsKeysProcessor& p, VintageLookAndFeel& l)
    : proc (p), lnf (l),
      dragAll   (p, 0, -1, "TUDO",  "Roots Keys riddim"),
      dragSkank (p, 1, -1, "SKANK", "Roots Keys skank"),
      dragBass  (p, 2, -1, "BAIXO", "Roots Keys baixo guia"),
      grid (p)
{
    auto& s = proc.apvts;

    riddimBox.setTextWhenNothingSelected ("RIDDIMS PRONTOS...");
    int id = 1;
    for (const auto& r : dub::riddimPresets()) riddimBox.addItem (r.name, id++);
    riddimBox.onChange = [this]
    {
        const int idx = riddimBox.getSelectedId() - 1;
        if (idx >= 0)
        {
            proc.applyRiddim (idx);
            statusText = "Riddim carregado: " + riddimBox.getItemText (idx);
            riddimBox.setSelectedId (0, juce::dontSendNotification);
            repaint();
        }
    };
    addAndMakeVisible (riddimBox);

    for (auto* b : { &previewBtn, &dawBtn, &bassBtn }) addAndMakeVisible (b);
    previewAtt = std::make_unique<APVTS::ButtonAttachment> (s, "dcPreview", previewBtn);
    dawAtt     = std::make_unique<APVTS::ButtonAttachment> (s, "dcOn", dawBtn);
    bassAtt    = std::make_unique<APVTS::ButtonAttachment> (s, "dcBass", bassBtn);

    setupKnob (bpm,   "dcBpm",      "BPM");
    setupKnob (reg,   "dcRegister", "Registro");
    setupKnob (vel,   "dcVel",      "Velocity");
    setupKnob (gate,  "dcGate",     "Gate");
    setupKnob (swing, "dcSwing",    "Swing");

    patternBox.addItem ("Auto (segue o instrumento)", 1);
    for (int pt = 0; pt < dub::NumPatterns; ++pt) patternBox.addItem (dub::patternName (pt), pt + 2);
    addAndMakeVisible (patternBox);
    patternAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "dcPattern", patternBox);
    repsBox.addItemList ({ "x1", "x2", "x4", "x8" }, 1);
    addAndMakeVisible (repsBox);
    repsAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "dcReps", repsBox);

    saveBtn.getProperties().set ("style", "chip");
    saveBtn.onClick = [this] { saveMidi(); };
    addAndMakeVisible (saveBtn);

    for (int i = 0; i < RootsKeysProcessor::dcSlots; ++i)
    {
        cards[i] = std::make_unique<dcui::ChordCard> (proc, i);
        addAndMakeVisible (*cards[i]);
    }
    for (auto* c : { &dragAll, &dragSkank, &dragBass }) addAndMakeVisible (c);
    addAndMakeVisible (grid);
    setSkin (0);
}

DubChordsPage::~DubChordsPage() = default;

void DubChordsPage::setupKnob (Knob& k, const juce::String& id, const juce::String& text)
{
    k.slider.setPopupDisplayEnabled (true, true, nullptr);
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<APVTS::SliderAttachment> (proc.apvts, id, k.slider);
    k.label.setText (text.toUpperCase(), juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (VintageLookAndFeel::mono (10.0f));
    k.label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k.label);
}

void DubChordsPage::placeKnob (Knob& k, juce::Point<int> c, int size)
{
    k.slider.setBounds (juce::Rectangle<int> (size, size).withCentre (c));
    k.label.setBounds (juce::Rectangle<int> (90, 14).withCentre ({ c.x, c.y + size / 2 + 9 }));
}

void DubChordsPage::setSkin (int skin)
{
    theme = dcui::Theme::make (skin);
    for (auto* b : { &previewBtn, &dawBtn, &bassBtn })
        b->getProperties().set ("style", theme.piano ? "led" : "rocker");
    for (auto* k : { &bpm, &reg, &vel, &gate, &swing })
    {
        k->slider.setColour (juce::Slider::rotarySliderOutlineColourId, theme.piano ? plasticMuted : brownInk);
        k->label.setColour (juce::Label::textColourId, theme.ink);
    }
    for (auto& c : cards) c->setTheme (theme);
    for (auto* c : { &dragAll, &dragSkank, &dragBass }) c->setTheme (theme);
    grid.setTheme (theme);
    repaint();
}

void DubChordsPage::refresh()
{
    for (auto& c : cards) c->refresh();
    grid.refresh();
    repaint (juce::Rectangle<int> (740, 8, 200, 40));   // visor de BPM
}

void DubChordsPage::saveMidi()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Roots Keys");
    dir.createDirectory();
    chooser = std::make_unique<juce::FileChooser> ("Salvar o riddim como MIDI", dir.getChildFile ("Roots Keys riddim.mid"), "*.mid");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              if (! f.hasFileExtension ("mid")) f = f.withFileExtension ("mid");
                              statusText = proc.writeDcMidiFile (f, 0) ? "Salvo: " + f.getFileName() : juce::String ("Nada para salvar");
                              repaint();
                          });
}

void DubChordsPage::paint (juce::Graphics& g)
{
    auto panel = [&] (juce::Rectangle<int> r)
    {
        if (! theme.piano)
        {
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.fillRoundedRectangle (r.toFloat().translated (0, 2.0f), 5.0f);
        }
        g.setColour (theme.panelFill);
        g.fillRoundedRectangle (r.toFloat(), 5.0f);
        g.setColour (theme.panelLine);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 5.0f, 1.0f);
    };
    panel ({ 0, 0, 952, 56 });
    panel ({ 760, 64, 192, 204 });
    panel ({ 0, 276, 952, 262 });

    g.setColour (theme.ink);
    g.setFont (VintageLookAndFeel::mono (14.0f));
    g.drawText ("DUB CHORDS", 14, 8, 130, 20, juce::Justification::centredLeft);
    g.setColour (theme.mutedInk);
    g.setFont (VintageLookAndFeel::mono (9.0f, false));
    g.drawText ("HARMONIA PARA RIDDIM", 14, 28, 140, 14, juce::Justification::centredLeft);

    g.setColour (theme.ink);
    g.setFont (VintageLookAndFeel::mono (11.0f));
    g.drawText ("RITMO", 772, 70, 100, 14, juce::Justification::centredLeft);
    g.drawText ("GRADE", 14, 286, 60, 14, juce::Justification::centredLeft);
    g.setColour (theme.mutedInk);
    g.setFont (VintageLookAndFeel::mono (9.0f, false));
    g.drawText ("ARRASTE:", 14, 300, 60, 12, juce::Justification::centredLeft);
    g.drawText ("REPETIR", 598, 284, 60, 10, juce::Justification::centredLeft);

    // visor de BPM / transporte
    auto bpmR = juce::Rectangle<float> (748.0f, 12.0f, 190.0f, 32.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (bpmR, 3.0f);
    g.setColour (theme.lcdBg);
    g.fillRoundedRectangle (bpmR.reduced (2.5f), 2.0f);
    g.setColour (theme.lcdInk);
    g.setFont (VintageLookAndFeel::mono (13.0f));
    const bool host = proc.dcHostPlaying.load() && proc.apvts.getRawParameterValue ("dcOn")->load() > 0.5f;
    const double hb = proc.dcHostBpm.load();
    juce::String txt;
    if (host)                                                                  txt = "DAW  " + juce::String (hb, 1) + " BPM";
    else if (proc.apvts.getRawParameterValue ("dcPreview")->load() > 0.5f)    txt = "PREVIEW  " + juce::String ((int) proc.apvts.getRawParameterValue ("dcBpm")->load()) + " BPM";
    else                                                                       txt = hb > 0.0 ? "PARADO  " + juce::String (hb, 1) + " BPM" : juce::String ("PARADO");
    g.drawText (txt, bpmR.reduced (8.0f, 0.0f), juce::Justification::centred);

    if (statusText.isNotEmpty())
    {
        g.setColour (theme.mutedInk);
        g.setFont (VintageLookAndFeel::mono (10.0f, false));
        g.drawFittedText (statusText, 800, 282, 140, 32, juce::Justification::centredRight, 2);
    }
}

void DubChordsPage::resized()
{
    // faixa superior: riddims, preview, DAW, BPM
    riddimBox.setBounds (156, 14, 250, 28);
    previewBtn.setBounds (418, 14, theme.piano ? 104 : 104, 28);
    dawBtn.setBounds (530, 14, 160, 28);
    placeKnob (bpm, { 715, 22 }, 30);
    bpm.label.setBounds (0, 0, 0, 0);   // o visor ao lado já diz "BPM"

    for (int i = 0; i < RootsKeysProcessor::dcSlots; ++i)
        cards[i]->setBounds (i * 190, 64, 182, 204);

    // ritmo
    patternBox.setBounds (770, 90, 172, 24);
    placeKnob (reg,   { 812, 150 }, 38);
    placeKnob (vel,   { 900, 150 }, 38);
    placeKnob (gate,  { 812, 222 }, 38);
    placeKnob (swing, { 900, 222 }, 38);

    // grade + barra de ferramentas
    dragAll.setBounds   (80, 282, 110, 30);
    dragSkank.setBounds (196, 282, 110, 30);
    dragBass.setBounds  (312, 282, 110, 30);
    bassBtn.setBounds   (434, 284, 150, 26);
    repsBox.setBounds   (598, 294, 64, 22);
    saveBtn.setBounds   (674, 284, 120, 28);
    grid.setBounds (10, 320, 932, 208);
}
