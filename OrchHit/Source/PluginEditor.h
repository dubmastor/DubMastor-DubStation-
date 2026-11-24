// Mastor Orch Hit MOH-88 — interface: aba SOM (timbres, envelope, efeitos) e aba MIDIs E GRADE (grooves, MIDIs, grade),
// com o TAMANHO DA NOTA e o teclado sempre embaixo.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class HitLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HitLookAndFeel();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

// visor LCD com o nome do timbre e a forma de onda do sample
class HitLcd : public juce::Component
{
public:
    explicit HitLcd (OrchHitProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    juce::String note = "--";
private:
    OrchHitProcessor& proc;
};

// envelope: arraste os pontos A, D/S e R
class EnvelopeEditor : public juce::Component
{
public:
    explicit EnvelopeEditor (OrchHitProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    struct Geo { float pad, x0, wA, wD, wS, wR, xA, xD, xS, xR, top, h; float y (float l) const { return top + (1 - l) * h; } };
    Geo geo() const;
    void gesture (bool begin);
    OrchHitProcessor& proc;
    int drag = -1;   // 0 A, 1 D/S, 2 R
};

// miniatura de um groove ou de um MIDI
class Thumb : public juce::Component
{
public:
    std::vector<moh::GridNote> notes; int ticks = moh::kTotalTicks; bool selected = false, relative = true;
    juce::String title, info;
    std::function<void()> onClick, onDouble, onRight;
    std::function<void (const juce::MouseEvent&)> onDrag;
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { if (onDouble) onDouble(); }
    void mouseDrag (const juce::MouseEvent& e) override { if (onDrag && e.getDistanceFromDragStart() > 6) { dragged = true; onDrag (e); } }
    void mouseDown (const juce::MouseEvent&) override { dragged = false; }
private:
    bool dragged = false;
};

// a grade de 4 compassos (piano roll): clique no vazio cria, arraste move, puxe a borda direita para o tamanho
class GridEditor : public juce::Component, public juce::DragAndDropTarget
{
public:
    explicit GridEditor (OrchHitProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool isInterestedInDragSource (const SourceDetails&) override { return true; }
    void itemDragMove (const SourceDetails& d) override { hover = d.localPosition.toFloat(); repaint(); }
    void itemDragExit (const SourceDetails&) override { hover = { -1, -1 }; repaint(); }
    void itemDropped (const SourceDetails&) override;
    void undo();
    void commit (const moh::Grid& g);
    bool erase = false; int snap = 4;
    std::function<void (const juce::String&, int, int)> onDrop;   // (MIDI arrastado, tick do compasso, nota base)
    static constexpr int lo = 29, hi = 62;
private:
    juce::Rectangle<float> area() const { return getLocalBounds().toFloat().withTrimmedLeft (38.0f).reduced (0, 1); }
    float xOf (double tick) const { auto a = area(); return a.getX() + (float) (tick / moh::kTotalTicks) * a.getWidth(); }
    float rowH() const { return area().getHeight() / (float) (hi - lo + 1); }
    int noteAt (juce::Point<float>) const;
    double tickAt (float x) const { auto a = area(); return (x - a.getX()) / a.getWidth() * moh::kTotalTicks; }
    int pitchAt (float y) const { return hi - (int) std::floor ((y - area().getY()) / rowH()); }
    OrchHitProcessor& proc;
    std::vector<moh::Grid> history;
    moh::Grid work;
    int dragIdx = -1; bool resizing = false, moved = false; moh::GridNote start {};
    juce::Point<float> hover { -1, -1 };
};

// arraste o MIDI da grade para a timeline da DAW
class MidiDragZone : public juce::Component
{
public:
    explicit MidiDragZone (OrchHitProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
private:
    OrchHitProcessor& proc;
    bool dragging = false;
};

class OrchHitEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit OrchHitEditor (OrchHitProcessor&);
    ~OrchHitEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void setPage (int p);

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    struct Knob { juce::Slider s; juce::String label; std::unique_ptr<SA> att; };
    void timerCallback() override;
    void knob (Knob&, const juce::String& id, const juce::String& label);
    void chip (juce::TextButton&, const juce::String& text, std::function<void()> click, const juce::String& kind = {});
    void rebuildClips();
    void placeClipAt (const std::vector<moh::GridNote>& notes, int t0, int root);

    OrchHitProcessor& proc;
    HitLookAndFeel lnf;
    int page = 0;

    juce::TextButton tabSom, tabMidi;
    HitLcd lcd;
    // SOM
    juce::TextButton timbreBtn[7], envPreset[3];
    EnvelopeEditor env;
    Knob vol, rev, hand;
    juce::TextButton handsBtn, echoBtn, satBtn;
    std::unique_ptr<BA> handsAtt, echoAtt, satAtt;
    juce::Rectangle<int> timbreArea, envArea, fxArea;
    // MIDIs E GRADE
    Thumb grooveThumb[5];
    juce::OwnedArray<Thumb> clipThumbs;
    juce::TextButton previewBtn, dawBtn, drumsBtn, drawBtn, eraseBtn, undoBtn, clearBtn, saveBtn, snapBtn[3];
    std::unique_ptr<BA> previewAtt, dawAtt, drumsAtt;
    Knob bpm;
    GridEditor gridEd;
    MidiDragZone dragZone;
    juce::Rectangle<int> grooveArea, clipArea, gridArea;
    // sempre embaixo
    juce::TextButton lenBtn[moh::kNumLengths];
    juce::MidiKeyboardComponent keyboard;
    int lastTimbre = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchHitEditor)
};
