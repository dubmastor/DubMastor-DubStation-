#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "MonstroEngine.h"

class MonstroProcessor : public juce::AudioProcessor
{
public:
    MonstroProcessor();
    ~MonstroProcessor() override = default;

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
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- chamado pela interface (thread da mensagem) ----
    void selectKit (int index);                 // troca o kit, o BPM e os grooves de fábrica do estilo
    void loadStyle (int style);                 // troca só os grooves
    void hitPad (int pad, float vel);           // toque na tela (fila para a thread de áudio)
    int  getStep (int seq, int pad, int step) const { return grid[seq][pad][step].load(); }
    void setStep (int seq, int pad, int step, int v) { grid[seq][pad][step].store ((uint8_t) v); }
    void clearSeq (int seq);
    void pushUndo();
    bool undo();
    mm::Grid seqGrid (int seq) const;
    bool loadSampleFile (int pad, const juce::File& f);
    int  loadFolder (const juce::File& dir);
    void clearSamples();
    juce::String sampleName (int pad) const;
    bool hasSample (int pad) const { return engine.samples[pad] != nullptr; }
    uint32_t padHits (int pad) const { return engine.hits[pad].load (std::memory_order_relaxed); }
    int currentStep() const { return playStep.load(); }
    bool isRunning() const { return running.load(); }
    double shownBpm() const { return hostBpm.load(); }

    std::atomic<bool> internalPlay { false }, recording { false };
    std::atomic<int> selectedPad { 0 };
    std::atomic<int> forcedStyle { -1 };
    juce::String logoA, logoM;   // imagens dos logos (PNG/JPG em base64), salvas no projeto

private:
    juce::AudioParameterInt* pKit = nullptr;
    juce::AudioParameterChoice* pSeq = nullptr;
    std::atomic<float>* bpmP = nullptr, *swingP = nullptr, *volP = nullptr, *satOnP = nullptr, *satDrvP = nullptr,
                      *echoOnP = nullptr, *echoTP = nullptr, *echoFbP = nullptr, *echoMixP = nullptr;
    std::atomic<float>* tuneP[mm::kPads] {}, *decP[mm::kPads] {};

    mm::Engine engine;
    std::atomic<uint8_t> grid[mm::kSeqs][mm::kPads][mm::kSteps] {};
    std::vector<mm::Grid> history;
    std::atomic<int> pendingHit[mm::kPads] {};      // velocidade*127 pendente (0 = nada)
    std::atomic<int> playStep { -1 };
    std::atomic<bool> running { false };
    std::atomic<double> hostBpm { 140 };
    juce::String samplePaths[mm::kPads];
    double sr = 44100, internalPos = 0, lastPos = -1;
    bool wasRunning = false;

    void writeGrid (int seq, const mm::Grid& g);
    void recordHit (int pad, double pos16);
    void renderChunk (juce::AudioBuffer<float>&, int start, int n);
    juce::AudioFormatManager formats;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonstroProcessor)
};
