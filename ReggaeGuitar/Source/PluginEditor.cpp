#include "PluginEditor.h"

namespace
{
    const juce::Colour cream   { 0xffe4d8bc };
    const juce::Colour creamHi { 0xffefe5cc };
    const juce::Colour ink     { 0xff2b241d };
    const juce::Colour inkSoft { 0xff4a4136 };
    const juce::Colour teal    { 0xff2c5650 };
    const juce::Colour amber   { 0xffffa928 };
    const juce::Colour green   { 0xff3be070 };
    const juce::Colour blue    { 0xff4d8ff0 };
    const juce::Colour purple  { 0xffa86ae8 };
    const juce::Colour orange  { 0xffff8a1e };
    const juce::Colour snare   { 0xffc8301e };
    constexpr int W = 1200, H = 900;
}

// ---------------------------------------------------------------- grade
void StepGrid::paint (juce::Graphics& g)
{
    static const char* lab[16] = { "1", ".", "e", ".", "2", ".", "e", ".", "3", ".", "e", ".", "4", ".", "e", "." };
    const float cw = (float) getWidth() / 16.0f, h = (float) getHeight();
    const int now = proc.statusStep.load();
    const int start = (int) proc.get ("varStart");
    const int stroke = (int) proc.get ("stroke");
    for (int i = 0; i < 16; ++i)
    {
        auto r = juce::Rectangle<float> (i * cw + 2.0f, 0.0f, cw - 4.0f, h).reduced (0, 2);
        const int v = (int) proc.get (fill ? ReggaeGuitarProcessor::fillId (i) : ReggaeGuitarProcessor::stepId (i));
        juce::Colour c = juce::Colour (i % 4 == 0 ? 0xff3a332b : 0xff221d18);
        juce::String mark;
        if (! fill)
        {
            if (v == 1) { c = amber; mark = stroke == mrg::StBangBang ? "||" : (stroke == mrg::StShuffle ? "|.|" : "|"); }
            if (v == 2) { c = green; mark = "."; }
        }
        else
        {
            static const juce::uint32 cols[6] = { 0, 0xffffa928, 0xffff8a1e, 0xff4d8ff0, 0xff3be070, 0xffa86ae8 };
            static const char* marks[6] = { "", "|", "||", "=", ".", "n" };
            if (v > 0) { c = juce::Colour (cols[v]); mark = marks[v]; }
        }
        juce::ColourGradient gr (c.brighter (0.25f), r.getX(), r.getY(), c.darker (0.35f), r.getX(), r.getBottom(), false);
        g.setGradientFill (gr);
        g.fillRoundedRectangle (r, 3.0f);
        if (fill && i < (start) * 4)
        { g.setColour (cream.withAlpha (0.6f)); g.fillRoundedRectangle (r, 3.0f); }
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        if (i == 4 || i == 12) { g.setColour (snare); g.drawRoundedRectangle (r.reduced (2.0f), 3.0f, 2.0f); }
        if (! fill && i == now) { g.setColour (juce::Colours::white.withAlpha (0.35f)); g.fillRoundedRectangle (r, 3.0f); }
        const bool lit = v > 0;
        g.setColour (lit ? ink : juce::Colour (0xff8f8470));
        g.setFont (VintageLookAndFeel::mono (11.0f));
        g.drawText (mark, r.withTrimmedBottom (r.getHeight() * 0.45f), juce::Justification::centredBottom);
        g.drawText (lab[i], r.withTrimmedTop (r.getHeight() * 0.55f), juce::Justification::centred);
    }
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    const int i = juce::jlimit (0, 15, (int) (e.position.x / ((float) getWidth() / 16.0f)));
    const juce::String id = fill ? ReggaeGuitarProcessor::fillId (i) : ReggaeGuitarProcessor::stepId (i);
    const int n = fill ? 6 : 3;
    proc.setParam (id, (float) (((int) proc.get (id) + 1) % n));
    if (fill) proc.setParam ("varType", (float) mrg::VarEditada);
    repaint();
}

