#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace toon
{
    // paleta "desenho animado dos anos 30"
    const juce::Colour ink    { 0xff1e1410 };
    const juce::Colour cream  { 0xfff4e7c5 };
    const juce::Colour paper  { 0xfffbf3dc };
    const juce::Colour mustard{ 0xfff2b632 };
    const juce::Colour red    { 0xffe2463a };
    const juce::Colour teal   { 0xff3fa7a0 };
    const juce::Colour wall   { 0xff2b7f80 };
    const juce::Colour wood   { 0xffa65a2e };
    const juce::Colour woodDk { 0xff7a3b1e };

    juce::Font title (float h);   // Archivo Black
    juce::Font comic (float h);   // Comic Relief Bold

    // caixa com contorno grosso e sombra dura deslocada (estilo cartoon)
    void box (juce::Graphics&, juce::Rectangle<float>, juce::Colour fill, float corner, float outline = 4.0f, float shadow = 6.0f);
    void outlinedText (juce::Graphics&, const juce::String&, const juce::Font&, juce::Rectangle<float>,
                       juce::Colour fill, juce::Colour outline, float stroke, float shadow,
                       juce::Justification = juce::Justification::centred);
}

class ToonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ToonLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawBubble (juce::Graphics&, juce::BubbleComponent&, const juce::Point<float>& tip, const juce::Rectangle<float>& body) override;
    juce::Font getSliderPopupFont (juce::Slider&) override { return toon::comic (16.0f); }
    int getSliderPopupPlacement (juce::Slider&) override { return juce::BubbleComponent::above; }
    juce::Font getLabelFont (juce::Label&) override { return toon::comic (15.0f); }
};

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

// Seletor dos três aparelhos, com ícones desenhados
class ModeSelector : public juce::Component
{
public:
    explicit ModeSelector (juce::RangedAudioParameter&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    int getMode() const { return mode; }
private:
    juce::Rectangle<float> slot (int i) const;
    static void drawIcon (juce::Graphics&, int which, juce::Rectangle<float>, bool on);
    juce::ParameterAttachment att;
    int mode = 0;
};

// Painel com todo o layout no tamanho base; o editor o escala para a janela redimensionável
class RadioPanel : public juce::Component, private juce::Timer
{
public:
    explicit RadioPanel (RadioFoneProcessor&);
    ~RadioPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kW = 960, kH = 620;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        juce::Colour accent;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name, juce::Colour accent);
    void setupToggle (Toggle&, const juce::String& paramId, const juce::String& text);
    void placeKnob (Knob&, juce::Point<int> centre, int size);
    void renderBackground();
    void drawDialNeedle (juce::Graphics&);
    void drawTape (juce::Graphics&);
    void drawVu (juce::Graphics&);
    void drawLamp (juce::Graphics&);

    RadioFoneProcessor& proc;
    ToonLookAndFeel lnf;

    ModeSelector modeSel;
    Knob tune, band, drive, noise, defect, mix;
    Knob echoDiv, echoMs, echoHeads, echoFb, echoTone, echoWow, echoSend;
    Toggle echoSync, echoPP, bypassBtn;
    MomentaryButton throwBtn;
    Knob dry, out;

    juce::Image background;
    juce::Rectangle<float> dialWin, tapeWin, vuArea, lampArea;
    float reelAngle = 0.0f, vuPos = 0.0f, lampGlow = 0.0f, wiggle = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioPanel)
};

class RadioFoneEditor : public juce::AudioProcessorEditor
{
public:
    explicit RadioFoneEditor (RadioFoneProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        const int w = RadioPanel::kW, h = RadioPanel::kH;
        setResizable (true, true);
        setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        setSize (w, h);
    }
    void resized() override
    {
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) RadioPanel::kW));
    }
private:
    RadioPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioFoneEditor)
};
