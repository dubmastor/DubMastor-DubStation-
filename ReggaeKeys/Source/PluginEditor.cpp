#include "PluginEditor.h"
#include "SkinTextures.h"
#include "BinaryData.h"

class DubChordsProcessor;
std::unique_ptr<juce::Component> makeChordMachinePanel (DubChordsProcessor&);   // Source/MachineEditorUnit.cpp

using namespace vintage;

namespace
{
constexpr int kW = 1000, kH = 800;

// cores das drawbars no padrão Hammond: marrom (sub), marfim (oitavas), preta (quintas/terças)
juce::Colour drawbarColour (int i)
{
    switch (i)
    {
        case 0: case 1: return juce::Colour (0xff6b3418);
        case 4: case 6: case 7: return juce::Colour (0xff141414);
        default: return ivory;
    }
}

juce::String versionText()
{
    auto parts = juce::StringArray::fromTokens (JucePlugin_VersionString, ".", "");
    return "v" + parts[0] + "." + (parts.size() > 1 ? parts[1] : juce::String ("0"));
}

const char* kPianoPresetShort[4] = { "M1 CLASSIC", "DUB SKANK", "BRIGHT HOUSE", "SOFT BALLAD" };
const char* kOrganPresetShort[4] = { "BUBBLE", "PERC", "DUB DARK", "GOSPEL" };

// ----- layout fixo (1000 x 724)
const juce::Rectangle<int> kPlate      { 415, 16, 170, 40 };   // centralizada no cabeçalho
const juce::Rectangle<int> kEcho       { 24, 372, 616, 120 };
const juce::Rectangle<int> kFlanger    { 24, 504, 616, 120 };
const juce::Rectangle<int> kOut        { 652, 372, 324, 252 };
const juce::Rectangle<int> kKeysBed    { 24, 636, 952, 152 };
// piano
const juce::Rectangle<int> kSpeakerL   { 24, 86, 186, 274 };
const juce::Rectangle<int> kSpeakerR   { 790, 86, 186, 274 };
const juce::Rectangle<int> kPianoPanel { 222, 86, 556, 274 };
const juce::Rectangle<int> kLcd        { 276, 96, 448, 64 };
// órgão
const juce::Rectangle<int> kConsole    { 210, 94, 580, 258 };
const juce::Rectangle<int> kLeslie     { 40, 100, 150, 110 };
const juce::Rectangle<int> kPerc       { 810, 100, 150, 132 };
} // namespace

