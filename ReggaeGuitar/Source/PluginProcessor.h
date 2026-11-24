#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "GuitarEngine.h"

// Mastor Reggae Guitar MRG-68: guitarra de reggae dos guitarristas jamaicanos (skank, bang-bang, chop,
// picking e lead), no andamento da DAW, na contagem com a caixa no 2 e no 4.
class ReggaeGuitarProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numSlots = 4;
    static constexpr int numVoices = 64;
    static constexpr int scopeSize = 1024;

    ReggaeGuitarProcessor();
    ~ReggaeGuitarProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    // presets de fábrica = os guitarristas
    int getNumPrograms() override { return mrg::NumPlayers; }
    int getCurrentProgram() override { return juce::jlimit (0, mrg::NumPlayers - 1, (int) get ("player")); }
    void setCurrentProgram (int index) override { applyPlayer (index); }
    const juce::String getProgramName (int index) override { return mrg::player (index).name; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- usados pela interface
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    std::atomic<int> statusStep { -1 }, statusBar { -1 };
    std::atomic<bool> running { false };
    float scope[scopeSize] {};
    std::atomic<int> scopeWrite { 0 };

    void applyPlayer (int index);            // carrega guitarra, ampli, golpe, abafamento, efeitos, BPM e variação
    void loadVariation (int type);           // preenche o compasso da variação com o modelo
    int currentTrack() const { return trackIndex.load(); }
    void setTrack (int t);

    float get (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float value);
    static juce::String stepId (int i) { return "s" + juce::String (i + 1); }
    static juce::String fillId (int i) { return "v" + juce::String (i + 1); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    mrg::GrooveSettings grooveSettings() const;
    mrg::SoundParams soundParams (double bpm) const;
    std::array<int, 4> chordForBar (long long bar) const;
    std::array<int, 4> chordFromNote (int note) const;
    void doAction (const mrg::Action& a, const std::array<int, 4>& chord, int sampleOffset, double bpm);
    void strum (const std::array<int, 4>& chord, float vel, int offset, double holdSec, int mute, bool up, bool top3,
                double spread, double dbl);
    mrg::StringVoice& freeVoice();
    void playLead (int note, float vel, int offset, double holdSec, bool sustain, int mute, float pan = 0.1f);
    void handleMidi (const juce::MidiMessage& m, int offset, bool grooveRunning);

    std::vector<mrg::StringVoice> voices;   // no heap: cada corda guarda as linhas de atraso (~80 kB)
    mrg::Chain chain;
    mrg::Band band;
    std::vector<float> busL, busR;
    double sr = 48000.0, internalPpq = 0.0, lastPpq = -1.0;
    long long lastStep = -1;
    uint32_t seed = 1;
    std::array<int, 4> keysChord { 57, 60, 64, 69 };
    bool haveKeysChord = false;
    std::atomic<int> trackIndex { 0 };
    double bend = 0.0, vibDepth = 0.0, vibPhase = 0.0;
    juce::Random rnd;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReggaeGuitarProcessor)
};
