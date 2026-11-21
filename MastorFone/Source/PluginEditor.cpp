#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

//==============================================================================
namespace
{
    struct Fonts
    {
        Typeface::Ptr title = Typeface::createSystemTypefaceFor (BinaryData::ArchivoBlackRegular_ttf, (size_t) BinaryData::ArchivoBlackRegular_ttfSize);
        Typeface::Ptr comic = Typeface::createSystemTypefaceFor (BinaryData::ComicReliefBold_ttf, (size_t) BinaryData::ComicReliefBold_ttfSize);
    };
    Fonts& fonts() { static SharedResourcePointer<Fonts> f; return *f; }

    const Colour glassA { 0xfff9dc96 }, glassB { 0xffe7a948 }, tapeBg { 0xff2a1d16 }, tapeBrown { 0xff6b3a1f };
}

Font toon::title (float h) { return Font (FontOptions (fonts().title).withHeight (h)); }
Font toon::comic (float h) { return Font (FontOptions (fonts().comic).withHeight (h)); }

void toon::box (Graphics& g, Rectangle<float> r, Colour fill, float corner, float outline, float shadow)
{
    if (shadow > 0.0f)
    {
        g.setColour (ink);
        g.fillRoundedRectangle (r.translated (shadow, shadow), corner);
    }
    g.setColour (fill);
    g.fillRoundedRectangle (r, corner);
    // brilho de "tinta" no topo
    g.setColour (Colours::white.withAlpha (0.22f));
    g.fillRoundedRectangle (r.reduced (outline + 3.0f).withHeight (jmin (10.0f, r.getHeight() * 0.18f)), corner * 0.5f);
    g.setColour (ink);
    g.drawRoundedRectangle (r, corner, outline);
}

void toon::outlinedText (Graphics& g, const String& text, const Font& font, Rectangle<float> r,
                         Colour fill, Colour outline, float stroke, float shadow, Justification just)
{
    GlyphArrangement ga;
    ga.addFittedText (font, text, r.getX(), r.getY(), r.getWidth(), r.getHeight(), just, 1, 0.8f);
    Path p; ga.createPath (p);
    const PathStrokeType st (stroke, PathStrokeType::curved, PathStrokeType::rounded);
    if (shadow > 0.0f)
    {
        const auto t = AffineTransform::translation (shadow, shadow);
        g.setColour (ink);
        g.strokePath (p, st, t);
        g.fillPath (p, t);
    }
    if (stroke > 0.0f) { g.setColour (outline); g.strokePath (p, st); }
    g.setColour (fill);
    g.fillPath (p);
}

//==============================================================================
ToonLookAndFeel::ToonLookAndFeel()
{
    setColour (TooltipWindow::textColourId, toon::ink);
    setColour (Label::textColourId, toon::ink);
    setColour (ToggleButton::textColourId, toon::ink);
}

void ToonLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float a0, float a1, Slider& s)
{
    const auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = area.getCentre();
    const float R = jmin (area.getWidth(), area.getHeight()) * 0.5f - 9.0f;
    const auto accent = s.findColour (Slider::rotarySliderFillColourId);
    const float ang = a0 + pos * (a1 - a0);

    // bolinhas da escala
    const bool stepped = s.getInterval() >= 1.0;
    const int ticks = stepped ? (int) std::round (s.getMaximum() - s.getMinimum()) + 1 : 11;
    for (int i = 0; i < ticks; ++i)
    {
        const float t = a0 + (float) i / (float) (ticks - 1) * (a1 - a0);
        const auto p = c.getPointOnCircumference (R + 6.0f, t);
        const bool lit = (float) i / (float) (ticks - 1) <= pos + 1e-4f;
        g.setColour (lit ? toon::ink : toon::ink.withAlpha (0.28f));
        const float d = (i == 0 || i == ticks - 1) ? 4.6f : 3.4f;
        g.fillEllipse (Rectangle<float> (d, d).withCentre (p));
    }

    auto circle = [&] (float r, Point<float> o = {}) { return Rectangle<float> (2 * r, 2 * r).withCentre (c + o); };

    // sombra dura
    g.setColour (toon::ink);
    g.fillEllipse (circle (R, { 3.5f, 4.5f }));
    // anel colorido com "dentes" (serrilhado de baquelite)
    Path teeth;
    const int nT = 14;
    for (int i = 0; i < nT * 2; ++i)
    {
        const float t = ang + (float) i / (float) (nT * 2) * MathConstants<float>::twoPi;
        const auto p = c.getPointOnCircumference (i % 2 ? R * 0.93f : R, t);
        if (i == 0) teeth.startNewSubPath (p); else teeth.lineTo (p);
    }
    teeth.closeSubPath();
    teeth = teeth.createPathWithRoundedCorners (3.0f);
    g.setColour (accent);
    g.fillPath (teeth);
    g.setColour (toon::ink);
    g.strokePath (teeth, PathStrokeType (3.2f, PathStrokeType::curved));

    // tampa creme
    const float capR = R * 0.68f;
    g.setColour (toon::paper);
    g.fillEllipse (circle (capR));
    g.setColour (toon::ink.withAlpha (0.12f));
    g.fillEllipse (circle (capR * 0.8f, { capR * 0.12f, capR * 0.14f }));
    g.setColour (toon::paper);
    g.fillEllipse (circle (capR * 0.8f, { -capR * 0.05f, -capR * 0.05f }));
    g.setColour (toon::ink);
    g.drawEllipse (circle (capR), 3.0f);

    // brilho
    g.setColour (Colours::white.withAlpha (0.85f));
    g.fillEllipse (Rectangle<float> (capR * 0.42f, capR * 0.26f).withCentre (c + Point<float> (-capR * 0.38f, -capR * 0.5f)));

    // ponteiro
    const auto p0 = c.getPointOnCircumference (capR * 0.12f, ang), p1 = c.getPointOnCircumference (R * 0.84f, ang);
    g.setColour (toon::ink);
    g.drawLine (Line<float> (p0, p1), 6.0f);
    g.fillEllipse (Rectangle<float> (6.0f, 6.0f).withCentre (p1));
    g.fillEllipse (Rectangle<float> (6.0f, 6.0f).withCentre (p0));

    if (! s.isEnabled())
    {
        g.setColour (toon::cream.withAlpha (0.6f));
        g.fillEllipse (circle (R + 2));
    }
}

void ToonLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool down)
{
    const bool on = b.getToggleState();
    const bool danger = (bool) b.getProperties()["danger"];
    auto r = b.getLocalBounds().toFloat().reduced (2.0f).withTrimmedRight (4.0f).withTrimmedBottom (4.0f);
    const float press = on || down ? 3.0f : 0.0f;
    r = r.translated (press, press);
    toon::box (g, r, on ? (danger ? toon::red : toon::mustard) : (over ? toon::paper : toon::cream),
               r.getHeight() * 0.5f, 3.0f, 4.0f - press);

    // lampadinha
    auto led = Rectangle<float> (11.0f, 11.0f).withCentre ({ r.getX() + 15.0f, r.getCentreY() });
    g.setColour (on ? Colours::white : toon::ink.withAlpha (0.25f));
    g.fillEllipse (led);
    g.setColour (toon::ink);
    g.drawEllipse (led, 2.0f);

    g.setColour (on && danger ? toon::paper : toon::ink);
    g.setFont (toon::comic (14.5f));
    g.drawFittedText (b.getButtonText(), r.withTrimmedLeft (24.0f).withTrimmedRight (6.0f).toNearestInt(), Justification::centred, 1);
}

void ToonLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    // THROW: botão redondo vermelho
    auto r = b.getLocalBounds().toFloat().reduced (2.0f).withTrimmedRight (5.0f).withTrimmedBottom (5.0f);
    const float press = down ? 4.0f : 0.0f;
    r = r.translated (press, press);
    toon::box (g, r, down ? toon::red.darker (0.15f) : (over ? toon::red.brighter (0.12f) : toon::red),
               r.getHeight() * 0.5f, 3.5f, 5.0f - press);
}

void ToonLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (2.0f).withTrimmedRight (5.0f).withTrimmedBottom (5.0f);
    if (down) r = r.translated (4.0f, 4.0f);
    toon::outlinedText (g, b.getButtonText(), toon::title (19.0f), r, toon::paper, toon::ink, 4.0f, 0.0f);
}

void ToonLookAndFeel::drawBubble (Graphics& g, BubbleComponent&, const Point<float>& tip, const Rectangle<float>& body)
{
    Path p;
    p.addRoundedRectangle (body.reduced (1.5f), 9.0f);
    const float cx = jlimit (body.getX() + 12.0f, body.getRight() - 12.0f, tip.x);
    const bool below = tip.y > body.getBottom();
    Path t;
    const float y0 = below ? body.getBottom() - 3.0f : body.getY() + 3.0f;
    t.addTriangle (cx - 7, y0, cx + 7, y0, tip.x, tip.y);
    p.addPath (t);
    g.setColour (toon::paper);
    g.fillPath (p);
    g.setColour (toon::ink);
    g.strokePath (p, PathStrokeType (2.5f, PathStrokeType::curved));
}

//==============================================================================
ModeSelector::ModeSelector (RangedAudioParameter& p)
    : att (p, [this] (float v) { mode = jlimit (0, 2, roundToInt (v)); repaint(); })
{
    att.sendInitialUpdate();
    setMouseCursor (MouseCursor::PointingHandCursor);
}

Rectangle<float> ModeSelector::slot (int i) const
{
    const float gap = 16.0f;
    const float w = ((float) getWidth() - 2.0f * gap - 8.0f) / 3.0f;
    return { (float) i * (w + gap) + 1.0f, 1.0f, w, (float) getHeight() - 9.0f };
}

