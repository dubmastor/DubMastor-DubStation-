#include "MrmEditor.h"
#include "BpmPresets.h"
#include "MrmStyle.h"

#ifndef MRM_DRUMS_VER
 #define MRM_DRUMS_VER ""
#endif
#ifndef MRM_CHORDS_VER
 #define MRM_CHORDS_VER ""
#endif
#ifndef MRM_BASS_VER
 #define MRM_BASS_VER ""
#endif
#ifndef MRM_KNOB_VER
 #define MRM_KNOB_VER ""
#endif

namespace mrmui
{
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
    {
        g.setColour (col::amber.withAlpha (0.75f));
        g.fillRoundedRectangle (cap.withTop (cap.getBottom() - 3.0f).reduced (10.0f, 0.0f).translated (0, -2.0f), 1.5f);
    }

    const bool narrow = getWidth() < 110;
    lamp (g, juce::Rectangle<float> (narrow ? 8.0f : 10.0f, narrow ? 8.0f : 10.0f).withCentre ({ cap.getX() + (narrow ? 11.0f : 16.0f), cap.getCentreY() }), selected);

    auto txt = cap.withTrimmedLeft (narrow ? 18.0f : 28.0f).withTrimmedRight (narrow ? 3.0f : 6.0f);
    g.setColour (selected ? col::amber : col::cream.withAlpha (0.62f));
    g.setFont (stencil (getWidth() < 110 ? 12.5f : 14.5f, getWidth() < 110 ? 0.06f : 0.14f));
    g.drawFittedText (title, txt.withTrimmedBottom (txt.getHeight() * 0.42f).toNearestInt(), juce::Justification::centredBottom, 1, 0.8f);
    g.setColour (selected ? col::cream : col::cream.withAlpha (0.42f));
    g.setFont (stencil (getWidth() < 110 ? 7.5f : 8.5f, getWidth() < 110 ? 0.1f : 0.24f));
    g.drawFittedText (sub, txt.withTrimmedTop (txt.getHeight() * 0.6f).toNearestInt(), juce::Justification::centredTop, 1, 0.8f);
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
    : proc (p), att (*p.apvts.getParameter ("bpm"), [this] (float v)
      {
          value = v;
          bool match = false;    // o nome do estilo só fica enquanto o BPM é um dos presets dele
          for (const auto& pr : bpmpresets::presets) match = match || (styleName == pr.style && juce::roundToInt (v) == pr.bpm);
          if (! match) styleName = {};
          repaint();
      }, nullptr)
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void BpmWindow::refresh()
{
    const bool daw = proc.isDawPlaying();
    const double b = proc.getDawBpm();
    if (daw != dawShown || (daw && std::abs (b - dawBpm) > 0.01))
    {
        dawShown = daw;
        dawBpm = b;
        repaint();
    }
}

bool BpmWindow::onStyleArea (juce::Point<int> p) const { return p.x > juce::roundToInt (getWidth() * 0.4f); }

void BpmWindow::showStyleMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("BPM POR ESTILO");
    const char* last = nullptr;
    juce::PopupMenu sub;
    auto flush = [&] { if (last != nullptr) menu.addSubMenu (last, sub); sub = {}; };
    for (int i = 0; i < bpmpresets::numPresets; ++i)
    {
        const auto& pr = bpmpresets::presets[i];
        if (last == nullptr || juce::String (last) != pr.style) { flush(); last = pr.style; }
        sub.addItem (i + 1, juce::String (pr.name) + "  " + juce::String (pr.bpm), true, juce::roundToInt (value) == pr.bpm);
    }
    flush();
    juce::Component::SafePointer<BpmWindow> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        safe->att.setValueAsCompleteGesture ((float) bpmpresets::presets[r - 1].bpm);
        safe->styleName = bpmpresets::presets[r - 1].style;
        safe->repaint();
    });
}

