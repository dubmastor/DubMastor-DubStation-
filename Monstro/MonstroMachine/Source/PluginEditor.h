// Monstro Machine MM-2500 — interface no estilo da MPC2500: tela clicável, soft keys F1–F6, modos, DATA ◀ ▶,
// 16 pads, passos do pad escolhido, SEQ A–D, transporte, efeitos e MIDI para arrastar para a DAW.
#pragma once
#include "PluginProcessor.h"

namespace mmui
{
const juce::Colour body { 0xff3a3b3d }, body2 { 0xff2c2d2f }, edge { 0xff55575a }, silk { 0xffe8e6e0 },
                   lcd { 0xff9fd8c6 }, lcd2 { 0xff7cc2ad }, ink { 0xff0e2a24 }, pad { 0xff6b6d70 }, padHi { 0xff8a8c90 },
                   red { 0xffe0402f }, amber { 0xffffb020 };
}

class MonstroEditor;

// ---------------------------------------------------------------- tela (LCD) com itens clicáveis
class LcdScreen : public juce::Component
{
public:
    explicit LcdScreen (MonstroEditor& e) : ed (e) {}
    struct Item { juce::Rectangle<float> r; juce::String text, action; bool inv = false, box = false; float size = 18; juce::Justification just { juce::Justification::centredLeft }; };
    std::vector<Item> items;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
private:
    MonstroEditor& ed;
    int hover = -1;
};

class PadButton : public juce::Component
{
public:
    PadButton (MonstroEditor& e, int i) : ed (e), index (i) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    float glow = 0;
    juce::String label;
    bool current = false, user = false, muted = false;
private:
    MonstroEditor& ed;
    int index;
};

class StepButton : public juce::Component
{
public:
    StepButton (MonstroEditor& e, int s) : ed (e), step (s) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    int value = 0; bool now = false;
private:
    MonstroEditor& ed;
    int step;
    juce::uint32 downAt = 0; bool longDone = false;
};

class MidiDrag : public juce::TextButton
{
public:
    MidiDrag (MonstroProcessor& p, bool all) : juce::TextButton (all ? "MIDI A-D" : "MIDI SEQ"), proc (p), allSeqs (all) {}
    void mouseDrag (const juce::MouseEvent&) override;
private:
    MonstroProcessor& proc;
    bool allSeqs;
    bool dragging = false;
};

class MonstroEditor : public juce::AudioProcessorEditor, private juce::Timer, public juce::FileDragAndDropTarget
{
public:
    explicit MonstroEditor (MonstroProcessor&);
    ~MonstroEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray&, int x, int y) override;

    void act (const juce::String& action);
    void padPressed (int pad);
    void stepClicked (int step, bool accent);
    void buildScreen();
    void toast (const juce::String& msg);

    MonstroProcessor& proc;

private:
    void timerCallback() override;
    void refresh();
    void nudge (const juce::String& id, float delta, float lo, float hi);
    juce::Button& addBtn (juce::TextButton& b, const juce::String& action);

    LcdScreen screen { *this };
    juce::OwnedArray<PadButton> pads;
    juce::OwnedArray<StepButton> steps;
    juce::OwnedArray<juce::TextButton> fkeys, modes, seqBtns;
    juce::TextButton dPrev { "DATA <" }, dNext { "DATA >" }, playB { "PLAY" }, stopB { "STOP" }, recB { "REC" }, dawB { "TOCA COM A DAW" },
                     folderB { "CARREGAR PASTA" }, sampleB { "SAMPLE NO PAD" }, factoryB { "KIT DE FABRICA" }, undoB { "UNDO" }, tapB { "TAP" };
    MidiDrag midiSeq { proc, false }, midiAll { proc, true };
    juce::Slider bpmS, swingS, masterS;
    juce::Label bpmL, swingL, masterL, stepLbl, toastL;
    std::unique_ptr<juce::SliderParameterAttachment> bpmA, swingA, masterA;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Image logoAcademia, logoMonstro;
    std::array<juce::uint32, mm::numPads> lastHits {};
    juce::Array<double> taps;
    juce::uint32 toastUntil = 0;
    juce::String lastKey;
    std::vector<juce::String> fkeyActions;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonstroEditor)
};
