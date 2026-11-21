#include "PluginEditor.h"

using namespace vintage;

namespace
{
// cores das drawbars no padrão Hammond: marrom (sub), branca (oitavas), preta (quintas/terças)
juce::Colour drawbarColour (int i)
{
    switch (i)
    {
        case 0: case 1: return juce::Colour (0xff7a3e1d);
        case 4: case 6: case 7: return juce::Colour (0xff1b1b1b);
        default: return juce::Colour (0xfff4f0e6);
    }
}

void screw (juce::Graphics& g, float x, float y)
{
    g.setColour (silver);
    g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
    g.setColour (juce::Colour (0xff5a554c));
    g.drawLine (x - 3.0f, y + 1.5f, x + 3.0f, y - 1.5f, 1.2f);
}

// plaquinha creme com texto marrom (rótulo de painel)
void plate (juce::Graphics& g, juce::Rectangle<int> panel, const juce::String& text)
{
    g.setFont (VintageLookAndFeel::monoFont (13.0f));
    const int tw = text.length() * 8 + 20;   // fonte monoespaçada ~8 px por caractere
    auto r = juce::Rectangle<float> ((float) panel.getX() + 14.0f, (float) panel.getY() + 6.0f, (float) tw, 19.0f);
    g.setColour (cream);
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (brown);
    g.drawRoundedRectangle (r.reduced (2.0f), 1.5f, 1.0f);
    g.drawText (text, r, juce::Justification::centred);
}

// placa de alumínio escovado gravada "DUB STATION"
void metalPlate (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto r = area.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 4.0f);
    g.setColour (juce::Colour (0xffc9c6bd));
    g.fillRoundedRectangle (r, 4.0f);

    // escovado: linhas horizontais finas alternando claro/escuro
    juce::Random rnd (42);
    for (float y = r.getY() + 2.0f; y < r.getBottom() - 2.0f; y += 1.0f)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.04f + 0.06f * rnd.nextFloat()));
        g.drawHorizontalLine ((int) y, r.getX() + 2.0f, r.getRight() - 2.0f);
    }

    // chanfro
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.drawLine (r.getX() + 3.0f, r.getY() + 1.0f, r.getRight() - 3.0f, r.getY() + 1.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);

    screw (g, r.getX() + 8.0f, r.getY() + 8.0f);
    screw (g, r.getRight() - 8.0f, r.getY() + 8.0f);
    screw (g, r.getX() + 8.0f, r.getBottom() - 8.0f);
    screw (g, r.getRight() - 8.0f, r.getBottom() - 8.0f);

    // gravação (relevo baixo): sombra clara deslocada + texto escuro
    const auto font = juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 19.0f, juce::Font::bold));
    g.setFont (font);
    auto tr = r.reduced (16.0f, 0.0f);
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.drawText ("DUB STATION", tr.translated (0.0f, 1.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xff2a2622));
    g.drawText ("DUB STATION", tr, juce::Justification::centred);
}
} // namespace

