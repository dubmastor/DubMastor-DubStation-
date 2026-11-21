// Visual "Dub Station" do Mastor Fone: o mesmo acabamento do Dub Station 2.1 (FitaEco TE-12)
#include "PluginEditor.h"

using namespace juce;

namespace
{
const Colour cream   (0xffe9e2cc);
const Colour ink     (0xff3a2a1c);   // tinta marrom impressa no esmalte
const Colour amber   (0xffffa630);
const Colour lampOff (0xff4a3418);

constexpr int kWood = 26;
constexpr int kTopH = 212;

// escala do VU: linear em tensão de -20 VU a +3 VU, ponteiro abrindo ±42°
constexpr float kVuSwing = 0.7330f;
float vuToPos (float vu)
{
    const float a = std::pow (10.0f, jlimit (-30.0f, 8.0f, vu) / 20.0f);
    return (a - 0.1f) / (1.4125f - 0.1f);
}

const int headMasks[] = { 1, 2, 4, 3, 6, 5, 7 };

Font bold (float h) { return Font (FontOptions (h, Font::bold)); }
} // namespace

//==============================================================================
StationLookAndFeel::StationLookAndFeel()
{
    setColour (Label::textColourId, cream);
}

void StationLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                           float a0, float a1, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const bool stepped = s.getInterval() >= 1.0;
    const auto labels = StringArray::fromTokens (s.getProperties()["scaleLabels"].toString(), "|", "");
    const int steps = stepped ? (int) (s.getMaximum() - s.getMinimum()) + 1 : (labels.isEmpty() ? 11 : labels.size());
    const int sel = roundToInt (pos * (float) (steps - 1));
    const float ang = stepped ? a0 + (float) sel / (float) (steps - 1) * (a1 - a0) : a0 + pos * (a1 - a0);

    // escala impressa no painel
    const bool big = r > 50.0f;
    for (int i = 0; i < steps; ++i)
    {
        const float t = a0 + (float) i / (float) (steps - 1) * (a1 - a0);
        const bool lit = stepped && i == sel;
        g.setColour (lit ? amber : cream.withAlpha (0.8f));
        g.drawLine ({ c.getPointOnCircumference (r * 0.68f, t), c.getPointOnCircumference (r * 0.76f, t) }, 1.3f);

        String txt;
        if (labels.size() == steps)                txt = labels[i];
        else if (! stepped && (big ? i % 2 == 0 : (i == 0 || i == steps - 1 || i == steps / 2))) txt = String (i);
        if (txt.isNotEmpty() && r > 30.0f)
        {
            const auto np = c.getPointOnCircumference (r * (labels.isEmpty() ? 0.88f : 0.9f), t);
            const float bw = labels.isEmpty() ? r * 0.3f : r * 1.05f;
            g.setFont (bold (labels.isEmpty() ? jmax (9.0f, r * 0.15f) : jmax (9.0f, r * 0.115f)));
            g.drawText (txt, Rectangle<float> (np.x - bw / 2, np.y - 8, bw, 16), Justification::centred);
        }
    }

    // saia de alumínio torneado
    const float kr = r * 0.62f;
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - kr + 2, c.y - kr + 4, kr * 2, kr * 2);
    g.setGradientFill (ColourGradient (Colour (0xffeeeeee), c.x - kr, c.y - kr, Colour (0xff7d7d7d), c.x + kr, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    for (int i = 0; i < 6; ++i)
    {
        const float rr = kr * (0.72f + 0.045f * (float) i);
        g.setColour (Colours::white.withAlpha (i % 2 ? 0.10f : 0.0f).overlaidWith (Colours::black.withAlpha (i % 2 ? 0.0f : 0.06f)));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 1.0f);
    }
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.2f);
    g.setColour (stepped ? amber.darker (0.2f) : Colour (0xff111111));
    g.drawLine ({ c.getPointOnCircumference (kr * 0.70f, ang), c.getPointOnCircumference (kr * 0.98f, ang) }, jmax (1.8f, r * 0.03f));

    // tampa preta frisada
    const float cr = kr * 0.68f;
    g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), c.x, c.y - cr, Colour (0xff070707), c.x, c.y + cr, false));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    const int ribs = big ? 30 : 20;
    g.setColour (Colours::white.withAlpha (0.07f));
    for (int i = 0; i < ribs; ++i)
    {
        const float t = (float) i / (float) ribs * MathConstants<float>::twoPi;
        g.drawLine ({ c.getPointOnCircumference (cr * 0.86f, t), c.getPointOnCircumference (cr * 0.99f, t) }, 1.0f);
    }
    g.setGradientFill (ColourGradient (Colour (0xff2c2c2c), c.x, c.y - cr * 0.8f, Colour (0xff0e0e0e), c.x, c.y + cr * 0.8f, false));
    g.fillEllipse (c.x - cr * 0.84f, c.y - cr * 0.84f, cr * 1.68f, cr * 1.68f);
    g.setColour (cream);
    g.drawLine ({ c.getPointOnCircumference (cr * 0.35f, ang), c.getPointOnCircumference (cr * 0.80f, ang) }, jmax (2.0f, r * 0.04f));
}

void StationLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool down)
{
    if ((bool) b.getProperties()["big"])
    {
        // tecla grande de BYPASS: acende vermelho quando o efeito está desligado
        const bool on = b.getToggleState();
        auto r = b.getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (r.translated (1.5f, 3.0f), 6.0f);
        g.setGradientFill (ColourGradient (Colour (0xffeaeaea), r.getX(), r.getY(), Colour (0xff5a5a5a), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 6.0f);
        auto cap = r.reduced (3.0f).translated (0, down ? 1.0f : 0.0f);
        g.setGradientFill (ColourGradient (Colour (on ? 0xffe0442a : (over ? 0xff36322e : 0xff2a2724)), 0, cap.getY(),
                                           Colour (on ? 0xff7a1a0e : 0xff0c0b0a), 0, cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 4.5f);
        if (on)
        {
            g.setColour (Colour (0x55ff6a3a));
            g.drawRoundedRectangle (r.expanded (2.0f), 8.0f, 3.0f);
        }
        const auto lp = Point<float> (cap.getX() + 14.0f, cap.getCentreY());
        g.setColour (on ? Colour (0xffffd0a0) : Colour (0xff4a1612));
        g.fillEllipse (lp.x - 4.5f, lp.y - 4.5f, 9, 9);
        g.setColour (cream);
        g.setFont (bold (13.0f).withExtraKerningFactor (0.12f));
        g.drawText (b.getButtonText(), cap.withTrimmedLeft (22.0f), Justification::centred);
        return;
    }
    // tecla de baquelite com lâmpada âmbar, como as dos cabeçotes do Dub Station
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (1.0f, 2.0f), 4.0f);
    const float press = down ? 1.0f : 0.0f;
    r = r.translated (0, press);
    g.setGradientFill (ColourGradient (Colour (over ? 0xff3a3633 : 0xff2e2b28), 0, r.getY(), Colour (0xff0d0c0b), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 4.0f);
    g.setGradientFill (ColourGradient (Colour (0xffd8d8d8), r.getX(), r.getY(), Colour (0xff5c5c5c), r.getRight(), r.getBottom(), false));
    g.drawRoundedRectangle (r, 4.0f, 1.6f);

    const auto lp = Point<float> (r.getX() + 13.0f, r.getCentreY());
    if (on)
    {
        g.setColour (amber.withAlpha (0.35f));
        g.fillEllipse (lp.x - 7, lp.y - 7, 14, 14);
    }
    g.setColour (on ? amber : lampOff);
    g.fillEllipse (lp.x - 4, lp.y - 4, 8, 8);

    g.setColour (on ? cream : cream.withAlpha (0.75f));
    g.setFont (bold (10.5f).withExtraKerningFactor (0.08f));
    g.drawFittedText (b.getButtonText(), r.withTrimmedLeft (22.0f).withTrimmedRight (4.0f).toNearestInt(), Justification::centred, 1);
}

void StationLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    // THROW: botão vermelho com aro cromado
    auto r = b.getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (1.5f, 3.0f), 7.0f);
    g.setGradientFill (ColourGradient (Colour (0xffeaeaea), r.getX(), r.getY(), Colour (0xff5a5a5a), r.getRight(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 7.0f);
    auto cap = r.reduced (3.5f).translated (0, down ? 1.5f : 0.0f);
    g.setGradientFill (ColourGradient (Colour (down ? 0xffb8321f : (over ? 0xffef5a36 : 0xffe04a2a)), 0, cap.getY(),
                                       Colour (down ? 0xff5e150b : 0xff8a1e10), 0, cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 5.0f);
    g.setColour (Colours::white.withAlpha (0.25f));
    g.fillRoundedRectangle (cap.reduced (4.0f, 0).withHeight (cap.getHeight() * 0.3f).translated (0, 2.0f), 3.0f);
}

void StationLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool down)
{
    g.setColour (cream);
    g.setFont (bold (14.0f).withExtraKerningFactor (0.1f));
    g.drawText (b.getButtonText(), b.getLocalBounds().toFloat().translated (0, down ? 1.5f : 0.0f), Justification::centred);
}

//==============================================================================
namespace
{
// desenho dos aparelhos em traço creme, como serigrafia no painel
void drawLineIcon (Graphics& g, int which, Rectangle<float> r, Colour c)
{
    const auto o = r.getCentre();
    const float s = jmin (r.getWidth() / 80.0f, r.getHeight() / 44.0f);
    auto P = [&] (float x, float y) { return Point<float> (o.x + x * s, o.y + y * s); };
    auto R = [&] (float x, float y, float w, float h) { return Rectangle<float> (o.x + x * s, o.y + y * s, w * s, h * s); };
    const PathStrokeType st (2.0f * s + 0.3f, PathStrokeType::curved, PathStrokeType::rounded);
    g.setColour (c);
    Path p;
    if (which == 0)
    {
        p.addRoundedRectangle (R (-30, -14, 60, 34), 7.0f * s);
        p.addEllipse (R (-25, -9, 24, 24));
        p.addRoundedRectangle (R (5, -9, 20, 10), 2.0f * s);
        p.addEllipse (R (7, 6, 7, 7)); p.addEllipse (R (17, 6, 7, 7));
        p.startNewSubPath (P (16, -14)); p.lineTo (P (30, -24));
        g.strokePath (p, st);
        for (int k = 0; k < 3; ++k) g.drawLine (Line<float> (P (-20, -3 + (float) k * 6), P (-6, -3 + (float) k * 6)), 1.4f * s);
        g.fillEllipse (Rectangle<float> (5 * s, 5 * s).withCentre (P (30, -24)));
    }
    else if (which == 1)
    {
        Path base;
        base.startNewSubPath (P (-24, -2)); base.lineTo (P (24, -2)); base.lineTo (P (32, 20)); base.lineTo (P (-32, 20)); base.closeSubPath();
        p.addPath (base.createPathWithRoundedCorners (5.0f * s));
        p.addEllipse (R (-11, 0, 22, 18));
        p.addRoundedRectangle (R (-24, -19, 48, 8), 4.0f * s);
        p.addEllipse (R (-35, -21, 17, 12)); p.addEllipse (R (18, -21, 17, 12));
        g.strokePath (p, st);
        for (int k = 0; k < 7; ++k)
            g.fillEllipse (Rectangle<float> (2.6f * s, 2.6f * s).withCentre (P (0, 9).getPointOnCircumference (6.0f * s, -2.4f + (float) k * 0.75f)));
    }
    else
    {
        Path cone;
        cone.startNewSubPath (P (-22, -7)); cone.lineTo (P (20, -20)); cone.lineTo (P (20, 20)); cone.lineTo (P (-22, 7)); cone.closeSubPath();
        p.addPath (cone.createPathWithRoundedCorners (3.0f * s));
        p.addEllipse (R (15, -21, 11, 42));
        p.addRoundedRectangle (R (-33, -5, 11, 10), 3.0f * s);
        p.addRoundedRectangle (R (-10, 4, 7, 15), 2.0f * s);
        p.addCentredArc (o.x + 27.0f * s, o.y, 8.0f * s, 8.0f * s, 0.0f, 0.7f, 2.45f, true);
        g.strokePath (p, st);
    }
}
} // namespace

StationModeKeys::StationModeKeys (RangedAudioParameter& p)
    : att (p, [this] (float v) { mode = jlimit (0, 2, roundToInt (v)); repaint(); })
{
    att.sendInitialUpdate();
    setMouseCursor (MouseCursor::PointingHandCursor);
}

Rectangle<float> StationModeKeys::key (int i) const
{
    auto r = getLocalBounds().toFloat().withTrimmedTop (32.0f);
    const float gap = 8.0f, h = (r.getHeight() - 2 * gap) / 3.0f;
    return { r.getX(), r.getY() + (float) i * (h + gap), r.getWidth(), h };
}

void StationModeKeys::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < 3; ++i)
        if (key (i).contains (e.position))
            att.setValueAsCompleteGesture ((float) i);
}

