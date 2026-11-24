#include "PluginEditor.h"
#include "SkinTextures.h"
#include <BinaryData.h>

using namespace juce;
using namespace vintage;

namespace
{
    const Colour fillOrange { 0xffd78a2a }, grooveGreen { 0xff3fae5e }, gridBg { 0xff17120e }, gold { 0xffe2b956 };

    String u8s (const char* s) { return String::fromUTF8 (s); }
}

// =====================================================================  grade
Rectangle<float> FillGrid::cell (int row, int col) const
{
    const float cw = (float) (getWidth() - labelW) / (float) cols;
    const float rh = (float) (getHeight() - headerH) / (float) mif::NumPieces;
    return { (float) labelW + col * cw, (float) headerH + row * rh, cw, rh };
}

void FillGrid::paint (Graphics& g)
{
    const auto st = proc.settings();
    const auto fill = proc.currentFill();
    const auto gb = mif::grooveBar (st.style);
    const int rs = mif::regionStart (st.size);
    const double ph = proc.playStep.load();
    const int headCol = ph < 0 ? -1 : (int) std::floor (ph) - (st.size == 3 ? 16 : 0);

    g.setColour (gridBg);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);

    // faixas: VIRADA (laranja) e GROOVE (verde)
    const auto c0 = cell (0, 0), c16 = cell (0, 16), c23 = cell (0, 23);
    g.setColour (fillOrange.withAlpha (0.35f));
    g.fillRect (Rectangle<float> (c0.getX(), 2.0f, c16.getX() - c0.getX() - 1.0f, 14.0f));
    g.setColour (grooveGreen.withAlpha (0.3f));
    g.fillRect (Rectangle<float> (c16.getX(), 2.0f, c23.getRight() - c16.getX(), 14.0f));
    g.setFont (VintageLookAndFeel::mono (10.0f));
    g.setColour (Colour (0xfff2a83a));
    g.drawText (u8s ("VIRADA \xc2\xb7 ") + String (mif::sizeName (st.size)).toUpperCase(), Rectangle<float> (c0.getX() + 6, 2.0f, 300, 14), Justification::centredLeft);
    g.setColour (Colour (0xff6fd08a));
    g.drawText (u8s ("GROOVE \xc2\xb7 ") + String (mif::styleName (st.style)).toUpperCase(), Rectangle<float> (c16.getX() + 6, 2.0f, 260, 14), Justification::centredLeft);

    // contagem: 1 i e a 2 i e a ... (a caixa no 2 e no 4)
    static const char* lab[4] = { "", "i", "e", "a" };
    for (int i = 0; i < cols; ++i)
    {
        const auto c = cell (0, i);
        const bool beat = i % 4 == 0, snare = (i % 16 == 4 || i % 16 == 12);
        g.setColour (snare ? Colour (0xffff8a5c) : (beat ? ivory : plasticMuted));
        g.setFont (VintageLookAndFeel::mono (beat ? 11.0f : 9.5f));
        g.drawText (beat ? String (((i / 4) % 4) + 1) : String (lab[i % 4]), Rectangle<float> (c.getX(), 17.0f, c.getWidth(), 15.0f), Justification::centred);
    }

    const auto& rws = mif::rows();
    for (int r = 0; r < mif::NumPieces; ++r)
    {
        const int p = rws[(size_t) r];
        const auto rc = cell (r, 0);
        g.setColour (ivoryDark);
        g.setFont (VintageLookAndFeel::mono (10.0f));
        g.drawText (mif::piece (p).name, Rectangle<float> (4.0f, rc.getY(), (float) labelW - 8.0f, rc.getHeight()), Justification::centredRight);
        for (int i = 0; i < cols; ++i)
        {
            auto c = cell (r, i).reduced (1.5f);
            Colour col = Colour (0xff2a221b);
            if (i % 4 == 0) col = Colour (0xff332920);
            bool on = false; int v = 0;
            if (i <= 16)
            {
                for (auto& x : fill) if (x.p == p && (int) std::lround (x.s) == i) v = std::max (v, x.v);
                on = v > 0 && ! (i == 16 && st.land == 1);
                if (on) col = fillOrange.withAlpha (0.45f + 0.18f * (float) v);
                if (i < rs && st.size != 3) col = col.withMultipliedAlpha (0.35f);
            }
            if (i >= 16)
            {
                const bool g0 = std::any_of (gb.begin(), gb.end(), [&] (const mif::Ev& x) { return x.p == p && (int) x.s == i - 16; });
                if (g0 && ! (i == 16 && on && st.land == 0)) { col = grooveGreen.withAlpha (0.75f); on = true; }
            }
            if (i == headCol) col = col.brighter (0.5f);
            g.setColour (col);
            g.fillRoundedRectangle (c, 2.0f);
            if (i == 16) { g.setColour (gold.withAlpha (0.8f)); g.drawRoundedRectangle (c, 2.0f, 1.0f); }
        }
    }
    // linha da chegada
    g.setColour (gold);
    g.fillRect (Rectangle<float> (c16.getX() - 1.5f, (float) headerH, 2.0f, (float) (getHeight() - headerH)));
}

