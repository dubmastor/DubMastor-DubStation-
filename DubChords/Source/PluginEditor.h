#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Mesmo visual do DubMastor Big Knob: painel creme, madeira, baquelite, lâmpadas e displays âmbar
class DubLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DubLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

//==============================================================================
// Área que você arrasta direto para a timeline da DAW
class MidiDragZone : public juce::Component
{
public:
    explicit MidiDragZone (DubChordsProcessor& p) : proc (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }

private:
    DubChordsProcessor& proc;
    bool dragging = false;
};

//==============================================================================
// Base dos controles clicáveis: liga o componente a um parâmetro (com undo/automação da DAW)
class ParamControl : public juce::Component
{
public:
    explicit ParamControl (juce::RangedAudioParameter& p)
        : param (p), att (p, [this] (float v) { value = v; repaint(); }, nullptr)
    {
        att.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setRepaintsOnMouseActivity (true);
    }
protected:
    int  index() const       { return juce::roundToInt (value); }
    void select (int i)      { att.setValueAsCompleteGesture ((float) i); }
    int  numChoices() const  { return param.getNumSteps(); }

    juce::RangedAudioParameter& param;
    float value = 0.0f;
private:
    juce::ParameterAttachment att;
};

// Tônica: teclado mecânico de uma oitava (teclas de marfim e baquelite)
class NoteKeys : public ParamControl
{
public:
    using ParamControl::ParamControl;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent& e) override { hover = keyAt (e.position); repaint(); }
    void mouseExit (const juce::MouseEvent&) override   { hover = -1; repaint(); }
private:
    juce::Rectangle<float> keyRect (int note) const;
    int keyAt (juce::Point<float>) const;
    int hover = -1;
};

// Maior / menor: dois botões de pressão intertravados com lâmpada
class ModeSwitch : public ParamControl
{
public:
    using ParamControl::ParamControl;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { select (e.position.x < (float) getWidth() * 0.5f ? 0 : 1); }
};

// Duração: janela de contador mecânico; clique à direita avança, à esquerda volta
class DurationWindow : public ParamControl
{
public:
    using ParamControl::ParamControl;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override { side = e.position.x < (float) getWidth() * 0.5f ? -1 : 1; repaint(); }
    void mouseExit (const juce::MouseEvent&) override   { side = 0; repaint(); }
private:
    int side = 0;
};

//==============================================================================
// Painel com todo o layout no tamanho base; o editor o escala para a janela redimensionável
class DubChordsPanel : public juce::Component, private juce::Timer
{
public:
    explicit DubChordsPanel (DubChordsProcessor&);
    ~DubChordsPanel() override;

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
    };
    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<ButtonAttach> att;
    };
    struct ChordModule
    {
        std::unique_ptr<NoteKeys> keys;
        std::unique_ptr<ModeSwitch> mode;
        std::unique_ptr<DurationWindow> dur;
        Toggle on;                       // só nos acordes 2, 3 e 4
        juce::Rectangle<int> area, display;
    };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name);
    void setupToggle (Toggle&, const juce::String& paramId, const juce::String& text);
    void placeKnob (Knob&, juce::Rectangle<int>);
    void renderBackground();
    void drawKnobScale (juce::Graphics&, juce::Slider&, int ticks, const juce::StringArray& labels = {});
    void drawTimeline (juce::Graphics&, juce::Rectangle<float>);
    void exportMidi();
    bool slotEnabled (int i) const;

    DubChordsProcessor& proc;
    DubLookAndFeel lnf;

    std::array<ChordModule, DubChordsProcessor::numSlots> mods;
    Knob pattern, center, velocity, gate, swing;
    Knob bpm, volume, reps;
    Toggle preview, sound, bass;
    juce::TextButton exportBtn { "EXPORTAR" };
    MidiDragZone dragZone;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image background;
    juce::Rectangle<int> woodL, woodR, plate;
    juce::Rectangle<int> chordPanel, rhythmPanel, soundPanel, midiPanel, timeline, patternDisplay;
    std::array<bool, DubChordsProcessor::numSlots> lastOn {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubChordsPanel)
};

//==============================================================================
class DubChordsEditor : public juce::AudioProcessorEditor
{
public:
    explicit DubChordsEditor (DubChordsProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        const int w = panel.getWidth(), h = panel.getHeight();
        setResizable (true, true);
        setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        setSize (w, h);
    }
    void resized() override
    {
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) panel.getWidth()));
    }
private:
    DubChordsPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubChordsEditor)
};
