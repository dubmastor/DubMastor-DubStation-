// Mastor Reggae Clavinet MRCL-71 — processador: o clavinet (ClavEngine.h) e a CLAVINET MACHINE (LineEngine.h).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include "ClavEngine.h"
#include "LineEngine.h"

class ReggaeClavinetProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    static constexpr int numSlots = 4;
    ReggaeClavinetProcessor();
    ~ReggaeClavinetProcessor() override;

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
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    float get (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float plainValue);

    // ---- CLAVINET MACHINE
    std::vector<mrcl::Chord> getChords() const;          // acordes do plugin (ACORDE 1..4 ligados)
    mrcl::Settings getSettings() const;
    mrcl::Line getSuggestion (int style) const;          // A, B, C
    mrcl::Line getCurrentLine() const;                   // a sugestão em uso (ou o MIDI editado)
    bool hasEdits() const { return std::atomic_load (&edits) != nullptr; }
    void setEdits (const mrcl::Line&);
    void clearEdits();
    void newIdeas();                                     // GERAR NOVAS (outra semente)
    void applyTimbre (int t);
    void applyBankEntry (int i);
    int getBankEntry() const { return bankEntry.load(); }
    bool writeMidiFile (const juce::File&) const;
    double exportBpm() const;

    std::atomic<double> playBeat { -1.0 };               // posição no loop (visor), -1 = parado
    std::atomic<int> midiChordRoot { -1 }, midiChordMinor { 0 };   // acorde ouvido no MIDI da trilha

    static juce::String noteName (int n) { return juce::String (mrcl::rootName (n)) + juce::String (n / 12 - 2); }   // C3 = dó central (Cubase)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    void rebuildLines();
    juce::String settingsKey() const;

    mrcl::ClavInstrument ins;
    double sr = 48000.0;

    // linhas prontas para o áudio: [estilo][acorde do MIDI: 0..23 (tônica * 2 + menor) | 24 = acordes do plugin]
    struct LineSet { std::array<std::array<mrcl::Line, 25>, 3> l; };
    std::shared_ptr<const LineSet> lines;
    std::shared_ptr<const mrcl::Line> edits;
    juce::String lastKey, editsKey;
    std::atomic<int> bankEntry { -1 };

    // agenda das notas da linha
    struct Pending { int64_t at; int note; };
    std::array<Pending, 512> offs {}; int numOffs = 0;
    int64_t sampleClock = 0;
    double previewBeat = 0.0, lastBlockEndBeat = -1.0;
    bool wasPlaying = false;
    std::array<bool, 128> held {};
    struct Ev { int at; int note; float vel; };
    std::vector<Ev> evs = std::vector<Ev> (0);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReggaeClavinetProcessor)
};
