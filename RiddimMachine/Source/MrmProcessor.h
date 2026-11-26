// Mastor Riddim Machine MRM-0420 — DubStation
// Junta num instrumento só:
//   MRDM-12   Mastor Reggae Drum Machine   (bateria, com os samples DM)
//   MRCM-1240 Mastor Reggae Chord Machine  (skank de piano e bubble de órgão)
//   MRBM-69   Mastor Reggae Bass Machine   (linha de baixo, com os timbres do MRDBI-07)
//   MRO-68    Mastor Reggae Organ          (aba ÓRGÃO, desde a 2.8: grooves de órgão e timbres, como o plugin separado)
//   Mastor Big Knob                        (CONTROL ROOM: filtro, tape echo, sirene e maximizer no master)
// A harmonia (4 acordes) é uma só para o MRCM-1240 e o MRBM-69, e o BPM é um só para todos.
// Tudo roda no mesmo relógio: a bateria começa no compasso 1 (ou na intro), a harmonia e o baixo
// entram depois da intro, e o PLAY RIDDIM toca tudo junto sem a DAW.
#pragma once

#include "SubPlugins.h"
#include "MrmLink.h"
#include "MrmExtras.h"

class MrmProcessor : public juce::AudioProcessor,
                     private juce::Timer
{
public:
    MrmProcessor();
    ~MrmProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mastor Riddim Machine MRM-0420"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "MRM-0420"; }   // nome vazio reprova no validador da Steinberg
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- MRM-0420
    juce::AudioProcessorValueTreeState apvts;      // play, bpm, tamanho, semente, máquinas ligadas (visíveis na DAW)

    DubChordsProcessor&   getChords() noexcept { return chords; }
    DubDrumsProcessor&    getDrums()  noexcept { return drums; }
    BassMachineProcessor& getBass()   noexcept { return bass; }
    DubKnobProcessor&     getKnob()   noexcept { return knob; }
    ReggaeOrganProcessor& getOrgan()  noexcept { return organ; }

    enum Machine { Drums = 0, Chords, Bass, NumMachines };
    // 2.8 · o órgão (MRO-68) vem depois das três máquinas do riddim: tem ligar, mute, volume, pan e solo,
    // e no ARRANJO toca onde a harmonia (RIDDIM) toca. Organ = NumMachines.
    static constexpr int Organ = NumMachines, NumMixer = NumMachines + 1;
    static const char* machineOnId (int m);        // "drumsOn", "chordsOn", "bassOn"
    bool isMachineOn (int m) const;
    static const char* muteId (int m);             // CONTROL ROOM: "muteDrums", "muteChords", "muteBass"
    bool isAudible (int m) const;                  // ligada e sem mute

    static constexpr int numLengths = 5;
    static int lengthBars (int index);             // 4, 8, 16, 32, 64 compassos
    int    riddimBars() const;
    double introBeats() const;                     // 4 com a VIRADA DE INTRO do MRDM-12 ligada

    void newRiddim();
    void newLine (int machine);                    // NOVA LINHA: semente nova só para a bateria (Drums) ou só para o baixo (Bass)                              // JACKPOT: sorteia outra semente para a bateria e o baixo
    void applyRiddimPreset (int index);           // PRESET de riddim: harmonia + BPM + estilo + baterista + baixista
    void applyBankRiddim (int index);             // BANCO ÚNICO (RiddimBank.h): a música inteira nas três máquinas
    int  currentBankRiddim() const;               // -1 = nenhuma
    void randomRiddim();                           // ALAVANCA: aleatoriedade total (harmonia, durações, estilo e
                                                   // baterista, padrão do skank, baixista e semente)

    // MIDI: o riddim inteiro (só as máquinas ligadas) ou uma máquina só, como no plugin dela
    bool writeRiddimFile (const juce::File&) const;
    juce::String suggestedRiddimName() const;
    bool writeMachineFile (int machine, const juce::File&) const;
    juce::String suggestedMachineName (int machine) const;

    // posição tocando agora (semínimas desde o início do riddim, intro incluída); -1 = parado
    double getPlayBeat() const noexcept     { return playBeat.load(); }
    bool   isMasterPlaying() const noexcept { return masterPlaying.load(); }
    bool   isDawPlaying() const noexcept    { return hostPlaying.load(); }
    double getDawBpm() const noexcept       { return hostBpm.load(); }
    float  getOutputLevel() const noexcept  { return outLevel.load(); }       // 0..1, alto-falante da aba RIDDIM

    // abas: PRINCIPAL, BATERIA, BAIXO, RIDDIM (harmonia · keys), ARRANJO, MIXER, SHOW, CONTROL ROOM
    // (a ordem das máquinas é sempre bateria, baixo e riddim)
    enum Tab { TabMain = 0, TabDrums, TabBass, TabChords, TabOrgan, TabArrange, TabMixer, TabShow, TabControl };
    static constexpr int numTabs = 9;
    int  getCurrentTab() const noexcept     { return currentTab.load(); }
    void setCurrentTab (int t) noexcept     { currentTab.store (juce::jlimit (0, numTabs - 1, t)); }

    // ---- MIXER: volume (dB), pan e solo de cada máquina, medidor depois do fader
    static const char* volId (int m);   // "volDrums", "volChords", "volBass"
    static const char* panId (int m);
    static const char* soloId (int m);
    float getMachineLevel (int m) const noexcept { return meters[(size_t) juce::jlimit (0, NumMixer - 1, m)].load(); }

    // ---- ARRANJO (editado na aba ARRANJO; vale quando "TOCAR O ARRANJO" está ligado)
    mrm::Arrangement getArrangement() const;
    void setArrangement (const mrm::Arrangement&);
    int  getArrangeSlot() const noexcept    { return arrSlot.load(); }      // parte tocando agora (-1 = nenhuma)
    int  getArrangeBar() const noexcept     { return arrBar.load(); }       // compasso dentro da parte
    bool isArrangeEnded() const noexcept    { return arrEnded.load(); }
    void requestArrangeJump (int slot) noexcept { arrJump.store (slot); }   // só no PLAY RIDDIM (a DAW manda no relógio dela)

    // ---- SHOW: 10 riddims salvos no projeto (o estado inteiro, menos o próprio SHOW)
    static constexpr int numShowSlots = 10;
    struct ShowSlot { juce::String name, info; bool used = false; };
    ShowSlot getShowSlot (int i) const;
    void saveShowSlot (int i, const juce::String& name);
    bool loadShowSlot (int i);
    void renameShowSlot (int i, const juce::String& name);
    void clearShowSlot (int i);
    bool exportShow (const juce::File&) const;
    bool importShow (const juce::File&);
    int  getShowCurrent() const noexcept    { return showCurrent; }

    // ---- EXPORTAR A MÚSICA: o arranjo inteiro renderizado fora do tempo real (WAV 24 bits) + o MIDI dele
    struct ExportJob;
    bool startSongExport (const juce::File& wav);   // false se já tem uma exportação rodando
    float getExportProgress() const noexcept { return exportProgress.load(); }   // 0..1, -1 = parado
    juce::String getExportMessage() const;
    double songSeconds() const;

    bool offlineRender = false;   // instância usada só para exportar (não publica o link com o Mastor Tune)

private:
    // Relógio que cada máquina enxerga (o da DAW, o do PLAY RIDDIM ou o da DAW atrasado pela intro)
    struct SubPlayHead : juce::AudioPlayHead
    {
        juce::Optional<PositionInfo> getPosition() const override { return info; }
        juce::Optional<PositionInfo> info;
    };

    // Parâmetros ligados entre máquinas (harmonia, BPM): cada ponto converte para um valor comum
    struct LinkPoint
    {
        juce::AudioProcessorValueTreeState* state;
        juce::String id;
        std::function<int (float)> toCommon;
        std::function<float (int)> fromCommon;
    };
    struct Link { std::vector<LinkPoint> points; std::vector<int> last; };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void buildLinks();
    void syncLinks();
    void timerCallback() override;
    float raw (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    void processDelayed (juce::AudioProcessor&, SubPlayHead&, juce::AudioBuffer<float>& out,
                         juce::MidiBuffer& outMidi, juce::MidiBuffer& scratch,
                         const juce::AudioPlayHead::PositionInfo&, bool playing, double blockBeats,
                         const juce::MidiBuffer* input, int numSamples);

    DubChordsProcessor   chords;
    DubDrumsProcessor    drums;
    BassMachineProcessor bass;
    DubKnobProcessor     knob;
    ReggaeOrganProcessor organ;
    SubPlayHead chordsHead, drumsHead, bassHead, knobHead, organHead;

    juce::AudioBuffer<float> chordsBuf, drumsBuf, bassBuf, organBuf, mixBuf, fxBuf;
    juce::MidiBuffer chordsMidi, drumsMidi, bassMidi, scratchMidi, emptyMidi;

    double sr = 44100.0;
    bool wasMaster = false;
    double masterPpq = 0.0;
    float gains[NumMixer] { 1.0f, 1.0f, 1.0f, 1.0f };
    float fxGain = 1.0f;

    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> playBeat { -1.0 };
    std::atomic<bool>   masterPlaying { false }, hostPlaying { false };
    std::atomic<float>  outLevel { 0.0f };
    std::atomic<int>    currentTab { 0 };

    // ARRANJO: o editor muda a cópia de mensagem; o áudio lê um instantâneo imutável
    mutable juce::CriticalSection arrLock;
    mrm::Arrangement arrangement { mrm::Arrangement::makeDefault() };
    std::shared_ptr<const mrm::Arrangement> arrAudio;
    std::atomic<int> arrSlot { -1 }, arrBar { -1 };
    std::atomic<bool> arrEnded { false };
    std::atomic<int> arrJump { -1 };

    // MIXER
    float panL[NumMixer] { 1, 1, 1, 1 }, panR[NumMixer] { 1, 1, 1, 1 };
    std::array<std::atomic<float>, NumMixer> meters {};
    juce::MidiBuffer spanMidi, spanIn;
    void renderSpan (int off, int len, const juce::AudioPlayHead::PositionInfo&, bool playing,
                     const juce::MidiBuffer& input, const std::array<bool, NumMachines>& sectionOn, bool master);
    float targetGain (int m, bool sectionOn) const;

    // SIRENE + Mastor Tape Echo MTE-12
    mrm::Siren siren;
    fitaeco::TapeEcho sirenEcho;
    juce::AudioBuffer<float> sirenBuf, sirenWet;
    float sirenEchoMix = 1.0f;
    juce::int64 sirenIdle = 1 << 30;   // amostras desde que a sirene calou

    // SHOW
    juce::ValueTree showTree { "SHOW" };
    int showCurrent = -1;
    juce::XmlElement buildState (bool withShow);

    // EXPORTAR
    std::unique_ptr<juce::Thread> exportThread;
    std::atomic<float> exportProgress { -1.0f };
    juce::String exportMessage;
    mutable juce::CriticalSection exportLock;
    friend struct ExportJob;

    std::vector<Link> links;
    int lastSeed = -1;
    mrmlink::Writer linkWriter;   // harmonia para o Mastor Tune MT-0420
    void publishLink();
    bool lastPreview[NumMixer] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MrmProcessor)
};