void FillGrid::mouseDown (const MouseEvent& e)
{
    const auto pt = e.position;
    if (pt.y < headerH) return;
    const int row = (int) ((pt.y - headerH) / ((float) (getHeight() - headerH) / mif::NumPieces));
    if (row < 0 || row >= mif::NumPieces) return;
    const int p = mif::rows()[(size_t) row];
    if (pt.x < labelW) { proc.audition (p, 100); return; }
    const int col = (int) ((pt.x - labelW) / ((float) (getWidth() - labelW) / cols));
    if (col < 0 || col > 16) { proc.audition (p, 100); return; }   // o groove é fixo (só ouvir)
    proc.toggleCell (p, col);
    repaint();
}

// =====================================================================  MIDI
File FillMidiChip::makeFile (const File& dir) const
{
    auto f = dir.getChildFile (proc.midiFileName());
    proc.writeMidiFile (f);
    return f;
}

void FillMidiChip::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colours::black);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (dragging ? keyOrange : console);
    g.fillRoundedRectangle (r.reduced (2.0f), 4.0f);
    g.setColour (fillOrange);
    g.fillRect (r.reduced (2.0f).removeFromLeft (6.0f));
    g.setColour (dragging ? console : ivory);
    g.setFont (VintageLookAndFeel::mono (12.0f));
    auto text = r.reduced (10.0f, 6.0f).withTrimmedLeft (6.0f);
    g.drawText (u8s ("\xe2\x87\xa3 ARRASTE O MIDI PARA A DAW"), text.removeFromTop (text.getHeight() * 0.55f), Justification::centred);
    g.setFont (VintageLookAndFeel::mono (9.5f, false));
    g.setColour (ivoryDark);
    g.drawText (u8s ("ou clique para salvar o .mid \xc2\xb7 notas do MRD-12, canal 10"), text, Justification::centred);
}

void FillMidiChip::mouseDrag (const MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true; repaint();
    auto f = makeFile (File::getSpecialLocation (File::tempDirectory));
    if (f.existsAsFile())
        DragAndDropContainer::performExternalDragDropOfFiles (StringArray (f.getFullPathName()), false, this,
                                                              [this] { dragging = false; repaint(); });
    else { dragging = false; repaint(); }
}

