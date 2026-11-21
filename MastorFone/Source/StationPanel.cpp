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
constexpr const char* kVersion = "1.6";
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
namespace
{
const float kFreqs[] = { 540, 600, 700, 800, 900, 1000, 1200, 1400, 1600 };
float valueToKHz (float v)   // -100..100 -> kHz, na mesma escala do dial
{
    const float t = jlimit (0.0f, 1.0f, (v + 100.0f) / 200.0f) * 8.0f;
    const int i = jmin (7, (int) t);
    return kFreqs[i] + (kFreqs[i + 1] - kFreqs[i]) * (t - (float) i);
}
}

TuningDial::TuningDial (RangedAudioParameter& p)
    : param (p), att (p, [this] (float v) { value = v; repaint(); })
{
    att.sendInitialUpdate();
    setMouseCursor (MouseCursor::LeftRightResizeCursor);
}

String TuningDial::getText() const { return String (roundToInt (valueToKHz (value) / 10.0f) * 10) + " kHz"; }

// escala total do dial (em pixels) para a largura visível: a escala é mais longa que a janela e gira
static float dialSpan (float visibleW) { return visibleW * 2.4f; }

void TuningDial::mouseDown (const MouseEvent&) { dragStartValue = value; att.beginGesture(); }
void TuningDial::mouseDrag (const MouseEvent& e)
{
    // arrastar move a escala por baixo da agulha (como girar o botão de um rádio)
    const float w = (float) getWidth() - 8.0f;
    att.setValueAsPartOfGesture (jlimit (-100.0f, 100.0f, dragStartValue - 200.0f * (float) e.getDistanceFromDragStartX() / dialSpan (w)));
}
void TuningDial::mouseUp (const MouseEvent&)     { att.endGesture(); }
void TuningDial::mouseDoubleClick (const MouseEvent&) { att.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }
void TuningDial::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    att.setValueAsCompleteGesture (jlimit (-100.0f, 100.0f, value + (w.deltaY + w.deltaX) * 40.0f));
}

void TuningDial::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    // legenda + leituras: frequência sintonizada e frequência de áudio que o aparelho mais excita
    auto cap = b.removeFromTop (22.0f);
    g.setColour (amber);
    g.setFont (bold (17.0f));
    g.drawText ("SINTONIA", cap.withTrimmedLeft (4.0f), Justification::centredLeft);
    g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 15.0f, Font::bold));
    g.drawText (getText(), cap.withTrimmedRight (6.0f), Justification::centredRight);
    const String ex = String::fromUTF8 ("excita ") + (excitedHz >= 1000.0f ? String (excitedHz / 1000.0f, 1) + " kHz" : String (roundToInt (excitedHz)) + " Hz");
    g.setColour (Colour (0xff5dff8a));
    g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 12.0f, Font::plain));
    g.drawText (ex, cap.withTrimmedLeft (108.0f).withTrimmedRight (110.0f), Justification::centred);

    // moldura cromada + vidro iluminado por trás (mesmo tom do VU)
    auto glass = b.reduced (4.0f);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (glass.expanded (3.0f).translated (1.5f, 3.0f), 6.0f);
    g.setGradientFill (ColourGradient (Colour (0xffe8e8e8), glass.getX(), glass.getY(), Colour (0xff4c4c4c), glass.getRight(), glass.getBottom(), false));
    g.fillRoundedRectangle (glass.expanded (3.0f), 6.0f);
    g.setGradientFill (ColourGradient (Colour (0xfff7e8b8), glass.getCentreX(), glass.getCentreY(),
                                       Colour (0xffd9b86a), glass.getX(), glass.getY(), true));
    g.fillRoundedRectangle (glass, 4.0f);

    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (glass.toNearestInt());
    const float cx = glass.getCentreX();
    const float span = dialSpan (glass.getWidth());
    const float t0 = (value + 100.0f) / 200.0f;              // posição atual na escala (0..1)
    auto xOf = [&] (float t) { return cx + (t - t0) * span; };

    // faixa âmbar em volta da agulha: largura da BANDA (o que o aparelho deixa passar)
    {
        const float half = glass.getWidth() * (0.06f + 0.16f * bandW);
        auto band = Rectangle<float> (cx - half, glass.getY(), half * 2.0f, glass.getHeight());
        g.setGradientFill (ColourGradient (Colour (0x00ffa630), band.getX(), 0, Colour (0x00ffa630), band.getRight(), 0, false));
        ColourGradient cg (Colour (0x00ff8a00), band.getX(), 0, Colour (0x00ff8a00), band.getRight(), 0, false);
        cg.addColour (0.5, Colour (0x66ff8a00));
        g.setGradientFill (cg);
        g.fillRect (band);
    }

    const Colour printInk (0xff1a1a1a);
    g.setColour (printInk);
    for (int i = 0; i <= 40; ++i)
    {
        const float x = xOf ((float) i / 40.0f);
        if (x < glass.getX() - 4 || x > glass.getRight() + 4) continue;
        const bool major = i % 5 == 0;
        g.fillRect (x - (major ? 1.0f : 0.6f), glass.getY() + 6.0f, major ? 2.0f : 1.2f, major ? 14.0f : 8.0f);
    }
    // números: o mais perto da agulha fica maior e em vermelho
    int nearest = 0; float nd = 1e9f;
    for (int i = 0; i < 9; ++i) { const float d = std::abs (xOf ((float) i / 8.0f) - cx); if (d < nd) { nd = d; nearest = i; } }
    for (int i = 0; i < 9; ++i)
    {
        const float x = xOf ((float) i / 8.0f);
        if (x < glass.getX() - 30 || x > glass.getRight() + 30) continue;
        const bool hot = i == nearest && nd < span / 16.0f;
        g.setColour (hot ? Colour (0xffc0262d) : printInk);
        g.setFont (bold (hot ? 15.0f : 12.0f));
        g.drawText (String ((int) kFreqs[i]), Rectangle<float> (52, 18).withCentre ({ x, glass.getY() + 31.0f }), Justification::centred);
    }
    // pontas da escala
    g.setColour (printInk.withAlpha (0.5f));
    for (float t : { 0.0f, 1.0f })
    {
        const float x = xOf (t) + (t == 0.0f ? -14.0f : 14.0f);
        g.fillRect (t == 0.0f ? glass.getX() : x, glass.getY(), t == 0.0f ? jmax (0.0f, x - glass.getX()) : glass.getRight() - x, 3.0f);
    }
    g.setColour (Colour (0xffc0262d));
    g.setFont (bold (10.0f).withExtraKerningFactor (0.12f));
    g.drawText (String::fromUTF8 ("ONDAS MÉDIAS"), Rectangle<float> (glass.getX() + 10, glass.getBottom() - 17, 120, 14), Justification::centredLeft);
    g.drawText ("kHz", Rectangle<float> (glass.getRight() - 50, glass.getBottom() - 17, 40, 14), Justification::centredRight);

    // agulha fixa no meio, com sombra
    g.setColour (Colours::black.withAlpha (0.2f));
    g.fillRect (cx + 1.5f, glass.getY() + 3.0f, 2.5f, glass.getHeight() - 4.0f);
    g.setColour (Colour (0xffc0262d));
    g.fillRect (cx - 1.25f, glass.getY() + 1.0f, 2.5f, glass.getHeight() - 2.0f);
    Path tri; tri.addTriangle (cx - 6, glass.getY(), cx + 6, glass.getY(), cx, glass.getY() + 7);
    g.fillPath (tri);

    // sombra das bordas (tambor curvo) + reflexo do vidro
    ColourGradient edge (Colours::black.withAlpha (0.35f), glass.getX(), 0, Colours::black.withAlpha (0.35f), glass.getRight(), 0, false);
    edge.addColour (0.18, Colours::transparentBlack);
    edge.addColour (0.82, Colours::transparentBlack);
    g.setGradientFill (edge);
    g.fillRect (glass);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.25f), glass.getX(), glass.getY(),
                                       Colours::transparentWhite, glass.getX(), glass.getY() + glass.getHeight() * 0.5f, false));
    g.fillRect (glass.withHeight (glass.getHeight() * 0.45f));
}