RootsKeysEditor::RootsKeysEditor (RootsKeysProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;

    auto headerLabel = [] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredRight);
        l.setFont (VintageLookAndFeel::monoFont (13.0f));
        l.setColour (juce::Label::textColourId, mustard);
    };

    // Modo
    headerLabel (modeLabel, "MODE");
    addAndMakeVisible (modeLabel);
    modeBox.addItemList ({ "Piano", "Organ" }, 1);
    addAndMakeVisible (modeBox);
    modeAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "mode", modeBox);

    // Presets (separados por instrumento)
    headerLabel (presetLabel, "PRESET");
    addAndMakeVisible (presetLabel);
    presetBox.addSectionHeading ("PIANO");
    for (int i = 0; i < proc.getNumPrograms(); ++i)
    {
        if (i == RootsKeysProcessor::getNumPianoPresets())
        {
            presetBox.addSeparator();
            presetBox.addSectionHeading ("ORGAN");
        }
        presetBox.addItem (proc.getProgramName (i), i + 1);
    }
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0) proc.setCurrentProgram (idx);
    };
    addAndMakeVisible (presetBox);

    // Piano
    setupKnob (pBright, "pianoBright", "Brilho");
    setupKnob (pDecay,  "pianoDecay",  "Decay");
    setupKnob (pChorus, "pianoChorus", "Chorus");
    setupKnob (pLevel,  "pianoLevel",  "Volume");

    // Drawbars
    for (int i = 0; i < 9; ++i)
    {
        auto& d = drawbars[i];
        d.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        d.slider.setColour (juce::Slider::thumbColourId, drawbarColour (i));
        addAndMakeVisible (d.slider);
        d.att = std::make_unique<APVTS::SliderAttachment> (s, "db" + juce::String (i + 1), d.slider);
        d.label.setText (RootsKeysProcessor::drawbarNames()[i], juce::dontSendNotification);
        d.label.setJustificationType (juce::Justification::centred);
        d.label.setFont (VintageLookAndFeel::monoFont (12.0f));
        addAndMakeVisible (d.label);
    }

    // Percussão / Leslie
    addAndMakeVisible (percButton);
    percAtt = std::make_unique<APVTS::ButtonAttachment> (s, "percOn", percButton);
    percHarmBox.addItemList ({ "2nd", "3rd" }, 1);
    addAndMakeVisible (percHarmBox);
    percHarmAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "percHarm", percHarmBox);
    percDecayBox.addItemList ({ "Fast", "Slow" }, 1);
    addAndMakeVisible (percDecayBox);
    percDecayAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "percDecay", percDecayBox);
    addAndMakeVisible (leslieButton);
    leslieAtt = std::make_unique<APVTS::ButtonAttachment> (s, "leslieFast", leslieButton);

    setupKnob (oClick,     "click",      "Click");
    setupKnob (oDrive,     "drive",      "Drive");
    setupKnob (oLeslieMix, "leslieMix",  "Leslie Mix");
    setupKnob (oLevel,     "organLevel", "Organ Vol");
    setupKnob (mReverb,    "reverb",     "Reverb");

    // Eco de fita FitaEco TE-12
    addAndMakeVisible (echoButton);
    echoAtt = std::make_unique<APVTS::ButtonAttachment> (s, "echoOn", echoButton);
    for (int i = 1; i <= 12; ++i) echoModeBox.addItem ("Mode " + juce::String (i), i);
    addAndMakeVisible (echoModeBox);
    echoModeAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "echoMode", echoModeBox);
    setupKnob (eRate,      "echoRate",      "Rate");
    setupKnob (eIntensity, "echoIntensity", "Intensity");
    setupKnob (eVol,       "echoVol",       "Echo Vol");
    setupKnob (eSpring,    "echoSpring",    "Spring");
    setupKnob (eWow,       "echoWow",       "Wow");
    setupKnob (eAge,       "echoAge",       "Tape Age");

    // Maximizer + VU
    addAndMakeVisible (maxButton);
    maxAtt = std::make_unique<APVTS::ButtonAttachment> (s, "maxOn", maxButton);
    setupKnob (mxGain,  "maxGain",    "Gain");
    setupKnob (mxCeil,  "maxCeiling", "Ceiling");
    setupKnob (mMaster, "master",     "Master");
    grLabel.setFont (VintageLookAndFeel::monoFont (13.0f));
    grLabel.setJustificationType (juce::Justification::centred);
    grLabel.setColour (juce::Label::backgroundColourId, lcdBg);
    grLabel.setColour (juce::Label::textColourId, lcdText);
    grLabel.setColour (juce::Label::outlineColourId, juce::Colour (0xff0b0907));
    grLabel.setText ("GR  0.0", juce::dontSendNotification);
    addAndMakeVisible (grLabel);
    addAndMakeVisible (vuL);
    addAndMakeVisible (vuR);

    // Teclado: teclas creme, pintadas de laranja conforme o MIDI tocado
    keyboard.setAvailableRange (36, 96);   // C2..C7 (61 teclas, como um manual de Hammond)
    keyboard.setScrollButtonsVisible (false);
    keyboard.setWantsKeyboardFocus (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, cream);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff141210));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff6b5a44));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, orange.withAlpha (0.9f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, mustard.withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.4f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, brown);
    addAndMakeVisible (keyboard);

    setSize (1000, 724);
    startTimerHz (30);
}

