// Mastor Reggae Clavinet MRCL-71 — interface: aba INSTRUMENTO (pele de clavinet dos anos 70) e aba CLAVINET MACHINE.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

juce::String timbreLcdName (int t);
juce::String timbreName (int t);

class ClavLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ClavLookAndFeel();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

// botão de balancim do clavinet (filtros e captadores)
class RockerButton : public juce::Button
{
public:
    RockerButton (juce::String label, juce::Colour c) : juce::Button (label), colour (c) { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    juce::Colour colour;
};

// visor MIDI editável: toque numa nota para apagar, no vazio para pôr uma nota do acorde (grade de 1/16)
class LineRoll : public juce::Component
{
public:
    explicit LineRoll (ReggaeClavinetProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    int style = -1;          // -1 = a linha em uso (editável); 0..2 = miniatura de uma sugestão
private:
    ReggaeClavinetProcessor& proc;
    int lo = 24, hi = 60; juce::Rectangle<float> area; double beats = 8;
};

// arraste o MIDI para a timeline da DAW
class MidiDragZone : public juce::Component
{
public:
    explicit MidiDragZone (ReggaeClavinetProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
private:
    ReggaeClavinetProcessor& proc;
    bool dragging = false;
};

class ReggaeClavinetEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ReggaeClavinetEditor (ReggaeClavinetProcessor&);
    ~ReggaeClavinetEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void setPage (int p);

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    struct Knob { juce::Slider s; juce::String label; std::unique_ptr<SA> att; };

    void timerCallback() override;
    void knob (Knob&, const juce::String& id, const juce::String& label, bool rotary = true);
    void chip (juce::TextButton&, const juce::String& text, std::function<void()> click, bool light = false);
    void showBank();
    void exportMidi();
    void layoutInstrument();
    void layoutMachine();
    std::vector<juce::Component*> pageComponents (int p);

    ReggaeClavinetProcessor& proc;
    ClavLookAndFeel lnf;
    int page = 0;

    // cabeçalho
    juce::TextButton tabInst, tabMach, previewBtn, octBtn[6];
    juce::ToggleButton dawBtn { "TOCA COM A DAW" };
    std::unique_ptr<BA> previewAtt, dawAtt;
    Knob bpm;

    // INSTRUMENTO
    std::unique_ptr<RockerButton> filt[4], pick[2];
    std::unique_ptr<BA> filtAtt[4], pickAtt[2];
    juce::TextButton timbreBtn[6];
    Knob bright, decay, mute, vol, wahSens, wahRange, phRate, phDepth, echoTime, echoFb, echoMix, reverb, master;
    juce::TextButton wahBtn, phBtn, echoBtn, maxBtn;
    std::unique_ptr<BA> wahAtt, phAtt, echoAtt, maxAtt;
    juce::MidiKeyboardComponent keyboard;
    juce::Rectangle<int> lidArea, lcdArea, fxArea[4];

    // CLAVINET MACHINE
    juce::TextButton srcBtn[2], bankBtn;
    juce::ComboBox rootBox[4], durBox[4];
    std::unique_ptr<CA> rootAtt[4], durAtt[4];
    juce::TextButton majBtn[4], minBtn[4];
    juce::ToggleButton onBtn[4];
    std::unique_ptr<BA> onAtt[4];
    juce::TextButton musBtn[7], fillBtn[4], everyBtn[2], notaBtn[2], newBtn, exportBtn, undoBtn, ouvirBtn[3], usarBtn[3];
    Knob center, density, swing, velocity;
    juce::ComboBox repsBox; std::unique_ptr<CA> repsAtt;
    LineRoll roll;
    std::unique_ptr<LineRoll> mini[3];
    MidiDragZone dragZone;
    juce::Rectangle<int> escuta, musico, sugest, midiSec, cardArea[4], beatsArea, whoArea;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReggaeClavinetEditor)
};
