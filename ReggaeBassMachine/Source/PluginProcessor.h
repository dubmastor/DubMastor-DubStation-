#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "BassEngine.h"
#include "DubBassTimbre.h"
#include "AccompEngine.h"
#include "../../ReggaeDrumMachine/Source/DrumSynth.h"      // bateria acústica do Dub Drums
#include "../../ReggaeChordMachine/Source/DubSynth.h"      // piano M1 + Hammond do Roots Keys

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

    // ouvir a tônica ao escolher (teclado da TÔNICA, mesmo sistema da CHORD MACHINE): a nota do baixo soa enquanto o mouse estiver apertado
    void auditionStart (int slot);
    void auditionStop();
    const juce::MidiBuffer& getAuditionMidi() const noexcept { return auditionOut; }
    // notas da linha soando agora em cada acorde (bit 0..11 = dó..si), para pintar o teclado da tônica no play
    juce::uint32 takePlayingMask (int slot) noexcept
    {
        if (! juce::isPositiveAndBelow (slot, numSlots)) return 0;
        return playMask[(size_t) slot].load() | hitMask[(size_t) slot].exchange (0);
    }
    void newIdea();                                                      // sorteia outra IDEIA

    // MIDI editável (thread da interface). A linha editada fica salva no projeto e vale até um
    // controle do gerador (estilo, ideia, harmonia, densidade...) ser mexido, como no MRDM-12.
    std::vector<mbm::NoteEvent> getCurrentLine (double& loopBeatsOut) const;   // editada, se houver
    // reggae "de livro" (todas as notas na pentatônica do acorde): o MRM-0420 liga; o MRBM-69 sozinho fica livre
    void setStrictReggae (bool on) { if (strictReggae.exchange (on) != on) dirty.store (true); }
    void setEdits (std::vector<mbm::NoteEvent> events, double loopBeatsIn);
    bool hasEdits() const;
    void clearEdits();
    void dropStaleEdits();                                               // chamada pelo timer da interface

    // RIDDIMS CLÁSSICOS (RiddimPresets.h): carrega harmonia, estilo, BPM, bateria e timbre de um riddim.
    // O número fica salvo no projeto (propriedade "riddim" do estado; -1 = nenhum).
    void loadRiddim (int index);
    int currentRiddim() const { return (int) apvts.state.getProperty ("riddim", -1); }

    // BANCO ÚNICO DE RIDDIMS (RiddimBank/Source/RiddimBank.h, o mesmo das outras machines): harmonia e BPM
    // da música; o baixo é o do riddim do MRBM quando ele existe, senão o baixista mais próximo de quem gravou.
    // O número fica salvo no projeto (propriedade "bank"; -1 = nenhum).
    void applyBankEntry (int index);
    int currentBank() const { return (int) apvts.state.getProperty ("bank", -1); }

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
    std::atomic<bool> strictReggae { false };

    struct Pending { int sample; bool on; int note; int vel; int slot; };
    std::vector<Pending> pending;

    bool active[128] {};
    signed char activeSlot[128] {};
    std::array<std::atomic<juce::uint32>, numSlots> playMask {}, hitMask {};
    void publishPlaying();

    std::atomic<int> auditionReq { 0 }, auditionReqNote { -1 }, auditionReqVel { 100 };   // 1 = tocar, 2 = soltar
    int auditionNote = -1;
    juce::MidiBuffer auditionOut;
    void runAudition (juce::MidiBuffer&);
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
