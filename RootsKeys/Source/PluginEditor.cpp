#include "PluginEditor.h"
#include "SkinTextures.h"
#include "BinaryData.h"

using namespace vintage;

namespace
{
constexpr int kW = 1000, kH = 724;

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

const char* kPianoPresetShort[4] = { "M1 CLASSIC", "DUB SKANK", "BRIGHT HOUSE", "SOFT BALLAD" };
const char* kOrganPresetShort[4] = { "BUBBLE", "PERC", "DUB DARK", "GOSPEL" };

// ----- layout fixo (1000 x 724)
const juce::Rectangle<int> kPlate      { 318, 16, 170, 40 };
const juce::Rectangle<int> kSignature  { 500, 8, 250, 60 };
const juce::Rectangle<int> kEcho       { 24, 372, 616, 120 };
const juce::Rectangle<int> kOut        { 652, 372, 324, 120 };
const juce::Rectangle<int> kKeysBed    { 24, 504, 952, 208 };
// piano
const juce::Rectangle<int> kSpeakerL   { 24, 86, 186, 274 };
const juce::Rectangle<int> kSpeakerR   { 790, 86, 186, 274 };
const juce::Rectangle<int> kPianoPanel { 222, 86, 556, 274 };
const juce::Rectangle<int> kLcd        { 276, 98, 448, 74 };
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
    scriptFace = juce::Typeface::createSystemTypefaceFor (BinaryData::KaushanScriptRegular_ttf,
                                                          (size_t) BinaryData::KaushanScriptRegular_ttfSize);
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

    lcd.setVisible (piano);
    wheels.setVisible (piano);
    for (auto& b : pianoPreset) b.setVisible (piano);
    for (auto* k : { &faderBright, &faderDecay, &faderChorus, &faderLevel })
    { k->slider.setVisible (piano); k->label.setVisible (piano); }

    for (auto& d : drawbars) d->setVisible (! piano);
    for (auto* b : { &leslieSwitch, &percOn, &perc3rd, &percSlow }) b->setVisible (! piano);
    for (auto& b : organPreset) b.setVisible (! piano);
    for (auto* k : { &oLeslieMix, &oDrive, &oClick, &oLevel })
    { k->slider.setVisible (! piano); k->label.setVisible (! piano); }

    // cores que dependem do visual
    const auto tickOnPanel  = piano ? plasticMuted : brownInk;   // knobs sobre painel (marfim no órgão)
    const auto tickOnWood   = ivory;
    const auto labelOnPanel = piano ? plasticText : brownInk;
    for (auto* k : { &eRate, &eIntensity, &eVol, &eSpring, &eWow, &eAge, &mxGain, &mxCeil, &mMaster, &mReverb })
    {
        k->slider.setColour (juce::Slider::rotarySliderOutlineColourId, tickOnPanel);
        k->label.setColour (juce::Label::textColourId, labelOnPanel);
        k->slider.setVisible (true); k->label.setVisible (true);
    }
    for (auto* k : { &oLeslieMix, &oDrive, &oClick, &oLevel })
    {
        k->slider.setColour (juce::Slider::rotarySliderOutlineColourId, tickOnWood);
        k->label.setColour (juce::Label::textColourId, ivory);
    }
    for (auto* k : { &faderBright, &faderDecay, &faderChorus, &faderLevel })
        k->label.setColour (juce::Label::textColourId, plasticText);

    echoButton.getProperties().set ("style", piano ? "led" : "rocker");
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

    sendLookAndFeelChange();
    resized();
    repaint();
}

void RootsKeysEditor::updateLcd()
{
    auto pct = [this] (const char* id) { return juce::String (juce::roundToInt (proc.apvts.getRawParameterValue (id)->load() * 100.0f)).paddedLeft (' ', 2); };
    const int prog = proc.getCurrentProgram();
    const juce::String l1 = prog < RootsKeysProcessor::getNumPianoPresets()
        ? "P0" + juce::String (prog + 1) + " " + proc.getProgramName (prog)
        : juce::String ("P--  Piano: Edit");
    lcd.setLines (l1, "BRI " + pct ("pianoBright") + "  DEC " + pct ("pianoDecay")
                    + "  CHO " + pct ("pianoChorus") + "  VOL " + pct ("pianoLevel"));
}

