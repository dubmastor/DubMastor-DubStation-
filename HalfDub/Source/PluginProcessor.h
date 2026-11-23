#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "HalfDubEngine.h"

class HalfDubProcessor : public juce::AudioProcessor
{
public:
    HalfDubProcessor();
    ~HalfDubProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "INIT"; }
    void changeProgramName (int, const juce::String&) override {}

    // o bypass do DAW controla o nosso parâmetro "bypass" (troca suave)
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // nomes das opções (usados também pela interface)
    static const juce::StringArray& speedNames();
    static const juce::StringArray& bandSpeedNames();
    static const juce::StringArray& loopNames();
    static const juce::StringArray& divisionNames();
    static juce::String bandId (int band, const char* what) { return "b" + juce::String (band) + what; }

    // notas MIDI dos pads (C1 D1 E1 F1 no padrão do Cubase, C3 = 60) e do DROP (C2)
    static constexpr int kPadNotes[4] = { 36, 38, 40, 41 };
    static constexpr int kDropNote = 48;
    bool isMidiDropHeld() const { return midiDrop.load(); }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    hd::Engine engine;

private:
    float getP (const char* id) const { return params.at (id)->load(); }
    float getP (const juce::String& id) const { return params.at (id.toStdString())->load(); }
    hd::EngineParams readParams();

    std::map<std::string, std::atomic<float>*> params;
    int midiPad = 0;
    std::atomic<bool> midiDrop { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HalfDubProcessor)
};
