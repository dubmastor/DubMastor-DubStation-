#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
const juce::Colour cream     (0xffe9e2cc);
const juce::Colour ink       (0xff3a2a1c);   // brown print on the enamel
const juce::Colour amber     (0xffffa630);
const juce::Colour lampOff   (0xff4a3418);

constexpr int kWood = 30;
constexpr int kW = 1080 + 2 * kWood, kH = 640;
constexpr int kTopH = 230;

// knob index -> parameter id / label
// 0 mode, 1 rate (GIANT, highlighted) | 2 intensity, 3 echo, 4 reverb (medium)
// 5 input, 6 wow, 7 age, 8 dry, 9 output (small)
constexpr int kNumKnobs = 10, kModeIdx = 0, kNumBig = 2, kNumMedium = 3;
const char* ids[kNumKnobs]    = { "mode", "rate", "intensity", "echo", "reverb",
                                  "input", "wow", "age", "dry", "output" };
const char* labels[kNumKnobs] = { "MODE SELECTOR", "REPEAT RATE", "INTENSITY", "ECHO VOLUME", "REVERB VOLUME",
                                  "INPUT", "WOW/FLUTTER", "TAPE AGE", "DRY", "OUTPUT" };
constexpr int kNumEq = 6;
constexpr int kNumEqPresets = 4;
const char* eqPresetNames[kNumEqPresets] = { "FLAT", "HPF", "LPF", "HPF+LPF" };
const float eqPresetValues[kNumEqPresets][kNumEq] = {
    {   0.0f,  0.0f,  0.0f,  0.0f,  0.0f,   0.0f },   // flat
    { -12.0f, -9.0f, -3.0f,  0.0f,  0.0f,   0.0f },   // high-pass: thin the repeats
    {   0.0f,  0.0f,  0.0f, -2.0f, -8.0f, -12.0f },   // low-pass: dark, dubby repeats
    { -12.0f, -9.0f, -3.0f, -2.0f, -8.0f, -12.0f } }; // band-pass: telephone-like repeats
const char* eqLabels[kNumEq] = { "100", "250", "630", "1.6k", "4k", "10k" };

// VU scale: linear in voltage from -20 VU to +3 VU, needle swing +-42 degrees
constexpr float kVuSwing = 0.7330f; // 42 degrees in radians
float vuToPos (float vu)
{
    const float a = std::pow (10.0f, juce::jlimit (-30.0f, 8.0f, vu) / 20.0f);
    return (a - 0.1f) / (1.4125f - 0.1f);
}

juce::String modeDescription (int m)
{
    const int mask = fitaeco::TapeEcho::headMask (m);
    juce::StringArray heads;
    for (int h = 0; h < 3; ++h) if (mask & (1 << h)) heads.add (juce::String (h + 1));
    juce::String s = "MODE " + juce::String (m) + "  -  ";
    s << (heads.isEmpty() ? juce::String ("so reverb") : "cabeca " + heads.joinIntoString (" + "));
    if (m >= 5 && m <= 11) s << " + reverb";
    return s;
}
} // namespace

// ------------------------------------------------------------------ look&feel
VintageLookAndFeel::VintageLookAndFeel()
{
    setColour (juce::Label::textColourId, cream);
}

void VintageLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float a0, float a1, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const bool stepped = s.getInterval() >= 1.0;
    const int steps = stepped ? (int) (s.getMaximum() - s.getMinimum()) + 1 : 11;
    const int sel = juce::roundToInt (pos * (float) (steps - 1));
    const float ang = stepped ? a0 + (float) sel / (float) (steps - 1) * (a1 - a0) : a0 + pos * (a1 - a0);

    // printed scale on the panel
    g.setFont (juce::FontOptions (juce::jmax (9.0f, r * 0.15f), juce::Font::bold));
    for (int i = 0; i < steps; ++i)
    {
        const float t = a0 + (float) i / (float) (steps - 1) * (a1 - a0);
        const bool lit = stepped && i == sel;
        g.setColour (lit ? amber : cream.withAlpha (0.8f));
        g.drawLine ({ c.getPointOnCircumference (r * 0.68f, t), c.getPointOnCircumference (r * 0.76f, t) }, 1.3f);
        const bool numbered = stepped || (r > 50.0f ? (i % 2 == 0) : (i == 0 || i == steps - 1 || i == steps / 2));
        if (numbered && r > 30.0f)
        {
            const auto np = c.getPointOnCircumference (r * 0.88f, t);
            const float bw = r * 0.3f;
            g.drawText (juce::String (stepped ? i + 1 : i), juce::Rectangle<float> (np.x - bw / 2, np.y - bw / 2, bw, bw),
                        juce::Justification::centred);
        }
    }

    // spun-aluminium skirt (giant knobs get a real cylinder: side wall, depth, reflections)
    const float kr = r * 0.62f;
    const bool big = r > 75.0f;
    if (big)
    {
        const float depth = kr * 0.15f;
        g.setColour (juce::Colours::black.withAlpha (0.30f));                  // long soft shadow
        g.fillEllipse (c.x - kr + 7, c.y - kr + 16, kr * 2, kr * 2);
        g.setColour (juce::Colours::black.withAlpha (0.45f));                  // contact shadow
        g.fillEllipse (c.x - kr + 2, c.y - kr + depth + 3, kr * 2, kr * 2);
        juce::ColourGradient side (juce::Colour (0xff8e8e8a), c.x - kr, c.y, juce::Colour (0xff8a8a86), c.x + kr, c.y, false);
        side.addColour (0.5, juce::Colour (0xff3a3a38));
        side.addColour (0.18, juce::Colour (0xffb8b8b4));
        g.setGradientFill (side);                                              // side wall
        g.fillEllipse (c.x - kr, c.y - kr + depth, kr * 2, kr * 2);
        g.fillRect (c.x - kr, c.y, kr * 2, depth);
        g.setColour (juce::Colours::black.withAlpha (0.35f));                  // knurling on the wall
        for (int i = 0; i <= 36; ++i)
        {
            const float t = juce::MathConstants<float>::halfPi + juce::MathConstants<float>::pi * (float) i / 36.0f;
            const auto top = c.getPointOnCircumference (kr, t);
            g.drawLine (top.x, top.y, top.x, top.y + depth, 1.0f);
        }
    }
    else
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (c.x - kr + 2, c.y - kr + 4, kr * 2, kr * 2);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f2f0), c.x - kr, c.y - kr,
                                             juce::Colour (0xff7a7a77), c.x + kr, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    for (int i = 0; i < 6; ++i) // concentric machining rings
    {
        const float rr = kr * (0.72f + 0.045f * (float) i);
        g.setColour (juce::Colours::white.withAlpha (i % 2 ? 0.10f : 0.0f).overlaidWith (juce::Colours::black.withAlpha (i % 2 ? 0.0f : 0.06f)));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 1.0f);
    }
    if (big) // anisotropic reflections of spun metal: two light and two dark sectors
    {
        const float inner = 0.66f;
        for (int k = 0; k < 4; ++k)
        {
            juce::Path sector;
            const float a = -0.95f + (float) k * juce::MathConstants<float>::halfPi;
            sector.addPieSegment (c.x - kr, c.y - kr, kr * 2, kr * 2, a - 0.28f, a + 0.28f, inner);
            g.setColour (k % 2 == 0 ? juce::Colours::white.withAlpha (0.30f) : juce::Colours::black.withAlpha (0.13f));
            g.fillPath (sector);
        }
        g.setColour (juce::Colours::white.withAlpha (0.55f));                  // bright rim on top
        juce::Path rim;
        rim.addCentredArc (c.x, c.y, kr - 1.0f, kr - 1.0f, 0.0f, -1.9f, 0.6f, true);
        g.strokePath (rim, juce::PathStrokeType (1.4f));
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.2f);
    g.setColour (stepped ? amber.darker (0.2f) : juce::Colour (0xff111111)); // index line on the skirt
    g.drawLine ({ c.getPointOnCircumference (kr * 0.70f, ang), c.getPointOnCircumference (kr * 0.98f, ang) },
                juce::jmax (1.8f, r * 0.03f));

    // black ribbed cap (raised, with a glossy highlight on the giant knobs)
    const float cr = kr * 0.68f;
    if (big)
    {
        const float capDepth = cr * 0.14f;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - cr + 1, c.y - cr + capDepth + 3, cr * 2, cr * 2);
        g.setColour (juce::Colour (0xff050505));
        g.fillEllipse (c.x - cr, c.y - cr + capDepth, cr * 2, cr * 2);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3a), c.x, c.y - cr,
                                             juce::Colour (0xff070707), c.x, c.y + cr, false));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    const int ribs = r > 50 ? 30 : 20;
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    for (int i = 0; i < ribs; ++i)
    {
        const float t = (float) i / (float) ribs * juce::MathConstants<float>::twoPi;
        g.drawLine ({ c.getPointOnCircumference (cr * 0.86f, t), c.getPointOnCircumference (cr * 0.99f, t) }, 1.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c2c), c.x, c.y - cr * 0.8f,
                                             juce::Colour (0xff0e0e0e), c.x, c.y + cr * 0.8f, false));
    g.fillEllipse (c.x - cr * 0.84f, c.y - cr * 0.84f, cr * 1.68f, cr * 1.68f);
    if (big)
    {
        const auto hl = juce::Point<float> (c.x - cr * 0.32f, c.y - cr * 0.40f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.38f), hl.x, hl.y,
                                                 juce::Colours::white.withAlpha (0.0f), hl.x + cr * 0.62f, hl.y, true));
        g.fillEllipse (c.x - cr * 0.84f, c.y - cr * 0.84f, cr * 1.68f, cr * 1.68f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        juce::Path capRim;
        capRim.addCentredArc (c.x, c.y, cr * 0.84f, cr * 0.84f, 0.0f, -1.7f, 0.4f, true);
        g.strokePath (capRim, juce::PathStrokeType (1.2f));
    }
    g.setColour (cream);
    g.drawLine ({ c.getPointOnCircumference (cr * 0.35f, ang), c.getPointOnCircumference (cr * 0.80f, ang) },
                juce::jmax (2.0f, r * 0.04f));
}

void VintageLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    // chrome bat-lever toggle: lever up = effect ON, lever down = BYPASS
    const auto r = b.getLocalBounds().toFloat();
    const auto c = r.getCentre();
    const bool bypassed = b.getToggleState();
    const float nut = juce::jmin (r.getWidth(), r.getHeight()) * 0.26f;

    juce::Path hex; // hex nut
    for (int i = 0; i < 6; ++i)
    {
        const auto p = c.getPointOnCircumference (nut, (float) i * juce::MathConstants<float>::pi / 3.0f + 0.52f);
        if (i == 0) hex.startNewSubPath (p); else hex.lineTo (p);
    }
    hex.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillPath (hex, juce::AffineTransform::translation (1.5f, 2.5f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f0ee), c.x - nut, c.y - nut, juce::Colour (0xff6d6d6a), c.x + nut, c.y + nut, false));
    g.fillPath (hex);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.strokePath (hex, juce::PathStrokeType (1.0f));
    g.setColour (juce::Colour (0xff222222));
    g.fillEllipse (c.x - nut * 0.45f, c.y - nut * 0.45f, nut * 0.9f, nut * 0.9f);

    // lever
    const float len = r.getHeight() * 0.42f;
    const float dir = bypassed ? 1.0f : -1.0f;
    const auto tip = juce::Point<float> (c.x, c.y + dir * len);
    juce::Path lever;
    lever.startNewSubPath (c.x - 3.2f, c.y);
    lever.lineTo (c.x + 3.2f, c.y);
    lever.lineTo (tip.x + 2.0f, tip.y);
    lever.lineTo (tip.x - 2.0f, tip.y);
    lever.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillPath (lever, juce::AffineTransform::translation (2.5f, 3.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8c8c88), c.x - 3, c.y, juce::Colour (0xff8c8c88), c.x + 3, c.y, false));
    g.fillPath (lever);
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.drawLine (c.x - 0.6f, c.y, tip.x - 0.6f, tip.y, 1.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, tip.x - 3, tip.y - 3, juce::Colour (0xff6a6a66), tip.x + 4, tip.y + 4, false));
    g.fillEllipse (tip.x - 4.5f, tip.y - 4.5f, 9.0f, 9.0f);
    if (highlighted)
    {
        g.setColour (amber.withAlpha (0.25f));
        g.drawEllipse (r.reduced (2.0f), 1.0f);
    }
}

void VintageLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    // small EQ preset keys: dark bakelite, lit amber when the preset is active
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (0.0f, down ? 0.5f : 1.5f), 3.0f);
    if (on)
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffc061), 0, r.getY(), juce::Colour (0xffd9861a), 0, r.getBottom(), false));
    else
        g.setGradientFill (juce::ColourGradient (juce::Colour (highlighted ? 0xff3e3a35 : 0xff34312d), 0, r.getY(),
                                                 juce::Colour (0xff181614), 0, r.getBottom(), false));
    g.fillRoundedRectangle (down ? r.translated (0.0f, 1.0f) : r, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.5f : 0.12f));
    g.drawLine (r.getX() + 3, r.getY() + 0.8f, r.getRight() - 3, r.getY() + 0.8f, 1.0f);
}

void VintageLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.06f));
    g.setColour (b.getToggleState() ? juce::Colour (0xff1a1205) : cream.withAlpha (0.85f));
    g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, down ? 1 : 0), juce::Justification::centred);
}

void VintageLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                                           float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cx = (float) x + (float) w * 0.5f;
    const float thumbR = (float) getSliderThumbRadius (s);
    const float top = (float) y + thumbR, bot = (float) (y + h) - thumbR;

    // scale ticks every 6 dB, longer at 0 dB
    for (int i = 0; i <= 4; ++i)
    {
        const float ty = top + (bot - top) * (float) i / 4.0f;
        const float len = i == 2 ? 13.0f : 8.0f;
        g.setColour (cream.withAlpha (i == 2 ? 0.8f : 0.45f));
        g.drawLine (cx - 6 - len, ty, cx - 6, ty, 1.0f);
        g.drawLine (cx + 6, ty, cx + 6 + len, ty, 1.0f);
    }
    // slot
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (cx - 2.5f, top - 4, 5.0f, bot - top + 8, 2.5f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (cx - 2.5f, top - 4, 5.0f, bot - top + 8, 2.5f, 1.0f);

    // chrome fader cap
    const float cw = juce::jmin ((float) w - 6.0f, 30.0f), ch = 18.0f;
    auto cap = juce::Rectangle<float> (cx - cw / 2, sliderPos - ch / 2, cw, ch);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (cap.translated (1.5f, 3.0f), 3.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f0f0), cap.getX(), cap.getY(),
                                             juce::Colour (0xff6e6e6e), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    for (float ly : { cap.getY() + 4.0f, cap.getBottom() - 4.0f })
        g.drawLine (cap.getX() + 3, ly, cap.getRight() - 3, ly, 1.0f);
    g.setColour (amber.darker (0.1f));
    g.fillRect (cap.getX() + 2, cap.getCentreY() - 1.0f, cap.getWidth() - 4, 2.0f);
}

// ---------------------------------------------------------------------- editor
FitaEcoEditor::FitaEcoEditor (FitaEcoProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    plateFace  = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoBlackRegular_ttf,
                                                          (size_t) BinaryData::ArchivoBlackRegular_ttfSize);
    scriptFace = juce::Typeface::createSystemTypefaceFor (BinaryData::KaushanScriptRegular_ttf,
                                                          (size_t) BinaryData::KaushanScriptRegular_ttfSize);
    for (int i = 0; i < kNumKnobs; ++i)
    {
        auto& k = knobs[(size_t) i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (k.slider);
        k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids[i], k.slider);
        k.label.setText (labels[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        const float fs = i < kNumBig ? 17.0f : (i < kNumBig + kNumMedium ? 13.0f : 9.5f);
        k.label.setFont (juce::FontOptions (fs, juce::Font::bold));
        k.label.setColour (juce::Label::textColourId, i < kNumBig ? amber : cream);
        addAndMakeVisible (k.label);
    }

    for (int b = 0; b < kNumEq; ++b)
    {
        auto& f = eqFaders[(size_t) b];
        f.slider.setSliderStyle (juce::Slider::LinearVertical);
        f.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (f.slider);
        f.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, "eq" + juce::String (b + 1), f.slider);
        f.slider.setDoubleClickReturnValue (true, 0.0);
        f.label.setText (eqLabels[b], juce::dontSendNotification);
        f.label.setJustificationType (juce::Justification::centred);
        f.label.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        f.label.setColour (juce::Label::textColourId, cream);
        addAndMakeVisible (f.label);
    }

    for (int i = 0; i < kNumEqPresets; ++i)
    {
        auto& b = eqPresets[(size_t) i];
        b.setButtonText (eqPresetNames[i]);
        b.setClickingTogglesState (false);
        b.onClick = [this, i] { applyEqPreset (i); };
        addAndMakeVisible (b);
    }

    bypassSwitch.setName ("bypass");
    bypassSwitch.setTooltip ("Bypass: alavanca para cima = efeito ligado");
    addAndMakeVisible (bypassSwitch);
    bypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "bypass", bypassSwitch);

    status.setJustificationType (juce::Justification::centredLeft);
    status.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    status.setColour (juce::Label::textColourId, amber);
    addAndMakeVisible (status);

    setSize (kW, kH);
    startTimerHz (30);
}

FitaEcoEditor::~FitaEcoEditor() { setLookAndFeel (nullptr); }

void FitaEcoEditor::applyEqPreset (int idx)
{
    for (int b = 0; b < kNumEq; ++b)
        if (auto* prm = proc.apvts.getParameter ("eq" + juce::String (b + 1)))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (eqPresetValues[idx][b]));
            prm->endChangeGesture();
        }
}

int FitaEcoEditor::matchingEqPreset() const
{
    for (int i = 0; i < kNumEqPresets; ++i)
    {
        bool same = true;
        for (int b = 0; b < kNumEq && same; ++b)
            same = std::abs (proc.apvts.getRawParameterValue ("eq" + juce::String (b + 1))->load() - eqPresetValues[i][b]) < 0.05f;
        if (same) return i;
    }
    return -1;
}

void FitaEcoEditor::timerCallback()
{
    const float rate = proc.apvts.getRawParameterValue ("rate")->load();
    tapePhase += 0.6f + 2.2f * rate;
    shownMode = (int) proc.apvts.getRawParameterValue ("mode")->load();

    // VU ballistics (~300 ms rise, slight overshoot), peak lamp above +3 VU
    const float vu = proc.getRecVu();
    const float target = juce::jlimit (-0.03f, 1.06f, vuToPos (vu));
    needleVel = (needleVel + (target - needlePos) * 0.30f) * 0.58f;
    needlePos += needleVel;
    peakHold = vu > 3.0f ? 15 : juce::jmax (0, peakHold - 1);
    repaint (vuArea);

    juce::String text = modeDescription (shownMode);
    for (int i = 0; i < kNumKnobs; ++i)
    {
        auto& s = knobs[(size_t) i].slider;
        if (s.isMouseOverOrDragging())
        {
            if (i != kModeIdx)
                text = juce::String (labels[i]) + ":  " + s.getTextFromValue (s.getValue());
            break;
        }
    }
    for (int b = 0; b < kNumEq; ++b)
    {
        auto& f = eqFaders[(size_t) b].slider;
        if (f.isMouseOverOrDragging())
            text = "EQ " + juce::String (eqLabels[b]) + "Hz:  " + (f.getValue() > 0 ? "+" : "")
                   + juce::String (f.getValue(), 1) + " dB";
    }
    status.setText (text, juce::dontSendNotification);
    repaint (tapeWindow);

    const int active = matchingEqPreset();
    for (int i = 0; i < kNumEqPresets; ++i)
        eqPresets[(size_t) i].setToggleState (i == active, juce::dontSendNotification);
    repaint (juce::Rectangle<int> (16, 16).withCentre (bypassLamp.toInt()).expanded (6));
}

void FitaEcoEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    // metal name plate: engraved "DUB STATION", tagline, version in an amber inset
    const auto pl = plateArea;
    auto engraved = [&g] (const juce::String& t, juce::Rectangle<float> r, juce::Justification j, juce::Colour c)
    {
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawText (t, r.translated (0.0f, 1.0f), j);
        g.setColour (c);
        g.drawText (t, r, j);
    };
    g.setFont (juce::FontOptions (plateFace).withHeight (28.0f).withKerningFactor (0.04f));
    engraved ("DUB STATION", { pl.getX() + 16, pl.getY() + 10, pl.getWidth() - 100, 40 },
              juce::Justification::centredLeft, juce::Colour (0xff262626));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.12f));
    engraved (juce::String (juce::CharPointer_UTF8 ("TAPE ECHO  \xc2\xb7  SPRING REVERB")), { pl.getX() + 17, pl.getY() + 54, pl.getWidth() - 100, 14 },
              juce::Justification::centredLeft, juce::Colour (0xff3a3a3a));
    const auto inset = juce::Rectangle<float> (pl.getRight() - 74, pl.getY() + 18, 56, 46);
    g.setFont (juce::FontOptions (plateFace).withHeight (24.0f));
    g.setColour (amber.withAlpha (0.35f));
    g.drawText ("2.1", inset.expanded (1.0f), juce::Justification::centred);
    g.setColour (amber);
    g.drawText ("2.1", inset, juce::Justification::centred);

    // "DUB MACHINE" printed on the enamel
    g.setColour (ink);
    g.setFont (juce::FontOptions (plateFace).withHeight (27.0f).withKerningFactor (0.10f));
    g.drawText ("DUB MACHINE", juce::Rectangle<float> (pl.getX() + 6, pl.getBottom() + 12, 330, 34), juce::Justification::centredLeft);
    g.setColour (ink.withAlpha (0.7f));
    g.drawLine (pl.getX() + 8, pl.getBottom() + 50, pl.getRight() - 6, pl.getBottom() + 50, 1.2f);

    // bypass switch legends + effect lamp
    {
        const auto sw = bypassSwitch.getBounds().toFloat();
        g.setColour (ink);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.08f));
        g.drawText ("ON", juce::Rectangle<float> (sw.getRight() + 2, sw.getY() + 2, 60, 12), juce::Justification::centredLeft);
        g.drawText ("BYPASS", juce::Rectangle<float> (sw.getRight() + 2, sw.getBottom() - 14, 60, 12), juce::Justification::centredLeft);
        const bool on = ! bypassSwitch.getToggleState();
        const auto lp = bypassLamp;
        g.setColour (juce::Colour (0xff8a8a8a));
        g.fillEllipse (lp.x - 13, lp.y - 13, 26, 26);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawEllipse (lp.x - 13, lp.y - 13, 26, 26, 1.0f);
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0x88ff3b2f), lp.x, lp.y, juce::Colour (0x00ff3b2f), lp.x + 22, lp.y, true));
            g.fillEllipse (lp.x - 22, lp.y - 22, 44, 44);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffd0c0), lp.x - 3, lp.y - 3, juce::Colour (0xffd8231a), lp.x + 8, lp.y + 8, true));
        }
        else
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7a3a32), lp.x - 3, lp.y - 3, juce::Colour (0xff3a0e0a), lp.x + 8, lp.y + 8, true));
        g.fillEllipse (lp.x - 9, lp.y - 9, 18, 18);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.fillEllipse (lp.x - 5, lp.y - 6, 4, 3);
        g.setColour (ink);
        g.drawText ("EFFECT ON", juce::Rectangle<float> (lp.x + 18, lp.y - 7, 90, 14), juce::Justification::centredLeft);
    }

    // "DubMastor" signature, brush script, bottom-right corner of the dark panel
    {
        juce::Graphics::ScopedSaveState ss (g);
        const auto sa = sigArea.toFloat();
        g.addTransform (juce::AffineTransform::rotation (-0.07f, sa.getRight(), sa.getCentreY()));
        g.setFont (juce::FontOptions (scriptFace).withHeight (38.0f));
        const auto txt = juce::Rectangle<float> (sa.getX(), sa.getY() - 16, sa.getWidth() - 20, 42);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText ("DubMastor", txt.translated (1.5f, 2.0f), juce::Justification::centredRight);
        g.setColour (cream);
        g.drawText ("DubMastor", txt, juce::Justification::centredRight);
        juce::Path swash;
        const float x1 = sa.getRight() - 22, y1 = sa.getBottom() - 14;
        swash.startNewSubPath (x1 - 190, y1 + 2);
        swash.cubicTo (x1 - 130, y1 - 4, x1 - 60, y1 + 4, x1, y1 - 6);
        g.setColour (amber.withAlpha (0.85f));
        g.strokePath (swash, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // graphic EQ captions
    auto eq = eqArea.toFloat();
    g.setColour (cream.withAlpha (0.85f));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("GRAPHIC EQ", eq.withHeight (26.0f).withTrimmedLeft (10), juce::Justification::centredLeft);
    if (auto* f0 = &eqFaders[0].slider; f0->getHeight() > 0)
    {
        const float thumbR = (float) lnf.getSliderThumbRadius (*f0);
        const float top = (float) f0->getY() + thumbR, bot = (float) f0->getBottom() - thumbR;
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        const char* dbTxt[] = { "+12", "0", "-12" };
        for (int i = 0; i < 3; ++i)
        {
            const float ty = top + (bot - top) * (float) i / 2.0f;
            g.setColour (cream.withAlpha (i == 1 ? 0.85f : 0.55f));
            g.drawText (dbTxt[i], juce::Rectangle<float> (eq.getX() + 6, ty - 6, 28, 12), juce::Justification::centredRight);
        }
    }

    drawTapeTransport (g);
    drawVuNeedle (g);
}

void FitaEcoEditor::drawTapeTransport (juce::Graphics& g)
{
    auto win = tapeWindow.toFloat();
    // recessed smoked-glass window
    g.setColour (juce::Colour (0xff0d0f0e));
    g.fillRoundedRectangle (win, 10.0f);

    const float L = win.getX() + 70, R = win.getRight() - 70;
    const float T = win.getY() + 38, B = win.getY() + 128;
    const float cy = (T + B) * 0.5f, rad = (B - T) * 0.5f;

    // tape loop path
    juce::Path loop;
    loop.addRoundedRectangle (L - rad, T, (R - L) + 2 * rad, B - T, rad);
    const float perim = loop.getLength();

    g.setColour (juce::Colours::black.withAlpha (0.55f));                 // shadow of the tape on the deck
    g.strokePath (loop, juce::PathStrokeType (9.0f), juce::AffineTransform::translation (1.5f, 3.5f));
    g.setColour (juce::Colour (0xff1e1008));                               // tape edges
    g.strokePath (loop, juce::PathStrokeType (9.0f));
    g.setColour (juce::Colour (0xff5a361b));                               // oxide coating
    g.strokePath (loop, juce::PathStrokeType (7.0f));
    g.setColour (juce::Colour (0xff7d4f2a).withAlpha (0.85f));             // sheen band along the middle
    g.strokePath (loop, juce::PathStrokeType (2.6f));

    // moving grain: irregular light/dark specks that travel with the tape (speed = repeat rate)
    {
        const float step = 3.5f;
        const int base = (int) std::floor (tapePhase / step);
        const float off = tapePhase - (float) base * step;
        int k = 0;
        for (float d = off; d < perim; d += step, ++k)
        {
            const auto id = (juce::uint32) (k - base);
            const juce::uint32 h = (id * 2654435761u) ^ (id >> 7);
            const float a = (float) ((h >> 8) & 255) / 255.0f;
            if (a < 0.45f) continue;
            const auto pt = loop.getPointAlongPath (d);
            const bool light = (h & 1) != 0;
            const float jitter = (float) ((h >> 16) & 7) / 7.0f * 4.0f - 2.0f;  // across the tape width
            const auto nxt = loop.getPointAlongPath (juce::jmin (perim, d + 0.8f));
            const auto dirv = (nxt - pt);
            const float len = juce::jmax (0.001f, std::sqrt (dirv.x * dirv.x + dirv.y * dirv.y));
            const juce::Point<float> nrm (-dirv.y / len, dirv.x / len);
            const auto q = pt + nrm * jitter;
            g.setColour (light ? juce::Colour (0xffb07a46).withAlpha (0.25f + 0.35f * a) : juce::Colour (0xff1a0d05).withAlpha (0.20f + 0.30f * a));
            g.fillEllipse (q.x - 1.2f, q.y - 0.8f, 2.4f, 1.6f);
        }
    }
    g.setColour (juce::Colours::white.withAlpha (0.10f));                  // thin specular edge
    g.strokePath (loop, juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0.0f, -3.0f));

    // flanged chrome guide wheels, rubber pinch roller, polished capstan
    auto wheel = [&g] (juce::Point<float> c, float r, float spin, bool rubber)
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - r + 2, c.y - r + 3, r * 2, r * 2);
        if (rubber)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c2c), c.x - r, c.y - r, juce::Colour (0xff050505), c.x + r, c.y + r, false));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            juce::Path sheen;
            sheen.addCentredArc (c.x, c.y, r * 0.8f, r * 0.8f, 0.0f, spin, spin + 1.2f, true);
            g.strokePath (sheen, juce::PathStrokeType (1.5f));
        }
        else
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f2f0), c.x - r, c.y - r, juce::Colour (0xff5c5c5a), c.x + r, c.y + r, false));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.drawEllipse (c.x - r * 0.86f, c.y - r * 0.86f, r * 1.72f, r * 1.72f, 1.0f); // flange step
        }
        const float hub = r * (rubber ? 0.45f : 0.62f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe6e6e4), c.x - hub, c.y - hub, juce::Colour (0xff6e6e6c), c.x + hub, c.y + hub, false));
        g.fillEllipse (c.x - hub, c.y - hub, hub * 2, hub * 2);
        g.setColour (juce::Colour (0xff1c1c1c));                         // rotating holes
        for (int i = 0; i < 3; ++i)
        {
            const auto hp = c.getPointOnCircumference (hub * 0.55f, spin + (float) i * 2.0944f);
            const float hr = hub * 0.2f;
            g.fillEllipse (hp.x - hr, hp.y - hr, hr * 2, hr * 2);
        }
        g.setColour (juce::Colour (0xff333333));
        g.fillEllipse (c.x - 2.2f, c.y - 2.2f, 4.4f, 4.4f);
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.fillEllipse (c.x - r * 0.55f, c.y - r * 0.62f, r * 0.35f, r * 0.22f);
    };
    wheel ({ L - rad + 26.0f, cy }, 22.0f, tapePhase * 0.05f, false);
    wheel ({ R + rad - 26.0f, cy }, 22.0f, tapePhase * 0.05f, false);
    wheel ({ R - 20.0f, B - 12.0f }, 14.0f, -tapePhase * 0.08f, true);   // pinch roller
    {
        const auto cap = juce::Point<float> (R - 20.0f, B + 12.0f);          // capstan shaft
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffffff), cap.x - 5, cap.y - 5, juce::Colour (0xff707070), cap.x + 5, cap.y + 5, false));
        g.fillEllipse (cap.x - 5.5f, cap.y - 5.5f, 11.0f, 11.0f);
        g.setColour (juce::Colour (0xff2a2a2a));
        g.drawLine ({ cap, cap.getPointOnCircumference (5.0f, tapePhase * 0.25f) }, 1.4f);
    }

    // heads along the bottom run: ERASE, REC, 1, 2, 3 (metal shields with a gap and two screws)
    const int mask = fitaeco::TapeEcho::headMask (juce::jlimit (1, 12, shownMode));
    const char* hn[] = { "ERASE", "REC", "1", "2", "3" };
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    for (int h = 0; h < 5; ++h)
    {
        const float hx = L + 40.0f + (float) h * ((R - L - 90.0f) / 4.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (hx - 11, B + 1, 24, 20, 3.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd8d8d6), hx - 12, B - 2,
                                                 juce::Colour (0xff56565a), hx + 12, B + 20, false));
        g.fillRoundedRectangle (hx - 12, B - 2, 24, 20, 3.0f);
        g.setColour (juce::Colour (0xff101010));
        g.drawLine (hx, B - 2, hx, B + 5, 1.2f);                         // head gap
        g.setColour (juce::Colour (0xff3a3a3a));
        g.fillEllipse (hx - 9, B + 11, 3.5f, 3.5f);
        g.fillEllipse (hx + 5.5f, B + 11, 3.5f, 3.5f);
        const bool on = h <= 1 ? (h == 1) : (mask & (1 << (h - 2))) != 0;
        g.setColour (on ? amber : lampOff);
        g.fillEllipse (hx - 4, B + 24, 8, 8);
        g.setColour (cream.withAlpha (0.75f));
        g.drawText (hn[h], juce::Rectangle<float> (hx - 22, B + 33, 44, 11), juce::Justification::centred);
    }

    // spring reverb lamp
    const bool rev = fitaeco::TapeEcho::reverbOn (shownMode);
    const auto rp = juce::Point<float> (win.getRight() - 30.0f, win.getY() + 18.0f);
    g.setColour (rev ? amber : lampOff);
    g.fillEllipse (rp.x - 4, rp.y - 4, 8, 8);
    g.setColour (cream.withAlpha (0.75f));
    g.drawText ("SPRING", juce::Rectangle<float> (rp.x - 60, rp.y - 6, 50, 12), juce::Justification::right);

    // glass reflection + bezel
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), win.getX(), win.getY(),
                                             juce::Colours::transparentWhite, win.getX() + 120, win.getBottom(), false));
    g.fillRoundedRectangle (win, 10.0f);
    g.setColour (juce::Colour (0xff8f8a7c));
    g.drawRoundedRectangle (win.expanded (2.0f), 11.0f, 3.0f);
}

