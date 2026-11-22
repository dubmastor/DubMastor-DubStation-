#pragma once

// Interface do Dub Drums no mesmo visual do DubMastor Dub Chords:
// painel creme envelhecido, laterais de madeira, módulos com plaqueta preta,
// displays âmbar, botões de pressão com lâmpada piloto e knobs de baquelite.

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class DubLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DubLookAndFeel();
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
    explicit MidiDragZone (DubDrumsProcessor& p) : proc (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }

private:
    DubDrumsProcessor& proc;
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
    int  index() const    { return juce::roundToInt (value); }
    void select (int i)   { att.setValueAsCompleteGesture ((float) i); }
    void beginGesture()   { att.beginGesture(); }
    void setDuring (float v) { att.setValueAsPartOfGesture (v); }
    void endGesture()     { att.endGesture(); }

    juce::RangedAudioParameter& param;
    float value = 0.0f;

private:
    juce::ParameterAttachment att;
};

// Botões de pressão intertravados com lâmpada (escolha um), em grade de linhas x colunas
class ChoiceButtons : public ParamControl
{
public:
    ChoiceButtons (juce::RangedAudioParameter& p, juce::StringArray labels, int columns, bool compactCaps = false)
        : ParamControl (p), names (std::move (labels)), cols (columns), compact (compactCaps) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<void (int)> onSelect;   // chamado quando o usuário escolhe (não na automação)

private:
    juce::Rectangle<float> cell (int i) const;
    juce::StringArray names;
    int cols;
    bool compact;
};

// BATIDA PRINCIPAL: bumbo e caixa por colcheia (1 e 2 e 3 e 4 e). Clique liga/desliga;
// arrastar pinta (ou apaga) várias de uma vez.
class BeatGrid : public juce::Component
{
public:
    BeatGrid (juce::RangedAudioParameter& kick, juce::RangedAudioParameter& snare);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> cell (int row, int col) const;
    bool hit (juce::Point<float>, int& row, int& col) const;
    void setCell (int row, int col, bool on);
    juce::ParameterAttachment kickAtt, snareAtt;
    int masks[2] { 0, 0 };
    int dragRow = -1;
    bool dragValue = false;
};

// Chave seletora do baterista: knob "cabeça de galinha" de baquelite com três posições,
// lâmpada e nome gravado em cada posição. Clique no nome, arraste o knob ou use a rodinha.
class DrummerSwitch : public ParamControl
{
public:
    using ParamControl::ParamControl;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Point<float> centre() const;
    float radius() const;
    juce::Rectangle<float> labelArea (int i) const;
    static float angleOf (int i) { return juce::MathConstants<float>::pi * 0.21f * ((float) i - 2.0f); }   // 5 posições
    bool dragging = false;
    int dragStartIndex = 0;
};

// Compassos: janela de contador mecânico; clique à direita avança, à esquerda volta
class BarsWindow : public ParamControl
{
public:
    using ParamControl::ParamControl;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override { side = e.position.x < (float) getWidth() * 0.5f ? -1 : 1; repaint(); }
    void mouseExit (const juce::MouseEvent&) override   { side = 0; repaint(); }

private:
    int side = 0;
};