void StationModeKeys::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < 3; ++i) if (key (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; repaint(); }
}

void StationModeKeys::mouseExit (const MouseEvent&) { hover = -1; repaint(); }

void StationModeKeys::paint (Graphics& g)
{
    g.setColour (amber);
    g.setFont (bold (17.0f));
    g.drawText ("APARELHO", getLocalBounds().toFloat().withHeight (30.0f), Justification::centred);

    static const char* names[] = { "R\xc3\x81" "DIO AM", "TELEFONE", "MEGAFONE" };
    for (int i = 0; i < 3; ++i)
    {
        const bool on = i == mode, over = i == hover;
        auto r = key (i).reduced (1.0f);
        // sombra + aro cromado
        g.setColour (Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (1.5f, 3.0f), 7.0f);
        g.setGradientFill (ColourGradient (Colour (0xffe8e8e8), r.getX(), r.getY(), Colour (0xff4c4c4c), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 7.0f);
        // tecla de baquelite (afundada quando escolhida)
        auto cap = r.reduced (3.0f).translated (0, on ? 1.5f : 0.0f);
        g.setGradientFill (ColourGradient (Colour (on ? 0xff1a1714 : (over ? 0xff3a3633 : 0xff2e2b28)), 0, cap.getY(),
                                           Colour (on ? 0xff050505 : 0xff0d0c0b), 0, cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 5.0f);
        if (on)
        {
            g.setGradientFill (ColourGradient (amber.withAlpha (0.20f), cap.getCentreX(), cap.getCentreY(), amber.withAlpha (0.0f), cap.getRight(), cap.getCentreY(), true));
            g.fillRoundedRectangle (cap, 5.0f);
        }
        // desenho do aparelho + nome + lâmpada
        auto iconArea = cap.withWidth (cap.getHeight() * 1.6f).reduced (8.0f, 6.0f).translated (6.0f, 0);
        drawLineIcon (g, i, iconArea, on ? amber : cream.withAlpha (over ? 0.95f : 0.75f));
        auto txt = cap.withTrimmedLeft (iconArea.getRight() - cap.getX() + 14.0f).withTrimmedRight (22.0f);
        g.setColour (on ? amber : cream.withAlpha (0.85f));
        g.setFont (bold (12.5f).withExtraKerningFactor (0.08f));
        g.drawText (String::fromUTF8 (names[i]), txt, Justification::centredLeft);
        const auto lp = Point<float> (cap.getRight() - 13.0f, cap.getCentreY());
        if (on)
        {
            g.setColour (amber.withAlpha (0.35f));
            g.fillEllipse (lp.x - 7, lp.y - 7, 14, 14);
        }
        g.setColour (on ? amber : lampOff);
        g.fillEllipse (lp.x - 4, lp.y - 4, 8, 8);
    }
}

//==============================================================================
StationPanel::StationPanel (RadioFoneProcessor& p)
    : proc (p), modeKeys (*p.apvts.getParameter ("mode")), throwBtn ("THROW", p.apvts.getParameter ("throw"))
{
    setLookAndFeel (&lnf);
    setSize (kW, kH);

    addAndMakeVisible (modeKeys);
    setupKnob (tune,   "tune",   "SINTONIA", 0);
    tune.slider.getProperties().set ("scaleLabels", "540|600|700|800|1000|1200|1600");
    setupKnob (band,   "band",   "BANDA", 1);
    setupKnob (drive,  "drive",  "DRIVE", 1);
    setupKnob (noise,  "noise",  "CHIADO", 1);
    setupKnob (defect, "defect", "DEFEITO", 1);

    setupKnob (echoDiv,   "echoDiv",   "TEMPO", 2);
    setupKnob (echoMs,    "echoMs",    "TEMPO", 2);
    setupKnob (echoHeads, "echoHeads", String::fromUTF8 ("CABEÇOTES"), 2);
    setupKnob (echoFb,    "echoFb",    "INTENSIDADE", 2);
    setupKnob (echoTone,  "echoTone",  "TOM", 2);
    setupKnob (echoWow,   "echoWow",   "WOW/FLUTTER", 2);
    setupKnob (echoSend,  "echoSend",  "ENVIO", 2);
    setupKnob (mix, "mix", "MIX", 2);
    setupKnob (dry, "dry", "DRY", 2);
    setupKnob (out, "out", String::fromUTF8 ("SAÍDA"), 2);

    setupToggle (echoSync, "echoSync", "SYNC");
    setupToggle (echoPP, "echoPP", "PING-PONG");
    setupToggle (bypassBtn, "bypass", "BYPASS");
    bypassBtn.button.getProperties().set ("big", true);
    addAndMakeVisible (throwBtn);
    throwBtn.setTooltip ("Segure para jogar a voz no eco");

    skinBtn.setButtonText ("VISUAL CARTOON");
    skinBtn.setClickingTogglesState (false);
    skinBtn.onClick = [this] { if (onSwitchSkin) onSwitchSkin(); };
    addAndMakeVisible (skinBtn);

    status.setJustificationType (Justification::centredLeft);
    status.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 12.5f, Font::plain));
    status.setColour (Label::textColourId, amber);
    status.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (status);

    resized();
    startTimerHz (30);
}

StationPanel::~StationPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void StationPanel::setupKnob (Knob& k, const String& id, const String& name, int size)
{
    auto& s = k.slider;
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible (s);
    k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
    if (auto* prm = proc.apvts.getParameter (id))
        s.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    k.name = name;
    k.label.setText (name, dontSendNotification);
    k.label.setJustificationType (Justification::centred);
    k.label.setInterceptsMouseClicks (false, false);
    k.label.setFont (bold (size == 0 ? 17.0f : size == 1 ? 13.0f : 9.5f));
    k.label.setColour (Label::textColourId, size == 0 ? amber : cream);
    addAndMakeVisible (k.label);
}

void StationPanel::setupToggle (Toggle& t, const String& id, const String& text)
{
    t.button.setButtonText (text);
    t.button.setClickingTogglesState (true);
    addAndMakeVisible (t.button);
    t.att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, t.button);
}

