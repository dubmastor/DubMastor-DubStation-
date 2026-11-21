#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "RootsKeysEngine.h"
#include "DubChordsEngine.h"

struct Preset
{
    const char* name;
    int mode;
    float pianoBright, pianoDecay, pianoChorus, pianoLevel;
    int drawbars[9];
    bool percOn, perc3rd, percFast;
    float click, drive, leslieMix, organLevel;
    bool leslieFast;
    float reverb;
};

class RootsKeysProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener
{
public:
    RootsKeysProcessor();
    ~RootsKeysProcessor() override;

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
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    static int getNumPianoPresets();
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& drawbarNames();

    juce::AudioProcessorValueTreeState apvts;

    // teclado da interface: pinta as teclas tocadas via MIDI (e permite tocar com o mouse)
    juce::MidiKeyboardState keyboardState;

    // medidores para o VU e o maximizer (escritos no áudio, lidos pela interface)
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f }, gainReduction { 0.0f };
    // rodas de pitch/mod que chegam pelo MIDI (só para mostrar na tela)
    std::atomic<float> pitchWheel { 0.0f }, modWheel { 0.0f };

    // ---------------- aba DUB CHORDS (gerador de harmonia que toca o próprio piano/órgão)
    static constexpr int dcSlots = 4;
    std::vector<dub::ChordSlot> getDcSlots() const;          // só os acordes ligados
    dub::ChordSlot getDcSlot (int i) const;                  // um cartão, ligado ou não
    bool isDcSlotOn (int i) const;
    dub::Settings getDcSettings() const;
    int effectiveDcPattern() const;                          // "Auto" segue o instrumento
    double getDcExportBpm() const;
    void applyRiddim (int index);
    // slot = -1: a progressão inteira; slot >= 0: só aquele acorde
    bool writeDcMidiFile (const juce::File& file, int slot = -1) const;
    std::atomic<double> dcPlayPos { -1.0 }, dcHostBpm { 0.0 };
    std::atomic<bool> dcHostPlaying { false };

private:
    rk::Engine engine;
    int currentProgram = 0;
    std::atomic<float>* dbParam[9] {};

    // sequenciador Dub Chords (só na thread de áudio)
    struct DcPending { int sample; bool on; int note; int vel; };
    std::vector<dub::NoteEvent> dcLoop;
    std::vector<DcPending> dcPending;
    double dcLoopBeats = 0.0, dcExpectedPpq = 0.0, dcInternalPpq = 0.0, sampleRateHz = 48000.0;
    bool dcWasPlaying = false, dcActive[128] {};
    std::atomic<bool> dcDirty { true };
    void parameterChanged (const juce::String&, float) override { dcDirty.store (true); }
    void runDubChords (juce::MidiBuffer& midi, int numSamples);
    void dcAllNotesOff (juce::MidiBuffer& midi, int sample);

    void setParam (const juce::String& id, float value);
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RootsKeysProcessor)
};
