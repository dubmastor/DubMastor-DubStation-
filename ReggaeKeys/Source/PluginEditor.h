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

// botão GTR SKANK: o LED do teclado e, no lugar do texto, uma Tele desenhada
// (corpo de um corte só, escudo preto, placa da ponte cromada, braço de maple e mão com as tarraxas de um lado)
class GuitarIconToggle : public juce::ToggleButton
{
public:
    GuitarIconToggle() : juce::ToggleButton ("") {}

    // coordenadas 0..1, guitarra em pé (mão em cima)
    static juce::Path teleBody()
    {
        juce::Path p;
        p.startNewSubPath (0.445f, 0.50f);
        p.lineTo (0.31f, 0.50f);
        p.quadraticTo (0.20f, 0.51f, 0.20f, 0.62f);       // ombro do lado grave
        p.quadraticTo (0.21f, 0.70f, 0.19f, 0.80f);
        p.quadraticTo (0.20f, 0.97f, 0.50f, 0.975f);      // fundo
        p.quadraticTo (0.80f, 0.97f, 0.80f, 0.82f);
        p.quadraticTo (0.79f, 0.72f, 0.77f, 0.66f);
        p.quadraticTo (0.72f, 0.60f, 0.62f, 0.62f);       // o corte (cutaway) do lado agudo
        p.quadraticTo (0.57f, 0.60f, 0.555f, 0.50f);
        p.closeSubPath();
        return p;
    }
    static juce::Path teleNeck()
    {
        juce::Path p;
        p.addRectangle (0.458f, 0.10f, 0.084f, 0.43f);
        p.startNewSubPath (0.458f, 0.115f);               // mão com as 6 tarraxas de um lado
        p.lineTo (0.44f, 0.035f);
        p.quadraticTo (0.45f, 0.0f, 0.50f, 0.005f);
        p.lineTo (0.60f, 0.03f);
        p.quadraticTo (0.625f, 0.05f, 0.60f, 0.075f);
        p.lineTo (0.542f, 0.115f);
        p.closeSubPath();
        p.setUsingNonZeroWinding (true);
        return p;
    }
    static juce::Path telePickguard()
    {
        juce::Path p;
        p.startNewSubPath (0.44f, 0.535f);
        p.lineTo (0.33f, 0.54f);
        p.quadraticTo (0.27f, 0.57f, 0.28f, 0.64f);
        p.quadraticTo (0.30f, 0.71f, 0.39f, 0.72f);
        p.lineTo (0.45f, 0.69f);
        p.lineTo (0.45f, 0.60f);
        p.closeSubPath();
        return p;
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        getLookAndFeel().drawToggleButton (g, *this, highlighted, down);
        auto box = getLocalBounds().toFloat().reduced (1.0f).withTrimmedLeft (14.0f);
        const float sz = juce::jmin (box.getHeight(), box.getWidth()) * 1.02f;
        const auto area = juce::Rectangle<float> (sz, sz).withCentre (box.getCentre());
        const auto tf = juce::AffineTransform::rotation (juce::degreesToRadians (40.0f), 0.5f, 0.5f)
                            .scaled (sz, sz).translated (area.getX(), area.getY());
        const bool on = getToggleState();
        const juce::Colour body   = on ? juce::Colour (0xffe9c27a) : juce::Colour (0xff9a9a9f);   // butterscotch
        const juce::Colour maple  = on ? juce::Colour (0xfff3dfab) : juce::Colour (0xffb9b9be);
        const juce::Colour guard  = juce::Colour (0xff141416);
        const juce::Colour chrome = on ? juce::Colour (0xffe4e6e9) : juce::Colour (0xffc4c5c8);

        g.setColour (body);    g.fillPath (teleBody(), tf);
        g.setColour (guard);   g.fillPath (telePickguard(), tf);
        g.setColour (maple);   g.fillPath (teleNeck(), tf);
        g.setColour (chrome);
        {
            juce::Path plate; plate.addRoundedRectangle (0.43f, 0.80f, 0.17f, 0.11f, 0.015f);          // placa da ponte
            juce::Path ctrl;  ctrl.addRoundedRectangle (0.66f, 0.74f, 0.07f, 0.17f, 0.02f);            // placa dos controles
            juce::Path neckPu; neckPu.addRoundedRectangle (0.455f, 0.635f, 0.09f, 0.04f, 0.01f);      // captador do braço
            g.fillPath (plate, tf); g.fillPath (ctrl, tf); g.fillPath (neckPu, tf);
        }
        g.setColour (juce::Colour (0xff141416));
        {
            juce::Path bridgePu; bridgePu.addRoundedRectangle (0.465f, 0.81f, 0.07f, 0.035f, 0.008f); // captador da ponte
            g.fillPath (bridgePu, tf);
        }
        g.setColour (juce::Colours::black.withAlpha (0.55f));                                          // contorno fino
        g.strokePath (teleBody(), juce::PathStrokeType (0.012f), tf);
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
    juce::TextButton modePiano { "PIANO" }, modeOrgan { "ORGAN" };
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

    // órgão
    std::unique_ptr<DrawbarComponent> drawbars[9];
    juce::ToggleButton leslieSwitch { "LESLIE" }, percOn { "ON" }, perc3rd { "3RD" }, percSlow { "SLOW" };
    std::unique_ptr<APVTS::ButtonAttachment> leslieAtt, percOnAtt, perc3rdAtt, percSlowAtt;
    Knob oLeslieMix, oDrive, oClick, oLevel;
    juce::TextButton organPreset[4];

    // eco de fita FitaEco TE-12
    juce::ToggleButton echoButton { "ECHO" };
    std::unique_ptr<APVTS::ButtonAttachment> echoAtt;
    GuitarIconToggle gtrButton;   // GTR SKANK (LED + ícone de Tele) (guitarra do skank junto com o piano)
    std::unique_ptr<APVTS::ButtonAttachment> gtrAtt;
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
    void updateVisibility();
public:
    void setPage (int newPage);   // 0 = instrumento, 1 = CHORD MACHINE
private:
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RootsKeysEditor)
};