void BpmWindow::mouseDown (const juce::MouseEvent& e)
{
    styleClick = onStyleArea (e.getPosition());
    if (styleClick) { showStyleMenu(); return; }
    dragStart = value; att.beginGesture();
}
void BpmWindow::mouseUp (const juce::MouseEvent&)        { if (! styleClick) att.endGesture(); styleClick = false; }
void BpmWindow::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (onStyleArea (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::UpDownResizeCursor);
}
void BpmWindow::mouseDrag (const juce::MouseEvent& e)
{
    if (styleClick) return;
    styleName = {};
    att.setValueAsPartOfGesture (juce::jlimit (50.0f, 180.0f, std::round (dragStart - (float) e.getDistanceFromDragStartY() * 0.35f)));
}
void BpmWindow::mouseDoubleClick (const juce::MouseEvent& e) { if (! onStyleArea (e.getPosition())) { styleName = {}; att.setValueAsCompleteGesture (75.0f); } }
void BpmWindow::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const int step = w.deltaY > 0 ? 1 : w.deltaY < 0 ? -1 : 0;
    if (step != 0) att.setValueAsCompleteGesture (juce::jlimit (50.0f, 180.0f, value + (float) step));
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
    displayGlass (g, r);
    const juce::String num (juce::roundToInt (dawShown ? dawBpm : (double) value));
    auto numArea = r.withTrimmedRight (r.getWidth() * 0.6f);
    auto unitArea = r.withTrimmedLeft (r.getWidth() * 0.4f);
    g.setFont (stencil (22.0f, 0.06f));
    glowText (g, num, numArea, juce::Justification::centredRight);
    g.setFont (stencil (9.0f, 0.2f));
    g.setColour (col::amber.withAlpha (0.75f));
    g.drawText ("BPM", unitArea.withTrimmedBottom (unitArea.getHeight() * 0.5f).withTrimmedLeft (6), juce::Justification::centredLeft);
    g.setColour (col::amber.withAlpha (0.5f));
    g.setFont (stencil (8.0f, 0.12f));
    g.drawText (dawShown ? juce::String ("DAW") : (styleName.isNotEmpty() ? styleName : juce::String ("ESTILO")) + juce::String::fromUTF8 (" \xe2\x96\xbe"), unitArea.withTrimmedTop (unitArea.getHeight() * 0.5f).withTrimmedLeft (6), juce::Justification::centredLeft);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f), 3.0f);
}

//==============================================================================
Header::Header (MrmProcessor& p, std::function<void (int)> onTab)
    : proc (p), bpm (p)
{
    setOpaque (true);
    // ordem das máquinas: sempre bateria, baixo e riddim
    const juce::String titles[MrmProcessor::numTabs] = { "PRINCIPAL", "BATERIA", "BAIXO", "RIDDIM", u8 ("\xc3\x93RG\xc3\x83O"), "ARRANJO", "MIXER", "SHOW", "CONTROL" };
    const juce::String subs[MrmProcessor::numTabs] = { "RESUMO", "MRD-12", "MRB-69", u8 ("MRK-80 \xc2\xb7 KEYS"), "MRO-68", "PARTES", "+ SIRENE",
                                                       "10 RIDDIMS", "ROOM \xc2\xb7 BIG KNOB" };
    for (int i = 0; i < MrmProcessor::numTabs; ++i)
    {
        tabs[(size_t) i] = std::make_unique<TabKey> (titles[i], subs[i]);
        tabs[(size_t) i]->onClick = [onTab, i] { onTab (i); };
        addAndMakeVisible (*tabs[(size_t) i]);
    }
    addAndMakeVisible (play);
    playAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "play", play);
    addAndMakeVisible (bpm);
    setSize (MrmRoot::baseW, height);
    startTimerHz (15);
}

Header::~Header() { stopTimer(); }

void Header::setTab (int t)
{
    for (int i = 0; i < MrmProcessor::numTabs; ++i) tabs[(size_t) i]->setSelected (i == t);
}

void Header::timerCallback() { bpm.refresh(); }