void StationPanel::place (Knob& k, Rectangle<int> cell, int labelH)
{
    cell = cell.reduced (4);
    const int side = jmin (cell.getWidth(), cell.getHeight() - labelH);
    cell = cell.withSizeKeepingCentre (cell.getWidth(), labelH + side);   // nome logo acima do botão
    k.label.setBounds (cell.removeFromTop (labelH));
    k.slider.setBounds (cell.withSizeKeepingCentre (side, side));
}

void StationPanel::resized()
{
    auto content = getLocalBounds().reduced (kWood, 0);
    topPanel = content.removeFromTop (kTopH);
    plateArea = { (float) topPanel.getX() + 24.0f, 22.0f, 326.0f, 80.0f };
    {
        auto tp = topPanel;
        tp.removeFromLeft (350);
        vuArea = tp.removeFromLeft (190).reduced (6, 16);
        tapeWin = tp.reduced (14, 18);
    }
    lampPos = { plateArea.getRight() - 14.0f, (float) topPanel.getBottom() - 26.0f };
    status.setBounds (tapeWin.getX() + 14, tapeWin.getY() + 8, tapeWin.getWidth() - 28, 18);

    auto rows = content.reduced (8, 0).withTrimmedTop (10).withTrimmedBottom (8);
    auto rowA = rows.removeFromTop (214);
    rows.removeFromTop (6);
    auto rowB = rows;

    // linha A: APARELHO e SINTONIA gigantes, depois os quatro médios
    modeKeys.setBounds (rowA.removeFromLeft (226).reduced (14, 2));
    place (tune, rowA.removeFromLeft (226), 34);
    const int wm = 108;
    rowA.removeFromLeft ((rowA.getWidth() - 4 * wm) / 2);
    for (auto* k : { &band, &drive, &noise, &defect })
        place (*k, rowA.removeFromLeft (wm), 18);

    // linha B: placa do ECO DE FITA à esquerda, mistura/saída à direita
    echoPlate = rowB.removeFromLeft (594);
    auto ep = echoPlate.reduced (8, 6);
    ep.removeFromTop (16);                      // legenda
    auto btnCol = ep.removeFromRight (104);
    const int wk = ep.getWidth() / 6;
    Knob* ek[] = { &echoDiv, &echoHeads, &echoFb, &echoTone, &echoWow, &echoSend };
    for (int i = 0; i < 6; ++i)
    {
        auto cell = ep.removeFromLeft (wk);
        place (*ek[i], cell, 15);
        if (i == 0) place (echoMs, cell, 15);
    }
    btnCol = btnCol.reduced (4, 2);
    echoSync.button.setBounds (btnCol.removeFromTop (30));
    btnCol.removeFromTop (8);
    echoPP.button.setBounds (btnCol.removeFromTop (30));
    btnCol.removeFromTop (10);
    throwBtn.setBounds (btnCol.removeFromTop (jmin (52, btnCol.getHeight())));

    rightArea = rowB;
    auto ra = rowB.reduced (6, 4);
    ra.removeFromTop (20);                      // legenda
    auto bottom = ra.removeFromBottom (34);
    const int ws = ra.getWidth() / 3;
    for (auto* k : { &mix, &dry, &out })
        place (*k, ra.removeFromLeft (ws), 15);
    skinBtn.setBounds (bottom.withSizeKeepingCentre (150, 28));
    // BYPASS grande no painel de cima, entre "FEITO NO BRASIL" e a lâmpada NO AR
    bypassBtn.button.setBounds ((int) plateArea.getX() + 120, topPanel.getBottom() - 46, 116, 36);

    renderBackground();
}

//==============================================================================
String StationPanel::statusText()
{
    if (modeKeys.getHover() >= 0)
        return String::fromUTF8 ("APARELHO:  clique para escolher");
    Knob* all[] = { &tune, &band, &drive, &noise, &defect, &echoDiv, &echoMs, &echoHeads,
                    &echoFb, &echoTone, &echoWow, &echoSend, &mix, &dry, &out };
    for (auto* k : all)
        if (k->slider.isVisible() && k->slider.isMouseOverOrDragging())
            return k->name + ":  " + k->slider.getTextFromValue (k->slider.getValue());

    static const char* modes[] = { "R\xc3\x81" "DIO AM", "TELEFONE", "MEGAFONE" };
    String t = String::fromUTF8 (modes[jlimit (0, 2, modeKeys.getMode())]) + "  -  eco ";
    if (echoSync.button.getToggleState())
        t << RadioFoneProcessor::divisionNames()[jlimit (0, 6, (int) echoDiv.slider.getValue())]
          << " @ " << roundToInt (proc.getDisplayBpm()) << " BPM";
    else
        t << roundToInt (echoMs.slider.getValue()) << " ms";
    return t;
}

void StationPanel::timerCallback()
{
    const bool sync = echoSync.button.getToggleState();
    echoDiv.slider.setVisible (sync);
    echoMs.slider.setVisible (! sync);

    // VU (0 VU = -18 dBFS), balística de ~300 ms com leve overshoot
    const float lvl = proc.getOutputLevel();
    const float vu = 20.0f * std::log10 (lvl + 1e-9f) + 18.0f;
    const float target = jlimit (-0.03f, 1.06f, vuToPos (vu));
    needleVel = (needleVel + (target - needlePos) * 0.30f) * 0.58f;
    needlePos += needleVel;
    peakHold = vu > 3.0f ? 15 : jmax (0, peakHold - 1);
    lampGlow += ((lvl > 0.003f ? 1.0f : 0.0f) - lampGlow) * 0.3f;

    const bool echoOn = proc.getEchoLevel() > 0.0005f || echoSend.slider.getValue() > 0.5 || throwBtn.isDown();
    if (echoOn) tapePhase += 0.6f + 1.6f * jlimit (0.2f, 3.0f, 0.4f / jmax (0.04f, proc.getEchoDelaySec()));

    status.setText (statusText(), dontSendNotification);
    repaint (vuArea);
    repaint (tapeWin.expanded (4));
    repaint (Rectangle<float> (40, 40).withCentre (lampPos).toNearestInt());
}

