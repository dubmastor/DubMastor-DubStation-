#include "MrmPages.h"

namespace mrmui
{
namespace
{
    constexpr int kW = MainPage::kW, kH = MainPage::kH;
    const juce::Colour machineColour[MrmProcessor::NumMachines] = { juce::Colour (0xff2fbf55), juce::Colour (0xffe8432c), col::jamGold };
    const char* machineName[MrmProcessor::NumMachines] = { "BATERIA", "RIDDIM", "BAIXO" };
    const char* machineModel[MrmProcessor::NumMachines] = { "MRD-12", "MRK-80", "MRB-69" };
    // ordem das máquinas na tela: sempre bateria, baixo e riddim
    constexpr int screenOrder[MrmProcessor::NumMachines] = { MrmProcessor::Drums, MrmProcessor::Bass, MrmProcessor::Chords };

    juce::String mmss (double s)
    {
        const int t = juce::roundToInt (s);
        return juce::String (t / 60) + ":" + juce::String (t % 60).paddedLeft ('0', 2);
    }
}

//==============================================================================
void PageBase::renderBackground (std::vector<std::pair<juce::Rectangle<int>, juce::String>> sections)
{
    const int W = getWidth(), H = getHeight();
    if (W <= 0 || H <= 0) return;
    background = juce::Image (juce::Image::ARGB, W, H, true);
    juce::Graphics g (background);
    juce::Random rng (7);
    paintWood (g, { 0, 0, 28, H }, rng);
    paintWood (g, { W - 28, 0, 28, H }, rng);
    paintPlate (g, { 28, 0, W - 56, H }, rng, false, true);
    for (auto [x, y] : { std::pair<float, float> { 40.0f, 14.0f }, { (float) W - 40.0f, 14.0f }, { 40.0f, (float) H - 12.0f }, { (float) W - 40.0f, (float) H - 12.0f } })
        screw (g, x, y, rng);
    for (auto& [r, t] : sections) section (g, r, t);
}

void PageBase::paint (juce::Graphics& g)
{
    g.fillAll (col::plate);
    if (background.isValid()) g.drawImageAt (background, 0, 0);
}

void KeyButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (0, 2.0f), 4.0f);
    auto cap = r.translated (0, down ? 1.5f : 0.0f);
    juce::ColourGradient cg (juce::Colour (0xff3a3632).brighter (over && isEnabled() ? 0.1f : 0.0f), cap.getX(), cap.getY(),
                             juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 4.0f);
    if (lit)
    {
        g.setColour (litColour.withAlpha (0.85f));
        g.fillRoundedRectangle (cap.withTop (cap.getBottom() - 3.0f).reduced (8.0f, 0.0f).translated (0, -1.5f), 1.0f);
    }
    g.setColour (! isEnabled() ? col::cream.withAlpha (0.25f) : lit ? litColour : col::cream.withAlpha (0.78f));
    g.setFont (stencil (fontSize, 0.1f));
    g.drawFittedText (getButtonText(), cap.reduced (6.0f, 2.0f).toNearestInt(), juce::Justification::centred, 2, 0.7f);
}

//==============================================================================
// ARRANJO · ordem da música em quadrados
ArrangeTimeline::ArrangeTimeline (MrmProcessor& p) : proc (p)
{
    setTooltip (u8 ("Arraste os quadrados para mudar a ordem. \xe2\x97\x80 \xe2\x96\xb6 movem, \xc3\x97 tira da m\xc3\xbasica. "
                    "Com o PLAY RIDDIM tocando, clique num quadrado para pular para ele."));
}

std::vector<ArrangeTimeline::Block> ArrangeTimeline::layout() const
{
    const auto a = proc.getArrangement();
    std::vector<Block> out;
    const float pad = 10.0f, gap = 6.0f, h = 84.0f;
    const float W = (float) getWidth() - 2.0f * pad;
    float x = pad, y = pad;
    for (int k = 0; k < (int) a.order.size(); ++k)
    {
        const auto* sec = a.sectionAt (k);
        const float w = juce::jlimit (92.0f, W, 34.0f + (sec ? (float) sec->bars : 4.0f) * 12.0f);
        if (x + w > pad + W) { x = pad; y += h + gap; }
        Block b;
        b.r = { x, y, w, h };
        const float by = y + h - 26.0f;
        b.left = { x + 6.0f, by, 24.0f, 20.0f };
        b.right = { x + 33.0f, by, 24.0f, 20.0f };
        b.del = { x + 60.0f, by, 24.0f, 20.0f };
        out.push_back (b);
        x += w + gap;
    }
    return out;
}