// =====================================================================  Drawbar
void DrawbarComponent::paint (juce::Graphics& g)
{
    const float cx = (float) getWidth() * 0.5f;
    g.setColour (ivoryDark);
    g.setFont (VintageLookAndFeel::mono (11.0f));
    g.drawText (name, 0, 0, getWidth(), 14, juce::Justification::centred);

    const float top = 18.0f;
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (cx - 9.0f, top, 18.0f, 8.0f, 2.0f);

    const float len = (float) value * step();
    if (value > 0)
    {
        g.setColour (ivoryDark);
        g.fillRect (cx - 7.0f, top + 4.0f, 14.0f, len);
        g.setColour (brownInk);
        g.setFont (VintageLookAndFeel::mono (10.0f));
        for (int n = 1; n <= value; ++n)
            g.drawText (juce::String (n), juce::Rectangle<float> (cx - 7.0f, top + 4.0f + (float) (n - 1) * step(), 14.0f, step()),
                        juce::Justification::centred);
    }
    auto tab = juce::Rectangle<float> (cx - 18.0f, top + 4.0f + len, 36.0f, 26.0f);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (tab.translated (0, 2.0f), 5.0f);
    g.setColour (tabColour);
    g.fillRoundedRectangle (tab, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillRoundedRectangle (tab.reduced (4.0f, 0).withHeight (3.0f).translated (0, 3.0f), 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (tab, 5.0f, 1.0f);
}

void DrawbarComponent::mouseDown (const juce::MouseEvent&)
{
    dragStart = value;
    attachment.beginGesture();
}

void DrawbarComponent::mouseDrag (const juce::MouseEvent& e)
{
    const int v = juce::jlimit (0, 8, dragStart + juce::roundToInt ((float) e.getDistanceFromDragStartY() / step()));
    if (v != value) attachment.setValueAsPartOfGesture ((float) v);
}

void DrawbarComponent::mouseUp (const juce::MouseEvent&) { attachment.endGesture(); }

void DrawbarComponent::mouseDoubleClick (const juce::MouseEvent&)
{
    attachment.setValueAsCompleteGesture (value == 0 ? 8.0f : 0.0f);
}

// =====================================================================  LCD
void LcdDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (r, 4.0f);
    auto lcdR = r.reduced (8.0f, 7.0f);
    g.setColour (lcdBg);
    g.fillRoundedRectangle (lcdR, 2.0f);
    g.setColour (juce::Colour (0xff7fa95c).withAlpha (0.35f));
    for (float x = lcdR.getX(); x < lcdR.getRight(); x += 3.0f)
        g.drawVerticalLine ((int) x, lcdR.getY(), lcdR.getBottom());
    g.setColour (lcdInk.withAlpha (0.06f));   // "fantasma" dos segmentos apagados
    g.fillRect (lcdR.reduced (8.0f, 6.0f).withHeight (20.0f));
    g.setColour (lcdInk);
    g.setFont (VintageLookAndFeel::mono (18.0f));
    g.drawText (line1, lcdR.reduced (12.0f, 6.0f).withHeight (22.0f), juce::Justification::centredLeft);
    g.setFont (VintageLookAndFeel::mono (14.0f, false));
    g.drawText (line2, lcdR.reduced (12.0f, 6.0f).withTrimmedTop (26.0f), juce::Justification::centredLeft);
}

// =====================================================================  Rodas
void WheelsComponent::paint (juce::Graphics& g)
{
    const float vals[2] = { 0.5f + 0.5f * pitchV, modV };
    const char* names[2] = { "PITCH", "MOD" };
    for (int i = 0; i < 2; ++i)
    {
        const float x = (float) i * 44.0f;
        auto slot = juce::Rectangle<float> (x, 0.0f, 30.0f, (float) getHeight() - 20.0f);
        g.setColour (juce::Colour (0xff030303));
        g.fillRoundedRectangle (slot, 4.0f);
        auto wheel = slot.reduced (6.0f, 8.0f);
        g.setColour (juce::Colour (0xff2a2a2c));
        g.fillRoundedRectangle (wheel, 6.0f);
        g.setColour (juce::Colour (0xff141416));
        for (float y = wheel.getY() + 4.0f; y < wheel.getBottom(); y += 8.0f)
            g.drawHorizontalLine ((int) y, wheel.getX() + 1, wheel.getRight() - 1);
        const float my = wheel.getBottom() - 6.0f - vals[i] * (wheel.getHeight() - 12.0f);
        g.setColour (juce::Colour (0xffe8e8e8));
        g.fillRect (wheel.getX(), my, wheel.getWidth(), 6.0f);
        g.setColour (plasticMuted);
        g.setFont (VintageLookAndFeel::mono (9.0f));
        g.drawText (names[i], juce::Rectangle<float> (x - 6.0f, (float) getHeight() - 16.0f, 42.0f, 14.0f), juce::Justification::centred);
    }
}

// =====================================================================  Editor
RootsKeysEditor::RootsKeysEditor (RootsKeysProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    plateFace  = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoBlackRegular_ttf,
                                                          (size_t) BinaryData::ArchivoBlackRegular_ttfSize);
    pianoTitleFace = juce::Typeface::createSystemTypefaceFor (BinaryData::MichromaRegular_ttf,
                                                              (size_t) BinaryData::MichromaRegular_ttfSize);
    organTitleFace = juce::Typeface::createSystemTypefaceFor (BinaryData::RootsKeysDisplayBoldItalic_ttf,
                                                              (size_t) BinaryData::RootsKeysDisplayBoldItalic_ttfSize);
    lnf.setSkin (0);
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;

    // modo (troca o visual)
    for (auto* b : { &modePiano, &modeOrgan })
    {
        b->setClickingTogglesState (false);
        addAndMakeVisible (b);
    }
    modePiano.onClick = [this] { setModeParam (0); };
    for (auto* t : { &tabInstrument, &tabChords })
    {
        t->setClickingTogglesState (false);
        t->getProperties().set ("style", "chip");
        addAndMakeVisible (t);
    }
    tabInstrument.onClick = [this] { setPage (0); };
    tabChords.onClick     = [this] { setPage (1); };
    machinePanel = makeChordMachinePanel (proc.getMachine());
    addChildComponent (*machinePanel);
    addChildComponent (machineOnBtn);
    machineOnAtt = std::make_unique<APVTS::ButtonAttachment> (s, "machineOn", machineOnBtn);
    modeOrgan.onClick = [this] { setModeParam (1); };

    // presets: 4 de piano (botões com LED) e 4 de órgão (teclas invertidas)
    for (int i = 0; i < 4; ++i)
    {
        pianoPreset[i].setButtonText ("P0" + juce::String (i + 1));
        pianoPreset[i].getProperties().set ("style", "membrane");
        pianoPreset[i].onClick = [this, i] { proc.setCurrentProgram (i); };
        addChildComponent (pianoPreset[i]);

        organPreset[i].setButtonText (kOrganPresetShort[i]);
        organPreset[i].getProperties().set ("style", "reverseKey");
        organPreset[i].onClick = [this, i] { proc.setCurrentProgram (RootsKeysProcessor::getNumPianoPresets() + i); };
        addChildComponent (organPreset[i]);
    }

    // piano: timbres
    {
        const char* names[kNumTimbres] = { "M1", "GRAND", "MT-40", "FM EP", "CP-80", "C7", "M1 GRAND", "M1 COMP", "HONKY", "CZ-101", "RHODES", "SUITCASE", "WURLI" };
        for (int i = 0; i < kNumTimbres; ++i)
        {
            pianoTimbre[i].setButtonText (names[i]);
            pianoTimbre[i].getProperties().set ("style", "chip");
            pianoTimbre[i].setClickingTogglesState (false);
            pianoTimbre[i].onClick = [this, i]
            {
                if (auto* prm = proc.apvts.getParameter ("pianoType"))
                    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
            };
            addChildComponent (pianoTimbre[i]);
        }
    }
    wheels.onMove = [this] (int w, float v) { proc.setWheelFromUI (w, v); };

    // piano
    addChildComponent (lcd);
    setupKnob (faderBright, "pianoBright", "Brilho", true);
    setupKnob (faderDecay,  "pianoDecay",  "Decay",  true);
    setupKnob (faderChorus, "pianoChorus", "Chorus", true);
    setupKnob (faderLevel,  "pianoLevel",  "Volume", true);
    addChildComponent (wheels);

    // órgão
    for (int i = 0; i < 9; ++i)
    {
        auto* prm = s.getParameter ("db" + juce::String (i + 1));
        drawbars[i] = std::make_unique<DrawbarComponent> (*prm, RootsKeysProcessor::drawbarNames()[i],
                                                          drawbarColour (i));
        addChildComponent (*drawbars[i]);
    }
    leslieSwitch.getProperties().set ("style", "halfmoon");
    for (auto* t : { &percOn, &perc3rd, &percSlow }) t->getProperties().set ("style", "tab");
    for (auto* b : { &leslieSwitch, &percOn, &perc3rd, &percSlow }) addChildComponent (b);
    leslieAtt   = std::make_unique<APVTS::ButtonAttachment> (s, "leslieFast", leslieSwitch);
    percOnAtt   = std::make_unique<APVTS::ButtonAttachment> (s, "percOn", percOn);
    perc3rdAtt  = std::make_unique<APVTS::ButtonAttachment> (s, "percHarm", perc3rd);    // ligado = 3º harmônico
    percSlowAtt = std::make_unique<APVTS::ButtonAttachment> (s, "percDecay", percSlow);  // ligado = decaimento lento
    setupKnob (oLeslieMix, "leslieMix",  "Leslie Mix");
    setupKnob (oDrive,     "drive",      "Drive");
    setupKnob (oClick,     "click",      "Click");
    setupKnob (oLevel,     "organLevel", "Organ Vol");

    // GTR SKANK: a guitarra do skank (timbre do MRG-68) tocando junto com o piano — sempre começa desligada
    gtrButton.getProperties().set ("style", "led");
    gtrButton.setTooltip ("Guitarra do skank junto com o piano (timbre do Mastor Reggae Guitar MRG-68)");
    addChildComponent (gtrButton);
    gtrAtt = std::make_unique<APVTS::ButtonAttachment> (s, "gtrOn", gtrButton);

    // eco de fita FitaEco TE-12
    addAndMakeVisible (echoButton);
    echoAtt = std::make_unique<APVTS::ButtonAttachment> (s, "echoOn", echoButton);
    for (int i = 1; i <= 12; ++i) echoModeBox.addItem ("MODE " + juce::String (i), i);
    addAndMakeVisible (echoModeBox);
    echoModeAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "echoMode", echoModeBox);
    setupKnob (eRate,      "echoRate",      "Rate");
    setupKnob (eIntensity, "echoIntensity", "Intensity");
    setupKnob (eVol,       "echoVol",       "Echo Vol");
    setupKnob (eSpring,    "echoSpring",    "Spring");
    setupKnob (eWow,       "echoWow",       "Wow");
    setupKnob (eAge,       "echoAge",       "Tape Age");

    // flanger estilo dub (Mad Professor)
    addAndMakeVisible (flButton);
    flAtt = std::make_unique<APVTS::ButtonAttachment> (s, "flOn", flButton);
    setupKnob (fRate,     "flRate",     "Rate");
    setupKnob (fDepth,    "flDepth",    "Depth");
    setupKnob (fManual,   "flManual",   "Manual");
    setupKnob (fFeedback, "flFeedback", "Feedback");
    setupKnob (fMix,      "flMix",      "Mix");

    // maximizer + VU
    addAndMakeVisible (maxButton);
    maxAtt = std::make_unique<APVTS::ButtonAttachment> (s, "maxOn", maxButton);
    setupKnob (mxGain,  "maxGain",    "Gain");
    setupKnob (mxCeil,  "maxCeiling", "Ceil");
    setupKnob (mMaster, "master",     "Master");
    setupKnob (mReverb, "reverb",     "Reverb");
    grLabel.setFont (VintageLookAndFeel::mono (11.0f));
    grLabel.setJustificationType (juce::Justification::centred);
    grLabel.setText ("GR 0.0", juce::dontSendNotification);
    addAndMakeVisible (grLabel);
    addAndMakeVisible (vuL);
    addAndMakeVisible (vuR);

    // teclado: as teclas acendem conforme o MIDI tocado
    keyboard.setAvailableRange (36, 96);   // C2..C7 (61 teclas)
    keyboard.setScrollButtonsVisible (false);
    keyboard.setOctaveForMiddleC (4);      // nota 60 = C4
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    setSize (kW, kH);
    applySkin ((int) s.getRawParameterValue ("mode")->load());
    startTimerHz (30);
}