//==============================================================================
Rectangle<float> drawCrtFrame (Graphics& g, Rectangle<float> b, const String& labelText, Path& tube)
{
    const Colour phosphor (0xff5dff8a);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (b.reduced (2.0f).translated (1.5f, 3.0f), 10.0f);
    g.setGradientFill (ColourGradient (Colour (0xff2a2a2a), 0, b.getY(), Colour (0xff0b0b0b), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b.reduced (2.0f), 10.0f);
    auto label = b.removeFromBottom (18.0f);
    auto screen = b.reduced (12.0f, 9.0f);
    g.setGradientFill (ColourGradient (Colour (0xffe8e8e8), screen.getX(), screen.getY(), Colour (0xff4c4c4c), screen.getRight(), screen.getBottom(), false));
    g.drawRoundedRectangle (screen.expanded (3.0f), 14.0f, 2.5f);
    tube.clear();
    tube.addRoundedRectangle (screen, 12.0f);
    g.setGradientFill (ColourGradient (Colour (0xff0f2a17), screen.getCentreX(), screen.getCentreY(),
                                       Colour (0xff030a05), screen.getX(), screen.getY(), true));
    g.fillPath (tube);

    g.setColour (cream.withAlpha (0.85f));
    g.setFont (bold (9.5f).withExtraKerningFactor (0.12f));
    g.drawText (labelText, label.withTrimmedLeft (14.0f).translated (0, -2.0f), Justification::centredLeft);
    const auto lp = Point<float> (label.getRight() - 18.0f, label.getCentreY() - 3.0f);
    g.setColour (phosphor);
    g.fillEllipse (lp.x - 3.5f, lp.y - 3.5f, 7.0f, 7.0f);
    return screen;
}

void drawCrtGlass (Graphics& g, Rectangle<float> screen, const Path& tube)
{
    g.setColour (Colours::black.withAlpha (0.18f));
    for (float y = screen.getY(); y < screen.getBottom(); y += 3.0f)
        g.drawHorizontalLine (roundToInt (y), screen.getX(), screen.getRight());
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.10f), screen.getX(), screen.getY(),
                                       Colours::transparentWhite, screen.getX() + screen.getWidth() * 0.6f, screen.getY() + screen.getHeight() * 0.6f, false));
    g.fillPath (tube);
}

