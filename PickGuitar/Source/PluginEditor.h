#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <deque>
#include "PluginProcessor.h"
#include "VUMeter.h"        // o mesmo VU do Mastor Reggae Piano MRP-80 (ReggaePiano/Source)
#include "SkinTextures.h"   // a mesma madeira do kit

// cores do Dub Station (as mesmas da versão para navegador)
namespace pg
{
const juce::Colour chassis { 0xff1f1610 }, chassis2 { 0xff2b1f16 }, plate { 0xff3a2a1e }, plate2 { 0xff2e2117 };
const juce::Colour cream { 0xffefe2c4 }, ink { 0xff1b130d }, amber { 0xfff2a541 }, gold { 0xffe6b83a };
const juce::Colour red { 0xffc8402f }, green { 0xff4f9a5a }, blue { 0xff4a8cf0 }, text { 0xfff3e7cf };
const juce::Colour muted { 0xffb9a684 }, line { 0xff5a4430 }, dark { 0xff120c08 };
}

class PickLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PickLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return juce::FontOptions (juce::jmin (14.0f, (float) h * 0.42f), juce::Font::bold); }
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

// knob com nome embaixo, ligado a um parâmetro
class Knob : public juce::Component
{
public:
    Knob (PickGuitarProcessor& p, const juce::String& id, juce::String label, juce::Colour arc = pg::amber);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::Slider slider;
private:
    juce::String name;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// fileira de botões para um parâmetro de escolha (um aceso)
class Chips : public juce::Component
{
public:
    Chips (PickGuitarProcessor& p, const juce::String& id, juce::StringArray labels, juce::Colour on = pg::amber);
    void resized() override;
    std::function<void (int)> onChange;
private:
    juce::OwnedArray<juce::TextButton> buttons;
    juce::ParameterAttachment attachment;
    void refresh (int v);
};

// botão liga/bypass (padrão do kit: lâmpada + texto)
class Bypass : public juce::Component
{
public:
    Bypass (PickGuitarProcessor& p, const juce::String& id, juce::String onText);
    void resized() override;
    void paint (juce::Graphics&) override;
private:
    juce::TextButton button;
    juce::String onLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    void update();
};

class Combo : public juce::Component
{
public:
    Combo (PickGuitarProcessor& p, const juce::String& id, juce::String label);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::ComboBox box;
private:
    juce::String name;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

// grade de 16 semicolcheias (1 e & a), toque para trocar: pausa → abafada → acento → aberta → pica-pau
class GridView : public juce::Component
{
public:
    explicit GridView (PickGuitarProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    PickGuitarProcessor& proc;
    juce::Rectangle<float> cellRect (int i) const;
};

// braço da guitarra: acende a nota tocada (laranja = abafada, verde = aberta)
class NeckView : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void hit (int midi, bool open) { hits.push_back ({ midi, open, juce::Time::getMillisecondCounterHiRes() }); if (hits.size() > 24) hits.pop_front(); }
private:
    struct Hit { int midi; bool open; double t; };
    std::deque<Hit> hits;
};

// mapa do pica-pau: 8 compassos x 16 semicolcheias, editável
class PicaPauMap : public juce::Component
{
public:
    explicit PicaPauMap (PickGuitarProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::String info() const;
private:
    PickGuitarProcessor& proc;
    juce::Rectangle<float> cellRect (int row, int col) const;
};

// EQ gráfico editável com o mouse
class EqGraph : public juce::Component
{
public:
    explicit EqGraph (PickGuitarProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    juce::String readout;
private:
    PickGuitarProcessor& proc;
    int drag = -1, sel = -1;
    float xOf (float f) const;
    float fOf (float x) const;
    float yOf (float db) const;
    float dbOf (float y) const;
    int hitNode (juce::Point<float>) const;
    void setBand (int i, float f, float g, float q, bool gesture);
    void updateReadout (int i);
};

class ScopeView : public juce::Component
{
public:
    explicit ScopeView (PickGuitarProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    PickGuitarProcessor& proc;
};

class PresetLcd : public juce::Component
{
public:
    explicit PresetLcd (PickGuitarProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    PickGuitarProcessor& proc;
};

//==============================================================================
class PickGuitarEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PickGuitarEditor (PickGuitarProcessor&);
    ~PickGuitarEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setPage (int page);
    void refreshForSnapshot() { timerCallback(); }
    static constexpr int kW = 1120, kH = 860;

private:
    void timerCallback() override;
    static int getNumMus();
    void layoutPages();
    juce::Rectangle<int> pageArea() const { return { 26, 330, kW - 26, 394 }; }

    PickGuitarProcessor& proc;
    PickLookAndFeel lnf;
    juce::Image wood;

    // cabeçalho, visor e transporte
    Combo key, scale;
    VUMeter vu { "MPG-11" };
    Knob master;
    juce::TextButton power;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAtt;
    juce::TextButton eraPrev, eraNext, musPrev, musNext;
    PresetLcd lcd;
    juce::TextButton play;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAtt;
    juce::Slider bpm;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAtt;
    juce::TextButton guide;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> guideAtt;
    ScopeView scope;
    juce::OwnedArray<juce::TextButton> tabs;
    int page = 0;

    // LINHA
    juce::Component pLinha;
    Chips mode;
    Combo src, line, oct, shape;
    GridView grid;
    juce::TextButton gClear, gFill, gReset;
    NeckView neck;
    Knob gate, human, swing, acc;
    Chips art, rep;
    juce::Slider dose;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> doseAtt;
    Knob ppSwing;
    PicaPauMap map;
    juce::TextButton ppClear;

    // TIMBRE
    juce::Component pTimbre;
    Chips pickup;
    Knob mute, pos, atk, drive, tone, body;

    // EQUALIZADOR
    juce::Component pEq;
    Bypass eqOn;
    juce::OwnedArray<juce::TextButton> eqPresets;
    EqGraph eq;
    Knob bigKnob;

    // SATURADOR + TAPE ECHO
    juce::Component pFx;
    Bypass satOn;
    Chips satType;
    Knob satDrive, satMix, satOut;
    Bypass echoOn;
    Combo echoSync;
    Knob echoFb, echoMix, spring;
    juce::TextButton throwBtn;

    // GUITARRISTAS
    juce::Component pMus;
    juce::OwnedArray<juce::TextButton> musButtons;

    juce::MidiKeyboardComponent keyboard;
    int lastPickCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PickGuitarEditor)
};
