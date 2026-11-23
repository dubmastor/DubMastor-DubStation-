// Mastor Tune MT-0420 — DubStation
// Afinador de voz para reggae: afina de forma transparente (sem mudar o timbre) ou robótica (hard-tune),
// acha a harmonia sozinho escutando o riddim (entrada lateral), lendo o MRM-0420 ou o MIDI de outra
// máquina de harmonia, e tem um editor de notas preso à timeline da DAW.
// Aba PROCESSAMENTO: EQ, saturador, Big Knob, Tape Echo e Maximizer (tudo desligado no início).
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "TuneEngine.h"
#include "NoteTrack.h"
#include "../../BigKnob/Source/TubbyFilter.h"
#include "../../BigKnob/Source/DubFX.h"
#include "../../RiddimMachine/Source/MrmLink.h"
#include "../../KeyScout/Source/ScoutLink.h"

class MastorTuneProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    MastorTuneProcessor();
    ~MastorTuneProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }    // harmonia de outras máquinas por MIDI
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "MT-0420"; }
    void changeProgramName (int, const juce::String&) override {}

    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& keyNames();
    static const juce::StringArray& scaleNames();
    static const juce::StringArray& divisionNames();

    // ---- para a interface
    enum Source { SrcManual = 0, SrcRiddim, SrcMidi, SrcMrm, SrcScout };
    struct Status
    {
        int   source = SrcManual;          // de onde veio a harmonia agora
        bool  mrmOpen = false, mrmPlaying = false, riddimSignal = false, riddimConnected = false, midiChord = false;
        bool  scoutOpen = false, scoutSignal = false; int scoutKey = -1; bool scoutMinor = false;
        bool  tuneBypass = false;
        int   keyRoot = 9, scale = mt::Menor; // tom usado agora
        bool  keyAuto = true;
        int   chordRoot = -1; bool chordMinor = false;
        float sungMidi = 0.0f;  int target = -1;
        float inDb = -100.0f, outPeak[2] { 0, 0 }, grDb = 0.0f;
        float lufs = -70.0f;
    };
    Status getStatus() const;

    NoteTrack& getNotes() { return notes; }
    void reanalyse() { notes.clear(); notesChanged = true; }
    std::atomic<bool> notesChanged { true };
    double getHostBpm() const { return hostBpm.load(); }
    double getPlayPpq() const { return playPpq.load(); }
    bool   isHostPlaying() const { return hostPlaying.load(); }

    // usuário tocou num tom: sai do automático
    void chooseKey (int root);
    void chooseScale (int scale);
    void setAuto (bool on);

    float getP (const char* id) const { return params.at (id)->load(); }

private:
    void timerCallback() override;
    void updateHarmony();

    std::map<std::string, std::atomic<float>*> params;

    mt::TuneEngine engine;
    mt::RiddimEar ear;
    mt::VoiceEQ eq;
    mt::Saturator sat;
    dub::TubbyFilter bigKnob;
    dub::TapeEcho echo;
    dub::Maximizer maximizer;
    dub::LoudnessMeter loudness;
    NoteTrack notes;
    mrmlink::Reader mrmReader;
    scoutlink::Reader scoutReader;

    juce::AudioBuffer<float> work, dryCopy, bkBuf;
    std::array<std::vector<float>, 2> bypassDelay;
    int bypassIdx = 0, totalLatency = 0;
    juce::SmoothedValue<float> eqMix, satMix, bkMix, outGain, bypassMix;
    double sr = 44100.0;
    int64_t samplesDone = 0;
    float punchFast[2] {}, punchSlow[2] {};

    // posição da DAW no bloco atual (para prender as notas à timeline)
    double blockPpq = 0.0, blockBpm = 120.0;
    int64_t blockStart = 0;
    bool blockPlaying = false;

    // harmonia (thread de mensagens -> áudio)
    std::atomic<int>  mrmChordMask { 0 }, mrmKeyRoot { -1 };
    std::atomic<bool> mrmKeyMinor { false }, mrmOpen { false }, mrmPlaying { false };
    std::atomic<bool> scoutOpen { false }, scoutSignal { false }, scoutMinor { false }, scoutChordMinor { false };
    std::atomic<int>  scoutKey { -1 }, scoutChordRoot { -1 };
    std::atomic<int>  usedSource { SrcManual }, usedKey { 9 }, usedScale { 1 }, usedChordRoot { -1 };
    std::atomic<bool> usedChordMinor { false }, riddimSig { false }, riddimConn { false }, midiHeld { false };
    std::atomic<float> sungMidi { 0 }, inDb { -100 }, outPk[2] { { 0 }, { 0 } }, grDb { 0 }, lufsS { -70 };
    std::atomic<int> targetNote { -1 };
    std::atomic<double> hostBpm { 120.0 }, playPpq { 0.0 };
    std::atomic<bool> hostPlaying { false };
    std::array<bool, 128> heldNotes {};

    // análises -> editor de notas
    juce::AbstractFifo frameFifo { 8192 };
    std::vector<HopFrame> frameBuf = std::vector<HopFrame> (8192);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MastorTuneProcessor)
};