//==============================================================================
void StationPanel::paint (Graphics& g)
{
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    // plaqueta de metal: "MASTOR FONE" gravado, legenda e versão no visor âmbar
    const auto pl = plateArea;
    auto engraved = [&g] (const String& t, Rectangle<float> r, Justification j, Colour c)
    {
        g.setColour (Colours::white.withAlpha (0.55f));
        g.drawText (t, r.translated (0.0f, 1.0f), j);
        g.setColour (c);
        g.drawText (t, r, j);
    };
    g.setFont (toon::title (25.0f).withExtraKerningFactor (0.03f));
    engraved ("MASTOR FONE", { pl.getX() + 15, pl.getY() + 10, pl.getWidth() - 86, 40 }, Justification::centredLeft, Colour (0xff262626));
    g.setFont (bold (9.5f).withExtraKerningFactor (0.12f));
    engraved (String::fromUTF8 ("RÁDIO AM  ·  TELEFONE  ·  MEGAFONE"), { pl.getX() + 17, pl.getY() + 53, pl.getWidth() - 96, 14 },
              Justification::centredLeft, Colour (0xff3a3a3a));
    const auto inset = Rectangle<float> (pl.getRight() - 70, pl.getY() + 17, 54, 46);
    g.setFont (toon::title (23.0f));
    g.setColour (amber.withAlpha (0.35f));
    g.drawText ("1.2", inset.expanded (1.0f), Justification::centred);
    g.setColour (amber);
    g.drawText ("1.2", inset, Justification::centred);

    // assinatura "Dub Station Solutions" em tinta marrom, com traço
    {
        Graphics::ScopedSaveState ss (g);
        const auto sp = Point<float> (pl.getX() + 10, pl.getBottom() + 4);
        g.addTransform (AffineTransform::rotation (-0.05f, sp.x, sp.y + 26));
        g.setColour (ink);
        g.setFont (toon::script (38.0f));
        g.drawText ("Dub Station Solutions", Rectangle<float> (sp.x, sp.y, 330, 44), Justification::centredLeft);
        Path swash;
        swash.startNewSubPath (sp.x + 10, sp.y + 46);
        swash.cubicTo (sp.x + 90, sp.y + 38, sp.x + 190, sp.y + 52, sp.x + 290, sp.y + 36);
        g.strokePath (swash, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // origem + lâmpada NO AR
    g.setColour (ink.withAlpha (0.85f));
    g.setFont (bold (10.0f).withExtraKerningFactor (0.1f));
    g.drawText ("FEITO NO BRASIL", Rectangle<float> (pl.getX() + 2, (float) topPanel.getBottom() - 34, 110, 14), Justification::left);
    const auto lamp = lampPos;
    g.setColour (Colour (0xff8a8a8a));
    g.fillEllipse (lamp.x - 13, lamp.y - 13, 26, 26);
    if (lampGlow > 0.02f)
    {
        g.setGradientFill (ColourGradient (Colour (0xffff9d3a).withAlpha (0.5f * lampGlow), lamp.x, lamp.y,
                                           Colour (0x00ff9d3a), lamp.x + 18, lamp.y, true));
        g.fillEllipse (lamp.x - 18, lamp.y - 18, 36, 36);
    }
    g.setGradientFill (ColourGradient (Colour (0xffffe0a0).interpolatedWith (Colour (0xff5a3a20), 1.0f - lampGlow), lamp.x - 4, lamp.y - 4,
                                       Colour (0xffd2560c).interpolatedWith (Colour (0xff3a2010), 1.0f - lampGlow), lamp.x + 9, lamp.y + 9, true));
    g.fillEllipse (lamp.x - 9, lamp.y - 9, 18, 18);
    g.setColour (ink);
    g.setFont (bold (10.0f));
    g.drawText ("NO AR", Rectangle<float> (lamp.x - 70, lamp.y - 7, 50, 14), Justification::right);

    // legendas da placa do eco e da mistura
    g.setColour (cream.withAlpha (0.85f));
    g.setFont (bold (11.0f).withExtraKerningFactor (0.06f));
    g.drawText ("ECO DE FITA", echoPlate.toFloat().reduced (12, 6).withHeight (18.0f), Justification::centredLeft);
    g.drawText (String::fromUTF8 ("MISTURA  ·  SAÍDA"), rightArea.toFloat().reduced (12, 6).withHeight (18.0f), Justification::centredLeft);

    drawTape (g);
    drawVuNeedle (g);
}

void StationPanel::drawTape (Graphics& g)
{
    auto win = tapeWin.toFloat();
    g.setColour (Colour (0xff0d0f0e));
    g.fillRoundedRectangle (win, 10.0f);

    const float L = win.getX() + 64, R = win.getRight() - 64;
    const float T = win.getY() + 34, B = win.getY() + 112;
    const float cy = (T + B) * 0.5f, rad = (B - T) * 0.5f;

    // laço da fita com as marcas de óxido andando
    Path loop;
    loop.addRoundedRectangle (L - rad, T, (R - L) + 2 * rad, B - T, rad);
    g.setColour (Colour (0xff4a2e18));
    g.strokePath (loop, PathStrokeType (7.0f));
    g.setColour (Colour (0xff6b4626));
    const float perim = loop.getLength();
    for (float d = std::fmod (tapePhase, 14.0f); d < perim; d += 14.0f)
    {
        const auto pt = loop.getPointAlongPath (d);
        g.fillEllipse (pt.x - 1.5f, pt.y - 1.5f, 3.0f, 3.0f);
    }

    auto roller = [&g] (Point<float> c, float r, float spin)
    {
        g.setGradientFill (ColourGradient (Colour (0xffd0d0d0), c.x - r, c.y - r, Colour (0xff6a6a6a), c.x + r, c.y + r, false));
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        g.setColour (Colour (0xff2a2a2a));
        for (int i = 0; i < 3; ++i)
            g.drawLine ({ c, c.getPointOnCircumference (r * 0.8f, spin + (float) i * 2.0944f) }, 1.5f);
        g.fillEllipse (c.x - 3, c.y - 3, 6, 6);
    };
    roller ({ L - rad + 24.0f, cy }, 20.0f, tapePhase * 0.05f);
    roller ({ R + rad - 24.0f, cy }, 20.0f, tapePhase * 0.05f);

    // cabeçotes: ERASE, REC, 1, 2, 3
    const int mask = headMasks[jlimit (0, 6, (int) echoHeads.slider.getValue())];
    const char* hn[] = { "ERASE", "REC", "1", "2", "3" };
    g.setFont (bold (9.5f));
    for (int h = 0; h < 5; ++h)
    {
        const float hx = L + 26.0f + (float) h * ((R - L - 52.0f) / 4.0f);
        g.setGradientFill (ColourGradient (Colour (0xffb8b8b8), hx - 11, B - 2, Colour (0xff5e5e5e), hx + 11, B + 18, false));
        g.fillRoundedRectangle (hx - 11, B - 1, 22, 18, 3.0f);
        g.setColour (Colour (0xff1a1a1a));
        g.drawLine (hx, B - 1, hx, B + 4, 1.0f);
        const bool on = h <= 1 ? (h == 1) : (mask & (1 << (h - 2))) != 0;
        g.setColour (on ? amber : lampOff);
        g.fillEllipse (hx - 4, B + 21, 8, 8);
        g.setColour (cream.withAlpha (0.75f));
        g.drawText (hn[h], Rectangle<float> (hx - 22, B + 30, 44, 11), Justification::centred);
    }

    // vidro fumê + moldura
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.07f), win.getX(), win.getY(),
                                       Colours::transparentWhite, win.getX() + 120, win.getBottom(), false));
    g.fillRoundedRectangle (win, 10.0f);
    g.setColour (Colour (0xff8f8a7c));
    g.drawRoundedRectangle (win.expanded (2.0f), 11.0f, 3.0f);
}