void RootsKeysEditor::timerCallback()
{
    applySkin ((int) proc.apvts.getRawParameterValue ("mode")->load());

    vuL.update (proc.meterL.load(), 1.0f / 30.0f);
    vuR.update (proc.meterR.load(), 1.0f / 30.0f);
    grLabel.setText ("GR " + juce::String (proc.gainReduction.load(), 1), juce::dontSendNotification);

    const int prog = proc.getCurrentProgram(), np = RootsKeysProcessor::getNumPianoPresets();
    for (int i = 0; i < 4; ++i)
    {
        pianoPreset[i].setToggleState (prog == i, juce::dontSendNotification);
        organPreset[i].setToggleState (prog == np + i, juce::dontSendNotification);
    }
    if (skin == 0)
    {
        updateLcd();
        wheels.setValues (proc.pitchWheel.load(), proc.modWheel.load());
    }
}

// ---------------------------------------------------------------------  fundo (gerado 1x por visual)
void RootsKeysEditor::drawPlate (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 3.0f);
    g.setColour (juce::Colour (0xffc9c6bd));
    g.fillRoundedRectangle (r, 3.0f);
    juce::Random rnd (42);
    for (float y = r.getY() + 2.0f; y < r.getBottom() - 2.0f; y += 1.0f)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.04f + 0.06f * rnd.nextFloat()));
        g.drawHorizontalLine ((int) y, r.getX() + 2.0f, r.getRight() - 2.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.drawLine (r.getX() + 3.0f, r.getY() + 1.0f, r.getRight() - 3.0f, r.getY() + 1.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    for (auto pt : { juce::Point<float> (r.getX() + 7, r.getY() + 7), juce::Point<float> (r.getRight() - 7, r.getY() + 7),
                     juce::Point<float> (r.getX() + 7, r.getBottom() - 7), juce::Point<float> (r.getRight() - 7, r.getBottom() - 7) })
    {
        g.setColour (juce::Colour (0xff9a978f));
        g.fillEllipse (pt.x - 3.0f, pt.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colour (0xff5a554c));
        g.drawLine (pt.x - 2.2f, pt.y + 1.0f, pt.x + 2.2f, pt.y - 1.0f, 1.0f);
    }
    g.setFont (juce::FontOptions (plateFace).withHeight (17.0f));
    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.drawText ("DUB STATION", r.translated (0.0f, 1.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xff2a2622));
    g.drawText ("DUB STATION", r, juce::Justification::centred);
}

// assinatura "DubMastor" em pincel, igual à do Dub Station / FitaEco
void RootsKeysEditor::drawSignature (juce::Graphics& g, juce::Rectangle<float> sa, juce::Colour ink, juce::Colour swashCol)
{
    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (juce::AffineTransform::rotation (-0.07f, sa.getRight(), sa.getCentreY()));
    g.setFont (juce::FontOptions (scriptFace).withHeight (36.0f));
    const auto txt = juce::Rectangle<float> (sa.getX(), sa.getY() + 2.0f, sa.getWidth() - 20.0f, 42.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("DubMastor", txt.translated (1.5f, 2.0f), juce::Justification::centredRight);
    g.setColour (ink);
    g.drawText ("DubMastor", txt, juce::Justification::centredRight);
    juce::Path swash;
    const float x1 = sa.getRight() - 22.0f, y1 = sa.getBottom() - 10.0f;
    swash.startNewSubPath (x1 - 170.0f, y1 + 2.0f);
    swash.cubicTo (x1 - 115.0f, y1 - 4.0f, x1 - 55.0f, y1 + 4.0f, x1, y1 - 6.0f);
    g.setColour (swashCol.withAlpha (0.85f));
    g.strokePath (swash, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
        // ---------------- teclado anos 80: plástico preto texturizado
        static const juce::Image plastic = skin::makePlastic (kW, kH, 30.0f);
        g.drawImageAt (plastic, 0, 0);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRect (0, 0, kW, 74);
        g.setColour (juce::Colour (0xff4a4a4e));
        g.drawHorizontalLine (74, 0.0f, (float) kW);
        // arranhões leves
        juce::Random rnd (11);
        for (int i = 0; i < 46; ++i)
        {
            const float x = rnd.nextFloat() * kW, y = rnd.nextFloat() * kH, l = 8.0f + rnd.nextFloat() * 40.0f, a = rnd.nextFloat() * 0.6f - 0.3f;
            g.setColour (juce::Colour (0xff9a9aa0).withAlpha (0.06f + rnd.nextFloat() * 0.12f));
            g.drawLine (x, y, x + l * std::cos (a), y + l * std::sin (a), 0.5f + rnd.nextFloat() * 0.7f);
        }

        g.setColour (plasticMuted);
        g.setFont (juce::FontOptions (14.0f, juce::Font::italic));
        g.drawText ("Dub Station", 28, 10, 200, 18, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xfff2f2f2));
        g.setFont (juce::FontOptions (plateFace).withHeight (27.0f));
        g.drawText ("ROOTS KEYS", 28, 28, 280, 30, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xff7a7a7e));
        g.setFont (VintageLookAndFeel::mono (9.0f, false));
        g.drawText ("DIGITAL PIANO SYNTHESIS  RK-1", 30, 58, 260, 12, juce::Justification::centredLeft);
        g.setFont (VintageLookAndFeel::mono (10.0f, false));
        g.drawText ("MODE", 764, 12, 60, 12, juce::Justification::centredLeft);

        // caixas de som com grade
        for (auto sp : { kSpeakerL, kSpeakerR })
        {
            g.setColour (juce::Colour (0xff060607));
            g.fillRoundedRectangle (sp.toFloat(), 10.0f);
            g.setColour (juce::Colour (0xff3a3a3e));
            g.drawRoundedRectangle (sp.toFloat().reduced (0.5f), 10.0f, 1.0f);
            auto grille = sp.reduced (10).toFloat();
            g.setColour (juce::Colour (0xff141416));
            g.fillRoundedRectangle (grille, 6.0f);
            for (float y = grille.getY() + 4.0f; y < grille.getBottom() - 2.0f; y += 8.0f)
                for (float x = grille.getX() + 4.0f; x < grille.getRight() - 2.0f; x += 8.0f)
                {
                    g.setColour (juce::Colours::black);
                    g.fillEllipse (x - 2.2f, y - 2.2f, 4.4f, 4.4f);
                    g.setColour (juce::Colour (0xff303034));
                    g.fillEllipse (x - 1.5f, y - 1.5f, 1.6f, 1.6f);
                }
            g.setColour (juce::Colour (0xff55555a));
            g.setFont (VintageLookAndFeel::mono (9.0f, false));
            g.drawText ("RK SPEAKER", sp.getX(), sp.getBottom() - 22, sp.getWidth(), 12, juce::Justification::centred);
        }

        auto panel = [&] (juce::Rectangle<int> r)
        {
            g.setColour (juce::Colours::white.withAlpha (0.025f));
            g.fillRoundedRectangle (r.toFloat(), 4.0f);
            g.setColour (juce::Colour (0xff3a3a3e));
            g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 4.0f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.drawHorizontalLine (r.getY() + 1, (float) r.getX() + 4, (float) r.getRight() - 4);
        };
        panel (kPianoPanel);
        panel (kEcho);
        panel (kOut);
        panel (kKeysBed);
        g.setColour (juce::Colour (0xff050505));
        g.fillRoundedRectangle (kLcd.toFloat(), 4.0f);

        g.setFont (VintageLookAndFeel::mono (9.0f, false));
        g.setColour (juce::Colour (0xff95959a));
        for (int i = 0; i < 4; ++i)
            g.drawText (kPianoPresetShort[i], 290 + i * 106 - 6, 216, 96, 12, juce::Justification::centred);

        sectionTitle (kEcho, "TAPE ECHO", "FITAECO TE-12", plasticText, plasticMuted);
        sectionTitle (kOut, "MAXIMIZER", "", plasticText, plasticMuted);
        g.setColour (juce::Colour (0xff3a3a3e));
        g.drawHorizontalLine (kEcho.getY() + 24, (float) kEcho.getX() + 14, (float) kEcho.getRight() - 14);
        g.drawHorizontalLine (kOut.getY() + 24, (float) kOut.getX() + 14, (float) kOut.getRight() - 14);
        g.setColour (juce::Colour (0xff5a1310));
        g.fillRect (128, 512, 840, 4);   // feltro
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
        g.drawText ("Dub Station", 30, 10, 220, 20, juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (juce::Font::getDefaultSerifFontName(), 32.0f, juce::Font::bold | juce::Font::italic));
        g.drawText ("Roots Keys", 30, 28, 280, 38, juce::Justification::centredLeft);

        // trilho escuro da chave de modo
        g.setColour (brownInk);
        g.fillRoundedRectangle (764.0f, 24.0f, 196.0f, 32.0f, 16.0f);

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
        ivoryPanel (kOut);
        g.setFont (VintageLookAndFeel::mono (10.0f));
        g.setColour (brownInk);
        g.drawText ("LESLIE", kLeslie.getX(), kLeslie.getY() + 8, kLeslie.getWidth(), 14, juce::Justification::centred);
        g.drawText ("PERCUSSION", kPerc.getX(), kPerc.getY() + 8, kPerc.getWidth(), 14, juce::Justification::centred);
        sectionTitle (kEcho, "TAPE ECHO", "FITAECO TE-12", brownInk, brownInk.withAlpha (0.6f));
        sectionTitle (kOut, "MAXIMIZER", "", brownInk, brownInk);
        g.setColour (brownInk.withAlpha (0.3f));
        g.drawHorizontalLine (kEcho.getY() + 24, (float) kEcho.getX() + 14, (float) kEcho.getRight() - 14);
        g.drawHorizontalLine (kOut.getY() + 24, (float) kOut.getX() + 14, (float) kOut.getRight() - 14);

        g.setColour (brownInk);
        g.fillRoundedRectangle (kKeysBed.toFloat(), 6.0f);
        g.setColour (juce::Colour (0xff7a1a14));
        g.fillRect (34, 512, 932, 5);   // feltro
        g.setColour (juce::Colour (0xffa89a82));
        g.setFont (VintageLookAndFeel::mono (10.0f, false));
        g.drawText ("MOD WHEEL = LESLIE FAST", kConsole.getX(), kConsole.getBottom() - 18, kConsole.getWidth(), 12, juce::Justification::centred);
    }

    drawPlate (g, kPlate.toFloat());
    drawSignature (g, kSignature.toFloat(), piano ? juce::Colour (0xffe8e8e8) : ivory,
                   piano ? ledRed : juce::Colour (0xffb8862e));
}

