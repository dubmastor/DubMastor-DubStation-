// DUB BATTERY REGGAE 1.0 — Dub Station
// Sampler de bateria com 16 pads (notas 36..51), estilo Native Instruments Battery / MPC / SP-404:
// cada pad tem sample com início e fim, envelope AHDSR, afinação, filtro, pan, reverse e grupo de choke.
// Já vem com os timbres DM (e provisórios sintetizados nos pads sem sample) e presets de fábrica.
// A aba DUB DRUMS é o Mastor Reggae Drum Machine MRDM-12 inteiro (código usado direto da pasta DubDrums):
// o groove toca nos pads do Battery e o MIDI é exportado/arrastado igual ao plugin separado.
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PadKit.h"

#include "DrumsNames.h"
#include "../../DubDrums/Source/PluginProcessor.h"
#include "DrumsNamesEnd.h"

namespace dbr
{
// Um sample carregado num pad (imutável depois de criado; trocado inteiro, sem travar o áudio)
struct Sample
{
    juce::AudioBuffer<float> audio;      // 1 ou 2 canais
    double rate = 44100.0;
    juce::String name;                   // nome curto no visor
    juce::String ref;                    // "f:3" = timbre de fábrica 3; senão o caminho do arquivo
    std::vector<float> peaks;            // forma de onda para o visor (máximo de cada fatia)
    double lengthMs() const { return audio.getNumSamples() * 1000.0 / rate; }
};
using SamplePtr = std::shared_ptr<const Sample>;
} // namespace dbr

class BatteryProcessor : public juce::AudioProcessor,
                         private juce::Timer
{
public:
    BatteryProcessor();
    ~BatteryProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dub Battery Reggae"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "DBR-1"; }   // nome vazio reprova no validador da Steinberg
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- Battery
    juce::AudioProcessorValueTreeState apvts;
    juce::RangedAudioParameter& padParam (int pad, dbr::P p) const { return *apvts.getParameter (dbr::pid (pad, p)); }
    float padValue (int pad, dbr::P p) const noexcept { return raw[(size_t) pad][(size_t) p]->load(); }

    DubDrumsProcessor& getDrums() noexcept { return drums; }

    // samples (thread da interface)
    dbr::SamplePtr getSample (int pad) const       { return std::atomic_load (&padSamples[(size_t) pad]); }
    void setFactorySample (int pad, int factoryIndex);
    bool loadSampleFile (int pad, const juce::File&);
    int  factoryIndexOf (int pad) const;           // -1 = arquivo do usuário
    static int numFactorySamples();
    static juce::String factorySampleName (int index);

    // tocar um pad pela interface (clique)
    void triggerPad (int pad, int velocity);
    void releasePad (int pad);

    // luzes e visor
    juce::uint32 getHitCount (int pad) const noexcept { return hitCount[(size_t) pad].load(); }
    float getHitVelocity (int pad) const noexcept     { return hitVel[(size_t) pad].load(); }
    float getPlayPosition (int pad) const noexcept    { return playPos[(size_t) pad].load(); }   // 0..1 no sample, -1 parado
    float getOutputLevel() const noexcept             { return outLevel.load(); }

    // presets: os de fábrica primeiro, depois os gravados (Documentos/Dub Battery Reggae/Presets)
    juce::StringArray getPresetNames() const;
    int  getNumFactoryPresets() const;
    int  getPresetIndex() const noexcept           { return presetIndex; }
    juce::String getPresetName() const             { return presetName; }
    void loadPreset (int index);
    bool savePreset (const juce::String& name);
    static juce::File userPresetFolder();

    int  selectedPad = 0;                           // pad aberto no visor / envelope (salvo no projeto)
    int  currentTab  = 0;                           // 0 = BATTERY, 1 = DUB DRUMS
    bool isGroovePlaying() const;                   // PLAY GROOVE (o preview do Dub Drums)
    void setGroovePlaying (bool);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    void setPadSample (int pad, dbr::SamplePtr);
    static dbr::SamplePtr makeSample (juce::AudioFormatReader&, juce::String name, juce::String ref);
    void resetPadsToDefault();
    juce::ValueTree presetTree() const;
    void applyPresetTree (const juce::ValueTree&);
    void loadSampleRef (int pad, const juce::String& ref);

    //==========================================================================
    struct Voice
    {
        bool active = false;
        int pad = 0, note = 0;
        dbr::SamplePtr sample;
        double pos = 0.0, step = 1.0;
        int first = 0, last = 0;            // região tocada [first, last)
        bool reverse = false, gate = false, released = false;
        enum Stage { Attack, Hold, Decay, Sustain, Release, Choke } stage = Attack;
        float env = 0.0f, stageLeft = 0.0f;
        float gain = 1.0f;
        float ic1[2] {}, ic2[2] {};         // filtro (SVF passa-baixa)
        juce::uint64 age = 0;
    };
    void startVoice (int pad, int velocity);
    void noteOff (int pad);
    void renderVoices (float* l, float* r, int start, int num);

    std::array<std::array<std::atomic<float>*, (size_t) dbr::P::count>, dbr::numPads> raw {};
    std::array<dbr::SamplePtr, dbr::numPads> padSamples;
    std::vector<dbr::SamplePtr> factory;                  // timbres embutidos, decodificados uma vez
    std::vector<dbr::SamplePtr> graveyard;                // samples trocados: liberados fora do áudio
    std::array<Voice, 64> voices;
    juce::uint64 voiceCounter = 0;

    std::array<std::atomic<juce::uint32>, dbr::numPads> hitCount {};
    std::array<std::atomic<float>, dbr::numPads> hitVel {}, playPos {};
    std::atomic<float> outLevel { 0.0f };

    juce::MidiMessageCollector uiNotes;                   // cliques nos pads
    double sr = 44100.0;

    // aba DUB DRUMS: o gerador roda aqui dentro, com o relógio da DAW ou o do PLAY GROOVE
    struct SubPlayHead : juce::AudioPlayHead
    {
        juce::Optional<PositionInfo> getPosition() const override { return info; }
        juce::Optional<PositionInfo> info;
    };
    DubDrumsProcessor drums;
    SubPlayHead drumsHead;
    juce::AudioBuffer<float> drumsBuf;
    juce::MidiBuffer drumsMidi, events;
    double groovePpq = 0.0;
    bool wasGroove = false;

    int presetIndex = 0;
    juce::String presetName { "ROOTS DM" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BatteryProcessor)
};
