#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "VintageLookAndFeel.h"
#include "VUMeter.h"

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

// Rodas de pitch e modulação: mostram o MIDI que chega e podem ser arrastadas com o mouse
// (pitch volta ao centro ao soltar, como a roda de verdade; mod fica onde parou)
class WheelsComponent : public juce::Component
{
public:
    WheelsComponent() { setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
    std::function<void (int wheel, float value)> onMove;   // 0 = pitch (-1..1), 1 = mod (0..1)
    void setValues (float pitch, float mod)
    {
        if (dragWheel >= 0) return;   // durante o arraste manda a mão
        if (std::abs (pitch - pitchV) < 0.005f && std::abs (mod - modV) < 0.005f) return;
        pitchV = pitch; modV = mod; repaint();
    }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override
    {
        dragWheel = e.x < 37 ? 0 : 1;
        moveTo (e.y);
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (dragWheel >= 0) moveTo (e.y); }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragWheel == 0)
        {
            pitchV = 0.0f;
            if (onMove) onMove (0, 0.0f);
            repaint();
        }
        dragWheel = -1;
    }
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (e.x >= 37) { modV = 0.0f; if (onMove) onMove (1, 0.0f); repaint(); }
    }
private:
    float pitchV = 0.0f, modV = 0.0f;
    int dragWheel = -1;
    void moveTo (int y)
    {
        const float top = 14.0f, bottom = (float) getHeight() - 34.0f;
        const float v = juce::jlimit (0.0f, 1.0f, (bottom - (float) y) / (bottom - top));
        if (dragWheel == 0) pitchV = v * 2.0f - 1.0f; else modV = v;
        if (onMove) onMove (dragWheel, dragWheel == 0 ? pitchV : modV);
        repaint();
    }
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
    void setMetersForScreenshot (float l, float r) { timerCallback(); vuL.setNeedle (l); vuR.setNeedle (r); grLabel.setText ("GR -1.2", juce::dontSendNotification); updateLcd(); }

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
    int skin = -1, page = 0;
    juce::Image background;
    int backgroundSkin = -1, backgroundPage = -1, backgroundLook = -1;
    // acabamento do gabinete no modo piano, conforme o timbre:
    // 0 teclado preto anos 80 (M1, M1 COMP, M1 GRAND, Honky, FM EP) · 1 Rhodes Stage · 2 Rhodes Suitcase
    // 3 Wurlitzer 200A · 4 Yamaha CP / C7 / Grand · 5 Casio CZ-101 / MT-40
    int look = 0;
    static int lookForTimbre (int pianoType);

    // cabeçalho
    juce::TextButton tabInstrument { "INSTRUMENTO" }, tabChords { "CHORD MACHINE" };
    std::unique_ptr<juce::Component> machinePanel;            // o MRCM-1240 inteiro (Source/MachineEditorUnit.cpp)
    juce::ToggleButton machineOnBtn { "TOCA COM A DAW" };      // parâmetro machineOn
    std::unique_ptr<APVTS::ButtonAttachment> machineOnAtt;

    // piano anos 80
    LcdDisplay lcd;
    juce::TextButton pianoPreset[4];
    static constexpr int kNumTimbres = 13;   // M1 / GRAND / MT-40 / FM EP / CP-80 / C7 / M1 GRAND / M1 COMP / HONKY / CZ-101 / RHODES / SUITCASE / WURLI
    juce::TextButton pianoTimbre[kNumTimbres];
    Knob faderBright, faderDecay, faderChorus, faderLevel;   // faders verticais
    WheelsComponent wheels;


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
    void updateVisibility();
public:
    void setPage (int newPage);   // 0 = instrumento, 1 = CHORD MACHINE
private:
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RootsKeysEditor)
};
