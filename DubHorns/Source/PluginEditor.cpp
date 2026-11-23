#include "PluginEditor.h"
#include "Musicians.h"

using namespace juce;

static Font uiFont (float size, bool bold = false, float kerning = 0.0f)
{
    return Font (FontOptions (size, bold ? Font::bold : Font::plain)).withExtraKerningFactor (kerning);
}

void setParamValue (AudioProcessorValueTreeState& s, const String& id, float plain)
{
    if (auto* p = s.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plain));
        p->endChangeGesture();
    }
}

static String upper (const String& s)
{
    auto u = s.toUpperCase();
    const char* pairs[][2] = { { "ç", "Ç" }, { "ã", "Ã" }, { "á", "Á" }, { "é", "É" }, { "ê", "Ê" }, { "í", "Í" },
                               { "ó", "Ó" }, { "ô", "Ô" }, { "ú", "Ú" }, { "â", "Â" }, { "õ", "Õ" } };
    for (auto& p : pairs) u = u.replace (String::fromUTF8 (p[0]), String::fromUTF8 (p[1]));
    return u;
}

static void groupBox (Graphics& g, Rectangle<int> r, const String& title, Colour accent = hc::amber)
{
    g.setColour (Colours::black.withAlpha (0.22f));
    g.fillRoundedRectangle (r.toFloat(), 8.0f);
    g.setColour (hc::line);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 8.0f, 1.0f);
    if (title.isNotEmpty())
    {
        g.setColour (accent);
        g.setFont (uiFont (11.0f, true, 0.18f));
        g.drawText (title, r.reduced (12, 8).removeFromTop (14), Justification::left);
    }
}

//==============================================================================
HornsLookAndFeel::HornsLookAndFeel()
{
    setColour (Slider::textBoxTextColourId, hc::cream);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Label::textColourId, hc::muted);
    setColour (ComboBox::textColourId, hc::cream);
    setColour (ComboBox::arrowColourId, hc::amber);
    setColour (PopupMenu::backgroundColourId, Colour (0xff1e150e));
    setColour (PopupMenu::textColourId, hc::cream);
    setColour (PopupMenu::highlightedBackgroundColourId, hc::amber);
    setColour (PopupMenu::highlightedTextColourId, hc::ink);
    setColour (TextButton::textColourOffId, hc::cream);
    setColour (TextButton::textColourOnId, hc::ink);
    setColour (ScrollBar::thumbColourId, hc::line);
}

void HornsLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float r = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const bool onEcho = s.getProperties().contains ("echo");
    const Colour accent = onEcho ? Colour (0xfff6efd9) : hc::amber;

    Path bg; bg.addCentredArc (c.x, c.y, r - 2, r - 2, 0, a0, a1, true);
    g.setColour (Colour (0xff120c08)); g.strokePath (bg, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
    Path arc; arc.addCentredArc (c.x, c.y, r - 2, r - 2, 0, a0, ang, true);
    g.setColour (s.isEnabled() ? accent : accent.withAlpha (0.3f));
    g.strokePath (arc, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    const float kr = r - 7.0f;
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - kr + 1.5f, c.y - kr + 2.5f, kr * 2, kr * 2);
    ColourGradient body (Colour (0xff6b5440), c.x - kr * 0.5f, c.y - kr * 0.6f, Colour (0xff140e09), c.x + kr, c.y + kr, true);
    body.addColour (0.55, Colour (0xff2e2218));
    g.setGradientFill (body);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (Colours::black);
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.2f);
    // ranhuras da baquelite
    g.setColour (Colours::white.withAlpha (0.05f));
    for (int i = 0; i < 16; ++i)
    {
        const float t = MathConstants<float>::twoPi * (float) i / 16.0f;
        g.drawLine (c.x + std::sin (t) * (kr - 3), c.y - std::cos (t) * (kr - 3), c.x + std::sin (t) * kr, c.y - std::cos (t) * kr, 1.0f);
    }
    Path ptr; ptr.addRoundedRectangle (-1.6f, -kr + 3, 3.2f, kr * 0.55f, 1.5f);
    g.setColour (hc::cream);
    g.fillPath (ptr, AffineTransform::rotation (ang).translated (c.x, c.y));
}

void HornsLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider::SliderStyle, Slider& s)
{
    const float cx = (float) x + (float) w * 0.5f;
    g.setColour (Colour (0xff0d0906));
    g.fillRoundedRectangle (cx - 3, (float) y, 6, (float) h, 3);
    // marca de 0 dB
    const float mid = (float) y + (float) h * 0.5f;
    g.setColour (hc::line);
    g.drawLine (cx - 12, mid, cx + 12, mid, 1.0f);
    const float zeroY = (float) s.getPositionOfValue (0.0);
    g.setColour (hc::amber.withAlpha (0.8f));
    g.fillRect (cx - 2, jmin (pos, zeroY), 4.0f, std::abs (pos - zeroY));
    Rectangle<float> cap (cx - 13, pos - 8, 26, 16);
    ColourGradient cg (Colour (0xffd9d6cf), cap.getX(), cap.getY(), Colour (0xff5e5b55), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 3);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (cap, 3, 1);
    g.drawLine (cap.getX() + 3, pos, cap.getRight() - 3, pos, 1.5f);
}

void HornsLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const auto tint = b.getProperties()["tint"].toString();
    Colour top (0xff4b3725), bot (0xff33251a);
    if (on)
    {
        if (tint == "red")        { top = Colour (0xffe0634a); bot = Colour (0xff9c2c1d); }
        else if (tint == "green") { top = Colour (0xff6cbb74); bot = Colour (0xff357a3e); }
        else                      { top = Colour (0xfff2b25a); bot = Colour (0xffd98a2b); }
    }
    else if (tint == "red")   { top = Colour (0xff8a3324); bot = Colour (0xff5a1c12); }
    else if (tint == "green") { top = Colour (0xff3f7a46); bot = Colour (0xff26502b); }
    if (over) { top = top.brighter (0.08f); bot = bot.brighter (0.08f); }
    if (down) std::swap (top, bot);
    g.setGradientFill (ColourGradient (top, 0, r.getY(), bot, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? hc::amber.withAlpha (0.9f) : Colours::black);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawLine (r.getX() + 4, r.getY() + 1.5f, r.getRight() - 4, r.getY() + 1.5f);
}

void HornsLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    const bool on = b.getToggleState();
    const auto tint = b.getProperties()["tint"].toString();
    g.setColour (on && tint.isEmpty() ? hc::ink : hc::cream);
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 2), Justification::centred, 2);
}

Font HornsLookAndFeel::getTextButtonFont (TextButton& b, int h)
{
    const float sz = b.getProperties().contains ("big") ? 17.0f : jmin (12.5f, (float) h * 0.48f);
    return uiFont (sz, true, 0.04f);
}

void HornsLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    Rectangle<float> r (0, 0, (float) w, (float) h);
    g.setColour (Colour (0xff120c08));
    g.fillRoundedRectangle (r.reduced (0.5f), 4.0f);
    g.setColour (box.getProperties().contains ("need") ? hc::red : box.hasKeyboardFocus (false) ? hc::amber : hc::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
    Path p;
    const float ax = (float) w - 14, ay = (float) h * 0.5f;
    p.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (hc::amber);
    g.fillPath (p);
}

Font HornsLookAndFeel::getComboBoxFont (ComboBox&) { return uiFont (12.5f); }
Font HornsLookAndFeel::getPopupMenuFont()          { return uiFont (14.0f); }

Label* HornsLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (uiFont (11.0f, true));
    l->setColour (Label::textColourId, s.getProperties().contains ("echo") ? Colour (0xfff6efd9) : hc::cream);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    return l;
}

//==============================================================================
KnobCtl::KnobCtl (AudioProcessorValueTreeState& s, const String& id, const String& name)
{
    slider.setSliderStyle (Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (Slider::TextBoxBelow, true, 80, 15);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (180);
    addAndMakeVisible (slider);
    label.setText (upper (name), dontSendNotification);
    slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    label.setJustificationType (Justification::centred);
    label.setFont (uiFont (10.0f, true, 0.06f));
    label.setMinimumHorizontalScale (0.6f);
    addAndMakeVisible (label);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    if (auto* p = s.getParameter (id)) slider.setDoubleClickReturnValue (true, (double) p->convertFrom0to1 (p->getDefaultValue()));
}

void KnobCtl::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromBottom (14));
    slider.setBounds (r);
}

ParamToggle::ParamToggle (AudioProcessorValueTreeState& s, const String& id, const String& offText, const String& onText)
    : off (offText), on (onText.isEmpty() ? offText : onText)
{
    setClickingTogglesState (true);
    att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this);
    updateText();
    onStateChange = [this] { updateText(); };
}
void ParamToggle::buttonStateChanged() { updateText(); }
void ParamToggle::updateText()
{
    const auto t = getToggleState() ? on : off;
    if (getButtonText() != t) setButtonText (t);
}

ParamCombo::ParamCombo (AudioProcessorValueTreeState& s, const String& id)
{
    if (auto* p = dynamic_cast<AudioParameterChoice*> (s.getParameter (id)))
        addItemList (p->choices, 1);
    att = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, *this);
}

HoldButton::HoldButton (AudioProcessorValueTreeState& s, const String& id, const String& label)
    : TextButton (label), param (s.getParameter (id)) {}

void HoldButton::buttonStateChanged()
{
    const bool down = isDown();
    if (down == last || param == nullptr) return;
    last = down;
    param->beginChangeGesture();
    param->setValueNotifyingHost (down ? 1.0f : 0.0f);
    param->endChangeGesture();
    setToggleState (down, dontSendNotification);
}

