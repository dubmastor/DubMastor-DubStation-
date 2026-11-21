#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>

#include "BassVoice.h"
#include "Maximizer.h"
#include "ScopeFifo.h"

namespace ParamID
{
    // Oscilador
    inline constexpr const char* mode       = "mode";
    inline constexpr const char* harm2      = "harm2";
    inline constexpr const char* harm3      = "harm3";
    inline constexpr const char* glide      = "glide";
    inline constexpr const char* pitchAmt   = "pitchAmt";
    inline constexpr const char* pitchTime  = "pitchTime";
    // Amp / filtro
    inline constexpr const char* attack     = "attack";
    inline constexpr const char* decay      = "decay";
    inline constexpr const char* sustain    = "sustain";
    inline constexpr const char* release    = "release";
    inline constexpr const char* cutoff     = "cutoff";
    inline constexpr const char* reso       = "reso";
    // Drive / saturação
    inline constexpr const char* drvOn      = "drvOn";
    inline constexpr const char* drvType    = "drvType";
    inline constexpr const char* drive      = "drive";
    inline constexpr const char* drvTone    = "drvTone";
    inline constexpr const char* drvMix     = "drvMix";
    // EQ gráfico
    inline constexpr const char* eqOn       = "eqOn";
    // Reverb
    inline constexpr const char* revOn      = "revOn";
    inline constexpr const char* revSize    = "revSize";
    inline constexpr const char* revDamp    = "revDamp";
    inline constexpr const char* revWidth   = "revWidth";
    inline constexpr const char* revLowCut  = "revLowCut";
    inline constexpr const char* revMix     = "revMix";
    // Bass boost / maximizer / saída
    inline constexpr const char* boost      = "boost";
    inline constexpr const char* maxBypass  = "maxBypass";
    inline constexpr const char* maxGain    = "maxGain";
    inline constexpr const char* maxCeiling = "maxCeiling";
    inline constexpr const char* maxRelease = "maxRelease";
    inline constexpr const char* output     = "output";
}

//==============================================================================
class DubBassAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int   numEqBands = 10;
    static constexpr std::array<float, numEqBands> eqFrequencies { 31.5f, 63.0f, 125.0f, 250.0f, 500.0f,
                                                                   1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f };
    static constexpr float eqQ         = 1.41f;   // banda de ~1 oitava
    static constexpr float boostFreq   = 60.0f;
    static constexpr float boostQ      = 0.9f;
    static constexpr float boostGainDb = 4.0f;

    static juce::String eqParamID (int band) { return "eq" + juce::String (band); }

    DubBassAudioProcessor();
    ~DubBassAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double getSampleRateSafe() const noexcept { return currentSampleRate > 0.0 ? currentSampleRate : 48000.0; }

    juce::AudioProcessorValueTreeState apvts;
    ScopeFifo scopeFifo;
    juce::MidiKeyboardState keyboardState;   // teclado da interface: toca e acende com o MIDI
    std::atomic<float> gainReductionDb { 0.0f };

    // medidores para a interface (dBFS de pico por bloco)
    std::array<std::atomic<float>, 2> inPeakDb  { { -100.0f, -100.0f } };   // entrada do maximizer
    std::array<std::atomic<float>, 2> outPeakDb { { -100.0f, -100.0f } };   // saída final
    std::atomic<float> driveLevel { 0.0f };    // nível (0..1+) que entra na válvula, já com o ganho do DRIVE
    std::atomic<float> currentNote { -1.0f };  // nota MIDI soando (-1 = silêncio)

private:
    void updateVoiceParams();
    void renderVoice (float* out, int start, int end);
    void handleMidi (const juce::MidiMessage& msg);
    void processDrive (juce::dsp::AudioBlock<float>& block);
    void updateEq();
    void processReverb (juce::AudioBuffer<float>& buffer, int numSamples);
    void updateBoost (int numSamples);

    struct Params
    {
        std::atomic<float> *mode{}, *harm2{}, *harm3{}, *glide{}, *pitchAmt{}, *pitchTime{},
                           *attack{}, *decay{}, *sustain{}, *release{}, *cutoff{}, *reso{},
                           *drvOn{}, *drvType{}, *drive{}, *drvTone{}, *drvMix{},
                           *eqOn{},
                           *revOn{}, *revSize{}, *revDamp{}, *revWidth{}, *revLowCut{}, *revMix{},
                           *boost{}, *maxBypass{}, *maxGain{}, *maxCeiling{}, *maxRelease{}, *output{};
        std::array<std::atomic<float>*, numEqBands> eq {};
    } prm;

    double currentSampleRate = 0.0;

    // Gerador
    BassVoice voice;
    juce::dsp::StateVariableTPTFilter<float> voiceFilter;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutoffSm;

    // Drive (oversampling 2x)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    double osSampleRate = 0.0;
    juce::SmoothedValue<float> driveGainSm, driveMixSm;
    std::array<float, 2> toneState {}, dcX {}, dcY {};

    // EQ gráfico + bass boost
    using StereoIIR = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    std::array<StereoIIR, numEqBands> eqBands;
    std::array<float, numEqBands> lastEqGain {};
    StereoIIR boostFilter;
    juce::SmoothedValue<float> boostDbSm;
    bool boostNeedsUpdate = true;

    // Reverb (envio paralelo com low-cut no retorno — estilo dub)
    juce::dsp::Reverb reverb;
    juce::dsp::StateVariableTPTFilter<float> reverbHP;
    juce::AudioBuffer<float> reverbBuffer;
    bool reverbWasOn = false;

    // Maximizer + saída
    Maximizer maximizer;
    juce::SmoothedValue<float> outGainSm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubBassAudioProcessor)
};
