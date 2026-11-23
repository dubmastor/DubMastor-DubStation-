// Visual Dub Station do Mastor Half Dub MHD-050 (mesmo acabamento do Tape Echo e do Phone Vox)
#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

//==============================================================================
namespace
{
    struct Fonts
    {
        Typeface::Ptr title  = Typeface::createSystemTypefaceFor (BinaryData::ArchivoBlackRegular_ttf, (size_t) BinaryData::ArchivoBlackRegular_ttfSize);
        Typeface::Ptr script = Typeface::createSystemTypefaceFor (BinaryData::KaushanScriptRegular_ttf, (size_t) BinaryData::KaushanScriptRegular_ttfSize);
    };
    Fonts& fonts() { static SharedResourcePointer<Fonts> f; return *f; }

    // versão "x.y" a partir da versão do CMakeLists (project(HalfDub VERSION x.y.0))
    String versionXY()
    {
        auto parts = StringArray::fromTokens (JucePlugin_VersionString, ".", "");
        return parts.size() >= 2 ? parts[0] + "." + parts[1] : String (JucePlugin_VersionString);
    }

    const Colour lampOff (0xff4a3418);
    const Colour bandCol[3] { Colour (0xff4fae5a), Colour (0xffe6b83a), Colour (0xffd0402f) };

    // escala do VU: linear em tensão de -20 VU a +3 VU, ponteiro abrindo ±42°
    constexpr float kVuSwing = 0.7330f;
    float vuToPos (float vu)
    {
        const float a = std::pow (10.0f, jlimit (-30.0f, 8.0f, vu) / 20.0f);
        return (a - 0.1f) / (1.4125f - 0.1f);
    }

    const char* kBkSteps[12] = { "OFF", "70", "100", "150", "250", "500", "1k", "2k", "3k", "5k", "7.5k", "10k" };
}

const Colour hdui::cream    (0xffe9e2cc);
const Colour hdui::amber    (0xfff39a2c);
const Colour hdui::ink      (0xff2a2a2a);
const Colour hdui::chipText (0xffdddddd);
const Colour hdui::phosphor (0xff3fcf6a);

Font hdui::title (float h)  { return Font (FontOptions (fonts().title).withHeight (h)); }
Font hdui::script (float h) { return Font (FontOptions (fonts().script).withHeight (h)); }
Font hdui::bold (float h)   { return Font (FontOptions (h, Font::bold)); }
Font hdui::mono (float h, bool b) { return Font (FontOptions (Font::getDefaultMonospacedFontName(), h, b ? Font::bold : Font::plain)); }

void hdui::drawChip (Graphics& g, Rectangle<float> r, const String& text, bool on, bool over, float fontH, bool lamp)
{
    r = r.reduced (1.5f);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (0, 3.0f), 6.0f);
    if (on)
    {
        g.setColour (amber.withAlpha (0.35f));
        g.fillRoundedRectangle (r.expanded (2.5f), 8.0f);
        g.setGradientFill (ColourGradient (Colour (0xfff5b04c), 0, r.getY(), Colour (0xffd9821e), 0, r.getBottom(), false));
    }
    else
        g.setGradientFill (ColourGradient (Colour (over ? 0xff3a3a3a : 0xff2d2d2d), 0, r.getY(), Colour (0xff121212), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on ? Colour (0xffffcf85) : Colour (0xff888888));
    g.drawRoundedRectangle (r, 6.0f, 1.8f);

    auto tr = r;
    if (lamp)
    {
        const auto lp = Point<float> (r.getX() + 15.0f, r.getCentreY());
        g.setColour (on ? Colour (0xffffe0a0) : Colour (0xff4a1a12));
        g.fillEllipse (lp.x - 5, lp.y - 5, 10, 10);
        if (on) { g.setColour (Colour (0x66ffffff)); g.fillEllipse (lp.x - 2.5f, lp.y - 3.5f, 4, 3); }
        tr = tr.withTrimmedLeft (22.0f);
    }
    g.setColour (on ? Colour (0xff1a1008) : chipText);
    g.setFont (bold (fontH).withExtraKerningFactor (0.10f));
    g.drawFittedText (text, tr.toNearestInt().reduced (3, 0), Justification::centred, 1);
}

//==============================================================================
HalfDubLookAndFeel::HalfDubLookAndFeel()
{
    setColour (Label::textColourId, hdui::cream);
}

void HalfDubLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                           float a0, float a1, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    if ((bool) s.getProperties()["bigknob"]) { drawBigKnob (g, b, pos, a0, a1, s); return; }

    using namespace hdui;
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const bool stepped = s.getInterval() >= 1.0;
    const auto labels = StringArray::fromTokens (s.getProperties()["scaleLabels"].toString(), "|", "");
    const int steps = stepped ? (int) (s.getMaximum() - s.getMinimum()) + 1 : (labels.isEmpty() ? 11 : labels.size());
    const int sel = roundToInt (pos * (float) (steps - 1));
    const float ang = stepped ? a0 + (float) sel / (float) (steps - 1) * (a1 - a0) : a0 + pos * (a1 - a0);
    const bool big = r > 70.0f;

    // escala impressa
    for (int i = 0; i < steps; ++i)
    {
        const float t = a0 + (float) i / (float) (steps - 1) * (a1 - a0);
        const bool lit = stepped && i == sel;
        g.setColour (lit ? amber : Colour (0xffcccccc));
        g.drawLine ({ c.getPointOnCircumference (r * (big ? 0.66f : 0.70f), t), c.getPointOnCircumference (r * (big ? 0.76f : (i % 5 == 0 ? 0.84f : 0.78f)), t) },
                    lit ? 3.0f : (i % 5 == 0 ? 2.0f : 1.3f));
        String txt;
        if (labels.size() == steps) txt = labels[i];
        else if (! stepped && (i == 0 || i == steps - 1 || i == steps / 2)) txt = String (i);
        if (txt.isNotEmpty() && r > 28.0f)
        {
            const auto np = c.getPointOnCircumference (r * (labels.isEmpty() ? 0.97f : 0.90f), t);
            const float bw = labels.isEmpty() ? r * 0.4f : r * 0.5f;
            g.setColour (lit ? amber : (txt == "REV" ? Colour (0xff6fe08a) : Colour (0xffdddddd)));
            g.setFont (bold (labels.isEmpty() ? 11.0f : (lit ? 19.0f : 16.0f)));
            g.drawText (txt, Rectangle<float> (np.x - bw / 2, np.y - 10, bw, 20), Justification::centred);
        }
    }

    // saia de alumínio + tampa preta (como no layout aprovado)
    const float kr = r * 0.62f;
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - kr, c.y - kr + 3, kr * 2, kr * 2);
    g.setGradientFill (ColourGradient (Colour (0xfffafafa), c.x - kr * 0.3f, c.y - kr * 0.5f, Colour (0xff7a7a7a), c.x + kr, c.y + kr, true));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    const float cr = kr * 0.72f;
    g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), c.x - cr * 0.3f, c.y - cr * 0.5f, Colour (0xff050505), c.x + cr, c.y + cr, true));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    g.setColour (Colours::black);
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 1.0f);
    g.setColour (Colours::white);
    g.drawLine ({ c.getPointOnCircumference (kr * 0.25f, ang), c.getPointOnCircumference (kr * 0.66f, ang) }, jmax (2.5f, kr * 0.08f));
}

void HalfDubLookAndFeel::drawBigKnob (Graphics& g, Rectangle<float> b, float pos, float a0, float a1, Slider& s)
{
    using namespace hdui;
    const auto c = b.getCentre();
    const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float r = R * 0.64f;                         // aro prateado
    const bool stepped = (bool) s.getProperties()["stepped"];
    const int n = 12;
    const int sel = roundToInt (pos * (float) (n - 1));
    const float ang = stepped ? a0 + (float) sel / (float) (n - 1) * (a1 - a0) : a0 + pos * (a1 - a0);

    // escala: degraus de mesa (STEP) ou contínua 20 Hz–20 kHz
    for (int i = 0; i < n; ++i)
    {
        const float t = a0 + (float) i / (float) (n - 1) * (a1 - a0);
        const bool lit = stepped && i == sel;
        g.setColour (lit ? amber : Colour (0xffcccccc));
        g.drawLine ({ c.getPointOnCircumference (r + 8.0f, t), c.getPointOnCircumference (r + 18.0f, t) }, lit ? 3.0f : 2.5f);
        if (stepped)
        {
            const auto np = c.getPointOnCircumference (r + 36.0f, t);
            g.setFont (bold (lit ? 15.0f : 13.0f));
            g.drawText (kBkSteps[i], Rectangle<float> (np.x - 22, np.y - 9, 44, 18), Justification::centred);
        }
    }
    if (! stepped)
    {
        const char* cl[5] = { "20", "100", "1k", "5k", "20k" };
        const float cf[5] = { 20.0f, 100.0f, 1000.0f, 5000.0f, 20000.0f };
        g.setColour (Colour (0xffdddddd));
        g.setFont (bold (13.0f));
        for (int i = 0; i < 5; ++i)
        {
            const float t = a0 + std::log (cf[i] / 20.0f) / std::log (1000.0f) * (a1 - a0);
            const auto np = c.getPointOnCircumference (r + 36.0f, t);
            g.drawText (cl[i], Rectangle<float> (np.x - 22, np.y - 9, 44, 18), Justification::centred);
        }
    }

    // aro prateado serrilhado
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - r, c.y - r + 4, r * 2, r * 2);
    g.setGradientFill (ColourGradient (Colour (0xfffbfbfb), c.x - r * 0.3f, c.y - r * 0.5f, Colour (0xff6c6c6c), c.x + r, c.y + r, true));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (Colours::black.withAlpha (0.25f));
    for (int k = 0; k < 60; ++k)
    {
        const float t = (float) k / 60.0f * MathConstants<float>::twoPi;
        g.drawLine ({ c.getPointOnCircumference (r - 4.0f, t), c.getPointOnCircumference (r - 12.0f, t) }, 1.5f);
    }
    // centro preto com frisos
    const float cr = r * 0.78f;
    g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), c.x - cr * 0.2f, c.y - cr * 0.4f, Colour (0xff050505), c.x + cr, c.y + cr, true));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    g.setColour (Colours::black);
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 2.0f);
    g.setColour (Colour (0xff111111));
    for (int k = 0; k < 36; ++k)
    {
        const float t = (float) k / 36.0f * MathConstants<float>::twoPi;
        g.drawLine ({ c.getPointOnCircumference (r * 0.66f, t), c.getPointOnCircumference (r * 0.74f, t) }, 3.0f);
    }
    // ponteiro creme + miolo amarelo
    g.setColour (Colour (0xfff4f0e0));
    g.drawLine ({ c.getPointOnCircumference (r * 0.28f, ang), c.getPointOnCircumference (r * 0.72f, ang) }, 6.0f);
    const float yr = r * 0.26f;
    g.setGradientFill (ColourGradient (Colour (0xffffe680), c.x - yr * 0.4f, c.y - yr * 0.5f, Colour (0xffc9a11a), c.x + yr, c.y + yr, true));
    g.fillEllipse (c.x - yr, c.y - yr, yr * 2, yr * 2);
}

void HalfDubLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool down)
{
    const bool on = b.getToggleState();
    if ((bool) b.getProperties()["big"])
    {
        // tecla grande de BYPASS: acende vermelho quando o efeito está desligado
        auto r = b.getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (r.translated (1.5f, 4.0f), 7.0f);
        g.setGradientFill (ColourGradient (Colour (0xffd8d8d8), r.getX(), r.getY(), Colour (0xff6a6a6a), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 7.0f);
        auto cap = r.reduced (3.0f).translated (0, down ? 1.0f : 0.0f);
        g.setGradientFill (ColourGradient (Colour (on ? 0xffe0442a : (over ? 0xff363636 : 0xff2b2b2b)), 0, cap.getY(),
                                           Colour (on ? 0xff7a1a0e : 0xff0d0d0d), 0, cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 5.0f);
        const auto lp = Point<float> (cap.getX() + 40.0f, cap.getCentreY());
        g.setColour (on ? Colour (0xffffd0a0) : Colour (0xff4a1a12));
        g.fillEllipse (lp.x - 6, lp.y - 6, 12, 12);
        g.setColour (Colour (0xffeeeeee));
        g.setFont (hdui::bold (17.0f).withExtraKerningFactor (0.18f));
        g.drawText (b.getButtonText(), cap.withTrimmedLeft (30.0f), Justification::centred);
        return;
    }
    const float fh = b.getProperties().contains ("fontH") ? (float) b.getProperties()["fontH"] : 12.0f;
    const bool lamp = ! (bool) b.getProperties()["noLamp"];
    hdui::drawChip (g, b.getLocalBounds().toFloat(), b.getButtonText(), on, over || down, fh, lamp);
}

//==============================================================================
ChoiceChips::ChoiceChips (RangedAudioParameter& p, StringArray l, int columns, float fh)
    : param (p), att (p, [this] (float v) { value = roundToInt (v); repaint(); }), labels (std::move (l)), cols (jmax (1, columns)), fontH (fh)
{
    att.sendInitialUpdate();
}

Rectangle<float> ChoiceChips::cell (int i) const
{
    const int rows = (labels.size() + cols - 1) / cols;
    const float gap = 6.0f;
    const float w = ((float) getWidth() - gap * (float) (cols - 1)) / (float) cols;
    const float h = ((float) getHeight() - gap * (float) (rows - 1)) / (float) rows;
    return { (float) (i % cols) * (w + gap), (float) (i / cols) * (h + gap), w, h };
}

int ChoiceChips::indexAt (Point<float> p) const
{
    for (int i = 0; i < labels.size(); ++i) if (cell (i).contains (p)) return i;
    return -1;
}

void ChoiceChips::paint (Graphics& g)
{
    for (int i = 0; i < labels.size(); ++i)
        hdui::drawChip (g, cell (i), labels[i], i == value, i == hover, fontH);
}

void ChoiceChips::mouseDown (const MouseEvent& e)
{
    const int i = indexAt (e.position);
    if (i >= 0) att.setValueAsCompleteGesture ((float) i);
}

void ChoiceChips::mouseMove (const MouseEvent& e)
{
    const int i = indexAt (e.position);
    if (i != hover) { hover = i; repaint(); }
}

//==============================================================================
LoopStepper::LoopStepper (RangedAudioParameter& p)
    : param (p), att (p, [this] (float v) { value = roundToInt (v); repaint(); })
{
    att.sendInitialUpdate();
}

void LoopStepper::step (int d)
{
    const int n = roundToInt (param.getNormalisableRange().end) + 1;
    att.setValueAsCompleteGesture ((float) jlimit (0, n - 1, value + d));
}

void LoopStepper::mouseDown (const MouseEvent& e) { step (e.position.x < (float) getWidth() * 0.5f ? -1 : 1); }
void LoopStepper::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) { step (w.deltaY > 0 ? 1 : -1); }

void LoopStepper::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0c0c0c));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (Colour (0xff444444));
    g.drawRoundedRectangle (r.reduced (0.75f), 5.0f, 1.5f);
    g.setColour (hdui::amber);
    g.setFont (hdui::mono (14.0f, true));
    g.drawText (String::fromUTF8 ("◂"), r.removeFromLeft (26.0f), Justification::centred);
    g.drawText (String::fromUTF8 ("▸"), r.removeFromRight (26.0f), Justification::centred);
    g.drawText (HalfDubProcessor::loopNames()[jlimit (0, 7, value)], r, Justification::centred);
}

//==============================================================================
DropButton::DropButton (HalfDubProcessor& p)
    : proc (p), drop (*p.apvts.getParameter ("drop")), latch (*p.apvts.getParameter ("latch"))
{
    setRepaintsOnMouseActivity (false);
}

void DropButton::setDrop (bool on)
{
    drop.beginChangeGesture();
    drop.setValueNotifyingHost (on ? 1.0f : 0.0f);
    drop.endChangeGesture();
}

void DropButton::mouseDown (const MouseEvent& e)
{
    if (e.position.getDistanceFrom (getLocalBounds().toFloat().getCentre()) > (float) getWidth() * 0.5f) return;
    pressed = true;
    if (latch.getValue() > 0.5f) setDrop (drop.getValue() < 0.5f);
    else setDrop (true);
    repaint();
}

void DropButton::mouseUp (const MouseEvent&)
{
    if (! pressed) return;
    pressed = false;
    if (latch.getValue() < 0.5f) setDrop (false);
    repaint();
}

void DropButton::update()
{
    const bool on = drop.getValue() > 0.5f || proc.isMidiDropHeld();
    if (on != lit) { lit = on; repaint(); }
}

void DropButton::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const float R = b.getWidth() * 0.5f;
    // base preta com aro de cobre
    g.setColour (Colours::black.withAlpha (0.7f));
    g.fillEllipse (b.reduced (2).translated (0, 6));
    g.setGradientFill (ColourGradient (Colour (0xff2a2a2a), c.x, c.y - R * 0.1f, Colour (0xff0a0a0a), c.x + R, c.y + R, true));
    g.fillEllipse (b.reduced (2));
    g.setColour (Colour (0xffb87a2e));
    g.drawEllipse (b.reduced (3.5f), 3.0f);

    const float r = R * 0.79f;
    // aro cromado
    g.setColour (Colour (0xff555555));
    g.fillEllipse (c.x - r - 9, c.y - r - 9, (r + 9) * 2, (r + 9) * 2);
    g.setColour (Colour (0xffcfcfcf));
    g.fillEllipse (c.x - r - 6, c.y - r - 6, (r + 6) * 2, (r + 6) * 2);
    if (lit)
    {
        g.setGradientFill (ColourGradient (Colour (0x88e8402a), c.x, c.y, Colour (0x00e8402a), c.x + r + 40, c.y, true));
        g.fillEllipse (c.x - r - 40, c.y - r - 40, (r + 40) * 2, (r + 40) * 2);
    }
    const float press = pressed ? 2.0f : 0.0f;
    ColourGradient cap (Colour (lit ? 0xffff9a7a : 0xff8a4a3e), c.x - r * 0.25f, c.y - r * 0.45f + press,
                        Colour (lit ? 0xff6a140a : 0xff2a0a06), c.x + r, c.y + r, true);
    cap.addColour (0.5, Colour (lit ? 0xffd0402f : 0xff5a2018));
    g.setGradientFill (cap);
    g.fillEllipse (c.x - r, c.y - r + press, r * 2, r * 2);
    g.setColour (Colours::black.withAlpha (0.25f));
    g.drawEllipse (c.x - r, c.y - r + press, r * 2, r * 2, 2.0f);

    g.setFont (hdui::title (40.0f).withExtraKerningFactor (0.08f));
    g.setColour (Colours::black.withAlpha (0.4f));
    g.drawText ("DROP", Rectangle<float> (c.x - r, c.y - 32 + 2 + press, r * 2, 50), Justification::centred);
    g.setColour (lit ? Colours::white : Colour (0xffd8b0a8));
    g.drawText ("DROP", Rectangle<float> (c.x - r, c.y - 32 + press, r * 2, 50), Justification::centred);
    g.setFont (hdui::bold (12.0f).withExtraKerningFactor (0.25f));
    g.setColour (lit ? Colours::white : Colour (0xffc89a90));
    g.drawText (lit ? String::fromUTF8 ("● LIGADO") : String ("DESLIGADO"), Rectangle<float> (c.x - r, c.y + 18 + press, r * 2, 18), Justification::centred);
}

