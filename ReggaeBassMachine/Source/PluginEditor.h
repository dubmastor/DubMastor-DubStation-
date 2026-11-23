#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Mesmo visual da família DubMastor (Big Knob / Dub Chords): painel creme, madeira, baquelite,
// lâmpadas e displays âmbar; osciloscópio de tubo verde do Mastor Fone.
class BassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BassLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

//==============================================================================
// Área que você arrasta direto para a timeline da DAW
class MidiDragZone : public juce::Component
{
public:
    explicit MidiDragZone (BassMachineProcessor& p) : proc (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }

private:
    BassMachineProcessor& proc;
    bool dragging = false;
};

//==============================================================================
// Base dos controles clicáveis: liga o componente a um parâmetro (com undo/automação da DAW)
class ParamControl : public juce::Component
{
public:
    explicit ParamControl (juce::RangedAudioParameter& p)
        : param (p), att (p, [this] (float v) { value = v; repaint(); }, nullptr)
    {
        att.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
protected:
    int  index() const  { return juce::roundToInt (value); }
    void select (int i) { att.setValueAsCompleteGesture ((float) i); }

    juce::RangedAudioParameter& param;
    float value = 0.0f;
private:
    juce::ParameterAttachment att;
};

// Tônica: teclado mecânico de uma oitava
class NoteKeys : public ParamControl
{
public:
    using ParamControl::ParamControl;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent& e) override { hover = keyAt (e.position); repaint(); }
    void mouseExit (const juce::MouseEvent&) override   { hover = -1; repaint(); }
private:
    juce::Rectangle<float> keyRect (int note) const;
    int keyAt (juce::Point<float>) const;
    int hover = -1;
};

// Tipo do acorde: 4 botões de pressão intertravados (MAIOR, MENOR, 7, m7)
class QualitySwitch : public ParamControl
{
public:
    using ParamControl::ParamControl;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
};

// Janela de contador mecânico com setas: duração do acorde ou número da IDEIA
class CounterWindow : public ParamControl
{
public:
    enum Kind { Duration, Idea };
    CounterWindow (juce::RangedAudioParameter& p, Kind k) : ParamControl (p), kind (k) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override { side = e.position.x < (float) getWidth() * 0.5f ? -1 : 1; repaint(); }
    void mouseExit (const juce::MouseEvent&) override   { side = 0; repaint(); }
private:
    int minIndex() const;
    int maxIndex() const;
    Kind kind;
    int side = 0;
};

// Botoeira de teclas intertravadas (seletor de rádio valvulado / jukebox): apertar uma solta a outra
class InterlockKeys : public ParamControl
{
public:
    InterlockKeys (juce::RangedAudioParameter& p, juce::StringArray names, bool isVertical)
        : ParamControl (p), labels (std::move (names)), vertical (isVertical) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent&) override { repaint(); }
private:
    juce::Rectangle<float> keyRect (int i) const;
    juce::StringArray labels;
    bool vertical;
};

// PLAY / STOP do preview: tampa grande de botão de pedal com lâmpada
class PlayButton : public ParamControl
{
public:
    using ParamControl::ParamControl;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { select (index() == 0 ? 1 : 0); }
};

// Visor da linha de baixo: rolo de piano âmbar com os acordes e o cursor de reprodução
// Visor editável: clique num espaço vazio põe nota; arraste a nota para mover (tempo e altura),
// arraste a borda direita para esticar; duplo clique (ou botão direito) apaga. Grade de 1/16 (Shift = 1/32).
class BassRoll : public juce::Component
{
public:
    explicit BassRoll (BassMachineProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::CrosshairCursor); }
    void update();   // chamado pelo timer do painel
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
private:
    void computeGeometry();
    int noteAt (juce::Point<float>, bool& onRightEdge) const;
    double beatAt (float x) const;
    int pitchAt (float y) const;
    juce::Rectangle<float> noteRect (const mbm::NoteEvent&) const;

    BassMachineProcessor& proc;
    std::vector<mbm::NoteEvent> events;
    std::vector<mbm::ChordSlot> slots;
    double loopBeats = 0.0;
    bool editedLine = false;

    // geometria do visor (congelada durante um arraste)
    juce::Rectangle<float> area, header, keysCol;
    int lo = 28, hi = 52;
    float rowH = 6.0f;

    enum class Drag { None, Move, Resize } drag = Drag::None;
    int dragIndex = -1;
    mbm::NoteEvent dragOrig {};
    juce::Point<float> dragFrom;
    bool dragChanged = false;
};