void ArrangeTimeline::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0d0906));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (col::amber.withAlpha (0.25f));
    {
        juce::Path p; p.addRoundedRectangle (r.reduced (1.0f), 8.0f);
        const float dash[] = { 6.0f, 5.0f };
        juce::Path d; juce::PathStrokeType (1.0f).createDashedStroke (d, p, dash, 2);
        g.fillPath (d);
    }
    const auto a = proc.getArrangement();
    const auto blocks = layout();
    if (blocks.empty())
    {
        g.setColour (col::cream.withAlpha (0.5f));
        g.setFont (stencil (14.0f, 0.06f));
        g.drawFittedText (u8 ("Toque em \"+ NA M\xc3\x9aSICA\" numa parte para montar a ordem da m\xc3\xbasica"), getLocalBounds().reduced (20), juce::Justification::centred, 2);
        return;
    }
    const bool live = litSlot >= 0;
    for (int k = 0; k < (int) blocks.size(); ++k)
    {
        const auto* sec = a.sectionAt (k);
        if (sec == nullptr) continue;
        auto b = blocks[(size_t) k].r;
        const bool now = k == litSlot;
        const juce::Colour c (sec->colour);
        if (now)
        {
            // trecho tocando: aceso, com brilho na cor da parte e um pulso a cada tempo
            for (int i = 4; i >= 1; --i)
            {
                g.setColour (c.withAlpha (0.10f + 0.06f * pulse));
                g.fillRoundedRectangle (b.expanded ((float) i * 4.0f + 3.0f * pulse), 9.0f + (float) i * 3.0f);
            }
            b = b.translated (0, -3.0f);
        }
        if (k == dragTo && dragFrom >= 0 && dragFrom != dragTo)
        {
            g.setColour (juce::Colours::white);
            g.fillRect (b.getX() - 5.0f, b.getY(), 3.0f, b.getHeight());
        }
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (b.translated (0, 3.0f), 7.0f);
        g.setColour (live && ! now ? c.darker (1.2f) : now ? c.brighter (0.25f) : c);
        g.fillRoundedRectangle (b, 7.0f);
        if (k == dragFrom) { g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (b, 7.0f); }
        if (now)
        {
            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (b.expanded (2.0f), 8.0f, 3.0f);
            const float prog = (float) (litBar + 1) / (float) juce::jmax (1, sec->bars);
            g.fillRoundedRectangle (b.withTop (b.getBottom() - 6.0f).withWidth (b.getWidth() * prog), 2.0f);
        }
        const auto ink = juce::Colour (0xff14100c).withAlpha (live && ! now ? 0.6f : 1.0f);
        g.setColour (ink);
        g.setFont (stencil (14.0f, 0.04f));
        g.drawFittedText ((now ? u8 ("\xe2\x96\xb6 ") : juce::String()) + sec->name, b.reduced (8.0f, 6.0f).withHeight (20.0f).toNearestInt(), juce::Justification::topLeft, 1, 0.7f);
        g.setFont (stencil (11.0f, 0.04f));
        g.drawText (juce::String (sec->bars) + " COMP.", b.reduced (8.0f, 26.0f).withHeight (14.0f), juce::Justification::topLeft);
        g.drawText (juce::String (k + 1), b.reduced (6.0f, 6.0f), juce::Justification::topRight);
        const auto& bl = blocks[(size_t) k];
        for (auto [rr, t] : { std::pair<juce::Rectangle<float>, juce::String> { bl.left.translated (0, now ? -3.0f : 0.0f), u8 ("\xe2\x97\x80") },
                              { bl.right.translated (0, now ? -3.0f : 0.0f), u8 ("\xe2\x96\xb6") }, { bl.del.translated (0, now ? -3.0f : 0.0f), u8 ("\xc3\x97") } })
        {
            g.setColour (juce::Colours::black.withAlpha (0.3f));
            g.fillRoundedRectangle (rr, 4.0f);
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.setFont (stencil (11.0f, 0.0f));
            g.drawText (t, rr, juce::Justification::centred);
        }
    }
}

void ArrangeTimeline::mouseDown (const juce::MouseEvent& e)
{
    downPos = e.position;
    dragFrom = dragTo = -1;
    const auto blocks = layout();
    auto a = proc.getArrangement();
    for (int k = 0; k < (int) blocks.size(); ++k)
    {
        const auto& b = blocks[(size_t) k];
        if (! b.r.contains (e.position)) continue;
        if (b.left.contains (e.position)) { if (k > 0) std::swap (a.order[(size_t) k - 1], a.order[(size_t) k]); }
        else if (b.right.contains (e.position)) { if (k + 1 < (int) a.order.size()) std::swap (a.order[(size_t) k + 1], a.order[(size_t) k]); }
        else if (b.del.contains (e.position)) a.order.erase (a.order.begin() + k);
        else { dragFrom = k; return; }
        proc.setArrangement (a);
        if (onChange) onChange();
        repaint();
        return;
    }
}

void ArrangeTimeline::mouseDrag (const juce::MouseEvent& e)
{
    if (dragFrom < 0 || e.position.getDistanceFrom (downPos) < 6.0f) return;
    const auto blocks = layout();
    dragTo = (int) blocks.size();
    for (int k = 0; k < (int) blocks.size(); ++k)
        if (blocks[(size_t) k].r.expanded (3.0f).contains (e.position)) { dragTo = k; break; }
    repaint();
}

void ArrangeTimeline::mouseUp (const juce::MouseEvent& e)
{
    if (dragFrom < 0) return;
    if (e.position.getDistanceFrom (downPos) < 6.0f || dragTo < 0)
    {
        // clique: com o PLAY RIDDIM tocando o arranjo, pula para a parte
        auto a = proc.getArrangement();
        if (! a.on) { a.on = true; proc.setArrangement (a); }
        proc.requestArrangeJump (dragFrom);
        if (! proc.isDawPlaying() && ! proc.isMasterPlaying())
            if (auto* pl = proc.apvts.getParameter ("play")) pl->setValueNotifyingHost (1.0f);
    }
    else if (dragTo != dragFrom)
    {
        auto a = proc.getArrangement();
        const int v = a.order[(size_t) dragFrom];
        a.order.erase (a.order.begin() + dragFrom);
        int to = dragTo > dragFrom ? dragTo - 1 : dragTo;
        to = juce::jlimit (0, (int) a.order.size(), to);
        a.order.insert (a.order.begin() + to, v);
        proc.setArrangement (a);
    }
    dragFrom = dragTo = -1;
    if (onChange) onChange();
    repaint();
}

