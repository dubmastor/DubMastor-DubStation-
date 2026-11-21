#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PatternGenerator.h"
#include "DrumSynth.h"

class DubDrumsProcessor : public juce::AudioProcessor,
                          private juce::AudioProcessorValueTreeState::Listener,
                          private juce::Timer
{
public:
    DubDrumsProcessor();
    ~DubDrumsProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DubMastor Dub Drums"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- Dub Drums
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // peças da bateria, na ordem do mapa de notas (opção avançada)
    struct Piece { const char* id; const char* name; int gmNote; };
    static constexpr int numPieces = 9;
    static const Piece pieces[numPieces];

    dubdrums::Pattern getPatternCopy() const;     // thread da interface
    int  getPatternVersion() const noexcept       { return patternVersion.load(); }

    // posição tocando agora, na linha do tempo "intro + loop" (em semínimas); -1 = parado
    double getPlayheadBeat() const noexcept       { return playheadBeat.load(); }
    bool isHostPlaying() const noexcept           { return hostPlaying.load(); }
    bool isPreviewing() const noexcept            { return previewing.load(); }

    void newVariation();                          // sorteia uma nova semente
    void applyStyleDefaults (int styleIndex);     // bumbo e caixa do estilo na BATIDA PRINCIPAL
    bool writeMidiFile (const juce::File& target) const;
    juce::String suggestedFileName() const;
    int mapNote (int gmNote) const;               // nota GM -> nota escolhida pelo usuário

private:
    void parameterChanged (const juce::String&, float) override;
    void timerCallback() override;
    void regenerate();
    dubdrums::Settings readSettings() const;

    // agenda as notas de [start, end) da linha do tempo "intro + loop"
    template <typename Fn> void forEachEventIn (double start, double end, Fn&& fn) const;

    mutable juce::SpinLock patternLock;
    dubdrums::Pattern pattern;
    std::atomic<bool> dirty { true };
    std::atomic<int>  patternVersion { 0 };

    std::atomic<double> playheadBeat { -1.0 };
    std::atomic<bool>   hostPlaying  { false };
    std::atomic<bool>   previewing   { false };
    bool wasPlaying = false, wasPreviewing = false;
    double previewPos = 0.0;
    double sr = 44100.0;

    dubdrums::DrumSynth synth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubDrumsProcessor)
};