void Header::resized()
{
    const int W = getWidth(), H = getHeight();
    woodL = { 0, 0, 28, H };
    woodR = { W - 28, 0, 28, H };
    plate = { 28, 0, W - 56, H };
    tabSection       = { 52, 86, 820, 56 };
    transportSection = { 888, 86, W - 52 - 888, 56 };

    auto a = tabSection.reduced (10, 0).withTrimmedTop (11).withHeight (40);
    const int gap = 8, tw = (a.getWidth() - gap * (MrmProcessor::numTabs - 1)) / MrmProcessor::numTabs;
    for (int i = 0; i < MrmProcessor::numTabs; ++i)
    {
        tabs[(size_t) i]->setBounds (a.removeFromLeft (tw));
        a.removeFromLeft (gap);
    }
    auto t = transportSection.reduced (10, 0).withTrimmedTop (11).withHeight (40);
    play.setBounds (t.removeFromLeft (142));
    t.removeFromLeft (10);
    bpm.setBounds (t);
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

    paintWood (g, woodL, rng);
    paintWood (g, woodR, rng);
    paintPlate (g, plate, rng, true, false);
    {   // faixa de console anos 70 (verde-petróleo do MRM-0420) atrás do nome; cada máquina tem a sua cor
        const auto band = plate.withHeight (74).toFloat();
        const juce::Colour bc (0xff3d5a55);
        g.setColour (bc);
        g.fillRect (band);
        for (int i = 0; i < 900; ++i)
        {
            const float x = band.getX() + rng.nextFloat() * band.getWidth(), y = band.getY() + rng.nextFloat() * band.getHeight();
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.03f + rng.nextFloat() * 0.05f));
            g.fillRect (x, y, 1.0f + rng.nextFloat() * 2.0f, 1.0f);
        }
        g.setColour (bc.darker (0.6f));
        g.fillRect (band.getX(), band.getBottom() - 3.0f, band.getWidth(), 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRect (band.getX(), band.getBottom(), band.getWidth(), 1.0f);
    }
    screw (g, (float) plate.getX() + 12.0f, 12.0f, rng);
    screw (g, (float) plate.getRight() - 12.0f, 12.0f, rng);

    // --- nome com a versão logo depois (padrão DubStation) + placa de latão MRM-0420
    juce::String ver (JucePlugin_VersionString);
    if (ver.containsChar ('.') && ver.fromFirstOccurrenceOf (".", false, false).containsChar ('.'))
        ver = ver.upToLastOccurrenceOf (".", false, false);
    const juce::String title ("MASTOR RIDDIM MACHINE " + ver);
    g.setFont (stencil (30.0f, 0.2f));
    const float titleW = textWidth (g.getCurrentFont(), title);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawText (title, juce::Rectangle<float> (60.0f, 14.0f, titleW + 10.0f, 38.0f).translated (0.8f, 1.0f), juce::Justification::left);
    g.setColour (juce::Colour (0xfff3e6c8));
    g.drawText (title, juce::Rectangle<float> (60.0f, 14.0f, titleW + 10.0f, 38.0f), juce::Justification::left);
    engraved (g, u8 ("BATERIA + BAIXO + RIDDIM + ARRANJO + MIXER + SHOW  \xc2\xb7  RIDDIM STARTER"),
              juce::Rectangle<float> (62.0f, 51.0f, 700.0f, 13.0f), juce::Justification::left, 10.0f, 0.25f, juce::Colour (0xfff3e6c8).withAlpha (0.75f));

    {   // placa de latão grande com o modelo
        g.setFont (stencil (25.0f, 0.3f));
        const float tw = textWidth (g.getCurrentFont(), "MRM-0420");
        brassPlate (g, juce::Rectangle<float> (60.0f + titleW + 26.0f, 11.0f, tw + 92.0f, 48.0f), "MRM-0420", 25.0f, rng);
    }

    {   // marca DUBSTATION com as faixas da bandeira da Jamaica pintadas à mão
        const float sw = 220.0f, sx = (float) plate.getRight() - 30.0f - sw, sh = 6.0f;
        const juce::Colour faded[] = {
            col::jamGreen.interpolatedWith (juce::Colour (0xff6f7a4a), 0.35f),
            col::jamGold.interpolatedWith (juce::Colour (0xffb08a3a), 0.40f),
            juce::Colour (0xff2b2621) };
        for (int k = 0; k < 3; ++k)
        {
            const float y = 16.0f + 8.0f * (float) k;
            juce::Path band;
            band.startNewSubPath (sx, y + rng.nextFloat() * 0.8f);
            for (float x = sx; x <= sx + sw; x += 6.0f) band.lineTo (x, y + (rng.nextFloat() - 0.3f) * 0.9f);
            for (float x = sx + sw; x >= sx; x -= 6.0f) band.lineTo (x, y + sh + (rng.nextFloat() - 0.7f) * 0.9f);
            band.closeSubPath();
            juce::ColourGradient fade (faded[k].withAlpha (0.92f), sx, y, faded[k].withAlpha (0.70f), sx + sw, y, false);
            g.setGradientFill (fade);
            g.fillPath (band);
        }
        engraved (g, "DUBSTATION", juce::Rectangle<float> (sx, 42.0f, sw, 22.0f), juce::Justification::centredRight, 19.0f, 0.42f, juce::Colour (0xfff3e6c8));
    }

    section (g, tabSection, "ABAS");
    section (g, transportSection, "TRANSPORTE");

    // emenda com o painel de baixo: friso gravado
    const float y = (float) getHeight() - 1.5f;
    g.setColour (col::ink.withAlpha (0.35f));
    g.drawLine ((float) plate.getX(), y - 1.0f, (float) plate.getRight(), y - 1.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.drawLine ((float) plate.getX(), y, (float) plate.getRight(), y, 1.0f);
}