//==============================================================================
ArrangePage::ArrangePage (MrmProcessor& p) : proc (p), timeline (p)
{
    setOpaque (true);
    topSection   = { 52, 22, 1176, 120 };
    orderSection = { 52, 166, 1176, 250 };
    partsSection = { 52, 440, 1176, 950 };
    statusArea   = { 70, 100, 1140, 30 };

    for (auto* k : { &onKey, &loopKey, &exportKey, &stdKey, &clearKey, &newKey }) addAndMakeVisible (*k);
    onKey.fontSize = 15.0f;
    onKey.onClick   = [this] { edit ([] (mrm::Arrangement& a) { a.on = ! a.on; }); };
    loopKey.onClick = [this] { edit ([] (mrm::Arrangement& a) { a.loop = ! a.loop; }); };
    stdKey.onClick  = [this] { edit ([] (mrm::Arrangement& a) { auto d = mrm::Arrangement::makeDefault(); a.order.clear();
                                       for (int i : d.order) if (i < (int) a.sections.size()) a.order.push_back (i); }); };
    clearKey.onClick = [this] { edit ([] (mrm::Arrangement& a) { a.order.clear(); }); };
    newKey.onClick = [this]
    {
        edit ([] (mrm::Arrangement& a)
        {
            static const juce::uint32 cols[] = { 0xffb06ab0, 0xff6ab0a0, 0xffc0a040, 0xffd07050 };
            mrm::Section s;
            s.id = "p" + juce::String (juce::Time::currentTimeMillis() % 100000);
            s.name = juce::String::fromUTF8 ("Nova parte ") + juce::String ((int) a.sections.size() + 1);
            s.colour = cols[a.sections.size() % 4];
            s.bars = 4;
            a.sections.push_back (s);
        });
    };
    exportKey.onClick = [this] { exportSong(); };
    addAndMakeVisible (timeline);
    timeline.onChange = [this] { refresh(); };

    setSize (kW, kH);
    refresh();
    startTimerHz (30);
}

ArrangePage::~ArrangePage() { stopTimer(); }

void ArrangePage::edit (std::function<void (mrm::Arrangement&)> f)
{
    auto a = proc.getArrangement();
    f (a);
    proc.setArrangement (a);
    refresh();
}

void ArrangePage::refresh()
{
    const auto a = proc.getArrangement();
    onKey.lit = a.on;
    onKey.setButtonText (a.on ? juce::String ("TOCAR O ARRANJO: SIM") : u8 ("TOCAR O ARRANJO: N\xc3\x83O"));
    loopKey.lit = a.loop;
    loopKey.setButtonText (a.loop ? juce::String ("REPETIR NO FIM: SIM") : u8 ("REPETIR NO FIM: N\xc3\x83O"));

    if ((int) a.sections.size() != lastRows)
    {
        lastRows = (int) a.sections.size();
        rows.clear();
        rows.resize (a.sections.size());
        for (int i = 0; i < (int) a.sections.size(); ++i)
        {
            auto& r = rows[(size_t) i];
            r.minus = std::make_unique<KeyButton> (u8 ("\xe2\x88\x92"));
            r.plus  = std::make_unique<KeyButton> ("+");
            r.add   = std::make_unique<KeyButton> (u8 ("+ NA M\xc3\x9aSICA"), col::jamGreen.brighter (0.6f));
            r.minus->onClick = [this, i] { edit ([i] (mrm::Arrangement& x) { auto& b = x.sections[(size_t) i].bars; b = juce::jmax (1, b - 1); }); };
            r.plus->onClick  = [this, i] { edit ([i] (mrm::Arrangement& x) { auto& b = x.sections[(size_t) i].bars; b = juce::jmin (64, b + 1); }); };
            r.add->onClick   = [this, i] { edit ([i] (mrm::Arrangement& x) { x.order.push_back (i); }); };
            if (i >= 6)   // as 6 partes de fábrica não somem
            {
                r.del = std::make_unique<KeyButton> (u8 ("\xc3\x97"));
                r.del->onClick = [this, i]
                {
                    edit ([i] (mrm::Arrangement& x)
                    {
                        std::vector<int> o;
                        for (int k : x.order) if (k != i) o.push_back (k > i ? k - 1 : k);
                        x.order = o;
                        x.sections.erase (x.sections.begin() + i);
                    });
                };
                addAndMakeVisible (*r.del);
            }
            for (int s = 0; s < MrmProcessor::NumMachines; ++s)
            {
                const int m = screenOrder[s];
                r.mach[(size_t) m] = std::make_unique<KeyButton> (machineName[m], machineColour[m].brighter (0.3f));
                r.mach[(size_t) m]->onClick = [this, i, m] { edit ([i, m] (mrm::Arrangement& x) { auto& on = x.sections[(size_t) i].on[(size_t) m]; on = ! on; }); };
                addAndMakeVisible (*r.mach[(size_t) m]);
            }
            r.name = std::make_unique<juce::Label>();
            r.name->setEditable (true);
            r.name->setFont (stencil (17.0f, 0.06f));
            r.name->setColour (juce::Label::textColourId, col::cream);
            r.name->setColour (juce::Label::backgroundColourId, juce::Colour (0xff14100c));
            r.name->setColour (juce::Label::outlineColourId, juce::Colours::black);
            r.name->setTooltip ("Clique duas vezes para renomear a parte");
            r.name->onTextChange = [this, i]
            {
                const auto t = rows[(size_t) i].name->getText().trim();
                if (t.isNotEmpty()) edit ([i, t] (mrm::Arrangement& x) { x.sections[(size_t) i].name = t.substring (0, 24); });
            };
            for (auto* c : { (juce::Component*) r.minus.get(), (juce::Component*) r.plus.get(), (juce::Component*) r.add.get(), (juce::Component*) r.name.get() })
                addAndMakeVisible (*c);
        }
        resized();
    }
    for (int i = 0; i < (int) rows.size(); ++i)
    {
        const auto& sec = a.sections[(size_t) i];
        auto& r = rows[(size_t) i];
        if (! r.name->isBeingEdited()) r.name->setText (sec.name, juce::dontSendNotification);
        for (int m = 0; m < MrmProcessor::NumMachines; ++m) { r.mach[(size_t) m]->lit = sec.on[(size_t) m]; r.mach[(size_t) m]->repaint(); }
    }
    timeline.repaint();
    repaint();
}

