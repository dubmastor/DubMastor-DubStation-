#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "VintageLookAndFeel.h"   // o mesmo visual do Mastor Reggae Piano MRP-80 (ReggaePiano/Source)
#include "VUMeter.h"

// Drawbar estilo Hammond (igual à do MRP-80): puxar PARA BAIXO aumenta o volume (0..8)
class OrganDrawbar : public juce::Component
{
public:
    OrganDrawbar (juce::RangedAudioParameter& p, juce::String n, juce::Colour tab)
        : name (std::move (n)), tabColour (tab),
          attachment (p, [this] (float v) { value = juce::roundToInt (v); repaint(); }, nullptr)
    {
        attachment.sendInitialUpdate();
    }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { dragStart = value; attachment.beginGesture(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const int v = juce::jlimit (0, 8, dragStart + juce::roundToInt ((float) e.getDistanceFromDragStartY() / step()));
        if (v != value) attachment.setValueAsPartOfGesture ((float) v);
    }
    void mouseUp (const juce::MouseEvent&) override { attachment.endGesture(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { attachment.setValueAsCompleteGesture (value == 0 ? 8.0f : 0.0f); }
private:
    juce::String name;
    juce::Colour tabColour;
    juce::ParameterAttachment attachment;
    int value = 0, dragStart = 0;
    float step() const { return ((float) getHeight() - 30.0f - 26.0f) / 8.0f; }
};

// Teclado que também acende as notas que o groove está tocando
class GrooveKeyboard : public juce::MidiKeyboardComponent
{
public:
    GrooveKeyboard (juce::MidiKeyboardState& st, ReggaeOrganProcessor& p)
        : juce::MidiKeyboardComponent (st, horizontalKeyboard), proc (p) {}
protected:
    void drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour line, juce::Colour text) override
    {
        juce::MidiKeyboardComponent::drawWhiteNote (note, g, area, isDown || proc.lit[note].load(), isOver, line, text);
    }
    void drawBlackNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour fill) override
    {
        juce::MidiKeyboardComponent::drawBlackNote (note, g, area, isDown || proc.lit[note].load(), isOver, fill);
    }
private:
    ReggaeOrganProcessor& proc;
};

// RIDDIM: visor azul com a música do banco único carregada; clique abre o menu (ESTILOS · RIDDIMS A–Z · MÚSICAS A–Z),
// as setas passam para a anterior / próxima
class RiddimVisor : public juce::Component
{
public:
    explicit RiddimVisor (ReggaeOrganProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    void showMenu();
    void step (int dir);
    ReggaeOrganProcessor& proc;
};

// A GRADE do groove: 16 semicolcheias (1 e & a), mão direita e mão esquerda, com a bateria de referência
// (caixa no 2 e no 4). Clique troca vazio → acento → fantasma → vazio; arraste pinta o mesmo valor.
class GrooveGrid : public juce::Component
{
public:
    explicit GrooveGrid (ReggaeOrganProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
private:
    bool cellAt (juce::Point<float>, bool& rightHand, int& step) const;
    juce::Rectangle<float> cell (int row, int step) const;   // row 0 = bateria, 1 = mão direita, 2 = mão esquerda
    ReggaeOrganProcessor& proc;
    int paintLevel = -1;
    bool paintRight = true;
};

// Visor MIDI editável: o loop inteiro com as três camadas em cores.
// Clique num espaço vazio põe nota (na camada do PINCEL); arraste a nota para mover, a borda direita para esticar;
// duplo clique (ou botão direito) apaga. Grade de 1/16 (Shift = 1/32). Editou: o plugin toca o MIDI editado.
class OrganRoll : public juce::Component
{
public:
    explicit OrganRoll (ReggaeOrganProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::CrosshairCursor); }
    void update();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    int brush = mro::LayerChop;
private:
    void computeGeometry();
    int noteAt (juce::Point<float>, bool& onRightEdge) const;
    double beatAt (float x) const;
    int pitchAt (float y) const;
    juce::Rectangle<float> noteRect (const mro::Event&) const;
    void commit();