//==============================================================================
namespace
{
struct VuGeom
{
    Rectangle<float> housing, face, label;
    Point<float> pivot;
    float radius;
};
VuGeom vuGeom (Rectangle<int> area)
{
    VuGeom v;
    v.housing = area.toFloat();
    v.face = v.housing.reduced (11.0f).withHeight (v.housing.getHeight() - 52.0f);
    v.label = Rectangle<float> (v.housing.getX(), v.face.getBottom() + 6.0f, v.housing.getWidth(), v.housing.getBottom() - v.face.getBottom() - 10.0f);
    v.pivot = { v.face.getCentreX(), v.face.getBottom() + 6.0f };
    v.radius = v.face.getHeight() * 0.74f;
    return v;
}
} // namespace

void StationPanel::drawVuFace (Graphics& g)
{
    const auto v = vuGeom (vuArea);
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (v.housing.translated (2.0f, 3.0f), 8.0f);
    g.setGradientFill (ColourGradient (Colour (0xff2a2a2a), 0, v.housing.getY(), Colour (0xff0b0b0b), 0, v.housing.getBottom(), false));
    g.fillRoundedRectangle (v.housing, 8.0f);
    g.setGradientFill (ColourGradient (Colour (0xffe8e8e8), v.face.getX(), v.face.getY(), Colour (0xff4c4c4c), v.face.getRight(), v.face.getBottom(), false));
    g.drawRoundedRectangle (v.face.expanded (3.0f), 4.0f, 2.5f);
    g.setGradientFill (ColourGradient (Colour (0xfff7e8b8), v.face.getCentreX(), v.face.getY() + v.face.getHeight() * 0.55f,
                                       Colour (0xffd9b86a), v.face.getX(), v.face.getY(), true));
    g.fillRoundedRectangle (v.face, 3.0f);

    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (v.face.toNearestInt());
    const auto c = v.pivot;
    const float R = v.radius;
    auto angleOf = [] (float pos) { return -kVuSwing + 2.0f * kVuSwing * jlimit (0.0f, 1.0f, pos); };

    Path black, red;
    black.addCentredArc (c.x, c.y, R, R, 0.0f, angleOf (0.0f), angleOf (vuToPos (0.0f)), true);
    red.addCentredArc (c.x, c.y, R, R, 0.0f, angleOf (vuToPos (0.0f)), angleOf (1.0f), true);
    g.setColour (Colour (0xff1a1a1a));
    g.strokePath (black, PathStrokeType (1.6f));
    g.setColour (Colour (0xffc0262d));
    g.strokePath (red, PathStrokeType (5.0f));

    const float marks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    g.setFont (bold (10.0f));
    for (float m : marks)
    {
        const float t = angleOf (vuToPos (m));
        const bool hot = m > 0.0f, major = m != -2.0f;
        g.setColour (hot ? Colour (0xffc0262d) : Colour (0xff1a1a1a));
        g.drawLine ({ c.getPointOnCircumference (R - (major ? 7.0f : 4.0f), t), c.getPointOnCircumference (R + 2.0f, t) }, major ? 1.6f : 1.0f);
        if (major)
        {
            const auto np = c.getPointOnCircumference (R + 11.0f, t);
            const String txt = m > 0 ? "+" + String ((int) m) : String ((int) std::abs (m));
            g.drawText (txt, Rectangle<float> (np.x - 12, np.y - 7, 24, 14), Justification::centred);
        }
    }
    g.setColour (Colour (0xff1a1a1a));
    g.setFont (toon::title (21.0f));
    g.drawText ("VU", Rectangle<float> (v.face.getX(), v.face.getY() + v.face.getHeight() * 0.48f, v.face.getWidth(), 26), Justification::centred);
}

void StationPanel::drawVuNeedle (Graphics& g)
{
    const auto v = vuGeom (vuArea);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (v.face.toNearestInt());
        const float t = -kVuSwing + 2.0f * kVuSwing * jlimit (-0.03f, 1.06f, needlePos);
        const auto tip = v.pivot.getPointOnCircumference (v.radius + 4.0f, t);
        const auto base = v.pivot.getPointOnCircumference (v.radius * 0.30f, t);
        g.setColour (Colours::black.withAlpha (0.18f));
        g.drawLine ({ base.translated (2.0f, 3.0f), tip.translated (2.0f, 3.0f) }, 1.6f);
        g.setColour (Colour (0xff111111));
        g.drawLine ({ base, tip }, 1.4f);

        auto cover = v.face.withTop (v.face.getBottom() - 18.0f);
        g.setGradientFill (ColourGradient (Colour (0xff2b2b2b), 0, cover.getY(), Colour (0xff0c0c0c), 0, cover.getBottom(), false));
        g.fillRect (cover);
        g.setColour (Colours::white.withAlpha (0.15f));
        g.drawLine (cover.getX(), cover.getY(), cover.getRight(), cover.getY(), 1.0f);

        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.22f), v.face.getX(), v.face.getY(),
                                           Colours::transparentWhite, v.face.getX() + v.face.getWidth() * 0.55f, v.face.getY() + v.face.getHeight() * 0.6f, false));
        Path glare;
        glare.startNewSubPath (v.face.getX(), v.face.getY());
        glare.lineTo (v.face.getRight() * 0.6f + v.face.getX() * 0.4f, v.face.getY());
        glare.lineTo (v.face.getX(), v.face.getY() + v.face.getHeight() * 0.7f);
        glare.closeSubPath();
        g.fillPath (glare);
    }

    g.setColour (cream.withAlpha (0.85f));
    g.setFont (bold (9.5f).withExtraKerningFactor (0.12f));
    g.drawText (String::fromUTF8 ("NÍVEL"), v.label.withTrimmedLeft (14.0f), Justification::centredLeft);
    const auto lp = Point<float> (v.label.getRight() - 22.0f, v.label.getCentreY());
    g.setColour (peakHold > 0 ? Colour (0xffff3b2f) : Colour (0xff4a1612));
    g.fillEllipse (lp.x - 4.5f, lp.y - 4.5f, 9.0f, 9.0f);
    if (peakHold > 0)
    {
        g.setColour (Colour (0x66ff3b2f));
        g.fillEllipse (lp.x - 8.0f, lp.y - 8.0f, 16.0f, 16.0f);
    }
    g.setColour (cream.withAlpha (0.6f));
    g.setFont (bold (8.0f));
    g.drawText ("PEAK", Rectangle<float> (lp.x - 46.0f, lp.y - 6.0f, 36.0f, 12.0f), Justification::centredRight);
}

