#include "MrmEditor.h"

namespace mrmui
{
namespace col
{
    const juce::Colour plate     { 0xffe4d8bc };
    const juce::Colour plateDark { 0xffc9b994 };
    const juce::Colour ink       { 0xff2b241d };
    const juce::Colour inkSoft   { 0xff5c5144 };
    const juce::Colour labelBg   { 0xff1a1714 };
    const juce::Colour cream     { 0xffefe5cc };
    const juce::Colour jamGreen  { 0xff0b8a3c };
    const juce::Colour jamGold   { 0xffeabd12 };
    const juce::Colour amber     { 0xffffa928 };
    const juce::Colour glass     { 0xff17110a };
    const juce::Colour woodDark  { 0xff2e1a0d };
    const juce::Colour woodLight { 0xff6b4423 };
}

static juce::Font stencil (float size, float kerning = 0.08f)
{
    return juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (kerning);
}

static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

static void lamp (juce::Graphics& g, juce::Rectangle<float> r, bool on)
{
    if (on)
    {
        g.setColour (col::amber.withAlpha (0.32f));
        g.fillEllipse (r.expanded (5.0f));
        juce::ColourGradient lit (juce::Colour (0xffffe3a0), r.getCentreX(), r.getCentreY(),
                                  col::amber.darker (0.2f), r.getRight(), r.getBottom(), true);
        g.setGradientFill (lit);
    }
    else
        g.setColour (juce::Colour (0xff4a3a22));
    g.fillEllipse (r);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (r, 1.0f);
}

//==============================================================================
void TabKey::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const bool over = isMouseOver() && ! selected;

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (0, 3.0f), 4.0f);
    auto cap = r.translated (0, selected ? 2.5f : 0.0f);
    juce::ColourGradient cg (juce::Colour (selected ? 0xff2c2925 : 0xff3a3632).brighter (over ? 0.09f : 0.0f),
                             cap.getX(), cap.getY(), juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (selected ? 0.04f : 0.12f));
    g.drawHorizontalLine ((int) cap.getY() + 1, cap.getX() + 4, cap.getRight() - 4);
    if (selected)
    {   // tecla afundada: friso âmbar embaixo
        g.setColour (col::amber.withAlpha (0.75f));
        g.fillRoundedRectangle (cap.removeFromBottom (3.0f).reduced (10.0f, 0.0f).translated (0, -2.0f), 1.5f);
        cap = r.translated (0, 2.5f);
    }

    lamp (g, juce::Rectangle<float> (11.0f, 11.0f).withCentre ({ cap.getX() + 20.0f, cap.getCentreY() }), selected);

    auto txt = cap.withTrimmedLeft (34.0f).withTrimmedRight (8.0f);
    g.setColour (selected ? col::amber : col::cream.withAlpha (0.62f));
    g.setFont (stencil (16.0f, 0.18f));
    g.drawText (title, txt.withTrimmedBottom (txt.getHeight() * 0.42f), juce::Justification::centredBottom);
    g.setColour ((selected ? col::cream : col::cream.withAlpha (0.42f)));
    g.setFont (stencil (9.5f, 0.28f));
    g.drawText (sub, txt.withTrimmedTop (txt.getHeight() * 0.6f), juce::Justification::centredTop);
}

//==============================================================================
void PlayKey::paintButton (juce::Graphics& g, bool over, bool down)
{
    const bool on = getToggleState();
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (r, 6.0f);
    auto cap = r.reduced (4.0f).translated (0, (down || on) ? 1.5f : 0.0f);
    const juce::Colour base = on ? col::jamGold.darker (0.15f) : col::jamGreen;
    juce::ColourGradient cg (base.brighter (over ? 0.12f : 0.0f), cap.getX(), cap.getY(), base.darker (0.75f), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.12f : 0.22f));
    g.fillRoundedRectangle (cap.withHeight (cap.getHeight() * 0.38f).reduced (4, 2), 3.0f);

    lamp (g, juce::Rectangle<float> (11.0f, 11.0f).withCentre ({ cap.getX() + 17.0f, cap.getCentreY() }), on);

    auto txt = cap.withTrimmedLeft (30.0f);
    const auto ic = juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ txt.getX() + 7.0f, cap.getCentreY() });
    juce::Path sym;
    if (on) sym.addRectangle (ic.reduced (1.5f));
    else    sym.addTriangle (ic.getX() + 1.0f, ic.getY(), ic.getX() + 1.0f, ic.getBottom(), ic.getRight(), ic.getCentreY());
    const juce::Colour ink = on ? juce::Colour (0xff2e2108) : col::jamGold;
    g.setColour (ink);
    g.fillPath (sym);
    g.setFont (stencil (12.5f, 0.06f));
    g.drawText (on ? "PARAR" : "PLAY RIDDIM", txt.withTrimmedLeft (16.0f), juce::Justification::centred);
}