RootsKeysEditor::~RootsKeysEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void RootsKeysEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& text)
{
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
    k.slider.setNumDecimalPlacesToDisplay (2);
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<APVTS::SliderAttachment> (proc.apvts, paramId, k.slider);
    k.label.setText (text.toUpperCase(), juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (VintageLookAndFeel::monoFont (11.0f));
    addAndMakeVisible (k.label);
}

void RootsKeysEditor::layoutKnobRow (juce::Rectangle<int> r, std::initializer_list<Knob*> knobs)
{
    const int w = r.getWidth() / (int) knobs.size();
    for (auto* k : knobs)
    {
        auto cell = r.removeFromLeft (w).reduced (2, 0);
        k->label.setBounds (cell.removeFromTop (14));
        k->slider.setBounds (cell);
    }
}

void RootsKeysEditor::timerCallback()
{
    vuL.update (proc.meterL.load(), 1.0f / 30.0f);
    vuR.update (proc.meterR.load(), 1.0f / 30.0f);
    const float gr = proc.gainReduction.load();
    grLabel.setText ("GR " + juce::String (gr, 1), juce::dontSendNotification);
}

void RootsKeysEditor::paint (juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();

    // gabinete de nogueira com veios
    g.fillAll (wood);
    juce::Random rnd (1977);
    for (int i = 0; i < 90; ++i)
    {
        juce::Path grain;
        const float y0 = rnd.nextFloat() * (float) H;
        const float amp = 1.0f + rnd.nextFloat() * 3.0f, freq = 0.004f + rnd.nextFloat() * 0.01f;
        grain.startNewSubPath (0.0f, y0);
        for (int xx = 8; xx <= W; xx += 8)
            grain.lineTo ((float) xx, y0 + amp * std::sin ((float) xx * freq + (float) i));
        g.setColour ((i % 3 == 0 ? woodDark : juce::Colour (0xff6b4226)).withAlpha (0.25f + 0.2f * rnd.nextFloat()));
        g.strokePath (grain, juce::PathStrokeType (0.8f + rnd.nextFloat()));
    }

    // cabeçalho escuro + faixas retrô 70s
    g.setColour (panelDark);
    g.fillRect (14, 0, W - 28, 70);
    const juce::Colour stripes[3] = { orange, mustard, brown };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (stripes[i]);
        g.fillRect (14, 70 + i * 4, W - 28, 4);
    }

    g.setColour (mustard);
    g.setFont (juce::Font (juce::FontOptions (17.0f, juce::Font::bold | juce::Font::italic)));
    g.drawText ("Mastor", 30, 6, 200, 20, juce::Justification::centredLeft);
    g.setColour (cream);
    g.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold | juce::Font::italic)));
    g.drawText ("Roots Keys", 30, 22, 260, 34, juce::Justification::centredLeft);
    g.setColour (mustard.withAlpha (0.8f));
    g.setFont (VintageLookAndFeel::monoFont (10.0f));
    g.drawText ("MODEL RK-1  v1.2", 32, 54, 200, 12, juce::Justification::centredLeft);

    metalPlate (g, plateArea);

    auto panel = [&] (juce::Rectangle<int> r, juce::Colour fill, const juce::String& title)
    {
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r.toFloat().translated (0.0f, 3.0f), 6.0f);
        g.setColour (fill);
        g.fillRoundedRectangle (r.toFloat(), 6.0f);
        g.setColour (silver.withAlpha (0.35f));
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 6.0f, 1.0f);
        const auto f = r.toFloat().reduced (8.0f);
        screw (g, f.getRight(), f.getY());
        screw (g, f.getX(), f.getBottom());
        screw (g, f.getRight(), f.getBottom());
        if (title.isNotEmpty()) plate (g, r, title);
    };
    panel (pianoArea, charcoal, "PIANO  M1 STYLE");
    panel (organArea, leather,  "TONEWHEEL ORGAN");
    panel (fxArea,    charcoal, "ORGAN FX / REVERB");
    panel (echoArea,  leather,  "TAPE ECHO  FITAECO TE-12");
    panel (outArea,   charcoal, "MAXIMIZER / VU");

    // cama do teclado com feltro vermelho
    g.setColour (woodDark);
    g.fillRoundedRectangle (keysArea.toFloat(), 6.0f);
    g.setColour (juce::Colour (0xff7a1a14));
    g.fillRect (keyboard.getX(), keyboard.getY() - 4, keyboard.getWidth(), 4);

    g.setColour (cream.withAlpha (0.45f));
    g.setFont (VintageLookAndFeel::monoFont (10.0f));
    g.drawText ("MOD WHEEL = LESLIE FAST  |  SUSTAIN = PIANO", organArea.getX() + 14,
                organArea.getBottom() - 20, organArea.getWidth() - 40, 14, juce::Justification::centredLeft);
}

