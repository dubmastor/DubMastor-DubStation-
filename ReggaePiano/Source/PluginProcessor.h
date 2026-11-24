#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "RootsKeysEngine.h"

class DubChordsProcessor;   // MRCM-1240 inteiro na aba CHORD MACHINE (Source/MachineProcessorUnit.cpp)

struct Preset
{
    const char* name;
    int mode;
    float pianoBright, pianoDecay, pianoChorus, pianoLevel;
    int drawbars[9];
    bool percOn, perc3rd, percFast;
    float click, drive, leslieMix, organLevel;
    bool leslieFast;
    float reverb;
};

class RootsKeysProcessor : public juce::AudioProcessor
{
public:
    RootsKeysProcessor();
    ~RootsKeysProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    static int getNumPianoPresets();
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& drawbarNames();

    juce::AudioProcessorValueTreeState apvts;

    // teclado da interface: pinta as teclas tocadas via MIDI (e permite tocar com o mouse)
    juce::MidiKeyboardState keyboardState;

    // medidores para o VU e o maximizer (escritos no áudio, lidos pela interface)
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f }, gainReduction { 0.0f };
    // rodas de pitch/mod que chegam pelo MIDI (só para mostrar na tela)
    std::atomic<float> pitchWheel { 0.0f }, modWheel { 0.0f };
    // rodas mexidas na tela (a thread de áudio aplica no próximo bloco)
    static constexpr float kNoWheelReq = -9.0f;
    std::atomic<float> uiPitchReq { kNoWheelReq }, uiModReq { kNoWheelReq };
    void setWheelFromUI (int wheel, float value)
    {
        if (wheel == 0) { uiPitchReq.store (juce::jlimit (-1.0f, 1.0f, value)); pitchWheel.store (value); }
        else            { uiModReq.store (juce::jlimit (0.0f, 1.0f, value));    modWheel.store (value); }
    }

    // ---------------- aba CHORD MACHINE (o Mastor Reggae Chord Machine MRCM-1240 inteiro, só maior e menor)
    // SOM INTERNO da machine desligado: os acordes tocam no piano/órgão deste instrumento (com eco, flanger e maximizer).
    // Ligado: o som próprio da machine (piano M1 no skank + Hammond no bubble, ao mesmo tempo), somado à saída.
    DubChordsProcessor& getMachine() noexcept { return *machine; }

private:
    rk::Engine engine;
    int currentProgram = 0;
    std::atomic<float>* dbParam[9] {};

    std::unique_ptr<DubChordsProcessor> machine;
    juce::AudioBuffer<float> machineBuf;
    juce::MidiBuffer machineMidi;
    void runMachine (juce::MidiBuffer& midi, int numSamples);
    void migrateOldDubChords (const juce::ValueTree& oldState);   // projetos do Roots Keys 2.x (aba DUB CHORDS)

    void setParam (const juce::String& id, float value);
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RootsKeysProcessor)
};
