#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "VintageLookAndFeel.h"
#include "VUMeter.h"

class RootsKeysEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit RootsKeysEditor (RootsKeysProcessor&);
    ~RootsKeysEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> att;
    };
    struct Drawbar
    {
        juce::Slider slider { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> att;
    };

    RootsKeysProcessor& proc;
    VintageLookAndFeel lnf;

    // cabeçalho
    juce::ComboBox modeBox, presetBox;
    juce::Label modeLabel, presetLabel;
    std::unique_ptr<APVTS::ComboBoxAttachment> modeAtt;

    // piano
    Knob pBright, pDecay, pChorus, pLevel;

    // órgão
    Drawbar drawbars[9];
    juce::ComboBox percHarmBox, percDecayBox;
    std::unique_ptr<APVTS::ComboBoxAttachment> percHarmAtt, percDecayAtt;
    juce::ToggleButton percButton { "Percussion" }, leslieButton { "Leslie Fast" };
    std::unique_ptr<APVTS::ButtonAttachment> percAtt, leslieAtt;
    Knob oClick, oDrive, oLeslieMix, oLevel, mReverb;

    // eco de fita
    juce::ToggleButton echoButton { "Echo" };
    std::unique_ptr<APVTS::ButtonAttachment> echoAtt;
    juce::ComboBox echoModeBox;
    std::unique_ptr<APVTS::ComboBoxAttachment> echoModeAtt;
    Knob eRate, eIntensity, eVol, eSpring, eWow, eAge;

    // maximizer + VU
    juce::ToggleButton maxButton { "Max" };
    std::unique_ptr<APVTS::ButtonAttachment> maxAtt;
    Knob mxGain, mxCeil, mMaster;
    juce::Label grLabel;
    VUMeter vuL { "L" }, vuR { "R" };

    // teclado pintado pelo MIDI
    juce::MidiKeyboardComponent keyboard;

    juce::Rectangle<int> pianoArea, organArea, fxArea, echoArea, outArea, keysArea, plateArea;

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& text);
    void layoutKnobRow (juce::Rectangle<int> r, std::initializer_list<Knob*> knobs);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RootsKeysEditor)
};
