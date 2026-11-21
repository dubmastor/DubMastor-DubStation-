#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "TapeEchoDSP.h"

class FitaEcoProcessor : public juce::AudioProcessor
{
public:
    FitaEcoProcessor();
    ~FitaEcoProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FitaEco TE-12"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    juce::AudioParameterBool* getBypassParameter() const override { return bypassParam; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // oscilloscope: last output samples (mono), written by the audio thread
    static constexpr int kScopeSize = 8192;
    void copyScope (float* dest, int n) const
    {
        const int w = scopeWrite.load (std::memory_order_acquire);
        for (int i = 0; i < n; ++i)
            dest[i] = scopeBuf[(size_t) ((w - n + i) & (kScopeSize - 1))];
    }

    // level at the record head, in VU (0 VU = TapeEcho::kRecRef), read by the editor's meter
    float getRecVu() const { return recVu.load(); }

private:
    fitaeco::TapeEcho echo;
    juce::AudioBuffer<float> monoScratch, dryCopy;
    juce::AudioParameterBool* bypassParam = nullptr;
    juce::SmoothedValue<float> bypassMix; // 0 = effect, 1 = dry (click-free crossfade)
    std::atomic<float> recVu { -60.0f };
    std::array<float, kScopeSize> scopeBuf {};
    std::atomic<int> scopeWrite { 0 };

    std::atomic<float>* pInput {}; std::atomic<float>* pMode {}; std::atomic<float>* pRate {};
    std::atomic<float>* pIntensity {}; std::atomic<float>* pEcho {}; std::atomic<float>* pReverb {};
    std::array<std::atomic<float>*, fitaeco::Params::kEqBands> pEq {}; std::atomic<float>* pWow {};
    std::atomic<float>* pAge {}; std::atomic<float>* pDry {}; std::atomic<float>* pOutput {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FitaEcoProcessor)
};