//==============================================================================
void PanelHolder::addCover (juce::Rectangle<int> area, juce::String text)
{
    auto c = std::make_unique<Cover>();
    c->text = std::move (text);
    addAndMakeVisible (*c);
    covers.emplace_back (area, std::move (c));
    resized();
}

void PanelHolder::Cover::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (6.0f);
    g.setColour (col::plate.withAlpha (0.93f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::ink.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
    g.setColour (col::inkSoft);
    g.setFont (stencil (15.0f, 0.12f));
    g.drawFittedText (text, r.reduced (24.0f, 8.0f).toNearestInt(), juce::Justification::centred, 3, 0.8f);
}

void PanelHolder::resized()
{
    const float pw = (float) panel.getWidth(), ph = (float) (panel.getHeight() - cropTop);
    scale = juce::jmin ((float) getWidth() / pw, (float) getHeight() / ph);
    const float x = ((float) getWidth() - pw * scale) * 0.5f;
    const float y = std::floor (((float) getHeight() - ph * scale) * 0.5f);   // painel centralizado na aba
    shown = { x, y, pw * scale, ph * scale };
    const auto t = juce::AffineTransform::scale (scale).translated (x, y - (float) cropTop * scale);
    panel.setTransform (t);
    for (auto& [area, c] : covers)
        c->setBounds (area.toFloat().transformedBy (t).getSmallestIntegerContainer());

    // fundo: o painel creme do MRM-0420 continua onde o painel da máquina não chega
    if (getWidth() <= 0) return;
    background = juce::Image (juce::Image::ARGB, getWidth() * 2, getHeight() * 2, true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (2.0f));
    juce::Random rng (77);
    const int W = getWidth(), H = getHeight();
    paintWood (g, { 0, 0, 28, H }, rng);
    paintWood (g, { W - 28, 0, 28, H }, rng);
    paintPlate (g, { 28, 0, W - 56, H }, rng, false, true);
    screw (g, 40.0f, (float) H - 12.0f, rng);
    screw (g, (float) W - 40.0f, (float) H - 12.0f, rng);
    const float free = (float) H - shown.getBottom();
    if (shown.getY() > 34.0f)
        engraved (g, u8 ("MASTOR RIDDIM MACHINE MRM-0420  \xc2\xb7  DUBSTATION"),
                  juce::Rectangle<float> (28.0f, shown.getY() * 0.5f - 8.0f, (float) W - 56.0f, 16.0f),
                  juce::Justification::centred, 11.0f, 0.4f);
    if (free > 34.0f)
        engraved (g, u8 ("UMA M\xc3\x81QUINA POR ABA  \xc2\xb7  A ABA RIDDIM RESUME TODAS"),
                  juce::Rectangle<float> (28.0f, shown.getBottom() + free * 0.5f - 8.0f, (float) W - 56.0f, 16.0f),
                  juce::Justification::centred, 11.0f, 0.4f);
}

void PanelHolder::paint (juce::Graphics& g)
{
    g.fillAll (col::woodDark);
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());
    // sombra do painel da máquina sobre o painel creme
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (shown.withY (shown.getBottom()).withHeight (4.0f));
    if (shown.getY() > 1.0f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillRect (shown.withY (shown.getY() - 3.0f).withHeight (3.0f));
    }
}
//==============================================================================
MuteKey::MuteKey (juce::RangedAudioParameter& p, juce::String t, juce::String sb, juce::Colour c)
    : att (p, [this] (float v) { muted = v > 0.5f; repaint(); }, nullptr), title (std::move (t)), sub (std::move (sb)), colour (c)
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setRepaintsOnMouseActivity (true);
}