void Scope::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff17110a)); g.fillRoundedRectangle (r, 3.0f);
    juce::Path p;
    const int w = proc.scopeWrite.load(), N = ReggaeGuitarProcessor::scopeSize;
    for (int x = 0; x < getWidth(); ++x)
    {
        const int idx = (w + (int) ((float) x / (float) getWidth() * (float) N)) % N;
        const float y = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, proc.scope[idx]) * r.getHeight() * 0.45f;
        if (x == 0) p.startNewSubPath ((float) x, y); else p.lineTo ((float) x, y);
    }
    g.setColour (amber); g.strokePath (p, juce::PathStrokeType (1.6f));
}

// ---------------------------------------------------------------- editor
ReggaeGuitarEditor::Knob& ReggaeGuitarEditor::knob (const juce::String& id, const juce::String& label)
{
    auto k = std::make_unique<Knob>();
    k->s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k->s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k->s.setPopupDisplayEnabled (true, true, this);
    k->label = label;
    addAndMakeVisible (k->s);
    k->a = std::make_unique<SA> (proc.apvts, id, k->s);
    byId[id] = &k->s;
    knobs.push_back (std::move (k));
    return *knobs.back();
}

ReggaeGuitarEditor::Toggle& ReggaeGuitarEditor::toggle (const juce::String& id, const juce::String& text)
{
    auto t = std::make_unique<Toggle>();
    t->b.setButtonText (text);
    t->b.getProperties().set ("style", "led");
    addAndMakeVisible (t->b);
    t->a = std::make_unique<BA> (proc.apvts, id, t->b);
    byId[id] = &t->b;
    toggles.push_back (std::move (t));
    return *toggles.back();
}