// ---------------------------------------------------------------- VU meter
namespace
{
struct VuGeom
{
    juce::Rectangle<float> housing, face, label;
    juce::Point<float> pivot;
    float radius;
};
VuGeom vuGeom (juce::Rectangle<int> area)
{
    VuGeom v;
    v.housing = area.toFloat();
    v.face = v.housing.reduced (11.0f).withHeight (124.0f);
    v.label = juce::Rectangle<float> (v.housing.getX(), v.face.getBottom() + 6.0f, v.housing.getWidth(), v.housing.getBottom() - v.face.getBottom() - 10.0f);
    v.pivot = { v.face.getCentreX(), v.face.getBottom() + 6.0f };
    v.radius = v.face.getHeight() * 0.74f;
    return v;
}
} // namespace

void FitaEcoEditor::drawVuFace (juce::Graphics& g)
{
    const auto v = vuGeom (vuArea);
    // black bakelite housing with chrome inner ring
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (v.housing.translated (2.0f, 3.0f), 8.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2a2a), 0, v.housing.getY(), juce::Colour (0xff0b0b0b), 0, v.housing.getBottom(), false));
    g.fillRoundedRectangle (v.housing, 8.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8e8e8), v.face.getX(), v.face.getY(), juce::Colour (0xff4c4c4c), v.face.getRight(), v.face.getBottom(), false));
    g.drawRoundedRectangle (v.face.expanded (3.0f), 4.0f, 2.5f);

    // backlit face (warm incandescent glow, brighter in the middle)
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7e8b8), v.face.getCentreX(), v.face.getY() + v.face.getHeight() * 0.55f,
                                             juce::Colour (0xffd9b86a), v.face.getX(), v.face.getY(), true));
    g.fillRoundedRectangle (v.face, 3.0f);

    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (v.face.toNearestInt());

    const auto c = v.pivot;
    const float R = v.radius;
    auto angleOf = [] (float pos) { return -kVuSwing + 2.0f * kVuSwing * juce::jlimit (0.0f, 1.0f, pos); };

    // scale arcs: black up to 0 VU, red from 0 to +3
    juce::Path black, red;
    black.addCentredArc (c.x, c.y, R, R, 0.0f, angleOf (0.0f), angleOf (vuToPos (0.0f)), true);
    red.addCentredArc (c.x, c.y, R, R, 0.0f, angleOf (vuToPos (0.0f)), angleOf (1.0f), true);
    g.setColour (juce::Colour (0xff1a1a1a));
    g.strokePath (black, juce::PathStrokeType (1.6f));
    g.setColour (juce::Colour (0xffc0262d));
    g.strokePath (red, juce::PathStrokeType (5.0f));

    // ticks + numbers
    const float marks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    for (float m : marks)
    {
        const float t = angleOf (vuToPos (m));
        const bool hot = m > 0.0f, major = m != -2.0f;
        g.setColour (hot ? juce::Colour (0xffc0262d) : juce::Colour (0xff1a1a1a));
        g.drawLine ({ c.getPointOnCircumference (R - (major ? 7.0f : 4.0f), t), c.getPointOnCircumference (R + 2.0f, t) }, major ? 1.6f : 1.0f);
        if (major)
        {
            const auto np = c.getPointOnCircumference (R + 11.0f, t);
            const juce::String txt = m > 0 ? "+" + juce::String ((int) m) : juce::String ((int) std::abs (m));
            g.drawText (txt, juce::Rectangle<float> (np.x - 12, np.y - 7, 24, 14), juce::Justification::centred);
        }
    }
    g.setColour (juce::Colour (0xff1a1a1a));
    g.setFont (juce::FontOptions (plateFace).withHeight (22.0f));
    g.drawText ("VU", juce::Rectangle<float> (v.face.getX(), v.face.getY() + v.face.getHeight() * 0.50f, v.face.getWidth(), 26), juce::Justification::centred);
}

