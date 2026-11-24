#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

inline juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

namespace mmui
{
    const juce::Colour body  { 0xff3a3b3d }, body2 { 0xff2c2d2f }, silk { 0xffe8e6e0 }, lcd { 0xff9fd8c6 }, lcd2 { 0xff7cc2ad },
                       lcdInk { 0xff0e2a24 }, pad { 0xff6b6d70 }, padHi { 0xff8a8c90 }, red { 0xffe0402f }, amber { 0xffffb020 },
                       paper { 0xffe8e6e0 }, ink { 0xff1b1b1b };
}

class MonstroLook : public juce::LookAndFeel_V4
{
public:
    MonstroLook();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
};

class PadButton : public juce::Component
{
public:
    PadButton (MonstroProcessor& p, int i) : proc (p), index (i) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void flash() { flashUntil = juce::Time::getMillisecondCounter() + 110; repaint(); }
    bool flashing() const { return juce::Time::getMillisecondCounter() < flashUntil; }
    MonstroProcessor& proc; int index; uint32_t flashUntil = 0, seen = 0; bool wasFlashing = false;
};

class StepRow : public juce::Component
{
public:
    explicit StepRow (MonstroProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    MonstroProcessor& proc; int downStep = -1; uint32_t downAt = 0;
    int stepAt (juce::Point<int>) const;
};

class Lcd : public juce::Component
{
public:
    explicit Lcd (MonstroProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    MonstroProcessor& proc;
};

class LogoSlot : public juce::Component
{
public:
    LogoSlot (juce::String t) : text (std::move (t)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    void setImageData (const juce::String& b64);
    std::function<void()> onClick;
    juce::String text; juce::Image img;
};

class MidiDrag : public juce::Component
{
public:
    explicit MidiDrag (MonstroProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    juce::File writeTemp();
    MonstroProcessor& proc; bool dragging = false;
};

class MonstroEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MonstroEditor (MonstroProcessor&);
    ~MonstroEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void bindPadSliders();
    void chooseLogo (bool academia);
    void setParam (const juce::String& id, float value);
    float param (const juce::String& id) const;

    MonstroProcessor& proc;
    MonstroLook look;
    Lcd lcd { proc };
    LogoSlot logoA { "Logo\nAcademia de Beats" }, logoM { "Logo\nMonstro.zip" };
    juce::TextButton fkeys[6];
    juce::ComboBox kitBox;
    juce::TextButton recB { "REC" }, playB { "PLAY" }, stopB { "STOP" }, tapB { "TAP" }, undoB { "UNDO" };
    juce::Slider bpmS, swingS, tuneS, decS, satS, echoTS, echoFS, echoMS;
    juce::Label bpmL, swingL, tuneL, decL, satL, echoTL, echoFL, echoML;
    juce::TextButton satB { "SATURADOR" }, echoB { "TAPE ECHO" };
    juce::TextButton seqB[4], folderB { "CARREGAR PASTA" }, sampleB { "SAMPLE NO PAD" }, clearSmpB { "LIMPAR SAMPLES" }, saveMidiB { "SALVAR MIDI" };
    juce::OwnedArray<PadButton> pads;
    StepRow steps { proc };
    MidiDrag midiDrag { proc };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> bpmA, swingA, tuneA, decA, satA, echoTA, echoFA, echoMA;
    std::unique_ptr<BA> satBA, echoBA;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> infoArea;
    int shownKit = -1, boundPad = -1, shownSeq = -1;
    juce::Array<double> taps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonstroEditor)
};
