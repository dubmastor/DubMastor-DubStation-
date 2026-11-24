#pragma once

#include "PluginProcessor.h"
#include "VintageLookAndFeel.h"

// a grade da virada: 11 peças x 24 passos (16 da virada + a chegada + 7 do groove), na contagem da Dub Station
class FillGrid : public juce::Component
{
public:
    explicit FillGrid (IntroFillsProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int labelW = 92, headerH = 34, cols = 24;
private:
    juce::Rectangle<float> cell (int row, int col) const;
    IntroFillsProcessor& proc;
};

// arraste para a DAW (ou clique para salvar em Documentos/Mastor Intro Fills)
class FillMidiChip : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit FillMidiChip (IntroFillsProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<void (const juce::String&)> onSaved;
private:
    juce::File makeFile (const juce::File& dir) const;
    IntroFillsProcessor& proc;
    bool dragging = false;
};

class IntroFillsEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit IntroFillsEditor (IntroFillsProcessor&);
    ~IntroFillsEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshForSnapshot() { timerCallback(); }

    static constexpr int kW = 1000, kH = 690;

private:
    void timerCallback() override;
    void refresh();
    void renderBackground (juce::Graphics&);

    struct Knob
    {
        juce::Slider s;
        juce::String label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    void setupKnob (Knob& k, const char* id, const juce::String& label);
    void setupChip (juce::TextButton& b, const juce::String& text);
    void setupToggle (juce::TextButton& b, const juce::String& text, const char* id,
                      std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& att);

    IntroFillsProcessor& proc;
    VintageLookAndFeel lnf;
    juce::Typeface::Ptr plateFace, titleFace;
    juce::Image background;

    juce::TextButton drummerKeys[8];
    juce::TextButton prevFill { "<" }, nextFill { ">" }, resetBtn { "VOLTAR AO ORIGINAL" };
    juce::TextButton styleChips[mif::NumStyles], sizeChips[mif::NumSizes], landChips[2];
    juce::TextButton playBtn, loopBtn, echoBtn, satBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAtt, loopAtt, echoAtt, satAtt;
    Knob kInt, kHum, kSwing, kGhost, kVol, kBpm;
    FillGrid grid;
    FillMidiChip midiChip;
    juce::String status;
    int lastDrummer = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IntroFillsEditor)
};
