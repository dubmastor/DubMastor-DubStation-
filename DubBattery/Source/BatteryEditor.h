// Interface do DUB BATTERY REGGAE 1.0 no visual Dub Station (painel creme envelhecido, laterais de madeira,
// plaquetas pretas, displays âmbar) com os pads de borracha de uma MPC que acendem por dentro como na SP-404
// e um visor LCD âmbar no estilo do Oberheim DMX.
//   Cabeçalho: nome, abas BATTERY / DUB DRUMS, PLAY GROOVE e PRESETS (◀ ▶ e GRAVAR).
//   Aba BATTERY: visor do SAMPLE (início e fim arrastáveis), ENVELOPE gráfico (AHDSR), 16 PADS,
//                controles do pad escolhido, CHOKE e MASTER.
//   Aba DUB DRUMS: o Mastor Reggae Drum Machine MRDM-12 inteiro (exportar / arrastar o MIDI).
#pragma once

#include "BatteryProcessor.h"

#include "DrumsNames.h"
#include "../../DubDrums/Source/PluginEditor.h"
#include "DrumsNamesEnd.h"

namespace dbrui
{
//==============================================================================
// Tecla de pressão com lâmpada piloto (aba, PLAY GROOVE, botões do visor)
class Key : public juce::Component
{
public:
    enum class Cap { Dark, Green, Red };
    Key (juce::String titleIn, juce::String subIn = {}, Cap capIn = Cap::Dark)
        : title (std::move (titleIn)), sub (std::move (subIn)), cap (capIn)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
    void setLit (bool l) { if (l != lit) { lit = l; repaint(); } }
    void setTitle (const juce::String& t) { title = t; repaint(); }
    std::function<void()> onClick;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        down = false;
        repaint();
        if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
    }
    bool showLamp = true;

private:
    juce::String title, sub;
    Cap cap;
    bool lit = false, down = false;
};

// Botões intertravados com lâmpada ligados a um parâmetro (escolha ou liga/desliga). Pode trocar de pad.
class ChoiceRow : public juce::Component
{
public:
    ChoiceRow (juce::StringArray labelsIn, int columns) : labels (std::move (labelsIn)), cols (columns)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
    void bind (juce::RangedAudioParameter&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool toggle = false;   // um botão só que liga/desliga um parâmetro bool

private:
    juce::Rectangle<float> cell (int i) const;
    juce::StringArray labels;
    int cols;
    float value = 0.0f;
    std::unique_ptr<juce::ParameterAttachment> att;
};

// Knob de baquelite (o mesmo do MRDM-12) com nome em cima e valor num display âmbar embaixo
class KnobBox : public juce::Component
{
public:
    KnobBox (juce::String captionIn, std::function<juce::String (float)> fmt);
    ~KnobBox() override;
    void bind (juce::AudioProcessorValueTreeState&, const juce::String& paramId);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;

private:
    juce::String caption;
    std::function<juce::String (float)> format;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    DrumsLookAndFeel lnf;   // o LookAndFeel do MRDM-12 (renomeado em DrumsNames.h)
};

//==============================================================================
// Pad de borracha (MPC) que acende em âmbar por dentro (SP-404). Clique toca e seleciona;
// a altura do clique dá a força (em cima = mais forte). Solte um WAV/AIFF em cima para carregar.
class Pad : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    Pad (BatteryProcessor&, int index);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dropHover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override             { dropHover = false; repaint(); }
    void filesDropped (const juce::StringArray&, int, int) override;
    std::function<void (int)> onSelect, onLoaded;

    float flash = 0.0f;       // 0..1, aceso quando toca
    bool selected = false;
    juce::uint32 lastHit = 0;

private:
    BatteryProcessor& proc;
    int index;
    bool dropHover = false;
};

// Visor do SAMPLE: forma de onda do pad escolhido, início e fim arrastáveis, posição tocando
class SampleVisor : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit SampleVisor (BatteryProcessor&);
    void setPad (int pad);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    std::function<void()> onLoaded;
    void tick();

private:
    juce::Rectangle<float> waveArea() const;
    float xOf (float frac) const;
    float fracOf (float x) const;
    BatteryProcessor& proc;
    int pad = 0;
    int dragging = 0, hover = 0;   // 1 = início, 2 = fim
    float lastPlay = -2.0f, lastStart = -1.0f, lastEnd = -1.0f;
    const void* lastSample = nullptr;
};

// ENVELOPE gráfico (AHDSR): arraste os pontos A, H, D/S e R
class EnvelopeView : public juce::Component
{
public:
    explicit EnvelopeView (BatteryProcessor&);
    void setPad (int pad);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void tick();

private:
    struct Shape { juce::Point<float> a, h, d, s, r; juce::Rectangle<float> area; };
    Shape shape() const;
    juce::RangedAudioParameter& prm (dbr::P p) const { return proc.padParam (pad, p); }
    float val (dbr::P p) const { return proc.padValue (pad, p); }
    void setVal (dbr::P p, float plain);
    BatteryProcessor& proc;
    int pad = 0;
    int dragging = 0, hover = 0;   // 1 = A, 2 = H, 3 = D/S, 4 = R
    juce::String lastKey;
};

