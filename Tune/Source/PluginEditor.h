// Interface do Mastor Tune MT-0420 — visual DubStation (o mesmo painel creme, madeira, plaquetas e
// lâmpadas do MRM-0420). Duas abas: AFINAÇÃO (harmonia, tom, knob CLEAN/ROBÔ, medidor, editor de notas)
// e PROCESSAMENTO (EQ gráfico, saturador, Big Knob, Tape Echo e Maximizer).
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace mtui
{
// tecla preta com lâmpada (abas, escolhas, liga/desliga)
class LampKey : public juce::Component
{
public:
    LampKey (juce::String t, juce::String s = {}) : text (std::move (t)), sub (std::move (s)) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    std::function<void()> onClick;
    std::function<bool()> isOn;
    void setText (const juce::String& t) { if (t != text) { text = t; repaint(); } }
    float fontSize = 12.5f;
    bool green = false;   // tecla verde grande (ESCUTAR RIDDIM)
    bool pulse = false; float pulsePhase = 0.0f;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { pressed = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override { pressed = false; repaint(); if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
private:
    juce::String text, sub;
    bool pressed = false;
};

// botão cromado com saia preta (parâmetro); arraste para cima/baixo, duplo clique volta ao padrão
class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter& p, juce::String label, float size = 1.0f);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool darkPanel = false;
private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    juce::String label;
    float value = 0.0f, startNorm = 0.0f;
};

// o knob grande CLEAN <-> ROBÔ
class TuneKnob : public juce::Component
{
public:
    explicit TuneKnob (juce::RangedAudioParameter& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    float getValue() const { return value; }
private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    float value = 25.0f, startNorm = 0.0f;
};

// teclado de uma oitava para escolher o tom
class KeyPiano : public juce::Component
{
public:
    explicit KeyPiano (MastorTuneProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    int shown = 9;
private:
    int keyAt (juce::Point<float>) const;
    MastorTuneProcessor& proc;
};

// VU creme: quantos cents a voz está fora da nota
class CentsMeter : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void set (float cents, bool active);
private:
    float needle = 0.0f, vel = 0.0f, target = 0.0f; bool on = false;
};

// teclado de baixo: acende a nota cantada (e a nota corrigida) e apaga as notas fora do tom e da escala
class ScaleKeyboard : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void set (uint16_t allowedMask, float sungMidi, int targetMidi, bool bypassed);
    static constexpr int kLow = 36, kHigh = 83;   // C2 .. B5
private:
    uint16_t mask = 0x0fff;
    float sung = 0.0f; int target = -1; bool byp = false;
    float glow = 0.0f;
};

// editor de notas (preso à timeline da DAW)
class NoteEditor : public juce::Component
{
public:
    explicit NoteEditor (MastorTuneProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void refresh (bool notesChanged);
    float blink = 0.0f;
private:
    struct Choice { juce::Rectangle<float> r; int midi; };
    juce::Rectangle<float> rollArea() const;
    float xOf (double ppq) const;
    float yOf (float midi) const;
    MastorTuneProcessor& proc;
    double viewStart = 0.0, viewBeats = 16.0;
    float lowNote = 48.0f, highNote = 72.0f;
    int selected = -1;                 // id da nota escolhida
    std::vector<Choice> choices;
    juce::Rectangle<float> popup;
    double dragStartView = 0.0;
    bool follow = true;
};

// caixa tracejada "ARRASTE A MELODIA": arrasta o MIDI das notas afinadas para fora do plugin
class MelodyDrag : public juce::Component
{
public:
    explicit MelodyDrag (MastorTuneProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
private:
    MastorTuneProcessor& proc;
    bool dragging = false;
};

// EQ gráfico: 5 pontos (HP, 1, 2, 3, LP) arrastáveis com o mouse
class EqGraph : public juce::Component
{
public:
    explicit EqGraph (MastorTuneProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void applyPreset (int i);
    juce::String readout() const;
    int active = 2;
private:
    struct Node { const char* f; const char* g; };
    mt::EqSettings settings() const;
    float xOfHz (float hz) const;
    float hzOfX (float x) const;
    float yOfDb (float db) const;
    juce::Point<float> nodePos (int i) const;
    void setParam (const char* id, float v);
    MastorTuneProcessor& proc;
    int drag = -1;
};

// animações (fita, válvula, alto-falante, rolos do eco) e medidores do maximizer
class SatView : public juce::Component { public: explicit SatView (MastorTuneProcessor& p) : proc (p) {} void paint (juce::Graphics&) override; float phase = 0; private: MastorTuneProcessor& proc; };
class EchoView : public juce::Component { public: explicit EchoView (MastorTuneProcessor& p) : proc (p) {} void paint (juce::Graphics&) override; float phase = 0; private: MastorTuneProcessor& proc; };
class MaxMeters : public juce::Component
{
public:
    explicit MaxMeters (MastorTuneProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void update();
private:
    MastorTuneProcessor& proc;
    float out[2] { -60, -60 }, gr = 0, inLvl = -60;
};
class BigKnobView : public juce::Component
{
public:
    explicit BigKnobView (juce::RangedAudioParameter& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    float value = 0.3f, startNorm = 0.0f;
};

//==============================================================================
class Panel : public juce::Component, private juce::Timer
{
public:
    explicit Panel (MastorTuneProcessor&);
    ~Panel() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void showTab (int t);
    static constexpr int kW = 1200, kH = 1080;
private:
    void timerCallback() override;
    void renderBackground();
    LampKey& addKey (const juce::String& text, std::function<void()> click, std::function<bool()> on, const juce::String& sub = {});
    LampKey& addToggle (const char* paramId, const juce::String& onText, const juce::String& offText);
    LampKey& addChoice (const char* paramId, int index, const juce::String& text);
    float param (const char* id) const { return proc.getP (id); }
    void setParam (const char* id, float v);

    MastorTuneProcessor& proc;
    int tab = 0;
    juce::Image background;
    std::vector<std::unique_ptr<LampKey>> keys;
    std::vector<juce::Component*> tunePage, procPage;
    std::vector<std::unique_ptr<juce::Component>> owned;

    LampKey *tabTune = nullptr, *tabProc = nullptr, *listenKey = nullptr, *autoKey = nullptr, *reKey = nullptr;
    std::vector<LampKey*> scaleKeys, eqPresetKeys;
    TuneKnob* tuneKnob = nullptr;
    KeyPiano* piano = nullptr;
    CentsMeter* meter = nullptr;
    NoteEditor* noteEditor = nullptr;
    MelodyDrag* melody = nullptr;
    EqGraph* eqGraph = nullptr;
    SatView* satView = nullptr;
    EchoView* echoView = nullptr;
    MaxMeters* maxMeters = nullptr;
    ScaleKeyboard* keyboard = nullptr;
    LampKey* bypassKey = nullptr;
    MastorTuneProcessor::Status st;
    float anim = 0.0f;

    // áreas (coordenadas do painel 1200 x 960)
    juce::Rectangle<int> secTabs, secHarm, secKey, secKnob, secMeter, secNotes;
    juce::Rectangle<int> secEq, secSat, secBk, secEcho, secMax, secKeys;
    juce::Rectangle<float> mrmGlass, riddimGlass, infoBox, keyGlass, knobGlass, noteGlass, eqRead, maxRead, doubtGlass;
};
} // namespace mtui

class MastorTuneEditor : public juce::AudioProcessorEditor
{
public:
    explicit MastorTuneEditor (MastorTuneProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        const int w = mtui::Panel::kW, h = mtui::Panel::kH;
        setResizable (true, true);
        setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        setSize ((int) (w * 0.85f), (int) (h * 0.85f));
    }
    void resized() override
    {
        panel.setBounds (0, 0, mtui::Panel::kW, mtui::Panel::kH);
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) mtui::Panel::kW));
    }
    mtui::Panel& getPanel() { return panel; }
private:
    mtui::Panel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MastorTuneEditor)
};