ReggaeGuitarEditor::Combo& ReggaeGuitarEditor::combo (const juce::String& id, const juce::String& label, bool attach)
{
    auto c = std::make_unique<Combo>();
    if (auto* prm = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        c->c.addItemList (prm->choices, 1);
    c->label = label;
    addAndMakeVisible (c->c);
    if (attach) c->a = std::make_unique<CA> (proc.apvts, id, c->c);
    byId[id] = &c->c;
    combos.push_back (std::move (c));
    return *combos.back();
}

ReggaeGuitarEditor::ReggaeGuitarEditor (ReggaeGuitarProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), mainGrid (p, false), fillGrid (p, true), scope (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    lnf.setSkin (1);
    setLookAndFeel (&lnf);

    // topo
    // guitarrista e modelo da variação não têm attachment: só o clique do usuário carrega o preset
    auto& pl = combo ("player", "GUITARRISTA", false);
    pl.c.onChange = [this, &pl] { if (pl.c.getSelectedItemIndex() >= 0) proc.applyPlayer (pl.c.getSelectedItemIndex()); };
    knob ("bpm", "BPM (SEM DAW)");
    toggle ("play", "PLAY");
    toggle ("groove", "GROOVE");
    toggle ("band", "ACOMPANHAMENTO");
    combo ("chordSrc", "ACORDES");
    for (int i = 1; i <= 4; ++i) { combo ("root" + juce::String (i), "ACORDE " + juce::String (i)); toggle ("minor" + juce::String (i), "m"); }
    for (auto* b : { &prevTrack, &nextTrack, &resetVar })
    { b->getProperties().set ("style", "chip"); addAndMakeVisible (*b); }
    prevTrack.onClick = [this] { proc.setTrack (proc.currentTrack() - 1); repaint(); };
    nextTrack.onClick = [this] { proc.setTrack (proc.currentTrack() + 1); repaint(); };

    // padrão e variação
    addAndMakeVisible (mainGrid);
    addAndMakeVisible (fillGrid);
    toggle ("varOn", "VARIACAO");
    auto& vt = combo ("varType", "MODELO", false);
    vt.c.onChange = [this, &vt] { const int t = vt.c.getSelectedItemIndex(); if (t >= 0 && t < mrg::VarEditada) proc.loadVariation (t); };
    combo ("varEvery", "FREQUENCIA");
    combo ("varStart", "COMECA NO TEMPO");
    knob ("varInt", "INTENSIDADE");
    resetVar.onClick = [this]
    {
        int t = (int) proc.get ("varType");
        if (t >= mrg::VarEditada) t = mrg::player ((int) proc.get ("player")).var;
        proc.loadVariation (t);
    };

    // golpe, guitarra, ampli
    combo ("stroke", "GOLPE"); combo ("mute", "ABAFAMENTO");
    knob ("lag", "ATRASO"); knob ("strum", "STRUM"); knob ("dur", "DURACAO");
    combo ("guitar", "GUITARRA"); combo ("pickup", "CAPTADOR");
    knob ("gvol", "VOLUME"); knob ("tone", "TONE");
    combo ("amp", "AMPLI");
    knob ("treble", "TREBLE"); knob ("middle", "MIDDLE"); knob ("bass", "BASS"); knob ("reverb", "REVERB");
    addAndMakeVisible (scope);

    // efeitos
    for (auto [on, k, name] : std::initializer_list<std::tuple<const char*, const char*, const char*>> {
            { "wahOn", "wah", "WAH" }, { "phOn", "ph", "PHASER" }, { "chOn", "ch", "CHORUS" },
            { "molaOn", "mola", "MOLA" }, { "echoOn", "echo", "TAPE ECHO" }, { "satOn", "sat", "SATURADOR" } })
    { knob (k, ""); toggle (on, name); }
    knob ("out", "SAIDA");
    combo ("art", "ARTICULACAO");

    keyboard.setAvailableRange (24, 96);
    keyboard.setKeyWidth (26.0f);
    addAndMakeVisible (keyboard);

    pl.c.setSelectedItemIndex ((int) proc.get ("player"), juce::dontSendNotification);
    vt.c.setSelectedItemIndex ((int) proc.get ("varType"), juce::dontSendNotification);
    setSize (W, H);
    startTimerHz (30);
}

ReggaeGuitarEditor::~ReggaeGuitarEditor() { setLookAndFeel (nullptr); }

juce::Rectangle<int> ReggaeGuitarEditor::section (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title) const
{
    auto f = r.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.45f)); g.drawRoundedRectangle (f.translated (1, 1), 4.0f, 1.2f);
    g.setColour (ink.withAlpha (0.55f)); g.drawRoundedRectangle (f, 4.0f, 1.2f);
    g.setFont (VintageLookAndFeel::mono (11.0f));
    const int tw = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), title) + 22;
    auto tag = juce::Rectangle<float> (f.getX() + 12, f.getY() - 10, (float) tw, 20);
    g.setColour (juce::Colour (0xff1a1714)); g.fillRoundedRectangle (tag, 2.0f);
    g.setColour (creamHi); g.drawText (title, tag, juce::Justification::centred);
    return r;
}