//==============================================================================
EqCurve::EqCurve (RadioFoneProcessor& p) : proc (p)
{
    setInterceptsMouseClicks (false, false);
    const int n = 160;
    hz.resize (n); db.assign (n, -60.0f);
    for (int i = 0; i < n; ++i) hz[(size_t) i] = 40.0f * std::pow (16000.0f / 40.0f, (float) i / (float) (n - 1));
}

void EqCurve::update()
{
    lastDb = db;
    proc.getVoiceCurve (hz.data(), db.data(), (int) hz.size());
    if (db != lastDb) repaint();
}

void EqCurve::paint (Graphics& g)
{
    const Colour phosphor (0xff5dff8a);
    Path tube;
    const auto screen = drawCrtFrame (g, getLocalBounds().toFloat(), String::fromUTF8 ("EQUALIZAÇÃO  ·  APARELHO"), tube);
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (tube);

    const float top = 6.0f, bottom = -42.0f;   // dB
    auto xOf = [&] (float f) { return screen.getX() + 6.0f + (screen.getWidth() - 12.0f) * std::log (f / 40.0f) / std::log (16000.0f / 40.0f); };
    auto yOf = [&] (float d) { return screen.getY() + 6.0f + (screen.getHeight() - 16.0f) * (top - jlimit (bottom, top, d)) / (top - bottom); };

    // retícula: décadas e dB
    g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 9.0f, Font::plain));
    for (float f : { 100.0f, 1000.0f, 10000.0f })
    {
        g.setColour (phosphor.withAlpha (0.18f));
        g.drawVerticalLine (roundToInt (xOf (f)), screen.getY(), screen.getBottom());
        g.setColour (phosphor.withAlpha (0.45f));
        const bool last = f > 5000.0f;
        g.drawText (f >= 1000 ? String ((int) (f / 1000)) + "k" : String ((int) f),
                    Rectangle<float> (30, 10).withCentre ({ xOf (f) + (last ? -18.0f : 18.0f), screen.getBottom() - 9 }),
                    last ? Justification::centredRight : Justification::centredLeft);
    }
    for (float f : { 200.0f, 300.0f, 500.0f, 2000.0f, 3000.0f, 5000.0f })
    {
        g.setColour (phosphor.withAlpha (0.08f));
        g.drawVerticalLine (roundToInt (xOf (f)), screen.getY(), screen.getBottom());
    }
    for (float d : { 0.0f, -12.0f, -24.0f, -36.0f })
    {
        g.setColour (phosphor.withAlpha (d == 0.0f ? 0.22f : 0.10f));
        g.drawHorizontalLine (roundToInt (yOf (d)), screen.getX(), screen.getRight());
    }

    // curva com brilho e preenchimento por baixo
    Path curve, fill;
    for (size_t i = 0; i < hz.size(); ++i)
    {
        const float x = xOf (hz[i]), y = yOf (db[i]);
        if (i == 0) { curve.startNewSubPath (x, y); fill.startNewSubPath (x, screen.getBottom()); }
        else curve.lineTo (x, y);
        fill.lineTo (x, y);
    }
    fill.lineTo (xOf (hz.back()), screen.getBottom());
    fill.closeSubPath();
    g.setGradientFill (ColourGradient (phosphor.withAlpha (0.22f), 0, screen.getY(), phosphor.withAlpha (0.02f), 0, screen.getBottom(), false));
    g.fillPath (fill);
    g.setColour (phosphor.withAlpha (0.2f));
    g.strokePath (curve, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (phosphor);
    g.strokePath (curve, PathStrokeType (1.7f, PathStrokeType::curved, PathStrokeType::rounded));

    drawCrtGlass (g, screen, tube);
}

//==============================================================================
void DigitalMeter::update()
{
    for (int c = 0; c < 2; ++c)
    {
        const float d = 20.0f * std::log10 (proc.getOutPeak (c) + 1e-9f);
        lvl[c] = d > lvl[c] ? d : jmax (d, lvl[c] - 1.2f);       // cai ~36 dB/s
        if (lvl[c] >= hold[c]) { hold[c] = lvl[c]; holdT[c] = 45; }
        else if (--holdT[c] <= 0) hold[c] = jmax (lvl[c], hold[c] - 1.5f);
    }
    const float r = proc.getGainReductionDb();
    gr = r < gr ? r : gr + (r - gr) * 0.15f;
    repaint();
}

