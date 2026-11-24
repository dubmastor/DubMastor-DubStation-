// Monstro Machine MM-2500 — processador: 16 pads com os kits do Monstro.zip (Academia de Beats),
// 4 SEQs de 16 passos, timbres da mesma família do mesmo produtor, amostras do usuário, saturador e tape echo.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include <map>
#include "MonstroEngine.h"

class MonstroProcessor : public juce::AudioProcessor
{
public:
    MonstroProcessor();
    ~MonstroProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    float get (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float plainValue);
    static juce::String pid (const char* base, int pad) { return juce::String (base) + juce::String (pad); }

    // ---- produtor (PROGRAM): carrega o BPM, o swing e o groove do produtor; cada produtor tem vários kits
    int kit() const { return juce::jlimit (0, mm::numKits() - 1, (int) get ("kit")); }
    void selectKit (int index, bool keepSeqs = false);
    void loadStyle (const juce::String& styleKey);          // troca os 4 SEQs pelo groove de outro estilo
    void loadProducerGroove();                               // volta para o groove do produtor
    int pk() const { return juce::jlimit (0, juce::jmax (0, kitCount() - 1), (int) get ("pkit")); }
    int kitCount() const { return kitCountOf (kit()); }      // KIT 1…N do produtor (da pasta ou de fábrica)
    int kitCountOf (int producer) const;
    void selectPk (int n);
    juce::String kitLabel (int producer, int n) const;      // nome do kit da pasta (vazio nos kits de fábrica)
    bool hasLib (int producer) const { return lib.count (producer) > 0; }
    int lastLoadProducers = 0;
    bool styleIsProducer() const { return styleOverride.isEmpty(); }
    juce::String style() const { return styleOverride.isNotEmpty() ? styleOverride : juce::String (mm::kits()[(size_t) kit()].style); }

    // ---- sequências: 4 SEQs × 16 pads × 16 passos (0 vazio, 1 nota, 2 acento)
    int seq() const { return juce::jlimit (0, mm::numSeqs - 1, (int) get ("seq")); }
    int getStep (int s, int pad, int step) const { return seqs[(size_t) s][(size_t) pad][(size_t) step].load(); }
    void setStep (int s, int pad, int step, int v) { seqs[(size_t) s][(size_t) pad][(size_t) step].store ((uint8_t) v); }
    int notesIn (int s) const;
    void clearSeq (int s);
    void pushUndo();
    bool undo();

    // ---- timbres: ◀ ▶ troca dentro da família, no kit do mesmo produtor (ou nos arquivos da pasta carregada)
    int variation (int pad) const { return (int) get (pid ("var", pad)); }
    int varCount (int pad) const;
    void setVariation (int pad, int v);
    juce::String timbreName (int pad) const;
    bool hasUserFiles (int pad) const;

    // ---- amostras do usuário
    bool loadSample (int pad, const juce::File&);
    void clearSample (int pad);
    int loadFolder (const juce::File& dir);                  // separa por produtor e por kit; devolve quantos kits achou
    void clearUserKit();                                     // tira os sons do pad (a pasta continua carregada)
    void forgetFolders();                                    // volta tudo para os kits de fábrica
    juce::String userKitName() const { return hasLib (kit()) ? kitLabel (kit(), pk()) : juce::String(); }
    bool hasSample (int pad) const { return std::atomic_load (&samples[(size_t) pad]) != nullptr; }

    // ---- toque na tela / MIDI que chega
    void playPad (int pad, float vel);
    std::array<std::atomic<uint32_t>, mm::numPads> padHits {};
    std::atomic<int> playStep { -1 };
    std::atomic<float> outLevel { 0.0f };
    std::atomic<bool> recording { false };
    // a tela lembra onde o usuário estava
    int uiPad = 0, uiKitVol = 1; juce::String uiScreen { "main" };

    // ---- MIDI para a DAW (seqIndex = -1: A, B, C e D em sequência)
    bool writeMidiFile (const juce::File&, int seqIndex) const;
    juce::File tempMidi (int seqIndex) const;
    double currentBpm() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void refreshCache();

    std::array<std::array<std::array<std::atomic<uint8_t>, mm::numSteps>, mm::numPads>, mm::numSeqs> seqs;
    std::vector<std::vector<uint8_t>> undoStack;
    juce::String styleOverride;

    mm::Synth synth;
    mm::Fx fx;
    double sr = 48000.0, previewBeat = 0.0;
    bool wasPreview = false;
    std::atomic<double> hostBpm { 0.0 };

    // caráter do kit e variações (recalculados quando o kit ou o timbre mudam)
    int cachedKit = -1;
    mm::Char ch {};
    std::array<int, mm::numPads> cachedVar {};
    std::array<mm::Var, mm::numPads> vars {};

    std::array<std::shared_ptr<const mm::SampleData>, mm::numPads> samples;
    std::array<juce::File, mm::numPads> sampleFile;
    std::array<juce::Array<juce::File>, mm::numFams> famFiles;   // arquivos do kit atual por família
    struct UserKit { juce::String name; std::array<juce::Array<juce::File>, mm::numFams> files; };
    std::map<int, std::vector<UserKit>> lib;                      // kits da pasta, por produtor
    juce::StringArray folders;                                    // pastas carregadas (salvas no projeto)
    std::map<int, std::vector<UserKit>> buildLib (const juce::File& dir) const;
    void applyUserKit();

    juce::AbstractFifo uiFifo { 64 };
    std::array<std::pair<int, float>, 64> uiQueue {};
    struct Ev { int at; int pad; float vel; int step; };
    std::vector<Ev> evs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonstroProcessor)
};