void FillMidiChip::mouseUp (const MouseEvent& e)
{
    if (e.getDistanceFromDragStart() >= 6) { dragging = false; repaint(); return; }
    auto f = makeFile (File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Mastor Intro Fills"));
    if (onSaved) onSaved (f.existsAsFile() ? u8s ("MIDI SALVO EM DOCUMENTOS/MASTOR INTRO FILLS") : u8s ("N\xc3\x83O CONSEGUI SALVAR O MIDI"));
}

// =====================================================================  editor
IntroFillsEditor::IntroFillsEditor (IntroFillsProcessor& p)
    : AudioProcessorEditor (&p), proc (p), grid (p), midiChip (p)
{
    plateFace = Typeface::createSystemTypefaceFor (BinaryData::ArchivoBlackRegular_ttf, (size_t) BinaryData::ArchivoBlackRegular_ttfSize);
    titleFace = Typeface::createSystemTypefaceFor (BinaryData::RootsKeysDisplayBoldItalic_ttf, (size_t) BinaryData::RootsKeysDisplayBoldItalic_ttfSize);
    lnf.setSkin (1);
    setLookAndFeel (&lnf);

    const auto& ds = mif::drummers();
    for (int i = 0; i < 8; ++i)
    {
        auto& b = drummerKeys[i];
        if (i >= (int) ds.size()) continue;
        b.setButtonText (String (ds[(size_t) i].label).replace ("\n", " "));
        b.getProperties().set ("style", "reverseKey");
        b.setClickingTogglesState (false);
        b.onClick = [this, i] { proc.selectDrummer (i); };
        addAndMakeVisible (b);
    }
    for (auto* b : { &prevFill, &nextFill, &resetBtn })
    {
        b->getProperties().set ("style", "chip");
        addAndMakeVisible (*b);
    }
    prevFill.setButtonText (u8s ("\xe2\x97\x80")); nextFill.setButtonText (u8s ("\xe2\x96\xb6"));
    prevFill.onClick = [this] { proc.stepFill (-1); };
    nextFill.onClick = [this] { proc.stepFill (1); };
    resetBtn.onClick = [this] { proc.resetFill(); grid.repaint(); };

    for (int i = 0; i < mif::NumStyles; ++i)
    {
        setupChip (styleChips[i], String (mif::styleName (i)).toUpperCase());
        styleChips[i].onClick = [this, i] { proc.setParam ("style", (float) i); };
    }
    for (int i = 0; i < mif::NumSizes; ++i)
    {
        setupChip (sizeChips[i], String (mif::sizeName (i)).toUpperCase());
        sizeChips[i].onClick = [this, i] { proc.setParam ("size", (float) i); };
    }
    setupChip (landChips[0], "COM O ESTALO");
    setupChip (landChips[1], "SEM O ESTALO");
    for (int i = 0; i < 2; ++i) landChips[i].onClick = [this, i] { proc.setParam ("land", (float) i); };

    setupToggle (playBtn, u8s ("\xe2\x96\xb6 PREVIEW"), "play", playAtt);
    setupToggle (loopBtn, "LOOP", "loop", loopAtt);
    setupToggle (echoBtn, "TAPE ECHO", "echoOn", echoAtt);
    setupToggle (satBtn, "SATURADOR", "satOn", satAtt);

    setupKnob (kInt, "intensity", "INTENSIDADE");
    setupKnob (kHum, "human", "HUMANIZAR");
    setupKnob (kSwing, "swing", "SWING");
    setupKnob (kGhost, "ghost", "GHOST");
    setupKnob (kVol, "volume", "VOLUME");
    setupKnob (kBpm, "bpm", "BPM");

    addAndMakeVisible (grid);
    addAndMakeVisible (midiChip);
    midiChip.setTooltip (u8s ("Arraste a virada + 2 compassos de groove para uma trilha da DAW (notas do Mastor Reggae Drums MRD-12)"));
    midiChip.onSaved = [this] (const String& s) { status = s; repaint(); };

    setSize (kW, kH);
    refresh();
    startTimerHz (30);
}

IntroFillsEditor::~IntroFillsEditor() { stopTimer(); setLookAndFeel (nullptr); }

void IntroFillsEditor::setupKnob (Knob& k, const char* id, const String& label)
{
    k.label = label;
    k.s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    k.s.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (k.s);
    k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.s);
}

void IntroFillsEditor::setupChip (TextButton& b, const String& text)
{
    b.setButtonText (text);
    b.getProperties().set ("style", "chip");
    addAndMakeVisible (b);
}

void IntroFillsEditor::setupToggle (TextButton& b, const String& text, const char* id,
                                    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment>& att)
{
    setupChip (b, text);
    b.setClickingTogglesState (true);
    att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, b);
}