//==============================================================================
//  NAIPE
//==============================================================================
class InstModule : public Component
{
public:
    InstModule (HornsProcessor& p, int i) : idx (i)
    {
        const String id = HornsProcessor::instIds()[i];
        auto& s = p.apvts;
        onBtn = std::make_unique<ParamToggle> (s, id + "_on", "FORA", "NO NAIPE");
        lvl = std::make_unique<KnobCtl> (s, id + "_lvl", "Volume");
        pan = std::make_unique<KnobCtl> (s, id + "_pan", "Pan");
        harm = std::make_unique<ParamCombo> (s, id + "_harm");
        oct = std::make_unique<ParamCombo> (s, id + "_oct");
        for (Component* c : { (Component*) onBtn.get(), (Component*) lvl.get(), (Component*) pan.get(), (Component*) harm.get(), (Component*) oct.get() })
            addAndMakeVisible (c);
        onBtn->onClick = [this] { repaint(); };
        onBtn->onStateChange = [this] { onBtn->updateText(); repaint(); };
    }
    void paint (Graphics& g) override
    {
        const bool on = onBtn->getToggleState();
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (on ? hc::amber.withAlpha (0.07f) : Colours::black.withAlpha (0.2f));
        g.fillRoundedRectangle (r, 8);
        g.setColour (on ? hc::amber : hc::line);
        g.drawRoundedRectangle (r, 8, on ? 1.5f : 1.0f);
        // lâmpada
        Rectangle<float> lamp (14, 18, 14, 14);
        if (on)
        {
            g.setColour (hc::amber.withAlpha (0.35f)); g.fillEllipse (lamp.expanded (5));
            g.setGradientFill (ColourGradient (Colour (0xffffe3a0), lamp.getCentreX() - 2, lamp.getCentreY() - 2, hc::amber, lamp.getRight(), lamp.getBottom(), true));
        }
        else g.setColour (Colour (0xff3a1d12));
        g.fillEllipse (lamp);
        g.setColour (Colours::black); g.drawEllipse (lamp, 1);
        const auto& d = horns::instruments()[(size_t) idx];
        g.setColour (hc::cream);
        g.setFont (uiFont (16.0f, true, 0.06f));
        g.drawText (d.name, 38, 10, 200, 20, Justification::left);
        g.setColour (hc::muted);
        g.setFont (uiFont (11.5f));
        g.drawText (HU (descPt()), 38, 30, getWidth() - 150, 16, Justification::left);
        g.setFont (uiFont (10.0f, true, 0.1f));
        g.drawText ("HARMONIA", harm->getBounds().translated (0, -15).withHeight (14), Justification::left);
        g.drawText ("OITAVA", oct->getBounds().translated (0, -15).withHeight (14), Justification::left);
    }
    void resized() override
    {
        onBtn->setBounds (getWidth() - 106, 12, 94, 30);
        lvl->setBounds (12, 54, 74, 86);
        pan->setBounds (88, 54, 74, 86);
        harm->setBounds (172, 72, getWidth() - 184, 26);
        oct->setBounds (172, 120, getWidth() - 184, 26);
    }
private:
    const char* descPt() const
    {
        static const char* d[] = { "Brilhante, ataque rápido", "Redondo e aveludado", "Nasal e cortante",
                                   "Encorpado e rouco, o sax das pontadas", "Grave, gordo, ataque mais lento", "A base grave do naipe" };
        return d[idx];
    }
    int idx;
    std::unique_ptr<ParamToggle> onBtn;
    std::unique_ptr<KnobCtl> lvl, pan;
    std::unique_ptr<ParamCombo> harm, oct;
};

class NaipePage : public HornsPage
{
public:
    explicit NaipePage (HornsProcessor& p) : HornsPage (p)
    {
        for (int i = 0; i < horns::kNumInst; ++i) addAndMakeVisible (*mods.emplace_back (std::make_unique<InstModule> (p, i)));
        // ordem: trompete, flugel, alto, tenor, trombone, barítono  (harm: 0 uníss. 2 3ª abaixo 3 5ª abaixo 5 6ª abaixo)
        presets = {
            { "Trio clássico (TPT · TEN · TBN)", { 1, 0, 0, 1, 1, 0 }, { 0, 0, 0, -1, 0, 0 }, { 0, 0, 0, 0, 2, 0 } },
            { "Trompete + Sax Tenor",            { 1, 0, 0, 1, 0, 0 }, { 0, 0, 0, -1, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },
            { "Sax Tenor solo (pontadas)",       { 0, 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0, 0 },  { 0, 0, 0, 0, 0, 0 } },
            { "Saxes (alto · tenor · barítono)", { 0, 0, 1, 1, 0, 1 }, { 0, 0, 0, 0, 0, -1 }, { 0, 0, 0, 2, 0, 0 } },
            { "Naipe cheio (6)",                 { 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, -1, 0, -1 }, { 0, 2, 3, 0, 5, 0 } },
            { "Trombone solo",                   { 0, 0, 0, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0 },  { 0, 0, 0, 0, 0, 0 } },
        };
        for (size_t k = 0; k < presets.size(); ++k)
        {
            auto& b = *buttons.emplace_back (std::make_unique<TextButton> (HU (presets[k].name)));
            addAndMakeVisible (b);
            b.onClick = [this, k]
            {
                const auto ids = HornsProcessor::instIds();
                for (int i = 0; i < horns::kNumInst; ++i)
                {
                    setParamValue (proc.apvts, ids[i] + "_on", (float) presets[k].on[(size_t) i]);
                    if (presets[k].on[(size_t) i])
                    {
                        setParamValue (proc.apvts, ids[i] + "_oct", (float) (presets[k].oct[(size_t) i] + 2));
                        setParamValue (proc.apvts, ids[i] + "_harm", (float) presets[k].harm[(size_t) i]);
                    }
                }
                for (size_t j = 0; j < buttons.size(); ++j) buttons[j]->setToggleState (j == k, dontSendNotification);
                for (auto& m : mods) m->repaint();
                proc.previewNote (60 + (int) proc.apvts.getRawParameterValue ("key")->load(), horns::Ten);
            };
        }
    }
    void paint (Graphics& g) override
    {
        g.setColour (hc::cream); g.setFont (uiFont (19.0f, true, 0.05f));
        g.drawText ("MONTE O SEU NAIPE", 20, 6, 600, 24, Justification::left);
        g.setColour (hc::muted); g.setFont (uiFont (12.5f));
        g.drawText (HU ("Ligue os instrumentos. Cada um dobra a nota em uníssono, oitava ou harmoniza em 3ª/5ª/6ª dentro do tom e escala lá de cima: uma nota vira um acorde de metais."),
                    20, 30, getWidth() - 40, 18, Justification::left);
    }
    void resized() override
    {
        auto row = Rectangle<int> (20, 56, getWidth() - 40, 30);
        const int bw = (row.getWidth() - 5 * 8) / 6;
        for (auto& b : buttons) { b->setBounds (row.removeFromLeft (bw)); row.removeFromLeft (8); }
        const int cw = (getWidth() - 40 - 2 * 14) / 3, ch = 154;
        for (int i = 0; i < (int) mods.size(); ++i)
            mods[(size_t) i]->setBounds (20 + (i % 3) * (cw + 14), 100 + (i / 3) * (ch + 12), cw, ch);
    }
private:
    struct Preset { const char* name; std::array<int, 6> on, oct, harm; };
    std::vector<Preset> presets;
    std::vector<std::unique_ptr<InstModule>> mods;
    std::vector<std::unique_ptr<TextButton>> buttons;
};

//==============================================================================
//  TIMBRE
//==============================================================================
class TimbrePage : public HornsPage
{
public:
    explicit TimbrePage (HornsProcessor& p) : HornsPage (p)
    {
        auto add = [&] (std::vector<std::unique_ptr<KnobCtl>>& v, const char* id, const char* name)
        { addAndMakeVisible (*v.emplace_back (std::make_unique<KnobCtl> (p.apvts, id, HU (name)))); };
        add (tone, "bright", "Brilho"); add (tone, "breath", "Sopro"); add (tone, "vib", "Vibrato");
        add (tone, "growl", "Rouco");
        add (tone, "scoop", "Scoop");   add (tone, "attack", "Ataque"); add (tone, "release", "Release");
        add (tape, "drive", "Saturação"); add (tape, "band", "Banda (agudo)"); add (tape, "room", "Sala");
        add (tape, "lowcut", "Corte grave"); add (tape, "hiss", "Chiado");
        add (human, "human", "Humanizar"); add (human, "width", "Abertura estéreo");

        // valores: bright breath vib scoop attack release drive band lowcut hiss human width
        presets = {
            { "Randy's '72 · brilhante e seco",  { .68f, .32f, .28f, .30f, .8f, .22f, .30f, 10500, 90, .12f, .45f, .6f, .30f } },
            { "Channel One · punch",             { .78f, .30f, .22f, .35f, .6f, .20f, .50f, 11500, 110, .10f, .40f, .7f, .20f } },
            { "Tubby's dubplate",                { .52f, .42f, .30f, .45f, 1.f, .35f, .55f, 7600, 130, .30f, .55f, .8f, .25f } },
            { "Black Ark haze",                  { .42f, .55f, .35f, .50f, 1.2f, .45f, .65f, 6200, 100, .42f, .65f, .9f, .55f } },
            { "Treasure Isle · rocksteady suave", { .36f, .45f, .45f, .35f, 1.3f, .40f, .20f, 8200, 70, .18f, .50f, .6f, .40f } },
            { "Rádio AM de Kingston",            { .60f, .30f, .30f, .30f, .9f, .30f, .70f, 4800, 320, .35f, .50f, .3f, .15f } },
        };
        static const char* ids[13] = { "bright", "breath", "vib", "scoop", "attack", "release", "drive", "band", "lowcut", "hiss", "human", "width", "room" };
        for (size_t k = 0; k < presets.size(); ++k)
        {
            auto& b = *buttons.emplace_back (std::make_unique<TextButton> (HU (presets[k].n)));
            addAndMakeVisible (b);
            b.onClick = [this, k]
            {
                for (int i = 0; i < 13; ++i) setParamValue (proc.apvts, ids[i], presets[k].v[i]);
                for (size_t j = 0; j < buttons.size(); ++j) buttons[j]->setToggleState (j == k, dontSendNotification);
                proc.previewNote (60 + (int) proc.apvts.getRawParameterValue ("key")->load(), horns::Ten);
            };
        }
    }
    void paint (Graphics& g) override
    {
        g.setColour (hc::cream); g.setFont (uiFont (19.0f, true, 0.05f));
        g.drawText (HU ("TIMBRE DE ESTÚDIO JAMAICANO, ANOS 70"), 20, 6, 700, 24, Justification::left);
        g.setColour (hc::muted); g.setFont (uiFont (12.5f));
        g.drawText (HU ("Banda estreita de console, saturação de fita, sopro na palheta e chiado. Arraste os knobs; duplo clique volta ao padrão."),
                    20, 30, getWidth() - 40, 18, Justification::left);
        groupBox (g, gTone, HU ("SOPRO & ARTICULAÇÃO"));
        groupBox (g, gTape, "FITA & CONSOLE");
        groupBox (g, gHuman, "NAIPE VIVO");
        groupBox (g, hint, "DICAS");
        g.setColour (hc::muted); g.setFont (uiFont (12.5f));
        g.drawFittedText (HU ("• Brilho controla quanto o filtro abre com a força da nota: o \"blat\" do metal.\n\n"
                              "• Scoop faz a nota entrar um pouco abaixo e subir, como o músico de verdade.\n\n"
                              "• Banda e Corte grave imitam a mesa e a fita dos anos 70: menos agudo, menos grave, mais médio.\n\n"
                              "• Humanizar solta um pouco o tempo e a afinação entre os músicos do naipe.\n\n"
                              "• Rouco é a aspereza da palheta, o tenor estilo Dean Fraser. Sala é a sala de gravação.\n\n"
                              "• Fall e doit seguram a nota e caem junto com o volume, como o músico de verdade."),
                          hint.reduced (14, 12).withTrimmedTop (22), Justification::topLeft, 28);
    }
    void resized() override
    {
        auto row = Rectangle<int> (20, 56, getWidth() - 40, 30);
        const int bw = (row.getWidth() - 5 * 8) / 6;
        for (auto& b : buttons) { b->setBounds (row.removeFromLeft (bw)); row.removeFromLeft (8); }
        const int top = 110, kh = 104, kw = 84;
        gTone  = { 20, top, 4 * kw + 30, 2 * kh + 46 };
        gTape  = { gTone.getRight() + 16, top, 3 * kw + 30, 2 * kh + 46 };
        gHuman = { gTape.getRight() + 16, top, kw + 30, 2 * kh + 46 };
        auto place = [&] (std::vector<std::unique_ptr<KnobCtl>>& v, Rectangle<int> g, int cols)
        {
            for (int i = 0; i < (int) v.size(); ++i)
                v[(size_t) i]->setBounds (g.getX() + 15 + (i % cols) * kw, g.getY() + 32 + (i / cols) * kh, kw, kh - 6);
        };
        place (tone, gTone, 4); place (tape, gTape, 3); place (human, gHuman, 1);
        hint = { gHuman.getRight() + 16, top, getWidth() - gHuman.getRight() - 36, 2 * kh + 46 };
    }
private:
    struct T { const char* n; float v[13]; };
    std::vector<T> presets;
    std::vector<std::unique_ptr<KnobCtl>> tone, tape, human;
    std::vector<std::unique_ptr<TextButton>> buttons;
    Rectangle<int> gTone, gTape, gHuman, hint;
};

//==============================================================================
//  MÚSICOS — presets no padrão da Riddim Machine (visor azul de MPC: ESTILO ◀ ▶, MÚSICO ◀ ▶, LISTA)
//==============================================================================
namespace mpc   // mesmo visual do MRM-0420 (RiddimMachine/Source/MrmStyle.h)
{
    const Colour light { 0xffe8f3ff };
    static void lcd (Graphics& g, Rectangle<float> r)
    {
        g.setColour (Colour (0xff0a0c10)); g.fillRoundedRectangle (r.expanded (4.0f), 5.0f);
        g.setColour (Colours::white.withAlpha (0.25f)); g.drawRoundedRectangle (r.expanded (4.0f).translated (0, 1.0f), 5.0f, 1.0f);
        ColourGradient gl (Colour (0xff4a8cf0), r.getX(), r.getY(), Colour (0xff1d4aa8), r.getX(), r.getBottom(), false);
        gl.addColour (0.55, Colour (0xff2e66cc));
        g.setGradientFill (gl); g.fillRoundedRectangle (r, 3.0f);
        g.setColour (Colours::black.withAlpha (0.07f));
        for (float y = r.getY() + 2.0f; y < r.getBottom() - 1.0f; y += 3.0f) g.drawHorizontalLine ((int) y, r.getX() + 2.0f, r.getRight() - 2.0f);
        g.setColour (Colours::black.withAlpha (0.35f)); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    }
    static void shine (Graphics& g, Rectangle<float> r)
    {
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.16f), r.getX(), r.getY(), Colours::transparentWhite, r.getX(), r.getY() + r.getHeight() * 0.45f, false));
        g.fillRoundedRectangle (r, 3.0f);
    }
    static void text (Graphics& g, const String& t, Rectangle<float> r, Justification j, float size, float alpha = 1.0f, float kerning = 0.08f)
    {
        g.setFont (uiFont (size, true, kerning));
        g.setColour (Colours::white.withAlpha (0.22f * alpha));
        for (auto o : { Point<float> (-0.8f, 0), { 0.8f, 0 }, { 0, -0.8f }, { 0, 0.8f } }) g.drawText (t, r.translated (o.x, o.y), j, true);
        g.setColour (light.withAlpha (alpha)); g.drawText (t, r, j, true);
    }
    static void key (Graphics& g, Rectangle<float> r, const String& t, bool down, bool lit = false, float size = 14.0f)
    {
        g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r.translated (0, 2.0f), 3.0f);
        auto cap = r.translated (0, down ? 1.5f : 0.0f);
        g.setGradientFill (ColourGradient (Colour (0xff2c2925), cap.getX(), cap.getY(), Colour (0xff141210), cap.getX(), cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (Colour (0xff4a8cf0).withAlpha (down || lit ? 0.95f : 0.35f));
        g.fillRoundedRectangle (cap.withTop (cap.getBottom() - 3.0f).reduced (5.0f, 0.0f).translated (0, -1.5f), 1.0f);
        g.setColour (lit ? mpc::light : hc::cream);
        g.setFont (uiFont (size, true, 0.05f));
        g.drawFittedText (t, cap.withTrimmedBottom (3.0f).reduced (6.0f, 0.0f).toNearestInt(), Justification::centred, 2, 0.8f);
    }
}