void DigitalMeter::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    // visor preto rebaixado
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (b.translated (1.5f, 3.0f), 6.0f);
    g.setColour (Colour (0xff070707));
    g.fillRoundedRectangle (b, 6.0f);
    g.setGradientFill (ColourGradient (Colour (0xffe8e8e8), b.getX(), b.getY(), Colour (0xff4c4c4c), b.getRight(), b.getBottom(), false));
    g.drawRoundedRectangle (b, 6.0f, 2.0f);

    auto inner = b.reduced (8.0f, 10.0f).withTrimmedTop (14.0f);
    auto labels = inner.removeFromBottom (14.0f);
    inner.removeFromBottom (4.0f);
    auto scale = inner.removeFromLeft (26.0f);
    const float barW = (inner.getWidth() - 12.0f) / 3.0f;
    const int segs = 30;
    const float segH = inner.getHeight() / (float) segs;
    const float minDb = -48.0f;
    auto dbToFrac = [&] (float d) { return jlimit (0.0f, 1.0f, (d - minDb) / (0.0f - minDb)); };

    // escala em dB
    g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 9.0f, Font::plain));
    for (float d : { 0.0f, -3.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f })
    {
        const float y = inner.getBottom() - dbToFrac (d) * inner.getHeight();
        g.setColour (d > -6.0f ? Colour (0xffff5a4a) : cream.withAlpha (0.7f));
        g.drawText (String ((int) d), Rectangle<float> (scale.getX(), y - 5, scale.getWidth() - 3, 10), Justification::centredRight);
    }

    auto segColour = [] (float d)
    {
        return d > -3.0f ? Colour (0xffff3b2f) : d > -12.0f ? Colour (0xffffb020) : Colour (0xff3dff6e);
    };
    for (int c = 0; c < 2; ++c)
    {
        const float x = inner.getX() + (float) c * (barW + 6.0f);
        const float lit = dbToFrac (lvl[c]) * (float) segs;
        const int holdSeg = (int) (dbToFrac (hold[c]) * (float) segs);
        for (int k = 0; k < segs; ++k)
        {
            const float d = minDb + (0.0f - minDb) * ((float) k + 0.5f) / (float) segs;
            const auto r = Rectangle<float> (x, inner.getBottom() - (float) (k + 1) * segH + 1.0f, barW, segH - 2.0f);
            const bool on = (float) k < lit || k == holdSeg - 1;
            const auto col = segColour (d);
            g.setColour (on ? col : col.withAlpha (0.10f));
            g.fillRect (r);
            if (on)
            {
                g.setColour (col.withAlpha (0.25f));
                g.fillRect (r.expanded (1.5f, 1.0f));
            }
        }
    }
    // redução de ganho: desce do topo, em âmbar
    {
        const float x = inner.getX() + 2.0f * (barW + 6.0f);
        const float grFrac = jlimit (0.0f, 1.0f, -gr / 12.0f);   // 0..12 dB
        for (int k = 0; k < segs; ++k)
        {
            const auto r = Rectangle<float> (x, inner.getY() + (float) k * segH + 1.0f, barW, segH - 2.0f);
            const bool on = (float) k < grFrac * (float) segs;
            g.setColour (on ? amber : amber.withAlpha (0.10f));
            g.fillRect (r);
        }
    }
    g.setColour (cream.withAlpha (0.8f));
    g.setFont (bold (9.0f));
    const char* names[] = { "L", "R", "GR" };
    for (int c = 0; c < 3; ++c)
        g.drawText (names[c], Rectangle<float> (inner.getX() + (float) c * (barW + 6.0f), labels.getY(), barW, labels.getHeight()), Justification::centred);
    // leitura numérica
    g.setColour (Colour (0xff3dff6e));
    g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 10.0f, Font::bold));
    const float pk = jmax (lvl[0], lvl[1]);
    g.drawText (pk <= -59.0f ? String ("-inf") : String (pk, 1), Rectangle<float> (b.getX(), b.getY() + 6, b.getWidth() * 0.5f, 12), Justification::centred);
    g.setColour (amber);
    g.drawText ("GR " + String (gr, 1), Rectangle<float> (b.getCentreX(), b.getY() + 6, b.getWidth() * 0.5f, 12), Justification::centred);
}

//==============================================================================
void CrtScope::update()
{
    const int N = (int) buf.size(), show = 1024;
    proc.copyScope (buf.data(), N);
    // gatilho: primeira subida pelo zero (com histerese) na primeira metade, para a onda ficar parada
    int start = N - show;
    for (int i = N - show - 1; i > 1; --i)
        if (buf[(size_t) i - 1] < -0.01f && buf[(size_t) i] >= 0.0f) { start = i; break; }
    start = jmin (start, N - show);

    prev2 = prev1; prev1 = trace;
    const int cols = 220;
    trace.assign ((size_t) cols * 2, 0.0f);
    for (int c = 0; c < cols; ++c)
    {
        float lo = 1e9f, hi = -1e9f;
        const int a = start + c * show / cols, b = start + (c + 1) * show / cols;
        for (int i = a; i < b; ++i) { lo = jmin (lo, buf[(size_t) i]); hi = jmax (hi, buf[(size_t) i]); }
        trace[(size_t) c * 2] = lo; trace[(size_t) c * 2 + 1] = hi;
    }
    repaint();
}