RootsKeysEditor::~RootsKeysEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void RootsKeysEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& text, bool fader)
{
    if (fader) k.slider.setSliderStyle (juce::Slider::LinearVertical);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setPopupDisplayEnabled (true, true, this);
    addChildComponent (k.slider);
    k.att = std::make_unique<APVTS::SliderAttachment> (proc.apvts, paramId, k.slider);
    k.label.setText (text.toUpperCase(), juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (VintageLookAndFeel::mono (10.0f));
    k.label.setInterceptsMouseClicks (false, false);
    addChildComponent (k.label);
}

void RootsKeysEditor::placeKnob (Knob& k, juce::Point<int> c, int size, int labelW)
{
    k.slider.setBounds (juce::Rectangle<int> (size, size).withCentre (c));
    k.label.setBounds (juce::Rectangle<int> (labelW, 14).withCentre ({ c.x, c.y + size / 2 + 9 }));
}

void RootsKeysEditor::setModeParam (int m)
{
    if (auto* prm = proc.apvts.getParameter ("mode"))
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) m));
    applySkin (m);
}

void RootsKeysEditor::applySkin (int newSkin)
{
    newSkin = juce::jlimit (0, 1, newSkin);
    if (newSkin == skin) return;
    skin = newSkin;
    const bool piano = (skin == 0);
    lnf.setSkin (skin);

    modePiano.setToggleState (piano, juce::dontSendNotification);
    modeOrgan.setToggleState (! piano, juce::dontSendNotification);
    for (auto* b : { &modePiano, &modeOrgan }) b->getProperties().set ("style", piano ? "membrane" : "pill");

    // cores que dependem do visual
    const auto tickOnPanel  = piano ? plasticMuted : brownInk;   // knobs sobre painel (marfim no órgão)
    const auto tickOnWood   = ivory;
    const auto labelOnPanel = piano ? plasticText : brownInk;
    for (auto* k : { &eRate, &eIntensity, &eVol, &eSpring, &eWow, &eAge, &fRate, &fDepth, &fManual, &fFeedback, &fMix,
                      &mxGain, &mxCeil, &mMaster, &mReverb })
    {
        k->slider.setColour (juce::Slider::rotarySliderOutlineColourId, tickOnPanel);
        k->label.setColour (juce::Label::textColourId, labelOnPanel);
    }
    for (auto* k : { &oLeslieMix, &oDrive, &oClick, &oLevel })
    {
        k->slider.setColour (juce::Slider::rotarySliderOutlineColourId, tickOnWood);
        k->label.setColour (juce::Label::textColourId, ivory);
    }
    for (auto* k : { &faderBright, &faderDecay, &faderChorus, &faderLevel })
        k->label.setColour (juce::Label::textColourId, plasticText);

    echoButton.getProperties().set ("style", piano ? "led" : "rocker");
    flButton.getProperties().set ("style", piano ? "led" : "rocker");
    maxButton.getProperties().set ("style", piano ? "led" : "rocker");
    grLabel.setColour (juce::Label::backgroundColourId, piano ? juce::Colour (0xff050505) : knobBlack);
    grLabel.setColour (juce::Label::textColourId, piano ? lcdBg : ivory);

    vuL.setColours (piano ? juce::Colour (0xffa6c97f) : juce::Colour (0xfff7efd9), piano ? lcdInk : juce::Colour (0xff2a2420),
                    piano ? juce::Colour (0xff050505) : juce::Colour (0xff2a231e));
    vuR.setColours (piano ? juce::Colour (0xffa6c97f) : juce::Colour (0xfff7efd9), piano ? lcdInk : juce::Colour (0xff2a2420),
                    piano ? juce::Colour (0xff050505) : juce::Colour (0xff2a231e));

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, piano ? juce::Colour (0xfff2f2ef) : ivory);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, piano ? juce::Colour (0xff0b0b0c) : console);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, piano ? juce::Colour (0xff9a9a9a) : juce::Colour (0xffa89a82));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, (piano ? keyGreen : keyOrange).withAlpha (0.92f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, (piano ? keyGreen : keyOrange).withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, piano ? juce::Colour (0xff6a6a6e) : brownInk);

    for (auto* t : { &tabInstrument, &tabChords }) t->getProperties().set ("style", "chip");
    updateVisibility();
    sendLookAndFeelChange();
    resized();
    repaint();
}