class MusicosPage : public HornsPage, private Timer
{
public:
    explicit MusicosPage (HornsProcessor& p) : HornsPage (p)
    {
        setMouseCursor (MouseCursor::PointingHandCursor);
        startTimerHz (6);
    }

    int current() const { return jlimit (-1, musicos::numPresets - 1, (int) proc.apvts.state.getProperty ("musico", -1)); }
    static int folderOf (int preset)
    {
        if (preset < 0) return -1;
        for (int f = 0; f < musicos::numFolders; ++f)
            if (String::fromUTF8 (musicos::presets[preset].folder) == String::fromUTF8 (musicos::folders[f])) return f;
        return -1;
    }
    static Array<int> presetsOf (int folder)
    {
        Array<int> out;
        for (int i = 0; i < musicos::numPresets; ++i) if (folderOf (i) == folder) out.add (i);
        return out;
    }

    void apply (int i)
    {
        if (i < 0 || i >= musicos::numPresets) return;
        const auto& pr = musicos::presets[i];
        const auto ids = HornsProcessor::instIds();
        for (int k = 0; k < horns::kNumInst; ++k)
        {
            const auto& sc = pr.sec[k];
            setParamValue (proc.apvts, ids[k] + "_on", (float) sc.on);
            setParamValue (proc.apvts, ids[k] + "_lvl", sc.lvl);
            setParamValue (proc.apvts, ids[k] + "_pan", sc.pan);
            setParamValue (proc.apvts, ids[k] + "_oct", (float) (sc.oct + 2));
            setParamValue (proc.apvts, ids[k] + "_harm", (float) sc.harm);
        }
        for (int k = 0; k < 14; ++k) setParamValue (proc.apvts, musicos::timbreIds[k], pr.t[k]);
        setParamValue (proc.apvts, "echoOn", (float) pr.echoOn);
        setParamValue (proc.apvts, "sync", (float) pr.sync);
        setParamValue (proc.apvts, "fb", pr.fb);
        setParamValue (proc.apvts, "send", pr.send);
        setParamValue (proc.apvts, "tone", pr.tone);
        for (int b = 0; b < 10; ++b) setParamValue (proc.apvts, "eq" + String (b), pr.eq[b]);
        proc.apvts.state.setProperty ("musico", i, nullptr);
        folderSel = folderOf (i);
        proc.previewNote (60 + (int) proc.apvts.getRawParameterValue ("key")->load(), pr.artic);
        repaint();
    }

    void stepFolder (int d)
    {
        const int cur = folderOf (current());
        folderSel = ((cur >= 0 ? cur : folderSel) + d + musicos::numFolders) % musicos::numFolders;
        const auto l = presetsOf (folderSel);
        if (! l.isEmpty()) apply (l[0]);
    }
    void stepPreset (int d)
    {
        const int cur = current();
        const auto l = presetsOf (cur >= 0 ? folderOf (cur) : folderSel);
        if (l.isEmpty()) return;
        const int pos = l.indexOf (cur), n = l.size();
        apply (l[pos < 0 ? (d > 0 ? 0 : n - 1) : (pos + d + n) % n]);
    }
    void showList()
    {
        PopupMenu menu;
        menu.addSectionHeader (HU ("MÚSICOS DA JAMAICA POR ESTILO"));
        const int cur = current();
        for (int f = 0; f < musicos::numFolders; ++f)
        {
            PopupMenu sub;
            const auto l = presetsOf (f);
            for (int i : l) sub.addItem (i + 1, HU (musicos::presets[i].name) + HU ("   ·  ") + HU (musicos::presets[i].role), true, cur == i);
            menu.addSubMenu (HU (musicos::folders[f]) + "   (" + String (l.size()) + ")", sub, true, nullptr, folderOf (cur) == f);
        }
        Component::SafePointer<MusicosPage> safe (this);
        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (this), [safe] (int r) { if (safe != nullptr && r > 0) safe->apply (r - 1); });
    }

    void resized() override
    {
        const int y = 58;
        keys[0] = { 20, y, 44, 34 };      keys[1] = { 70, y, 44, 34 };
        keys[2] = { 20, y + 40, 44, 34 }; keys[3] = { 70, y + 40, 44, 34 };
        listKey = { getWidth() - 20 - 112, y, 112, 74 };
        lcd = { 204, y + 2, listKey.getX() - 14 - 204, 70 };
        // teclas de todos os presets, uma coluna por estilo
        const int top = 162, colW = (getWidth() - 40 - (musicos::numFolders - 1) * 8) / musicos::numFolders;
        for (int f = 0; f < musicos::numFolders; ++f)
        {
            const auto l = presetsOf (f);
            for (int k = 0; k < l.size(); ++k)
                presetKeys[(size_t) l[k]] = { 20 + f * (colW + 8), top + 22 + k * 40, colW, 34 };
            folderHead[(size_t) f] = { 20 + f * (colW + 8), top, colW, 18 };
        }
        card = { 20, 360, getWidth() - 40, 96 };
    }

    void paint (Graphics& g) override
    {
        g.setColour (hc::cream); g.setFont (uiFont (19.0f, true, 0.05f));
        g.drawText (HU ("MÚSICOS DA JAMAICA"), 20, 6, 700, 24, Justification::left);
        g.setColour (hc::muted); g.setFont (uiFont (12.5f));
        g.drawText (HU ("Cada preset carrega a estética de um músico ou naipe: instrumentos, timbre, rouco, sala, eco e EQ. O tom e a escala continuam os seus."),
                    20, 30, getWidth() - 40, 18, Justification::left);

        const String L = String::fromUTF8 ("\xe2\x97\x80"), R = String::fromUTF8 ("\xe2\x96\xb6");
        for (int i = 0; i < 4; ++i) mpc::key (g, keys[i].toFloat(), i % 2 == 0 ? L : R, pressed == i);
        g.setColour (hc::cream); g.setFont (uiFont (10.0f, true, 0.25f));
        g.drawText ("ESTILO", 122, keys[0].getY(), 76, 34, Justification::centredLeft);
        g.drawText (HU ("MÚSICO"), 122, keys[2].getY(), 76, 34, Justification::centredLeft);

        auto l = lcd.toFloat();
        mpc::lcd (g, l);
        auto t = l.reduced (14.0f, 6.0f);
        const int cur = current();
        const int f = cur >= 0 ? folderOf (cur) : folderSel;
        const auto list = presetsOf (f);
        mpc::text (g, HU (musicos::folders[f]), t.withHeight (16.0f), Justification::centredLeft, 13.0f, 0.85f, 0.2f);
        if (cur >= 0)
        {
            const auto& pr = musicos::presets[cur];
            mpc::text (g, String (list.indexOf (cur) + 1).paddedLeft ('0', 2) + "/" + String (list.size()).paddedLeft ('0', 2),
                       t.withHeight (16.0f), Justification::centredRight, 12.0f, 0.8f);
            mpc::text (g, upper (HU (pr.name)), t.withTrimmedTop (17.0f).withHeight (22.0f), Justification::centredLeft, 17.0f, 1.0f, 0.04f);
            mpc::text (g, HU (pr.role) + HU ("  ·  ") + upper (HU (horns::articName (pr.artic))), t.withTrimmedTop (41.0f), Justification::centredLeft, 10.5f, 0.85f, 0.06f);
        }
        else
        {
            mpc::text (g, HU ("ESCOLHA UM MÚSICO  ·  \xe2\x97\x80 \xe2\x96\xb6"), t.withTrimmedTop (17.0f).withHeight (22.0f), Justification::centredLeft, 16.0f, 0.8f);
            mpc::text (g, HU ("DEAN FRASER, VIN GORDON, TOMMY McCOOK, BOBBY ELLIS E OS NAIPES CLÁSSICOS"), t.withTrimmedTop (41.0f), Justification::centredLeft, 10.5f, 0.7f);
        }
        mpc::shine (g, l);

        auto lk = listKey.toFloat();
        mpc::key (g, lk, "", pressed == 4);
        g.setColour (hc::cream); g.setFont (uiFont (14.0f, true, 0.12f));
        g.drawText ("LISTA", lk.withTrimmedBottom (34.0f).translated (0, pressed == 4 ? 1.5f : 0.0f), Justification::centredBottom);
        g.setFont (uiFont (7.5f, true, 0.12f)); g.setColour (hc::cream.withAlpha (0.6f));
        g.drawText (HU ("POR ESTILO \xe2\x96\xbe"), lk.withTrimmedTop (44.0f).withHeight (14.0f).translated (0, pressed == 4 ? 1.5f : 0.0f), Justification::centred);

        // teclas por estilo
        for (int ff = 0; ff < musicos::numFolders; ++ff)
        {
            g.setColour (ff == f ? hc::amber : hc::muted); g.setFont (uiFont (9.5f, true, 0.12f));
            g.drawFittedText (HU (musicos::folders[ff]), folderHead[(size_t) ff], Justification::centredLeft, 1, 0.7f);
        }
        for (int i = 0; i < musicos::numPresets; ++i)
            mpc::key (g, presetKeys[(size_t) i].toFloat(), upper (HU (musicos::presets[i].name)), pressed == 100 + i, cur == i, 10.5f);

        // cartão do estilo
        groupBox (g, card, cur >= 0 ? HU ("O ESTILO") : HU ("COMO USAR"));
        g.setColour (hc::cream); g.setFont (uiFont (13.5f));
        auto c = card.reduced (14, 10).withTrimmedTop (22);
        if (cur >= 0)
        {
            const auto& pr = musicos::presets[cur];
            g.drawFittedText (HU (pr.desc), c.removeFromTop (22), Justification::left, 1);
            String naipe;
            const char* sh[6] = { "TPT", "FLG", "ALT", "TEN", "TBN", "BAR" };
            for (int k = 0; k < 6; ++k) if (pr.sec[k].on) naipe << (naipe.isEmpty() ? "" : HU (" · ")) << sh[k];
            g.setColour (hc::muted); g.setFont (uiFont (12.0f));
            g.drawFittedText (HU ("Carrega: naipe ") + naipe + HU ("  ·  rouco ") + String (roundToInt (pr.t[12] * 100)) + HU ("%  ·  sala ")
                              + String (roundToInt (pr.t[13] * 100)) + HU ("%  ·  eco ") + String (horns::syncDivs()[(size_t) pr.sync].name)
                              + HU ("  ·  articulação de demonstração: ") + horns::articName (pr.artic)
                              + HU (".  Depois ajuste à vontade nas outras abas."), c, Justification::topLeft, 2);
        }
        else
            g.drawFittedText (HU ("Use ◀ ▶ para andar pelos estilos e pelos músicos, ou toque numa tecla. O som aparece na hora: toque o teclado ou uma frase MIDI.\n"
                                  "Homenagem ao jeito de tocar de cada um. Não são gravações dos músicos: é o motor do Dub Horns ajustado para lembrar o estilo."),
                              c, Justification::topLeft, 3);
    }

    void mouseDown (const MouseEvent& e) override
    {
        const auto pt = e.getPosition();
        pressed = -1;
        for (int i = 0; i < 4; ++i)
            if (keys[i].contains (pt)) { pressed = i; if (i < 2) stepFolder (i == 0 ? -1 : 1); else stepPreset (i == 2 ? -1 : 1); repaint(); return; }
        if (listKey.contains (pt) || lcd.contains (pt)) { pressed = 4; repaint(); showList(); return; }
        for (int i = 0; i < musicos::numPresets; ++i)
            if (presetKeys[(size_t) i].contains (pt)) { pressed = 100 + i; apply (i); return; }
    }
    void mouseUp (const MouseEvent&) override { pressed = -1; repaint(); }