void ModeSelector::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < 3; ++i)
        if (slot (i).expanded (4.0f).contains (e.position))
            att.setValueAsCompleteGesture ((float) i);
}

void ModeSelector::paint (Graphics& g)
{
    static const char* labels[] = { "R\xc3\x81" "DIO AM", "TELEFONE", "MEGAFONE" };
    for (int i = 0; i < 3; ++i)
    {
        const bool on = i == mode;
        auto r = slot (i);
        if (on) r = r.translated (4.0f, 4.0f);
        toon::box (g, r, on ? toon::mustard : toon::paper, 18.0f, 4.0f, on ? 2.0f : 6.0f);
        auto icon = r.reduced (8.0f).withTrimmedBottom (24.0f);
        drawIcon (g, i, icon, on);
        g.setColour (toon::ink);
        g.setFont (toon::comic (15.0f));
        g.drawFittedText (String::fromUTF8 (labels[i]), r.removeFromBottom (28.0f).toNearestInt(), Justification::centred, 1);
    }
}

void ModeSelector::drawIcon (Graphics& g, int which, Rectangle<float> r, bool on)
{
    const auto c = r.getCentre();
    const float s = jmin (r.getWidth() / 110.0f, r.getHeight() / 62.0f);
    const PathStrokeType thick (3.2f * s + 0.4f, PathStrokeType::curved, PathStrokeType::rounded);
    auto P = [&] (float x, float y) { return Point<float> (c.x + x * s, c.y + y * s); };
    auto R = [&] (float x, float y, float w, float h) { return Rectangle<float> (c.x + x * s, c.y + y * s, w * s, h * s); };
    auto fillStroke = [&] (const Path& p, Colour fill)
    {
        g.setColour (toon::ink); g.fillPath (p, AffineTransform::translation (2.5f * s, 3.0f * s));
        g.setColour (fill); g.fillPath (p);
        g.setColour (toon::ink); g.strokePath (p, thick);
    };

    if (on)
    {
        // risquinhos de movimento
        g.setColour (toon::ink);
        for (int k = 0; k < 3; ++k)
        {
            const float a = -2.4f + (float) k * 0.38f;
            const auto q0 = P (0, 0).getPointOnCircumference (44.0f * s, a), q1 = P (0, 0).getPointOnCircumference (53.0f * s, a);
            g.drawLine ({ q0, q1 }, 3.0f * s);
        }
    }

    if (which == 0)
    {
        // radinho de válvula com antena
        Path ant; ant.startNewSubPath (P (18, -18)); ant.lineTo (P (34, -32));
        g.setColour (toon::ink); g.strokePath (ant, PathStrokeType (3.0f * s, PathStrokeType::curved, PathStrokeType::rounded));
        g.fillEllipse (R (31, -36, 8, 8));
        Path body; body.addRoundedRectangle (R (-36, -18, 72, 42), 12.0f * s);
        fillStroke (body, toon::red);
        Path spk; spk.addEllipse (R (-30, -12, 30, 30));
        fillStroke (spk, toon::paper);
        g.setColour (toon::ink);
        for (int k = 0; k < 3; ++k) g.drawLine (Line<float> (P (-24, -4 + (float) k * 7), P (-6, -4 + (float) k * 7)), 2.2f * s);
        Path dial; dial.addRoundedRectangle (R (6, -11, 24, 13), 3.0f * s);
        fillStroke (dial, Colour (0xfff9dc96));
        g.setColour (toon::red); g.drawLine (Line<float> (P (17, -10), P (17, 1)), 2.0f * s);
        Path k1; k1.addEllipse (R (8, 7, 9, 9)); fillStroke (k1, toon::paper);
        Path k2; k2.addEllipse (R (20, 7, 9, 9)); fillStroke (k2, toon::paper);
    }
    else if (which == 1)
    {
        // telefone de disco
        Path base;
        base.startNewSubPath (P (-30, -4)); base.lineTo (P (30, -4)); base.lineTo (P (40, 26)); base.lineTo (P (-40, 26)); base.closeSubPath();
        base = base.createPathWithRoundedCorners (8.0f * s);
        fillStroke (base, toon::teal);
        Path dial; dial.addEllipse (R (-14, -1, 28, 24));
        fillStroke (dial, toon::paper);
        g.setColour (toon::ink);
        for (int k = 0; k < 7; ++k)
        {
            const auto h = P (0, 11).getPointOnCircumference (8.0f * s, -2.4f + (float) k * 0.75f);
            g.fillEllipse (Rectangle<float> (3.6f * s, 3.6f * s).withCentre (h));
        }
        Path hs;
        hs.addRoundedRectangle (R (-30, -24, 60, 10), 5.0f * s);
        hs.addEllipse (R (-44, -27, 22, 15));
        hs.addEllipse (R (22, -27, 22, 15));
        fillStroke (hs, toon::red);
        Path cord; cord.startNewSubPath (P (-40, 22)); cord.cubicTo (P (-52, 18), P (-50, 0), P (-42, -14));
        g.setColour (toon::ink); g.strokePath (cord, PathStrokeType (2.4f * s, PathStrokeType::curved, PathStrokeType::rounded));
    }
    else
    {
        // megafone
        if (on)
        {
            g.setColour (toon::ink);
            for (int k = 0; k < 2; ++k)
            {
                Path a; const float rr = (12.0f + (float) k * 9.0f) * s;
                a.addCentredArc (c.x + 36.0f * s, c.y, rr, rr, 0.0f, 0.55f, 2.6f, true);
                g.strokePath (a, PathStrokeType (3.0f * s, PathStrokeType::curved, PathStrokeType::rounded));
            }
        }
        Path handle; handle.addRoundedRectangle (R (-12, 4, 10, 20), 3.0f * s);
        fillStroke (handle, toon::paper);
        Path mp; mp.addRoundedRectangle (R (-42, -7, 16, 14), 4.0f * s);
        fillStroke (mp, toon::teal);
        Path cone;
        cone.startNewSubPath (P (-28, -9)); cone.lineTo (P (26, -26)); cone.lineTo (P (26, 26)); cone.lineTo (P (-28, 9)); cone.closeSubPath();
        cone = cone.createPathWithRoundedCorners (4.0f * s);
        fillStroke (cone, toon::red);
        g.setColour (toon::paper.withAlpha (0.8f));
        g.drawLine (Line<float> (P (-20, -6), P (14, -17)), 3.0f * s);
        Path mouth; mouth.addEllipse (R (19, -27, 15, 54));
        fillStroke (mouth, toon::paper);
    }
}

