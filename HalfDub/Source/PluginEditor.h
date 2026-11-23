#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace hdui
{
    juce::Font title (float h);    // Archivo Black (plaquetas)
    juce::Font script (float h);   // Kaushan Script (assinatura DubMastor)
    juce::Font bold (float h);
    juce::Font mono (float h, bool b = false);
    extern const juce::Colour cream, amber, ink, chipText, phosphor;
    // botão estilo "chip" do layout: escuro com aro prateado, âmbar quando ligado
    void drawChip (juce::Graphics&, juce::Rectangle<float>, const juce::String& text, bool on, bool over, float fontH = 13.0f, bool lamp = false);
}

// --------------------------------------------------------------------------
class HalfDubLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HalfDubLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
private:
    void drawBigKnob (juce::Graphics&, juce::Rectangle<float>, float pos, float a0, float a1, juce::Slider&);
};

// Fileira de opções de um parâmetro (escolha ou liga/desliga): clique para escolher
class ChoiceChips : public juce::Component
{
public:
    ChoiceChips (juce::RangedAudioParameter&, juce::StringArray labels, int columns, float fontH = 13.0f);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    std::function<juce::Colour (int)> accent;   // cor opcional do item escolhido
private:
    juce::Rectangle<float> cell (int i) const;
    int indexAt (juce::Point<float>) const;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    juce::StringArray labels;
    int cols, value = 0, hover = -1;
    float fontH;
};

// ◂ valor ▸ para o tamanho do loop
class LoopStepper : public juce::Component
{
public:
    explicit LoopStepper (juce::RangedAudioParameter&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    void step (int d);
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    int value = 4;
};

// Botão grande DROP (vermelho): segurar ou travar
class DropButton : public juce::Component
{
public:
    explicit DropButton (HalfDubProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void update();
private:
    void setDrop (bool on);
    HalfDubProcessor& proc;
    juce::RangedAudioParameter& drop;
    juce::RangedAudioParameter& latch;
    bool lit = false, pressed = false;
};

// Pad do SOUND SYSTEM: liga só enquanto está pressionado (ou pela nota MIDI)
class PadButton : public juce::Component
{
public:
    PadButton (HalfDubProcessor&, const juce::String& paramId, int index, const juce::String& name, const juce::String& note);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { set (true); }
    void mouseUp (const juce::MouseEvent&) override { set (false); }
    void update();
private:
    void set (bool on);
    HalfDubProcessor& proc;
    juce::RangedAudioParameter& param;
    int index;
    juce::String name, note;
    bool hot = false;
};

// Visor de cima: mapa da fita com uma faixa por banda
class TimeMap : public juce::Component
{
public:
    explicit TimeMap (HalfDubProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    void setStatus (const juce::String& s) { if (s != status) { status = s; } }
private:
    HalfDubProcessor& proc;
    juce::String status;
};

// VU digital de LEDs da saída (L/R)
class OutMeter : public juce::Component
{
public:
    explicit OutMeter (HalfDubProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    void update();
private:
    HalfDubProcessor& proc;
    float lvl[2] { -60, -60 }, hold[2] { -60, -60 };
    int holdT[2] { 0, 0 };
};

// EQ gráfico editável com o mouse
class EqGraph : public juce::Component
{
public:
    explicit EqGraph (HalfDubProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    hd::EqSettings current() const;
    int getHover() const { return hover >= 0 ? hover : drag; }
    juce::String describe (int node) const;
private:
    juce::Rectangle<float> plot() const;
    float xOf (float f) const;
    float fOf (float x) const;
    float yOf (float d) const;
    float dOf (float y) const;
    juce::Point<float> nodePos (int i, const hd::EqSettings&) const;
    int nodeAt (juce::Point<float>) const;
    juce::RangedAudioParameter* p (const juce::String& id) const { return proc.apvts.getParameter (id); }
    void setP (const juce::String& id, float v) const;
    HalfDubProcessor& proc;
    int drag = -1, hover = -1;
};

// Presets do EQ (FLAT, HPF, LPF, HPF+LPF, DUB, RÁDIO)
class EqPresets : public juce::Component
{
public:
    explicit EqPresets (HalfDubProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void update();
    static constexpr int kNum = 6;
private:
    juce::Rectangle<float> cell (int i) const;
    int matching() const;
    void apply (int i);
    HalfDubProcessor& proc;
    int sel = -1, hover = -1;
};

// --------------------------------------------------------------------------
class HalfDubPanel : public juce::Component, private juce::Timer
{
public:
    explicit HalfDubPanel (HalfDubProcessor&);
    ~HalfDubPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kW = 1710, kH = 1470;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::String name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };

    void timerCallback() override;
    Knob& knob (const juce::String& id, const juce::String& name);
    Toggle& toggle (const juce::String& id, const juce::String& text);
    void placeKnob (Knob&, float cx, float cy, float metalR);
    void renderBackground();
    void drawVuFace (juce::Graphics&);
    void drawVuNeedle (juce::Graphics&);
    juce::String statusText();
    juce::String bigKnobText() const;

    HalfDubProcessor& proc;
    HalfDubLookAndFeel lnf;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Toggle>> toggles;
    std::vector<std::unique_ptr<juce::Component>> others;
    std::map<juce::String, Knob*> knobById;

    TimeMap timeMap;
    OutMeter meter;
    EqGraph eqGraph;
    EqPresets eqPresets;
    DropButton dropBtn;
    std::vector<PadButton*> pads;

    juce::Image background;
    juce::Rectangle<int> vuArea { 612, 24, 262, 272 };
    juce::Point<float> lampPos { 517.0f, 183.0f };
    float needlePos = 0, needleVel = 0, lampGlow = 0;
    int peakHold = 0;
    juce::String lastBk, lastStatus;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HalfDubPanel)
};

class HalfDubEditor : public juce::AudioProcessorEditor
{
public:
    explicit HalfDubEditor (HalfDubProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        const int w = HalfDubPanel::kW, h = HalfDubPanel::kH;
        setResizable (true, true);
        setResizeLimits (w * 2 / 5, h * 2 / 5, w, h);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        setSize (w * 3 / 5, h * 3 / 5);
    }
    void resized() override
    {
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) HalfDubPanel::kW));
    }
    HalfDubPanel panel;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HalfDubEditor)
};