void FitaEcoEditor::drawVuNeedle (juce::Graphics& g)
{
    const auto v = vuGeom (vuArea);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (v.face.toNearestInt());
        const float t = -kVuSwing + 2.0f * kVuSwing * juce::jlimit (-0.03f, 1.06f, needlePos);
        const auto tip = v.pivot.getPointOnCircumference (v.radius + 4.0f, t);
        const auto base = v.pivot.getPointOnCircumference (v.radius * 0.30f, t);
        g.setColour (juce::Colours::black.withAlpha (0.18f)); // soft shadow on the face
        g.drawLine ({ base.translated (2.0f, 3.0f), tip.translated (2.0f, 3.0f) }, 1.6f);
        g.setColour (juce::Colour (0xff111111));
        g.drawLine ({ base, tip }, 1.4f);

        // dark cover hiding the pivot, with the meter's screw
        auto cover = v.face.withTop (v.face.getBottom() - 20.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b2b2b), 0, cover.getY(), juce::Colour (0xff0c0c0c), 0, cover.getBottom(), false));
        g.fillRect (cover);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawLine (cover.getX(), cover.getY(), cover.getRight(), cover.getY(), 1.0f);

        // glass reflection
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.22f), v.face.getX(), v.face.getY(),
                                                 juce::Colours::transparentWhite, v.face.getX() + v.face.getWidth() * 0.55f, v.face.getY() + v.face.getHeight() * 0.6f, false));
        juce::Path glare;
        glare.startNewSubPath (v.face.getX(), v.face.getY());
        glare.lineTo (v.face.getRight() * 0.6f + v.face.getX() * 0.4f, v.face.getY());
        glare.lineTo (v.face.getX(), v.face.getY() + v.face.getHeight() * 0.7f);
        glare.closeSubPath();
        g.fillPath (glare);
    }

    // label strip: REC LEVEL + PEAK lamp
    g.setColour (cream.withAlpha (0.85f));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold).withKerningFactor (0.12f));
    g.drawText ("REC LEVEL", v.label.withTrimmedLeft (14.0f), juce::Justification::centredLeft);
    const auto lp = juce::Point<float> (v.label.getRight() - 24.0f, v.label.getCentreY());
    g.setColour (peakHold > 0 ? juce::Colour (0xffff3b2f) : juce::Colour (0xff4a1612));
    g.fillEllipse (lp.x - 4.5f, lp.y - 4.5f, 9.0f, 9.0f);
    if (peakHold > 0)
    {
        g.setColour (juce::Colour (0x66ff3b2f));
        g.fillEllipse (lp.x - 8.0f, lp.y - 8.0f, 16.0f, 16.0f);
    }
    g.setColour (cream.withAlpha (0.6f));
    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
    g.drawText ("PEAK", juce::Rectangle<float> (lp.x - 46.0f, lp.y - 6.0f, 36.0f, 12.0f), juce::Justification::centredRight);
}