// Semente: display âmbar; arraste para cima/baixo ou use a rodinha
class SeedWindow : public ParamControl
{
public:
    explicit SeedWindow (juce::RangedAudioParameter& p) : ParamControl (p) { setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { dragStart = index(); beginGesture(); }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override   { endGesture(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool animating() const;   // JACKPOT: os números rodam quando a semente muda

private:
    int dragStart = 1;
    int shown = -1;
    double animStart = -10000.0;
    bool userDrag = false;
};

// Nota MIDI de uma peça (opção avançada): display âmbar com setas; clique nas setas, arraste ou use a rodinha
class NoteWindow : public ParamControl
{
public:
    using ParamControl::ParamControl;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { if (dragging) endGesture(); dragging = false; }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void setNote (int n) { select (juce::jlimit (0, 127, n)); }

private:
    int dragStart = 0;
    bool dragging = false;
};

// Botão de pressão simples (lâmpada acesa enquanto aperta)
class PushButton : public juce::Component
{
public:
    explicit PushButton (juce::String t, bool redCap = false) : text (std::move (t)), red (redCap)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
    std::function<void()> onClick;
    float attention = 0.0f;   // 0..1: brilho pulsando para chamar atenção (NOVA VARIAÇÃO)
    float sheen = -1.0f;      // posição do reflexo que passa pelo botão (-1 = sem reflexo)
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        down = false;
        repaint();
        if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
    }

private:
    juce::String text;
    bool red = false;
    bool down = false;
};

// PREVIEW: botão grande iluminado (verde tocando, com a lâmpada pulsando)
class DrumsPreviewButton : public juce::ToggleButton
{
public:
    DrumsPreviewButton() { setMouseCursor (juce::MouseCursor::PointingHandCursor); setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
    float pulse = 0.0f;   // 0..1, animado pelo painel enquanto toca
};

// Alto-falante vintage no meio do SOM: o cone mexe com o som que sai do plugin.
// Três desenhos de época: 0 = gabinete de bateria eletrônica valvulada de 1959 (tweed + tweeter em cima),
// 1 = rádio valvulado de mesa dos anos 50 (baquelite, frisos cromados e olho mágico verde),
// 2 = rádio transistorizado dos anos 60 (alumínio perfurado).
class VintageSpeaker : public juce::Component
{
public:
    VintageSpeaker() { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    int design = 0;
    float level = 0.0f;   // 0..1
};

//==============================================================================
// Opção avançada: qual nota MIDI cada peça da bateria usa
class NoteMapPanel : public juce::Component
{
public:
    explicit NoteMapPanel (DubDrumsProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;   // clique fora da placa fecha
    std::function<void()> onClose;

private:
    DubDrumsProcessor& proc;
    std::vector<std::unique_ptr<NoteWindow>> notes;
    PushButton resetGm { "PADRAO GM" }, close { "FECHAR" };
    juce::Rectangle<int> card;
};

//==============================================================================
// Painel com todo o layout no tamanho base; o editor o escala para a janela redimensionável
class DubDrumsPanel : public juce::Component, private juce::Timer
{
public:
    explicit DubDrumsPanel (DubDrumsProcessor&);
    ~DubDrumsPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // grade PADRÃO editável: clique põe/tira nota, arrastar pinta, rodinha muda a força
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    bool gridHit (juce::Point<float>, int& row, double& beat) const;
    int  barTagAt (juce::Point<float>) const;          // faixa "COMPASSO n" / "INTRO" no PADRÃO (-3 = nenhuma)
    void dragBar (int bar);                            // arrasta só esse compasso para a DAW
    std::vector<std::pair<juce::Rectangle<float>, int>> barTags;
    int pressedBar = -3;
    bool barDragged = false;
    bool dragAdd = true, dragging = false;
    int lastRow = -1, lastCell = -1;

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

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name);
    void setupToggle (Toggle&, const juce::String& paramId, const juce::String& text);
    void placeKnob (Knob&, juce::Rectangle<int>);
    void renderBackground();
    void drawKnobScale (juce::Graphics&, juce::Slider&, int ticks, const juce::StringArray& labels = {});
    void drawPattern (juce::Graphics&, juce::Rectangle<float>);
    void exportMidi();
    int choice (const char* id) const;

    DubDrumsProcessor& proc;
    DubLookAndFeel lnf;

    ChoiceButtons styleButtons, hatButtons, backbeatButtons, kitButtons;
    DrummerSwitch drummerSwitch;
    BarsWindow bars;
    SeedWindow seed;
    PushButton variation { "NOVA VARIACAO", true };
    BeatGrid beatGrid;
    PushButton notesButton { "NOTAS" };
    Toggle fill, intro, swingAll, sound, colorMidi;
    DrumsPreviewButton previewButton;
  public:
    VintageSpeaker speaker;
  private:
    std::unique_ptr<ButtonAttach> previewAtt;
    PushButton clearEdits { "DESFAZER EDICAO" };
    bool showingEdits = false;
    Knob swing, dynamics;
    Knob rndKick, rndSnare, rndHat, rndPerc, channel, bpm, volume;
    NoteMapPanel noteMap;
    juce::TextButton exportBtn { "EXPORTAR" };
    MidiDragZone dragZone;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image background;
    juce::Rectangle<int> woodL, woodR, plate;
    juce::Rectangle<int> styleMod, loopMod, drumMod, hatMod, dynMod, randPanel, soundPanel, midiPanel, grid;
    juce::Rectangle<int> drummerDisplay;
    juce::Rectangle<int> styleDisplay, hatDisplay, dynDisplay;
    dubdrums::Pattern pattern;
    int lastVersion = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubDrumsPanel)
};

//==============================================================================
class DubDrumsEditor : public juce::AudioProcessorEditor
{
public:
    explicit DubDrumsEditor (DubDrumsProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        const int w = panel.getWidth(), h = panel.getHeight();
        setResizable (true, true);
        setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        setSize (w, h);
    }
    void resized() override
    {
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) panel.getWidth()));
    }

private:
    DubDrumsPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubDrumsEditor)
};