private:
    void timerCallback() override { const int c = current(); if (c != shown) { shown = c; if (c >= 0) folderSel = folderOf (c); repaint(); } }
    Rectangle<int> lcd, keys[4], listKey, card;
    std::array<Rectangle<int>, musicos::numPresets> presetKeys {};
    std::array<Rectangle<int>, musicos::numFolders> folderHead {};
    int pressed = -1, shown = -2, folderSel = 5;
};

//==============================================================================
//  MASTOR TAPE ECHO
//==============================================================================
class EchoPage : public HornsPage
{
public:
    explicit EchoPage (HornsProcessor& p) : HornsPage (p), sync (p.apvts, "sync"),
        echoOn (p.apvts, "echoOn", "ECHO DESLIGADO", "ECHO LIGADO"),
        throwBtn (p.apvts, "throw", "SEGURE: DUB THROW"), selfOsc (p.apvts, "selfOsc", "SEGURE: AUTO-OSCILAR")
    {
        addAndMakeVisible (sync);
        time = std::make_unique<KnobCtl> (p.apvts, "echoTime", "Tempo livre");
        time->slider.getProperties().set ("echo", true);
        addAndMakeVisible (*time);
        const char* ids[7] = { "fb", "send", "ret", "tone", "bass", "wow", "sat" };
        const char* nm[7] = { "Intensidade", "Envio", "Volume eco", "Agudos", "Graves (corte)", "Wow & flutter", "Saturação fita" };
        for (int i = 0; i < 7; ++i)
        {
            auto& k = *knobs.emplace_back (std::make_unique<KnobCtl> (p.apvts, ids[i], HU (nm[i])));
            k.slider.getProperties().set ("echo", true);
            k.label.setColour (Label::textColourId, Colour (0xffe4ddc6));
            addAndMakeVisible (k);
        }
        time->label.setColour (Label::textColourId, Colour (0xffe4ddc6));
        for (int h = 0; h < 3; ++h)
            addAndMakeVisible (*heads.emplace_back (std::make_unique<ParamToggle> (p.apvts, "head" + String (h + 1), "H" + String (h + 1))));
        addAndMakeVisible (echoOn);
        throwBtn.getProperties().set ("tint", "red");
        throwBtn.getProperties().set ("big", true);
        selfOsc.getProperties().set ("big", true);
        addAndMakeVisible (throwBtn);
        addAndMakeVisible (selfOsc);
    }
    void tick() override
    {
        const float msNow = proc.getEchoMs();
        const bool on = proc.apvts.getRawParameterValue ("echoOn")->load() > 0.5f;
        if (on) reelAngle += 0.08f * (0.6f + 400.0f / jmax (60.0f, msNow));
        time->slider.setEnabled ((int) proc.apvts.getRawParameterValue ("sync")->load() == 0);
        if (std::abs (msNow - lastMs) > 0.5f || on) { lastMs = msNow; repaint (face.withTrimmedLeft (face.getWidth() - 420).withHeight (110)); }
    }
    void paint (Graphics& g) override
    {
        auto r = face.toFloat();
        g.setColour (Colours::black); g.fillRoundedRectangle (r.expanded (2), 12);
        g.setGradientFill (ColourGradient (Colour (0xff5d6b4f), 0, r.getY(), Colour (0xff46523b), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 10);
        g.setColour (Colours::white.withAlpha (0.12f)); g.drawLine (r.getX() + 10, r.getY() + 1.5f, r.getRight() - 10, r.getY() + 1.5f);
        // parafusos
        for (auto pt : { r.getTopLeft() + Point<float> (12, 12), r.getTopRight() + Point<float> (-12, 12), r.getBottomLeft() + Point<float> (12, -12), r.getBottomRight() + Point<float> (-12, -12) })
        {
            g.setGradientFill (ColourGradient (Colour (0xfffafafa), pt.x - 2, pt.y - 2, Colour (0xff6a6a6a), pt.x + 3, pt.y + 3, true));
            g.fillEllipse (pt.x - 4, pt.y - 4, 8, 8);
        }
        g.setColour (Colour (0xfff6efd9));
        g.setFont (uiFont (26.0f, true, 0.12f));
        g.drawText ("MASTOR TAPE ECHO", face.getX() + 28, face.getY() + 20, 460, 30, Justification::left);
        g.setColour (Colour (0xffd7d0b8));
        g.setFont (uiFont (11.0f, true, 0.3f));
        g.drawText (HU ("SPACE DELAY · 3 CABEÇAS · FITA"), face.getX() + 30, face.getY() + 52, 400, 14, Justification::left);

        // rolos de fita + LCD
        auto drawReel = [&] (Point<float> c)
        {
            g.setColour (Colour (0xff111111)); g.fillEllipse (c.x - 34, c.y - 34, 68, 68);
            g.setColour (Colour (0xff2a2a2a)); g.fillEllipse (c.x - 28, c.y - 28, 56, 56);
            g.setColour (Colour (0xff999999).withAlpha (0.8f));
            for (int i = 0; i < 3; ++i)
            {
                Path sp; sp.addPieSegment (c.x - 24, c.y - 24, 48, 48, 0.0f, 0.35f, 0.25f);
                g.fillPath (sp, AffineTransform::rotation (reelAngle + (float) i * MathConstants<float>::twoPi / 3.0f, c.x, c.y));
            }
            g.setColour (Colour (0xff777777)); g.fillEllipse (c.x - 6, c.y - 6, 12, 12);
            g.setColour (Colours::black); g.drawEllipse (c.x - 34, c.y - 34, 68, 68, 2);
        };
        const float ry = (float) face.getY() + 54;
        drawReel ({ (float) face.getRight() - 360, ry });
        drawReel ({ (float) face.getRight() - 60, ry });
        auto lcd = Rectangle<float> ((float) face.getRight() - 290, ry - 20, 160, 40);
        g.setColour (Colours::black); g.fillRoundedRectangle (lcd.expanded (2), 5);
        g.setColour (Colour (0xff0e1a0e)); g.fillRoundedRectangle (lcd, 4);
        g.setColour (Colour (0xff8fff8f));
        g.setFont (uiFont (22.0f, true));
        g.drawText (String (roundToInt (proc.getEchoMs())) + " ms", lcd, Justification::centred);

        g.setColour (Colour (0xffe4ddc6)); g.setFont (uiFont (11.0f, true, 0.12f));
        g.drawText ("TEMPO", sync.getBounds().translated (0, -17).withHeight (14), Justification::left);
        g.drawText (HU ("CABEÇAS"), heads.front()->getBounds().translated (0, -17).withWidth (150).withHeight (14), Justification::left);
        g.setFont (uiFont (11.5f));
        g.drawFittedText (HU ("Cabeça 2 e 3 repetem em 2x e 3x o tempo, como nos ecos de fita clássicos."),
                          heads.front()->getBounds().translated (0, 38).withWidth (170).withHeight (40), Justification::topLeft, 3);
        g.drawFittedText (HU ("Throw manda tudo para o eco enquanto você segura (o pedal de sustain do teclado MIDI faz o mesmo). Auto-oscilar sobe a intensidade além de 100%."),
                          Rectangle<int> (selfOsc.getRight() + 18, selfOsc.getY(), face.getRight() - selfOsc.getRight() - 40, 52), Justification::centredLeft, 3);
    }
    void resized() override
    {
        face = getLocalBounds().reduced (20, 14);
        const int y0 = face.getY() + 128;
        sync.setBounds (face.getX() + 28, y0, 170, 26);
        time->setBounds (face.getX() + 70, y0 + 36, 84, 100);
        for (int h = 0; h < 3; ++h) heads[(size_t) h]->setBounds (face.getX() + 228 + h * 56, y0, 50, 30);
        int x = face.getX() + 420;
        for (auto& k : knobs) { k->setBounds (x, y0 - 6, 92, 112); x += 96; }
        const int by = face.getBottom() - 84;
        echoOn.setBounds (face.getX() + 28, by + 8, 150, 40);
        throwBtn.setBounds (face.getX() + 196, by, 250, 56);
        selfOsc.setBounds (throwBtn.getRight() + 14, by, 250, 56);
    }
private:
    ParamCombo sync;
    std::unique_ptr<KnobCtl> time;
    std::vector<std::unique_ptr<KnobCtl>> knobs;
    std::vector<std::unique_ptr<ParamToggle>> heads;
    ParamToggle echoOn;
    HoldButton throwBtn, selfOsc;
    Rectangle<int> face;
    float reelAngle = 0.0f, lastMs = 0.0f;
};

//==============================================================================
//  EQUALIZADOR
//==============================================================================
class EqCurve : public Component
{
public:
    explicit EqCurve (HornsProcessor& p) : proc (p) { refresh(); }
    bool refresh()
    {
        const auto P = proc.currentParams();
        std::array<float, 12> v {};
        for (int i = 0; i < 10; ++i) v[(size_t) i] = P.eq[(size_t) i];
        v[10] = P.tubby; v[11] = P.tubbyRes;
        if (v == last) return false;
        last = v; repaint(); return true;
    }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Colour (0xff120c08)); g.fillRoundedRectangle (r, 6);
        g.setColour (Colours::black); g.drawRoundedRectangle (r.reduced (0.5f), 6, 1);
        auto fx = [&] (double f) { return r.getX() + (float) (std::log (f / 20.0) / std::log (1000.0)) * r.getWidth(); };
        auto gy = [&] (double db) { return r.getCentreY() - (float) (db / 15.0) * (r.getHeight() * 0.5f - 8); };
        g.setColour (Colour (0xff2c2016));
        for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 }) g.drawVerticalLine ((int) fx (f), r.getY(), r.getBottom());
        for (double d : { -12.0, -6.0, 0.0, 6.0, 12.0 }) g.drawHorizontalLine ((int) gy (d), r.getX(), r.getRight());
        g.setColour (Colour (0xff7a6a50)); g.setFont (uiFont (10.0f));
        for (auto [f, t] : { std::pair<double, const char*> { 100.0, "100" }, { 1000.0, "1k" }, { 10000.0, "10k" } })
            g.drawText (t, (int) fx (f) + 3, (int) r.getBottom() - 14, 40, 12, Justification::left);

        const double sr = 48000.0;
        std::array<horns::Biquad, 11> bq;
        for (int i = 0; i < 10; ++i) horns::setEqBand (bq[(size_t) i], i, last[(size_t) i], (float) sr);
        bq[10].highpass (last[10], horns::tubbyQ (last[11]), (float) sr);
        Path curve; Path fill;
        const int N = 240;
        for (int k = 0; k < N; ++k)
        {
            const double f = 20.0 * std::pow (1000.0, (double) k / (N - 1));
            double db = 0; for (auto& b : bq) db += b.magnitudeDb (f, sr);
            const float x = fx (f), y = gy (jlimit (-15.0, 15.0, db));
            if (k == 0) { curve.startNewSubPath (x, y); fill.startNewSubPath (x, r.getBottom()); }
            else curve.lineTo (x, y);
            fill.lineTo (x, y);
        }
        fill.lineTo (r.getRight(), r.getBottom()); fill.closeSubPath();
        g.setColour (hc::amber.withAlpha (0.12f)); g.fillPath (fill);
        g.setColour (hc::amber); g.strokePath (curve, PathStrokeType (2.5f));
    }
