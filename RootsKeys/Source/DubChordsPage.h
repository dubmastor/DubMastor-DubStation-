// DubChordsPage.h — aba DUB CHORDS do DubMastor Roots Keys
// Monte a progressão, ouça no próprio piano/órgão (com eco e flanger) e arraste o MIDI para a DAW.
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "VintageLookAndFeel.h"

namespace dcui
{
struct Theme
{
    bool piano = true;
    juce::Colour panelFill, panelLine, ink, mutedInk, accent, accentInk, gridBg, gridLine, noteFill, bassFill, lcdBg, lcdInk;
    static Theme make (int skin);
};

// Área que se arrasta para a DAW: escreve um .mid temporário e entrega o arquivo ao host
class MidiDragChip : public juce::Component, public juce::SettableTooltipClient
{
public:
    MidiDragChip (RootsKeysProcessor& p, int partIn, int slotIn, juce::String text, juce::String fileStem)
        : proc (p), part (partIn), slot (slotIn), label (std::move (text)), stem (std::move (fileStem))
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setTooltip ("Clique e arraste para a timeline da DAW");
    }
    void setTheme (const Theme& t) { theme = t; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { dragging = false; }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }
private:
    RootsKeysProcessor& proc;
    int part, slot;
    juce::String label, stem;
    Theme theme;
    bool dragging = false;
};

// Grade (piano roll) do loop: mostra as vozes, o baixo guia, os acordes e o cursor da DAW.
// A grade inteira também é arrastável.
class PianoRollGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit PianoRollGrid (RootsKeysProcessor& p) : proc (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setTooltip ("Arraste a grade para a timeline da DAW (skank + baixo guia)");
    }
    void setTheme (const Theme& t) { theme = t; repaint(); }
    void refresh();                       // chamado ~30x/s
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { dragging = false; }
    void mouseDrag (const juce::MouseEvent&) override;
private:
    RootsKeysProcessor& proc;
    Theme theme;
    std::vector<dub::NoteEvent> events;
    std::vector<dub::ChordSlot> slots;
    double loopBeats = 0.0, playPos = -1.0;
    juce::String key;
    bool dragging = false;
};

class ChordCard : public juce::Component
{
public:
    ChordCard (RootsKeysProcessor& p, int index);
    ~ChordCard() override;
    void setTheme (const Theme& t);
    void refresh();
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
private:
    using APVTS = juce::AudioProcessorValueTreeState;
    RootsKeysProcessor& proc;
    int index;
    Theme theme;
    juce::TextButton notes[12];
    std::unique_ptr<juce::ParameterAttachment> rootAtt;
    juce::ComboBox qualBox, durBox;
    std::unique_ptr<APVTS::ComboBoxAttachment> qualAtt, durAtt;
    juce::ToggleButton onButton { "ON" };
    std::unique_ptr<APVTS::ButtonAttachment> onAtt;
    MidiDragChip drag;
    int root = 0;
    juce::String shownLabel;
    bool shownOn = true;
};
} // namespace dcui

class DubChordsPage : public juce::Component
{
public:
    DubChordsPage (RootsKeysProcessor&, VintageLookAndFeel&);
    ~DubChordsPage() override;
    void setSkin (int skin);
    void refresh();                       // chamado pelo timer do editor
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> att;
    };

    RootsKeysProcessor& proc;
    VintageLookAndFeel& lnf;
    dcui::Theme theme;

    juce::ComboBox riddimBox;
    juce::ToggleButton previewBtn { "PREVIEW" }, dawBtn { "TOCAR COM A DAW" }, bassBtn { "BAIXO GUIA" };
    std::unique_ptr<APVTS::ButtonAttachment> previewAtt, dawAtt, bassAtt;
    Knob bpm, reg, vel, gate, swing;
    juce::ComboBox patternBox, repsBox;
    std::unique_ptr<APVTS::ComboBoxAttachment> patternAtt, repsAtt;
    juce::TextButton saveBtn { "SALVAR .MID" };
    std::unique_ptr<juce::FileChooser> chooser;

    std::unique_ptr<dcui::ChordCard> cards[RootsKeysProcessor::dcSlots];
    dcui::MidiDragChip dragAll, dragSkank, dragBass;
    dcui::PianoRollGrid grid;
    juce::String statusText;

    void setupKnob (Knob&, const juce::String& id, const juce::String& text);
    void placeKnob (Knob&, juce::Point<int> c, int size);
    void saveMidi();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubChordsPage)
};