//==============================================================================
RadioPanel::RadioPanel (RadioFoneProcessor& p)
    : proc (p), modeSel (*p.apvts.getParameter ("mode")),
      throwBtn ("THROW", p.apvts.getParameter ("throw"))
{
    setLookAndFeel (&lnf);
    setSize (kW, kH);
    addAndMakeVisible (modeSel);

    setupKnob (tune,   "tune",   "SINTONIA", toon::red);
    setupKnob (band,   "band",   "BANDA",    toon::teal);
    setupKnob (drive,  "drive",  "DRIVE",    toon::mustard);
    setupKnob (noise,  "noise",  "CHIADO",   toon::red);
    setupKnob (defect, "defect", "DEFEITO",  toon::teal);
    setupKnob (mix,    "mix",    "MIX",      toon::mustard);

    setupKnob (echoDiv,   "echoDiv",   "TEMPO",       toon::mustard);
    setupKnob (echoMs,    "echoMs",    "TEMPO",       toon::mustard);
    setupKnob (echoHeads, "echoHeads", String::fromUTF8 ("CABEÇOTES"), toon::teal);
    setupKnob (echoFb,    "echoFb",    "INTENSIDADE", toon::red);
    setupKnob (echoTone,  "echoTone",  "TOM",         toon::teal);
    setupKnob (echoWow,   "echoWow",   "WOW",         toon::mustard);
    setupKnob (echoSend,  "echoSend",  "ENVIO",       toon::red);
    setupKnob (dry,       "dry",       "DRY",         toon::teal);
    setupKnob (out,       "out",       String::fromUTF8 ("SAÍDA"), toon::mustard);

    setupToggle (echoSync, "echoSync", "SYNC");
    setupToggle (echoPP,   "echoPP",   "PING-PONG");
    setupToggle (bypassBtn, "bypass",  "BYPASS");
    bypassBtn.button.getProperties().set ("danger", true);

    addAndMakeVisible (throwBtn);
    throwBtn.setTooltip ("Segure para jogar a voz no eco");

    renderBackground();
    startTimerHz (30);
}

RadioPanel::~RadioPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void RadioPanel::setupKnob (Knob& k, const String& id, const String& name, Colour accent)
{
    auto& s = k.slider;
    s.setSliderStyle (Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (MathConstants<float>::pi * 1.22f, MathConstants<float>::pi * 2.78f, true);
    s.setColour (Slider::rotarySliderFillColourId, accent);
    s.setPopupDisplayEnabled (true, true, this, 1500);
    s.setMouseCursor (MouseCursor::UpDownResizeCursor);
    addAndMakeVisible (s);
    k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
    if (auto* prm = proc.apvts.getParameter (id))
        s.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    k.accent = accent;
    k.label.setText (name, dontSendNotification);
    k.label.setJustificationType (Justification::centred);
    k.label.setInterceptsMouseClicks (false, false);
    k.label.setFont (toon::comic (15.0f));
    addAndMakeVisible (k.label);
}

void RadioPanel::setupToggle (Toggle& t, const String& id, const String& text)
{
    t.button.setButtonText (text);
    t.button.setClickingTogglesState (true);
    addAndMakeVisible (t.button);
    t.att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, t.button);
}