void FitaEcoEditor::resized()
{
    auto content = getLocalBounds().reduced (kWood, 0);
    topPanel = content.removeFromTop (kTopH);
    plateArea = juce::Rectangle<float> ((float) topPanel.getX() + 28.0f, 24.0f, 332.0f, 82.0f);
    bottomPanel = content;
    {
        auto tp = topPanel;
        tp.removeFromLeft (380);
        vuArea = tp.removeFromLeft (200).reduced (10, 22);
        tapeWindow = tp.reduced (24, 22);
    }
    status.setBounds (tapeWindow.getX() + 14, tapeWindow.getY() + 8, 300, 18);
    bypassSwitch.setBounds ((int) plateArea.getX() + 8, kTopH - 70, 42, 60);
    bypassLamp = { plateArea.getX() + 170.0f, (float) kTopH - 40.0f };

    auto rows = bottomPanel.reduced (10, 0).withTrimmedTop (12).withTrimmedBottom (10);
    auto rowA = rows.removeFromTop (222);
    rows.removeFromTop (8);
    auto rowB = rows;

    auto place = [] (Knob& k, juce::Rectangle<int> cell, int labelH)
    {
        cell = cell.reduced (4);
        const int side = juce::jmin (cell.getWidth(), cell.getHeight() - labelH);
        cell = cell.withSizeKeepingCentre (cell.getWidth(), labelH + side); // label sits right above its knob
        k.label.setBounds (cell.removeFromTop (labelH));
        k.slider.setBounds (cell.withSizeKeepingCentre (side, side));
    };

    // row A: giant MODE + REPEAT RATE, then the medium echo knobs
    for (int i = 0; i < kNumBig; ++i) place (knobs[(size_t) i], rowA.removeFromLeft (240), 34);
    const int wm = 150; // fixed widths keep MODE / REPEAT RATE the stars
    rowA.removeFromLeft ((rowA.getWidth() - kNumMedium * wm) / 2);
    for (int i = kNumBig; i < kNumBig + kNumMedium; ++i) place (knobs[(size_t) i], rowA.removeFromLeft (wm), 18);

    // row B: graphic EQ on the left, small utility knobs on the right
    eqArea = rowB.removeFromLeft (460);
    auto faders = eqArea.reduced (8, 6);
    {
        auto caption = faders.removeFromTop (22);   // "GRAPHIC EQ" + preset keys
        caption.removeFromLeft (100);
        const int pw = 62;
        caption.removeFromLeft (caption.getWidth() - kNumEqPresets * pw - 4);
        for (auto& b : eqPresets) b.setBounds (caption.removeFromLeft (pw).reduced (2, 1));
    }
    faders.removeFromTop (4);
    faders.removeFromLeft (30);  // dB scale
    auto freqRow = faders.removeFromBottom (14);
    const int fw = faders.getWidth() / kNumEq;
    for (int b = 0; b < kNumEq; ++b)
    {
        auto& f = eqFaders[(size_t) b];
        f.slider.setBounds (faders.removeFromLeft (fw).reduced (8, 0));
        f.label.setBounds (freqRow.removeFromLeft (fw));
    }
    sigArea = rowB.removeFromBottom (44);
    const int ws = 96;
    rowB.removeFromLeft ((rowB.getWidth() - (kNumKnobs - kNumBig - kNumMedium) * ws) / 2);
    for (int i = kNumBig + kNumMedium; i < kNumKnobs; ++i) place (knobs[(size_t) i], rowB.removeFromLeft (ws), 15);

    renderBackground();
}