void ReggaeGuitarEditor::paint (juce::Graphics& g)
{
    // painel creme com madeira na lateral
    g.fillAll (juce::Colour (0xff2e1a0d));
    juce::ColourGradient bg (juce::Colour (0xffe8ddc2), 0, 0, juce::Colour (0xffc9b994), (float) W, (float) H, false);
    bg.addColour (0.5, cream);
    g.setGradientFill (bg);
    g.fillRect (20, 0, W - 20, H);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5e3a1e), 0, 0, juce::Colour (0xff2a170a), 20, 0, false));
    g.fillRect (0, 0, 20, H);

    // cabeçalho verde-petróleo, placa de latão e listras
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d6b64), 0, 0, juce::Colour (0xff244842), 0, 80, false));
    g.fillRect (20, 0, W - 20, 80);
    g.setColour (creamHi);
    g.setFont (VintageLookAndFeel::mono (28.0f));
    g.drawText ("MASTOR REGGAE GUITAR " + juce::String (JucePlugin_VersionString).upToLastOccurrenceOf (".", false, false),
                44, 12, 620, 36, juce::Justification::centredLeft);
    g.setFont (VintageLookAndFeel::mono (10.0f));
    g.setColour (juce::Colour (0xffd9ceb2));
    g.drawText (juce::String::fromUTF8 ("SKANK \xc2\xb7 BANG-BANG \xc2\xb7 PICKING \xc2\xb7 LEAD \xc2\xb7 CAIXA NO 2 E NO 4"), 44, 48, 620, 16, juce::Justification::centredLeft);
    {
        auto plate = juce::Rectangle<float> (690, 18, 190, 44);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2df9e), plate.getX(), plate.getY(), juce::Colour (0xff86692a), plate.getX(), plate.getBottom(), false));
        g.fillRoundedRectangle (plate, 5.0f);
        g.setColour (juce::Colour (0xff3a2a10));
        g.setFont (VintageLookAndFeel::mono (24.0f));
        g.drawText ("MRG-68", plate, juce::Justification::centred);
    }
    const juce::Colour stripes[3] = { juce::Colour (0xff0b8a3c), juce::Colour (0xffeabd12), juce::Colour (0xff141210) };
    for (int i = 0; i < 3; ++i) { g.setColour (stripes[i]); g.fillRoundedRectangle (960.0f, 18.0f + i * 8.0f, 210.0f, 6.0f, 3.0f); }
    g.setColour (creamHi); g.setFont (VintageLookAndFeel::mono (15.0f));
    g.drawText ("D U B S T A T I O N", 950, 46, 230, 20, juce::Justification::centredRight);

    section (g, rTop, juce::String::fromUTF8 ("GUITARRISTA \xc2\xb7 GROOVE"));
    section (g, rPattern, juce::String::fromUTF8 ("PADR\xc3\x83O \xc2\xb7 CAIXA NO 2 E NO 4 \xc2\xb7 SKANK NOS \"E\""));
    section (g, rVar, juce::String::fromUTF8 ("VARIA\xc3\x87\xc3\x83O \xc2\xb7 COMPASSO DA VARIA\xc3\x87\xc3\x83O"));
    section (g, rHand, juce::String::fromUTF8 ("GOLPE \xc2\xb7 M\xc3\x83O"));
    section (g, rGuitar, "GUITARRA");
    section (g, rAmp, "AMPLI " + juce::String::fromUTF8 ("\xc2\xb7") + " CAIXA");
    section (g, rFx, juce::String::fromUTF8 ("EFEITOS \xc2\xb7 ARTICULA\xc3\x87\xc3\x83O"));

    // visor da faixa de referência
    {
        const auto& pl = mrg::player ((int) proc.get ("player"));
        const auto& tr = pl.tracks[juce::jlimit (0, pl.numTracks - 1, proc.currentTrack())];
        auto v = juce::Rectangle<float> ((float) rTop.getX() + 250, (float) rTop.getY() + 20, 420, 36);
        g.setColour (juce::Colour (0xff17110a)); g.fillRoundedRectangle (v, 4.0f);
        g.setColour (amber); g.setFont (VintageLookAndFeel::mono (16.0f));
        g.drawText (tr.name, v.reduced (12, 0), juce::Justification::centredLeft);
        g.setFont (VintageLookAndFeel::mono (10.0f));
        g.drawText (juce::String (tr.info).toUpperCase() + "  " + juce::String (tr.bpm) + " BPM", v.reduced (12, 0), juce::Justification::centredRight);
    }

    // rótulos
    g.setColour (inkSoft);
    g.setFont (VintageLookAndFeel::mono (9.5f));
    for (auto& k : knobs)
        if (k->label.isNotEmpty())
            g.drawText (k->label, k->s.getBounds().translated (0, k->s.getHeight() - 2).withHeight (14).expanded (16, 0), juce::Justification::centred);
    for (auto& c : combos)
        g.drawText (c->label, c->c.getBounds().translated (0, -14).withHeight (13), juce::Justification::centredLeft);

    // luzes de compasso
    {
        const int every = (int) proc.get ("varEvery") == 0 ? 2 : ((int) proc.get ("varEvery") == 1 ? 4 : 8);
        const int bar = proc.statusBar.load();
        for (int b = 0; b < every; ++b)
        {
            auto r = juce::Rectangle<float> ((float) rVar.getRight() - 30.0f - (float) (every - b) * 26.0f, (float) rVar.getY() + 22, 20, 20);
            g.setColour (b == bar ? amber : juce::Colour (0xff2a241e)); g.fillEllipse (r);
            if (b == every - 1 && proc.get ("varOn") > 0.5f) { g.setColour (green); g.drawEllipse (r, 2.0f); }
            g.setColour (b == bar ? ink : juce::Colour (0xff8f8470)); g.setFont (VintageLookAndFeel::mono (10.0f));
            g.drawText (juce::String (b + 1), r, juce::Justification::centred);
        }
    }
    g.setColour (inkSoft); g.setFont (VintageLookAndFeel::mono (9.0f, false));
    g.drawText (juce::String::fromUTF8 ("TECLADO: C0-B0 ARTICULA\xc3\x87\xc3\x83O \xc2\xb7 C1-B2 ACORDES (SKANK) \xc2\xb7 C3 EM DIANTE NOTAS (PICKING E LEAD) \xc2\xb7 FORTE = SKANK, FRACO = GHOST"),
                rKeys.getX(), rKeys.getY() - 16, rKeys.getWidth(), 14, juce::Justification::centredLeft);
}