void RootsKeysEditor::setPage (int newPage)
{
    page = juce::jlimit (0, 1, newPage);
    updateVisibility();
    resized();
    repaint();
}

void RootsKeysEditor::updateVisibility()
{
    const bool piano = (skin != 1), inst = (page == 0);
    tabInstrument.setToggleState (inst, juce::dontSendNotification);
    tabChords.setToggleState (! inst, juce::dontSendNotification);
    machinePanel->setVisible (! inst);
    machineOnBtn.setVisible (! inst);

    // área do teclado (a CHORD MACHINE tem o próprio teclado de acordes e usa a janela inteira)
    keyboard.setVisible (inst);
    wheels.setVisible (inst && piano);
    for (auto& b : organPreset) b.setVisible (inst && ! piano);

    // instrumento
    lcd.setVisible (inst && piano);
    for (auto& b : pianoPreset) b.setVisible (inst && piano);
    for (auto& b : pianoTimbre) b.setVisible (inst && piano);
    gtrButton.setVisible (inst && piano);
    for (auto* k : { &faderBright, &faderDecay, &faderChorus, &faderLevel })
    { k->slider.setVisible (inst && piano); k->label.setVisible (inst && piano); }
    for (auto& d : drawbars) d->setVisible (inst && ! piano);
    for (auto* b : { &leslieSwitch, &percOn, &perc3rd, &percSlow }) b->setVisible (inst && ! piano);
    for (auto* k : { &oLeslieMix, &oDrive, &oClick, &oLevel })
    { k->slider.setVisible (inst && ! piano); k->label.setVisible (inst && ! piano); }
    for (auto* k : { &eRate, &eIntensity, &eVol, &eSpring, &eWow, &eAge, &fRate, &fDepth, &fManual, &fFeedback, &fMix,
                      &mxGain, &mxCeil, &mMaster, &mReverb })
    { k->slider.setVisible (inst); k->label.setVisible (inst); }
    for (juce::Component* comp : { (juce::Component*) &echoButton, (juce::Component*) &echoModeBox, (juce::Component*) &flButton,
                                   (juce::Component*) &maxButton, (juce::Component*) &grLabel, (juce::Component*) &vuL, (juce::Component*) &vuR })
        comp->setVisible (inst);
}

void RootsKeysEditor::updateLcd()
{
    auto pct = [this] (const char* id) { return juce::String (juce::roundToInt (proc.apvts.getRawParameterValue (id)->load() * 100.0f)).paddedLeft (' ', 2); };
    const int prog = proc.getCurrentProgram();
    const int type = (int) proc.apvts.getRawParameterValue ("pianoType")->load();
    static const char* typeNames[kNumTimbres] = { "M1", "GRAND PIANO", "MT-40", "FM EP", "YAMAHA CP-80", "YAMAHA C7", "M1 GRAND PIANO",
                                                  "M1 PIANO 16 COMP", "HONKY TONK", "CASIO CZ-101",
                                                  "RHODES STAGE", "RHODES SUITCASE", "WURLITZER 200A" };
    const juce::String l1 = type != 0 ? juce::String ("TIMBRE  ") + typeNames[juce::jlimit (0, kNumTimbres - 1, type)]
                          : prog < RootsKeysProcessor::getNumPianoPresets()
                              ? "P0" + juce::String (prog + 1) + " " + proc.getProgramName (prog)
                              : juce::String ("P--  Piano: Edit");
    lcd.setLines (l1, "BRI " + pct ("pianoBright") + "  DEC " + pct ("pianoDecay")
                    + "  CHO " + pct ("pianoChorus") + "  VOL " + pct ("pianoLevel"));
}

int RootsKeysEditor::lookForTimbre (int t)
{
    static const int m[13] = { 0, 4, 5, 0, 4, 4, 0, 0, 0, 5, 1, 2, 3 };
    return m[juce::jlimit (0, 12, t)];
}

