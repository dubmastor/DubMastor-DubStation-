// Mastor Reggae Percussion MRP-77 — processador: 6 peças de percussão, a PERC MACHINE (grade de até 8 compassos)
// e os 38 pads (notas fora da faixa 36–51 do MRD-12).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include "PercEngine.h"

class ReggaePercussionProcessor : public juce::AudioProcessor
{
public:
    ReggaePercussionProcessor();
    ~ReggaePercussionProcessor() override = default;

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
    double getTailLengthSeconds() const override { return 2.0; }
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
    static juce::String tid (const char* base, int t) { return juce::String (base) + juce::String (t); }

    // ---- grade (PERC MACHINE): 6 peças × 128 passos (8 compassos de 16 semicolcheias); 0 = vazio, 1 = fraca, 2 = acento
    int getStep (int track, int step) const { return pat[(size_t) track][(size_t) step].load(); }
    void setStep (int track, int step, int v) { pat[(size_t) track][(size_t) step].store ((uint8_t) v); }
    void loadGroove (int index);
    void clearPattern();
    int numBars() const;
    bool isAudible (int track) const;
    int soundOf (int track) const { return juce::jlimit (0, mrp::numSounds - 1, (int) get (tid ("snd", track))); }
    void cycleSound (int track, int dir);
    juce::String grooveName() const;

    // ---- AUTO MIX: acerta o volume das 6 peças pelo nível real de cada timbre; isLoud marca a peça alta demais
    int autoMix();                          // devolve quantos volumes mudaram
    bool isLoud (int track) const;
    float loudRatio (int track) const;      // nível / alvo (1,8 ≈ +5 dB = ALTO)
    float baseLevel (int track) const;

    // ---- pads e notas MIDI (ajustáveis para o kit do usuário)
    std::array<std::atomic<int>, mrp::numPads> padNote;
    void resetNotes();
    void playPad (int pad, float vel);                 // toque na tela (fila para o áudio)

    // ---- amostras do usuário (uma por peça)
    bool loadSample (int track, const juce::File&);
    void clearSample (int track);
    juce::String sampleName (int track) const { return sampleFile[(size_t) track].getFileName(); }

    // ---- MIDI (arquivo para a DAW): bar = -1 todos, track = -1 todas as peças ouvidas
    bool writeMidiFile (const juce::File&, int track = -1, int bar = -1) const;
    juce::File tempMidi (int track = -1, int bar = -1) const;

    // ---- para a interface: luzes no ritmo
    std::array<std::atomic<uint32_t>, mrp::numPads> padHits {};
    std::array<std::atomic<uint32_t>, mrp::numTracks> trackHits {};
    std::array<std::atomic<float>, mrp::numTracks> trackVel {};
    std::atomic<int> playStep { -1 };      // passo tocando (0..127), -1 = parado
    std::atomic<float> outLevel { 0.0f };
    std::atomic<int> grooveIndex { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void trigger (int track, int snd, float vel, bool lg, int atSample, float stepDur);

    std::array<std::array<std::atomic<uint8_t>, 128>, mrp::numTracks> pat;
    mrp::Synth synth;
    mrp::FxChain fx;
    double sr = 48000.0;
    double previewBeat = 0.0;
    bool wasPreview = false;
    mrp::Rng rng;

    std::array<std::shared_ptr<const mrp::SampleData>, mrp::numTracks> samples;
    std::array<juce::File, mrp::numTracks> sampleFile;
    std::array<std::atomic<float>, mrp::numTracks> sampleLvl {};

    // toques da tela → áudio
    juce::AbstractFifo uiFifo { 64 };
    std::array<std::pair<int, float>, 64> uiQueue {};

    struct Ev { int at; int track; int snd; float vel; bool lg; bool guide; int step; };
    std::vector<Ev> evs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReggaePercussionProcessor)
};