//==============================================================================
BpmWindow::BpmWindow (MrmProcessor& p)
    : proc (p), param (*p.apvts.getParameter ("bpm")),
      att (param, [this] (float v) { value = v; repaint(); }, nullptr)
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void BpmWindow::refresh()
{
    const bool daw = proc.getPlayBeat() >= 0.0 && ! proc.isMasterPlaying();
    const double b = proc.getChords().getExportBpm();
    if (daw != dawShown || (daw && std::abs (b - dawBpm) > 0.01))
    {
        dawShown = daw;
        dawBpm = b;
        repaint();
    }
}

void BpmWindow::mouseDown (const juce::MouseEvent&)      { dragStart = value; att.beginGesture(); }
void BpmWindow::mouseUp (const juce::MouseEvent&)        { att.endGesture(); }
void BpmWindow::mouseDrag (const juce::MouseEvent& e)
{
    att.setValueAsPartOfGesture (juce::jlimit (60.0f, 180.0f, std::round (dragStart - (float) e.getDistanceFromDragStartY() * 0.35f)));
}
void BpmWindow::mouseDoubleClick (const juce::MouseEvent&) { att.setValueAsCompleteGesture (75.0f); }
void BpmWindow::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const int step = w.deltaY > 0 ? 1 : w.deltaY < 0 ? -1 : 0;
    if (step != 0) att.setValueAsCompleteGesture (juce::jlimit (60.0f, 180.0f, value + (float) step));
}

void BpmWindow::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (b.translated (0, 2.0f), 5.0f);
    juce::ColourGradient frame (juce::Colour (0xff3a3632), b.getX(), b.getY(), juce::Colour (0xff141210), b.getX(), b.getBottom(), false);
    g.setGradientFill (frame);
    g.fillRoundedRectangle (b, 5.0f);

    auto r = b.reduced (7.0f, 6.0f);
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r.expanded (2), 4.0f);
    juce::ColourGradient gl (col::glass.brighter (0.15f), r.getX(), r.getY(), col::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r, 3.0f);

    const juce::String num (juce::roundToInt (dawShown ? dawBpm : (double) value));
    auto numArea = r.withTrimmedRight (r.getWidth() * 0.42f);
    auto unitArea = r.withTrimmedLeft (r.getWidth() * 0.58f);
    g.setFont (stencil (22.0f, 0.06f));
    g.setColour (col::amber.withAlpha (0.18f));
    for (auto o : { juce::Point<float> (-1, 0), { 1, 0 }, { 0, -1 }, { 0, 1 } })
        g.drawText (num, numArea.translated (o.x, o.y), juce::Justification::centredRight);
    g.setColour (col::amber);
    g.drawText (num, numArea, juce::Justification::centredRight);
    g.setFont (stencil (9.0f, 0.2f));
    g.setColour (col::amber.withAlpha (0.75f));
    g.drawText ("BPM", unitArea.withTrimmedBottom (unitArea.getHeight() * 0.5f).withTrimmedLeft (6), juce::Justification::centredLeft);
    g.setColour (col::amber.withAlpha (0.5f));
    g.drawText (dawShown ? "DAW" : "LINK", unitArea.withTrimmedTop (unitArea.getHeight() * 0.5f).withTrimmedLeft (6), juce::Justification::centredLeft);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f), 3.0f);
}