void CrtScope::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const Colour phosphor (0xff5dff8a);

    Path tube;
    const auto screen = drawCrtFrame (g, b, String::fromUTF8 ("OSCILOSCÓPIO  ·  SAÍDA"), tube);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (tube);

        // retícula
        g.setColour (phosphor.withAlpha (0.13f));
        for (int i = 1; i < 8; ++i)
        {
            const float x = screen.getX() + screen.getWidth() * (float) i / 8.0f;
            g.drawVerticalLine (roundToInt (x), screen.getY(), screen.getBottom());
        }
        for (int i = 1; i < 4; ++i)
        {
            const float y = screen.getY() + screen.getHeight() * (float) i / 4.0f;
            g.drawHorizontalLine (roundToInt (y), screen.getX(), screen.getRight());
        }
        g.setColour (phosphor.withAlpha (0.22f));
        g.drawHorizontalLine (roundToInt (screen.getCentreY()), screen.getX(), screen.getRight());

        // traço com persistência do fósforo (quadros anteriores mais apagados)
        auto drawTrace = [&] (const std::vector<float>& t, float alpha, bool glow)
        {
            if (t.size() < 4) return;
            const int cols = (int) t.size() / 2;
            const float cy = screen.getCentreY(), amp = screen.getHeight() * 0.45f;
            auto yOf = [&] (float v) { return cy - amp * jlimit (-1.0f, 1.0f, v * 1.6f); };
            Path p;
            for (int c = 0; c < cols; ++c)
            {
                const float x = screen.getX() + 4.0f + (screen.getWidth() - 8.0f) * (float) c / (float) (cols - 1);
                const float mid = 0.5f * (t[(size_t) c * 2] + t[(size_t) c * 2 + 1]);
                if (c == 0) p.startNewSubPath (x, yOf (mid)); else p.lineTo (x, yOf (mid));
                // faixa entre o mínimo e o máximo da coluna (detalhe da onda comprimida)
                g.setColour (phosphor.withAlpha (0.35f * alpha));
                g.drawLine (x, yOf (t[(size_t) c * 2]), x, yOf (t[(size_t) c * 2 + 1]), 1.0f);
            }
            if (glow)
            {
                g.setColour (phosphor.withAlpha (0.18f * alpha));
                g.strokePath (p, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }
            g.setColour (phosphor.withAlpha (alpha));
            g.strokePath (p, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        };
        drawTrace (prev2, 0.18f, false);
        drawTrace (prev1, 0.35f, false);
        drawTrace (trace, 1.0f, true);

        drawCrtGlass (g, screen, tube);
    }

}

//==============================================================================
StationPanel::StationPanel (RadioFoneProcessor& p)
    : proc (p), modeKeys (*p.apvts.getParameter ("mode")), dial (*p.apvts.getParameter ("tune")), scope (p), eqCurve (p), meter (p), throwBtn ("THROW", p.apvts.getParameter ("throw"))
{
    setLookAndFeel (&lnf);
    setSize (kW, kH);

    addAndMakeVisible (modeKeys);
    addAndMakeVisible (dial);
    addAndMakeVisible (scope);
    addAndMakeVisible (eqCurve);
    addAndMakeVisible (meter);
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

    setupKnob (maxThresh, "maxThresh", "GANHO", 2);
    setupKnob (maxCeil, "maxCeil", "TETO", 2);
    setupToggle (maxOn, "maxOn", "LIGADO");
    setupToggle (echoSync, "echoSync", "SYNC");
    setupToggle (echoPP, "echoPP", "PING-PONG");
    setupToggle (bypassBtn, "bypass", "BYPASS");
    bypassBtn.button.getProperties().set ("big", true);
    addAndMakeVisible (throwBtn);
    throwBtn.setTooltip ("Segure para jogar a voz no eco");


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
    plateArea = { (float) topPanel.getX() + 24.0f, 20.0f, 326.0f, 74.0f };
    {
        auto tp = topPanel;
        tp.removeFromLeft (350);
        vuArea = tp.removeFromLeft (190).reduced (6, 16);
        tapeWin = tp.reduced (14, 18);
    }
    lampPos = { plateArea.getRight() - 14.0f, plateArea.getBottom() + 26.0f };
    status.setBounds (tapeWin.getX() + 14, tapeWin.getY() + 8, tapeWin.getWidth() - 28, 18);
    // BYPASS grande no painel de cima, ao lado da lâmpada NO AR
    bypassBtn.button.setBounds ((int) plateArea.getX() + 4, topPanel.getBottom() - 44, 132, 36);

    auto rows = content.reduced (8, 0).withTrimmedTop (10).withTrimmedBottom (8);

    // lateral direita: MAXIMIZER com VU digital
    maxiArea = rows.removeFromRight (156);
    rows.removeFromRight (10);
    {
        auto m = maxiArea.reduced (8, 4);
        m.removeFromTop (22);                   // legenda
        auto controls = m.removeFromBottom (150);
        meter.setBounds (m.reduced (6, 8));
        maxOn.button.setBounds (controls.removeFromBottom (30).reduced (8, 0));
        controls.removeFromBottom (6);
        place (maxThresh, controls.removeFromLeft (controls.getWidth() / 2), 15);
        place (maxCeil, controls, 15);
    }

    auto rowA = rows.removeFromTop (252);
    rows.removeFromTop (6);
    auto rowB = rows;

    // linha A: teclas dos aparelhos | dial + controles | osciloscópio e curva
    modeKeys.setBounds (rowA.removeFromLeft (226).reduced (14, 2).withTrimmedBottom (20));
    rowA.removeFromLeft (6);
    {
        auto crt = rowA.removeFromRight (270);
        scope.setBounds (crt.removeFromTop (crt.getHeight() / 2).reduced (2, 1));
        eqCurve.setBounds (crt.reduced (2, 1));
    }
    rowA.removeFromRight (8);
    dial.setBounds (rowA.removeFromTop (96).reduced (4, 2));
    rowA.removeFromTop (8);
    auto knobsA = rowA;
    const int wm = knobsA.getWidth() / 4;
    for (auto* k : { &band, &drive, &noise, &defect })
        place (*k, knobsA.removeFromLeft (wm), 18);

    // linha B: placa do ECO DE FITA à esquerda, mistura/saída à direita
    echoPlate = rowB.removeFromLeft (600);
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
    btnCol = btnCol.withSizeKeepingCentre (btnCol.getWidth(), jmin (btnCol.getHeight(), 138));
    echoSync.button.setBounds (btnCol.removeFromTop (30));
    btnCol.removeFromTop (8);
    echoPP.button.setBounds (btnCol.removeFromTop (30));
    btnCol.removeFromTop (10);
    throwBtn.setBounds (btnCol.removeFromTop (jmin (52, btnCol.getHeight())));

    rightArea = rowB.withTrimmedLeft (8);
    auto ra = rightArea.reduced (6, 4);
    ra.removeFromTop (20);                      // legenda
    const int ws = ra.getWidth() / 3;
    for (auto* k : { &mix, &dry, &out })
        place (*k, ra.removeFromLeft (ws), 15);

    renderBackground();
}

