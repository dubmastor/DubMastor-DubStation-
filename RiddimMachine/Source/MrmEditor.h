// Interface do Mastor Riddim Machine MRM-0420 (DubStation):
// cabeçalho com o nome em destaque, as abas RIDDIM (resumo), HARMONIA (MRCM-1240), BATERIA (MRDM-12),
// BAIXO (MRBM-69) e BIG KNOB, o PLAY RIDDIM e o BPM único. Cada aba de máquina mostra o painel
// completo dela, igualzinho ao plugin separado.
#pragma once

#include "MrmMain.h"

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

// PLAY RIDDIM: tampa de pedal verde (dourada tocando), toca todas as máquinas juntas
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
    juce::ParameterAttachment att;
    float value = 75.0f, dragStart = 75.0f;
    bool dawShown = false;
    double dawBpm = 0.0;
};

// Cabeçalho completo (fundo pintado uma vez, controles por cima)
class Header : public juce::Component, private juce::Timer
{
public:
    static constexpr int height = 150;
    Header (MrmProcessor&, std::function<void (int)> onTab);
    ~Header() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void setTab (int t);

private:
    void timerCallback() override;
    void renderBackground();

    MrmProcessor& proc;
    std::array<std::unique_ptr<TabKey>, MrmProcessor::numTabs> tabs;
    PlayKey play;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAtt;
    BpmWindow bpm;

    juce::Image background;
    juce::Rectangle<int> woodL, woodR, plate, tabSection, transportSection;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Header)
};

// Mostra o painel completo de uma máquina, sem o título dela (o cabeçalho do MRM-0420 já tem o nome),
// escalado para caber na área das abas e centralizado num fundo de rack
class PanelHolder : public juce::Component
{
public:
    PanelHolder (juce::Component& panelIn, int cropTopIn) : panel (panelIn), cropTop (cropTopIn)
    {
        addAndMakeVisible (panel);
        setOpaque (true);
    }
    // cobre uma parte do painel (em coordenadas do painel) com uma plaqueta de aviso, sem deixar clicar
    void addCover (juce::Rectangle<int> panelArea, juce::String text);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    struct Cover : juce::Component
    {
        juce::String text;
        void paint (juce::Graphics&) override;
    };
    juce::Component& panel;
    int cropTop;
    float scale = 1.0f;
    juce::Rectangle<float> shown;
    juce::Image background;
    std::vector<std::pair<juce::Rectangle<int>, std::unique_ptr<Cover>>> covers;
};
// Aba CONTROL ROOM: o Mastor Big Knob inteiro (filtro, tape echo, sirene, maximizer) no master,
// o mute de cada máquina (DRUMS, BASS, RIDDIM) e as chaves CONTROL ROOM e TAPE ECHO
class ControlRoomPage : public juce::Component
{
public:
    ControlRoomPage (MrmProcessor&, juce::Component& knobPanel, int cropTop);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    PanelHolder holder;
    MuteKey drums, bass, riddim;
    PowerSwitch fx, echo;
    juce::Image background;
    juce::Rectangle<int> muteSection, masterSection;
};
} // namespace mrmui

//==============================================================================
// Tudo no tamanho base (1280 x 1000); o editor escala para a janela redimensionável
class MrmRoot : public juce::Component
{
public:
    static constexpr int baseW = mrmui::MainPage::kW;
    static constexpr int baseH = mrmui::Header::height + mrmui::MainPage::kH;
    explicit MrmRoot (MrmProcessor&);
    void resized() override;
    void paintOverChildren (juce::Graphics&) override;
    void showTab (int t);

private:
    MrmProcessor& proc;
    mrmui::MainPage mainPage;
    DubChordsPanel   chordsPanel;
    DubDrumsPanel    drumsPanel;
    BassMachinePanel bassPanel;
    DubPanel         knobPanel;
    mrmui::PanelHolder chordsHolder, drumsHolder, bassHolder;
    mrmui::ControlRoomPage controlRoom;
    mrmui::Header header;
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