//==============================================================================
PadButton::PadButton (HalfDubProcessor& p, const String& id, int i, const String& n, const String& nt)
    : proc (p), param (*p.apvts.getParameter (id)), index (i), name (n), note (nt) {}

void PadButton::set (bool on)
{
    param.beginChangeGesture();
    param.setValueNotifyingHost (on ? 1.0f : 0.0f);
    param.endChangeGesture();
    update();
}

void PadButton::update()
{
    const bool h = proc.engine.padNow.load() == index || param.getValue() > 0.5f;
    if (h != hot) { hot = h; repaint(); }
}

void PadButton::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (Colours::black.withAlpha (0.7f));
    g.fillRoundedRectangle (r.translated (0, 4), 8.0f);
    if (hot)
    {
        g.setColour (hdui::amber.withAlpha (0.45f));
        g.fillRoundedRectangle (r.expanded (3.0f), 10.0f);
    }
    g.setGradientFill (ColourGradient (Colour (hot ? 0xff4a3c2c : 0xff3a3532), 0, r.getY(), Colour (0xff1b1917), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (hot ? Colour (0xffffcf85) : Colour (0xff777777));
    g.drawRoundedRectangle (r, 8.0f, 2.0f);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawLine (r.getX() + 8, r.getY() + 1.5f, r.getRight() - 8, r.getY() + 1.5f, 1.0f);
    g.setColour (hot ? hdui::amber : Colour (0xffeeeeee));
    g.setFont (hdui::bold (15.0f).withExtraKerningFactor (0.12f));
    g.drawText (name, r.withTrimmedBottom (r.getHeight() * 0.35f).withTrimmedTop (r.getHeight() * 0.18f), Justification::centred);
    g.setColour (hdui::amber);
    g.setFont (hdui::mono (11.0f, true));
    g.drawText ("NOTA " + note, r.withTrimmedTop (r.getHeight() * 0.58f).withTrimmedBottom (r.getHeight() * 0.14f), Justification::centred);
}

//==============================================================================
void TimeMap::paint (Graphics& g)
{
    using namespace hdui;
    auto b = getLocalBounds().toFloat();
    // tela preta com moldura
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (b.translated (0, 4), 10.0f);
    g.setColour (Colour (0xff3a3a3a));
    g.fillRoundedRectangle (b, 10.0f);
    g.setColour (Colour (0xff0d0d0d));
    g.fillRoundedRectangle (b.reduced (3.0f), 9.0f);

    // cabeçalho âmbar
    g.setColour (amber);
    g.setFont (mono (17.0f));
    g.drawText (status, Rectangle<float> (b.getX() + 26, b.getY() + 12, b.getWidth() - 240, 28), Justification::centredLeft);
    const bool groove = proc.apvts.getRawParameterValue ("motor")->load() > 0.5f;
    g.setFont (bold (12.0f).withExtraKerningFactor (0.15f));
    g.setColour (Colour (0xffdddddd));
    const float hx = b.getRight() - 186;
    g.drawText ("GROOVE", Rectangle<float> (hx, b.getY() + 16, 70, 20), Justification::centredRight);
    g.drawText ("FITA", Rectangle<float> (hx + 102, b.getY() + 16, 40, 20), Justification::centredRight);
    for (int k = 0; k < 2; ++k)
    {
        const bool on = (k == 0) == groove;
        const auto lp = Point<float> (hx + (k == 0 ? 84.0f : 156.0f), b.getY() + 26);
        if (on) { g.setColour (amber.withAlpha (0.4f)); g.fillEllipse (lp.x - 9, lp.y - 9, 18, 18); }
        g.setColour (on ? amber : Colour (0xff4a1a12));
        g.fillEllipse (lp.x - 6, lp.y - 6, 12, 12);
    }

    // faixas das bandas
    const char* names[3] = { "GRAVE", "MÉDIO", "AGUDO" };
    const auto& sp = HalfDubProcessor::speedNames();
    const int speed = jlimit (0, 5, (int) proc.apvts.getRawParameterValue ("speed")->load());
    const float lx = b.getX() + 20, lw = b.getWidth() - 40, top = b.getY() + 52;
    const float laneH = 60.0f, laneGap = 8.0f, labelW = 110.0f;
    const int hp = proc.engine.histPos.load (std::memory_order_acquire);
    for (int i = 0; i < 3; ++i)
    {
        auto lane = Rectangle<float> (lx, top + (float) i * (laneH + laneGap), lw, laneH);
        g.setColour (Colour (0xff0f1a12));
        g.fillRoundedRectangle (lane, 4.0f);
        g.setColour (Colour (0xff1f3a26));
        g.drawRoundedRectangle (lane, 4.0f, 1.0f);
        auto wave = lane.withTrimmedLeft (labelW);
        g.setColour (Colour (0xff1d3324));
        for (int k = 0; k < 8; ++k) g.drawVerticalLine (roundToInt (wave.getX() + wave.getWidth() * (float) k / 8.0f), wave.getY(), wave.getBottom());

        const int bs = jlimit (0, 7, (int) proc.apvts.getRawParameterValue (HalfDubProcessor::bandId (i, "Speed"))->load());
        const String spName = bs == 0 ? sp[speed] : HalfDubProcessor::bandSpeedNames()[bs];
        const bool off = bs == 7;
        const bool wet = proc.engine.wetNow[i].load() > 0.5f;
        g.setColour (bandCol[i].withAlpha (off ? 0.4f : 1.0f));
        g.setFont (bold (14.0f));
        g.drawText (String::fromUTF8 (names[i]), Rectangle<float> (lane.getX() + 12, lane.getY() + 8, labelW, 20), Justification::centredLeft);
        g.setColour (amber.withAlpha (wet ? 1.0f : 0.5f));
        g.setFont (mono (14.0f));
        g.drawText (spName, Rectangle<float> (lane.getX() + 12, lane.getY() + 32, labelW, 20), Justification::centredLeft);

        // onda (histórico do nível da banda)
        {
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (wave.toNearestInt());
            const int cols = (int) wave.getWidth() / 2;
            Path pth;
            for (int c = 0; c < cols; ++c)
            {
                const int idx = (hp - cols + c + hd::Engine::kHist * 4) % hd::Engine::kHist;
                const float v = jlimit (0.0f, 1.0f, proc.engine.hist[(size_t) i][(size_t) idx].load (std::memory_order_relaxed) * 1.6f);
                const float x = wave.getX() + (float) c * 2.0f, a = v * (laneH * 0.45f);
                g.setColour (bandCol[i].withAlpha (off ? 0.25f : 0.75f));
                g.drawVerticalLine (roundToInt (x), lane.getCentreY() - a, lane.getCentreY() + a + 1);
            }
            // trajeto da leitura no loop (tracejado) + agulha de posição
            const float ratio = bs == 7 ? 0.0f : (bs == 0 ? hd::kSpeedRatio[speed] : hd::kSpeedRatio[bs - 1]);
            if (! off)
            {
                Path t;
                const float y0 = wave.getBottom() - 3, y1 = wave.getY() + 3;
                if (! wet || ratio >= 0.999f) { t.startNewSubPath (wave.getX(), y0); t.lineTo (wave.getRight(), y1); }
                else if (ratio < 0) { t.startNewSubPath (wave.getX(), y1); t.lineTo (wave.getRight(), y0); }
                else { t.startNewSubPath (wave.getX(), y0); t.lineTo (wave.getRight(), y0 - (y0 - y1) * ratio); }
                Path dashed;
                const float dash[2] = { 5.0f, 4.0f };
                PathStrokeType (2.0f).createDashedStroke (dashed, t, dash, 2);
                g.setColour ((i == 0 ? Colour (0xff3fcf6a) : i == 1 ? Colour (0xffffd27a) : Colour (0xffff8a70)).withAlpha (wet ? 0.95f : 0.4f));
                g.fillPath (dashed);
            }
            const float prog = proc.engine.progress[i].load();
            const float px = wave.getX() + wave.getWidth() * prog;
            g.setColour (Colours::white.withAlpha (0.85f));
            g.drawLine (px, wave.getY(), px, wave.getBottom(), 2.0f);
        }
    }
}

//==============================================================================
void OutMeter::update()
{
    for (int c = 0; c < 2; ++c)
    {
        const float d = 20.0f * std::log10 (proc.engine.peak[c].load() + 1e-9f);
        lvl[c] = d > lvl[c] ? d : jmax (d, lvl[c] - 1.2f);
        if (lvl[c] >= hold[c]) { hold[c] = lvl[c]; holdT[c] = 45; }
        else if (--holdT[c] <= 0) hold[c] = jmax (lvl[c], hold[c] - 1.5f);
    }
    repaint();
}

void OutMeter::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (Colour (0xff050505));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (Colour (0xffcccccc));
    g.drawRoundedRectangle (b, 6.0f, 2.0f);
    auto inner = b.reduced (10.0f, 12.0f);
    auto scale = inner.removeFromLeft (30.0f);
    inner.removeFromLeft (8.0f);
    const float barW = 34.0f, gap = 14.0f;
    const int segs = 34;
    const float segH = inner.getHeight() / (float) segs;
    const float minDb = -48.0f;
    auto frac = [&] (float d) { return jlimit (0.0f, 1.0f, (d - minDb) / -minDb); };
    g.setFont (hdui::mono (11.0f));
    for (float d : { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f })
    {
        const float y = inner.getBottom() - frac (d) * inner.getHeight();
        g.setColour (d == 0.0f ? Colour (0xffe8402a) : Colour (0xff999999));
        g.drawText (String ((int) d), Rectangle<float> (scale.getX(), y - 6, scale.getWidth(), 12), Justification::centredRight);
    }
    for (int c = 0; c < 2; ++c)
    {
        const float x = inner.getX() + (float) c * (barW + gap);
        const float lit = frac (lvl[c]) * (float) segs;
        const int holdSeg = (int) (frac (hold[c]) * (float) segs);
        for (int k = 0; k < segs; ++k)
        {
            const auto col = k > 30 ? Colour (0xffe8402a) : k > 27 ? hdui::amber : Colour (0xff3fcf6a);
            const bool on = (float) k < lit || k == holdSeg - 1;
            g.setColour (on ? col : Colour (0xff1d1d1d));
            g.fillRect (Rectangle<float> (x, inner.getBottom() - (float) (k + 1) * segH + 1.0f, barW, segH - 1.6f));
        }
    }
}