//==============================================================================
void RiddimDragZone::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r, 5.0f);
    juce::ColourGradient gl (col::glass.brighter (isMouseOver() ? 0.30f : 0.15f), r.getX(), r.getY(),
                             col::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r.reduced (3.0f), 4.0f);

    juce::Path outline, dashed;
    outline.addRoundedRectangle (r.reduced (6.0f), 3.0f);
    const float dashes[] = { 5.0f, 4.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, outline, dashes, 2);
    g.setColour (col::amber.withAlpha (isMouseOver() ? 0.95f : 0.6f));
    g.fillPath (dashed);

    g.setColour (col::amber);
    g.setFont (stencil (12.5f, 0.14f));
    g.drawText ("ARRASTE O RIDDIM", r.withTrimmedBottom (r.getHeight() * 0.44f).withTrimmedTop (3), juce::Justification::centredBottom);
    g.setColour (col::amber.withAlpha (0.6f));
    g.setFont (stencil (8.5f, 0.2f));
    g.drawText ("BATERIA + ACORDES, JUNTOS",
                r.withTrimmedTop (r.getHeight() * 0.58f), juce::Justification::centredTop);
}

void RiddimDragZone::mouseDrag (const juce::MouseEvent&)
{
    if (dragging) return;
    dragging = true;
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("MRM-0420");
    dir.createDirectory();
    auto file = dir.getChildFile (proc.suggestedRiddimName());
    if (proc.writeRiddimFile (file))
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (file.getFullPathName()), false, this);
}

//==============================================================================
Header::Header (MrmProcessor& p, std::function<void (int)> onTab)
    : proc (p), bpm (p), dragZone (p)
{
    setOpaque (true);
    harmony.onClick  = [onTab] { onTab (0); };
    drumsTab.onClick = [onTab] { onTab (1); };
    addAndMakeVisible (harmony);
    addAndMakeVisible (drumsTab);

    addAndMakeVisible (play);
    playAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "play", play);
    addAndMakeVisible (bpm);

    length = std::make_unique<ChoiceWindow> (*proc.apvts.getParameter ("riddimBars"), [] (int i) -> std::pair<juce::String, juce::String>
    {
        return { juce::String (MrmProcessor::lengthBars (i)), "COMP." };
    });
    addAndMakeVisible (*length);
    addAndMakeVisible (dragZone);

    exportBtn.setLookAndFeel (&lnf);
    exportBtn.onClick = [this] { exportRiddim(); };
    addAndMakeVisible (exportBtn);

    setSize (MrmRoot::baseW, height);
    startTimerHz (15);
}

Header::~Header()
{
    stopTimer();
    exportBtn.setLookAndFeel (nullptr);
}

void Header::timerCallback()
{
    bpm.refresh();
    dragZone.repaint();
}

void Header::exportRiddim()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("MRM-0420");
    dir.createDirectory();
    chooser = std::make_unique<juce::FileChooser> ("Salvar o riddim em MIDI", dir.getChildFile (proc.suggestedRiddimName()), "*.mid");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              if (! f.hasFileExtension ("mid")) f = f.withFileExtension ("mid");
                              if (! proc.writeRiddimFile (f))
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "MRM-0420",
                                                                          u8 ("N\xc3\xa3o foi poss\xc3\xadvel salvar o MIDI."));
                          });
}

void Header::resized()
{
    const int W = getWidth(), H = getHeight();
    woodL = { 0, 0, 28, H };
    woodR = { W - 28, 0, 28, H };
    plate = { 28, 0, W - 56, H };

    tabSection    = { 52, 92, 372, 66 };
    riddimSection = { tabSection.getRight() + 20, 92, W - 52 - (tabSection.getRight() + 20), 66 };

    {
        auto a = tabSection.reduced (12, 0).withTrimmedTop (13).withHeight (46);
        harmony.setBounds (a.removeFromLeft ((a.getWidth() - 8) / 2));
        a.removeFromLeft (8);
        drumsTab.setBounds (a);
    }
    {
        auto a = riddimSection.reduced (12, 0).withTrimmedTop (13).withHeight (46);
        const int gap = 9;
        play.setBounds (a.removeFromLeft (150));        a.removeFromLeft (gap);
        bpm.setBounds (a.removeFromLeft (108));         a.removeFromLeft (gap);
        length->setBounds (a.removeFromLeft (170));     a.removeFromLeft (gap);
        exportBtn.setBounds (a.removeFromRight (116));  a.removeFromRight (gap);
        dragZone.setBounds (a);
    }
    renderBackground();
}