void ArrangePage::resized()
{
    onKey.setBounds (70, 46, 300, 46);
    loopKey.setBounds (384, 46, 240, 46);
    exportKey.setBounds (638, 46, 270, 46);
    stdKey.setBounds (922, 46, 140, 46);
    clearKey.setBounds (1072, 46, 140, 46);
    timeline.setBounds (orderSection.reduced (16, 0).withTrimmedTop (28).withTrimmedBottom (16));
    newKey.setBounds (partsSection.getRight() - 196, partsSection.getY() + 22, 180, 34);
    int y = partsSection.getY() + 66;
    for (auto& r : rows)
    {
        r.area = { 70, y, 1140, 52 };
        r.name->setBounds (96, y + 9, 220, 34);
        r.minus->setBounds (330, y + 9, 40, 34);
        r.plus->setBounds (440, y + 9, 40, 34);
        int x = 500;
        for (int s = 0; s < MrmProcessor::NumMachines; ++s)
        {
            r.mach[(size_t) screenOrder[s]]->setBounds (x, y + 9, 140, 34);
            x += 148;
        }
        r.add->setBounds (954, y + 9, 170, 34);
        if (r.del) r.del->setBounds (1134, y + 9, 40, 34);
        y += 58;
    }
    renderBackground ({ { topSection, u8 ("ARRANJO  \xc2\xb7  A M\xc3\x9aSICA PARTE POR PARTE") }, { orderSection, u8 ("ORDEM DA M\xc3\x9aSICA") },
                        { partsSection, u8 ("PARTES  \xc2\xb7  QUANTOS COMPASSOS E QUEM TOCA EM CADA UMA") } });
}

void ArrangePage::paint (juce::Graphics& g)
{
    PageBase::paint (g);
    const auto a = proc.getArrangement();
    // linhas das partes: cor, contador de compassos
    for (int i = 0; i < (int) rows.size() && i < (int) a.sections.size(); ++i)
    {
        const auto& r = rows[(size_t) i];
        const auto& sec = a.sections[(size_t) i];
        const bool now = timeline.litSlot >= 0 && a.order.size() > (size_t) timeline.litSlot && a.order[(size_t) timeline.litSlot] == i;
        auto ar = r.area.toFloat();
        g.setColour (now ? juce::Colour (sec.colour).withAlpha (0.25f) : juce::Colours::black.withAlpha (0.06f));
        g.fillRoundedRectangle (ar, 5.0f);
        g.setColour (juce::Colour (sec.colour));
        g.fillRoundedRectangle (ar.withWidth (16.0f).reduced (2.0f, 4.0f), 3.0f);
        if (now) { g.drawRoundedRectangle (ar, 5.0f, 2.5f); }
        auto box = juce::Rectangle<float> (374.0f, ar.getY() + 9.0f, 62.0f, 34.0f);
        displayGlass (g, box);
        g.setFont (stencil (18.0f, 0.0f));
        glowText (g, juce::String (sec.bars), box.withTrimmedBottom (11.0f), juce::Justification::centred);
        g.setColour (col::cream.withAlpha (0.55f));
        g.setFont (stencil (8.0f, 0.2f));
        g.drawText ("COMPASSOS", box.withTrimmedTop (22.0f), juce::Justification::centred);
    }
    // resumo: partes, compassos, duração e o que está tocando
    const double bpm = proc.getDawBpm() > 0.0 && proc.isDawPlaying() ? proc.getDawBpm() : (double) proc.apvts.getRawParameterValue ("bpm")->load();
    const int tot = a.totalBars();
    juce::String sum = juce::String ((int) a.order.size()) + u8 (" partes  \xc2\xb7  ") + juce::String (tot) + u8 (" compassos  \xc2\xb7  ")
                     + mmss (tot * 4.0 * 60.0 / juce::jmax (30.0, bpm)) + " a " + juce::String (juce::roundToInt (bpm)) + " BPM";
    juce::String now;
    if (proc.getExportProgress() >= 0.0f || proc.getExportMessage().isNotEmpty()) now = proc.getExportMessage();
    else if (! a.on) now = u8 ("Arranjo desligado: o riddim toca em loop. Ligue TOCAR O ARRANJO ou clique num quadrado.");
    else if (timeline.litSlot >= 0)
        if (const auto* sec = a.sectionAt (timeline.litSlot))
            now = u8 ("\xe2\x96\xb6 ") + sec->name + u8 ("  \xc2\xb7  compasso ") + juce::String (timeline.litBar + 1) + " de " + juce::String (sec->bars)
                + u8 ("  \xc2\xb7  parte ") + juce::String (timeline.litSlot + 1) + " de " + juce::String ((int) a.order.size());
    engraved (g, sum, juce::Rectangle<float> (70.0f, 100.0f, 520.0f, 30.0f), juce::Justification::centredLeft, 13.0f, 0.1f, col::ink);
    g.setColour (col::ink);
    g.setFont (stencil (13.0f, 0.06f));
    g.drawFittedText (now, juce::Rectangle<int> (600, 100, 610, 30), juce::Justification::centredRight, 1, 0.7f);
    if (const float pr = proc.getExportProgress(); pr >= 0.0f)
    {
        auto bar = exportKey.getBounds().toFloat().withTop ((float) exportKey.getBottom() + 2.0f).withHeight (4.0f);
        g.setColour (juce::Colours::black.withAlpha (0.3f)); g.fillRect (bar);
        g.setColour (col::jamGreen); g.fillRect (bar.withWidth (bar.getWidth() * pr));
    }
}