//==============================================================================
namespace
{
    struct EqPreset { const char* name; float hp, lp, g[4]; };
    const EqPreset kEqPresets[EqPresets::kNum] = {
        { "FLAT", 20.0f, 20000.0f, { 0, 0, 0, 0 } },
        { "HPF", 90.0f, 20000.0f, { 0, 0, 0, 0 } },
        { "LPF", 20.0f, 3500.0f, { 0, 0, 0, 0 } },
        { "HPF+LPF", 120.0f, 4500.0f, { 0, 0, 0, 0 } },
        { "DUB", 35.0f, 9000.0f, { 5, -3, 1, -2 } },
        { "RÁDIO", 400.0f, 3000.0f, { 0, 0, 4, 0 } },
    };
    const float kEqF[4] = { 100.0f, 500.0f, 2000.0f, 7000.0f };
    const float kEqQ[4] = { 1.0f, 1.2f, 1.2f, 1.0f };
}

EqGraph::EqGraph (HalfDubProcessor& pr) : proc (pr) {}

hd::EqSettings EqGraph::current() const
{
    hd::EqSettings s;
    auto v = [this] (const String& id) { return proc.apvts.getRawParameterValue (id)->load(); };
    s.hp = v ("eqHp"); s.lp = v ("eqLp"); s.gainDb = v ("eqGain");
    for (int k = 0; k < 4; ++k)
    {
        const String n (k + 1);
        s.f[k] = v ("eqF" + n); s.g[k] = v ("eqG" + n); s.q[k] = v ("eqQ" + n);
    }
    return s;
}

Rectangle<float> EqGraph::plot() const { return getLocalBounds().toFloat().reduced (3.0f); }
float EqGraph::xOf (float f) const { auto p = plot(); return p.getX() + p.getWidth() * std::log10 (jlimit (20.0f, 20000.0f, f) / 20.0f) / 3.0f; }
float EqGraph::fOf (float x) const { auto p = plot(); return 20.0f * std::pow (10.0f, 3.0f * jlimit (0.0f, 1.0f, (x - p.getX()) / p.getWidth())); }
float EqGraph::yOf (float d) const { auto p = plot(); return p.getCentreY() - d / 15.0f * (p.getHeight() * 0.5f - 10.0f); }
float EqGraph::dOf (float y) const { auto p = plot(); return (p.getCentreY() - y) / (p.getHeight() * 0.5f - 10.0f) * 15.0f; }

Point<float> EqGraph::nodePos (int i, const hd::EqSettings& s) const
{
    auto p = plot();
    auto clampX = [&] (float x) { return jlimit (p.getX() + 14.0f, p.getRight() - 14.0f, x); };
    if (i == 0) return { clampX (xOf (s.hp)), yOf (hd::GraphicEq::responseDb (s, s.hp) - s.gainDb) };
    if (i == 5) return { clampX (xOf (s.lp)), yOf (hd::GraphicEq::responseDb (s, s.lp) - s.gainDb) };
    return { clampX (xOf (s.f[i - 1])), yOf (s.g[i - 1]) };
}

int EqGraph::nodeAt (Point<float> pt) const
{
    const auto s = current();
    int best = -1; float bd = 18.0f;
    for (int i = 0; i < 6; ++i)
    {
        const float d = nodePos (i, s).getDistanceFrom (pt);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

void EqGraph::setP (const String& id, float v) const
{
    if (auto* prm = p (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

static StringArray eqNodeParams (int i)
{
    if (i == 0) return { "eqHp" };
    if (i == 5) return { "eqLp" };
    const String n (i);
    return { "eqF" + n, "eqG" + n, "eqQ" + n };
}

void EqGraph::mouseDown (const MouseEvent& e)
{
    drag = nodeAt (e.position);
    if (drag >= 0) for (auto& id : eqNodeParams (drag)) if (auto* prm = p (id)) prm->beginChangeGesture();
}

void EqGraph::mouseDrag (const MouseEvent& e)
{
    if (drag < 0) return;
    const float f = fOf (e.position.x);
    const auto s = current();
    if (drag == 0) setP ("eqHp", jlimit (20.0f, jmin (2000.0f, s.lp * 0.8f), f));
    else if (drag == 5) setP ("eqLp", jlimit (jmax (200.0f, s.hp * 1.25f), 20000.0f, f));
    else
    {
        const String n (drag);
        setP ("eqF" + n, f);
        setP ("eqG" + n, jlimit (-12.0f, 12.0f, dOf (e.position.y)));
    }
    repaint();
}

void EqGraph::mouseUp (const MouseEvent&)
{
    if (drag >= 0) for (auto& id : eqNodeParams (drag)) if (auto* prm = p (id)) prm->endChangeGesture();
    drag = -1;
}

void EqGraph::mouseDoubleClick (const MouseEvent& e)
{
    const int i = nodeAt (e.position);
    if (i < 0) return;
    auto reset = [this] (const String& id, float v) { if (auto* prm = p (id)) { prm->beginChangeGesture(); setP (id, v); prm->endChangeGesture(); } };
    if (i == 0) reset ("eqHp", 20.0f);
    else if (i == 5) reset ("eqLp", 20000.0f);
    else reset ("eqG" + String (i), 0.0f);
    repaint();
}

void EqGraph::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const int i = nodeAt (e.position);
    if (i < 1 || i > 4) return;
    const String id = "eqQ" + String (i);
    if (auto* prm = p (id))
    {
        const float q = proc.apvts.getRawParameterValue (id)->load() * (w.deltaY > 0 ? 1.12f : 0.89f);
        prm->beginChangeGesture(); setP (id, jlimit (0.3f, 4.0f, q)); prm->endChangeGesture();
    }
    repaint();
}

void EqGraph::mouseMove (const MouseEvent& e)
{
    const int i = nodeAt (e.position);
    if (i != hover) { hover = i; setMouseCursor (i >= 0 ? MouseCursor::DraggingHandCursor : MouseCursor::CrosshairCursor); repaint(); }
}

String EqGraph::describe (int i) const
{
    const auto s = current();
    auto hzS = [] (float f) { return f >= 1000.0f ? String (f / 1000.0f, 1) + " kHz" : String (roundToInt (f)) + " Hz"; };
    if (i == 0) return "EQ HPF:  " + (s.hp <= 20.5f ? String ("OFF") : hzS (s.hp));
    if (i == 5) return "EQ LPF:  " + (s.lp >= 19900.0f ? String ("OFF") : hzS (s.lp));
    return "EQ " + String (i) + ":  " + hzS (s.f[i - 1]) + "  " + (s.g[i - 1] > 0 ? "+" : "") + String (s.g[i - 1], 1) + " dB  Q " + String (s.q[i - 1], 2);
}

void EqGraph::paint (Graphics& g)
{
    using namespace hdui;
    const auto s = current();
    const bool byp = proc.apvts.getRawParameterValue ("eqByp")->load() > 0.5f;
    auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff3a3a3a));
    g.fillRoundedRectangle (b, 9.0f);
    g.setColour (Colour (0xff0d1a11));
    g.fillRoundedRectangle (b.reduced (3.0f), 8.0f);
    const auto pl = plot();
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (pl.toNearestInt());
    const Colour grid (0xff1d3324), txt (0xff4f7a5a);
    g.setFont (mono (11.0f));
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = xOf (f);
        g.setColour (grid); g.drawVerticalLine (roundToInt (x), pl.getY(), pl.getBottom());
        g.setColour (txt);
        g.drawText (f >= 1000 ? String ((int) (f / 1000)) + "k" : String ((int) f), Rectangle<float> (x + 4, pl.getBottom() - 16, 40, 12), Justification::centredLeft);
    }
    for (float d : { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f })
    {
        const float y = yOf (d);
        g.setColour (d == 0 ? Colour (0xff2f5a3a) : grid); g.drawHorizontalLine (roundToInt (y), pl.getX(), pl.getRight());
        g.setColour (txt);
        g.drawText ((d > 0 ? "+" : "") + String ((int) d), Rectangle<float> (pl.getX() + 6, y - 14, 40, 12), Justification::centredLeft);
    }
    const float alpha = byp ? 0.35f : 1.0f;
    Path curve, fill;
    const int cols = (int) pl.getWidth() / 3;
    for (int c = 0; c <= cols; ++c)
    {
        const float x = pl.getX() + pl.getWidth() * (float) c / (float) cols;
        const float y = jlimit (pl.getY(), pl.getBottom(), yOf (hd::GraphicEq::responseDb (s, fOf (x)) ));
        if (c == 0) { curve.startNewSubPath (x, y); fill.startNewSubPath (x, pl.getBottom()); }
        else curve.lineTo (x, y);
        fill.lineTo (x, y);
    }
    fill.lineTo (pl.getRight(), pl.getBottom()); fill.closeSubPath();
    g.setColour (phosphor.withAlpha (0.12f * alpha)); g.fillPath (fill);
    g.setColour (phosphor.withAlpha (0.2f * alpha)); g.strokePath (curve, PathStrokeType (6.0f));
    g.setColour (phosphor.withAlpha (alpha)); g.strokePath (curve, PathStrokeType (2.5f));

    const char* nl[6] = { "HPF", "1", "2", "3", "4", "LPF" };
    for (int i = 0; i < 6; ++i)
    {
        const auto np = nodePos (i, s);
        const bool act = (i == 0 && s.hp > 20.5f) || (i == 5 && s.lp < 19900.0f) || (i > 0 && i < 5 && std::abs (s.g[i - 1]) > 0.05f);
        const bool hl = i == hover || i == drag;
        g.setColour ((act ? amber : Colour (0xff2a2a2a)).withAlpha (alpha));
        g.fillEllipse (np.x - 11, np.y - 11, 22, 22);
        g.setColour ((hl ? Colours::white : act ? Colour (0xffffd27a) : Colour (0xff888888)).withAlpha (alpha));
        g.drawEllipse (np.x - 11, np.y - 11, 22, 22, 2.0f);
        g.setColour ((act ? Colour (0xff1a1008) : Colour (0xffcccccc)).withAlpha (alpha));
        g.setFont (bold (i == 0 || i == 5 ? 8.0f : 11.0f));
        g.drawText (nl[i], Rectangle<float> (np.x - 11, np.y - 11, 22, 22), Justification::centred);
    }
    if (byp)
    {
        g.setColour (amber);
        g.setFont (bold (16.0f).withExtraKerningFactor (0.2f));
        g.drawText ("BYPASS", pl.withTrimmedTop (8).withHeight (24), Justification::centred);
    }
}

//==============================================================================
Rectangle<float> EqPresets::cell (int i) const
{
    const float w = ((float) getWidth() - 8.0f * (kNum - 1)) / (float) kNum;
    return { (float) i * (w + 8.0f), 0.0f, w, (float) getHeight() };
}

int EqPresets::matching() const
{
    auto v = [this] (const String& id) { return proc.apvts.getRawParameterValue (id)->load(); };
    for (int i = 0; i < kNum; ++i)
    {
        const auto& pr = kEqPresets[i];
        bool ok = std::abs (v ("eqHp") - pr.hp) < 0.5f && std::abs (v ("eqLp") - pr.lp) < 5.0f;
        for (int k = 0; k < 4 && ok; ++k)
            ok = std::abs (v ("eqG" + String (k + 1)) - pr.g[k]) < 0.05f && std::abs (v ("eqF" + String (k + 1)) - kEqF[k]) < 1.0f;
        if (ok) return i;
    }
    return -1;
}

void EqPresets::apply (int i)
{
    const auto& pr = kEqPresets[i];
    auto set = [this] (const String& id, float val)
    {
        if (auto* prm = proc.apvts.getParameter (id))
        {
            prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 (val)); prm->endChangeGesture();
        }
    };
    set ("eqHp", pr.hp); set ("eqLp", pr.lp);
    for (int k = 0; k < 4; ++k)
    {
        const String n (k + 1);
        set ("eqF" + n, kEqF[k]); set ("eqG" + n, pr.g[k]); set ("eqQ" + n, kEqQ[k]);
    }
    update();
}