private:
    HornsProcessor& proc;
    std::array<float, 12> last { { 99 } };
};

class EqPage : public HornsPage
{
public:
    explicit EqPage (HornsProcessor& p) : HornsPage (p), curve (p)
    {
        addAndMakeVisible (curve);
        for (int b = 0; b < 10; ++b)
        {
            auto& s = *faders.emplace_back (std::make_unique<Slider> (Slider::LinearVertical, Slider::TextBoxAbove));
            s.setTextBoxStyle (Slider::TextBoxAbove, true, 56, 14);
            s.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
            addAndMakeVisible (s);
            atts.emplace_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "eq" + String (b), s));
            s.setDoubleClickReturnValue (true, 0.0);
            s.textFromValueFunction = [] (double v) { return (v > 0 ? "+" : "") + String (v, v == std::floor (v) ? 0 : 1); };
            s.updateText();
        }
        tubby = std::make_unique<KnobCtl> (p.apvts, "tubby", HU ("Frequência"));
        res = std::make_unique<KnobCtl> (p.apvts, "tubbyRes", HU ("Ressonância"));
        addAndMakeVisible (*tubby); addAndMakeVisible (*res);

        presets = {
            { "Flat",            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 20 },
            { "Metais na cara",  { -4, -3, -1, 0, 1, 3, 4, 3, 1, -2 }, 60 },
            { "Tubby hi-pass",   { -12, -9, -5, -2, 0, 2, 4, 2, -2, -6 }, 180 },
            { "Dub escuro",      { 2, 3, 2, 1, 0, -1, -3, -5, -7, -9 }, 20 },
            { "Rádio 70s",       { -12, -12, -8, -2, 3, 5, 3, -2, -8, -12 }, 250 },
            { "Ar de fita",      { 1, 2, 1, 0, -1, 0, 1, 2, 1, -3 }, 30 },
        };
        for (size_t k = 0; k < presets.size(); ++k)
        {
            auto& b = *buttons.emplace_back (std::make_unique<TextButton> (HU (presets[k].n)));
            addAndMakeVisible (b);
            b.onClick = [this, k]
            {
                for (int i = 0; i < 10; ++i) setParamValue (proc.apvts, "eq" + String (i), presets[k].g[i]);
                setParamValue (proc.apvts, "tubby", presets[k].tub);
                for (size_t j = 0; j < buttons.size(); ++j) buttons[j]->setToggleState (j == k, dontSendNotification);
            };
        }
    }
    void tick() override { curve.refresh(); }
    void paint (Graphics& g) override
    {
        g.setColour (hc::cream); g.setFont (uiFont (19.0f, true, 0.05f));
        g.drawText (HU ("EQUALIZADOR GRÁFICO · 10 BANDAS"), 20, 6, 700, 24, Justification::left);
        g.setColour (hc::muted); g.setFont (uiFont (12.5f));
        g.drawText (HU ("±12 dB por banda, depois do timbre e antes do Tape Echo. O Filtro Tubby é um passa-altas ressonante para varrer ao vivo."),
                    20, 30, getWidth() - 40, 18, Justification::left);
        groupBox (g, fadersBox, {});
        groupBox (g, tubbyBox, "FILTRO TUBBY");
        g.setColour (hc::muted); g.setFont (uiFont (11.0f, true));
        const char* bands[10] = { "31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "16k" };
        for (int b = 0; b < 10; ++b)
            g.drawText (bands[b], faders[(size_t) b]->getBounds().withY (fadersBox.getBottom() - 22).withHeight (16), Justification::centred);
    }
    void resized() override
    {
        auto row = Rectangle<int> (20, 56, getWidth() - 40, 28);
        const int bw = (row.getWidth() - 5 * 8) / 6;
        for (auto& b : buttons) { b->setBounds (row.removeFromLeft (bw)); row.removeFromLeft (8); }
        curve.setBounds (20, 94, getWidth() - 40, 140);
        fadersBox = { 20, 246, getWidth() - 40 - 220, getHeight() - 256 };
        tubbyBox = { fadersBox.getRight() + 16, 246, 204, getHeight() - 256 };
        const int fw = (fadersBox.getWidth() - 20) / 10;
        for (int b = 0; b < 10; ++b) faders[(size_t) b]->setBounds (fadersBox.getX() + 10 + b * fw, fadersBox.getY() + 6, fw, fadersBox.getHeight() - 30);
        tubby->setBounds (tubbyBox.getX() + 14, tubbyBox.getY() + 40, 88, 110);
        res->setBounds (tubbyBox.getX() + 104, tubbyBox.getY() + 40, 88, 110);
    }
private:
    struct E { const char* n; float g[10]; float tub; };
    std::vector<E> presets;
    EqCurve curve;
    std::vector<std::unique_ptr<Slider>> faders;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> atts;
    std::unique_ptr<KnobCtl> tubby, res;
    std::vector<std::unique_ptr<TextButton>> buttons;
    Rectangle<int> fadersBox, tubbyBox;
};

//==============================================================================
//  FRASES MIDI
//==============================================================================
static String posLabel (int s) { return String (s / 16 + 1) + "." + String ((s % 16) / 4 + 1) + "." + String (s % 4 + 1); }
static const std::array<std::pair<int, const char*>, 8>& durList()
{
    static const std::array<std::pair<int, const char*>, 8> d {{ { 1, "1/16" }, { 2, "1/8" }, { 3, "1/8 pont." }, { 4, "1/4" },
                                                                  { 6, "1/4 pont." }, { 8, "1/2" }, { 12, "1/2 pont." }, { 16, "inteira" } }};
    return d;
}

class PatternCard : public Component
{
public:
    std::function<void()> onClick;
    String name, tag, desc;
    bool selected = false;
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool over = isMouseOver();
        g.setColour (selected ? hc::amber.withAlpha (0.12f) : Colours::black.withAlpha (over ? 0.3f : 0.2f));
        g.fillRoundedRectangle (r, 6);
        g.setColour (selected || over ? hc::amber : hc::line);
        g.drawRoundedRectangle (r, 6, 1);
        g.setColour (hc::cream); g.setFont (uiFont (13.0f, true));
        g.drawText (name, 10, 5, getWidth() - 70, 16, Justification::left);
        g.setColour (hc::amber); g.setFont (uiFont (10.5f, true));
        g.drawText (tag, getWidth() - 62, 5, 54, 16, Justification::right);
        g.setColour (hc::muted); g.setFont (uiFont (11.0f));
        g.drawFittedText (desc, 10, 22, getWidth() - 20, getHeight() - 26, Justification::topLeft, 2);
    }
    void mouseEnter (const MouseEvent&) override { repaint(); }
    void mouseExit (const MouseEvent&) override  { repaint(); }
    void mouseUp (const MouseEvent& e) override  { if (e.mouseWasClicked() && onClick) onClick(); }
};

class PhrasePage;

