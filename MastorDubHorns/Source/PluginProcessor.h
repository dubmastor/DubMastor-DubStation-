#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Version.h"
#include "HornEngine.h"
#include "Phrases.h"

#define HU(text) juce::String::fromUTF8 (text)

class HornsProcessor : public juce::AudioProcessor
{
public:
    HornsProcessor();
    ~HornsProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mastor Dub Horns"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // usado pelo editor (thread de mensagens)
    horns::Phrase getPhrase() const;
    void setPhrase (const horns::Phrase&);
    bool popPickedNote (int& note);              // notas vindas do MIDI da DAW (modo "escolher pelo teclado")
    void previewNote (int note, int artic);      // toca uma nota do naipe (ao escolher na lista)
    bool writeMidiFile (const juce::File& file) const;
    horns::Params currentParams() const;         // parâmetros atuais (para curvas e displays)

    float getVuLevel() const      { return vuLevel.load(); }
    float getEchoMs() const       { return echoMsNow.load(); }
    int   getPlayStep() const     { return playStep.load(); }   // -1 = parado
    bool  isHostPlaying() const   { return hostPlaying.load(); }
    double getBpmForDisplay() const;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    static juce::StringArray instIds();

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float param (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    float param (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void fillParams (horns::Params&, double bpm) const;

    struct SchedEv { int sample; int type; int note; float vel; int artic; int src; }; // type: 0 = off, 1 = on, 2 = todas off da fonte
    std::vector<SchedEv> sched;

    horns::Engine engine;
    std::vector<float> bufL, bufR;
    double sr = 44100.0;

    // frase (editor escreve, áudio lê com tryLock)
    mutable juce::SpinLock phraseLock;
    horns::Phrase phraseShared;
    std::atomic<int> phraseVersion { 1 };
    int phraseVersionAudio = 0;
    horns::Phrase phraseAudio;

    bool activePhrase[128] {};
    bool wasPlaying = false;
    double expectedPpq = 0.0, internalPpq = 0.0;
    bool cc64Throw = false;

    // fifos
    juce::AbstractFifo pickFifo { 64 };
    std::array<int, 64> pickBuf {};
    juce::AbstractFifo previewFifo { 32 };
    std::array<std::pair<int, int>, 32> previewBuf {};
    struct PreviewOff { int note = -1; int samplesLeft = 0; };
    std::array<PreviewOff, 16> previewOffs {};

    // guia one drop (só no clock interno)
    struct Click { float phase = 0, freq = 0, env = 0, decay = 0; };
    std::array<Click, 4> clicks {};
    void renderClicks (float* L, float* R, int n);

    std::atomic<float> vuLevel { 0.0f }, echoMsNow { 375.0f };
    std::atomic<int> playStep { -1 };
    std::atomic<bool> hostPlaying { false };
    std::atomic<double> hostBpm { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HornsProcessor)
};
