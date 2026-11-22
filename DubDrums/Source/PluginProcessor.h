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

    const juce::String getName() const override { return "Mastor Dub Drums Machine MDDM-12"; }
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

    dubdrums::Pattern getPatternCopy() const;     // thread da interface: o padrão que aparece na grade
    int  getPatternVersion() const noexcept       { return patternVersion.load(); }

    // Grade PADRÃO editável (thread da interface). beat = posição na linha do tempo "intro + loop".
    // As edições ficam salvas no projeto e valem até um controle que muda o padrão ser mexido.
    bool toggleNote (int gmNote, double beat, bool add);       // add=true põe, false tira; devolve se mudou
    bool noteAt (int gmNote, double beat) const;
    void changeVelocity (int gmNote, double beat, int delta);
    bool hasEdits() const;
    void clearEdits();

    // posição tocando agora, na linha do tempo "intro + loop" (em semínimas); -1 = parado
    double getPlayheadBeat() const noexcept       { return playheadBeat.load(); }
    bool isHostPlaying() const noexcept           { return hostPlaying.load(); }
    bool isPreviewing() const noexcept            { return previewing.load(); }

    void newVariation();                          // sorteia uma nova semente (e o preview recomeça do início)
    void applyStyleDefaults (int styleIndex);     // bumbo e caixa do estilo na BATIDA PRINCIPAL
    static constexpr int kAllBars = -2;           // bar: -1 = intro, 0..3 = compasso, kAllBars = tudo
    bool writeMidiFile (const juce::File& target, int bar = kAllBars) const;
    juce::String suggestedFileName (int bar = kAllBars) const;
    int mapNote (int gmNote) const;               // nota GM -> nota escolhida pelo usuário

private:
    void parameterChanged (const juce::String&, float) override;
    void timerCallback() override;
    void regenerate();
    void publish (const dubdrums::Pattern&);      // mostra na grade e entrega para a thread de áudio
    void storeEdits (const dubdrums::Pattern&);
    juce::String settingsKey() const;
    void loadSamples();
    dubdrums::Settings readSettings() const;

    // agenda as notas de [start, end) da linha do tempo "intro + loop"
    template <typename Fn> static void forEachEventIn (const dubdrums::Pattern&, double start, double end, Fn&& fn);

    // Padrão em três lugares, para a interface nunca travar o áudio:
    //  - display: o que a grade mostra (thread da interface, protegido por displayLock)
    //  - pending: o próximo padrão para o áudio (trocado com std::swap, sem alocar no áudio)
    //  - active : o que está tocando (só a thread de áudio mexe)
    // O áudio só adota o pendente na virada de um tempo: assim uma nova variação ou outro baterista
    // nunca toca a mesma nota duas vezes nem "embola" o groove no meio do tempo.
    mutable juce::SpinLock displayLock, pendingLock;
    dubdrums::Pattern display, pending, active, generated;
    std::atomic<bool> pendingReady { false };
    std::atomic<bool> restartRequested { false };   // NOVA VARIAÇÃO: o preview volta do começo
    std::atomic<bool> dirty { true };
    std::atomic<int>  patternVersion { 0 };
    std::array<bool, 128> held {};      // notas MIDI ligadas na saída (para desligar na troca)

    std::atomic<double> playheadBeat { -1.0 };
    std::atomic<bool>   hostPlaying  { false };
    std::atomic<bool>   previewing   { false };
    bool wasPlaying = false, wasPreviewing = false;
    double previewPos = 0.0;
    double sr = 44100.0;

    dubdrums::DrumSynth synth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubDrumsProcessor)
};