class EventRow : public Component
{
public:
    EventRow (PhrasePage& o, int i);
    void paint (Graphics& g) override
    {
        if (selected) { g.setColour (hc::amber.withAlpha (0.16f)); g.fillRect (getLocalBounds()); }
        g.setColour (hc::line.withAlpha (0.5f)); g.drawHorizontalLine (getHeight() - 1, 0, (float) getWidth());
        g.setColour (hc::muted); g.setFont (uiFont (12.0f, true));
        g.drawText (String (index + 1), 4, 0, 24, getHeight(), Justification::centred);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (2, 3).withTrimmedLeft (30);
        pos.setBounds (r.removeFromLeft (84)); r.removeFromLeft (8);
        note.setBounds (r.removeFromLeft (150)); r.removeFromLeft (8);
        dur.setBounds (r.removeFromLeft (110)); r.removeFromLeft (8);
        art.setBounds (r.removeFromLeft (110)); r.removeFromLeft (8);
        del.setBounds (r.removeFromLeft (28));
    }
    void mouseDown (const MouseEvent&) override;
    void fill (const horns::Phrase& ph, int key, int scale);
    int index;
    bool selected = false;
    ComboBox pos, note, dur, art;
    TextButton del { "X" };
private:
    PhrasePage& owner;
};

class RollView : public Component
{
public:
    const horns::Phrase* ph = nullptr;
    int selected = -1, playStep = -1, key = 9, scale = 0;
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Colour (0xff120c08)); g.fillRoundedRectangle (r, 6);
        if (ph == nullptr) return;
        const int len = jmax (1, ph->length);
        int lo = 200, hi = -1;
        for (auto& e : ph->events) if (e.note >= 0) { lo = jmin (lo, e.note); hi = jmax (hi, e.note); }
        if (hi < 0) { lo = 60 + key - 5; hi = 60 + key + 12; }
        lo -= 3; hi += 3;
        if (hi - lo < 14) { const int c = (hi + lo) / 2; lo = c - 7; hi = c + 7; }
        const float sw = r.getWidth() / (float) len, rh = r.getHeight() / (float) (hi - lo + 1);
        for (int m = lo; m <= hi; ++m)
        {
            const float y = r.getBottom() - (float) (m - lo + 1) * rh;
            if (horns::inScale (m, key, scale))
            {
                g.setColour (horns::mod12 (m - key) == 0 ? hc::red.withAlpha (0.16f) : hc::green.withAlpha (0.08f));
                g.fillRect (r.getX(), y, r.getWidth(), rh);
            }
        }
        for (int s = 0; s <= len; ++s)
        {
            g.setColour (s % 16 == 0 ? Colour (0xff7a5a3a) : s % 4 == 0 ? Colour (0xff3a2a1e) : Colour (0xff21180f));
            g.fillRect (r.getX() + (float) s * sw, r.getY(), s % 16 == 0 ? 2.0f : 1.0f, r.getHeight());
        }
        for (int i = 0; i < (int) ph->events.size(); ++i)
        {
            const auto& e = ph->events[(size_t) i];
            const int m = e.note < 0 ? (lo + hi) / 2 : e.note;
            const float y = r.getBottom() - (float) (m - lo + 1) * rh;
            Colour c = e.note < 0 ? hc::red.withAlpha (0.6f) : i == selected ? Colour (0xffffd27a) : hc::amber;
            if (playStep >= e.start && playStep < e.start + e.dur) c = Colour (0xfffff3d0);
            g.setColour (c);
            g.fillRoundedRectangle ((float) e.start * sw + 1, y + 1, jmax (3.0f, (float) e.dur * sw - 2), rh - 2, 2);
            if (e.note < 0) { g.setColour (Colours::white); g.setFont (uiFont (10.0f, true)); g.drawText ("?", (int) ((float) e.start * sw) + 3, (int) y, 12, (int) rh, Justification::centred); }
        }
        if (playStep >= 0) { g.setColour (Colours::white.withAlpha (0.22f)); g.fillRect ((float) playStep * sw, r.getY(), sw, r.getHeight()); }
        g.setColour (Colours::black); g.drawRoundedRectangle (r.reduced (0.5f), 6, 1);
    }
};

// arrasta o .mid da frase direto para a timeline da DAW
class MidiDrag : public Component
{
public:
    explicit MidiDrag (HornsProcessor& p) : proc (p) { setMouseCursor (MouseCursor::DraggingHandCursor); setRepaintsOnMouseActivity (true); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1);
        g.setColour (isMouseOver() ? hc::amber.withAlpha (0.2f) : Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (r, 5);
        g.setColour (hc::amber);
        float dl[] = { 4, 3 };
        Path p, dashed; p.addRoundedRectangle (r, 5);
        PathStrokeType (1.2f).createDashedStroke (dashed, p, dl, 2);
        g.fillPath (dashed);
        g.setFont (uiFont (11.5f, true, 0.06f));
        g.drawFittedText ("ARRASTE O MIDI\nPARA A DAW", getLocalBounds(), Justification::centred, 2);
    }
    void mouseDrag (const MouseEvent&) override
    {
        if (dragging) return;
        dragging = true;
        auto f = File::getSpecialLocation (File::tempDirectory).getChildFile ("Mastor Dub Horns - frase.mid");
        if (proc.writeMidiFile (f))
            DragAndDropContainer::performExternalDragDropOfFiles (StringArray (f.getFullPathName()), false, this);
    }
    void mouseUp (const MouseEvent&) override { dragging = false; }
private:
    HornsProcessor& proc;
    bool dragging = false;
};

class PhrasePage : public HornsPage, private MidiKeyboardState::Listener
{
public:
    explicit PhrasePage (HornsProcessor& p) : HornsPage (p),
        play (p.apvts, "phrasePlay", HU ("▶  TOCAR FRASE"), HU ("■  PARAR")), guide (p.apvts, "guide", "GUIA ONE DROP"), drag (p)
    {
        ph = proc.getPhrase();
        lastKey = key(); lastScale = scale();

        for (auto* b : { &libStab, &libIntro }) { b->setRadioGroupId (77); b->setClickingTogglesState (true); addAndMakeVisible (*b); }
        libStab.setToggleState (true, dontSendNotification);
        libStab.onClick = [this] { if (libStab.getToggleState()) { lib = 0; refreshLib(); } };
        libIntro.onClick = [this] { if (libIntro.getToggleState()) { lib = 1; refreshLib(); } };
        fillMode.addItem ("sugerir notas no tom", 1);
        fillMode.addItem (HU ("só ritmo (eu escolho as notas)"), 2);
        fillMode.setSelectedId ((int) proc.apvts.state.getProperty ("fillMode", 1), dontSendNotification);
        fillMode.onChange = [this] { proc.apvts.state.setProperty ("fillMode", fillMode.getSelectedId(), nullptr); };
        addAndMakeVisible (fillMode);
        for (int i = 0; i < 8; ++i)
        {
            auto& c = *cards.emplace_back (std::make_unique<PatternCard>());
            c.onClick = [this, i] { loadPattern (i); };
            addChildComponent (c);
        }
        refreshLib();

        play.getProperties().set ("tint", "green");
        addAndMakeVisible (play); addAndMakeVisible (guide);
        bpm = std::make_unique<KnobCtl> (p.apvts, "bpm", "BPM interno");
        swing = std::make_unique<KnobCtl> (p.apvts, "swing", "Swing");
        vel = std::make_unique<KnobCtl> (p.apvts, "phraseVel", HU ("Força"));
        for (auto* k : { bpm.get(), swing.get(), vel.get() }) addAndMakeVisible (*k);

        pick.setClickingTogglesState (true);
        pick.getProperties().set ("tint", "red");
        pick.onClick = [this] { setPicking (pick.getToggleState()); };
        addBtn.onClick = [this] { addEvent(); };
        reapply.onClick = [this]
        {
            for (auto& e : ph.events) e.note = horns::degreeToMidi (e.degree, key(), scale());
            push (true);
        };
        clearBtn.onClick = [this]
        {
            for (auto& e : ph.events) e.note = -1;
            selected = ph.events.empty() ? -1 : 0;
            push (true); setPicking (true);
        };
        exportBtn.onClick = [this] { exportMidi(); };
        for (auto* b : { &pick, &addBtn, &reapply, &clearBtn, &exportBtn }) addAndMakeVisible (*b);
        addAndMakeVisible (drag);
        addAndMakeVisible (status);
        status.setFont (uiFont (12.0f, true));
        status.setColour (Label::textColourId, hc::muted);

        roll.ph = &ph;
        addAndMakeVisible (roll);
        view.setViewedComponent (&list, false);
        view.setScrollBarsShown (true, false);
        addAndMakeVisible (view);
        rebuildRows();
        proc.keyboardState.addListener (this);
    }
    ~PhrasePage() override { proc.keyboardState.removeListener (this); }

    int key() const   { return (int) proc.apvts.getRawParameterValue ("key")->load(); }
    int scale() const { return (int) proc.apvts.getRawParameterValue ("scale")->load(); }

    void tick() override
    {
        int n;
        while (proc.popPickedNote (n)) if (picking) assignNote (n);
        const int ps = proc.getPlayStep();
        if (ps != roll.playStep) { roll.playStep = ps; roll.repaint(); }
        if (key() != lastKey || scale() != lastScale)
        {
            lastKey = key(); lastScale = scale();
            roll.key = lastKey; roll.scale = lastScale;
            for (auto& r : rows) r->fill (ph, lastKey, lastScale);
            roll.repaint();
        }
        const bool host = proc.isHostPlaying();
        const auto txt = picking ? (selected >= 0 ? HU ("teclado: escolhendo a nota da linha ") + String (selected + 1) : HU ("teclado: escolhendo notas"))
                                 : host ? HU ("seguindo o transporte da DAW") : HU ("teclado: tocar · DAW parada = clock interno");
        if (status.getText() != txt) status.setText (txt, dontSendNotification);
    }

    void paint (Graphics& g) override
    {
        g.setColour (hc::cream); g.setFont (uiFont (19.0f, true, 0.05f));
        g.drawText (HU ("SUGESTÕES MIDI · ATAQUES & INTRODUÇÕES"), 20, 6, 700, 24, Justification::left);
        g.setColour (hc::muted); g.setFont (uiFont (12.5f));
        g.drawText (HU ("As sugestões trazem ritmo e articulação. As notas são suas: escolha em cada linha ou ligue ESCOLHER PELO TECLADO e toque em sequência."),
                    20, 30, getWidth() - 40, 18, Justification::left);
        g.setFont (uiFont (11.0f, true));
        g.drawText ("AO CARREGAR:", 20, fillMode.getY(), 90, fillMode.getHeight(), Justification::left);
        g.setColour (hc::muted); g.setFont (uiFont (10.0f, true, 0.12f));
        auto hdr = view.getBounds().translated (0, -16).withHeight (14).withTrimmedLeft (32);
        g.drawText (HU ("POSIÇÃO"), hdr.removeFromLeft (92), Justification::left);
        g.drawText ("NOTA", hdr.removeFromLeft (158), Justification::left);
        g.drawText (HU ("DURAÇÃO"), hdr.removeFromLeft (118), Justification::left);
        g.drawText (HU ("ARTICULAÇÃO"), hdr.removeFromLeft (118), Justification::left);
        g.setColour (hc::amber); g.setFont (uiFont (12.0f, true));
        g.drawText (String (ph.name.empty() ? "nenhuma frase" : String::fromUTF8 (ph.name.c_str())), roll.getX(), roll.getY() - 18, 500, 16, Justification::left);
    }

