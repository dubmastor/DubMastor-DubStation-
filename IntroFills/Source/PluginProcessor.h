// Mastor Intro Fills MIF — viradas de introdução de bateria de reggae (bateristas jamaicanos), na contagem da Dub Station
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include "FillSampler.h"

class IntroFillsProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    IntroFillsProcessor();
    ~IntroFillsProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.5; }

    // presets de fábrica = as viradas (baterista + variação)
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- usados pela interface
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<double> playStep { -1.0 };          // passo da grade tocando agora (-1 = parado)
    std::atomic<float> meter { 0.0f };

    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float value);
    int drummer() const;
    int fill() const;
    void selectDrummer (int d);
    void stepFill (int delta);

    mif::Fill currentFill() const;                   // a virada (com as edições do usuário)
    void toggleCell (int piece, int step);           // clique na grade (passos 0..16)
    void resetFill();                                // volta a virada de fábrica
    bool isEdited() const;
    void audition (int piece, int vel = 96);         // toca uma peça (clique na grade / no nome da linha)

    mif::Settings settings() const;
    bool writeMidiFile (const juce::File& f) const;
    juce::String midiFileName() const;
    double previewBpm() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    void rebuild();
    juce::String settingsKey() const;

    struct Hit { int offset, piece, vel; };
    std::vector<Hit> hits;                           // reservado no prepare (sem alocar no áudio)
    mif::FillSampler sampler;
    std::shared_ptr<const mif::SeqPlayer> seq;       // trocado com atomic_load/atomic_store (interface x áudio)
    juce::String lastKey;
    mutable std::mutex editsLock;
    std::map<int, mif::Fill> edits;                  // chave: baterista * 16 + variação
    int editsVersion = 0;
    std::atomic<int> pendingAudition { -1 }, pendingAuditionVel { 96 };
    double sampleRateHz = 44100.0, previewBeat = 0.0, lastHostBeat = -1.0;
    bool wasRunning = false;
    std::atomic<double> hostBpm { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IntroFillsProcessor)
};
