// MRM-0420 2.7: abas ARRANJO, MIXER (com a sirene no Mastor Tape Echo MTE-12) e SHOW (10 riddims salvos).
// Mesmo visual da aba PRINCIPAL: laterais de madeira, painel creme, plaquetas pretas e displays âmbar.
#pragma once

#include "MrmMain.h"
#include "MrmStyle.h"

namespace mrmui
{
// fundo comum das abas novas: madeira, painel e as seções com título
class PageBase : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
protected:
    void renderBackground (std::vector<std::pair<juce::Rectangle<int>, juce::String>> sections);
    juce::Image background;
};

// botão de pressão preto com texto âmbar (o mesmo jeito das teclas da aba PRINCIPAL)
class KeyButton : public juce::Button
{
public:
    explicit KeyButton (const juce::String& t, juce::Colour lit = col::amber) : juce::Button (t), litColour (lit) {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
    bool lit = false;
    float fontSize = 13.0f;
private:
    juce::Colour litColour;
};

//==============================================================================
// ARRANJO
class ArrangeTimeline : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ArrangeTimeline (MrmProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<void()> onChange;
    int litSlot = -1, litBar = 0;
    float pulse = 0.0f;
private:
    struct Block { juce::Rectangle<float> r, left, right, del; };
    std::vector<Block> layout() const;
    MrmProcessor& proc;
    int dragFrom = -1, dragTo = -1;
    juce::Point<float> downPos;
};

class ArrangePage : public PageBase, private juce::Timer
{
public:
    explicit ArrangePage (MrmProcessor&);
    ~ArrangePage() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();
private:
    void timerCallback() override;
    void edit (std::function<void (mrm::Arrangement&)>);
    void exportSong();
    struct Row
    {
        std::unique_ptr<KeyButton> minus, plus, add, del;
        std::array<std::unique_ptr<KeyButton>, MrmProcessor::NumMachines> mach;
        std::unique_ptr<juce::Label> name;
        juce::Rectangle<int> area;
    };
    MrmProcessor& proc;
    ArrangeTimeline timeline;
    KeyButton onKey { "TOCAR O ARRANJO" }, loopKey { "REPETIR NO FIM" }, exportKey { "EXPORTAR WAV + MIDI", col::jamGreen.brighter (0.6f) },
              stdKey { u8 ("ORDEM PADR\xc3\x83O") }, clearKey { "LIMPAR A ORDEM" }, newKey { "+ NOVA PARTE" };
    std::vector<Row> rows;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String status;
    int lastRows = -1;
    juce::Rectangle<int> topSection, orderSection, partsSection, statusArea;
};

//==============================================================================
// MIXER
class ChannelFader : public juce::Component
{
public:
    ChannelFader (juce::RangedAudioParameter&, std::function<float()> level, juce::Colour);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void tick();
private:
    juce::Rectangle<float> track() const;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment att;
    std::function<float()> level;
    juce::Colour colour;
    float value = 0.0f, meter = 0.0f;
};

class MixerPage : public PageBase, private juce::Timer
{
public:
    explicit MixerPage (MrmProcessor&);
    ~MixerPage() override;
    void resized() override;
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    struct Strip
    {
        std::unique_ptr<ChannelFader> fader;
        std::unique_ptr<BigDial> pan;
        std::unique_ptr<MuteKey> mute;
        std::unique_ptr<PowerSwitch> solo;
        juce::Rectangle<int> area;
        int machine = 0;
    };
    MrmProcessor& proc;
    std::array<Strip, MrmProcessor::NumMixer> strips;
    std::unique_ptr<SirenFire> fire;
    std::unique_ptr<PowerSwitch> hold, echoOn;
    std::unique_ptr<MusicianKeys> mode;
    std::array<std::unique_ptr<BigDial>, 7> echoDials;
    juce::Rectangle<int> mixSection, sirenSection;
};

//==============================================================================
// SHOW
class ShowPage : public PageBase, private juce::Timer
{
public:
    explicit ShowPage (MrmProcessor&);
    ~ShowPage() override;
    void resized() override;
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    void askName (int slot, bool rename);
    struct Card
    {
        std::unique_ptr<KeyButton> play, save, rename, del;
        juce::Rectangle<int> area;
    };
    MrmProcessor& proc;
    std::array<Card, MrmProcessor::numShowSlots> cards;
    KeyButton exportKey { "EXPORTAR O SHOW" }, importKey { "IMPORTAR O SHOW" };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> showSection;
    juce::String message;
    int lastStamp = 0;
};
} // namespace mrmui
