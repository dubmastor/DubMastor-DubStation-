#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "DubEngine.h"
#include "RootsKeysSound.h"
#include "HarmonyPresets.h"

class DubChordsProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener,
                           private juce::Timer
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

    const juce::String getName() const override { return "Mastor Reggae Chord Machine MRCM-1240"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "MRCM-1240"; }   // nome vazio reprova no validador da Steinberg
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

    // MIDI editável (usado pelo Mastor Riddim Machine MRM-0420). Thread da interface.
    // A edição fica salva no projeto e vale até um controle do gerador (acordes, padrão...) ser mexido.
    std::vector<dub::NoteEvent> getCurrentLoop (double& loopBeatsOut) const;   // editado, se houver
    void setEdits (std::vector<dub::NoteEvent> events, double loopBeatsIn);
    bool hasEdits() const;
    void clearEdits();

    // PRESETS de harmonia (banco DubStation por estilo e década, Source/HarmonyPresets.h)
    void applyPreset (int index);                 // thread da interface: acordes, tipos, durações e ON
    int  getLastPreset() const noexcept { return lastPreset.load(); }   // -1 = nenhum

    // BANCO ÚNICO DE RIDDIMS (RiddimBank/Source/RiddimBank.h, o mesmo das outras machines)
    void applyBankEntry (int index);              // thread da interface: harmonia e BPM da música
    int  getBankEntry() const;                    // entrada atual do banco (-1 = nenhuma)

    // OUVIR O ACORDE: ao escolher tônica, maior/menor ou inversão, o acorde do cartão soa enquanto
    // o botão do mouse estiver apertado (as mesmas notas que o loop toca: inversão, registro e duas mãos).
    // Thread da interface; a thread de áudio toca no próximo bloco.
    void auditionStart (int slot);
    void auditionStop();
    // notas do "ouvir o acorde" saídas no último bloco (o MRK-80 toca mesmo com PREVIEW desligado)
    const juce::MidiBuffer& getAuditionMidi() const noexcept { return auditionOut; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String&, float) override { dirty.store (true); }
    void timerCallback() override;            // monta o loop novo fora da thread de áudio
    void publishLoop();                       // (thread de mensagens) monta e entrega o loop
    bool takeNewLoop();                       // (thread de áudio) pega o loop novo sem alocar
    void allNotesOff (juce::MidiBuffer& midi, int sample);
    void renderSound (juce::AudioBuffer<float>&, const juce::MidiBuffer&);

    float param (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    juce::String settingsKey() const;         // impressão digital dos controles que montam o loop

    std::atomic<int> lastPreset { -1 };
    mutable juce::SpinLock editLock;
    std::vector<dub::NoteEvent> edits;
    double editsLoopBeats = 0.0;
    juce::String editsKey;
    bool editsOn = false;

    RootsKeysSoundEngine sound;
    std::vector<float> renderL, renderR;

    std::vector<dub::NoteEvent> loop;         // só a thread de áudio mexe
    double loopBeats = 0.0;
    std::vector<dub::NoteEvent> incoming;     // loop novo esperando a troca (protegido por swapLock)
    double incomingBeats = 0.0;
    juce::SpinLock swapLock;
    std::atomic<bool> hasIncoming { false };
    std::atomic<bool> dirty { true };
    float pianoGain = 0.0f, organGain = 0.0f;  // últimos ganhos aplicados (rampas sem estalo)

    // ouvir o acorde
    static constexpr int maxAudition = 12;
    juce::SpinLock auditionLock;
    int auditionReqNotes[maxAudition] {}, auditionReqCount = 0, auditionReqVel = 100;
    std::atomic<int> auditionReq { 0 };       // 1 = tocar, 2 = soltar
    int auditionNotes[maxAudition] {}, auditionCount = 0;
    juce::MidiBuffer auditionOut;
    void runAudition (juce::MidiBuffer& midi);

    struct Pending { int sample; bool on; int channel; int note; int vel; };
    std::vector<Pending> pending;

    bool active[17][128] {};
    bool wasPlaying = false;
    bool soundWasOn = true;
    double expectedPpq = 0.0;
    double internalPpq = 0.0;
    double sr = 44100.0;

    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> displayPos { -1.0 };
    std::atomic<double> displayLoop { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubChordsProcessor)
};