void EqPresets::update()
{
    const int m = matching();
    if (m != sel) { sel = m; repaint(); }
}

void EqPresets::paint (Graphics& g)
{
    for (int i = 0; i < kNum; ++i)
        hdui::drawChip (g, cell (i), String::fromUTF8 (kEqPresets[i].name), i == sel, i == hover, 12.0f);
}

void EqPresets::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < kNum; ++i) if (cell (i).contains (e.position)) { apply (i); return; }
}

void EqPresets::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < kNum; ++i) if (cell (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; repaint(); }
}

//==============================================================================
HalfDubPanel::HalfDubPanel (HalfDubProcessor& p)
    : proc (p), timeMap (p), meter (p), eqGraph (p), eqPresets (p), dropBtn (p)
{
    setLookAndFeel (&lnf);
    setSize (kW, kH);
    auto& ap = proc.apvts;

    addAndMakeVisible (timeMap);
    addAndMakeVisible (meter);
    addAndMakeVisible (eqGraph);
    addAndMakeVisible (eqPresets);
    addAndMakeVisible (dropBtn);

    // --- VELOCIDADE (seletor grande)
    {
        auto& k = knob ("speed", "VELOCIDADE");
        k.slider.getProperties().set ("scaleLabels", HalfDubProcessor::speedNames().joinIntoString ("|"));
    }
    auto chips = [this] (const String& id, StringArray labels, int cols, Rectangle<int> r, float fh = 13.0f) -> ChoiceChips&
    {
        auto c = std::make_unique<ChoiceChips> (*proc.apvts.getParameter (id), std::move (labels), cols, fh);
        c->setBounds (r);
        addAndMakeVisible (*c);
        auto* raw = c.get();
        others.push_back (std::move (c));
        return *raw;
    };
    chips ("motor", { "FITA", "GROOVE" }, 2, { 148, 700, 226, 40 }, 14.0f);
    chips ("latch", { "SEGURAR", "TRAVADO" }, 2, { 418, 650, 244, 40 }, 14.0f);
    knob ("pwrDown", "POWER DOWN");
    knob ("pwrUp", "POWER UP");

    // --- BANDAS
    for (int b = 0; b < 3; ++b)
    {
        const int x = 718 + b * 226;
        chips (HalfDubProcessor::bandId (b, "Speed"), HalfDubProcessor::bandSpeedNames(), 4, { x + 12, 452, 188, 80 }, 12.0f);
        knob (HalfDubProcessor::bandId (b, "Mix"), "MIX");
        knob (HalfDubProcessor::bandId (b, "Smooth"), "SMOOTH");
        auto ls = std::make_unique<LoopStepper> (*ap.getParameter (HalfDubProcessor::bandId (b, "Loop")));
        ls->setBounds (x + 70, 708, 128, 34);
        addAndMakeVisible (*ls);
        others.push_back (std::move (ls));
        auto& s = toggle (HalfDubProcessor::bandId (b, "Solo"), "S");
        s.button.getProperties().set ("noLamp", true);
        s.button.getProperties().set ("fontH", 14.0f);
        s.button.setBounds (x + 166, 406, 34, 28);
    }

    // --- MIX · SAÍDA
    knob ("dryWet", "DRY/WET");
    knob ("out", String::fromUTF8 ("SAÍDA"));

    // --- SOUND SYSTEM
    {
        const char* ids[4] = { "padRewind", "padStop", "padReverse", "padSpin" };
        const char* names[4] = { "REWIND", "TAPE STOP", "REVERSE", "SPIN-BACK" };
        const char* notes[4] = { "C1", "D1", "E1", "F1" };
        for (int i = 0; i < 4; ++i)
        {
            auto pb = std::make_unique<PadButton> (proc, ids[i], i + 1, names[i], notes[i]);
            pb->setBounds (78 + i * 148, 882, 136, 100);
            addAndMakeVisible (*pb);
            pads.push_back (pb.get());
            others.push_back (std::move (pb));
        }
    }
    knob ("padDur", String::fromUTF8 ("DURAÇÃO"));
    toggle ("padSync", "SYNC").button.setBounds (330, 1010, 112, 38);

    // --- ECO DE FITA · WASHOUT
    knob ("echoDiv", "TEMPO");
    knob ("echoFb", "INTENS.");
    knob ("echoSend", "ENVIO");
    knob ("washout", "WASHOUT");
    toggle ("echoOn", "ECO LIGADO").button.setBounds (712, 1020, 132, 36);
    toggle ("echoPP", "PING-PONG").button.setBounds (852, 1020, 124, 36);
    toggle ("echoMte", "MTE-12 TAPE").button.setBounds (984, 1020, 150, 36);

    // --- FITA · SMOOTH
    knob ("wow", "WOW/FLUTTER");
    knob ("sat", String::fromUTF8 ("SATURAÇÃO"));
    chips ("smoothMode", { "PERC", "SUST", "FX", "SWELL" }, 4, { 1186, 1020, 228, 34 }, 11.0f);

    // --- EQ GRÁFICO
    eqPresets.setBounds (80, 1134, 560, 34);
    eqGraph.setBounds (80, 1178, 890, 200);
    {
        auto& t = toggle ("eqByp", "BYPASS EQ");
        t.button.setBounds (822, 1134, 148, 34);
    }
    chips ("eqPos", { String::fromUTF8 ("SÓ NO SINAL LENTO"), String::fromUTF8 ("NA SAÍDA TOTAL") }, 2, { 176, 1392, 380, 38 }, 11.0f);
    knob ("eqGain", "GANHO EQ");

    // --- MASTOR BIG KNOB
    {
        auto& k = knob ("bkCut", "BIG KNOB");
        k.slider.getProperties().set ("bigknob", true);
        k.slider.setRotaryParameters (MathConstants<float>::twoPi - degreesToRadians (140.0f), MathConstants<float>::twoPi + degreesToRadians (140.0f), true);
    }
    toggle ("bkStep", "STEP").button.setBounds (1334, 1216, 86, 36);
    toggle ("bkLow", "LOW P.").button.setBounds (1426, 1216, 94, 36);
    toggle ("bk24", "24dB").button.setBounds (1526, 1216, 76, 36);
    knob ("bkRes", String::fromUTF8 ("RESSONÂNCIA"));
    knob ("bkDrive", "DRIVE");
    knob ("bkMix", "MIX ");
    toggle ("bkByp", "BYPASS DO FILTRO").button.setBounds (1334, 1396, 266, 36);

    // --- topo
    {
        auto& t = toggle ("bypass", "BYPASS");
        t.button.getProperties().set ("big", true);
        t.button.setBounds (88, 250, 190, 52);
    }

    for (auto& t : toggles)
        if (t->button.getProperties()["fontH"].isVoid() && ! (bool) t->button.getProperties()["big"])
            t->button.getProperties().set ("fontH", 12.0f);

    resized();
    startTimerHz (30);
}

HalfDubPanel::~HalfDubPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

HalfDubPanel::Knob& HalfDubPanel::knob (const String& id, const String& name)
{
    auto k = std::make_unique<Knob>();
    auto& s = k->slider;
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    s.setMouseDragSensitivity (220);
    addAndMakeVisible (s);
    k->att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
    if (auto* prm = proc.apvts.getParameter (id))
        s.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    k->name = name.trim();
    auto* raw = k.get();
    knobById[id] = raw;
    knobs.push_back (std::move (k));
    return *raw;
}

