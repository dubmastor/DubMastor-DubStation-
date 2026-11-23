// Mastor Riddim Machine MRM-0420 — DubStation
// Junta num instrumento só:
//   MRDM-12   Mastor Reggae Drum Machine   (bateria, com os samples DM)
//   MRCM-1240 Mastor Reggae Chord Machine  (skank de piano e bubble de órgão)
//   MRBM-69   Mastor Reggae Bass Machine   (linha de baixo, com os timbres do MRDBI-07)
//   Mastor Big Knob                        (CONTROL ROOM: filtro, tape echo, sirene e maximizer no master)
// A harmonia (4 acordes) é uma só para o MRCM-1240 e o MRBM-69, e o BPM é um só para todos.
// Tudo roda no mesmo relógio: a bateria começa no compasso 1 (ou na intro), a harmonia e o baixo
// entram depois da intro, e o PLAY RIDDIM toca tudo junto sem a DAW.
#pragma once

#include "SubPlugins.h"
#include "MrmLink.h"

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

    enum Machine { Drums = 0, Chords, Bass, NumMachines };
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

    static constexpr int numTabs = 5;              // RIDDIM, HARMONIA, BATERIA, BAIXO, CONTROL ROOM
    int  getCurrentTab() const noexcept     { return currentTab.load(); }
    void setCurrentTab (int t) noexcept     { currentTab.store (juce::jlimit (0, numTabs - 1, t)); }

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
    SubPlayHead chordsHead, drumsHead, bassHead, knobHead;

    juce::AudioBuffer<float> chordsBuf, drumsBuf, bassBuf, mixBuf, fxBuf;
    juce::MidiBuffer chordsMidi, drumsMidi, bassMidi, scratchMidi, emptyMidi;

    double sr = 44100.0;
    bool wasMaster = false;
    double masterPpq = 0.0;
    float gains[NumMachines] { 1.0f, 1.0f, 1.0f };
    float fxGain = 1.0f;

    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> playBeat { -1.0 };
    std::atomic<bool>   masterPlaying { false }, hostPlaying { false };
    std::atomic<float>  outLevel { 0.0f };
    std::atomic<int>    currentTab { 0 };

    std::vector<Link> links;
    int lastSeed = -1;
    mrmlink::Writer linkWriter;   // harmonia para o Mastor Tune MT-0420
    void publishLink();
    bool lastPreview[NumMachines] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MrmProcessor)
};