void ArrangePage::timerCallback()
{
    const int slot = proc.getArrangeSlot(), bar = proc.getArrangeBar();
    const double beat = proc.getPlayBeat();
    const float pulse = beat >= 0.0 ? (float) std::pow (1.0 - std::fmod (beat, 1.0), 3.0) : 0.0f;
    const bool exporting = proc.getExportProgress() >= 0.0f;
    exportKey.setEnabled (! exporting);
    if (slot != timeline.litSlot || bar != timeline.litBar || std::abs (pulse - timeline.pulse) > 0.02f || exporting)
    {
        const bool changed = slot != timeline.litSlot || bar != timeline.litBar;
        timeline.litSlot = slot; timeline.litBar = bar; timeline.pulse = pulse;
        timeline.repaint();
        if (changed || exporting) repaint();
    }
    static juce::String lastMsg;
    if (proc.getExportMessage() != lastMsg) { lastMsg = proc.getExportMessage(); repaint(); }
}

void ArrangePage::exportSong()
{
    const auto a = proc.getArrangement();
    if (a.order.empty()) return;
    juce::String name = "MRM-0420 arranjo " + juce::String (a.totalBars()) + " compassos " + juce::String (juce::roundToInt (proc.apvts.getRawParameterValue ("bpm")->load())) + " BPM";
    chooser = std::make_unique<juce::FileChooser> ("Exportar a música (WAV + MIDI)",
                  juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile (name + ".wav"), "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              proc.startSongExport (f.withFileExtension ("wav"));
                              repaint();
                          });
}

//==============================================================================
// MIXER
ChannelFader::ChannelFader (juce::RangedAudioParameter& p, std::function<float()> lv, juce::Colour c)
    : param (p), att (p, [this] (float v) { value = param.convertTo0to1 (v); repaint(); }, nullptr), level (std::move (lv)), colour (c)
{
    att.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

juce::Rectangle<float> ChannelFader::track() const { return getLocalBounds().toFloat().withTrimmedTop (14.0f).withTrimmedBottom (14.0f).withTrimmedLeft (34.0f); }

void ChannelFader::tick()
{
    const float l = level ? level() : 0.0f;
    const float m = l > 0.0f ? juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (l, -60.0f) + 48.0f) / 54.0f) : 0.0f;
    if (std::abs (m - meter) > 0.004f) { meter = m; repaint(); }
}

void ChannelFader::paint (juce::Graphics& g)
{
    auto t = track();
    // medidor
    auto vu = juce::Rectangle<float> (t.getRight() - 14.0f, t.getY(), 10.0f, t.getHeight());
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (vu, 2.0f);
    auto lit = vu.withTop (vu.getBottom() - vu.getHeight() * meter);
    juce::ColourGradient vg (juce::Colour (0xffc8402f), vu.getX(), vu.getY(), juce::Colour (0xff4f9a5a), vu.getX(), vu.getBottom(), false);
    vg.addColour (0.25, juce::Colour (0xffe6b83a));
    g.setGradientFill (vg);
    g.fillRoundedRectangle (lit.reduced (1.5f), 1.5f);
    // trilho e escala
    auto rail = juce::Rectangle<float> (6.0f, t.getHeight()).withCentre ({ t.getX() + (t.getWidth() - 20.0f) * 0.5f, t.getCentreY() });
    g.setColour (juce::Colour (0xff0a0807));
    g.fillRoundedRectangle (rail, 3.0f);
    g.setFont (stencil (9.0f, 0.0f));
    for (float db : { 6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -40.0f, -60.0f })
    {
        const float y = t.getBottom() - param.convertTo0to1 (db) * t.getHeight();
        g.setColour (col::ink.withAlpha (0.55f));
        g.drawHorizontalLine ((int) y, t.getX(), rail.getX() - 3.0f);
        g.drawText (db == 0.0f ? "0" : juce::String ((int) db), juce::Rectangle<float> (t.getX() - 30.0f, y - 6.0f, 26.0f, 12.0f), juce::Justification::centredRight);
    }
    // botão do fader
    const float y = t.getBottom() - value * t.getHeight();
    auto cap = juce::Rectangle<float> (rail.getWidth() + 34.0f, 26.0f).withCentre ({ rail.getCentreX(), y });
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (cap.translated (0, 3.0f), 3.0f);
    juce::ColourGradient cg (juce::Colour (0xffefe9dc), cap.getX(), cap.getY(), juce::Colour (0xff7d7566), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (cap, 3.0f);
    g.setColour (colour.darker (0.2f));
    g.fillRect (cap.withSizeKeepingCentre (cap.getWidth() - 8.0f, 3.0f));
}

void ChannelFader::mouseDown (const juce::MouseEvent& e) { att.beginGesture(); mouseDrag (e); }
void ChannelFader::mouseDrag (const juce::MouseEvent& e)
{
    const auto t = track();
    value = juce::jlimit (0.0f, 1.0f, (t.getBottom() - e.position.y) / t.getHeight());
    att.setValueAsPartOfGesture (param.convertFrom0to1 (value));
    repaint();
}
void ChannelFader::mouseUp (const juce::MouseEvent&) { att.endGesture(); }
void ChannelFader::mouseDoubleClick (const juce::MouseEvent&) { att.setValueAsCompleteGesture (param.getDefaultValue() >= 0.0f ? param.convertFrom0to1 (param.getDefaultValue()) : 0.0f); }
void ChannelFader::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    value = juce::jlimit (0.0f, 1.0f, value + w.deltaY * 0.05f);
    att.setValueAsCompleteGesture (param.convertFrom0to1 (value));
}