void RootsKeysEditor::timerCallback()
{
    applySkin ((int) proc.apvts.getRawParameterValue ("mode")->load());
    {
        const int newLook = lookForTimbre ((int) proc.apvts.getRawParameterValue ("pianoType")->load());
        if (newLook != look) { look = newLook; repaint(); }
    }

    vuL.update (proc.meterL.load(), 1.0f / 30.0f);
    vuR.update (proc.meterR.load(), 1.0f / 30.0f);
    grLabel.setText ("GR " + juce::String (proc.gainReduction.load(), 1), juce::dontSendNotification);

    const int prog = proc.getCurrentProgram(), np = RootsKeysProcessor::getNumPianoPresets();
    for (int i = 0; i < 4; ++i)
    {
        pianoPreset[i].setToggleState (prog == i && (int) proc.apvts.getRawParameterValue ("pianoType")->load() == 0, juce::dontSendNotification);
        organPreset[i].setToggleState (prog == np + i, juce::dontSendNotification);
    }
    for (int i = 0; i < kNumTimbres; ++i)
        pianoTimbre[i].setToggleState ((int) proc.apvts.getRawParameterValue ("pianoType")->load() == i, juce::dontSendNotification);
    if (skin == 0)
    {
        updateLcd();
        wheels.setValues (proc.pitchWheel.load(), proc.modWheel.load());
    }
}

// ---------------------------------------------------------------------  fundo (gerado 1x por visual)
void RootsKeysEditor::drawPlate (juce::Graphics& g, juce::Rectangle<float> r)
{
    // emblema de latão dourado polido, sem parafusos
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.expanded (2.0f).translated (0.0f, 2.5f), 9.0f);
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff8a6418), r.getY() - 2.0f,
                                                       juce::Colour (0xff4a3208), r.getBottom() + 2.0f));
    g.fillRoundedRectangle (r.expanded (2.0f), 9.0f);
    juce::ColourGradient gold (juce::Colour (0xfffff3c4), 0.0f, r.getY(), juce::Colour (0xfff3d98a), 0.0f, r.getBottom(), false);
    gold.addColour (0.35, juce::Colour (0xffe2b956));
    gold.addColour (0.52, juce::Colour (0xffa9781f));
    gold.addColour (0.70, juce::Colour (0xffd8a943));
    g.setGradientFill (gold);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.42f), 8.0f);
    g.setColour (juce::Colour (0xff6b4a10).withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (3.0f), 6.0f, 1.0f);

    g.setFont (juce::FontOptions (plateFace).withHeight (17.0f));
    g.setColour (juce::Colour (0xfffff8dc).withAlpha (0.7f));
    g.drawText ("DUB STATION", r.translated (0.0f, 1.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xff3a2606));
    g.drawText ("DUB STATION", r, juce::Justification::centred);
}