void RadioPanel::placeKnob (Knob& k, Point<int> c, int size)
{
    k.slider.setBounds (Rectangle<int> (size, size).withCentre (c));
    k.label.setBounds (c.x - 58, c.y + size / 2 - 2, 116, 20);
}

void RadioPanel::resized()
{
    // --- APARELHO (painel da esquerda)
    dialWin = { 65.0f, 130.0f, 465.0f, 72.0f };
    modeSel.setBounds (65, 216, 473, 116);
    {
        const int y = 383, size = 74;
        Knob* ks[] = { &tune, &band, &drive, &noise, &defect, &mix };
        for (int i = 0; i < 6; ++i)
            placeKnob (*ks[i], { 65 + 39 + i * 77, y }, size);
    }
    lampArea = { 84.0f, 462.0f, 96.0f, 96.0f };
    vuArea   = { 222.0f, 462.0f, 160.0f, 96.0f };

    // --- ECO DE FITA (painel da direita)
    tapeWin = { 592.0f, 130.0f, 311.0f, 96.0f };
    const int cx[] = { 634, 748, 862 };
    placeKnob (echoDiv,   { cx[0], 274 }, 72);
    placeKnob (echoMs,    { cx[0], 274 }, 72);
    placeKnob (echoHeads, { cx[1], 274 }, 72);
    placeKnob (echoFb,    { cx[2], 274 }, 72);
    placeKnob (echoTone,  { cx[0], 370 }, 72);
    placeKnob (echoWow,   { cx[1], 370 }, 72);
    placeKnob (echoSend,  { cx[2], 370 }, 72);
    echoSync.button.setBounds (592, 432, 96, 38);
    echoPP.button.setBounds (696, 432, 116, 38);
    throwBtn.setBounds (818, 426, 92, 50);
    placeKnob (dry, { cx[0], 518 }, 62);
    placeKnob (out, { cx[1], 518 }, 62);
    bypassBtn.button.setBounds (812, 500, 104, 40);
}