//==============================================================================
String StationPanel::statusText()
{
    if (modeKeys.getHover() >= 0)
        return String::fromUTF8 ("APARELHO:  clique para escolher");
    if (dial.isMouseOverOrDragging())
        return "SINTONIA:  " + dial.getText();
    Knob* all[] = { &band, &drive, &noise, &defect, &echoDiv, &echoMs, &echoHeads,
                    &echoFb, &echoTone, &echoWow, &echoSend, &mix, &dry, &out, &maxThresh, &maxCeil };
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

    // a fita roda sempre, como no Dub Station: mais rápida quando o eco é mais curto
    const float rate = jlimit (0.0f, 1.0f, (0.9f - proc.getEchoDelaySec()) / 0.85f);
    tapePhase += 0.6f + 2.2f * rate;
    if (tapePhase > 1.0e6f) tapePhase -= 1.0e6f;

    status.setText (statusText(), dontSendNotification);
    scope.update();
    eqCurve.update();
    meter.update();
    dial.setExcited (proc.getVoicePeakHz(), (float) band.slider.getValue() / 100.0f);
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
    // nome completo (a fonte encolhe até caber) + modelo e versão
    {
        const String title ("MASTOR PHONE VOX");
        const float maxW = pl.getWidth() - 34.0f;
        float fs = 27.0f;
        while (fs > 14.0f && GlyphArrangement::getStringWidth (mf::title (fs).withExtraKerningFactor (0.03f), title) > maxW)
            fs -= 0.5f;
        g.setFont (mf::title (fs).withExtraKerningFactor (0.03f));
        engraved (title, { pl.getX() + 15, pl.getY() + 9, maxW + 4, 36 }, Justification::centredLeft, Colour (0xff262626));
    }
    g.setFont (bold (9.5f).withExtraKerningFactor (0.12f));
    engraved (String::fromUTF8 ("MPV-01  ·  ") + kVersion + String::fromUTF8 ("  ·  RÁDIO AM  ·  TELEFONE  ·  MEGAFONE"),
              { pl.getX() + 17, pl.getY() + 48, pl.getWidth() - 30, 14 }, Justification::centredLeft, Colour (0xff3a3a3a));

    // origem + lâmpada NO AR
    g.setColour (ink.withAlpha (0.85f));
    g.setFont (bold (10.0f).withExtraKerningFactor (0.1f));
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
    g.setColour (amber);
    g.setFont (bold (13.0f).withExtraKerningFactor (0.08f));
    g.drawText ("MAXIMIZER", maxiArea.toFloat().reduced (8, 8).withHeight (18.0f), Justification::centred);

    drawTape (g);
    drawVuNeedle (g);
}