void MuteKey::mouseDown (const juce::MouseEvent&) { att.setValueAsCompleteGesture (muted ? 0.0f : 1.0f); }

void MuteKey::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (0, 3.0f), 6.0f);
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (r, 6.0f);

    auto cap = r.reduced (6.0f).translated (0, muted ? 2.0f : 0.0f);
    const juce::Colour base = muted ? colour.withSaturation (colour.getSaturation() * 0.35f).darker (1.4f)
                                    : colour.brighter (isMouseOver() ? 0.12f : 0.0f);
    if (! muted)
    {
        g.setColour (colour.withAlpha (0.28f));
        g.fillRoundedRectangle (cap.expanded (4.0f), 7.0f);
    }
    juce::ColourGradient cg (base, cap.getX(), cap.getY(), base.darker (0.8f), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (muted ? 0.05f : 0.25f));
    g.fillRoundedRectangle (cap.withHeight (cap.getHeight() * 0.35f).reduced (5.0f, 3.0f), 3.0f);

    const auto ink = muted ? col::cream.withAlpha (0.45f) : juce::Colour (0xff1a1208);
    g.setColour (ink);
    g.setFont (stencil (juce::jmin (26.0f, cap.getHeight() * 0.34f), 0.2f));
    g.drawText (title, cap.withTrimmedBottom (cap.getHeight() * 0.38f), juce::Justification::centredBottom);
    g.setFont (stencil (9.0f, 0.3f));
    g.setColour (ink.withMultipliedAlpha (0.8f));
    g.drawText (sub, cap.withTrimmedTop (cap.getHeight() * 0.64f).withTrimmedBottom (cap.getHeight() * 0.14f), juce::Justification::centred);

    // lâmpada MUTE
    auto lampArea = juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ cap.getRight() - 18.0f, cap.getY() + 16.0f });
    lamp (g, lampArea, muted, juce::Colour (0xffff3a2a));
    g.setFont (stencil (8.5f, 0.2f));
    g.setColour (muted ? juce::Colour (0xffff6a5a) : ink.withMultipliedAlpha (0.6f));
    g.drawText ("MUTE", juce::Rectangle<float> (lampArea.getX() - 44.0f, lampArea.getY(), 40.0f, 12.0f), juce::Justification::centredRight);
}

