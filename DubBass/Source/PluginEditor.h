#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Visual da família DubMastor (mesmo do Dub Chords / Big Knob): painel de metal creme envelhecido,
// laterais de madeira, knobs de baquelite com saia de alumínio, lâmpadas piloto e displays âmbar.
class DubLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DubLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

//==============================================================================
// Fileira de botões de pressão intertravados com lâmpada (escolhe um parâmetro choice)
class ChoiceKeys : public juce::Component
{
public:
    ChoiceKeys (juce::RangedAudioParameter& p, juce::StringArray names);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    juce::StringArray labels;
    float value = 0.0f;
};

//==============================================================================
// Válvula 12AX7 animada: esquenta com o DRIVE e com o sinal; a cor vai do vermelho apagado
// ao laranja, âmbar e branco quente (com o brilho azul de válvula forçada no limite)
class TubeGlow : public juce::Component
{
public:
    TubeGlow() { setInterceptsMouseClicks (false, false); }
    void update (bool on, float driveDb, float level);
    void paint (juce::Graphics&) override;
    float getHeat() const noexcept { return heat; }
private:
    float heat = 0.0f, flicker = 0.0f;
    juce::Random rng;
};

//==============================================================================
// VU digital do maximizer no estilo Ozone: entrada L/R, redução de ganho e saída L/R,
// com leitura numérica, pico retido e marca do teto (ceiling)
class OzoneMeter : public juce::Component
{
public:
    explicit OzoneMeter (DubBassAudioProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void update();
    void paint (juce::Graphics&) override;
private:
    DubBassAudioProcessor& proc;
    float in[2]  { -100.0f, -100.0f }, out[2]  { -100.0f, -100.0f };
    float inHold[2] { -100.0f, -100.0f }, outHold[2] { -100.0f, -100.0f };
    int   inHoldT[2] { 0, 0 }, outHoldT[2] { 0, 0 };
    float gr = 0.0f, grHold = 0.0f;
    int   grHoldT = 0;
};

//==============================================================================
// Osciloscópio em tela de fósforo verde
class CrtScope : public juce::Component
{
public:
    explicit CrtScope (DubBassAudioProcessor& p) : proc (p) { ring.fill (0.0f); setInterceptsMouseClicks (false, false); }
    void update();
    void paint (juce::Graphics&) override;
private:
    DubBassAudioProcessor& proc;
    static constexpr int ringSize = 8192, displayLen = 2048;
    std::array<float, ringSize> ring;
    std::array<float, 2048> temp {};
    int writePos = 0;
    std::vector<float> trace, prev1, prev2;
};

// Curva do EQ gráfico (com o Bass Boost) em tela verde
class CrtEqCurve : public juce::Component
{
public:
    explicit CrtEqCurve (DubBassAudioProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
private:
    DubBassAudioProcessor& proc;
};

//==============================================================================
// Teclado no padrão Dub Station: marfim envelhecido, pretas de baquelite, barra cromada
// com feltro vermelho e lâmpada âmbar nas teclas tocadas (clique ou MIDI da DAW)
class DubKeyboard : public juce::MidiKeyboardComponent
{
public:
    explicit DubKeyboard (juce::MidiKeyboardState& s);
    void paint (juce::Graphics&) override;
protected:
    void drawWhiteNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour noteFillColour) override;
};

//==============================================================================
// Painel com todo o layout no tamanho base; o editor o escala para a janela redimensionável
class DubBassPanel : public juce::Component, private juce::Timer
{
public:
    explicit DubBassPanel (DubBassAudioProcessor&);
    ~DubBassPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttach> att;
        bool dark = false;           // knob no painel escuro (maximizer)
    };
    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<ButtonAttach> att;
    };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name, bool dark = false);
    void setupToggle (Toggle&, const juce::String& paramId, const juce::String& text);
    void placeKnob (Knob&, juce::Rectangle<int>);
    void renderBackground();
    void drawKnobScale (juce::Graphics&, Knob&, int ticks);

    DubBassAudioProcessor& proc;
    DubLookAndFeel lnf;

    // OSCILADOR
    ChoiceKeys modeKeys;
    Knob harm2, harm3, glide, pitchAmt, pitchTime;
    // ENVELOPE / FILTRO
    Knob attack, decay, sustain, release, cutoff, reso;
    // VÁLVULA / DRIVE
    Toggle drvOn;
    ChoiceKeys drvTypeKeys;
    Knob drive, drvTone, drvMix;
    TubeGlow tube;
    // EQ GRÁFICO
    Toggle eqOn;
    CrtEqCurve eqCurve;
    std::array<juce::Slider, DubBassAudioProcessor::numEqBands> eqFaders;
    std::array<std::unique_ptr<SliderAttach>, DubBassAudioProcessor::numEqBands> eqAtts;
    // REVERB
    Toggle revOn;
    Knob revSize, revDamp, revWidth, revLowCut, revMix;
    // BASS BOOST
    juce::TextButton boostBtn { "BASS BOOST" };
    std::unique_ptr<ButtonAttach> boostAtt;
    // SAÍDA
    Knob output;
    // OSCILOSCÓPIO
    CrtScope scope;
    // TECLADO
    DubKeyboard keyboard;
    // MAXIMIZER (lateral direita)
    OzoneMeter meter;
    Knob maxGain, maxCeiling, maxRelease;
    Toggle maxBypass;

    juce::Image background;
    juce::Rectangle<int> woodL, woodR, plate;
    juce::Rectangle<int> oscPanel, envPanel, scopePanel, drvPanel, eqPanel, revPanel, boostPanel, outPanel, maxPanel, keysPanel;
    juce::Rectangle<int> noteDisplay, tubeArea, faderArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubBassPanel)
};

//==============================================================================
class DubBassAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit DubBassAudioProcessorEditor (DubBassAudioProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        const int w = panel.getWidth(), h = panel.getHeight();
        setResizable (true, true);
        setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        setSize (w * 4 / 5, h * 4 / 5);
    }
    void resized() override
    {
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) panel.getWidth()));
    }
private:
    DubBassPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubBassAudioProcessorEditor)
};