    void resized() override
    {
        const int L = 20, LW = 300, R = L + LW + 18, RW = getWidth() - R - 20;
        libStab.setBounds (L, 56, 146, 28); libIntro.setBounds (L + 154, 56, 146, 28);
        fillMode.setBounds (L + 94, 92, LW - 94, 24);
        for (int i = 0; i < 8; ++i) cards[(size_t) i]->setBounds (L, 124 + i * 41, LW, 37);

        play.setBounds (R, 60, 170, 34);
        guide.setBounds (R + 178, 60, 130, 34);
        bpm->setBounds (R + 322, 48, 80, 76); swing->setBounds (R + 404, 48, 80, 76); vel->setBounds (R + 486, 48, 80, 76);
        drag.setBounds (R + RW - 126, 52, 126, 64);
        auto row = Rectangle<int> (R, 132, RW, 28);
        pick.setBounds (row.removeFromLeft (200)); row.removeFromLeft (8);
        addBtn.setBounds (row.removeFromLeft (80)); row.removeFromLeft (8);
        reapply.setBounds (row.removeFromLeft (150)); row.removeFromLeft (8);
        clearBtn.setBounds (row.removeFromLeft (120)); row.removeFromLeft (8);
        exportBtn.setBounds (row.removeFromLeft (130));
        status.setBounds (R, 164, RW, 18);
        roll.setBounds (R, 204, RW, 96);
        view.setBounds (R, 322, RW, getHeight() - 328);
        layoutRows();
    }

    // chamado pelas linhas
    void rowSelected (int i) { selected = i; for (auto& r : rows) { r->selected = r->index == i; r->repaint(); } roll.selected = i; roll.repaint(); }
    void rowEdited (int i)
    {
        auto& r = *rows[(size_t) i];
        auto& e = ph.events[(size_t) i];
        const int oldStart = e.start;
        e.start = r.pos.getSelectedId() - 1;
        e.note = r.note.getSelectedId() <= 1 ? -1 : r.note.getSelectedId() - 2;
        e.dur = r.dur.getSelectedId();
        e.artic = r.art.getSelectedId() - 1;
        selected = i;
        if (e.note >= 0) proc.previewNote (e.note, e.artic);
        push (e.start != oldStart);
    }
    void rowDeleted (int i)
    {
        ph.events.erase (ph.events.begin() + i);
        selected = jmin (selected, (int) ph.events.size() - 1);
        push (true);
    }

private:
    void handleNoteOn (MidiKeyboardState*, int, int note, float) override
    {
        if (! MessageManager::existsAndIsCurrentThread()) return;  // notas da DAW chegam pelo fifo
        if (picking) assignNote (note);
    }
    void handleNoteOff (MidiKeyboardState*, int, int, float) override {}

    void refreshLib()
    {
        const auto& defs = lib == 0 ? horns::stabPatterns() : horns::introPatterns();
        for (int i = 0; i < 8; ++i)
        {
            auto& c = *cards[(size_t) i];
            c.setVisible (i < (int) defs.size());
            if (i < (int) defs.size())
            {
                c.name = HU (defs[(size_t) i].name); c.tag = defs[(size_t) i].tag; c.desc = HU (defs[(size_t) i].desc);
                c.selected = String::fromUTF8 (ph.name.c_str()) == c.name;
                c.repaint();
            }
        }
    }

    void loadPattern (int i)
    {
        const auto& defs = lib == 0 ? horns::stabPatterns() : horns::introPatterns();
        if (i >= (int) defs.size()) return;
        const bool suggest = fillMode.getSelectedId() != 2;
        ph = horns::makePhrase (defs[(size_t) i], suggest, key(), scale());
        selected = suggest ? -1 : 0;
        push (true);
        refreshLib();
        setPicking (! suggest);
    }

    void addEvent()
    {
        if ((int) ph.events.size() >= horns::kMaxEvents) return;
        int s = 0;
        if (! ph.events.empty()) { const auto& l = ph.events.back(); s = jmin (ph.length - 1, l.start + l.dur); }
        if (ph.name.empty()) ph.name = "Minha frase";
        horns::PhraseEvent e; e.start = s; e.dur = 2; e.note = -1;
        ph.events.push_back (e);
        horns::sortEvents (ph);
        for (int k = 0; k < (int) ph.events.size(); ++k) if (ph.events[(size_t) k].note < 0 && ph.events[(size_t) k].start == s) selected = k;
        push (true);
        setPicking (true);
    }

    void assignNote (int note)
    {
        if (selected < 0 || selected >= (int) ph.events.size()) return;
        ph.events[(size_t) selected].note = note;
        int next = -1;
        for (int k = selected + 1; k < (int) ph.events.size(); ++k) if (ph.events[(size_t) k].note < 0) { next = k; break; }
        if (next < 0 && selected + 1 < (int) ph.events.size()) next = selected + 1;
        selected = next;
        push (false);
        if (selected < 0) setPicking (false);
    }

    void setPicking (bool on)
    {
        picking = on;
        pick.setToggleState (on, dontSendNotification);
        if (on && selected < 0 && ! ph.events.empty())
        {
            selected = 0;
            for (int k = 0; k < (int) ph.events.size(); ++k) if (ph.events[(size_t) k].note < 0) { selected = k; break; }
        }
        rowSelected (selected);
    }

    void push (bool structural)
    {
        horns::sortEvents (ph);
        proc.setPhrase (ph);
        if (structural || (int) rows.size() != (int) ph.events.size()) rebuildRows();
        else for (auto& r : rows) r->fill (ph, key(), scale());
        rowSelected (selected);
        repaint (roll.getBounds().translated (0, -20).withHeight (20));
    }

    void rebuildRows()
    {
        rows.clear();
        for (int i = 0; i < (int) ph.events.size(); ++i)
        {
            auto& r = *rows.emplace_back (std::make_unique<EventRow> (*this, i));
            r.fill (ph, key(), scale());
            r.selected = i == selected;
            list.addAndMakeVisible (r);
        }
        layoutRows();
        roll.repaint();
    }
    void layoutRows()
    {
        const int w = jmax (100, view.getWidth() - view.getScrollBarThickness());
        list.setSize (w, jmax (1, (int) rows.size() * 32));
        for (int i = 0; i < (int) rows.size(); ++i) rows[(size_t) i]->setBounds (0, i * 32, w, 32);
    }

    void exportMidi()
    {
        chooser = std::make_unique<FileChooser> ("Salvar a frase como MIDI",
            File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Mastor Dub Horns - frase.mid"), "*.mid");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
            [this] (const FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (f != File()) proc.writeMidiFile (f.withFileExtension ("mid"));
            });
    }

    horns::Phrase ph;
    int selected = -1, lib = 0, lastKey = -1, lastScale = -1;
    bool picking = false;
    TextButton libStab { "ATAQUES CURTOS" }, libIntro { HU ("INTRODUÇÕES") };
    ComboBox fillMode;
    std::vector<std::unique_ptr<PatternCard>> cards;
    ParamToggle play, guide;
    std::unique_ptr<KnobCtl> bpm, swing, vel;
    TextButton pick { "ESCOLHER PELO TECLADO" }, addBtn { "+ NOTA" }, reapply { "REAPLICAR NO TOM" }, clearBtn { "LIMPAR NOTAS" }, exportBtn { "EXPORTAR .MID" };
    MidiDrag drag;
    Label status;
    RollView roll;
    Viewport view;
    Component list;
    std::vector<std::unique_ptr<EventRow>> rows;
    std::unique_ptr<FileChooser> chooser;
};

EventRow::EventRow (PhrasePage& o, int i) : index (i), owner (o)
{
    for (auto* c : { &pos, &note, &dur, &art }) { addAndMakeVisible (*c); c->onChange = [this] { owner.rowEdited (index); }; }
    addAndMakeVisible (del);
    del.onClick = [this] { owner.rowDeleted (index); };
}

void EventRow::mouseDown (const MouseEvent&) { owner.rowSelected (index); }

void EventRow::fill (const horns::Phrase& ph, int key, int scale)
{
    const auto& e = ph.events[(size_t) index];
    pos.clear (dontSendNotification);
    for (int s = 0; s < ph.length; ++s) pos.addItem (posLabel (s), s + 1);
    pos.setSelectedId (e.start + 1, dontSendNotification);
    note.clear (dontSendNotification);
    note.addItem (HU ("— escolha —"), 1);
    for (int m = 86; m >= 36; --m)
    {
        String t = String (horns::midiName (m));
        if (horns::mod12 (m - key) == 0) t << HU ("   ● tônica");
        else if (horns::inScale (m, key, scale)) t << HU ("   •");
        note.addItem (t, m + 2);
    }
    note.setSelectedId (e.note < 0 ? 1 : e.note + 2, dontSendNotification);
    dur.clear (dontSendNotification);
    for (const auto& d : durList()) dur.addItem (d.second, d.first);
    if (dur.indexOfItemId (e.dur) < 0) dur.addItem (String (e.dur) + "/16", e.dur);
    dur.setSelectedId (e.dur, dontSendNotification);
    art.clear (dontSendNotification);
    for (int a = 0; a < horns::kNumArtic; ++a) art.addItem (horns::articName (a), a + 1);
    art.setSelectedId (e.artic + 1, dontSendNotification);
    if (e.note < 0) note.getProperties().set ("need", true); else note.getProperties().remove ("need");
    repaint();
}

//==============================================================================
//  VU, changelog, painel
//==============================================================================
void VuMeter::setLevel (float rms)
{
    const float db = 20.0f * std::log10 (rms + 1.0e-6f) + 12.0f;
    needle += (jlimit (-22.0f, 4.0f, db) - needle) * 0.25f;
    repaint();
}

void VuMeter::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colours::black); g.fillRoundedRectangle (r, 6);
    auto face = r.reduced (2);
    g.setGradientFill (ColourGradient (Colour (0xfff4e9cf), face.getCentreX(), face.getY(), Colour (0xffd9c79f), face.getCentreX(), face.getBottom(), false));
    g.fillRoundedRectangle (face, 5);
    const float cx = face.getCentreX(), cy = face.getBottom() + 22, rad = face.getHeight() + 6;
    auto ang = [] (float db) { return degreesToRadians (-50.0f + (db + 20.0f) / 23.0f * 100.0f); };
    Path scale; scale.addCentredArc (cx, cy, rad - 14, rad - 14, 0, ang (-20), ang (0), true);
    g.setColour (Colour (0xff3a2a1e)); g.strokePath (scale, PathStrokeType (1.2f));
    Path redz; redz.addCentredArc (cx, cy, rad - 14, rad - 14, 0, ang (0), ang (3), true);
    g.setColour (hc::red); g.strokePath (redz, PathStrokeType (3.0f));
    g.setFont (uiFont (8.5f, true));
    for (auto [db, t] : { std::pair<float, const char*> { -20.0f, "-20" }, { -10.0f, "-10" }, { -7.0f, "-7" }, { -5.0f, "-5" }, { -3.0f, "-3" }, { 0.0f, "0" }, { 3.0f, "+3" } })
    {
        const float a = ang (db);
        g.setColour (db >= 0 ? hc::red : Colour (0xff3a2a1e));
        g.drawLine (cx + std::sin (a) * (rad - 18), cy - std::cos (a) * (rad - 18), cx + std::sin (a) * (rad - 11), cy - std::cos (a) * (rad - 11), 1.0f);
        g.drawText (t, (int) (cx + std::sin (a) * (rad - 3)) - 10, (int) (cy - std::cos (a) * (rad - 3)) - 6, 20, 10, Justification::centred);
    }
    g.setColour (Colour (0xff3a2a1e)); g.setFont (uiFont (10.0f, true));
    g.drawText ("VU", face.withTrimmedTop (face.getHeight() - 22).toNearestInt(), Justification::centred);
    const float a = ang (needle);
    g.setColour (hc::ink);
    g.drawLine (cx, cy, cx + std::sin (a) * (rad - 8), cy - std::cos (a) * (rad - 8), 1.6f);
}