void Header::paint (juce::Graphics& g)
{
    g.fillAll (col::woodDark);
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());
}

void Header::renderBackground()
{
    if (getWidth() <= 0) return;
    const float scale = 2.0f;
    background = juce::Image (juce::Image::ARGB, (int) (getWidth() * scale), (int) (getHeight() * scale), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    juce::Random rng (420);

    // --- laterais de madeira (continuam as do painel de baixo)
    for (auto wood : { woodL, woodR })
    {
        const float wx = (float) wood.getX(), ww = (float) wood.getWidth();
        juce::ColourGradient wg (col::woodLight.darker (0.1f), wx, 0, col::woodDark, wx + ww, 0, false);
        wg.addColour (0.35, col::woodLight.darker (0.25f));
        wg.addColour (0.7,  col::woodLight.darker (0.45f));
        g.setGradientFill (wg);
        g.fillRect (wood);
        for (int i = 0; i < 40; ++i)
        {
            juce::Path grain;
            const float xx = wx + rng.nextFloat() * ww, ph = rng.nextFloat() * 6.0f, amp = 0.8f + rng.nextFloat() * 2.2f;
            grain.startNewSubPath (xx, 0);
            for (int yy = 0; yy <= getHeight(); yy += 14)
                grain.lineTo (xx + std::sin ((float) yy * 0.011f + ph) * amp, (float) yy);
            g.setColour ((i % 3 ? juce::Colours::black : col::woodLight.brighter (0.3f)).withAlpha (0.10f + rng.nextFloat() * 0.16f));
            g.strokePath (grain, juce::PathStrokeType (0.5f + rng.nextFloat() * 1.5f));
        }
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.10f), wx, 0, juce::Colours::transparentWhite, wx + 4.0f, 0, false);
        g.setGradientFill (edge);
        g.fillRect (wood.withWidth (4));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRect (wood, 1);
    }

    // --- painel de metal creme envelhecido
    juce::ColourGradient pg (col::plate.brighter (0.03f), (float) plate.getX(), (float) plate.getY(),
                             col::plateDark.darker (0.02f), (float) plate.getRight(), (float) plate.getBottom() + 400.0f, false);
    pg.addColour (0.5, col::plate.interpolatedWith (juce::Colour (0xffd9c48f), 0.35f));
    g.setGradientFill (pg);
    g.fillRect (plate);

    auto randIn = [&] (juce::Rectangle<int> r)
    { return juce::Point<float> ((float) r.getX() + rng.nextFloat() * (float) r.getWidth(), (float) r.getY() + rng.nextFloat() * (float) r.getHeight()); };

    for (int i = 0; i < 8; ++i)
    {
        const float rr = 60.0f + rng.nextFloat() * 200.0f;
        const auto pc = randIn (plate);
        juce::ColourGradient y (juce::Colour (0xffc9a64e).withAlpha (0.07f + rng.nextFloat() * 0.05f), pc.x, pc.y,
                                juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
        g.setGradientFill (y);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 1.4f).withCentre (pc));
    }
    for (int i = 0; i < 12500; ++i)
    {
        const auto pt = randIn (plate);
        g.setColour ((rng.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.03f + rng.nextFloat() * 0.07f));
        g.fillRect (pt.x, pt.y, 1.0f, 1.0f);
    }
    for (int i = 0; i < 9; ++i)
    {
        const float rr = 10.0f + rng.nextFloat() * 70.0f;
        const auto pc = randIn (plate);
        juce::ColourGradient stain (juce::Colour (0xff7a5a2c).withAlpha (0.05f + rng.nextFloat() * 0.06f), pc.x, pc.y,
                                    juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
        g.setGradientFill (stain);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (pc));
    }
    for (int i = 0; i < 40; ++i)
    {
        const auto p0 = randIn (plate);
        const float ang = rng.nextFloat() * 3.14159f, len = 4.0f + rng.nextFloat() * 38.0f;
        g.setColour ((rng.nextFloat() < 0.6f ? juce::Colours::white : juce::Colour (0xff4a3a24)).withAlpha (0.10f + rng.nextFloat() * 0.18f));
        g.drawLine (p0.x, p0.y, p0.x + std::cos (ang) * len, p0.y + std::sin (ang) * len, 0.5f);
    }
    {   // sujeira nas bordas de cima e dos lados
        const auto pf = plate.toFloat();
        const float d = 34.0f;
        const juce::Colour grime (0xff3d2b14);
        auto band = [&] (juce::Rectangle<float> r, juce::Point<float> from, juce::Point<float> to)
        {
            juce::ColourGradient eg (grime.withAlpha (0.30f), from.x, from.y, grime.withAlpha (0.0f), to.x, to.y, false);
            g.setGradientFill (eg);
            g.fillRect (r);
        };
        band (pf.withWidth (d),                { pf.getX(), 0 },     { pf.getX() + d, 0 });
        band (pf.withLeft (pf.getRight() - d), { pf.getRight(), 0 }, { pf.getRight() - d, 0 });
        band (pf.withHeight (d),               { 0, pf.getY() },     { 0, pf.getY() + d });
    }

    auto screw = [&] (float sx, float sy)
    {
        const auto r = juce::Rectangle<float> (11, 11).withCentre ({ sx, sy });
        juce::ColourGradient sg (juce::Colour (0xffe0ded8), r.getX(), r.getY(), juce::Colour (0xff5e5b55), r.getRight(), r.getBottom(), false);
        g.setGradientFill (sg);
        g.fillEllipse (r);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (r, 0.8f);
        const float a = rng.nextFloat() * 3.14f;
        g.drawLine (sx - std::cos (a) * 4.0f, sy - std::sin (a) * 4.0f, sx + std::cos (a) * 4.0f, sy + std::sin (a) * 4.0f, 1.4f);
    };
    screw ((float) plate.getX() + 12.0f, 12.0f);
    screw ((float) plate.getRight() - 12.0f, 12.0f);

    // --- nome com a versão logo depois (padrão DubMastor) + placa de latão MRM-0420
    juce::String ver (JucePlugin_VersionString);
    if (ver.containsChar ('.') && ver.fromFirstOccurrenceOf (".", false, false).containsChar ('.'))
        ver = ver.upToLastOccurrenceOf (".", false, false);
    const juce::String title ("MASTOR RIDDIM MACHINE " + ver);
    g.setFont (stencil (30.0f, 0.2f));
    const float titleW = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText (title, juce::Rectangle<float> (60.0f, 19.0f, titleW + 10.0f, 40.0f).translated (0.8f, 1.0f), juce::Justification::left);
    g.setColour (col::ink);
    g.drawText (title, juce::Rectangle<float> (60.0f, 19.0f, titleW + 10.0f, 40.0f), juce::Justification::left);

    g.setColour (col::inkSoft);
    g.setFont (stencil (10.5f, 0.25f));
    g.drawText (u8 ("DUB CHORDS + DUB DRUMS  \xc2\xb7  RIDDIM STARTER  \xc2\xb7  DUB STATION"),
                62, 58, 700, 14, juce::Justification::left);

    {   // placa de latão grande, com o nome do modelo gravado
        const juce::String t ("MRM-0420");
        g.setFont (stencil (27.0f, 0.3f));
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t);
        auto pl = juce::Rectangle<float> (60.0f + titleW + 30.0f, 14.0f, tw + 92.0f, 52.0f);
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.fillRoundedRectangle (pl.translated (2.0f, 3.5f), 4.0f);
        juce::ColourGradient brass (juce::Colour (0xffe8d390), pl.getX(), pl.getY(), juce::Colour (0xff86692a), pl.getX(), pl.getBottom(), false);
        brass.addColour (0.45, juce::Colour (0xffcaab5c));
        brass.addColour (0.55, juce::Colour (0xffb39446));
        g.setGradientFill (brass);
        g.fillRoundedRectangle (pl, 4.0f);
        for (int i = 0; i < 110; ++i)          // escovado
        {
            const float yy = pl.getY() + 1.0f + rng.nextFloat() * (pl.getHeight() - 2.0f);
            const float x0 = pl.getX() + rng.nextFloat() * pl.getWidth() * 0.5f;
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colour (0xff5a4316)).withAlpha (0.06f + rng.nextFloat() * 0.08f));
            g.drawHorizontalLine ((int) yy, x0, juce::jmin (pl.getRight() - 1.0f, x0 + 20.0f + rng.nextFloat() * pl.getWidth() * 0.6f));
        }
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawLine (pl.getX() + 3, pl.getY() + 1, pl.getRight() - 3, pl.getY() + 1, 1.0f);
        g.setColour (juce::Colour (0xff3d2c0c).withAlpha (0.7f));
        g.drawLine (pl.getX() + 3, pl.getBottom() - 1, pl.getRight() - 3, pl.getBottom() - 1, 1.2f);
        g.drawRoundedRectangle (pl, 4.0f, 0.9f);
        auto inner = pl.reduced (30.0f, 7.0f);
        g.setColour (juce::Colour (0xff4a3712).withAlpha (0.6f));
        g.drawRoundedRectangle (inner, 2.5f, 1.0f);
        for (float rx : { pl.getX() + 15.0f, pl.getRight() - 15.0f })
        {
            const auto rv = juce::Rectangle<float> (11, 11).withCentre ({ rx, pl.getCentreY() });
            juce::ColourGradient rg (juce::Colour (0xfff0dfa6), rv.getX(), rv.getY(), juce::Colour (0xff6b5220), rv.getRight(), rv.getBottom(), false);
            g.setGradientFill (rg);
            g.fillEllipse (rv);
            g.setColour (juce::Colour (0xff3d2c0c).withAlpha (0.6f));
            g.drawLine (rv.getX() + 2.5f, rv.getCentreY(), rv.getRight() - 2.5f, rv.getCentreY(), 1.0f);
        }
        // gravação: sombra clara embaixo, tinta escura por cima
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawText (t, inner.translated (1.0f, 1.2f), juce::Justification::centred);
        g.setColour (juce::Colour (0xff2a1d06));
        g.drawText (t, inner, juce::Justification::centred);
    }

    {   // faixas da bandeira da Jamaica pintadas à mão
        const float sw = 250.0f, sx = (float) plate.getRight() - 40.0f - sw, sh = 7.0f;
        const juce::Colour faded[] = {
            col::jamGreen.interpolatedWith (juce::Colour (0xff6f7a4a), 0.35f),
            col::jamGold.interpolatedWith (juce::Colour (0xffb08a3a), 0.40f),
            juce::Colour (0xff2b2621) };
        for (int k = 0; k < 3; ++k)
        {
            const float y = 27.0f + 9.0f * (float) k;
            juce::Path band;
            band.startNewSubPath (sx, y + rng.nextFloat() * 0.8f);
            for (float x = sx; x <= sx + sw; x += 6.0f) band.lineTo (x, y + (rng.nextFloat() - 0.3f) * 0.9f);
            for (float x = sx + sw; x >= sx; x -= 6.0f) band.lineTo (x, y + sh + (rng.nextFloat() - 0.7f) * 0.9f);
            band.closeSubPath();
            juce::ColourGradient fade (faded[k].withAlpha (0.92f), sx, y, faded[k].withAlpha (0.70f), sx + sw, y, false);
            fade.addColour (0.3, faded[k].brighter (0.12f).withAlpha (0.85f));
            g.setGradientFill (fade);
            g.fillPath (band);
        }
        g.setColour (col::inkSoft);
        g.setFont (stencil (9.0f, 0.3f));
        g.drawText ("RIDDIM MACHINE", juce::Rectangle<float> (sx, 56.0f, sw, 12.0f), juce::Justification::centredRight);
    }

    // --- seções com plaqueta preta (igual aos painéis)
    auto section = [&] (juce::Rectangle<int> r, const juce::String& t)
    {
        auto rf = r.toFloat();
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (rf.translated (1, 1), 4.0f, 1.0f);
        g.setColour (col::ink.withAlpha (0.55f));
        g.drawRoundedRectangle (rf, 4.0f, 1.2f);
        g.setFont (stencil (11.5f, 0.22f));
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 22.0f;
        auto tag = juce::Rectangle<float> (rf.getX() + 12, rf.getY() - 9, tw, 18);
        g.setColour (col::labelBg);
        g.fillRoundedRectangle (tag, 2.0f);
        g.setColour (col::cream);
        g.drawText (t, tag, juce::Justification::centred);
    };
    section (tabSection, u8 ("M\xc3\x93" "DULO"));
    section (riddimSection, "RIDDIM");

    // --- emenda com o painel de baixo: friso gravado
    const float y = (float) getHeight() - 1.5f;
    g.setColour (col::ink.withAlpha (0.35f));
    g.drawLine ((float) plate.getX(), y - 1.0f, (float) plate.getRight(), y - 1.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.drawLine ((float) plate.getX(), y, (float) plate.getRight(), y, 1.0f);
}
} // namespace mrmui