//==============================================================================
ControlRoomPage::ControlRoomPage (MrmProcessor& p, juce::Component& knobPanel, int cropTop)
    : holder (knobPanel, cropTop),
      drums  (*p.apvts.getParameter ("muteDrums"),  "DRUMS",  u8 ("MRD-12  \xc2\xb7  BATERIA"), juce::Colour (0xff2fbf55)),
      bass   (*p.apvts.getParameter ("muteBass"),   "BASS",   u8 ("MRB-69  \xc2\xb7  BAIXO"),   col::jamGold),
      riddim (*p.apvts.getParameter ("muteChords"), "RIDDIM", u8 ("MRCM-1240  \xc2\xb7  SKANK E BUBBLE"), juce::Colour (0xffe8432c)),
      fx   (*p.apvts.getParameter ("fxOn"), "CONTROL ROOM", "CONTROL ROOM"),
      echo (*p.getKnob().apvts.getParameter ("echoOn"), "TAPE ECHO", "TAPE ECHO"),
      organ (*p.apvts.getParameter ("muteOrgan"), u8 ("\xc3\x93RG\xc3\x83O"), u8 ("MRO-68  \xc2\xb7  GROOVES"), juce::Colour (0xff4f9a5a))
{
    for (juce::Component* c : { (juce::Component*) &holder, (juce::Component*) &drums, (juce::Component*) &bass,
                                (juce::Component*) &riddim, (juce::Component*) &fx, (juce::Component*) &echo, (juce::Component*) &organ })
        addAndMakeVisible (c);
}

void ControlRoomPage::resized()
{
    const int W = getWidth(), H = getHeight();
    holder.setBounds (0, 0, W, 660);
    muteSection   = { 52, 686, 820, 144 };
    masterSection = { 892, 686, W - 52 - 892, 144 };
    auto m = muteSection.reduced (14, 0).withTrimmedTop (16).withHeight (116);
    const int gap = 12, kw = (m.getWidth() - 3 * gap) / 4;
    drums.setBounds (m.removeFromLeft (kw));  m.removeFromLeft (gap);
    bass.setBounds (m.removeFromLeft (kw));   m.removeFromLeft (gap);
    riddim.setBounds (m.removeFromLeft (kw)); m.removeFromLeft (gap);
    organ.setBounds (m);
    auto a = masterSection.reduced (14, 0).withTrimmedTop (22);
    fx.setBounds (a.removeFromTop (44));
    a.removeFromTop (14);
    echo.setBounds (a.removeFromTop (44));

    background = juce::Image (juce::Image::ARGB, W * 2, (H - 660) * 2, true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (2.0f).translated (0.0f, -660.0f * 2.0f));
    juce::Random rng (3);
    paintWood (g, { 0, 660, 28, H - 660 }, rng);
    paintWood (g, { W - 28, 660, 28, H - 660 }, rng);
    paintPlate (g, { 28, 660, W - 56, H - 660 }, rng, false, true);
    screw (g, 40.0f, (float) H - 12.0f, rng);
    screw (g, (float) W - 40.0f, (float) H - 12.0f, rng);
    section (g, muteSection, u8 ("MUTE DAS M\xc3\x81QUINAS"));
    section (g, masterSection, "MASTER");
}

void ControlRoomPage::paint (juce::Graphics& g)
{
    g.fillAll (col::woodDark);
    if (background.isValid())
        g.drawImage (background, juce::Rectangle<float> (0.0f, 660.0f, (float) getWidth(), (float) getHeight() - 660.0f));
}

//==============================================================================
OrganPage::OrganPage (MrmProcessor& p, juce::Component& organPanel)
    : proc (p), holder (organPanel, 0),
      power (*p.apvts.getParameter ("organOn"), u8 ("\xc3\x93RG\xc3\x83O LIGADO"), u8 ("\xc3\x93RG\xc3\x83O DESLIGADO")),
      solo  (*p.apvts.getParameter ("soloOrgan"), "SOLO", "SOLO"),
      mute  (*p.apvts.getParameter ("muteOrgan"), u8 ("\xc3\x93RG\xc3\x83O"), u8 ("MRO-68  \xc2\xb7  NO RIDDIM"), juce::Colour (0xff4f9a5a)),
      vol   (*p.apvts.getParameter ("volOrgan")),
      pan   (*p.apvts.getParameter ("panOrgan"))
{
    for (juce::Component* c : { (juce::Component*) &holder, (juce::Component*) &power, (juce::Component*) &solo,
                                (juce::Component*) &mute, (juce::Component*) &vol, (juce::Component*) &pan })
        addAndMakeVisible (c);
    startTimerHz (10);
}

