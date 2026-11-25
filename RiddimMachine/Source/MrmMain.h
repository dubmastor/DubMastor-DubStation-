// Aba RIDDIM do MRM-0420: o resumo de tudo.
//  - HARMONIA: os 4 acordes (os mesmos controles do MRCM-1240), que valem para a harmonia e o baixo
//  - MÁQUINAS: cada máquina com a chave liga/desliga, os músicos clicáveis, o MIDI editável e o
//    ARRASTE só dela
//  - JACKPOT: a semente nos rolos e o botão NOVO RIDDIM
//  - RIDDIM: tamanho, ARRASTE O RIDDIM, EXPORTAR e o Big Knob no master
//  - alto-falante antigo com grade de metal no canto
#pragma once

#include "MrmProcessor.h"

namespace mrmui
{
//==============================================================================
// Chave liga/desliga de baquelite com lâmpada (ON verde / OFF apagada)
class PowerSwitch : public juce::Component
{
public:
    PowerSwitch (juce::RangedAudioParameter&, juce::String onText = "ON", juce::String offText = "OFF");
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    juce::String onText, offText;
    bool on = false;
};

// Músicos clicáveis: teclas de baquelite com lâmpada em grade (bateristas, padrões do skank, baixistas)
class MusicianKeys : public juce::Component
{
public:
    MusicianKeys (juce::RangedAudioParameter&, juce::StringArray names, int columns);
    std::function<void (int)> onSelect;   // chamado quando o usuário clica numa tecla
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
private:
    juce::Rectangle<float> keyRect (int i) const;
    int keyAt (juce::Point<float>) const;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    juce::StringArray names;
    int cols = 3, selected = 0;
};

// Tecla de MUTE de uma máquina na CONTROL ROOM: tampa colorida acesa enquanto a máquina soa,
// apagada com a lâmpada MUTE vermelha quando está mutada
class MuteKey : public juce::Component
{
public:
    MuteKey (juce::RangedAudioParameter&, juce::String title, juce::String sub, juce::Colour);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::ParameterAttachment att;
    juce::String title, sub;
    juce::Colour colour;
    bool muted = false;
};

// BIG KNOB na aba RIDDIM: o botão grande do filtro do Mastor Big Knob (arraste para cima/baixo,
// rodinha, duplo clique volta para 0 = filtro aberto)
class BigDial : public juce::Component
{
public:
    explicit BigDial (juce::RangedAudioParameter&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    float value = 0.0f, dragStart = 0.0f;
};

// FIRE da sirene: segura = toca (parâmetro sirFire do Big Knob), solta = as repetições continuam no delay
class SirenFire : public juce::Component
{
public:
    explicit SirenFire (juce::RangedAudioParameter&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::ParameterAttachment att;
    bool down = false;
};

// Seção BPM da aba RIDDIM: o BPM grande (clique e digite, arraste ou roda do mouse), -5 -1 +1 +5, TAP e os
// presets de BPM por época e estilo de reggae (BpmPresets.h), cada um numa tecla
class BpmPanel : public juce::Component, private juce::Timer
{
public:
    explicit BpmPanel (MrmProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void setBpm (float v);                 // 60..180, arredondado
    void tap (double ms);                  // TAP: média dos últimos toques
    int getChosen() const { return chosen; }
    void choosePreset (int i);
private:
    void timerCallback() override;
    void openEntry();
    void commitEntry();
    juce::Rectangle<int> display, steps[4], tapKey, stepL, stepR, presetLcd, keyArea (int i) const;
    void stepPreset (int delta);
    MrmProcessor& proc;
    juce::ParameterAttachment att;
    juce::TextEditor entry;
    float value = 75.0f, dragStart = 75.0f;
    int chosen = -1, pressed = -1;          // preset escolhido (some quando o BPM muda); tecla apertada
    bool dragging = false, dawShown = false;
    double dawBpm = 0.0;
    std::vector<double> taps;
    double tapFlash = 0.0;
};

// PRESETS no visor azul de MPC: estilo de reggae e música com as teclas ◀ ▶ (carrega na hora, como os presets
// do Dub Battery Reggae); clique no visor para a lista por estilo
class RiddimPresetPanel : public juce::Component, private juce::Timer
{
public:
    explicit RiddimPresetPanel (MrmProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void stepStyle (int delta);
    void stepSong (int delta);
    void showList();
private:
    void timerCallback() override;
    int current() const;
    juce::Array<int> songsOf (int style) const;
    int styleOf (int preset) const;
    MrmProcessor& proc;
    juce::Rectangle<int> lcd, keys[4], listKey;     // ◀ ▶ estilo, ◀ ▶ música
    int pressed = -1, shown = -2, styleSel = 0, shownPhase = -1;
};

// NOVA LINHA: outra sugestão só para a bateria ou só para o baixo (semente própria da máquina)
class NewLineKey : public juce::Component
{
public:
    NewLineKey (MrmProcessor& p, int m) : proc (p), machine (m) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent&) override;
    bool dimmed = false;
private:
    MrmProcessor& proc;
    int machine;
    bool down = false;
};

// Arraste para a timeline: uma máquina só (machine >= 0) ou o riddim inteiro (machine = -1)
class DragZone : public juce::Component
{
public:
    DragZone (MrmProcessor& p, int machineIn, juce::String titleIn, juce::String subIn)
        : proc (p), machine (machineIn), title (std::move (titleIn)), sub (std::move (subIn))
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
    bool dimmed = false;
private:
    MrmProcessor& proc;
    int machine;
    juce::String title, sub;
    bool dragging = false;
};

// Base dos visores de MIDI editáveis (vidro âmbar, compassos, cursor e botão DESFAZER EDIÇÃO)
class Lane : public juce::Component
{
public:
    explicit Lane (MrmProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::CrosshairCursor); }
    virtual void update() = 0;
protected:
    void paintFrame (juce::Graphics&, double totalBeats, double introBeats, double playBeat, const juce::StringArray& rowNames);
    juce::Rectangle<float> rollArea() const;     // área das notas (sem a coluna dos nomes e a régua)
    juce::Rectangle<float> rowArea (int row, int numRows) const;
    double beatAtX (float x, double totalBeats) const;
    float  xOfBeat (double beat, double totalBeats) const;
    MrmProcessor& proc;
};

// Bateria do MRDM-12: bumbo, caixa, aro, hi-hat e hi-hat aberto em semicolcheias.
// Clique põe/tira, arrastar pinta, rodinha muda a força (as mesmas edições da grade PADRÃO).
class DrumLane : public Lane
{
public:
    using Lane::Lane;
    void update() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    bool hit (juce::Point<float>, int& row, double& beat) const;
    dubdrums::Pattern pattern;
    int version = -1;
    bool dragAdd = true;
    int lastRow = -1;
    double lastBeat = -1.0;
};

// Harmonia do MRCM-1240: skank (piano) e bubble (órgão).
// Clique num espaço vazio põe um ataque do acorde; clique num ataque tira. Rodinha muda a força.
class ChordLane : public Lane
{
public:
    using Lane::Lane;
    void update() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    bool hit (juce::Point<float>, int& row, double& beat) const;
    std::vector<dub::NoteEvent> events;
    std::vector<dub::ChordSlot> slots;
    double loopBeats = 0.0;
};

// JACKPOT: três rolos de máquina caça-níquel com a semente (padrão MRDM-12: os números giram e param um a um)
class JackpotReels : public juce::Component
{
public:
    explicit JackpotReels (juce::RangedAudioParameter&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool animating() const;
private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    int value = 1, shown = -1, dragStart = 1;
    double animStart = -10000.0;
};

// NOVO RIDDIM: botão de fliperama grande, vermelho, com luzes de cassino correndo em volta
class JackpotButton : public juce::Component
{
public:
    JackpotButton() { setMouseCursor (juce::MouseCursor::PointingHandCursor); setRepaintsOnMouseActivity (true); }
    std::function<void()> onClick;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        down = false;
        repaint();
        if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
    }
    double phase = 0.0;     // animação das luzes (s)
    bool spinning = false;  // os rolos estão girando
private:
    bool down = false;
};

// ALAVANCA do JACKPOT: puxe (ou clique) e ela desce, solta e volta com mola; no fundo do curso sorteia tudo
class JackpotLever : public juce::Component
{
public:
    JackpotLever() { setMouseCursor (juce::MouseCursor::PointingHandCursor); setRepaintsOnMouseActivity (true); }
    std::function<void()> onPull;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void tick (double dt);          // animação (chamada pelo timer da página)
    float pull = 0.0f;              // 0 = em cima, 1 = no fundo
private:
    enum class State { Idle, Dragging, Going, Returning } state = State::Idle;
    float dragStartPull = 0.0f;
    bool fired = false;
};

// Alto-falante antigo: caixa de madeira com grade de metal perfurada; o cone mexe com o som
class GrilleSpeaker : public juce::Component
{
public:
    GrilleSpeaker() { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    void resized() override { grille = {}; }
    float level = 0.0f;
private:
    juce::Image grille;   // grade desenhada uma vez
};

//==============================================================================
class MainPage : public juce::Component, private juce::Timer
{
public:
    static constexpr int kW = 1280, kH = 1416;
    explicit MainPage (MrmProcessor&);
    ~MainPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void renderBackground();
    void exportRiddim();