//==============================================================================
namespace
{
    constexpr int kCropChords = 74;   // o painel aparece a partir da altura das plaquetas dos módulos
    constexpr int kCropDrums  = 74;
    constexpr float kChordsScale = (float) MrmRoot::baseW / 1240.0f;
    const int kContentH = juce::roundToInt ((846 - kCropChords) * kChordsScale);
    const int kBaseH = mrmui::Header::height + kContentH;
}

MrmRoot::MrmRoot (MrmProcessor& p)
    : proc (p), chordsPanel (p.getChords()), drumsPanel (p.getDrums()),
      header (p, [this] (int t) { showTab (t); })
{
    addChildComponent (chordsPanel);
    addChildComponent (drumsPanel);
    addAndMakeVisible (header);    // por cima dos painéis: cobre o título de cada um
    setSize (baseW, kBaseH);
    showTab (proc.getCurrentTab());
}

void MrmRoot::paintOverChildren (juce::Graphics& g)
{
    // sombra do cabeçalho sobre o painel da aba
    const float y = (float) mrmui::Header::height;
    juce::ColourGradient sh (juce::Colours::black.withAlpha (0.28f), 0.0f, y, juce::Colours::transparentBlack, 0.0f, y + 10.0f, false);
    g.setGradientFill (sh);
    g.fillRect (0.0f, y, (float) getWidth(), 10.0f);
}