OrganPage::~OrganPage() { stopTimer(); }

void OrganPage::timerCallback()
{
    const float db = proc.apvts.getRawParameterValue ("volOrgan")->load();
    if (db != lastDb) { lastDb = db; repaint (mixSection); }
}

void OrganPage::resized()
{
    const int W = getWidth(), H = getHeight(), top = barTop();
    holder.setBounds (0, 0, W, top);
    keySection = { 52, top + 22, 760, H - top - 40 };
    mixSection = { 832, top + 22, W - 52 - 832, H - top - 40 };
    auto k = keySection.reduced (16, 0).withTrimmedTop (22);
    power.setBounds (k.removeFromLeft (250).withHeight (44).translated (0, 24));
    k.removeFromLeft (18);
    mute.setBounds (k.removeFromLeft (240).withHeight (92));
    k.removeFromLeft (18);
    solo.setBounds (k.withHeight (44).translated (0, 24));
    auto m = mixSection.reduced (14, 0).withTrimmedTop (14);
    vol.setBounds (m.removeFromLeft (100).withHeight (90));
    m.removeFromLeft (110);
    pan.setBounds (m.removeFromLeft (100).withHeight (90));

    background = juce::Image (juce::Image::ARGB, W * 2, (H - top) * 2, true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (2.0f).translated (0.0f, -(float) top * 2.0f));
    juce::Random rng (5);
    paintWood (g, { 0, top, 28, H - top }, rng);
    paintWood (g, { W - 28, top, 28, H - top }, rng);
    paintPlate (g, { 28, top, W - 56, H - top }, rng, false, true);
    screw (g, 40.0f, (float) H - 12.0f, rng);
    screw (g, (float) W - 40.0f, (float) H - 12.0f, rng);
    section (g, keySection, u8 ("\xc3\x93RG\xc3\x83O NO RIDDIM  \xc2\xb7  HARMONIA E BPM DO RIDDIM"));
    section (g, mixSection, "VOLUME E PAN");
}

void OrganPage::paint (juce::Graphics& g)
{
    const int top = barTop();
    g.fillAll (col::woodDark);
    if (background.isValid())
        g.drawImage (background, juce::Rectangle<float> (0.0f, (float) top, (float) getWidth(), (float) (getHeight() - top)));
    // volume num visor âmbar, entre o VOLUME e o PAN
    auto m = mixSection.reduced (14, 0).withTrimmedTop (14);
    auto box = juce::Rectangle<float> ((float) m.getX() + 108.0f, (float) m.getY() + 24.0f, 96.0f, 40.0f);
    displayGlass (g, box);
    g.setFont (stencil (17.0f, 0.0f));
    glowText (g, lastDb <= -59.9f ? u8 ("\xe2\x88\x92\xe2\x88\x9e") : juce::String (lastDb, 1) + " dB", box, juce::Justification::centred);
    engraved (g, "VOLUME", juce::Rectangle<float> ((float) m.getX(), (float) m.getY() + 94.0f, 100.0f, 13.0f), juce::Justification::centred, 9.5f, 0.3f);
    engraved (g, "PAN", juce::Rectangle<float> ((float) m.getX() + 210.0f, (float) m.getY() + 94.0f, 100.0f, 13.0f), juce::Justification::centred, 9.5f, 0.3f);
    auto k = keySection.reduced (16, 0).withTrimmedTop (22);
    engraved (g, "LIGA O MRO-68 NO RIDDIM", juce::Rectangle<float> ((float) k.getX(), (float) k.getY() + 2.0f, 250.0f, 13.0f), juce::Justification::centred, 8.5f, 0.2f);
    engraved (g, u8 ("S\xc3\x93 O \xc3\x93RG\xc3\x83O TOCANDO"), juce::Rectangle<float> ((float) k.getRight() - 236.0f, (float) k.getY() + 2.0f, 236.0f, 13.0f), juce::Justification::centred, 8.5f, 0.2f);
}
} // namespace mrmui