MixerPage::MixerPage (MrmProcessor& p) : proc (p)
{
    setOpaque (true);
    mixSection   = { 52, 22, 1176, 640 };
    sirenSection = { 52, 690, 1176, 420 };
    for (int s = 0; s < MrmProcessor::NumMachines; ++s)
    {
        const int m = screenOrder[s];
        auto& st = strips[(size_t) s];
        st.machine = m;
        st.fader = std::make_unique<ChannelFader> (*proc.apvts.getParameter (MrmProcessor::volId (m)), [this, m] { return proc.getMachineLevel (m); }, machineColour[m]);
        st.pan = std::make_unique<BigDial> (*proc.apvts.getParameter (MrmProcessor::panId (m)));
        st.mute = std::make_unique<MuteKey> (*proc.apvts.getParameter (MrmProcessor::muteId (m)), machineName[m], machineModel[m], machineColour[m]);
        st.solo = std::make_unique<PowerSwitch> (*proc.apvts.getParameter (MrmProcessor::soloId (m)), "SOLO", "SOLO");
        for (juce::Component* c : { (juce::Component*) st.fader.get(), (juce::Component*) st.pan.get(), (juce::Component*) st.mute.get(), (juce::Component*) st.solo.get() })
            addAndMakeVisible (*c);
    }
    fire = std::make_unique<SirenFire> (*proc.apvts.getParameter ("sirenFire"));
    hold = std::make_unique<PowerSwitch> (*proc.apvts.getParameter ("sirenHold"), "HOLD", "HOLD");
    echoOn = std::make_unique<PowerSwitch> (*proc.apvts.getParameter ("sirEchoOn"), "TAPE ECHO LIGADO", "TAPE ECHO DESLIGADO");
    juce::StringArray modes;
    for (int i = 1; i <= 12; ++i) modes.add (juce::String (i));
    mode = std::make_unique<MusicianKeys> (*proc.apvts.getParameter ("sirEchoMode"), modes, 12);
    const char* ids[] = { "sirEchoInput", "sirEchoRate", "sirEchoInt", "sirEchoVol", "sirEchoRev", "sirEchoWow", "sirEchoAge" };
    for (size_t i = 0; i < echoDials.size(); ++i)
    {
        echoDials[i] = std::make_unique<BigDial> (*proc.apvts.getParameter (ids[i]));
        addAndMakeVisible (*echoDials[i]);
    }
    for (juce::Component* c : { (juce::Component*) fire.get(), (juce::Component*) hold.get(), (juce::Component*) echoOn.get(), (juce::Component*) mode.get() })
        addAndMakeVisible (*c);
    setSize (kW, kH);
    startTimerHz (30);
}

MixerPage::~MixerPage() { stopTimer(); }

void MixerPage::resized()
{
    for (int s = 0; s < MrmProcessor::NumMachines; ++s)
    {
        auto& st = strips[(size_t) s];
        const int x = 110 + s * 360;
        st.area = { x, 60, 320, 580 };
        st.mute->setBounds (x + 20, 74, 280, 74);
        st.solo->setBounds (x + 70, 160, 180, 36);
        st.pan->setBounds (x + 120, 206, 80, 76);
        st.fader->setBounds (x + 76, 300, 150, 320);
    }
    fire->setBounds (80, 724, 120, 110);
    hold->setBounds (214, 732, 150, 36);
    echoOn->setBounds (214, 782, 290, 40);
    mode->setBounds (540, 760, 660, 34);
    for (size_t i = 0; i < echoDials.size(); ++i)
        echoDials[i]->setBounds (96 + (int) i * 158, 880, 100, 94);
    renderBackground ({ { mixSection, u8 ("MIXER  \xc2\xb7  VOLUME, PAN, MUTE E SOLO DE CADA M\xc3\x81QUINA") },
                        { sirenSection, u8 ("SIRENE  \xc2\xb7  MASTOR TAPE ECHO MTE-12 (O MESMO MOTOR DO PLUGIN)") } });
}