//==============================================================================
void StationPanel::renderBackground()
{
    const int S = 2;
    const int W = kW * S, H = kH * S, topH = kTopH * S;
    background = Image (Image::RGB, W, H, false);
    Random rnd (1975);

    {   // esmalte creme (em cima) + aço pintado escuro (embaixo), pixel a pixel
        Image::BitmapData bd (background, Image::BitmapData::writeOnly);
        for (int y = 0; y < H; ++y)
        {
            const float vy = (float) y / (float) H;
            for (int x = 0; x < W; ++x)
            {
                const float vx = (float) x / (float) W;
                const float n = rnd.nextFloat() - 0.5f;
                if (y < topH)
                {
                    const float ey = (float) y / (float) topH;
                    const float edge = jmax (std::pow (std::abs (vx - 0.5f) * 2.0f, 4.0f), std::pow (std::abs (ey - 0.5f) * 2.0f, 4.0f));
                    const float k = 1.0f - 0.18f * edge + 0.025f * n + 0.02f * std::sin (vx * 23.0f) * std::sin (ey * 7.0f);
                    bd.setPixelColour (x, y, Colour::fromFloatRGBA (0.85f * k, 0.80f * k * (1.0f - 0.04f * edge), 0.68f * k * (1.0f - 0.10f * edge), 1.0f));
                }
                else
                {
                    float k = 0.12f + 0.015f * n + 0.01f * std::sin (vy * 90.0f + vx * 4.0f);
                    k *= 1.0f - 0.3f * std::pow (std::abs (vx - 0.5f) * 2.0f, 4.0f);
                    bd.setPixelColour (x, y, Colour::fromFloatRGBA (k * 1.08f, k * 0.98f, k * 0.90f, 1.0f));
                }
            }
        }
    }

    Graphics g (background);
    g.addTransform (AffineTransform::scale ((float) S));
    const float w = (float) kW, h = (float) kH, tH = (float) kTopH;

    // desgaste do esmalte e riscos no painel escuro
    for (int i = 0; i < 40; ++i)
    {
        const float cx = kWood + rnd.nextFloat() * (w - 2 * kWood), cy = rnd.nextFloat() * tH;
        const float r = 0.6f + rnd.nextFloat() * (rnd.nextFloat() < 0.1f ? 3.5f : 1.2f);
        g.setColour (Colour (0xff3b3026).withAlpha (0.35f + 0.4f * rnd.nextFloat()));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 1.6f);
    }
    for (int i = 0; i < 8; ++i)
    {
        const float cx = kWood + rnd.nextFloat() * (w - 2 * kWood), cy = rnd.nextFloat() * tH, r = 25.0f + rnd.nextFloat() * 60.0f;
        auto col = Colour (0xff8a6a3a).withAlpha (0.08f);
        g.setGradientFill (ColourGradient (col, cx, cy, col.withAlpha (0.0f), cx + r, cy, true));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    }
    for (int i = 0; i < 90; ++i)
    {
        const float x0 = kWood + rnd.nextFloat() * (w - 2 * kWood), y0 = tH + rnd.nextFloat() * (h - tH);
        const float len = 3.0f + rnd.nextFloat() * 22.0f, a = rnd.nextFloat() * MathConstants<float>::pi;
        g.setColour (Colours::white.withAlpha (0.04f + 0.07f * rnd.nextFloat()));
        g.drawLine (x0, y0, x0 + len * std::cos (a), y0 + len * std::sin (a), 0.6f);
    }

    // plaqueta de alumínio escovado com rebites
    {
        const auto pl = plateArea;
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (pl.translated (2.0f, 3.0f), 4.0f);
        g.setGradientFill (ColourGradient (Colour (0xffdcdcd8), 0, pl.getY(), Colour (0xff9c9c97), 0, pl.getBottom(), false));
        g.fillRoundedRectangle (pl, 4.0f);
        for (float yy = pl.getY() + 1.0f; yy < pl.getBottom() - 1.0f; yy += 0.5f)
        {
            g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (0.04f + 0.05f * rnd.nextFloat()));
            const float x0 = pl.getX() + 2.0f + rnd.nextFloat() * 30.0f, x1 = pl.getRight() - 2.0f - rnd.nextFloat() * 30.0f;
            g.drawLine (x0, yy, x1, yy, 0.5f);
        }
        g.setColour (Colours::white.withAlpha (0.7f));
        g.drawLine (pl.getX() + 3, pl.getY() + 1, pl.getRight() - 3, pl.getY() + 1, 1.0f);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (pl, 4.0f, 1.0f);
        for (auto rv : { Point<float> (pl.getX() + 8, pl.getY() + 8), Point<float> (pl.getRight() - 8, pl.getY() + 8),
                         Point<float> (pl.getX() + 8, pl.getBottom() - 8), Point<float> (pl.getRight() - 8, pl.getBottom() - 8) })
        {
            g.setColour (Colours::black.withAlpha (0.4f));
            g.fillEllipse (rv.x - 3.0f, rv.y - 2.0f, 6.5f, 6.5f);
            g.setGradientFill (ColourGradient (Colour (0xfff4f4f4), rv.x - 3, rv.y - 3, Colour (0xff6a6a6a), rv.x + 3, rv.y + 3, false));
            g.fillEllipse (rv.x - 3.0f, rv.y - 3.0f, 6.0f, 6.0f);
        }
        const auto inset = Rectangle<float> (pl.getRight() - 70, pl.getY() + 17, 54, 46);
        g.setColour (Colour (0xff0e0e0e));
        g.fillRoundedRectangle (inset, 3.0f);
        g.setColour (Colours::white.withAlpha (0.5f));
        g.drawLine (inset.getX() + 2, inset.getBottom() + 0.5f, inset.getRight() - 2, inset.getBottom() + 0.5f, 1.0f);
    }

    drawVuFace (g);

    // emenda dos painéis com friso cromado
    g.setColour (Colour (0xff111111));
    g.fillRect ((float) kWood, tH - 2.0f, w - 2.0f * kWood, 6.0f);
    g.setGradientFill (ColourGradient (Colour (0xffd8d8d8), 0, tH - 1, Colour (0xff6c6c6c), 0, tH + 3, false));
    g.fillRect ((float) kWood, tH - 1.0f, w - 2.0f * kWood, 3.0f);

    // divisórias gravadas
    auto engrave = [&g] (float x0, float y0, float x1, float y1)
    {
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawLine (x0, y0, x1, y1, 1.0f);
        g.setColour (Colours::white.withAlpha (0.08f));
        g.drawLine (x0 + (x0 == x1 ? 1.0f : 0.0f), y0 + (y0 == y1 ? 1.0f : 0.0f), x1 + (x0 == x1 ? 1.0f : 0.0f), y1 + (y0 == y1 ? 1.0f : 0.0f), 1.0f);
    };
    const float divY = (float) echoPlate.getY() - 4.0f;
    engrave (kWood + 20.0f, divY, w - kWood - 20.0f, divY);
    engrave ((float) echoPlate.getRight() + 4.0f, (float) echoPlate.getY() + 6.0f, (float) echoPlate.getRight() + 4.0f, (float) echoPlate.getBottom() - 4.0f);

    // placa rebaixada do eco
    auto plate = echoPlate.toFloat().reduced (4.0f, 2.0f);
    g.setColour (Colour (0xff0b0a09).withAlpha (0.7f));
    g.fillRoundedRectangle (plate, 6.0f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (plate.translated (0, 1), 6.0f, 1.0f);
    g.setColour (Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (plate, 6.0f, 1.0f);

    // poços destacados atrás de APARELHO e SINTONIA: rebaixo + aro cromado + brilho âmbar
    for (auto* k : { &tune })
    {
        const auto kb = k->slider.getBounds().toFloat();
        const auto c = kb.getCentre();
        const float rr = kb.getWidth() * 0.5f + 2.0f;
        g.setGradientFill (ColourGradient (amber.withAlpha (0.22f), c.x, c.y, amber.withAlpha (0.0f), c.x + rr + 12.0f, c.y, true));
        g.fillEllipse (c.x - rr - 12.0f, c.y - rr - 12.0f, (rr + 12.0f) * 2, (rr + 12.0f) * 2);
        g.setGradientFill (ColourGradient (Colour (0xff050505), c.x, c.y - rr, Colour (0xff2b2622), c.x, c.y + rr, false));
        g.fillEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2);
        g.setGradientFill (ColourGradient (Colour (0xffe8e8e8), c.x - rr, c.y - rr, Colour (0xff4c4c4c), c.x + rr, c.y + rr, false));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 3.5f);
        g.setColour (amber.withAlpha (0.55f));
        g.drawEllipse (c.x - rr - 4.0f, c.y - rr - 4.0f, (rr + 4.0f) * 2, (rr + 4.0f) * 2, 1.2f);
    }

    // parafusos
    auto screw = [&g, &rnd] (float cx, float cy)
    {
        const float r = 5.5f;
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillEllipse (cx - r + 1, cy - r + 2, r * 2, r * 2);
        g.setGradientFill (ColourGradient (Colour (0xffd0d0d0), cx - r, cy - r, Colour (0xff5a5a5a), cx + r, cy + r, false));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
        const float a = rnd.nextFloat() * MathConstants<float>::pi;
        g.setColour (Colour (0xff2a2a2a));
        g.drawLine (cx - r * 0.7f * std::cos (a), cy - r * 0.7f * std::sin (a), cx + r * 0.7f * std::cos (a), cy + r * 0.7f * std::sin (a), 1.5f);
    };
    for (float sy : { 12.0f, tH - 14.0f, h - 12.0f })
    {
        screw (kWood + 12.0f, sy);
        screw (w - kWood - 12.0f, sy);
    }

    // laterais de madeira
    {
        Image::BitmapData bd (background, Image::BitmapData::readWrite);
        const int woodW = kWood * S;
        for (int side = 0; side < 2; ++side)
        {
            const int x0 = side == 0 ? 0 : W - woodW;
            const float seed = side == 0 ? 1.3f : 4.7f;
            for (int y = 0; y < H; ++y)
                for (int i = 0; i < woodW; ++i)
                {
                    const float fx = (float) i / (float) S, fy = (float) y / (float) S;
                    const float warp = 3.0f * std::sin (fy * 0.012f + seed) + 1.2f * std::sin (fy * 0.047f + seed * 2.0f);
                    const float grain = std::sin ((fx + warp) * 1.7f + seed);
                    const float ring = std::pow (std::abs (std::sin ((fx * 0.35f + warp * 0.6f) + fy * 0.002f)), 6.0f);
                    float r = 0.40f + 0.06f * grain - 0.12f * ring + 0.03f * (rnd.nextFloat() - 0.5f);
                    r *= 0.55f + 0.45f * std::sin (MathConstants<float>::pi * ((float) i + 0.5f) / (float) woodW);
                    r *= 1.0f - 0.15f * std::pow (std::abs ((float) y / (float) H - 0.5f) * 2.0f, 4.0f);
                    bd.setPixelColour (x0 + i, y, Colour::fromFloatRGBA (r * 1.05f, r * 0.62f, r * 0.33f, 1.0f));
                }
        }
    }
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawLine ((float) kWood, 0.0f, (float) kWood, h, 1.5f);
    g.drawLine (w - (float) kWood, 0.0f, w - (float) kWood, h, 1.5f);

    // cantos arredondados do aparelho
    const auto unit = Rectangle<float> (0.0f, 0.0f, w, h);
    const float corner = 24.0f;
    Path outside;
    outside.addRectangle (unit.expanded (2.0f));
    outside.addRoundedRectangle (unit, corner);
    outside.setUsingNonZeroWinding (false);
    g.setColour (Colour (0xff0c0c0c));
    g.fillPath (outside);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (unit.reduced (1.5f), corner - 1.5f, 1.2f);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (unit.reduced (0.5f), corner, 1.0f);
}
