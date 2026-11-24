#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "VintageLookAndFeel.h"   // o mesmo visual dos instrumentos do kit (ReggaeKeys/Source)

// Grade de 16 semicolcheias (contagem com a caixa no 2 e no 4): clique troca o valor do passo
class StepGrid : public juce::Component
{
public:
    StepGrid (ReggaeGuitarProcessor& p, bool fillGrid) : proc (p), fill (fillGrid)
    { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    ReggaeGuitarProcessor& proc;
    bool fill;
};

// Osciloscópio da saída
class Scope : public juce::Component
{
public:
    explicit Scope (ReggaeGuitarProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    ReggaeGuitarProcessor& proc;
};

class ReggaeGuitarEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ReggaeGuitarEditor (ReggaeGuitarProcessor&);
    ~ReggaeGuitarEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob { juce::Slider s; juce::String label; std::unique_ptr<SA> a; };
    struct Toggle { juce::ToggleButton b; std::unique_ptr<BA> a; };
    struct Combo { juce::ComboBox c; juce::String label; std::unique_ptr<CA> a; };

    Knob& knob (const juce::String& id, const juce::String& label);
    Toggle& toggle (const juce::String& id, const juce::String& text);
    Combo& combo (const juce::String& id, const juce::String& label, bool attach = true);
    juce::Rectangle<int> section (juce::Graphics&, juce::Rectangle<int>, const juce::String&) const;

    ReggaeGuitarProcessor& proc;
    VintageLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Toggle>> toggles;
    std::vector<std::unique_ptr<Combo>> combos;
    std::map<juce::String, juce::Component*> byId;

    StepGrid mainGrid, fillGrid;
    Scope scope;
    juce::TextButton prevTrack { "<" }, nextTrack { ">" }, resetVar { "RESTAURAR MODELO" };
    juce::MidiKeyboardComponent keyboard;
    juce::Rectangle<int> rTop, rPattern, rVar, rHand, rGuitar, rAmp, rFx, rKeys;
    int lastStep = -2, lastBar = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReggaeGuitarEditor)
};