//==============================================================================
namespace
{
    // altura do título de cada painel (o cabeçalho do MRM-0420 já tem o nome)
    constexpr int kCropChords = 74, kCropDrums = 74, kCropBass = 74, kCropKnob = 64;
}

MrmRoot::MrmRoot (MrmProcessor& p)
    : proc (p), mainPage (p),
      chordsPanel (p.getChords()), drumsPanel (p.getDrums()), bassPanel (p.getBass()), knobPanel (p.getKnob()), organPanel (p.getOrgan()),
      chordsHolder (chordsPanel, kCropChords), drumsHolder (drumsPanel, kCropDrums),
      bassHolder (bassPanel, kCropBass), controlRoom (p, knobPanel, kCropKnob), organPage (p, organPanel),
      arrangePage (p), mixerPage (p), showPage (p),
      header (p, [this] (int t) { showTab (t); })
{
    for (juce::Component* c : { (juce::Component*) &mainPage, (juce::Component*) &chordsHolder, (juce::Component*) &drumsHolder,
                                (juce::Component*) &bassHolder, (juce::Component*) &controlRoom, (juce::Component*) &arrangePage,
                                (juce::Component*) &mixerPage, (juce::Component*) &showPage, (juce::Component*) &organPage })
        addChildComponent (c);
    addAndMakeVisible (header);

    setSize (baseW, baseH);
    showTab (proc.getCurrentTab());
}

void MrmRoot::showTab (int t)
{
    t = juce::jlimit (0, MrmProcessor::numTabs - 1, t);
    proc.setCurrentTab (t);
    mainPage.setVisible (t == MrmProcessor::TabMain);
    drumsHolder.setVisible (t == MrmProcessor::TabDrums);
    bassHolder.setVisible (t == MrmProcessor::TabBass);
    chordsHolder.setVisible (t == MrmProcessor::TabChords);
    organPage.setVisible (t == MrmProcessor::TabOrgan);
    arrangePage.setVisible (t == MrmProcessor::TabArrange);
    mixerPage.setVisible (t == MrmProcessor::TabMixer);
    showPage.setVisible (t == MrmProcessor::TabShow);
    controlRoom.setVisible (t == MrmProcessor::TabControl);
    if (t == MrmProcessor::TabArrange) arrangePage.refresh();
    header.setTab (t);
}

void MrmRoot::paintOverChildren (juce::Graphics& g)
{
    // sombra do cabeçalho sobre a aba
    const float y = (float) mrmui::Header::height;
    juce::ColourGradient sh (juce::Colours::black.withAlpha (0.28f), 0.0f, y, juce::Colours::transparentBlack, 0.0f, y + 10.0f, false);
    g.setGradientFill (sh);
    g.fillRect (0.0f, y, (float) getWidth(), 10.0f);
}

void MrmRoot::resized()
{
    header.setBounds (0, 0, baseW, mrmui::Header::height);
    const auto content = juce::Rectangle<int> (0, mrmui::Header::height, baseW, mrmui::MainPage::kH);
    mainPage.setBounds (content);
    for (auto* h : { &chordsHolder, &drumsHolder, &bassHolder })
        h->setBounds (content);
    controlRoom.setBounds (content);
    organPage.setBounds (content);
    for (juce::Component* c : { (juce::Component*) &arrangePage, (juce::Component*) &mixerPage, (juce::Component*) &showPage })
        c->setBounds (content);
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
        s = juce::jlimit (0.5, 1.0, ((double) d->userArea.getHeight() - 90.0) / (double) h);
    setSize (juce::roundToInt (w * s), juce::roundToInt (h * s));
}

void MrmEditor::resized()
{
    root.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) root.getWidth()));
}