void RootsKeysEditor::renderBackground (juce::Graphics& g, int forSkin)
{
    const bool piano = (forSkin == 0);
    auto sectionTitle = [&] (juce::Rectangle<int> r, const juce::String& t, const juce::String& sub, juce::Colour c, juce::Colour subC)
    {
        g.setFont (VintageLookAndFeel::mono (11.0f));
        g.setColour (c);
        g.drawText (t, r.getX() + 14, r.getY() + 6, 200, 14, juce::Justification::centredLeft);
        if (sub.isNotEmpty())
        {
            g.setColour (subC);
            g.setFont (VintageLookAndFeel::mono (10.0f, false));
            g.drawText (sub, r.getX() + 14 + t.length() * 7 + 12, r.getY() + 6, 200, 14, juce::Justification::centredLeft);
        }
    };

    if (piano)
    {
        // ---------------- gabinete conforme o timbre (look): o anos 80 preto é o de sempre
        struct Finish
        {
            juce::Colour body, header, trimHi, trimLo, cloth, clothA, clothB, band1, band2, plate, panelFill;
            const char* line; const char* badge; bool lightBody;
        };
        static const Finish finishes[6] = {
            // 0 teclado preto anos 80 (M1)
            { juce::Colour (0xff151517), juce::Colour (0xff18181a), juce::Colour (0xff9a9da2), juce::Colour (0xff5a5d62),
              juce::Colour (0xff1d1d20), juce::Colour (0xff151517), juce::Colour (0xff27272b), ledRed, juce::Colour (0xffe2b956),
              juce::Colour (0xff9a9a9e), juce::Colour (0x00000000), "DIGITAL PIANO SYNTHESIS", "ROOTS", false },
            // 1 Rhodes Stage: tolex preto, chapa de alumínio, feltro vermelho
            { juce::Colour (0xff1b1a19), juce::Colour (0xff101010), juce::Colour (0xffd9d9d6), juce::Colour (0xff8f8f8c),
              juce::Colour (0xff222120), juce::Colour (0xff1a1918), juce::Colour (0xff2c2b29), juce::Colour (0xff8e2a20), juce::Colour (0xffbfbfbb),
              juce::Colour (0xffd9d9d6), juce::Colour (0x00000000), "STAGE 73  ELECTRIC PIANO", "STAGE 73", false },
            // 2 Rhodes Suitcase: tolex preto e o gabinete de alto-falante com a luz do tremolo
            { juce::Colour (0xff1b1a19), juce::Colour (0xff101010), juce::Colour (0xffd9d9d6), juce::Colour (0xff8f8f8c),
              juce::Colour (0xff2a2927), juce::Colour (0xff262523), juce::Colour (0xff3a3936), juce::Colour (0xffe2483d), juce::Colour (0xffbfbfbb),
              juce::Colour (0xffd9d9d6), juce::Colour (0x00000000), "SUITCASE 73  TREMOLO", "SUITCASE", false },
            // 3 Wurlitzer 200A: plástico creme anos 60, faixa de controles marrom
            { juce::Colour (0xffe9dcc3), juce::Colour (0xff3c3226), juce::Colour (0xfff3e6c8), juce::Colour (0xffb9a888),
              juce::Colour (0xffd8c9ab), juce::Colour (0xffcdbd9e), juce::Colour (0xffe3d5b8), juce::Colour (0xff8a5a2b), juce::Colour (0xff5b4a35),
              juce::Colour (0xff5b4a35), juce::Colour (0xff2e261d), "200A  ELECTRIC PIANO", "200A", true },
            // 4 Yamaha CP / C7 / Grand: madeira escura com plaqueta dourada
            { juce::Colour (0xff2a1c14), juce::Colour (0xff1a110b), juce::Colour (0xffd6b36a), juce::Colour (0xff7a5f2c),
              juce::Colour (0xff1e1510), juce::Colour (0xff17100c), juce::Colour (0xff2a1d15), juce::Colour (0xffd6b36a), juce::Colour (0xff6b5a48),
              juce::Colour (0xffd6b36a), juce::Colour (0x00000000), "ELECTRIC GRAND  CP / C7", "GRAND", false },
            // 5 Casio CZ-101 / MT-40: plástico cinza, faixa escura, teclas azuis e vermelhas
            { juce::Colour (0xffb9bbbd), juce::Colour (0xff2b2d31), juce::Colour (0xffe4e5e6), juce::Colour (0xff8d8f92),
              juce::Colour (0xffa9abad), juce::Colour (0xff9fa1a3), juce::Colour (0xffb3b5b7), juce::Colour (0xff2b5fa8), juce::Colour (0xffc43a32),
              juce::Colour (0xff2b2d31), juce::Colour (0xff2b2d31), "PHASE DISTORTION  CZ / MT", "CZ / MT", true },
        };
        const Finish& F = finishes[juce::jlimit (0, 5, look)];

        if (look == 0)
        {
            static const juce::Image plastic = skin::makeSatin (kW, kH);
            g.drawImageAt (plastic, 0, 0);
        }
        else if (look == 4)
        {
            static const juce::Image wood = skin::makeWood (kW, kH);
            g.drawImageAt (wood, 0, 0);
            g.setColour (juce::Colours::black.withAlpha (0.55f));   // madeira escura, verniz de piano
            g.fillRect (0, 0, kW, kH);
        }
        else
        {
            g.fillAll (F.body);
            if (! F.lightBody)
            {   // tolex: trama fina
                g.setColour (juce::Colours::white.withAlpha (0.025f));
                for (int y = 0; y < kH; y += 3) g.drawHorizontalLine (y, 0.0f, (float) kW);
                g.setColour (juce::Colours::black.withAlpha (0.18f));
                for (int x = 0; x < kW; x += 4) g.drawVerticalLine (x, 0.0f, (float) kH);
            }
        }
        g.setColour (F.header);
        g.fillRect (0, 0, kW, 74);
        // friso sob o cabeçalho
        g.setGradientFill (juce::ColourGradient::vertical (F.trimHi, 74.0f, F.trimLo, 77.0f));
        g.fillRect (0, 74, kW, 3);
        if (look == 1 || look == 2)
        {   // feltro vermelho do Rhodes logo abaixo da chapa
            g.setColour (juce::Colour (0xff8e2a20));
            g.fillRect (0, 77, kW, 3);
        }

        g.setColour (plasticMuted);
        g.setFont (juce::FontOptions (13.0f, juce::Font::italic));
        g.drawText ("DubMastor", 28, 8, 200, 18, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xfff2f2f2));
        g.setFont (juce::FontOptions (pianoTitleFace).withHeight (23.0f).withKerningFactor (0.06f));
        g.drawText ("REGGAE KEYS", 26, 26, 340, 30, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xff75757a));
        g.setFont (VintageLookAndFeel::mono (9.0f, false));
        g.drawText (juce::String (F.line) + "  MRK-80  " + versionText(), 30, 58, 360, 12, juce::Justification::centredLeft);
        g.setFont (VintageLookAndFeel::mono (10.0f, false));
        g.drawText ("MODE", 764, 12, 60, 12, juce::Justification::centredLeft);

        // caixas de som com tela de tecido e faixa vermelha/dourada
        for (auto sp : { kSpeakerL, kSpeakerR })
            if (page == 0)
        {
            auto box = sp.toFloat();
            g.setColour (F.lightBody ? F.header : juce::Colour (0xff0a0a0b));
            g.fillRoundedRectangle (box, 8.0f);
            auto cloth = box.reduced (8.0f);
            g.setColour (F.cloth);
            g.fillRoundedRectangle (cloth, 4.0f);
            {
                juce::Graphics::ScopedSaveState ss (g);
                g.reduceClipRegion (cloth.toNearestInt());
                g.setColour (F.clothA);
                for (float y = cloth.getY(); y < cloth.getBottom(); y += (look == 2 ? 5.0f : 2.0f))   // Suitcase: grade larga do gabinete
                    g.drawHorizontalLine ((int) y, cloth.getX(), cloth.getRight());
                g.setColour (F.clothB);
                for (float x = cloth.getX(); x < cloth.getRight(); x += 2.0f)
                    g.drawVerticalLine ((int) x, cloth.getY(), cloth.getBottom());
                // leve volume do cone por trás do tecido
                g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.0f), cloth.getCentreX(), cloth.getCentreY() - 10.0f,
                                                         juce::Colours::black.withAlpha (0.35f), cloth.getX(), cloth.getY(), true));
                g.fillRect (cloth);
            }
            const float sy = cloth.getBottom() - 44.0f;
            g.setColour (F.band1);
            g.fillRect (cloth.getX(), sy, cloth.getWidth(), 4.0f);
            g.setColour (F.band2);
            g.fillRect (cloth.getX(), sy + 6.0f, cloth.getWidth(), 2.0f);
            g.setColour (F.plate);
            g.setFont (juce::FontOptions (pianoTitleFace).withHeight (10.0f).withKerningFactor (0.2f));
            g.drawText (F.badge, cloth.withTop (sy + 12.0f).withHeight (24.0f), juce::Justification::centred);
            if (look == 2)
            {   // luz do tremolo da maleta
                g.setColour (juce::Colour (0xffe2483d));
                g.fillEllipse (cloth.getRight() - 18.0f, cloth.getY() + 8.0f, 9.0f, 9.0f);
            }
        }

        auto panel = [&] (juce::Rectangle<int> r)
        {
            if (F.lightBody)
            {   // gabinete claro: a faixa de controles é escura, para os knobs e textos continuarem legíveis
                g.setColour (F.panelFill);
                g.fillRoundedRectangle (r.toFloat(), 4.0f);
            }
            g.setColour (juce::Colours::white.withAlpha (0.025f));
            g.fillRoundedRectangle (r.toFloat(), 4.0f);
            g.setColour (juce::Colour (0xff3a3a3e));
            g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 4.0f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.drawHorizontalLine (r.getY() + 1, (float) r.getX() + 4, (float) r.getRight() - 4);
        };
        panel (kKeysBed);
        if (page == 0)
        {
            panel (kPianoPanel);
            panel (kEcho);
            panel (kFlanger);
            panel (kOut);
            g.setColour (juce::Colour (0xff050505));
            g.fillRoundedRectangle (kLcd.toFloat(), 4.0f);

            g.setFont (VintageLookAndFeel::mono (9.0f, false));
            g.setColour (juce::Colour (0xff95959a));
            for (int i = 0; i < 4; ++i)
                g.drawText (kPianoPresetShort[i], 290 + i * 106 - 6, 230, 96, 12, juce::Justification::centred);
            g.setFont (VintageLookAndFeel::mono (10.0f));
            g.setColour (plasticText);
            g.drawText ("GUITAR", 706, 244, 74, 12, juce::Justification::centred);
            g.drawText ("SKANK", 706, 256, 74, 12, juce::Justification::centred);

            sectionTitle (kEcho, "TAPE ECHO", "MTE-12", plasticText, plasticMuted);
            sectionTitle (kFlanger, "FLANGER", "DUB JET SWEEP", plasticText, plasticMuted);
            sectionTitle (kOut, "MAXIMIZER", "", plasticText, plasticMuted);
            g.setColour (juce::Colour (0xff3a3a3e));
            for (auto r : { kEcho, kFlanger, kOut })
                g.drawHorizontalLine (r.getY() + 24, (float) r.getX() + 14, (float) r.getRight() - 14);
        }
        g.setColour (juce::Colour (0xff5a1310));
        g.fillRect (128, 644, 840, 4);   // feltro
    }
    else
    {
        // ---------------- órgão: madeira simples + marfim
        static const juce::Image wood = skin::makeWood (kW, kH);
        g.drawImageAt (wood, 0, 0);
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.fillRect (0, 0, kW, 74);
        g.setColour (brownInk);
        g.fillRect (0, 73, kW, 2);

        g.setColour (ivory);
        g.setFont (juce::FontOptions (juce::Font::getDefaultSerifFontName(), 16.0f, juce::Font::italic));
        g.drawText ("DubMastor", 30, 10, 120, 20, juce::Justification::centredLeft);
        g.setColour (ivory.withAlpha (0.75f));
        g.setFont (VintageLookAndFeel::mono (10.0f, false));
        g.drawText (versionText(), 112, 12, 60, 16, juce::Justification::centredLeft);
        g.setColour (ivory);
        g.setFont (juce::FontOptions (organTitleFace).withHeight (36.0f));
        g.drawText ("Reggae Keys", 28, 24, 320, 44, juce::Justification::centredLeft);

        // trilho escuro da chave de modo
        g.setColour (brownInk);
        g.fillRoundedRectangle (764.0f, 24.0f, 196.0f, 32.0f, 16.0f);

        if (page == 0)
        {
            // console preto das drawbars
            g.setColour (console);
            g.fillRoundedRectangle (kConsole.toFloat(), 4.0f);
            g.setColour (brownInk);
            g.drawRoundedRectangle (kConsole.toFloat(), 4.0f, 2.0f);

            auto ivoryPanel = [&] (juce::Rectangle<int> r)
            {
                g.setColour (juce::Colours::black.withAlpha (0.25f));
                g.fillRoundedRectangle (r.toFloat().translated (0, 2.0f), 5.0f);
                g.setColour (ivory);
                g.fillRoundedRectangle (r.toFloat(), 5.0f);
            };
            ivoryPanel (kLeslie);
            ivoryPanel (kPerc);
            ivoryPanel (kEcho);
            ivoryPanel (kFlanger);
            ivoryPanel (kOut);
            g.setFont (VintageLookAndFeel::mono (10.0f));
            g.setColour (brownInk);
            g.drawText ("LESLIE", kLeslie.getX(), kLeslie.getY() + 8, kLeslie.getWidth(), 14, juce::Justification::centred);
            g.drawText ("PERCUSSION", kPerc.getX(), kPerc.getY() + 8, kPerc.getWidth(), 14, juce::Justification::centred);
            sectionTitle (kEcho, "TAPE ECHO", "MTE-12", brownInk, brownInk.withAlpha (0.6f));
            sectionTitle (kFlanger, "FLANGER", "DUB JET SWEEP", brownInk, brownInk.withAlpha (0.6f));
            sectionTitle (kOut, "MAXIMIZER", "", brownInk, brownInk);
            g.setColour (brownInk.withAlpha (0.3f));
            for (auto r : { kEcho, kFlanger, kOut })
                g.drawHorizontalLine (r.getY() + 24, (float) r.getX() + 14, (float) r.getRight() - 14);
        }

        g.setColour (brownInk);
        g.fillRoundedRectangle (kKeysBed.toFloat(), 6.0f);
        g.setColour (juce::Colour (0xff7a1a14));
        g.fillRect (34, 644, 932, 5);   // feltro
        if (page == 0)
        {
            g.setColour (juce::Colour (0xffa89a82));
            g.setFont (VintageLookAndFeel::mono (10.0f, false));
            g.drawText ("MOD WHEEL = LESLIE FAST", kConsole.getX(), kConsole.getBottom() - 18, kConsole.getWidth(), 12, juce::Justification::centred);
        }
    }

    drawPlate (g, kPlate.toFloat());
}