void RootsKeysEditor::paint (juce::Graphics& g)
{
    if (backgroundSkin != skin || background.isNull())
    {
        background = juce::Image (juce::Image::RGB, kW, kH, true);
        juce::Graphics bg (background);
        renderBackground (bg, skin);
        backgroundSkin = skin;
    }
    g.drawImageAt (background, 0, 0);
}

void RootsKeysEditor::resized()
{
    const bool piano = (skin != 1);

    // cabeçalho
    modePiano.setBounds (piano ? juce::Rectangle<int> (764, 28, 96, 26) : juce::Rectangle<int> (766, 27, 95, 26));
    modeOrgan.setBounds (piano ? juce::Rectangle<int> (866, 28, 96, 26) : juce::Rectangle<int> (863, 27, 95, 26));

    // piano
    lcd.setBounds (kLcd);
    for (int i = 0; i < 4; ++i)
        pianoPreset[i].setBounds (290 + i * 106, 186, 84, 26);
    {
        Knob* fs[4] = { &faderBright, &faderDecay, &faderChorus, &faderLevel };
        for (int i = 0; i < 4; ++i)
        {
            const int cx = 318 + i * 122;
            fs[i]->label.setBounds (cx - 45, 236, 90, 14);
            fs[i]->slider.setBounds (cx - 22, 252, 44, 98);
        }
    }
    wheels.setBounds (40, 536, 74, 160);

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
        organPreset[i].setBounds (40, 528 + i * 42, 100, 36);

    // eco (igual nos dois visuais)
    echoButton.setBounds (kEcho.getX() + 14, kEcho.getY() + 36, piano ? 76 : 80, piano ? 26 : 34);
    echoModeBox.setBounds (kEcho.getX() + 14, kEcho.getY() + 80, 100, 24);
    {
        Knob* ks[6] = { &eRate, &eIntensity, &eVol, &eSpring, &eWow, &eAge };
        for (int i = 0; i < 6; ++i)
            placeKnob (*ks[i], { kEcho.getX() + 172 + i * 78, kEcho.getY() + 64 }, 46, 76);
    }

    // maximizer + VU
    vuL.setBounds (kOut.getX() + 12, kOut.getY() + 30, 92, 62);
    vuR.setBounds (kOut.getX() + 108, kOut.getY() + 30, 92, 62);
    maxButton.setBounds (kOut.getX() + 12, kOut.getY() + 94, piano ? 64 : 70, 24);
    grLabel.setBounds (kOut.getX() + 136, kOut.getY() + 96, 64, 18);
    placeKnob (mxGain,  { kOut.getX() + 236, kOut.getY() + 44 }, 32, 56);
    placeKnob (mxCeil,  { kOut.getX() + 292, kOut.getY() + 44 }, 32, 56);
    placeKnob (mReverb, { kOut.getX() + 236, kOut.getY() + 86 }, 32, 56);
    placeKnob (mMaster, { kOut.getX() + 292, kOut.getY() + 86 }, 32, 56);

    // teclado
    if (piano) keyboard.setBounds (128, 518, 840, 186);
    else       keyboard.setBounds (152, 520, 812, 184);
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f);
}