    struct ChordStrip
    {
        std::unique_ptr<NoteKeys> keys;
        std::unique_ptr<ModeSwitch> mode;
        std::unique_ptr<InversionSwitch> inv;
        std::unique_ptr<ChoiceWindow> dur;
        juce::ToggleButton on;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
        juce::Rectangle<int> area, display;
    };
    struct MachineRow
    {
        std::unique_ptr<PowerSwitch> power;
        std::unique_ptr<MusicianKeys> musicians;
        std::unique_ptr<DragZone> drag;
        juce::TextButton undo { juce::String::fromUTF8 ("DESFAZER EDI\xc3\x87\xc3\x83O") };
        Lane* lane = nullptr;
        juce::Rectangle<int> area, plateArea;
    };

    MrmProcessor& proc;
    DubLookAndFeel lnf;      // LookAndFeel do MRCM-1240 (chaves ON e EXPORTAR)
    std::array<ChordStrip, DubChordsProcessor::numSlots> strips;
    dub::Voicings voicings;

    DrumLane drumLane;
    ChordLane chordLane;
    BassRoll bassRoll;
    struct BassLaneHost : public Lane
    {
        BassLaneHost (MrmProcessor& p, BassRoll& r) : Lane (p), roll (r) { addAndMakeVisible (roll); }
        void update() override { roll.update(); }
        void resized() override { roll.setBounds (getLocalBounds()); }
        void paint (juce::Graphics&) override {}
        BassRoll& roll;
    } bassLane;
    std::array<MachineRow, MrmProcessor::NumMachines> rows;

