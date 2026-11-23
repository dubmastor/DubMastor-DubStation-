// Mastor Key Scout MKS-0420 — DubStation
// Vai no master (ou no grupo da base). Escuta o riddim, acha o tom, a escala (maior/menor) e o acorde de
// cada momento, e manda tudo para o Mastor Tune MT-0420 sem sidechain e sem roteamento.
// O áudio passa sem nenhuma mudança (latência zero).
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Tune/Source/TuneDSP.h"
#include "ScoutLink.h"

class KeyScoutProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    KeyScoutProcessor();
    ~KeyScoutProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "MKS-0420"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Status
    {
        bool signal = false, learned = false, hold = false;
        int keyRoot = -1; bool keyMinor = false;
        int chordRoot = -1; bool chordMinor = false;
        std::array<float, 12> chroma {};
        float levelDb = -100.0f;
    };
    Status getStatus() const;
    void relearn() { relearnFlag = true; }

private:
    void timerCallback() override;

    mt::RiddimEar ear;
    scoutlink::Writer writer;
    std::atomic<bool> relearnFlag { false };
    std::atomic<bool> sig { false }, learned { false }, keyMin { false }, chordMin { false };
    std::atomic<int> keyRoot { -1 }, chordRoot { -1 };
    std::array<std::atomic<float>, 12> chroma;
    std::atomic<float> level { -100.0f };
    int heldRoot = -1; bool heldMinor = false; bool wasHold = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyScoutProcessor)
};
