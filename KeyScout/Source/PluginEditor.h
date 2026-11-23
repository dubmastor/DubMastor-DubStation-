// Interface do Mastor Key Scout MKS-0420 — visual DubStation (painel creme, madeira, plaquetas, lâmpadas)
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace mksui
{
class Panel : public juce::Component, private juce::Timer
{
public:
    explicit Panel (KeyScoutProcessor&);
    ~Panel() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    static constexpr int kW = 880, kH = 560;
private:
    void timerCallback() override;
    void renderBackground();
    void drawKey (juce::Graphics&, juce::Rectangle<float>, const juce::String& text, bool on, bool pressed, bool green = false);
    void toggle (const char* id);

    KeyScoutProcessor& proc;
    KeyScoutProcessor::Status st;
    juce::Image background;
    std::array<float, 12> bars {};
    float anim = 0.0f;
    int pressed = -1;

    juce::Rectangle<int> secKey, secChroma, secLink;
    juce::Rectangle<float> keyGlass, chordGlass, chromaGlass, pianoR, linkGlass, keyListen, keyHold, keyRelearn, infoBox;
};
} // namespace mksui

class KeyScoutEditor : public juce::AudioProcessorEditor
{
public:
    explicit KeyScoutEditor (KeyScoutProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        const int w = mksui::Panel::kW, h = mksui::Panel::kH;
        setResizable (true, true);
        setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        setSize (w, h);
    }
    void resized() override
    {
        panel.setBounds (0, 0, mksui::Panel::kW, mksui::Panel::kH);
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) mksui::Panel::kW));
    }
    mksui::Panel& getPanel() { return panel; }
private:
    mksui::Panel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyScoutEditor)
};