// Display âmbar com setas < > ligado a um parâmetro de escolha (TIMBRE 2, JUNTAR COM). Rodinha também troca.
class StepSelector : public juce::Component
{
public:
    StepSelector (juce::String captionIn, std::function<juce::String (int)> textIn)
        : caption (std::move (captionIn)), text (std::move (textIn))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
    void bind (juce::RangedAudioParameter&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    std::function<void()> onChange;

private:
    void step (int delta);
    juce::String caption;
    std::function<juce::String (int)> text;
    juce::RangedAudioParameter* param = nullptr;
    float value = 0.0f;
    std::unique_ptr<juce::ParameterAttachment> att;
};

// Valor num display âmbar que se ajusta arrastando para cima/baixo (rodinha; duplo clique volta ao padrão)
class DragValue : public juce::Component
{
public:
    DragValue (juce::String captionIn, std::function<juce::String (float)> fmt)
        : caption (std::move (captionIn)), format (std::move (fmt)) { setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
    void bind (juce::RangedAudioParameter&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::String caption;
    std::function<juce::String (float)> format;
    juce::RangedAudioParameter* param = nullptr;
    float value = 0.0f, dragStart = 0.0f;
    std::unique_ptr<juce::ParameterAttachment> att;
};

// VU de LEDs do MASTER
class LedMeter : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    float level = 0.0f;
};

//==============================================================================
// Cabeçalho (fundo pintado uma vez, controles por cima)
class Header : public juce::Component
{
public:
    static constexpr int height = 140;
    Header (BatteryProcessor&, std::function<void (int)> onTab, std::function<void()> onPresetLoaded);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setTab (int t) { battery.setLit (t == 0); drumsTab.setLit (t == 1); }
    void refresh();

private:
    void renderBackground();
    void stepPreset (int delta);
    void askSave();
    BatteryProcessor& proc;
    std::function<void()> presetLoaded;
    Key battery { "BATTERY", "16 PADS" }, drumsTab { "DUB DRUMS", "GROOVE E MIDI" };
    Key play { "PLAY GROOVE", {}, Key::Cap::Green };
    Key prev { "<" }, next { ">" }, save { "GRAVAR", {}, Key::Cap::Red };
    juce::Rectangle<int> presetDisplay, presetSection, modeSection;
    juce::Image background;
};

//==============================================================================
// Aba BATTERY (1280 x 730)
class BatteryPanel : public juce::Component, private juce::Timer
{
public:
    static constexpr int width = 1280, height = 730;
    explicit BatteryPanel (BatteryProcessor&);
    ~BatteryPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void selectPad (int pad);
    void refreshAll();

private:
    void timerCallback() override;
    void renderBackground();
    void stepSample (int delta);
    void chooseFile();

    BatteryProcessor& proc;
    std::vector<std::unique_ptr<Pad>> pads;
    SampleVisor visor;
    EnvelopeView envelope;
    Key prevSample { "< SAMPLE" }, nextSample { "SAMPLE >" }, load { "CARREGAR" }, listen { "OUVIR" };
    ChoiceRow reverse { { "REVERSE" }, 1 }, mode { dbr::modeNames, 2 }, choke { dbr::chokeNames, 4 };
    StepSelector layerSel, linkSel;
    DragValue layerLevel, layerTune;
    KnobBox level, pan, tune, cutoff, velocity, masterLevel, tape;
    LedMeter meter;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image background;
    juce::Rectangle<int> woodL, woodR, plate, sampleSec, envSec, padSec, padCtlSec, layerSec, masterSec, padDisplay;
};
} // namespace dbrui

//==============================================================================
// Tudo no tamanho base (1280 x 870); o editor escala para a janela redimensionável
class BatteryRoot : public juce::Component
{
public:
    static constexpr int baseW = 1280;
    static constexpr int baseH = dbrui::Header::height + dbrui::BatteryPanel::height;
    explicit BatteryRoot (BatteryProcessor&);
    void resized() override;
    void paintOverChildren (juce::Graphics&) override;
    void showTab (int t);

private:
    BatteryProcessor& proc;
    dbrui::BatteryPanel batteryPanel;
    DubDrumsPanel drumsPanel;
    dbrui::Header header;
};

class BatteryEditor : public juce::AudioProcessorEditor
{
public:
    explicit BatteryEditor (BatteryProcessor&);
    void resized() override;

private:
    BatteryRoot root;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BatteryEditor)
};
