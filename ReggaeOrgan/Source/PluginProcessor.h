#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include "GroovePlayer.h"

class ReggaeOrganProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numSlots = 4;

    ReggaeOrganProcessor();
    ~ReggaeOrganProcessor() override = default;

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

    // presets de fábrica = os músicos
    int getNumPrograms() override { return mro::NumMusicians; }
    int getCurrentProgram() override { return juce::jlimit (0, mro::NumMusicians - 1, (int) get ("musico")); }
    void setCurrentProgram (int index) override { applyMusician (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- usados pela interface
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;          // cliques no teclado da tela
    std::atomic<bool> lit[128];                      // notas que o groove está tocando agora (acendem o teclado)
    std::atomic<int> statusBar { -1 }, statusRoot { 9 }, statusStep { -1 };
    std::atomic<bool> statusMinor { true }, statusRunning { false };
    std::atomic<double> playPos { -1.0 };            // posição no loop (tempos), -1 = parado
    std::atomic<float> meter { 0.0f };

    // os três caminhos para escolher o groove
    void applyMusician (int m);                      // MÚSICOS: grade, balanço, fills e registros do organista
    void applyEpoch (int e);                         // ÉPOCAS: os 10 estilos do banco de riddims
    void applyBankEntry (int index);                 // RIDDIMS: harmonia e BPM da música do banco único
    int getBankEntry() const;
    int getEpoch() const;

    // EQUALIZADOR e timbres (1.3)
    void setEq (const mro::EqParams&);
    mro::EqParams getEq() const;
    void applyEqPreset (int i);
    int getEqPreset() const;                         // -1 = ajuste próprio
    void applyMidnite (int i = 0);                   // TIMBRES MIDNITE (Ron Benjamin): registros, Leslie, drive, click e EQ
    int midniteIndex() const;                        // -1 = nenhum timbre Midnite carregado
    bool isMidnite() const { return midniteIndex() >= 0; }

    // atalhos da grade
    void applyFeel (int feel, int density);
    void setGridCell (bool rightHand, int step, int level);
    mro::Grid getGrid() const;
    void newVariation();                             // JACKPOT: outra variação

    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    float get (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float value);
    std::vector<mro::Chord> getChords() const;
    mro::Settings getSettings() const;
    double exportBpm() const;

    // ---- MIDI: o loop que está tocando (gerado ou editado no visor)
    struct Loop { std::vector<mro::Event> events; int bars = 0; };
    Loop getCurrentLoop() const;
    bool hasEdits() const { return std::atomic_load (&edits) != nullptr; }
    void setEdits (const std::vector<mro::Event>& events, int bars);
    void clearEdits();
    // layer = -1: as três camadas em trilhas separadas; 0..2: só uma camada
    bool writeMidiFile (const juce::File& file, int layer = -1) const;
    static const juce::StringArray& drawbarNames();
    const mro::LeslieDSP& leslieState() { return trio.leslie(); }   // para a animação da Leslie

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    mro::SoundParams getSound() const;
    static juce::String gridId (bool rightHand, int step) { return (rightHand ? "r" : "l") + juce::String (step + 1); }

    mro::OrganTrio trio;
    mro::GroovePlayer player;
    std::vector<mro::NoteCmd> cmds;
    bool held[128] {};
    mro::Chord keysChord;
    bool haveKeysChord = false;
    double sampleRateHz = 44100.0, previewPpq = 0.0;
    std::atomic<double> hostBpm { 0.0 };
    std::shared_ptr<const Loop> edits;               // trocado com atomic_load/atomic_store (interface x áudio)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReggaeOrganProcessor)
};