void StationPanel::drawTape (Graphics& g)
{
    // transporte de fita igual ao do Dub Station 2.1: a fita corre sempre e os roletes giram junto
    auto win = tapeWin.toFloat();
    // recessed smoked-glass window
    g.setColour (Colour (0xff0d0f0e));
    g.fillRoundedRectangle (win, 10.0f);

    const float L = win.getX() + 70, R = win.getRight() - 70;
    const float T = win.getY() + 38, B = win.getY() + 128;
    const float cy = (T + B) * 0.5f, rad = (B - T) * 0.5f;

    // tape loop path
    Path loop;
    loop.addRoundedRectangle (L - rad, T, (R - L) + 2 * rad, B - T, rad);
    const float perim = loop.getLength();

    g.setColour (Colours::black.withAlpha (0.55f));                 // shadow of the tape on the deck
    g.strokePath (loop, PathStrokeType (9.0f), AffineTransform::translation (1.5f, 3.5f));
    g.setColour (Colour (0xff1e1008));                               // tape edges
    g.strokePath (loop, PathStrokeType (9.0f));
    g.setColour (Colour (0xff5a361b));                               // oxide coating
    g.strokePath (loop, PathStrokeType (7.0f));
    g.setColour (Colour (0xff7d4f2a).withAlpha (0.85f));             // sheen band along the middle
    g.strokePath (loop, PathStrokeType (2.6f));

    // moving grain: irregular light/dark specks that travel with the tape (speed = repeat rate)
    {
        const float step = 3.5f;
        const int base = (int) std::floor (tapePhase / step);
        const float off = tapePhase - (float) base * step;
        int k = 0;
        for (float d = off; d < perim; d += step, ++k)
        {
            const auto id = (uint32) (k - base);
            const uint32 h = (id * 2654435761u) ^ (id >> 7);
            const float a = (float) ((h >> 8) & 255) / 255.0f;
            if (a < 0.45f) continue;
            const auto pt = loop.getPointAlongPath (d);
            const bool light = (h & 1) != 0;
            const float jitter = (float) ((h >> 16) & 7) / 7.0f * 4.0f - 2.0f;  // across the tape width
            const auto nxt = loop.getPointAlongPath (jmin (perim, d + 0.8f));
            const auto dirv = (nxt - pt);
            const float len = jmax (0.001f, std::sqrt (dirv.x * dirv.x + dirv.y * dirv.y));
            const Point<float> nrm (-dirv.y / len, dirv.x / len);
            const auto q = pt + nrm * jitter;
            g.setColour (light ? Colour (0xffb07a46).withAlpha (0.25f + 0.35f * a) : Colour (0xff1a0d05).withAlpha (0.20f + 0.30f * a));
            g.fillEllipse (q.x - 1.2f, q.y - 0.8f, 2.4f, 1.6f);
        }
    }
    g.setColour (Colours::white.withAlpha (0.10f));                  // thin specular edge
    g.strokePath (loop, PathStrokeType (1.0f), AffineTransform::translation (0.0f, -3.0f));

    // flanged chrome guide wheels, rubber pinch roller, polished capstan
    auto wheel = [&g] (Point<float> c, float r, float spin, bool rubber)
    {
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - r + 2, c.y - r + 3, r * 2, r * 2);
        if (rubber)
        {
            g.setGradientFill (ColourGradient (Colour (0xff2c2c2c), c.x - r, c.y - r, Colour (0xff050505), c.x + r, c.y + r, false));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (Colours::white.withAlpha (0.10f));
            Path sheen;
            sheen.addCentredArc (c.x, c.y, r * 0.8f, r * 0.8f, 0.0f, spin, spin + 1.2f, true);
            g.strokePath (sheen, PathStrokeType (1.5f));
        }
        else
        {
            g.setGradientFill (ColourGradient (Colour (0xfff2f2f0), c.x - r, c.y - r, Colour (0xff5c5c5a), c.x + r, c.y + r, false));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (Colours::black.withAlpha (0.35f));
            g.drawEllipse (c.x - r * 0.86f, c.y - r * 0.86f, r * 1.72f, r * 1.72f, 1.0f); // flange step
        }
        const float hub = r * (rubber ? 0.45f : 0.62f);
        g.setGradientFill (ColourGradient (Colour (0xffe6e6e4), c.x - hub, c.y - hub, Colour (0xff6e6e6c), c.x + hub, c.y + hub, false));
        g.fillEllipse (c.x - hub, c.y - hub, hub * 2, hub * 2);
        g.setColour (Colour (0xff1c1c1c));                         // rotating holes
        for (int i = 0; i < 3; ++i)
        {
            const auto hp = c.getPointOnCircumference (hub * 0.55f, spin + (float) i * 2.0944f);
            const float hr = hub * 0.2f;
            g.fillEllipse (hp.x - hr, hp.y - hr, hr * 2, hr * 2);
        }
        g.setColour (Colour (0xff333333));
        g.fillEllipse (c.x - 2.2f, c.y - 2.2f, 4.4f, 4.4f);
        g.setColour (Colours::white.withAlpha (0.6f));
        g.fillEllipse (c.x - r * 0.55f, c.y - r * 0.62f, r * 0.35f, r * 0.22f);
    };
    // wheels roll with the tape: angle = travelled distance / radius
    wheel ({ L - rad + 26.0f, cy }, 22.0f, tapePhase / 22.0f, false);
    wheel ({ R + rad - 26.0f, cy }, 22.0f, tapePhase / 22.0f, false);
    wheel ({ R - 20.0f, B - 12.0f }, 14.0f, -tapePhase / 14.0f, true);   // pinch roller
    {
        const auto cap = Point<float> (R - 20.0f, B + 12.0f);          // capstan shaft
        g.setGradientFill (ColourGradient (Colour (0xffffffff), cap.x - 5, cap.y - 5, Colour (0xff707070), cap.x + 5, cap.y + 5, false));
        g.fillEllipse (cap.x - 5.5f, cap.y - 5.5f, 11.0f, 11.0f);
        g.setColour (Colour (0xff2a2a2a));
        g.drawLine ({ cap, cap.getPointOnCircumference (5.0f, tapePhase / 5.5f) }, 1.4f);
    }

    // heads along the bottom run: ERASE, REC, 1, 2, 3 (metal shields with a gap and two screws)
    const int mask = headMasks[jlimit (0, 6, (int) echoHeads.slider.getValue())];
    const char* hn[] = { "ERASE", "REC", "1", "2", "3" };
    g.setFont (FontOptions (9.5f, Font::bold));
    for (int h = 0; h < 5; ++h)
    {
        const float hx = L + 40.0f + (float) h * ((R - L - 90.0f) / 4.0f);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (hx - 11, B + 1, 24, 20, 3.0f);
        g.setGradientFill (ColourGradient (Colour (0xffd8d8d6), hx - 12, B - 2,
                                                 Colour (0xff56565a), hx + 12, B + 20, false));
        g.fillRoundedRectangle (hx - 12, B - 2, 24, 20, 3.0f);
        g.setColour (Colour (0xff101010));
        g.drawLine (hx, B - 2, hx, B + 5, 1.2f);                         // head gap
        g.setColour (Colour (0xff3a3a3a));
        g.fillEllipse (hx - 9, B + 11, 3.5f, 3.5f);
        g.fillEllipse (hx + 5.5f, B + 11, 3.5f, 3.5f);
        const bool on = h <= 1 ? (h == 1) : (mask & (1 << (h - 2))) != 0;
        g.setColour (on ? amber : lampOff);
        g.fillEllipse (hx - 4, B + 24, 8, 8);
        g.setColour (cream.withAlpha (0.75f));
        g.drawText (hn[h], Rectangle<float> (hx - 22, B + 33, 44, 11), Justification::centred);
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
    g.setFont (mf::title (21.0f));
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
    }

    {   // plaquinha de latão "DUB STATION", padrão da linha Dub Station
        const auto pl = plateArea;
        Random rng (77);
        const auto f = mf::title (15.0f).withExtraKerningFactor (0.30f);
        const String t ("DUB STATION");
        const float tw = GlyphArrangement::getStringWidth (f, t);
        auto gp = Rectangle<float> (pl.getX() + 2.0f, pl.getBottom() + 10.0f, tw + 64.0f, 32.0f);
        g.setColour (Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (gp.translated (1.5f, 3.0f), 3.0f);
        ColourGradient brass (Colour (0xffe3cd85), gp.getX(), gp.getY(), Colour (0xff8a6d2c), gp.getX(), gp.getBottom(), false);
        brass.addColour (0.45, Colour (0xffc7a95a));
        brass.addColour (0.55, Colour (0xffb39446));
        g.setGradientFill (brass);
        g.fillRoundedRectangle (gp, 3.0f);
        for (int i = 0; i < 70; ++i)
        {
            const float yy = gp.getY() + 1.0f + rng.nextFloat() * (gp.getHeight() - 2.0f);
            const float x0 = gp.getX() + rng.nextFloat() * gp.getWidth() * 0.5f;
            g.setColour ((rng.nextBool() ? Colours::white : Colour (0xff5a4316)).withAlpha (0.06f + rng.nextFloat() * 0.08f));
            g.drawHorizontalLine ((int) yy, x0, jmin (gp.getRight() - 1.0f, x0 + 20.0f + rng.nextFloat() * gp.getWidth() * 0.6f));
        }
        g.setColour (Colours::white.withAlpha (0.55f));
        g.drawLine (gp.getX() + 2, gp.getY() + 1, gp.getRight() - 2, gp.getY() + 1, 1.0f);
        g.setColour (Colour (0xff3d2c0c).withAlpha (0.55f));
        g.drawRoundedRectangle (gp, 3.0f, 0.8f);
        for (float rx : { gp.getX() + 12.0f, gp.getRight() - 12.0f })
        {
            const auto rv = Rectangle<float> (9, 9).withCentre ({ rx, gp.getCentreY() });
            g.setGradientFill (ColourGradient (Colour (0xfff0dfa6), rv.getX(), rv.getY(), Colour (0xff6b5220), rv.getRight(), rv.getBottom(), false));
            g.fillEllipse (rv);
        }
        g.setFont (f);
        g.setColour (Colours::white.withAlpha (0.45f));
        g.drawText (t, gp.translated (0.8f, 1.0f), Justification::centred);
        g.setColour (Colour (0xff2e2108));
        g.drawText (t, gp, Justification::centred);
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
    engrave (kWood + 20.0f, divY, (float) maxiArea.getX() - 12.0f, divY);
    engrave ((float) maxiArea.getX() - 6.0f, tH + 14.0f, (float) maxiArea.getX() - 6.0f, h - 14.0f);

    // placa rebaixada do maximizer
    {
        auto mp = maxiArea.toFloat().reduced (2.0f, 2.0f);
        g.setColour (Colour (0xff0b0a09).withAlpha (0.7f));
        g.fillRoundedRectangle (mp, 6.0f);
        g.setColour (Colours::white.withAlpha (0.07f));
        g.drawRoundedRectangle (mp.translated (0, 1), 6.0f, 1.0f);
        g.setColour (Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (mp, 6.0f, 1.0f);
    }
    engrave ((float) echoPlate.getRight() + 4.0f, (float) echoPlate.getY() + 6.0f, (float) echoPlate.getRight() + 4.0f, (float) echoPlate.getBottom() - 4.0f);

    // placa rebaixada do eco
    auto plate = echoPlate.toFloat().reduced (4.0f, 2.0f);
    g.setColour (Colour (0xff0b0a09).withAlpha (0.7f));
    g.fillRoundedRectangle (plate, 6.0f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (plate.translated (0, 1), 6.0f, 1.0f);
    g.setColour (Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (plate, 6.0f, 1.0f);

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