//==============================================================================
void RadioPanel::renderBackground()
{
    background = Image (Image::ARGB, kW, kH, true);
    Graphics g (background);

    // parede com bolinhas
    g.fillAll (toon::wall);
    g.setColour (toon::ink.withAlpha (0.16f));
    for (int row = 0; row * 22 < kH + 22; ++row)
        for (int col = 0; col * 22 < kW + 22; ++col)
            g.fillEllipse ((float) (col * 22 + (row % 2) * 11), (float) (row * 22), 5.0f, 5.0f);

    // gabinete de madeira em forma de "capelinha"
    Path cab;
    cab.startNewSubPath (20, 132);
    cab.quadraticTo (20, 72, 80, 72);
    cab.lineTo (250, 72);
    cab.cubicTo (272, 2, 688, 2, 710, 72);
    cab.lineTo (880, 72);
    cab.quadraticTo (940, 72, 940, 132);
    cab.lineTo (940, 560);
    cab.quadraticTo (940, 600, 900, 600);
    cab.lineTo (60, 600);
    cab.quadraticTo (20, 600, 20, 560);
    cab.closeSubPath();

    g.setColour (toon::ink);
    g.fillPath (cab, AffineTransform::translation (10, 10));
    g.setGradientFill (ColourGradient (toon::wood.brighter (0.12f), 0, 20, toon::wood.darker (0.15f), 0, 600, false));
    g.fillPath (cab);
    {
        // veios da madeira
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (cab);
        Random rng (7);
        g.setColour (toon::woodDk.withAlpha (0.45f));
        for (int i = 0; i < 26; ++i)
        {
            const float y = 20.0f + (float) i * 23.0f + rng.nextFloat() * 8.0f;
            Path grain; grain.startNewSubPath (0, y);
            for (float x = 0; x <= kW; x += 60)
                grain.quadraticTo (x + 30, y + (rng.nextFloat() - 0.5f) * 14.0f, x + 60, y + (rng.nextFloat() - 0.5f) * 6.0f);
            g.strokePath (grain, PathStrokeType (i % 3 == 0 ? 2.6f : 1.6f));
        }
        for (auto k : { Point<float> (140, 590), Point<float> (900, 300), Point<float> (560, 90) })
        {
            g.drawEllipse (Rectangle<float> (34, 14).withCentre (k), 2.0f);
            g.drawEllipse (Rectangle<float> (18, 7).withCentre (k), 1.6f);
        }
        // brilho do verniz
        g.setColour (Colours::white.withAlpha (0.10f));
        g.fillRoundedRectangle (40, 80, 22, 500, 11);
    }
    g.setColour (toon::ink);
    g.strokePath (cab, PathStrokeType (6.0f, PathStrokeType::curved));

    // título
    toon::outlinedText (g, "MASTOR FONE", toon::title (50.0f), { 262, 22, 436, 52 },
                        toon::mustard, toon::ink, 7.0f, 5.0f);
    toon::outlinedText (g, "Dub Station Solutions", toon::comic (20.0f), { 262, 73, 436, 25 },
                        toon::paper, toon::ink, 3.5f, 0.0f);

    // painéis creme
    toon::box (g, { 40, 104, 515, 474 }, toon::cream, 26, 4.5f, 7);
    toon::box (g, { 575, 104, 345, 474 }, toon::cream, 26, 4.5f, 7);

    // bandeirinhas
    auto tab = [&] (Rectangle<float> r, const String& t, Colour c)
    {
        toon::box (g, r, c, 12, 3.5f, 4);
        toon::outlinedText (g, t, toon::title (17.0f), r.reduced (8, 2), toon::paper, toon::ink, 4.0f, 0.0f);
    };
    tab ({ 58, 92, 150, 30 }, "APARELHO", toon::red);
    tab ({ 593, 92, 172, 30 }, "ECO DE FITA", toon::teal);

    // janela do dial (ondas médias)
    toon::box (g, dialWin, glassA, 14, 4.0f, 0);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (dialWin.reduced (3).toNearestInt());
        g.setGradientFill (ColourGradient (glassA, dialWin.getX(), dialWin.getY(), glassB, dialWin.getX(), dialWin.getBottom(), false));
        g.fillRect (dialWin);
        g.setColour (toon::ink);
        const int freqs[] = { 540, 600, 700, 800, 900, 1000, 1200, 1400, 1600 };
        const float x0 = dialWin.getX() + 26, x1 = dialWin.getRight() - 46;
        for (int i = 0; i <= 40; ++i)
        {
            const float x = x0 + (x1 - x0) * (float) i / 40.0f;
            const bool big = i % 5 == 0;
            g.fillRect (x - (big ? 1.5f : 1.0f), dialWin.getY() + 10, big ? 3.0f : 2.0f, big ? 16.0f : 9.0f);
        }
        g.setFont (toon::comic (14.0f));
        for (int i = 0; i < 9; ++i)
        {
            const float x = x0 + (x1 - x0) * (float) i / 8.0f;
            g.drawText (String (freqs[i]), Rectangle<float> (50, 18).withCentre ({ x, dialWin.getY() + 38 }), Justification::centred);
        }
        g.setFont (toon::comic (12.0f));
        g.drawText ("kHz", Rectangle<float> (dialWin.getRight() - 44, dialWin.getY() + 29, 38, 18), Justification::centred);
        g.drawText (String::fromUTF8 ("ONDAS MÉDIAS"), Rectangle<float> (dialWin.getX(), dialWin.getBottom() - 22, dialWin.getWidth(), 16), Justification::centred);
        // reflexo do vidro
        g.setColour (Colours::white.withAlpha (0.35f));
        g.fillRoundedRectangle (dialWin.getX() + 10, dialWin.getY() + 5, dialWin.getWidth() * 0.5f, 4, 2);
    }
    g.setColour (toon::ink);
    g.drawRoundedRectangle (dialWin, 14, 4.0f);

    // grade do alto-falante (ripas art déco)
    const Rectangle<float> grille { 65, 452, 465, 114 };
    toon::box (g, grille, Colour (0xff4a2a18), 18, 4.0f, 0);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (grille.reduced (3).toNearestInt());
        for (int i = 0; i < 13; ++i)
        {
            const auto slat = Rectangle<float> (grille.getX() + 14.0f + (float) i * 35.5f, grille.getY() + 10, 16, grille.getHeight() - 20);
            g.setColour (Colour (0xffd79a52));
            g.fillRoundedRectangle (slat, 8);
            g.setColour (Colours::white.withAlpha (0.3f));
            g.fillRoundedRectangle (slat.withWidth (5).translated (3, 6).withTrimmedBottom (12), 2.5f);
            g.setColour (toon::ink);
            g.drawRoundedRectangle (slat, 8, 2.6f);
        }
    }
    g.setColour (toon::ink);
    g.drawRoundedRectangle (grille, 18, 4.0f);

    // janela da fita
    toon::box (g, tapeWin, tapeBg, 16, 4.0f, 0);

    // moldura do VU e da lâmpada
    toon::box (g, vuArea, toon::paper, 16, 4.0f, 5);
    toon::box (g, lampArea, toon::cream, 16, 4.0f, 5);
}

void RadioPanel::paint (Graphics& g)
{
    g.drawImageAt (background, 0, 0);
    drawDialNeedle (g);
    drawTape (g);
    drawVu (g);
    drawLamp (g);
}

