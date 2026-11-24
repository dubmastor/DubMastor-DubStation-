// Mastor Reggae Percussion MRP-77 — interface: aba PADS (24 pads, efeitos e as 6 peças) e aba PERC MACHINE
// (grade de até 8 compassos, uma cor por compasso, MIDI colorido e o esquema de notas sem conflito com o MRD-12).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class PercLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PercLookAndFeel();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

class ReggaePercussionEditor;

// pad (toque: mais perto do topo = mais forte)
class PadComp : public juce::Component
{
public:
    PadComp (ReggaePercussionProcessor& p, int i) : proc (p), index (i) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    float flash = 0;
private:
    ReggaePercussionProcessor& proc; int index;
};

// visor MPC azul do groove
class LcdComp : public juce::Component
{
public:
    explicit LcdComp (ReggaePercussionProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    ReggaePercussionProcessor& proc;
};

// curva do EQ: arraste HPF, BELL e LPF
class EqComp : public juce::Component
{
public:
    explicit EqComp (ReggaePercussionProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
private:
    float xOf (double f) const; double fOf (float x) const;
    ReggaePercussionProcessor& proc; int dragging = -1;
};

// arraste o MIDI (groove inteiro, uma peça ou um compasso) para a timeline da DAW
class DragZone : public juce::Component, public juce::SettableTooltipClient
{
public:
    DragZone (ReggaePercussionProcessor& p, int track, int bar) : proc (p), trackIx (track), barIx (bar) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
    juce::String title, sub; juce::Colour colour { 0xfff2a83a }; bool big = false;
private:
    ReggaePercussionProcessor& proc; int trackIx, barIx; bool dragging = false;
};

// grade da PERC MACHINE (um compasso por vez, na cor dele)
class GridComp : public juce::Component
{
public:
    GridComp (ReggaePercussionProcessor& p, ReggaePercussionEditor& e) : proc (p), ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int nameW = 236, headH = 26;
private:
    juce::Rectangle<float> cell (int row, int step) const;
    ReggaePercussionProcessor& proc; ReggaePercussionEditor& ed;
};

// canal de uma peça (aba PADS): pisca no ritmo em que a peça toca
class StripComp : public juce::Component
{
public:
    StripComp (ReggaePercussionProcessor&, int track);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();
    float flash = 0;
private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    ReggaePercussionProcessor& proc; int t;
    juce::TextButton mute, prev { "<" }, swap, next { ">" }, load { "SAMPLE" }, unload { "X" };
    juce::Slider dec, vol, pan;
    std::unique_ptr<SA> decA, volA, panA; std::unique_ptr<BA> muteA;
    DragZone drag;
    std::unique_ptr<juce::FileChooser> chooser;
    int shownSnd = -1;
};

class ReggaePercussionEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ReggaePercussionEditor (ReggaePercussionProcessor&);
    ~ReggaePercussionEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    int editBar = 0; bool editAll = true, follow = true;
    float rowFlash[mrp::numTracks] {};
    void setPage (int p);
    static juce::Colour barColour (int b);
    static void swapMenu (ReggaePercussionProcessor&, int track, juce::Component* target);

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    void timerCallback() override;
    void knob (juce::Slider&, const juce::String& id, std::unique_ptr<SA>&);
    void toggle (juce::TextButton&, const juce::String& text, const juce::String& id, std::unique_ptr<BA>&, const juce::String& kind = "key");
    void button (juce::TextButton&, const juce::String& text, std::function<void()> click, const juce::String& kind = "key");
    void exportMidi();
    void refreshBars();
    void refreshNotes();
    std::vector<juce::Component*> page0(), page1();

    ReggaePercussionProcessor& proc;
    PercLookAndFeel lnf;
    juce::TooltipWindow tips { this, 600 };
    int page = 0;

    // cabeçalho / transporte
    juce::TextButton playBtn, dawBtn, prevBtn { "<" }, nextBtn { ">" }, tabPads, tabMach, verBtn;
    std::unique_ptr<BA> playA, dawA;
    juce::ComboBox grooveBox;
    LcdComp lcd;

    // PADS
    std::vector<std::unique_ptr<PadComp>> padComps;
    EqComp eq;
    juce::ComboBox eqPreset;
    juce::TextButton eqBtn, satBtn, echoBtn;
    std::unique_ptr<BA> eqA, satA, echoA;
    juce::Slider big, drive, echoFb, echoMix;
    std::unique_ptr<SA> bigA, driveA, echoFbA, echoMixA;
    juce::ComboBox echoTime; std::unique_ptr<CA> echoTimeA;
    std::vector<std::unique_ptr<StripComp>> strips;

    // PERC MACHINE
    DragZone dragAll;
    std::vector<std::unique_ptr<DragZone>> chips;
    juce::TextButton exportBtn, colorBtn, swingMidiBtn, guideBtn, clearBtn, barsDown { "<" }, barsUp { ">" }, editAllBtn, followBtn, resetNotesBtn;
    std::unique_ptr<BA> colorA, swingMidiA, guideA;
    juce::ComboBox chBox;
    juce::Slider swingK, humanK, bpmK;
    std::unique_ptr<SA> swingA, humanA, bpmA;
    juce::TextButton barSel[8];
    juce::TextButton muteKey[mrp::numTracks];
    std::unique_ptr<BA> muteKeyA[mrp::numTracks];
    juce::TextButton rowPrev[mrp::numTracks], rowSwap[mrp::numTracks], rowNext[mrp::numTracks], rowM[mrp::numTracks], rowS[mrp::numTracks];
    std::unique_ptr<BA> rowMA[mrp::numTracks], rowSA[mrp::numTracks];
    GridComp grid;
    juce::Label noteLbl[mrp::numPads];
    juce::Slider noteSl[mrp::numPads];
    std::unique_ptr<juce::FileChooser> chooser;

    uint32_t lastPad[mrp::numPads] {}, lastTrack[mrp::numTracks] {};
    float keyFlash[mrp::numTracks] {};
    int shownBars = -1, shownStep = -2, shownGroove = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReggaePercussionEditor)
};