void MrmRoot::showTab (int t)
{
    t = juce::jlimit (0, 1, t);
    proc.setCurrentTab (t);
    chordsPanel.setVisible (t == 0);
    drumsPanel.setVisible (t == 1);
    header.setTab (t);
}

void MrmRoot::resized()
{
    header.setBounds (0, 0, baseW, mrmui::Header::height);
    const float top = (float) mrmui::Header::height;

    chordsPanel.setTransform (juce::AffineTransform::scale (kChordsScale)
                                  .translated (0.0f, top - (float) kCropChords * kChordsScale));

    const float sx = (float) baseW / (float) drumsPanel.getWidth();
    const float sy = (float) kContentH / (float) (drumsPanel.getHeight() - kCropDrums);
    drumsPanel.setTransform (juce::AffineTransform::scale (sx, sy)
                                 .translated (0.0f, top - (float) kCropDrums * sy));
}

//==============================================================================
MrmEditor::MrmEditor (MrmProcessor& p) : juce::AudioProcessorEditor (p), root (p)
{
    addAndMakeVisible (root);
    const int w = root.getWidth(), h = root.getHeight();
    setResizable (true, true);
    setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);

    // abre num tamanho que caiba na tela (notebooks com 768/900 px de altura)
    double s = 1.0;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        s = juce::jlimit (0.55, 1.0, ((double) d->userArea.getHeight() - 90.0) / (double) h);
    setSize (juce::roundToInt (w * s), juce::roundToInt (h * s));
}

void MrmEditor::resized()
{
    root.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) root.getWidth()));
}
