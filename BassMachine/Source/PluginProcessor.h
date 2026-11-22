#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "BassEngine.h"
#include "DubBassTimbre.h"
#include "AccompEngine.h"
#include "../../DubDrums/Source/DrumSynth.h"      // bateria acústica do Dub Drums
#include "../../DubChords/Source/DubSynth.h"      // piano M1 + Hammond do Roots Keys

class BassMachineProcessor : public juce::AudioProcessor,
                             private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int numSlots = 4;

    BassMachineProcessor();
    ~BassMachineProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mastor Reggae Bass Machine MRBM-69"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Usado pelo editor
    std::vector<mbm::ChordSlot> getSlots() const;
    mbm::Settings getSettings() const;
    double getExportBpm() const;
    bool writeMidiFile (const juce::File& file) const;
    double getPlayPositionInLoop() const { return displayPos.load(); }   // -1 = parado
    int getSoundingNote() const { return soundingNote.load(); }          // -1 = nenhuma
    void newIdea();                                                      // sorteia outra IDEIA

    // MIDI editável (thread da interface). A linha editada fica salva no projeto e vale até um
    // controle do gerador (estilo, ideia, harmonia, densidade...) ser mexido, como no MRDM-12.
    std::vector<mbm::NoteEvent> getCurrentLine (double& loopBeatsOut) const;   // editada, se houver
    void setEdits (std::vector<mbm::NoteEvent> events, double loopBeatsIn);
    bool hasEdits() const;
    void clearEdits();
    void dropStaleEdits();                                               // chamada pelo timer da interface

    // osciloscópio: últimas amostras da saída (mono)
    static constexpr int kScopeSize = 8192;
    void copyScope (float* dest, int n) const
    {
        const int w = scopeWrite.load (std::memory_order_acquire);
        for (int i = 0; i < n; ++i)
            dest[i] = scopeBuf[(size_t) ((w - n + i) & (kScopeSize - 1))];
    }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String&, float) override { dirty.store (true); }
    void rebuild();
    void allNotesOff (juce::MidiBuffer& midi, int sample);
    void renderSynth (juce::AudioBuffer<float>&, const juce::MidiBuffer&);
    void renderAccomp (juce::AudioBuffer<float>&, int numSamples);
    void accompOff();
    juce::AudioBuffer<float> monoBuf;

    float param (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    uint64_t settingsKey() const;   // impressão digital dos controles do gerador (sem alocar)

    mutable juce::SpinLock editLock;
    std::vector<mbm::NoteEvent> edits;
    double editsLoopBeats = 0.0;
    uint64_t editsKey = 0;
    bool editsOn = false;

    mbm::DubBassPreview synth;   // sistema de timbres do Dub Bass

    std::vector<mbm::NoteEvent> loop;
    std::vector<mbm::AccEvent> accLoop;        // bateria + teclado (só para o som do preview)
    dubdrums::DrumSynth acousticDrums;
    mbm::DigitalKit digitalDrums;
    DubSoundEngine keys;
    struct AccPending { int sample; bool on; int target; int note; int vel; };
    std::vector<AccPending> accPending;
    bool keysActive[2][128] {};
    bool lastKeysOn = false;
    double loopBeats = 0.0;
    std::atomic<bool> dirty { true };

    struct Pending { int sample; bool on; int note; int vel; };
    std::vector<Pending> pending;

    bool active[128] {};
    bool wasPlaying = false;
    double expectedPpq = 0.0;
    double internalPpq = 0.0;
    double sr = 44100.0;

    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> displayPos { -1.0 };
    std::atomic<int> soundingNote { -1 };

    std::array<float, kScopeSize> scopeBuf {};
    std::atomic<int> scopeWrite { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassMachineProcessor)
};