static const char* kChangelog =
    "v1.2 · 23/11/2026\n"
    "• Final de nota natural: fall e doit seguram a nota e caem junto com o volume (sem sirene robótica). Fim normal com o lábio relaxando.\n"
    "• Vibrato humano que para ao soltar a nota, afinação viva, golpe de língua, knobs Rouco e Sala.\n"
    "• Aba MÚSICOS: 24 presets no padrão da Riddim Machine (Dean Fraser, Vin Gordon, Tommy McCook, Bobby Ellis...).\n\n"
    "v1.1 · 21/11/2026\n"
    "• Agora é plugin VST3 (Windows e Mac) e AU (Mac), com app standalone, publicado no GitHub e no site do Dub Station.\n"
    "• Tape Echo: a realimentação não dispara mais sozinha (saturação com ganho unitário).\n"
    "• 5ª abaixo corrigida (era uma 4ª). Saturação do barramento não aumenta mais o volume.\n"
    "• Frases: exportar ou arrastar o MIDI para a DAW, uma trilha por instrumento já harmonizada.\n"
    "• Pedal de sustain (CC64) do teclado MIDI faz o Dub Throw.\n\n"
    "v1.0 · 21/11/2026\n"
    "• Primeira versão (no navegador): naipe com 6 instrumentos, timbres anos 70, Mastor Tape Echo, "
    "EQ gráfico de 10 bandas com Filtro Tubby, 8 ataques curtos e 6 introduções com notas escolhidas pelo usuário.";

ChangelogView::ChangelogView()
{
    addAndMakeVisible (close);
    close.onClick = [this] { setVisible (false); };
}

void ChangelogView::paint (Graphics& g)
{
    g.fillAll (Colours::black.withAlpha (0.7f));
    auto box = getLocalBounds().withSizeKeepingCentre (720, 440);
    g.setColour (hc::plate); g.fillRoundedRectangle (box.toFloat(), 10);
    g.setColour (hc::amber); g.drawRoundedRectangle (box.toFloat(), 10, 1.5f);
    g.setFont (uiFont (20.0f, true, 0.05f));
    g.drawText (HU ("HISTÓRICO DE VERSÕES"), box.reduced (22, 16).removeFromTop (26), Justification::left);
    g.setColour (hc::cream); g.setFont (uiFont (13.0f));
    g.drawFittedText (HU (kChangelog), box.reduced (22, 16).withTrimmedTop (36).withTrimmedBottom (44), Justification::topLeft, 28);
}

void ChangelogView::resized()
{
    auto box = getLocalBounds().withSizeKeepingCentre (720, 440);
    close.setBounds (box.getX() + 22, box.getBottom() - 46, 100, 30);
}

HornsPanel::HornsPanel (HornsProcessor& p)
    : proc (p), keyBox (p.apvts, "key"), scaleBox (p.apvts, "scale"),
      keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (keyBox); addAndMakeVisible (scaleBox);
    master = std::make_unique<KnobCtl> (p.apvts, "master", "Master");
    addAndMakeVisible (*master);
    addAndMakeVisible (vu);
    versionBtn.setButtonText ("v" MASTOR_HORNS_VERSION);
    versionBtn.setTooltip (HU ("Histórico de versões"));
    versionBtn.onClick = [this] { changelog.setVisible (true); changelog.toFront (false); };
    addAndMakeVisible (versionBtn);

    const char* names[6] = { "NAIPE", "TIMBRE '70s", "MASTOR TAPE ECHO", "EQUALIZADOR", "FRASES MIDI", "MÚSICOS" };
    for (int i = 0; i < 6; ++i)
    {
        auto& t = tabs[(size_t) i];
        t.setButtonText (HU (names[i]));
        t.setRadioGroupId (1);
        t.setClickingTogglesState (true);
        t.onClick = [this, i] { if (tabs[(size_t) i].getToggleState()) showPage (i); };
        addAndMakeVisible (t);
    }
    pages.push_back (std::make_unique<NaipePage> (p));
    pages.push_back (std::make_unique<TimbrePage> (p));
    pages.push_back (std::make_unique<EchoPage> (p));
    pages.push_back (std::make_unique<EqPage> (p));
    pages.push_back (std::make_unique<PhrasePage> (p));
    pages.push_back (std::make_unique<MusicosPage> (p));
    for (auto& pg : pages) addChildComponent (*pg);

    keyboard.setAvailableRange (36, 96);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setColour (MidiKeyboardComponent::whiteNoteColourId, Colour (0xfff3ead4));
    keyboard.setColour (MidiKeyboardComponent::blackNoteColourId, Colour (0xff1a130e));
    keyboard.setColour (MidiKeyboardComponent::keySeparatorLineColourId, Colour (0xff6a5a44));
    keyboard.setColour (MidiKeyboardComponent::keyDownOverlayColourId, hc::amber);
    keyboard.setColour (MidiKeyboardComponent::mouseOverKeyOverlayColourId, hc::amber.withAlpha (0.3f));
    keyboard.setColour (MidiKeyboardComponent::shadowColourId, Colours::black.withAlpha (0.3f));
    keyboard.setColour (MidiKeyboardComponent::upDownButtonBackgroundColourId, hc::chassisDark);
    keyboard.setColour (MidiKeyboardComponent::upDownButtonArrowColourId, hc::amber);
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    addChildComponent (changelog);
    setSize (W, H);
    showPage ((int) proc.apvts.state.getProperty ("page", 0));
    startTimerHz (30);
}

HornsPanel::~HornsPanel()
{
    stopTimer();
    pages.clear();
    setLookAndFeel (nullptr);
}

void HornsPanel::showPage (int i)
{
    page = jlimit (0, (int) pages.size() - 1, i);
    for (int k = 0; k < (int) pages.size(); ++k) pages[(size_t) k]->setVisible (k == page);
    tabs[(size_t) page].setToggleState (true, dontSendNotification);
    proc.apvts.state.setProperty ("page", page, nullptr);
}

void HornsPanel::timerCallback()
{
    vu.setLevel (proc.getVuLevel());
    if (auto* pg = pages[(size_t) page].get()) pg->tick();
}

void HornsPanel::paint (Graphics& g)
{
    // chassi e laterais de madeira
    g.fillAll (hc::chassis);
    auto wood = [&] (Rectangle<int> r)
    {
        g.setGradientFill (ColourGradient (Colour (0xff5b3519), (float) r.getX(), 0, Colour (0xff4a2a12), (float) r.getRight(), 0, false));
        g.fillRect (r);
        g.setColour (Colours::black.withAlpha (0.18f));
        for (int x = r.getX(); x < r.getRight(); x += 5) g.drawVerticalLine (x, (float) r.getY(), (float) r.getBottom());
        g.setColour (Colour (0xff8a5428).withAlpha (0.35f));
        g.fillRect (r.withTrimmedLeft (r.getWidth() / 3).withWidth (r.getWidth() / 4));
    };
    wood ({ 0, 0, 22, H }); wood ({ W - 22, 0, 22, H });

    // cabeçalho
    g.setGradientFill (ColourGradient (Colour (0xff3b2a1c), 0, 0, Colour (0xff291d13), 0, (float) header.getBottom(), false));
    g.fillRect (header);
    g.setColour (Colours::black); g.fillRect (header.getX(), header.getBottom() - 2, header.getWidth(), 2);
    g.setGradientFill (ColourGradient (Colour (0xffffd27a), 0, 18, Colour (0xffc8702a), 0, 52, false));
    g.setFont (uiFont (32.0f, true, 0.06f));
    g.drawText ("MASTOR DUB HORNS", header.getX() + 22, 14, 420, 38, Justification::left);
    g.setColour (hc::muted); g.setFont (uiFont (11.0f, true, 0.3f));
    g.drawText (HU ("DUB STATION · HORN SECTION · MDH-70"), header.getX() + 24, 52, 400, 14, Justification::left);
    const Colour stripes[3] = { hc::red, hc::gold, hc::green };
    for (int i = 0; i < 3; ++i) { g.setColour (stripes[i]); g.fillRect (header.getX() + 24 + i * 42, 70, 42, 6); }
    // caixa do tom
    auto kb = Rectangle<int> (keyBox.getX() - 52, keyBox.getY() - 6, scaleBox.getRight() - keyBox.getX() + 62, keyBox.getHeight() + 12);
    g.setColour (hc::chassisDark); g.fillRoundedRectangle (kb.toFloat(), 6);
    g.setColour (hc::line); g.drawRoundedRectangle (kb.toFloat(), 6, 1);
    g.setColour (hc::muted); g.setFont (uiFont (10.0f, true, 0.2f));
    g.drawText ("TOM", kb.getX() + 10, kb.getY(), 40, kb.getHeight(), Justification::left);
    g.drawText ("ESCALA", scaleBox.getX() - 58, kb.getY(), 54, kb.getHeight(), Justification::left);

    // barra das abas e área da página
    g.setColour (hc::chassisDark); g.fillRect (tabBar);
    g.setGradientFill (ColourGradient (hc::plate, 0, (float) pageArea.getY(), hc::plate2, 0, (float) pageArea.getBottom(), false));
    g.fillRect (pageArea);
    // barra do teclado
    g.setColour (hc::chassisDark); g.fillRect (kbBar);
    g.setColour (Colours::black); g.fillRect (kbBar.getX(), kbBar.getY(), kbBar.getWidth(), 2);
    g.setColour (hc::muted); g.setFont (uiFont (12.0f));
    g.drawText (HU ("Toque no teclado, use o MIDI da DAW ou um controlador. Pedal de sustain = Dub Throw. Uma nota toca o naipe inteiro com as harmonias."),
                kbBar.reduced (16, 0), Justification::centredLeft);
    g.setColour (Colour (0xff0d0906)); g.fillRect (kbBar.getX(), kbBar.getBottom(), kbBar.getWidth(), H - kbBar.getBottom());
}

void HornsPanel::resized()
{
    const int x0 = 22, w = W - 44;
    header = { x0, 0, w, 96 };
    tabBar = { x0, 96, w, 40 };
    pageArea = { x0, 136, w, 468 };
    kbBar = { x0, 604, w, 34 };

    keyBox.setBounds (x0 + 520, 34, 120, 28);
    scaleBox.setBounds (x0 + 712, 34, 170, 28);
    versionBtn.setBounds (x0 + 452, 34, 52, 26);
    vu.setBounds (x0 + w - 270, 12, 150, 74);
    master->setBounds (x0 + w - 106, 6, 84, 86);
    for (int i = 0; i < 6; ++i) tabs[(size_t) i].setBounds (x0 + 14 + i * 172, 102, 166, 32);
    for (auto& pg : pages) pg->setBounds (pageArea);
    keyboard.setBounds (x0 + 14, 640, w - 28, 150);
    keyboard.setKeyWidth ((float) (w - 28 - 40) / 36.0f);
    keyboard.setLowestVisibleKey (48);
    changelog.setBounds (getLocalBounds());
}
