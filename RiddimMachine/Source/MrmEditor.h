// Interface do Mastor Riddim Machine MRM-0420, no visual Dub Station:
// cabeçalho com o nome em destaque, as abas HARMONIA (Dub Chords) e BATERIA (Dub Drums) e a seção RIDDIM
// (PLAY RIDDIM, BPM ligado, tamanho, ARRASTE O RIDDIM e EXPORTAR). Abaixo, o painel da aba escolhida,
// igualzinho ao plugin separado.
#pragma once

#include "MrmProcessor.h"

namespace mrmui
{
// Tecla de aba: botão de pressão grande com lâmpada piloto e duas linhas de texto
class TabKey : public juce::Component
{
public:
    TabKey (juce::String titleIn, juce::String subIn) : title (std::move (titleIn)), sub (std::move (subIn))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
    void setSelected (bool s) { if (s != selected) { selected = s; repaint(); } }
    std::function<void()> onClick;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }

private:
    juce::String title, sub;
    bool selected = false;
};

// PLAY RIDDIM: tampa de pedal verde (dourada tocando), toca harmonia e bateria juntas
class PlayKey : public juce::Button
{
public:
    PlayKey() : juce::Button ("PLAY RIDDIM") { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// BPM: display âmbar; arraste para cima/baixo, rodinha, duplo clique volta para 75
class BpmWindow : public juce::Component
{
public:
    explicit BpmWindow (MrmProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void refresh();

private:
    MrmProcessor& proc;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    float value = 75.0f, dragStart = 75.0f;
    bool dawShown = false;
    double dawBpm = 0.0;
};

// ARRASTE O RIDDIM: a bateria e a harmonia num arquivo só, já alinhadas
class RiddimDragZone : public juce::Component
{
public:
    explicit RiddimDragZone (MrmProcessor& p) : proc (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }

private:
    MrmProcessor& proc;
    bool dragging = false;
};

// Cabeçalho completo (fundo pintado uma vez, controles por cima)
class Header : public juce::Component, private juce::Timer
{
public:
    static constexpr int height = 170;
    Header (MrmProcessor&, std::function<void (int)> onTab);
    ~Header() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void setTab (int t) { harmony.setSelected (t == 0); drumsTab.setSelected (t == 1); }

private:
    void timerCallback() override;
    void renderBackground();
    void exportRiddim();

    MrmProcessor& proc;
    DubLookAndFeel lnf;                    // o mesmo LookAndFeel do Dub Chords (botão EXPORTAR)
    TabKey harmony { "HARMONIA", "DUB CHORDS" }, drumsTab { "BATERIA", "DUB DRUMS" };
    PlayKey play;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAtt;
    BpmWindow bpm;
    std::unique_ptr<ChoiceWindow> length;
    RiddimDragZone dragZone;
    juce::TextButton exportBtn { "EXPORTAR" };
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image background;
    juce::Rectangle<int> woodL, woodR, plate, tabSection, riddimSection;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Header)
};
} // namespace mrmui

//==============================================================================
// Tudo no tamanho base (1280 x 967); o editor escala para a janela redimensionável
class MrmRoot : public juce::Component
{
public:
    static constexpr int baseW = 1280;
    explicit MrmRoot (MrmProcessor&);
    void resized() override;
    void paintOverChildren (juce::Graphics&) override;
    void showTab (int t);

private:
    MrmProcessor& proc;
    DubChordsPanel chordsPanel;
    DubDrumsPanel  drumsPanel;
    mrmui::Header  header;
};

class MrmEditor : public juce::AudioProcessorEditor
{
public:
    explicit MrmEditor (MrmProcessor&);
    void resized() override;

private:
    MrmRoot root;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MrmEditor)
};