HalfDubPanel::Toggle& HalfDubPanel::toggle (const String& id, const String& text)
{
    auto t = std::make_unique<Toggle>();
    t->button.setButtonText (text);
    t->button.setClickingTogglesState (true);
    addAndMakeVisible (t->button);
    t->att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, t->button);
    auto* raw = t.get();
    toggles.push_back (std::move (t));
    return *raw;
}

void HalfDubPanel::placeKnob (Knob& k, float cx, float cy, float metalR)
{
    const float r = metalR / 0.62f + 4.0f;
    k.slider.setBounds (Rectangle<float> (r * 2, r * 2).withCentre ({ cx, cy }).toNearestInt());
}

void HalfDubPanel::resized()
{
    if (knobById.count ("bkCut") == 0) return;   // ainda montando o painel
    auto K = [this] (const String& id) -> Knob& { return *knobById.at (id); };
    placeKnob (K ("speed"), 216, 540, 68);
    placeKnob (K ("pwrDown"), 480, 744, 26);
    placeKnob (K ("pwrUp"), 600, 744, 26);
    for (int b = 0; b < 3; ++b)
    {
        const float x = 718.0f + (float) b * 226.0f;
        placeKnob (K (HalfDubProcessor::bandId (b, "Mix")), x + 60, 640, 26);
        placeKnob (K (HalfDubProcessor::bandId (b, "Smooth")), x + 152, 640, 26);
    }
    placeKnob (K ("dryWet"), 1470, 708, 26);
    placeKnob (K ("out"), 1574, 708, 26);
    placeKnob (K ("padDur"), 150, 1030, 22);
    placeKnob (K ("echoDiv"), 752, 966, 28);
    placeKnob (K ("echoFb"), 858, 966, 28);
    placeKnob (K ("echoSend"), 964, 966, 28);
    placeKnob (K ("washout"), 1070, 966, 28);
    placeKnob (K ("wow"), 1240, 956, 28);
    placeKnob (K ("sat"), 1352, 956, 28);
    placeKnob (K ("eqGain"), 920, 1411, 15);
    placeKnob (K ("bkRes"), 1378, 1340, 22);
    placeKnob (K ("bkDrive"), 1466, 1340, 22);
    placeKnob (K ("bkMix"), 1554, 1340, 22);
    K ("bkCut").slider.setBounds (Rectangle<int> (300, 300).withCentre ({ 1166, 1296 }));

    timeMap.setBounds (904, 24, 746, 272);
    meter.setBounds (1426, 392, 190, 206);
    dropBtn.setBounds (420, 394, 240, 240);
    renderBackground();
}

//==============================================================================
String HalfDubPanel::bigKnobText() const
{
    auto v = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    if (v ("bkByp") > 0.5f) return "BYPASS";
    const bool lp = v ("bkLow") > 0.5f, st = v ("bkStep") > 0.5f;
    const float f = dub::knobToFrequency (v ("bkCut"), st);
    if (! lp && f <= 20.5f) return "OFF";
    const String hz = f >= 1000.0f ? String (f / 1000.0f, f >= 10000.0f ? 0 : 1) + " kHz" : String (roundToInt (f)) + " Hz";
    return String (lp ? "LPF" : "HPF") + String::fromUTF8 (" · ") + hz;
}

String HalfDubPanel::statusText()
{
    for (auto& [id, k] : knobById)
        if (k->slider.isMouseOverOrDragging())
        {
            if (id == "bkCut") return "BIG KNOB:  " + bigKnobText();
            return k->name + ":  " + k->slider.getTextFromValue (k->slider.getValue());
        }
    const int eqNode = eqGraph.getHover();
    if (eqNode >= 0) return eqGraph.describe (eqNode);

    auto v = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    const bool on = v ("drop") > 0.5f || proc.isMidiDropHeld();
    String t = "INIT  -  ";
    t << (on ? "DROP ON" : "DROP OFF") << String::fromUTF8 (" · ")
      << HalfDubProcessor::speedNames()[jlimit (0, 5, (int) v ("speed"))] << " VELOCIDADE" << String::fromUTF8 (" · ")
      << roundToInt (proc.engine.bpmNow.load()) << " BPM";
    return t;
}

void HalfDubPanel::timerCallback()
{
    // VU (0 VU = -18 dBFS)
    const float lvl = proc.engine.rms.load();
    const float vu = 20.0f * std::log10 (lvl + 1e-9f) + 18.0f;
    const float target = jlimit (-0.03f, 1.06f, vuToPos (vu));
    needleVel = (needleVel + (target - needlePos) * 0.30f) * 0.58f;
    needlePos += needleVel;
    peakHold = vu > 3.0f ? 15 : jmax (0, peakHold - 1);
    const bool wet = proc.engine.wetNow[0].load() > 0.5f || proc.engine.wetNow[1].load() > 0.5f || proc.engine.wetNow[2].load() > 0.5f;
    lampGlow += ((wet ? 1.0f : 0.0f) - lampGlow) * 0.3f;

    knobById.at ("bkCut")->slider.getProperties().set ("stepped", proc.apvts.getRawParameterValue ("bkStep")->load() > 0.5f);

    timeMap.setStatus (statusText());
    timeMap.repaint();
    meter.update();
    dropBtn.update();
    eqPresets.update();
    eqGraph.repaint();
    for (auto* pb : pads) pb->update();
    knobById.at ("bkCut")->slider.repaint();

    const auto bk = bigKnobText();
    if (bk != lastBk) { lastBk = bk; repaint (1334, 1164, 266, 44); }
    repaint (vuArea);
    repaint (Rectangle<float> (44, 44).withCentre (lampPos).toNearestInt());
}

//==============================================================================
void HalfDubPanel::paint (Graphics& g)
{
    using namespace hdui;
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    // lâmpada NO AR (acende com o DROP)
    const auto lp = lampPos;
    g.setColour (Colour (0xffbbbbbb));
    g.fillEllipse (lp.x - 21, lp.y - 21, 42, 42);
    if (lampGlow > 0.02f)
    {
        g.setGradientFill (ColourGradient (amber.withAlpha (0.6f * lampGlow), lp.x, lp.y, amber.withAlpha (0.0f), lp.x + 26, lp.y, true));
        g.fillEllipse (lp.x - 26, lp.y - 26, 52, 52);
    }
    g.setGradientFill (ColourGradient (Colour (0xffffc07a).interpolatedWith (Colour (0xff5a3a20), 1.0f - lampGlow), lp.x - 5, lp.y - 6,
                                       Colour (0xff8a4a10).interpolatedWith (Colour (0xff2a1a0a), 1.0f - lampGlow), lp.x + 17, lp.y + 17, true));
    g.fillEllipse (lp.x - 17, lp.y - 17, 34, 34);

    // visor do Big Knob
    {
        auto r = Rectangle<float> (1336, 1166, 262, 40);
        g.setColour (Colour (0xff0c0c0c)); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (Colour (0xff444444)); g.drawRoundedRectangle (r.reduced (1), 6.0f, 2.0f);
        g.setColour (amber); g.setFont (mono (18.0f));
        g.drawText (lastBk.isEmpty() ? bigKnobText() : lastBk, r, Justification::centred);
    }
    drawVuNeedle (g);
}

//==============================================================================
namespace
{
struct VuGeom { Rectangle<float> housing, face, label; Point<float> pivot; float radius; };
VuGeom vuGeom (Rectangle<int> area)
{
    VuGeom v;
    v.housing = area.toFloat();
    v.face = v.housing.reduced (14.0f).withHeight (v.housing.getHeight() - 78.0f);
    v.label = Rectangle<float> (v.housing.getX(), v.face.getBottom() + 8.0f, v.housing.getWidth(), v.housing.getBottom() - v.face.getBottom() - 16.0f);
    v.pivot = { v.face.getCentreX(), v.face.getBottom() + 4.0f };
    v.radius = v.face.getHeight() * 0.62f;
    return v;
}
}

void HalfDubPanel::drawVuFace (Graphics& g)
{
    using namespace hdui;
    const auto v = vuGeom (vuArea);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (v.housing.translated (0, 6.0f), 10.0f);
    g.setColour (Colour (0xff444444));
    g.fillRoundedRectangle (v.housing, 10.0f);
    g.setColour (Colour (0xff111111));
    g.fillRoundedRectangle (v.housing.reduced (3.0f), 9.0f);
    g.setGradientFill (ColourGradient (Colour (0xfff7ecc8), 0, v.face.getY(), Colour (0xffe4d09a), 0, v.face.getBottom(), false));
    g.fillRoundedRectangle (v.face, 6.0f);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (v.face, 6.0f, 3.0f);

    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (v.face.toNearestInt());
    const auto c = v.pivot;
    const float R = v.radius;
    auto angleOf = [] (float pos) { return -kVuSwing + 2.0f * kVuSwing * jlimit (0.0f, 1.0f, pos); };
    Path red;
    red.addCentredArc (c.x, c.y, R - 4, R - 4, 0.0f, angleOf (vuToPos (0.0f)), angleOf (1.0f), true);
    g.setColour (Colour (0xffc8202a));
    g.strokePath (red, PathStrokeType (6.0f));
    const float marks[] = { -20, -10, -7, -5, -3, -1, 0, 1, 2, 3 };
    g.setFont (bold (13.0f));
    for (float m : marks)
    {
        const float t = angleOf (vuToPos (m));
        const bool hot = m > 0.0f;
        g.setColour (hot ? Colour (0xffc8202a) : Colour (0xff222222));
        g.drawLine ({ c.getPointOnCircumference (R - 10.0f, t), c.getPointOnCircumference (R + 2.0f, t) }, 2.0f);
        const auto np = c.getPointOnCircumference (R + 16.0f, t);
        g.drawText (m > 0 ? "+" + String ((int) m) : String ((int) std::abs (m)), Rectangle<float> (np.x - 14, np.y - 8, 28, 16), Justification::centred);
    }
    g.setColour (Colour (0xff1a1a1a));
    g.setFont (title (34.0f));
    g.drawText ("VU", Rectangle<float> (v.face.getX(), v.face.getY() + v.face.getHeight() * 0.42f, v.face.getWidth(), 40), Justification::centred);
}