void IntroFillsEditor::refresh()
{
    const int d = proc.drummer();
    for (int i = 0; i < (int) mif::drummers().size(); ++i) drummerKeys[i].setToggleState (i == d, dontSendNotification);
    const int style = (int) proc.get ("style"), size = (int) proc.get ("size"), land = (int) proc.get ("land");
    for (int i = 0; i < mif::NumStyles; ++i) styleChips[i].setToggleState (i == style, dontSendNotification);
    for (int i = 0; i < mif::NumSizes; ++i) sizeChips[i].setToggleState (i == size, dontSendNotification);
    for (int i = 0; i < 2; ++i) landChips[i].setToggleState (i == land, dontSendNotification);
    resetBtn.setVisible (proc.isEdited());
    const bool many = mif::drummers()[(size_t) d].fills.size() > 1;
    prevFill.setEnabled (many); nextFill.setEnabled (many);
}

void IntroFillsEditor::timerCallback()
{
    refresh();
    grid.repaint();
    repaint (24, 150, 640, 56);   // visor da virada
}

void IntroFillsEditor::renderBackground (Graphics& g)
{
    static const Image wood = skin::makeWood (kW, kH);
    g.drawImageAt (wood, 0, 0);
    g.setColour (Colours::black.withAlpha (0.18f));
    g.fillRect (0, 0, kW, 74);
    g.setColour (Colour (0xff8e2a20));
    g.fillRect (0, 74, kW, 3);

    // título
    g.setColour (ivory);
    g.setFont (FontOptions (titleFace).withHeight (15.0f));
    g.drawText ("DubMastor", 26, 8, 200, 18, Justification::centredLeft);
    g.setFont (FontOptions (titleFace).withHeight (34.0f));
    g.drawText ("Intro Fills", 24, 22, 260, 44, Justification::centredLeft);
    g.setFont (VintageLookAndFeel::mono (11.0f));
    g.setColour (ivoryDark);
    g.drawText ("MIF  " + String (JucePlugin_VersionString).upToLastOccurrenceOf (".", false, false), 222, 30, 120, 14, Justification::centredLeft);
    g.drawText (u8s ("VIRADAS DE INTRODU\xc3\x87\xc3\x83O \xc2\xb7 BATERISTAS JAMAICANOS"), 222, 46, 330, 14, Justification::centredLeft);

    // plaqueta DUB STATION
    {
        Rectangle<float> pl (420, 18, 170, 38);
        g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (pl.translated (0, 2), 6.0f);
        g.setGradientFill (ColourGradient::vertical (Colour (0xfff3d27a), pl.getY(), Colour (0xffb98a2c), pl.getBottom()));
        g.fillRoundedRectangle (pl, 6.0f);
        g.setColour (Colour (0xff3e2110));
        g.setFont (FontOptions (plateFace).withHeight (16.0f));
        g.drawText ("DUB STATION", pl, Justification::centred);
    }

    // painéis
    auto panel = [&] (Rectangle<int> r, const String& title)
    {
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r.toFloat().translated (0, 2), 6.0f);
        g.setColour (console.withAlpha (0.92f));
        g.fillRoundedRectangle (r.toFloat(), 6.0f);
        if (title.isNotEmpty())
        {
            g.setColour (ivoryDark);
            g.setFont (VintageLookAndFeel::mono (10.0f));
            g.drawText (title, r.getX() + 10, r.getY() + 4, 300, 12, Justification::centredLeft);
        }
    };
    panel ({ 16, 84, 968, 74 }, "BATERISTAS");
    panel ({ 16, 164, 968, 106 }, "");
    panel ({ 16, 278, 968, 290 }, "");
    panel ({ 16, 576, 968, 106 }, "");

    g.setFont (VintageLookAndFeel::mono (10.0f));
    g.setColour (ivoryDark);
    g.drawText ("GROOVE", 28, 218, 70, 22, Justification::centredLeft);
    g.drawText ("TAMANHO", 28, 244, 70, 22, Justification::centredLeft);
    g.drawText ("CHEGADA", 610, 218, 70, 22, Justification::centredLeft);
}