    JackpotReels reels;
    JackpotButton jackpot;
    JackpotLever lever;
    juce::TextButton presets { juce::String::fromUTF8 ("PRESETS \xe2\x96\xbe") };
    void showPresets();
    std::unique_ptr<ChoiceWindow> length;
    DragZone riddimDrag;
    juce::TextButton exportBtn { "EXPORTAR" };
    std::unique_ptr<PowerSwitch> fxPower, echoPower;
    std::unique_ptr<MusicianKeys> drumStyle, filterMode;   // estilo de reggae da bateria; HIGH/LOW PASS do Big Knob
    std::unique_ptr<MusicianKeys> bassTimbre;              // TIMBRE do baixo: ROOTS, SUPER TONE (Casio da era digital) ou 808
    std::unique_ptr<BigDial> bigKnob;
    std::unique_ptr<PowerSwitch> knobBypass, sirenHold, sirenEcho;      // BYPASS do Big Knob; HOLD e TAPE ECHO da sirene
    std::unique_ptr<SirenFire> sirenFire;
    std::unique_ptr<MusicianKeys> sirenWave;
    std::array<std::unique_ptr<BigDial>, 6> sirenDials;                 // PITCH, RATE, DEPTH, DELAY, REPEATS, LEVEL
    std::unique_ptr<MuteKey> muteDrums, muteBass, muteRiddim;
    BpmPanel bpmPanel;
    RiddimPresetPanel presetPanel;
    std::unique_ptr<NewLineKey> newDrums, newBass;
    GrilleSpeaker speaker;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image background;
    juce::Rectangle<int> woodL, woodR, plate, harmonySection, machineSection, jackpotSection, riddimSection, speakerArea, knobSection, sirenSection, bpmBox, presetBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPage)
};
} // namespace mrmui