void RootsKeysEditor::paint (juce::Graphics& g)
{
    if (backgroundSkin != skin || backgroundPage != page || backgroundLook != look || background.isNull())
    {
        background = juce::Image (juce::Image::RGB, kW, kH, true);
        juce::Graphics bg (background);
        renderBackground (bg, skin);
        backgroundSkin = skin;
        backgroundPage = page;
        backgroundLook = look;
    }
    g.drawImageAt (background, 0, 0);
    if (page == 1)
    {   // fundo da aba CHORD MACHINE (o painel do MRCM-1240 ocupa quase tudo)
        g.setColour (juce::Colour (0xff1b1410));
        g.fillRect (0, 77, kW, kH - 77);
    }
}

void RootsKeysEditor::resized()
{
    const bool piano = (skin != 1);

    // abas
    tabInstrument.setBounds (222, 30, 88, 26);
    tabChords.setBounds (313, 30, 100, 26);
    machineOnBtn.setBounds (600, 30, 150, 26);
    {   // o painel do MRCM-1240 (1240 x 846) escalado para caber embaixo do cabeçalho
        const int pw = 1240, ph = 846, top = 78;
        const float sc = juce::jmin ((float) kW / (float) pw, (float) (kH - top) / (float) ph);
        machinePanel->setTransform ({});
        machinePanel->setBounds (0, 0, pw, ph);
        const float x = ((float) kW - pw * sc) * 0.5f, y = (float) top + ((float) (kH - top) - ph * sc) * 0.5f;
        machinePanel->setTransform (juce::AffineTransform::scale (sc).translated (x, y));
    }

    // cabeçalho
    modePiano.setBounds (piano ? juce::Rectangle<int> (764, 28, 96, 26) : juce::Rectangle<int> (766, 27, 95, 26));
    modeOrgan.setBounds (piano ? juce::Rectangle<int> (866, 28, 96, 26) : juce::Rectangle<int> (863, 27, 95, 26));

    // piano
    lcd.setBounds (kLcd);
    for (int i = 0; i < 4; ++i)
        pianoPreset[i].setBounds (290 + i * 106, 207, 84, 22);
    gtrButton.setBounds (714, 272, 58, 26);
    {
        // timbres em duas linhas de 7, agrupados por família (o índice do botão continua sendo o do parâmetro)
        static const int order[kNumTimbres] = { 7, 0, 6, 1, 8, 4, 5,          // Korg M1, acústicos e Yamaha CP-80 / C7
                                                10, 11, 12, 3, 9, 2 };      // Rhodes, Suitcase, Wurli, FM EP, CZ-101, MT-40
        for (int slot = 0; slot < kNumTimbres; ++slot)
        {
            const int row = slot < 7 ? 0 : 1, col = slot < 7 ? slot : slot - 7;
            pianoTimbre[order[slot]].setBounds (276 + col * 64, 164 + row * 21, 61, 19);
        }
    }
    {
        Knob* fs[4] = { &faderBright, &faderDecay, &faderChorus, &faderLevel };
        for (int i = 0; i < 4; ++i)
        {
            const int cx = 318 + i * 122;
            fs[i]->label.setBounds (cx - 45, 244, 90, 14);
            fs[i]->slider.setBounds (cx - 22, 258, 44, 92);
        }
    }
    wheels.setBounds (40, 652, 74, 128);

    // órgão
    for (int i = 0; i < 9; ++i)
        drawbars[i]->setBounds (258 + i * 60 - 24, 102, 48, 232);
    leslieSwitch.setBounds (kLeslie.getX() + 25, kLeslie.getY() + 30, 100, 74);
    percOn.setBounds   (kPerc.getX() + 12,  kPerc.getY() + 28, 38, 96);
    perc3rd.setBounds  (kPerc.getX() + 56,  kPerc.getY() + 28, 38, 96);
    percSlow.setBounds (kPerc.getX() + 100, kPerc.getY() + 28, 38, 96);
    placeKnob (oLeslieMix, { 80, 266 }, 52);
    placeKnob (oDrive,     { 152, 266 }, 52);
    placeKnob (oClick,     { 848, 280 }, 52);
    placeKnob (oLevel,     { 920, 280 }, 52);
    for (int i = 0; i < 4; ++i)
        organPreset[i].setBounds (40, 652 + i * 32, 100, 28);

    // eco (igual nos dois visuais)
    echoButton.setBounds (kEcho.getX() + 14, kEcho.getY() + 36, piano ? 76 : 80, piano ? 26 : 34);
    echoModeBox.setBounds (kEcho.getX() + 14, kEcho.getY() + 80, 100, 24);
    {
        Knob* ks[6] = { &eRate, &eIntensity, &eVol, &eSpring, &eWow, &eAge };
        for (int i = 0; i < 6; ++i)
            placeKnob (*ks[i], { kEcho.getX() + 172 + i * 78, kEcho.getY() + 64 }, 46, 76);
    }

    // flanger (mesma grade de knobs do eco)
    flButton.setBounds (kFlanger.getX() + 14, kFlanger.getY() + 36, piano ? 100 : 104, piano ? 26 : 34);
    {
        Knob* ks[5] = { &fRate, &fDepth, &fManual, &fFeedback, &fMix };
        for (int i = 0; i < 5; ++i)
            placeKnob (*ks[i], { kFlanger.getX() + 172 + i * 78, kFlanger.getY() + 64 }, 46, 76);
    }

    // maximizer + VU (painel alto, ocupa as duas fileiras)
    vuL.setBounds (kOut.getX() + 14, kOut.getY() + 34, 144, 100);
    vuR.setBounds (kOut.getX() + 166, kOut.getY() + 34, 144, 100);
    maxButton.setBounds (kOut.getX() + 14, kOut.getY() + 144, piano ? 70 : 74, piano ? 26 : 30);
    grLabel.setBounds (kOut.getX() + 100, kOut.getY() + 148, 72, 20);
    {
        Knob* ks[4] = { &mxGain, &mxCeil, &mReverb, &mMaster };
        for (int i = 0; i < 4; ++i)
            placeKnob (*ks[i], { kOut.getX() + 48 + i * 76, kOut.getY() + 204 }, 42, 70);
    }

    // teclado
    if (piano) keyboard.setBounds (128, 650, 840, 130);
    else       keyboard.setBounds (152, 651, 812, 129);
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f);
}