void MixerPage::paint (juce::Graphics& g)
{
    PageBase::paint (g);
    for (const auto& st : strips)
    {
        const int m = st.machine;
        auto ar = st.area.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.07f));
        g.fillRoundedRectangle (ar, 6.0f);
        g.setColour (machineColour[m]);
        g.fillRect (ar.withHeight (4.0f).reduced (8.0f, 0.0f));
        engraved (g, "PAN", juce::Rectangle<float> (ar.getX() + 50.0f, 238.0f, 60.0f, 13.0f), juce::Justification::centredRight, 10.0f, 0.3f);
        const float db = proc.apvts.getRawParameterValue (MrmProcessor::volId (m))->load();
        auto box = juce::Rectangle<float> (ar.getX() + 236.0f, 440.0f, 70.0f, 34.0f);
        displayGlass (g, box);
        g.setFont (stencil (16.0f, 0.0f));
        glowText (g, db <= -59.9f ? u8 ("\xe2\x88\x92\xe2\x88\x9e") : juce::String (db, 1), box.withTrimmedBottom (10.0f), juce::Justification::centred);
        g.setColour (col::cream.withAlpha (0.55f));
        g.setFont (stencil (8.0f, 0.2f));
        g.drawText ("dB", box.withTrimmedTop (22.0f), juce::Justification::centred);
    }
    engraved (g, u8 ("SEGURE O FIRE  \xc2\xb7  HOLD DEIXA TOCANDO"), juce::Rectangle<float> (40.0f, 840.0f, 340.0f, 13.0f), juce::Justification::centredLeft, 8.5f, 0.2f);
    engraved (g, u8 ("MODE SELECTOR  \xc2\xb7  1-4 ECO  \xc2\xb7  5-11 ECO + MOLA  \xc2\xb7  12 S\xc3\x93 MOLA"), juce::Rectangle<float> (540.0f, 738.0f, 660.0f, 14.0f),
              juce::Justification::centred, 9.5f, 0.25f);
    const char* names[] = { "INPUT", "REPEAT RATE", "INTENSITY", "ECHO", "REVERB", "WOW & FLUTTER", "TAPE AGE" };
    for (int i = 0; i < 7; ++i)
        engraved (g, names[i], juce::Rectangle<float> (76.0f + (float) i * 158.0f, 978.0f, 140.0f, 13.0f), juce::Justification::centred, 9.5f, 0.3f);
    engraved (g, u8 ("Padr\xc3\xa3o de f\xc3\xa1" "brica do MTE-12: Mode 7, tempo livre pelo Repeat Rate. A sirene usa o PITCH, RATE, DEPTH, FORMA e LEVEL da aba PRINCIPAL."),
              juce::Rectangle<float> (70.0f, 1020.0f, 1140.0f, 16.0f), juce::Justification::centred, 10.0f, 0.1f, col::ink);
}

void MixerPage::timerCallback()
{
    for (auto& st : strips) st.fader->tick();
    static float lastDb[3] {};
    bool ch = false;
    for (const auto& st : strips)
    {
        const float db = proc.apvts.getRawParameterValue (MrmProcessor::volId (st.machine))->load();
        if (db != lastDb[st.machine]) { lastDb[st.machine] = db; ch = true; }
    }
    if (ch) repaint();
}

//==============================================================================
// SHOW
ShowPage::ShowPage (MrmProcessor& p) : proc (p)
{
    setOpaque (true);
    showSection = { 52, 22, 1176, 980 };
    for (int i = 0; i < MrmProcessor::numShowSlots; ++i)
    {
        auto& c = cards[(size_t) i];
        c.play   = std::make_unique<KeyButton> (u8 ("\xe2\x96\xb6 TOCAR"), col::jamGreen.brighter (0.7f));
        c.save   = std::make_unique<KeyButton> ("SALVAR AQUI");
        c.rename = std::make_unique<KeyButton> ("RENOMEAR");
        c.del    = std::make_unique<KeyButton> ("APAGAR");
        c.play->fontSize = 15.0f;
        c.play->onClick = [this, i]
        {
            if (proc.loadShowSlot (i))
            {
                message = "Tocando " + proc.getShowSlot (i).name;
                if (! proc.isDawPlaying())
                    if (auto* pl = proc.apvts.getParameter ("play")) pl->setValueNotifyingHost (1.0f);
            }
        };
        c.save->onClick   = [this, i] { askName (i, false); };
        c.rename->onClick = [this, i] { askName (i, true); };
        c.del->onClick    = [this, i]
        {
            const auto nm = proc.getShowSlot (i).name;
            auto opts = juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon)
                            .withTitle ("SHOW").withMessage (u8 ("Apagar \"") + nm + u8 ("\" do lugar ") + juce::String (i + 1) + "?")
                            .withButton ("Apagar").withButton ("Cancelar").withAssociatedComponent (this);
            juce::AlertWindow::showAsync (opts, [this, i] (int r) { if (r == 1) { proc.clearShowSlot (i); message = u8 ("Lugar ") + juce::String (i + 1) + " apagado"; } });
        };
        for (juce::Component* k : { (juce::Component*) c.play.get(), (juce::Component*) c.save.get(), (juce::Component*) c.rename.get(), (juce::Component*) c.del.get() })
            addAndMakeVisible (*k);
    }
    for (auto* k : { &exportKey, &importKey }) addAndMakeVisible (*k);
    exportKey.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Exportar o SHOW", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("show-riddim-machine.mrmshow"), "*.mrmshow");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f != juce::File()) message = proc.exportShow (f.withFileExtension ("mrmshow")) ? "SHOW exportado: " + f.getFileName() : u8 ("N\xc3\xa3o consegui salvar o arquivo");
                              });
    };
    importKey.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Importar um SHOW", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.mrmshow");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f.existsAsFile()) message = proc.importShow (f) ? "SHOW importado: " + f.getFileName() : u8 ("Arquivo de SHOW inv\xc3\xa1lido");
                              });
    };
    setSize (kW, kH);
    startTimerHz (10);
}