// Osciloscópio de tubo verde: mostra a onda que sai do som interno
class CrtScope : public juce::Component
{
public:
    explicit CrtScope (BassMachineProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    void update();
private:
    BassMachineProcessor& proc;
    std::vector<float> buf = std::vector<float> (4096), trace, prev1, prev2;
};

// RIDDIM CLÁSSICO: visor âmbar com o riddim carregado; clique abre o menu (por estilo, baixista, década),
// as setas passam para o anterior / próximo
class RiddimBox : public juce::Component
{
public:
    explicit RiddimBox (BassMachineProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void refresh() { if (shown != proc.currentRiddim() || shownBank != proc.currentBank()) { shown = proc.currentRiddim(); shownBank = proc.currentBank(); repaint(); } }
private:
    void showMenu();
    void load (int index) { proc.loadRiddim (index); shown = -2; refresh(); }
    void loadBank (int index) { proc.applyBankEntry (index); shown = -2; refresh(); }
    BassMachineProcessor& proc;
    int shown = -2, shownBank = -2;
};

//==============================================================================
// Painel com todo o layout no tamanho base; o editor o escala para a janela redimensionável
class BassMachinePanel : public juce::Component, private juce::Timer
{
public:
    explicit BassMachinePanel (BassMachineProcessor&);
    ~BassMachinePanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kW = 1240, kH = 1064;

private:
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttach> att;
    };
    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<ButtonAttach> att;
    };
    struct ChordModule
    {
        std::unique_ptr<NoteKeys> keys;
        std::unique_ptr<QualitySwitch> qual;
        std::unique_ptr<CounterWindow> dur;
        Toggle on;                       // só nos acordes 2, 3 e 4
        juce::Rectangle<int> area, display;
    };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name, const juce::String& sliderName = {});
    void setupToggle (Toggle&, const juce::String& paramId, const juce::String& text);
    void placeKnob (Knob&, juce::Rectangle<int>);
    void renderBackground();
    void drawKnobScale (juce::Graphics&, juce::Slider&, int ticks, const juce::StringArray& labels = {}, float labelW = 44.0f);
    void exportMidi();
    bool slotEnabled (int i) const;

    BassMachineProcessor& proc;
    BassLookAndFeel lnf;

    std::array<ChordModule, BassMachineProcessor::numSlots> mods;
    std::unique_ptr<InterlockKeys> styleKeys, drumKeys, keysKeys, timbreKeys;
    juce::TextButton clearEditsBtn { juce::String::fromUTF8 ("DESFAZER EDIÇÃO") };

    // letreiro de palhetas (split-flap) com o nome do baixista
    void drawFlapBoard (juce::Graphics&, juce::Rectangle<float>);
    int flapStyle = -1;
    double flapStart = 0.0;
    Knob density, variation, swing, base;
    Knob velocity, gate;
    Knob bpm;
    Knob inGain, tone, drive, volume;
    Knob reps;
    Toggle sound, drumsOn, keysOn;
    Knob drumVol, keysVol;
    std::unique_ptr<CounterWindow> idea;
    std::unique_ptr<PlayButton> play;
    juce::TextButton newIdeaBtn { "NOVA IDEIA" };
    juce::TextButton exportBtn { "EXPORTAR" };
    MidiDragZone dragZone;
    BassRoll roll;
    CrtScope scope;
    RiddimBox riddimBox;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image background;
    juce::Rectangle<int> woodL, woodR, plate;
    juce::Rectangle<int> stylePanel, harmonyPanel, linePanel, previewPanel, timbrePanel, scopePanel, midiPanel, drumsPanel, keysPanel;
    juce::Rectangle<int> styleDisplay, timbreDisplay, ideaHint, timbreKeysLabel;
    std::array<bool, BassMachineProcessor::numSlots> lastOn {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassMachinePanel)
};

//==============================================================================
class BassMachineEditor : public juce::AudioProcessorEditor
{
public:
    explicit BassMachineEditor (BassMachineProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        const int w = panel.getWidth(), h = panel.getHeight();
        setResizable (true, true);
        setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        setSize (w * 9 / 10, h * 9 / 10);
    }
    void resized() override
    {
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) panel.getWidth()));
    }
private:
    BassMachinePanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassMachineEditor)
};