void HalfDubPanel::drawVuNeedle (Graphics& g)
{
    using namespace hdui;
    const auto v = vuGeom (vuArea);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (v.face.toNearestInt());
        const float t = -kVuSwing + 2.0f * kVuSwing * jlimit (-0.03f, 1.06f, needlePos);
        const auto tip = v.pivot.getPointOnCircumference (v.radius + 6.0f, t);
        const auto base = v.pivot.getPointOnCircumference (v.radius * 0.2f, t);
        g.setColour (Colours::black.withAlpha (0.18f));
        g.drawLine ({ base.translated (2.0f, 3.0f), tip.translated (2.0f, 3.0f) }, 2.0f);
        g.setColour (Colour (0xff111111));
        g.drawLine ({ base, tip }, 2.0f);
        auto cover = v.face.withTop (v.face.getBottom() - 30.0f);
        g.setColour (Colour (0xff1a1a1a));
        g.fillRect (cover);
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.2f), v.face.getX(), v.face.getY(),
                                           Colours::transparentWhite, v.face.getX() + v.face.getWidth() * 0.55f, v.face.getY() + v.face.getHeight() * 0.6f, false));
        g.fillRect (v.face.withHeight (v.face.getHeight() * 0.5f));
    }
    g.setColour (Colour (0xffdddddd));
    g.setFont (bold (13.0f).withExtraKerningFactor (0.15f));
    g.drawText (String::fromUTF8 ("NÍVEL"), v.label.withTrimmedLeft (30.0f), Justification::centredLeft);
    g.setColour (Colour (0xffcccccc));
    g.setFont (bold (11.0f).withExtraKerningFactor (0.15f));
    g.drawText ("PEAK", Rectangle<float> (v.label.getRight() - 100, v.label.getY(), 56, v.label.getHeight()), Justification::centredRight);
    const auto lp = Point<float> (v.label.getRight() - 32.0f, v.label.getCentreY());
    g.setColour (peakHold > 0 ? Colour (0xffff3b2f) : Colour (0xff4a1612));
    g.fillEllipse (lp.x - 7, lp.y - 7, 14, 14);
}

