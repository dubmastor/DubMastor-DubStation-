#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "PickEngine.h"

// Mastor Pick Guitar MPG-11 — pick guitar de reggae sintetizada (corda abafada) que nasce do baixo
class PickGuitarProcessor : public juce::AudioProcessor
{
public:
    PickGuitarProcessor();
    ~PickGuitarProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    // presets de fábrica = os guitarristas
    int getNumPrograms() override { return (int) mpg::presets().size(); }
    int getCurrentProgram() override { return juce::jlimit (0, getNumPrograms() - 1, (int) get ("musico")); }
    void setCurrentProgram (int index) override { applyMusician (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- usados pela interface
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;     // teclado da tela = entrada do baixo
    mpg::Engine engine;
    std::atomic<float> meter { 0.0f };
    std::atomic<bool> throwOn { false };       // DUB THROW (segurar)
    static constexpr int kScope = 2048;
    std::array<float, kScope> scope {};
    std::atomic<int> scopeWrite { 0 };

    void applyMusician (int index);
    void applyEqPreset (int index);
    mpg::Params makeParams() const;
    void setParam (const juce::String& id, float value);
    float get (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }

    // grade: '.' 'x' 'X' 'o' 'r' (parâmetros g1..g16, automatizáveis)
    static constexpr const char* kCells = ".xXor";
    char cell (int i) const { return kCells[juce::jlimit (0, 4, (int) get ("g" + juce::String (i + 1)))]; }
    void setCell (int i, char c);
    void cycleCell (int i);
    void setGrid (const char* g) { for (int i = 0; i < mpg::kSteps; ++i) setCell (i, g[i]); }

    // edições do mapa do pica-pau (salvas no projeto)
    int edit (int cyc) const { return edits[(size_t) cyc].load(); }
    void toggleEdit (int cyc);
    void clearEdits() { for (auto& e : edits) e.store (0); }
    int numEdits() const { int n = 0; for (auto& e : edits) n += e.load() != 0; return n; }

    static int repFromIndex (int i) { static const int r[3] = { 3, 2, 4 }; return r[juce::jlimit (0, 2, i)]; }
    static int indexFromRep (int r) { return r == 2 ? 1 : r == 4 ? 2 : 0; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    std::array<std::atomic<int8_t>, mpg::kCycle> edits {};
    double sampleRateHz = 48000.0, previewPpq = 0.0;
    std::vector<mpg::MidiIn> midiIn;
    juce::AudioBuffer<float> work;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PickGuitarProcessor)
};