void RadioPanel::drawDialNeedle (Graphics& g)
{
    const float t = (float) (tune.slider.getValue() + 100.0) / 200.0f;
    const float x0 = dialWin.getX() + 26, x1 = dialWin.getRight() - 46;
    const float x = x0 + (x1 - x0) * jlimit (0.0f, 1.0f, t);
    const Line<float> l { x, dialWin.getY() + 5, x, dialWin.getBottom() - 5 };
    g.setColour (toon::ink);
    g.drawLine (l, 7.5f);
    g.setColour (toon::red);
    g.drawLine (l, 3.5f);
    g.setColour (toon::ink);
    g.fillEllipse (Rectangle<float> (13, 13).withCentre ({ x, dialWin.getY() + 8 }));
    g.setColour (toon::red);
    g.fillEllipse (Rectangle<float> (8, 8).withCentre ({ x, dialWin.getY() + 8 }));
}

void RadioPanel::drawTape (Graphics& g)
{
    const auto r = tapeWin.reduced (6);
    const Point<float> cl { r.getX() + 52, r.getCentreY() + 4 }, cr { r.getRight() - 52, r.getCentreY() + 4 };
    const float R = 38.0f;

    // fita passando pelos cabeçotes
    Path tape;
    tape.startNewSubPath (cl.x, cl.y + R * 0.78f);
    tape.lineTo (r.getCentreX() - 22, r.getBottom() - 6);
    tape.lineTo (r.getCentreX() + 22, r.getBottom() - 6);
    tape.lineTo (cr.x, cr.y + R * 0.78f);
    g.setColour (tapeBrown);
    g.strokePath (tape, PathStrokeType (4.0f, PathStrokeType::curved));
    const int heads = (int) echoHeads.slider.getValue();
    static const int headMasks[] = { 1, 2, 4, 3, 6, 5, 7 };
    for (int h = 0; h < 3; ++h)
    {
        const bool on = headMasks[jlimit (0, 6, heads)] & (1 << h);
        auto hr = Rectangle<float> (12, 12).withCentre ({ r.getCentreX() - 18 + (float) h * 18, r.getBottom() - 10 });
        g.setColour (on ? toon::mustard : Colour (0xff8a8178));
        g.fillRoundedRectangle (hr, 3);
        g.setColour (toon::ink);
        g.drawRoundedRectangle (hr, 3, 2.0f);
    }

    auto reel = [&] (Point<float> c, float ang)
    {
        // rolo de fita (marrom) + carretel creme vazado
        g.setColour (tapeBrown);
        g.fillEllipse (Rectangle<float> (R * 1.55f, R * 1.55f).withCentre (c));
        Path disc; disc.addEllipse (Rectangle<float> (2 * R, 2 * R).withCentre (c));
        for (int k = 0; k < 3; ++k)
        {
            const float a = ang + (float) k * MathConstants<float>::twoPi / 3.0f;
            Path hole; hole.addEllipse (Rectangle<float> (R * 0.62f, R * 0.62f).withCentre (c.getPointOnCircumference (R * 0.52f, a)));
            disc.addPath (hole);
        }
        disc.setUsingNonZeroWinding (false);
        g.setColour (toon::paper);
        g.fillPath (disc);
        g.setColour (toon::ink);
        g.strokePath (disc, PathStrokeType (3.0f));
        g.fillEllipse (Rectangle<float> (12, 12).withCentre (c));
        g.setColour (Colours::white.withAlpha (0.7f));
        g.fillEllipse (Rectangle<float> (R * 0.4f, R * 0.22f).withCentre (c + Point<float> (-R * 0.42f, -R * 0.6f)));
    };
    reel (cl, reelAngle);
    reel (cr, reelAngle * 0.83f + 0.6f);

    // leitura do tempo
    String t;
    if (echoSync.button.getToggleState())
        t = RadioFoneProcessor::divisionNames()[jlimit (0, 6, (int) echoDiv.slider.getValue())]
            + String::fromUTF8 ("  ·  ") + String (roundToInt (proc.getDisplayBpm())) + " BPM";
    else
        t = String (roundToInt (echoMs.slider.getValue())) + " ms";
    const auto txt = Rectangle<float> (cl.x + R + 2, r.getY() + 4, cr.x - cl.x - 2 * R - 4, 22);
    toon::outlinedText (g, t, toon::comic (15.0f), txt, toon::paper, toon::ink, 0.0f, 0.0f);
    static const char* hn[] = { "1", "2", "3", "1+2", "2+3", "1+3", "1+2+3" };
    toon::outlinedText (g, String::fromUTF8 ("CAB. ") + hn[jlimit (0, 6, heads)], toon::comic (13.0f),
                        txt.translated (0, 22), toon::mustard, toon::ink, 0.0f, 0.0f);
}