//==============================================================================
void HalfDubPanel::renderBackground()
{
    using namespace hdui;
    const int S = 1;
    const int W = kW * S, H = kH * S;
    const float topH = 318.0f;
    background = Image (Image::RGB, W, H, false);
    Random rnd (1975);
    {   // esmalte creme (em cima) + aço pintado escuro (embaixo)
        Image::BitmapData bd (background, Image::BitmapData::writeOnly);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
            {
                const float vx = (float) x / (float) W, n = rnd.nextFloat() - 0.5f;
                if ((float) y < topH * S)
                {
                    const float ey = (float) y / (topH * S);
                    const float edge = jmax (std::pow (std::abs (vx - 0.5f) * 2.0f, 4.0f), std::pow (std::abs (ey - 0.45f) * 2.0f, 4.0f));
                    const float k = 1.0f - 0.10f * edge + 0.02f * n + 0.015f * std::sin (vx * 23.0f) * std::sin (ey * 7.0f);
                    bd.setPixelColour (x, y, Colour::fromFloatRGBA (0.92f * k, 0.88f * k, 0.78f * k, 1.0f));
                }
                else
                {
                    const float vy = (float) y / (float) H;
                    float k = 0.12f + 0.012f * n + 0.008f * std::sin (vy * 90.0f + vx * 4.0f);
                    k *= 1.0f - 0.35f * std::pow (std::abs (vx - 0.4f) * 1.6f, 3.0f);
                    bd.setPixelColour (x, y, Colour::fromFloatRGBA (k * 1.10f, k * 0.98f, k * 0.88f, 1.0f));
                }
            }
    }
    Graphics g (background);
    g.addTransform (AffineTransform::scale ((float) S));
    const float w = (float) kW, h = (float) kH;

    // riscos no painel escuro
    for (int i = 0; i < 120; ++i)
    {
        const float x0 = 44 + rnd.nextFloat() * (w - 88), y0 = topH + rnd.nextFloat() * (h - topH);
        const float len = 3.0f + rnd.nextFloat() * 22.0f, a = rnd.nextFloat() * MathConstants<float>::pi;
        g.setColour (Colours::white.withAlpha (0.03f + 0.05f * rnd.nextFloat()));
        g.drawLine (x0, y0, x0 + len * std::cos (a), y0 + len * std::sin (a), 0.6f);
    }

    auto txt = [&g] (const String& s, float x, float y, float wd, float hh, Font f, Colour c, Justification j = Justification::centredLeft)
    {
        g.setFont (f); g.setColour (c); g.drawText (s, Rectangle<float> (x, y, wd, hh), j);
    };
    auto inset = [&g] (Rectangle<float> r, float rad = 8.0f)
    {
        g.setColour (Colours::black.withAlpha (0.28f)); g.fillRoundedRectangle (r, rad);
        g.setColour (Colours::black.withAlpha (0.55f)); g.drawRoundedRectangle (r.translated (0, 1), rad, 2.0f);
        g.setColour (Colours::white.withAlpha (0.05f)); g.drawRoundedRectangle (r, rad, 1.0f);
    };
    auto screw = [&g, &rnd] (float cx, float cy, float r = 8.0f)
    {
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillEllipse (cx - r, cy - r + 1.5f, r * 2, r * 2);
        g.setGradientFill (ColourGradient (Colour (0xffeeeeee), cx - r * 0.3f, cy - r * 0.3f, Colour (0xff444444), cx + r, cy + r, true));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
        const float a = rnd.nextFloat() * MathConstants<float>::pi;
        g.setColour (Colour (0xff333333));
        g.drawLine (cx - r * 0.65f * std::cos (a), cy - r * 0.65f * std::sin (a), cx + r * 0.65f * std::cos (a), cy + r * 0.65f * std::sin (a), 2.0f);
    };

    // --- plaqueta de alumínio
    {
        const auto pl = Rectangle<float> (78, 28, 500, 118);
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (pl.translated (0, 4.0f), 6.0f);
        ColourGradient al (Colour (0xfff2f2f0), 0, pl.getY(), Colour (0xffe4e4e1), 0, pl.getBottom(), false);
        al.addColour (0.5, Colour (0xffc9c9c6));
        g.setGradientFill (al);
        g.fillRoundedRectangle (pl, 6.0f);
        for (float yy = pl.getY() + 2.0f; yy < pl.getBottom() - 2.0f; yy += 0.7f)
        {
            g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (0.03f + 0.04f * rnd.nextFloat()));
            g.drawLine (pl.getX() + 3 + rnd.nextFloat() * 30, yy, pl.getRight() - 3 - rnd.nextFloat() * 30, yy, 0.5f);
        }
        g.setColour (Colour (0xff9a9a96)); g.drawRoundedRectangle (pl.reduced (1), 6.0f, 2.0f);
        for (auto rv : { Point<float> (92, 42), Point<float> (564, 42), Point<float> (92, 132), Point<float> (564, 132) })
            screw (rv.x, rv.y, 5.0f);
        const String t ("MASTOR HALF DUB");
        float fs = 40.0f;
        while (fs > 20.0f && GlyphArrangement::getStringWidth (title (fs).withExtraKerningFactor (0.04f), t) > pl.getWidth() - 56) fs -= 0.5f;
        txt (t, pl.getX() + 25, pl.getY() + 15, pl.getWidth() - 40, 50, title (fs).withExtraKerningFactor (0.04f), Colours::white.withAlpha (0.8f));
        txt (t, pl.getX() + 24, pl.getY() + 14, pl.getWidth() - 40, 50, title (fs).withExtraKerningFactor (0.04f), Colour (0xff1c1c1c));
        txt (String::fromUTF8 ("MHD-050  ·  ") + versionXY() + String::fromUTF8 ("  ·  MEIA VELOCIDADE  ·  FITA  ·  REWIND"),
             pl.getX() + 24, pl.getY() + 74, pl.getWidth() - 40, 20, bold (12.0f).withExtraKerningFactor (0.14f), Colour (0xff444444));
    }
    // --- plaquinha de latão DUB STATION
    {
        auto gp = Rectangle<float> (80, 160, 330, 52);
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (gp.translated (0, 4), 6.0f);
        ColourGradient brass (Colour (0xfff3d27a), 0, gp.getY(), Colour (0xffe8c165), 0, gp.getBottom(), false);
        brass.addColour (0.55, Colour (0xffc99a3a));
        g.setGradientFill (brass);
        g.fillRoundedRectangle (gp, 6.0f);
        g.setColour (Colour (0xff9c7426)); g.drawRoundedRectangle (gp.reduced (1), 6.0f, 2.0f);
        txt ("DUB STATION", gp.getX(), gp.getY() + 1, gp.getWidth(), gp.getHeight(), title (22.0f).withExtraKerningFactor (0.42f), Colours::white.withAlpha (0.4f), Justification::centred);
        txt ("DUB STATION", gp.getX(), gp.getY(), gp.getWidth(), gp.getHeight(), title (22.0f).withExtraKerningFactor (0.42f), Colour (0xff2a1c08), Justification::centred);
    }
    txt ("NO AR", 420, 174, 70, 18, bold (13.0f).withExtraKerningFactor (0.15f), ink);
    txt ("MIDI", 300, 262, 60, 18, bold (13.0f).withExtraKerningFactor (0.15f), ink);
    {
        auto r = Rectangle<float> (360, 254, 190, 40);
        g.setColour (Colour (0xff111111)); g.fillRoundedRectangle (r, 5.0f);
        g.setColour (Colour (0xff444444)); g.drawRoundedRectangle (r.reduced (1), 5.0f, 2.0f);
        txt (String::fromUTF8 ("C1–F1 PADS · C2 DROP"), r.getX(), r.getY(), r.getWidth(), r.getHeight(), mono (12.5f), amber, Justification::centred);
    }
    drawVuFace (g);

    // emenda dos painéis
    g.setColour (Colour (0xff8a7c60)); g.fillRect (40.0f, topH - 3, w - 80, 3.0f);
    g.setColour (Colour (0xff111111)); g.fillRect (40.0f, topH, w - 80, 4.0f);

    // --- títulos e placas rebaixadas
    const Font secF = bold (14.0f).withExtraKerningFactor (0.18f);
    const Font titleF = bold (23.0f).withExtraKerningFactor (0.08f);
    const Font hintF = Font (FontOptions (12.0f));
    const Colour txtC (0xffe8e2d6), hintC (0xff8a8070), mutedC (0xffb5ab9a);
    txt ("VELOCIDADE", 92, 346, 260, 30, titleF, amber);
    {   // anel do seletor
        const Point<float> c (216, 540);
        g.setColour (Colour (0xff0c0b0a)); g.fillEllipse (c.x - 140, c.y - 140, 280, 280);
        g.setColour (Colour (0xffb87a2e)); g.drawEllipse (c.x - 140, c.y - 140, 280, 280, 2.0f);
    }
    txt ("MOTOR", 76, 710, 70, 20, bold (12.0f).withExtraKerningFactor (0.15f), mutedC);
    txt ("DROP", 470, 346, 200, 30, titleF, amber);
    txt ("POWER DOWN", 390, 782, 180, 18, bold (13.0f), txtC, Justification::centred);
    txt ("POWER UP", 510, 782, 180, 18, bold (13.0f), txtC, Justification::centred);

    inset ({ 700, 338, 690, 442 });
    txt ("BANDAS", 724, 352, 200, 22, secF, txtC);
    txt (String::fromUTF8 ("cada faixa de frequência com a sua velocidade"), 840, 353, 400, 20, hintF, hintC);
    const char* bn[3] = { "GRAVE", "MÉDIO", "AGUDO" };
    const char* br[3] = { "< 180 Hz", "180 Hz – 2.4 kHz", "> 2.4 kHz" };
    for (int b = 0; b < 3; ++b)
    {
        const float x = 718.0f + (float) b * 226.0f;
        auto r = Rectangle<float> (x, 390, 212, 372);
        g.setGradientFill (ColourGradient (Colour (0xff221e1b), 0, r.getY(), Colour (0xff141210), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (Colours::white.withAlpha (0.06f)); g.drawRoundedRectangle (r, 8.0f, 1.0f);
        g.setColour (bandCol[b]); g.fillRoundedRectangle (r.withHeight (6.0f), 3.0f);
        txt (String::fromUTF8 (bn[b]), x + 16, 404, 150, 24, bold (17.0f).withExtraKerningFactor (0.15f), bandCol[b]);
        txt (String::fromUTF8 (br[b]), x + 16, 428, 180, 16, mono (11.0f), hintC);
        txt ("MIX", x + 10, 574, 100, 16, bold (13.0f), Colour (0xffeeeeee), Justification::centred);
        txt ("SMOOTH", x + 102, 574, 100, 16, bold (13.0f), Colour (0xffeeeeee), Justification::centred);
        txt ("LOOP", x + 16, 716, 60, 18, bold (11.0f).withExtraKerningFactor (0.1f), mutedC);
    }
    inset ({ 1408, 338, 226, 442 });
    txt (String::fromUTF8 ("MIX · SAÍDA"), 1408, 352, 226, 26, bold (18.0f).withExtraKerningFactor (0.08f), amber, Justification::centred);
    txt ("L", 1474, 602, 30, 14, bold (12.0f), Colour (0xffeeeeee), Justification::centred);
    txt ("R", 1522, 602, 30, 14, bold (12.0f), Colour (0xffeeeeee), Justification::centred);
    txt ("DRY/WET", 1420, 642, 100, 16, bold (13.0f), Colour (0xffeeeeee), Justification::centred);
    txt (String::fromUTF8 ("SAÍDA"), 1524, 642, 100, 16, bold (13.0f), Colour (0xffeeeeee), Justification::centred);

    inset ({ 56, 830, 620, 240 });
    txt ("SOUND SYSTEM", 80, 840, 200, 22, secF, txtC);
    txt (String::fromUTF8 ("pads disparados por botão ou nota MIDI"), 270, 841, 380, 20, hintF, hintC);
    txt (String::fromUTF8 ("DURAÇÃO"), 190, 1022, 120, 16, bold (12.0f), Colour (0xffeeeeee));
    txt (String::fromUTF8 ("com SYNC: 1/16 a 1 compasso"), 452, 1022, 210, 16, hintF, hintC);

    inset ({ 692, 830, 460, 240 });
    txt (String::fromUTF8 ("ECO DE FITA · WASHOUT"), 716, 840, 300, 22, secF, txtC);
    txt (String::fromUTF8 ("só no sinal lento"), 990, 841, 150, 20, hintF, hintC);
    {
        const char* en[4] = { "TEMPO", "INTENS.", "ENVIO", "WASHOUT" };
        for (int i = 0; i < 4; ++i) txt (en[i], 702.0f + (float) i * 106.0f, 892, 100, 16, bold (13.0f), Colour (0xffeeeeee), Justification::centred);
    }
    inset ({ 1168, 830, 466, 240 });
    txt (String::fromUTF8 ("FITA · SMOOTH"), 1192, 840, 250, 22, secF, txtC);
    txt ("WOW/FLUTTER", 1180, 882, 120, 16, bold (13.0f), Colour (0xffeeeeee), Justification::centred);
    txt (String::fromUTF8 ("SATURAÇÃO"), 1292, 882, 120, 16, bold (13.0f), Colour (0xffeeeeee), Justification::centred);
    {   // assinatura DubMastor
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::rotation (-0.07f, 1520, 960));
        txt ("DubMastor", 1420, 920, 240, 70, script (50.0f), Colour (0xffefe6cf), Justification::centredLeft);
        g.setColour (amber); g.fillRoundedRectangle (1424, 994, 200, 3, 1.5f);
    }

    inset ({ 56, 1090, 1578, 362 });
    txt (String::fromUTF8 ("EQ GRÁFICO"), 80, 1100, 200, 22, secF, txtC);
    txt (String::fromUTF8 ("arraste os pontos com o mouse · roda = largura · duplo clique zera"), 232, 1101, 560, 20, hintF, hintC);
    txt ("ONDE ENTRA", 80, 1402, 100, 18, bold (11.0f).withExtraKerningFactor (0.1f), mutedC);
    txt ("GANHO EQ", 800, 1402, 100, 18, bold (11.0f).withExtraKerningFactor (0.1f), mutedC);
    g.setColour (Colours::white.withAlpha (0.06f)); g.fillRect (996.0f, 1100.0f, 2.0f, 342.0f);
    {   // placa do Mastor Big Knob
        auto r = Rectangle<float> (1016, 1102, 600, 340);
        g.setGradientFill (ColourGradient (Colour (0xff221e1b), 0, r.getY(), Colour (0xff141210), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (Colours::white.withAlpha (0.06f)); g.drawRoundedRectangle (r, 8.0f, 1.0f);
        auto gp = Rectangle<float> (1032, 1114, 300, 34);
        g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (gp.translated (0, 3), 5.0f);
        ColourGradient brass (Colour (0xfff3d27a), 0, gp.getY(), Colour (0xffe8c165), 0, gp.getBottom(), false);
        brass.addColour (0.55, Colour (0xffc99a3a));
        g.setGradientFill (brass); g.fillRoundedRectangle (gp, 5.0f);
        g.setColour (Colour (0xff9c7426)); g.drawRoundedRectangle (gp.reduced (1), 5.0f, 1.5f);
        txt ("MASTOR BIG KNOB", gp.getX(), gp.getY(), gp.getWidth(), gp.getHeight(), title (15.0f).withExtraKerningFactor (0.28f), Colour (0xff2a1c08), Justification::centred);
        txt (String::fromUTF8 ("MBK-76 · filtro em degraus King Tubby"), 1346, 1122, 260, 18, Font (FontOptions (11.0f)), hintC);
        const char* kn[3] = { "RESSONÂNCIA", "DRIVE", "MIX" };
        for (int i = 0; i < 3; ++i) txt (String::fromUTF8 (kn[i]), 1328.0f + (float) i * 88.0f, 1278, 100, 16, bold (12.0f), Colour (0xffeeeeee), Justification::centred);
    }

    // parafusos dos cantos
    for (auto p : { Point<float> (58, 18), Point<float> (1652, 18), Point<float> (58, 300), Point<float> (1652, 300), Point<float> (58, 1452), Point<float> (1652, 1452) })
        screw (p.x, p.y);

    // laterais de madeira
    {
        Image::BitmapData bd (background, Image::BitmapData::readWrite);
        const int woodW = 40 * S;
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
                    float r = 0.42f + 0.06f * grain - 0.12f * ring + 0.03f * (rnd.nextFloat() - 0.5f);
                    r *= 0.55f + 0.45f * std::sin (MathConstants<float>::pi * ((float) i + 0.5f) / (float) woodW);
                    bd.setPixelColour (x0 + i, y, Colour::fromFloatRGBA (r * 1.05f, r * 0.62f, r * 0.33f, 1.0f));
                }
        }
    }
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawLine (40.0f, 0.0f, 40.0f, h, 1.5f);
    g.drawLine (w - 40.0f, 0.0f, w - 40.0f, h, 1.5f);
}