void RootsKeysEditor::resized()
{
    auto area = getLocalBounds().reduced (24, 0);

    // cabeçalho: título | placa Dub Station | presets
    auto header = area.removeFromTop (70);
    plateArea = juce::Rectangle<int> (310, 12, 196, 46);
    auto presetRow = header.withTrimmedLeft (496).withSizeKeepingCentre (header.getWidth() - 496, 26);
    presetLabel.setBounds (presetRow.removeFromLeft (64));
    presetBox.setBounds (presetRow.removeFromLeft (212).reduced (3, 0));
    modeLabel.setBounds (presetRow.removeFromLeft (52));
    modeBox.setBounds (presetRow.reduced (3, 0));

    area.removeFromTop (20);
    area.removeFromBottom (12);

    // linha 1: piano | órgão
    auto row1 = area.removeFromTop (256);
    pianoArea = row1.removeFromLeft (250);
    row1.removeFromLeft (10);
    organArea = row1;
    area.removeFromTop (8);

    // linhas 2 e 3: FX do órgão / eco de fita | maximizer + VU
    auto rows23 = area.removeFromTop (256);
    outArea = rows23.removeFromRight (302);
    rows23.removeFromRight (10);
    fxArea = rows23.removeFromTop (124);
    rows23.removeFromTop (8);
    echoArea = rows23;
    area.removeFromTop (8);

    // teclado
    keysArea = area;

    // piano: 2x2 knobs
    {
        auto r = pianoArea.reduced (12).withTrimmedTop (24);
        const int w = r.getWidth() / 2, h = r.getHeight() / 2;
        Knob* ks[4] = { &pBright, &pDecay, &pChorus, &pLevel };
        for (int i = 0; i < 4; ++i)
        {
            auto cell = juce::Rectangle<int> (r.getX() + (i % 2) * w, r.getY() + (i / 2) * h, w, h).reduced (4);
            ks[i]->label.setBounds (cell.removeFromTop (16));
            ks[i]->slider.setBounds (cell);
        }
    }

    // órgão: drawbars à esquerda, chaves à direita
    {
        auto r = organArea.reduced (12).withTrimmedTop (26).withTrimmedBottom (20);
        auto bars = r.removeFromLeft (430);
        const int bw = bars.getWidth() / 9;
        for (int i = 0; i < 9; ++i)
        {
            auto cell = juce::Rectangle<int> (bars.getX() + i * bw, bars.getY(), bw, bars.getHeight());
            drawbars[i].label.setBounds (cell.removeFromTop (18));
            drawbars[i].slider.setBounds (cell.reduced (2, 0));
        }

        r.removeFromLeft (20);
        r.removeFromTop (10);
        percButton.setBounds (r.removeFromTop (28).removeFromLeft (190));
        r.removeFromTop (6);
        auto row = r.removeFromTop (26);
        percHarmBox.setBounds (row.removeFromLeft (84).reduced (2, 0));
        percDecayBox.setBounds (row.removeFromLeft (94).reduced (2, 0));
        r.removeFromTop (28);
        leslieButton.setBounds (r.removeFromTop (28).removeFromLeft (190));
    }

    // FX do órgão + reverb
    layoutKnobRow (fxArea.reduced (12, 8).withTrimmedTop (22), { &oClick, &oDrive, &oLeslieMix, &oLevel, &mReverb });

    // eco de fita: chave + sync à esquerda, 6 knobs
    {
        auto r = echoArea.reduced (12, 8).withTrimmedTop (22);
        auto left = r.removeFromLeft (124);
        left.removeFromTop (8);
        echoButton.setBounds (left.removeFromTop (28));
        left.removeFromTop (10);
        echoModeBox.setBounds (left.removeFromTop (26).reduced (2, 0));
        layoutKnobRow (r, { &eRate, &eIntensity, &eVol, &eSpring, &eWow, &eAge });
    }

    // maximizer + VU
    {
        auto r = outArea.reduced (12).withTrimmedTop (22);
        auto vus = r.removeFromTop (100);
        vuL.setBounds (vus.removeFromLeft (vus.getWidth() / 2).reduced (3, 2));
        vuR.setBounds (vus.reduced (3, 2));
        r.removeFromTop (6);
        auto left = r.removeFromLeft (92);
        left.removeFromTop (10);
        maxButton.setBounds (left.removeFromTop (28));
        left.removeFromTop (10);
        grLabel.setBounds (left.removeFromTop (24).reduced (2, 0));
        layoutKnobRow (r, { &mxGain, &mxCeil, &mMaster });
    }

    // teclado com 36 teclas brancas (C2..C7)
    auto kb = keysArea.reduced (10, 8).withTrimmedTop (6);
    keyboard.setBounds (kb);
    keyboard.setKeyWidth ((float) kb.getWidth() / 36.0f);
}
