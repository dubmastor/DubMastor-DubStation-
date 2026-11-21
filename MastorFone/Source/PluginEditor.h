#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace mf
{
    juce::Font title (float h);   // Archivo Black (plaqueta)
    juce::Font script (float h);  // TeX Gyre Chorus (assinatura, igual ao Dub Station)
}

// Botão que só fica ligado enquanto está pressionado (THROW)
class MomentaryButton : public juce::TextButton
{
public:
    MomentaryButton (const juce::String& name, juce::RangedAudioParameter* p) : juce::TextButton (name), param (p) {}
    void mouseDown (const juce::MouseEvent& e) override { set (true);  juce::TextButton::mouseDown (e); }
    void mouseUp   (const juce::MouseEvent& e) override { set (false); juce::TextButton::mouseUp (e); }
private:
    void set (bool on)
    {
        if (param == nullptr) return;
        param->beginChangeGesture();
        param->setValueNotifyingHost (on ? 1.0f : 0.0f);
        param->endChangeGesture();
    }
    juce::RangedAudioParameter* param;
};

// --------------------------------------------------------------------------
// Visual Dub Station: esmalte creme, plaqueta de alumínio, VU, janela da fita e painel escuro
class StationLookAndFeel : public juce::LookAndFeel_V4
{
public:
    StationLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

// Teclas com os desenhos dos aparelhos (visual Dub Station): clique para escolher
class StationModeKeys : public juce::Component
{
public:
    explicit StationModeKeys (juce::RangedAudioParameter&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    int getMode() const { return mode; }
    int getHover() const { return hover; }
private:
    juce::Rectangle<float> key (int i) const;
    juce::ParameterAttachment att;
    int mode = 0, hover = -1;
};

// Dial de rádio de ondas médias (540–1600 kHz): clique ou arraste para sintonizar
class TuningDial : public juce::Component
{
public:
    explicit TuningDial (juce::RangedAudioParameter&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    float getValue() const { return value; }   // -100..100
    juce::String getText() const;
private:
    juce::Rectangle<float> scale() const;
    float xToValue (float x) const;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    float value = 0.0f;
};

// Osciloscópio de tubo verde: mostra a onda que sai do plugin
class CrtScope : public juce::Component
{
public:
    explicit CrtScope (RadioFoneProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    void update();   // chamado pelo timer do painel
private:
    RadioFoneProcessor& proc;
    std::vector<float> buf = std::vector<float> (2048), trace, prev1, prev2;
};

class StationPanel : public juce::Component, private juce::Timer
{
public:
    explicit StationPanel (RadioFoneProcessor&);
    ~StationPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kW = 960, kH = 620;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        juce::String name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& id, const juce::String& name, int size);
    void setupToggle (Toggle&, const juce::String& id, const juce::String& text);
    void place (Knob&, juce::Rectangle<int> cell, int labelH);
    void renderBackground();
    void drawVuFace (juce::Graphics&);
    void drawVuNeedle (juce::Graphics&);
    void drawTape (juce::Graphics&);
    juce::String statusText();

    RadioFoneProcessor& proc;
    StationLookAndFeel lnf;

    StationModeKeys modeKeys;
    TuningDial dial;
    CrtScope scope;
    Knob band, drive, noise, defect;
    Knob echoDiv, echoMs, echoHeads, echoFb, echoTone, echoWow, echoSend;
    Knob mix, dry, out;
    Toggle echoSync, echoPP, bypassBtn;
    MomentaryButton throwBtn;
    juce::Label status;

    juce::Image background;
    juce::Rectangle<int> topPanel, vuArea, tapeWin, echoPlate, rightArea;
    juce::Rectangle<float> plateArea;
    juce::Point<float> lampPos;
    float needlePos = 0, needleVel = 0, tapePhase = 0, lampGlow = 0;
    int peakHold = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StationPanel)
};

class RadioFoneEditor : public juce::AudioProcessorEditor
{
public:
    explicit RadioFoneEditor (RadioFoneProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        const int w = StationPanel::kW, h = StationPanel::kH;
        setResizable (true, true);
        setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        setSize (w, h);
    }
    void resized() override
    {
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) StationPanel::kW));
    }
private:
    StationPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioFoneEditor)
};