void IntroFillsEditor::paint (Graphics& g)
{
    if (background.isNull())
    {
        background = Image (Image::RGB, kW, kH, true);
        Graphics bg (background);
        renderBackground (bg);
    }
    g.drawImageAt (background, 0, 0);

    // visor da virada (LCD âmbar)
    const auto& dr = mif::drummers()[(size_t) proc.drummer()];
    Rectangle<float> lcd (28, 172, 600, 40);
    g.setColour (Colour (0xff0d0b09)); g.fillRoundedRectangle (lcd, 4.0f);
    g.setColour (Colour (0xff1c140c)); g.fillRoundedRectangle (lcd.reduced (2), 3.0f);
    g.setColour (Colour (0xffffb347));
    g.setFont (VintageLookAndFeel::mono (16.0f));
    g.drawText (String (dr.name).toUpperCase() + "  -  " + String (dr.fills[(size_t) proc.fill()].name).toUpperCase() + (proc.isEdited() ? "  (EDITADA)" : ""),
                lcd.reduced (12, 0).withTrimmedBottom (14), Justification::centredLeft);
    g.setColour (Colour (0xffc98a3a));
    g.setFont (VintageLookAndFeel::mono (10.0f));
    g.drawText (u8s ("VARIA\xc3\x87\xc3\x83O ") + String (proc.fill() + 1) + "/" + String ((int) dr.fills.size()) + u8s (" \xc2\xb7 ") + dr.tag,
                lcd.reduced (12, 0).withTrimmedTop (22), Justification::centredLeft);

    // rótulos dos knobs
    g.setFont (VintageLookAndFeel::mono (10.0f));
    g.setColour (ivoryDark);
    for (auto* k : { &kInt, &kHum, &kSwing, &kGhost, &kVol })
        g.drawText (k->label, k->s.getBounds().getCentreX() - 50, k->s.getBottom() + 2, 100, 12, Justification::centred);
    g.setColour (ivory);
    g.drawText ("BPM " + String (roundToInt (proc.get ("bpm"))), kBpm.s.getBounds().getCentreX() - 40, kBpm.s.getBottom(), 80, 12, Justification::centred);
    if (status.isNotEmpty())
    {
        g.setColour (Colour (0xffffb347));
        g.drawText (status, midiChip.getX(), midiChip.getBottom() + 4, midiChip.getWidth(), 14, Justification::centred);
    }
}

void IntroFillsEditor::resized()
{
    const int nd = (int) mif::drummers().size();
    const int kw = (968 - 20 - (nd - 1) * 8) / nd;
    for (int i = 0; i < nd; ++i) drummerKeys[i].setBounds (26 + i * (kw + 8), 102, kw, 46);

    prevFill.setBounds (640, 172, 40, 40);
    nextFill.setBounds (686, 172, 40, 40);
    resetBtn.setBounds (736, 178, 170, 28);

    for (int i = 0; i < mif::NumStyles; ++i) styleChips[i].setBounds (100 + i * 100, 218, 96, 22);
    for (int i = 0; i < mif::NumSizes; ++i) sizeChips[i].setBounds (100 + i * 125, 244, 121, 22);
    for (int i = 0; i < 2; ++i) landChips[i].setBounds (690 + i * 140, 218, 136, 22);
    loopBtn.setBounds (690, 244, 136, 22);

    playBtn.setBounds (620, 22, 130, 32);
    kBpm.s.setBounds (770, 10, 46, 46);

    grid.setBounds (24, 284, 952, 278);

    const int ky = 590, ks = 52;
    Knob* ks5[5] = { &kInt, &kHum, &kSwing, &kGhost, &kVol };
    for (int i = 0; i < 5; ++i) ks5[i]->s.setBounds (40 + i * 92, ky, ks, ks);
    echoBtn.setBounds (500, 596, 110, 26);
    satBtn.setBounds (500, 630, 110, 26);
    midiChip.setBounds (640, 592, 330, 56);
}
