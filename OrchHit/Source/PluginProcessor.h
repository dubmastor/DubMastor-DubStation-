// Mastor Orch Hit MOH-88 — processador: o sampler do orchestra hit (HitEngine.h), os efeitos e a grade de 4 compassos.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include "HitEngine.h"

class OrchHitProcessor : public juce::AudioProcessor
{
public:
    OrchHitProcessor();
    ~OrchHitProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    float get (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float plainValue);

    // ---- timbres
    const std::vector<float>& sample (int t) const { return samples[(size_t) juce::jlimit (0, moh::numPresets() - 1, t)]; }

    // ---- grade (4 compassos, ticks de 1/64)
    moh::Grid getGrid() const;
    void setGrid (const moh::Grid&, bool editedByUser);
    bool gridIsCustom() const { return custom.load(); }
    void loadGroove (int i);                      // desenha o groove na grade (vira o groove selecionado)
    bool writeMidiFile (const juce::File&) const;

    // MIDIs do usuário (SALVAR COMO MIDI); os de fábrica estão em moh::clips()
    struct UserClip { juce::String name; std::vector<moh::GridNote> notes; };
    std::vector<UserClip> userClips;              // só na thread da interface

    std::atomic<double> playBeat { -1.0 };        // posição no loop de 16 tempos (visor), -1 = parado
    static juce::String noteName (int n) { return juce::String (moh::rootName (n)) + juce::String (moh::cubaseOctave (n)); }   // C3 = dó central (Cubase)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::vector<std::vector<float>> samples;
    std::shared_ptr<const moh::Grid> grid;
    std::atomic<bool> custom { false };

    moh::Sampler sampler;
    moh::PreviewDrums drums;
    juce::Reverb reverb;
    std::vector<float> echoBuf; int echoPos = 0;
    moh::Biquad echoHp, echoLp;
    std::vector<float> hitBuf, drumBuf, revL, revR;
    double sr = 48000.0, previewBeat = 0.0, lastBlockEndBeat = -1.0;
    bool wasPlaying = false;
    struct Ev { int at; int note; float vel; int64_t len; int kind; };   // kind: 0 nota, 1 solta, 2 bateria (note = tick no compasso)
    std::vector<Ev> evs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchHitProcessor)
};