// ---------------------------------------------------------------- textures
void FitaEcoEditor::renderBackground()
{
    const int S = 2;
    const int W = getWidth() * S, H = getHeight() * S, topH = kTopH * S;
    background = juce::Image (juce::Image::RGB, W, H, false);
    juce::Random rnd (1975);

    {   // enamel (top) + dark painted steel (bottom), per-pixel
        juce::Image::BitmapData bd (background, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < H; ++y)
        {
            const float vy = (float) y / (float) H;
            for (int x = 0; x < W; ++x)
            {
                const float vx = (float) x / (float) W;
                const float n = rnd.nextFloat() - 0.5f;
                if (y < topH)
                {
                    // cream enamel, slightly yellowed and grimy towards the edges
                    const float ey = (float) y / (float) topH;
                    const float edge = juce::jmax (std::pow (std::abs (vx - 0.5f) * 2.0f, 4.0f), std::pow (std::abs (ey - 0.5f) * 2.0f, 4.0f));
                    float k = 1.0f - 0.18f * edge + 0.025f * n + 0.02f * std::sin (vx * 23.0f) * std::sin (ey * 7.0f);
                    bd.setPixelColour (x, y, juce::Colour::fromFloatRGBA (0.85f * k, 0.80f * k * (1.0f - 0.04f * edge), 0.68f * k * (1.0f - 0.10f * edge), 1.0f));
                }
                else
                {
                    float k = 0.12f + 0.015f * n + 0.01f * std::sin (vy * 90.0f + vx * 4.0f);
                    k *= 1.0f - 0.3f * std::pow (std::abs (vx - 0.5f) * 2.0f, 4.0f);
                    bd.setPixelColour (x, y, juce::Colour::fromFloatRGBA (k * 1.08f, k * 0.98f, k * 0.90f, 1.0f));
                }
            }
        }
    }

    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale ((float) S));
    const float w = (float) getWidth(), h = (float) getHeight(), tH = (float) kTopH;

    // enamel wear: chips and stains
    for (int i = 0; i < 40; ++i)
    {
        const float cx = kWood + rnd.nextFloat() * (w - 2 * kWood), cy = rnd.nextFloat() * tH;
        const float r = 0.6f + rnd.nextFloat() * (rnd.nextFloat() < 0.1f ? 3.5f : 1.2f);
        g.setColour (juce::Colour (0xff3b3026).withAlpha (0.35f + 0.4f * rnd.nextFloat()));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 1.6f);
    }
    for (int i = 0; i < 8; ++i)
    {
        const float cx = kWood + rnd.nextFloat() * (w - 2 * kWood), cy = rnd.nextFloat() * tH, r = 25.0f + rnd.nextFloat() * 60.0f;
        auto col = juce::Colour (0xff8a6a3a).withAlpha (0.08f);
        g.setGradientFill (juce::ColourGradient (col, cx, cy, col.withAlpha (0.0f), cx + r, cy, true));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    }
    // scratches on the dark panel
    for (int i = 0; i < 90; ++i)
    {
        const float x0 = kWood + rnd.nextFloat() * (w - 2 * kWood), y0 = tH + rnd.nextFloat() * (h - tH);
        const float len = 3.0f + rnd.nextFloat() * 22.0f, a = rnd.nextFloat() * juce::MathConstants<float>::pi;
        g.setColour (juce::Colours::white.withAlpha (0.04f + 0.07f * rnd.nextFloat()));
        g.drawLine (x0, y0, x0 + len * std::cos (a), y0 + len * std::sin (a), 0.6f);
    }

    // brushed-aluminium name plate with rivets
    {
        const auto pl = plateArea;
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (pl.translated (2.0f, 3.0f), 4.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffdcdcd8), 0, pl.getY(), juce::Colour (0xff9c9c97), 0, pl.getBottom(), false));
        g.fillRoundedRectangle (pl, 4.0f);
        for (float yy = pl.getY() + 1.0f; yy < pl.getBottom() - 1.0f; yy += 0.5f) // brushing
        {
            g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.04f + 0.05f * rnd.nextFloat()));
            const float x0 = pl.getX() + 2.0f + rnd.nextFloat() * 30.0f, x1 = pl.getRight() - 2.0f - rnd.nextFloat() * 30.0f;
            g.drawLine (x0, yy, x1, yy, 0.5f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawLine (pl.getX() + 3, pl.getY() + 1, pl.getRight() - 3, pl.getY() + 1, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (pl, 4.0f, 1.0f);
        for (auto rv : { juce::Point<float> (pl.getX() + 8, pl.getY() + 8), juce::Point<float> (pl.getRight() - 8, pl.getY() + 8),
                         juce::Point<float> (pl.getX() + 8, pl.getBottom() - 8), juce::Point<float> (pl.getRight() - 8, pl.getBottom() - 8) })
        {
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.fillEllipse (rv.x - 3.0f, rv.y - 2.0f, 6.5f, 6.5f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f4f4), rv.x - 3, rv.y - 3, juce::Colour (0xff6a6a6a), rv.x + 3, rv.y + 3, false));
            g.fillEllipse (rv.x - 3.0f, rv.y - 3.0f, 6.0f, 6.0f);
        }
        // recessed black window for the version number
        const auto inset = juce::Rectangle<float> (pl.getRight() - 74, pl.getY() + 18, 56, 46);
        g.setColour (juce::Colour (0xff0e0e0e));
        g.fillRoundedRectangle (inset, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawLine (inset.getX() + 2, inset.getBottom() + 0.5f, inset.getRight() - 2, inset.getBottom() + 0.5f, 1.0f);
    }

    drawVuFace (g);


    // engraved dividers (row A / row B, EQ / small knobs)
    auto engrave = [&g] (float x0, float y0, float x1, float y1)
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawLine (x0, y0, x1, y1, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawLine (x0 + (x0 == x1 ? 1.0f : 0.0f), y0 + (y0 == y1 ? 1.0f : 0.0f),
                    x1 + (x0 == x1 ? 1.0f : 0.0f), y1 + (y0 == y1 ? 1.0f : 0.0f), 1.0f);
    };
    const float divY = (float) eqArea.getY() - 4.0f;
    engrave (kWood + 20.0f, divY, w - kWood - 20.0f, divY);
    engrave ((float) eqArea.getRight(), (float) eqArea.getY() + 6.0f, (float) eqArea.getRight(), (float) eqArea.getBottom() - 4.0f);

    // recessed EQ plate
    auto eqPlate = eqArea.toFloat().reduced (4.0f, 2.0f);
    g.setColour (juce::Colour (0xff0b0a09).withAlpha (0.7f));
    g.fillRoundedRectangle (eqPlate, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (eqPlate.translated (0, 1), 6.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (eqPlate, 6.0f, 1.0f);

    // highlighted wells behind MODE and REPEAT RATE: recess + chrome bezel + amber glow
    for (int i = 0; i < kNumBig; ++i)
    {
        const auto kb = knobs[(size_t) i].slider.getBounds().toFloat();
        const auto c = kb.getCentre();
        const float rr = kb.getWidth() * 0.5f + 2.0f;
        g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.22f), c.x, c.y, amber.withAlpha (0.0f), c.x + rr + 12.0f, c.y, true));
        g.fillEllipse (c.x - rr - 12.0f, c.y - rr - 12.0f, (rr + 12.0f) * 2, (rr + 12.0f) * 2);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff050505), c.x, c.y - rr,
                                                 juce::Colour (0xff2b2622), c.x, c.y + rr, false));
        g.fillEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8e8e8), c.x - rr, c.y - rr,
                                                 juce::Colour (0xff4c4c4c), c.x + rr, c.y + rr, false));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 3.5f);
        g.setColour (amber.withAlpha (0.55f));
        g.drawEllipse (c.x - rr - 4.0f, c.y - rr - 4.0f, (rr + 4.0f) * 2, (rr + 4.0f) * 2, 1.2f);
    }

    // screws
    auto screw = [&g, &rnd] (float cx, float cy)
    {
        const float r = 5.5f;
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (cx - r + 1, cy - r + 2, r * 2, r * 2);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd0d0d0), cx - r, cy - r,
                                                 juce::Colour (0xff5a5a5a), cx + r, cy + r, false));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
        const float a = rnd.nextFloat() * juce::MathConstants<float>::pi;
        g.setColour (juce::Colour (0xff2a2a2a));
        g.drawLine (cx - r * 0.7f * std::cos (a), cy - r * 0.7f * std::sin (a),
                    cx + r * 0.7f * std::cos (a), cy + r * 0.7f * std::sin (a), 1.5f);
    };
    for (float sy : { 16.0f, tH - 18.0f, h - 16.0f })
    {
        screw (kWood + 16.0f, sy);
        screw (w - kWood - 16.0f, sy);
    }

    // the enamel and the dark panel are separate plates with rounded corners, set in a dark frame
    {
        const float r = 14.0f, gap = 3.0f;
        const auto frame = juce::Rectangle<float> ((float) kWood, 0.0f, w - 2.0f * kWood, h);
        const auto enamelPlate = juce::Rectangle<float> ((float) kWood + gap, gap, w - 2.0f * kWood - 2 * gap, tH - 2 * gap);
        const auto darkPlate   = juce::Rectangle<float> ((float) kWood + gap, tH + gap, w - 2.0f * kWood - 2 * gap, h - tH - 2 * gap);
        juce::Path outside;
        outside.addRectangle (frame);
        outside.addRoundedRectangle (enamelPlate, r);
        outside.addRoundedRectangle (darkPlate, r);
        outside.setUsingNonZeroWinding (false);
        g.setColour (juce::Colour (0xff0e0c0a));
        g.fillPath (outside);
        g.setColour (juce::Colours::white.withAlpha (0.45f));            // bevel: light top edge, dark outline
        g.drawRoundedRectangle (enamelPlate.reduced (1.0f).withTrimmedBottom (2.0f), r - 1.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (enamelPlate, r, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (darkPlate.reduced (1.0f), r - 1.0f, 1.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (darkPlate, r, 1.2f);
    }

    // wooden end cheeks
    {
        juce::Image::BitmapData bd (background, juce::Image::BitmapData::readWrite);
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
                    r *= 0.55f + 0.45f * std::sin (juce::MathConstants<float>::pi * ((float) i + 0.5f) / (float) woodW);
                    r *= 1.0f - 0.15f * std::pow (std::abs ((float) y / (float) H - 0.5f) * 2.0f, 4.0f);
                    bd.setPixelColour (x0 + i, y, juce::Colour::fromFloatRGBA (r * 1.05f, r * 0.62f, r * 0.33f, 1.0f));
                }
        }
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine ((float) kWood, 0.0f, (float) kWood, h, 1.5f);
    g.drawLine (w - (float) kWood, 0.0f, w - (float) kWood, h, 1.5f);

    // rounded corners for the whole unit: mask the outside, then a soft bevel along the edge
    const auto unit = juce::Rectangle<float> (0.0f, 0.0f, w, h);
    const float corner = 24.0f;
    juce::Path outside;
    outside.addRectangle (unit.expanded (2.0f));
    outside.addRoundedRectangle (unit, corner);
    outside.setUsingNonZeroWinding (false);
    g.setColour (juce::Colour (0xff0c0c0c));
    g.fillPath (outside);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (unit.reduced (1.5f), corner - 1.5f, 1.2f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (unit.reduced (0.5f), corner, 1.0f);
}
