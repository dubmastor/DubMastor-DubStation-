#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "LoFiVoice.h"
#include "TapeEcho.h"

class RadioFoneProcessor : public juce::AudioProcessor
{
public:
    RadioFoneProcessor();
    ~RadioFoneProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    // o bypass do Cubase controla o nosso parâmetro "bypass" (troca suave)
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getOutputLevel() const { return outRms.load(); }   // RMS linear da saída (VU)
    float getEchoLevel() const   { return echoRms.load(); }  // RMS das repetições (anima as bobinas)
    float getEchoDelaySec() const { return echoDelay.load(); }
    float getDisplayBpm() const  { return displayBpm.load(); }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& divisionNames();
    static const juce::StringArray& modeNames();

private:
    float getP (const char* id) const { return params.at (id)->load(); }

    rf::LoFiVoice voice;
    dub::TapeEcho echo;
    juce::SmoothedValue<float> dryGain, outGain, bypassMix;

    std::atomic<float> outRms { 0.0f }, echoRms { 0.0f }, echoDelay { 0.4f }, displayBpm { 120.0f };
    std::map<std::string, std::atomic<float>*> params;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioFoneProcessor)
};