    ReggaeOrganProcessor& proc;
    std::vector<mro::Event> events;
    std::vector<mro::Chord> chords;
    int bars = 0;
    double loopBeats = 0.0;
    juce::Rectangle<float> area, header, keysCol;
    int lo = 36, hi = 84;
    float rowH = 4.0f;
    enum class Drag { None, Move, Resize } drag = Drag::None;
    int dragIndex = -1;
    mro::Event dragOrig {};
    juce::Point<float> dragFrom;
    bool dragChanged = false;
};

// Arraste para a DAW (ou clique para salvar em Documentos) o MIDI do loop: tudo ou uma camada
class MidiDragChip : public juce::Component, public juce::SettableTooltipClient
{
public:
    MidiDragChip (ReggaeOrganProcessor& p, int layerToExport, juce::String label)
        : proc (p), layer (layerToExport), text (std::move (label))
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setTooltip (juce::String::fromUTF8 ("Arraste para a timeline da DAW. Clique para salvar em Documentos/Mastor Reggae Organ."));
    }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<void (const juce::String&)> onSaved;
private:
    ReggaeOrganProcessor& proc;
    int layer;
    juce::String text;
    bool dragging = false;
    juce::File makeFile (const juce::File& dir) const;
};

class ReggaeOrganEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ReggaeOrganEditor (ReggaeOrganProcessor&);
    ~ReggaeOrganEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshForSnapshot() { timerCallback(); }
    void setPage (int p);

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> att;
    };
    // grupo de botões que escolhe um valor (de um parâmetro ou de uma ação da tela)
    struct Chips
    {
        juce::String paramId;
        juce::OwnedArray<juce::TextButton> buttons;
        std::function<void (int)> onPick;   // sem paramId
        std::function<int()> current;       // sem paramId
    };

    ReggaeOrganProcessor& proc;
    VintageLookAndFeel lnf;
    juce::Typeface::Ptr plateFace, titleFace;
    juce::Image background;
    int page = 0, backgroundPage = -1;

    // cabeçalho
    juce::ToggleButton grooveBtn { "GROOVE" }, playBtn { "PLAY" };
    std::unique_ptr<APVTS::ButtonAttachment> grooveAtt, playAtt;
    Knob bpm;

    // os três caminhos
    juce::TextButton musBtn[mro::NumMusicians];
    juce::ComboBox epochBox;
    RiddimVisor riddim;
    juce::TextButton jackpot { "JACKPOT" };
    int jackpotFlash = 0;
    Chips tabs;
    juce::Label status;

    // aba GROOVE
    GrooveGrid grid;
    int quickFeel = mro::FeelReggae, quickDensity = mro::DenEA;
    Chips feel, density, beat, nota;
    Knob swing, human, ghost, seed;
    juce::ToggleButton layerBtn[3];
    std::unique_ptr<APVTS::ButtonAttachment> layerAtt[3];
    Knob bubbleLvl, chopLvl;
    Chips fillStyle, fillEvery, chordSrc;

    // aba MIDI
    juce::ComboBox rootBox[ReggaeOrganProcessor::numSlots], durBox[ReggaeOrganProcessor::numSlots];
    std::unique_ptr<APVTS::ComboBoxAttachment> rootAtt[ReggaeOrganProcessor::numSlots], durAtt[ReggaeOrganProcessor::numSlots];
    juce::TextButton majBtn[ReggaeOrganProcessor::numSlots], minBtn[ReggaeOrganProcessor::numSlots];
    juce::ToggleButton slotOn[ReggaeOrganProcessor::numSlots];
    std::unique_ptr<APVTS::ButtonAttachment> slotOnAtt[ReggaeOrganProcessor::numSlots];
    OrganRoll roll;
    Chips brush;
    std::unique_ptr<MidiDragChip> dragAll, dragLayer[3];
    juce::ComboBox repsBox;
    std::unique_ptr<APVTS::ComboBoxAttachment> repsAtt;
    juce::TextButton backToGroove;
    juce::String savedMsg;

    // aba ÓRGÃO
    std::unique_ptr<OrganDrawbar> bubbleDb[9], chopDb[9];
    Chips leslie;
    juce::ToggleButton percBtn { "PERC" }, echoBtn { "ECHO" }, maxBtn { "MAX" };
    std::unique_ptr<APVTS::ButtonAttachment> percAtt, echoAtt, maxAtt;
    Knob drive, click, echoVol, echoInt, echoRate, reverb, master;
    VUMeter vu { "OUT" };

    GrooveKeyboard keyboard;

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& text);
    void placeKnob (Knob&, juce::Point<int> centre, int size, int labelW = 76);
    void setupChips (Chips&, const juce::String& paramId, const juce::StringArray& names);
    void refreshChips (Chips&);
    void layoutChips (Chips&, juce::Rectangle<int> area, int gap = 6);
    std::vector<juce::Component*> pageComponents (int p);
    void renderBackground (juce::Graphics&);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReggaeOrganEditor)
};
