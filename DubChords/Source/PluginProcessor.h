#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "DubEngine.h"
#include "DubSynth.h"

class DubChordsProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int numSlots = 4;

    DubChordsProcessor();
    ~DubChordsProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dub Chords"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Usado pelo editor
    std::vector<dub::ChordSlot> getSlots() const;
    dub::Settings getSettings() const;
    juce::String getProgressionText() const;
    dub::Voicings getVoicings() const;   // inversão sugerida e usada de cada acorde
    double getExportBpm() const;
    bool writeMidiFile (const juce::File& file) const;
    double getPlayPositionInLoop() const { return displayPos.load(); }   // -1 = parado
    double getLoopBeatsForDisplay() const { return displayLoop.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String&, float) override { dirty.store (true); }
    void rebuild();
    void allNotesOff (juce::MidiBuffer& midi, int sample);
    void renderSound (juce::AudioBuffer<float>&, const juce::MidiBuffer&);

    float param (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }

    DubSoundEngine sound;
    std::vector<float> renderL, renderR;

    std::vector<dub::NoteEvent> loop;
    double loopBeats = 0.0;
    std::atomic<bool> dirty { true };

    struct Pending { int sample; bool on; int channel; int note; int vel; };
    std::vector<Pending> pending;

    bool active[17][128] {};
    bool wasPlaying = false;
    double expectedPpq = 0.0;
    double internalPpq = 0.0;
    double sr = 44100.0;

    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> displayPos { -1.0 };
    std::atomic<double> displayLoop { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubChordsProcessor)
};
