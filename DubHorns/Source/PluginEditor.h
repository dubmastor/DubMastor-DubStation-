#pragma once
#include <functional>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace hc // cores do Dub Station
{
    const juce::Colour chassis  { 0xff2b1f16 }, chassisDark { 0xff1a120c }, plate { 0xff3a2a1e }, plate2 { 0xff2e2117 };
    const juce::Colour cream    { 0xffefe2c4 }, muted { 0xffb9a684 }, line { 0xff5a4430 }, ink { 0xff1b130d };
    const juce::Colour amber    { 0xfff2a541 }, gold { 0xffe6b83a }, red { 0xffc8402f }, green { 0xff4f9a5a };
    const juce::Colour echoPlate { 0xff56644a };
}

class HornsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HornsLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

//==============================================================================
// controles ligados a parâmetros
struct KnobCtl : public juce::Component
{
    KnobCtl (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name);
    void resized() override;
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

struct ParamToggle : public juce::TextButton
{
    ParamToggle (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& offText, const juce::String& onText = {});
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    juce::String off, on;
    void buttonStateChanged() override;
    void updateText();
};

struct ParamCombo : public juce::ComboBox
{
    ParamCombo (juce::AudioProcessorValueTreeState&, const juce::String& id);
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

// botão "segure" ligado a um parâmetro bool (throw, auto-oscilar)
struct HoldButton : public juce::TextButton
{
    HoldButton (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& text);
    void buttonStateChanged() override;
    juce::RangedAudioParameter* param = nullptr;
    bool last = false;
};

void setParamValue (juce::AudioProcessorValueTreeState&, const juce::String& id, float plain);

//==============================================================================
class HornsPage : public juce::Component
{
public:
    explicit HornsPage (HornsProcessor& p) : proc (p) {}
    virtual void tick() {}
protected:
    HornsProcessor& proc;
};

class NaipePage;
class TimbrePage;
class EchoPage;
class EqPage;
class PhrasePage;
class MusicosPage;

//==============================================================================
class VuMeter : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void setLevel (float rms);
private:
    float needle = -30.0f;
};

class ChangelogView : public juce::Component
{
public:
    ChangelogView();
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override { setVisible (false); }
    juce::TextButton close { "FECHAR" };
};

//==============================================================================
class HornsPanel : public juce::Component, private juce::Timer
{
public:
    static constexpr int W = 1200, H = 800;
    explicit HornsPanel (HornsProcessor&);
    ~HornsPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void showPage (int);

private:
    void timerCallback() override;

    HornsProcessor& proc;
    HornsLookAndFeel lnf;
    ParamCombo keyBox, scaleBox;
    std::unique_ptr<KnobCtl> master;
    VuMeter vu;
    juce::TextButton versionBtn;
    std::array<juce::TextButton, 6> tabs;
    std::vector<std::unique_ptr<HornsPage>> pages;
    juce::MidiKeyboardComponent keyboard;
    ChangelogView changelog;
    int page = 0;
    juce::Rectangle<int> header, tabBar, pageArea, kbBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HornsPanel)
};

class HornsEditor : public juce::AudioProcessorEditor
{
public:
    explicit HornsEditor (HornsProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        setResizable (true, true);
        setResizeLimits (HornsPanel::W / 2, HornsPanel::H / 2, HornsPanel::W * 3 / 2, HornsPanel::H * 3 / 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) HornsPanel::W / (double) HornsPanel::H);
        setSize (HornsPanel::W, HornsPanel::H);
    }
    void resized() override
    {
        panel.setBounds (0, 0, HornsPanel::W, HornsPanel::H);
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) HornsPanel::W));
    }
    HornsPanel panel;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HornsEditor)
};
