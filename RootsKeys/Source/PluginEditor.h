#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "VintageLookAndFeel.h"
#include "VUMeter.h"

// Drawbar estilo Hammond: puxar PARA BAIXO aumenta o volume (0..8)
class DrawbarComponent : public juce::Component
{
public:
    DrawbarComponent (juce::RangedAudioParameter& p, juce::String n, juce::Colour tab)
        : name (std::move (n)), tabColour (tab),
          attachment (p, [this] (float v) { value = juce::roundToInt (v); repaint(); }, nullptr)
    {
        attachment.sendInitialUpdate();
    }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::String name;
    juce::Colour tabColour;
    juce::ParameterAttachment attachment;
    int value = 0, dragStart = 0;
    float step() const { return ((float) getHeight() - 30.0f - 26.0f) / 8.0f; }
};

// Visor de cristal líquido verde (modo piano)
class LcdDisplay : public juce::Component
{
public:
    void setLines (const juce::String& a, const juce::String& b)
    {
        if (a == line1 && b == line2) return;
        line1 = a; line2 = b; repaint();
    }
    void paint (juce::Graphics&) override;
private:
    juce::String line1, line2;
};

// Rodas de pitch e modulação (mostram o MIDI que chega)
class WheelsComponent : public juce::Component
{
public:
    void setValues (float pitch, float mod)
    {
        if (std::abs (pitch - pitchV) < 0.005f && std::abs (mod - modV) < 0.005f) return;
        pitchV = pitch; modV = mod; repaint();
    }
    void paint (juce::Graphics&) override;
private:
    float pitchV = 0.0f, modV = 0.0f;
};

class RootsKeysEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit RootsKeysEditor (RootsKeysProcessor&);
    ~RootsKeysEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // usado pelo gerador de capturas de tela
    void applySkin (int newSkin);
    void setMetersForScreenshot (float l, float r) { vuL.setNeedle (l); vuR.setNeedle (r); grLabel.setText ("GR -1.2", juce::dontSendNotification); updateLcd(); }

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> att;
    };

    RootsKeysProcessor& proc;
    VintageLookAndFeel lnf;
    juce::Typeface::Ptr plateFace, pianoTitleFace, organTitleFace;
    int skin = -1;
    juce::Image background;
    int backgroundSkin = -1;

    // cabeçalho
    juce::TextButton modePiano { "PIANO" }, modeOrgan { "ORGAN" };

    // piano anos 80
    LcdDisplay lcd;
    juce::TextButton pianoPreset[4];
    Knob faderBright, faderDecay, faderChorus, faderLevel;   // faders verticais
    WheelsComponent wheels;

    // órgão
    std::unique_ptr<DrawbarComponent> drawbars[9];
    juce::ToggleButton leslieSwitch { "LESLIE" }, percOn { "ON" }, perc3rd { "3RD" }, percSlow { "SLOW" };
    std::unique_ptr<APVTS::ButtonAttachment> leslieAtt, percOnAtt, perc3rdAtt, percSlowAtt;
    Knob oLeslieMix, oDrive, oClick, oLevel;
    juce::TextButton organPreset[4];

    // eco de fita FitaEco TE-12
    juce::ToggleButton echoButton { "ECHO" };
    std::unique_ptr<APVTS::ButtonAttachment> echoAtt;
    juce::ComboBox echoModeBox;
    std::unique_ptr<APVTS::ComboBoxAttachment> echoModeAtt;
    Knob eRate, eIntensity, eVol, eSpring, eWow, eAge;

    // flanger estilo dub (Mad Professor)
    juce::ToggleButton flButton { "FLANGER" };
    std::unique_ptr<APVTS::ButtonAttachment> flAtt;
    Knob fRate, fDepth, fManual, fFeedback, fMix;

    // maximizer + VU
    juce::ToggleButton maxButton { "MAX" };
    std::unique_ptr<APVTS::ButtonAttachment> maxAtt;
    Knob mxGain, mxCeil, mMaster, mReverb;
    juce::Label grLabel;
    VUMeter vuL { "L" }, vuR { "R" };

    juce::MidiKeyboardComponent keyboard;

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& text, bool fader = false);
    void placeKnob (Knob&, juce::Point<int> centre, int size, int labelW = 80);
    void renderBackground (juce::Graphics&, int forSkin);
    void drawPlate (juce::Graphics&, juce::Rectangle<float> r);
    void updateLcd();
    void setModeParam (int m);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RootsKeysEditor)
};