void RadioPanel::drawVu (Graphics& g)
{
    auto r = vuArea.reduced (8);
    const Point<float> pivot { r.getCentreX(), r.getBottom() + 4 };
    const float R = r.getHeight() - 6;
    const float a0 = -1.05f, a1 = 1.05f;   // ângulos a partir do "meio-dia"
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (vuArea.reduced (4).toNearestInt());

    // zona vermelha
    Path red; red.addCentredArc (pivot.x, pivot.y, R, R, 0.0f, a0 + (a1 - a0) * 0.75f, a1, true);
    g.setColour (toon::red);
    g.strokePath (red, PathStrokeType (8.0f, PathStrokeType::mitered, PathStrokeType::butt));
    Path arc; arc.addCentredArc (pivot.x, pivot.y, R, R, 0.0f, a0, a1, true);
    g.setColour (toon::ink);
    g.strokePath (arc, PathStrokeType (2.5f));
    for (int i = 0; i <= 8; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 8.0f;
        auto pt = [&] (float rr) { return Point<float> (pivot.x + rr * std::sin (a), pivot.y - rr * std::cos (a)); };
        g.drawLine (Line<float> (pt (R - (i % 2 ? 5.0f : 10.0f)), pt (R + 4)), i % 2 ? 2.0f : 3.0f);
    }
    g.setFont (toon::title (14.0f));
    g.drawText ("VU", Rectangle<float> (40, 18).withCentre ({ pivot.x, pivot.y - R * 0.42f }), Justification::centred);
    g.setFont (toon::comic (11.0f));
    g.drawText ("-", Rectangle<float> (14, 14).withCentre ({ r.getX() + 8, r.getY() + 8 }), Justification::centred);
    g.setColour (toon::red);
    g.drawText ("+", Rectangle<float> (14, 14).withCentre ({ r.getRight() - 8, r.getY() + 8 }), Justification::centred);

    // ponteiro com balanço
    const float a = a0 + (a1 - a0) * vuPos + 0.03f * std::sin (wiggle);
    const Point<float> tip { pivot.x + (R + 2) * std::sin (a), pivot.y - (R + 2) * std::cos (a) };
    g.setColour (toon::ink);
    g.drawLine (Line<float> (pivot, tip), 3.5f);
    g.fillEllipse (Rectangle<float> (18, 18).withCentre (pivot));
}

void RadioPanel::drawLamp (Graphics& g)
{
    const auto c = Point<float> (lampArea.getCentreX(), lampArea.getY() + 36);
    if (lampGlow > 0.02f)
    {
        g.setGradientFill (ColourGradient (toon::red.withAlpha (0.55f * lampGlow), c.x, c.y,
                                           toon::red.withAlpha (0.0f), c.x + 34, c.y, true));
        g.fillEllipse (Rectangle<float> (68, 68).withCentre (c));
    }
    auto bulb = Rectangle<float> (34, 34).withCentre (c);
    g.setColour (toon::ink);
    g.fillEllipse (bulb.translated (2.5f, 3.0f));
    g.setColour (toon::red.darker (0.75f).interpolatedWith (toon::red.brighter (0.25f), lampGlow));
    g.fillEllipse (bulb);
    g.setColour (Colours::white.withAlpha (0.35f + 0.5f * lampGlow));
    g.fillEllipse (Rectangle<float> (11, 7).withCentre (c + Point<float> (-6, -8)));
    g.setColour (toon::ink);
    g.drawEllipse (bulb, 3.0f);
    toon::outlinedText (g, "NO AR", toon::title (14.0f), { lampArea.getX(), lampArea.getBottom() - 32, lampArea.getWidth(), 22 },
                        lampGlow > 0.5f ? toon::red : toon::paper, toon::ink, 3.5f, 0.0f);
}

void RadioPanel::timerCallback()
{
    const bool sync = echoSync.button.getToggleState();
    echoDiv.slider.setVisible (sync);
    echoMs.slider.setVisible (! sync);

    const float lvl = proc.getOutputLevel();
    const float db = 20.0f * std::log10 (lvl + 1e-9f) + 18.0f;     // 0 VU = -18 dBFS
    const float target = jlimit (0.0f, 1.0f, (db + 20.0f) / 23.0f);
    vuPos += (target > vuPos ? 0.35f : 0.12f) * (target - vuPos);
    wiggle += 0.9f;
    lampGlow += ((lvl > 0.003f ? 1.0f : 0.0f) - lampGlow) * 0.3f;

    const bool echoOn = proc.getEchoLevel() > 0.0005f || echoSend.slider.getValue() > 0.5 || throwBtn.isDown();
    const float speed = echoOn ? 0.09f * jlimit (0.4f, 3.0f, 0.4f / jmax (0.04f, proc.getEchoDelaySec())) : 0.0f;
    reelAngle += speed;
    if (reelAngle > 1000.0f) reelAngle -= 1000.0f;

    repaint (dialWin.expanded (8).toNearestInt());
    repaint (tapeWin.expanded (2).toNearestInt());
    repaint (vuArea.toNearestInt());
    repaint (lampArea.expanded (10).toNearestInt());
}