void ReggaeGuitarEditor::resized()
{
    const int x0 = 40, x1 = W - 20;
    rTop = { x0, 100, x1 - x0, 112 };
    rPattern = { x0, 228, x1 - x0, 86 };
    rVar = { x0, 330, x1 - x0, 150 };
    const int colW = (x1 - x0 - 32) / 3;
    rHand = { x0, 496, colW, 170 };
    rGuitar = { x0 + colW + 16, 496, colW, 170 };
    rAmp = { x0 + 2 * (colW + 16), 496, colW, 170 };
    rFx = { x0, 682, x1 - x0, 100 };
    rKeys = { x0, 812, x1 - x0, 80 };

    auto C = [this] (const juce::String& id) { return byId[id]; };
    // topo
    C ("player")->setBounds (rTop.getX() + 14, rTop.getY() + 30, 220, 26);
    prevTrack.setBounds (rTop.getX() + 676, rTop.getY() + 22, 30, 32);
    nextTrack.setBounds (rTop.getX() + 710, rTop.getY() + 22, 30, 32);
    C ("bpm")->setBounds (rTop.getX() + 1050, rTop.getY() + 8, 46, 46);
    C ("play")->setBounds (rTop.getX() + 830, rTop.getY() + 22, 90, 30);
    C ("groove")->setBounds (rTop.getX() + 925, rTop.getY() + 22, 100, 30);
    C ("band")->setBounds (rTop.getX() + 960, rTop.getY() + 72, 170, 28);
    C ("chordSrc")->setBounds (rTop.getX() + 14, rTop.getY() + 76, 220, 24);
    for (int i = 0; i < 4; ++i)
    {
        C ("root" + juce::String (i + 1))->setBounds (rTop.getX() + 250 + i * 162, rTop.getY() + 76, 80, 24);
        C ("minor" + juce::String (i + 1))->setBounds (rTop.getX() + 334 + i * 162, rTop.getY() + 74, 60, 28);
    }
    // padrão
    mainGrid.setBounds (rPattern.reduced (14, 0).withTrimmedTop (20).withTrimmedBottom (10));
    // variação
    C ("varOn")->setBounds (rVar.getX() + 14, rVar.getY() + 20, 120, 28);
    C ("varType")->setBounds (rVar.getX() + 150, rVar.getY() + 26, 140, 24);
    C ("varEvery")->setBounds (rVar.getX() + 306, rVar.getY() + 26, 140, 24);
    C ("varStart")->setBounds (rVar.getX() + 462, rVar.getY() + 26, 80, 24);
    C ("varInt")->setBounds (rVar.getX() + 560, rVar.getY() + 12, 44, 44);
    resetVar.setBounds (rVar.getX() + 630, rVar.getY() + 20, 170, 28);
    fillGrid.setBounds (rVar.getX() + 14, rVar.getY() + 72, rVar.getWidth() - 28, 66);
    // golpe
    C ("stroke")->setBounds (rHand.getX() + 14, rHand.getY() + 34, 150, 24);
    C ("mute")->setBounds (rHand.getX() + 180, rHand.getY() + 34, 160, 24);
    for (int i = 0; i < 3; ++i)
        C (juce::StringArray { "lag", "strum", "dur" }[i])->setBounds (rHand.getX() + 30 + i * 110, rHand.getY() + 76, 56, 56);
    // guitarra
    C ("guitar")->setBounds (rGuitar.getX() + 14, rGuitar.getY() + 34, 150, 24);
    C ("pickup")->setBounds (rGuitar.getX() + 180, rGuitar.getY() + 34, 150, 24);
    C ("gvol")->setBounds (rGuitar.getX() + 70, rGuitar.getY() + 76, 56, 56);
    C ("tone")->setBounds (rGuitar.getX() + 220, rGuitar.getY() + 76, 56, 56);
    // ampli
    C ("amp")->setBounds (rAmp.getX() + 14, rAmp.getY() + 34, 150, 24);
    scope.setBounds (rAmp.getX() + 180, rAmp.getY() + 24, rAmp.getWidth() - 194, 40);
    for (int i = 0; i < 4; ++i)
        C (juce::StringArray { "treble", "middle", "bass", "reverb" }[i])->setBounds (rAmp.getX() + 20 + i * 86, rAmp.getY() + 80, 50, 50);
    // efeitos
    const char* fxK[6] = { "wah", "ph", "ch", "mola", "echo", "sat" };
    const char* fxT[6] = { "wahOn", "phOn", "chOn", "molaOn", "echoOn", "satOn" };
    for (int i = 0; i < 6; ++i)
    {
        C (fxK[i])->setBounds (rFx.getX() + 24 + i * 140, rFx.getY() + 18, 44, 44);
        C (fxT[i])->setBounds (rFx.getX() + 6 + i * 140, rFx.getY() + 64, 128, 28);
    }
    C ("out")->setBounds (rFx.getX() + 870, rFx.getY() + 22, 50, 50);
    C ("art")->setBounds (rFx.getX() + 950, rFx.getY() + 40, 170, 24);
    keyboard.setBounds (rKeys);
}

void ReggaeGuitarEditor::timerCallback()
{
    for (const char* id : { "player", "varType" })
        if (auto* cb = dynamic_cast<juce::ComboBox*> (byId[id]))
            if (cb->getSelectedItemIndex() != (int) proc.get (id))
                cb->setSelectedItemIndex ((int) proc.get (id), juce::dontSendNotification);
    const int st = proc.statusStep.load(), bar = proc.statusBar.load();
    if (st != lastStep) { lastStep = st; mainGrid.repaint(); }
    if (bar != lastBar) { lastBar = bar; repaint (rVar); }
    fillGrid.repaint();
    scope.repaint();
    static int tick = 0;
    if (++tick % 15 == 0) repaint (rTop);
}