ShowPage::~ShowPage() { stopTimer(); }

void ShowPage::askName (int slot, bool rename)
{
    const auto cur = proc.getShowSlot (slot);
    auto* w = new juce::AlertWindow ("SHOW", rename ? u8 ("Novo nome do riddim no lugar ") + juce::String (slot + 1)
                                                    : u8 ("Nome do riddim no lugar ") + juce::String (slot + 1) + (cur.used ? u8 (" (substitui o que est\xc3\xa1 l\xc3\xa1)") : juce::String()),
                                     juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", cur.used ? cur.name : "Riddim " + juce::String (slot + 1));
    w->addButton (rename ? "Renomear" : "Salvar", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w, slot, rename] (int r)
    {
        const auto name = w->getTextEditorContents ("name").trim().substring (0, 40);
        if (r == 1)
        {
            if (rename) proc.renameShowSlot (slot, name);
            else { proc.saveShowSlot (slot, name); message = u8 ("Salvo no lugar ") + juce::String (slot + 1); }
        }
        repaint();
    }), true);
}

void ShowPage::resized()
{
    exportKey.setBounds (760, 44, 210, 40);
    importKey.setBounds (982, 44, 210, 40);
    for (int i = 0; i < MrmProcessor::numShowSlots; ++i)
    {
        auto& c = cards[(size_t) i];
        const int col_ = i / 5, row = i % 5;
        const int x = 70 + col_ * 576, y = 104 + row * 178;
        c.area = { x, y, 564, 166 };
        c.play->setBounds (x + 14, y + 104, 200, 48);
        c.save->setBounds (x + 222, y + 104, 128, 48);
        c.rename->setBounds (x + 356, y + 104, 98, 48);
        c.del->setBounds (x + 460, y + 104, 92, 48);
    }
    renderBackground ({ { showSection, u8 ("SHOW  \xc2\xb7  10 RIDDIMS SALVOS PARA TOCAR AO VIVO (FICAM NO PROJETO)") } });
}

void ShowPage::paint (juce::Graphics& g)
{
    PageBase::paint (g);
    for (int i = 0; i < MrmProcessor::numShowSlots; ++i)
    {
        const auto& c = cards[(size_t) i];
        const auto s = proc.getShowSlot (i);
        const bool cur = i == proc.getShowCurrent() && s.used;
        auto ar = c.area.toFloat();
        g.setColour (juce::Colour (0xff120c07));
        g.fillRoundedRectangle (ar, 8.0f);
        g.setColour (cur ? col::amber : s.used ? col::amber.withAlpha (0.45f) : juce::Colour (0xff3a2d22));
        g.drawRoundedRectangle (ar.reduced (1.0f), 8.0f, cur ? 3.0f : 1.5f);
        g.setColour (col::amber);
        g.setFont (stencil (40.0f, 0.0f));
        g.drawText (juce::String (i + 1), ar.withWidth (70.0f).withHeight (96.0f), juce::Justification::centred);
        g.setColour (s.used ? col::cream : col::cream.withAlpha (0.35f));
        g.setFont (stencil (20.0f, 0.04f));
        g.drawFittedText (s.used ? s.name : "vazio", juce::Rectangle<int> ((int) ar.getX() + 76, (int) ar.getY() + 18, 470, 30), juce::Justification::centredLeft, 1, 0.7f);
        g.setColour (col::cream.withAlpha (0.55f));
        g.setFont (stencil (12.0f, 0.06f));
        g.drawFittedText (s.used ? s.info : u8 ("Monte o riddim e toque em SALVAR AQUI"), juce::Rectangle<int> ((int) ar.getX() + 76, (int) ar.getY() + 54, 470, 34),
                          juce::Justification::topLeft, 2, 0.8f);
    }
    g.setColour (col::ink);
    g.setFont (stencil (14.0f, 0.06f));
    g.drawFittedText (message.isNotEmpty() ? message : u8 ("Salve at\xc3\xa9 10 riddims (harmonia, BPM, m\xc3\xbasicos, edi\xc3\xa7\xc3\xb5" "es, mixer, efeitos e arranjo). No show, \xe2\x96\xb6 TOCAR carrega e toca."),
                      juce::Rectangle<int> (70, 48, 680, 32), juce::Justification::centredLeft, 2, 0.8f);
}

void ShowPage::timerCallback()
{
    int stamp = proc.getShowCurrent() + 1;
    for (int i = 0; i < MrmProcessor::numShowSlots; ++i)
    {
        const auto s = proc.getShowSlot (i);
        stamp = stamp * 31 + (s.used ? s.name.hashCode() : 7);
        auto& c = cards[(size_t) i];
        c.play->setEnabled (s.used); c.rename->setEnabled (s.used); c.del->setEnabled (s.used);
        c.save->setButtonText (s.used ? "SALVAR POR CIMA" : "SALVAR AQUI");
    }
    stamp = stamp * 31 + message.hashCode();
    if (stamp != lastStamp) { lastStamp = stamp; repaint(); }
}
} // namespace mrmui
