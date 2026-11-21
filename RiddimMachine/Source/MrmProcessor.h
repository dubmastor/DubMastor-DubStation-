// Mastor Riddim Machine MRM-0420
// Junta o Dub Chords (harmonia: skank e bubble) e o Dub Drums (bateria) num só instrumento.
// Os dois geradores rodam dentro deste processador, com o mesmo relógio: a bateria começa no compasso 1
// (ou na intro), os acordes entram depois da intro, e o PLAY RIDDIM toca os dois juntos sem a DAW.
#pragma once

#include "SubPlugins.h"

class MrmProcessor : public juce::AudioProcessor,
                     private juce::Timer
{
public:
    MrmProcessor();
    ~MrmProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mastor Riddim Machine MRM-0420"; }
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

    // ---- MRM-0420
    juce::AudioProcessorValueTreeState apvts;      // play, bpm e tamanho do riddim (visíveis na DAW)
    DubChordsProcessor& getChords() noexcept { return chords; }
    DubDrumsProcessor&  getDrums()  noexcept { return drums; }

    static constexpr int numLengths = 5;
    static int lengthBars (int index);             // 4, 8, 16, 32, 64 compassos
    int    riddimBars() const;
    double introBeats() const;                     // 4 com a VIRADA DE INTRO do Dub Drums ligada

    // arquivo com o riddim inteiro: bateria + skank + bubble, alinhados
    bool writeRiddimFile (const juce::File&) const;
    juce::String suggestedRiddimName() const;

    // posição tocando agora (semínimas desde o início do riddim, intro incluída); -1 = parado
    double getPlayBeat() const noexcept   { return playBeat.load(); }
    bool   isMasterPlaying() const noexcept { return masterPlaying.load(); }

    int  getCurrentTab() const noexcept   { return currentTab.load(); }   // 0 = harmonia, 1 = bateria
    void setCurrentTab (int t) noexcept   { currentTab.store (t); }

private:
    // Relógio que cada gerador enxerga (o da DAW, o do PLAY RIDDIM ou o da DAW atrasado pela intro)
    struct SubPlayHead : juce::AudioPlayHead
    {
        juce::Optional<PositionInfo> getPosition() const override { return info; }
        juce::Optional<PositionInfo> info;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    float raw (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void processChords (const juce::AudioPlayHead::PositionInfo&, bool playing, double blockBeats,
                        const juce::MidiBuffer& input, int numSamples);

    DubChordsProcessor chords;
    DubDrumsProcessor  drums;
    SubPlayHead chordsHead, drumsHead;

    juce::AudioBuffer<float> chordsBuf, drumsBuf;
    juce::MidiBuffer chordsMidi, chordsMidiB, drumsMidi, inputA, inputB;

    double sr = 44100.0;
    bool wasMaster = false;
    double masterPpq = 0.0;

    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> playBeat { -1.0 };
    std::atomic<bool>   masterPlaying { false }, hostPlaying { false };
    std::atomic<int>    currentTab { 0 };

    // BPM ligado: o BPM do MRM, do Dub Chords e do Dub Drums andam juntos
    float lastBpm = -1.0f, lastChordsBpm = -1.0f, lastDrumsBpm = -1.0f;
    bool lastChordsPreview = false, lastDrumsPreview = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MrmProcessor)
};
