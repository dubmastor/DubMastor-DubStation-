#include "PluginEditor.h"
#include "BinaryData.h"
#include "../../RiddimMachine/Source/MrmStyle.h"

using namespace mrmui;

namespace
{
const char* const kNotes[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };

juce::Font script (float h)
{
    static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor (BinaryData::KaushanScriptRegular_ttf,
                                                                             (size_t) BinaryData::KaushanScriptRegular_ttfSize);
    return juce::Font (juce::FontOptions (tf).withHeight (h));
}

void darkGlass (juce::Graphics& g, juce::Rectangle<float> r)
{
    displayGlass (g, r, 4.0f, 0.12f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

uint16_t triad (int r, bool m) { return mt::triadMask (r, m); }
} // namespace

namespace mksui
{
Panel::Panel (KeyScoutProcessor& p) : proc (p)
{
    setSize (kW, kH);
    secKey    = { 50, 104, 380, 196 };
    secChroma = { 446, 104, 384, 196 };
    secLink   = { 50, 322, 780, 184 };
    keyGlass    = { 64.0f, 124.0f, 352.0f, 64.0f };
    chordGlass  = { 64.0f, 198.0f, 352.0f, 46.0f };
    keyHold     = { 64.0f, 252.0f, 172.0f, 38.0f };
    keyRelearn  = { 244.0f, 252.0f, 172.0f, 38.0f };
    chromaGlass = { 460.0f, 124.0f, 356.0f, 164.0f };
    linkGlass   = { 64.0f, 342.0f, 470.0f, 46.0f };
    keyListen   = { 546.0f, 340.0f, 270.0f, 50.0f };
    pianoR      = { 64.0f, 398.0f, 752.0f, 64.0f };
    infoBox     = { 64.0f, 470.0f, 752.0f, 30.0f };
    startTimerHz (30);
}

Panel::~Panel() { stopTimer(); }

void Panel::toggle (const char* id)
{
    if (auto* prm = proc.apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
        prm->endChangeGesture();
    }
}

void Panel::mouseDown (const juce::MouseEvent& e)
{
    pressed = keyHold.contains (e.position) ? 0 : keyRelearn.contains (e.position) ? 1 : keyListen.contains (e.position) ? 2 : -1;
    repaint();
}

void Panel::mouseUp (const juce::MouseEvent& e)
{
    if (pressed == 0 && keyHold.contains (e.position)) toggle ("hold");
    if (pressed == 1 && keyRelearn.contains (e.position)) proc.relearn();
    if (pressed == 2 && keyListen.contains (e.position)) toggle ("listen");
    pressed = -1;
    repaint();
}

void Panel::timerCallback()
{
    st = proc.getStatus();
    anim += 1.0f / 30.0f;
    for (int i = 0; i < 12; ++i)
    {
        const float t = st.signal ? st.chroma[(size_t) i] : 0.0f;
        bars[(size_t) i] += (t - bars[(size_t) i]) * (t > bars[(size_t) i] ? 0.5f : 0.12f);
    }
    repaint();
}

void Panel::renderBackground()
{
    const float scale = 2.0f;
    background = juce::Image (juce::Image::ARGB, (int) (kW * scale), (int) (kH * scale), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    juce::Random rng (420);
    paintWood (g, { 0, 0, 26, kH }, rng);
    paintWood (g, { kW - 26, 0, 26, kH }, rng);
    const juce::Rectangle<int> plate (26, 0, kW - 52, kH);
    paintPlate (g, plate, rng, false, true);
    {
        const auto band = plate.withHeight (82).toFloat();
        const juce::Colour bc (0xff3d5a55);
        g.setColour (bc); g.fillRect (band);
        for (int i = 0; i < 700; ++i)
        {
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.03f + rng.nextFloat() * 0.05f));
            g.fillRect (band.getX() + rng.nextFloat() * band.getWidth(), band.getY() + rng.nextFloat() * band.getHeight(), 1.0f + rng.nextFloat() * 2.0f, 1.0f);
        }
        g.setColour (bc.darker (0.6f)); g.fillRect (band.getX(), band.getBottom() - 3.0f, band.getWidth(), 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f)); g.fillRect (band.getX(), band.getBottom(), band.getWidth(), 1.0f);
    }
    for (auto [x, y] : { std::pair<float, float> { 38.0f, 94.0f }, { kW - 38.0f, 94.0f }, { 38.0f, kH - 12.0f }, { kW - 38.0f, kH - 12.0f } })
        screw (g, x, y, rng);

    juce::String ver (JucePlugin_VersionString);
    if (ver.containsChar ('.') && ver.fromFirstOccurrenceOf (".", false, false).containsChar ('.'))
        ver = ver.upToLastOccurrenceOf (".", false, false);
    const juce::String title ("MASTOR KEY SCOUT " + ver);
    g.setFont (stencil (24.0f, 0.2f));
    const float titleW = textWidth (g.getCurrentFont(), title);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawText (title, juce::Rectangle<float> (56.0f, 16.0f, titleW + 10.0f, 36.0f).translated (0.8f, 1.0f), juce::Justification::left);
    g.setColour (juce::Colour (0xfff3e6c8));
    g.drawText (title, juce::Rectangle<float> (56.0f, 16.0f, titleW + 10.0f, 36.0f), juce::Justification::left);
    engraved (g, u8 ("ESCUTA O RIDDIM  \xc2\xb7  MANDA O TOM AO MASTOR TUNE"),
              { 58.0f, 52.0f, 520.0f, 13.0f }, juce::Justification::left, 9.0f, 0.22f, juce::Colour (0xfff3e6c8).withAlpha (0.75f));
    {
        g.setFont (stencil (18.0f, 0.3f));
        const float tw = textWidth (g.getCurrentFont(), "MKS-0420");
        brassPlate (g, { 56.0f + titleW + 20.0f, 18.0f, tw + 72.0f, 40.0f }, "MKS-0420", 18.0f, rng);
    }
    {
        const float sw = 130.0f, sx = (float) plate.getRight() - 26.0f - sw, sh = 5.0f;
        const juce::Colour faded[] = { col::jamGreen.interpolatedWith (juce::Colour (0xff6f7a4a), 0.35f),
                                       col::jamGold.interpolatedWith (juce::Colour (0xffb08a3a), 0.40f), juce::Colour (0xff2b2621) };
        for (int k = 0; k < 3; ++k)
        {
            const float y = 20.0f + 7.0f * (float) k;
            juce::Path band;
            band.startNewSubPath (sx, y);
            for (float x = sx; x <= sx + sw; x += 6.0f) band.lineTo (x, y + (rng.nextFloat() - 0.3f) * 0.9f);
            for (float x = sx + sw; x >= sx; x -= 6.0f) band.lineTo (x, y + sh + (rng.nextFloat() - 0.7f) * 0.9f);
            band.closeSubPath();
            g.setColour (faded[k].withAlpha (0.85f));
            g.fillPath (band);
        }
        engraved (g, "DUBSTATION", { sx - 20.0f, 44.0f, sw + 20.0f, 20.0f }, juce::Justification::centredRight, 15.0f, 0.38f, juce::Colour (0xfff3e6c8));
    }
    section (g, secKey, u8 ("TOM DO RIDDIM  \xc2\xb7  ACORDE AGORA"));
    section (g, secChroma, u8 ("NOTAS QUE EST\xc3\x83O SOANDO"));
    section (g, secLink, u8 ("LINK  \xc2\xb7  MASTOR TUNE MT-0420"));
    engraved (g, u8 ("v") + ver + u8 ("  \xc2\xb7  GR\xc3\x81TIS  \xc2\xb7  O \xc3\x81UDIO PASSA SEM MUDAN\xc3\x87" "A  \xc2\xb7  LAT\xc3\x8ANCIA ZERO"),
              { 58.0f, 522.0f, 560.0f, 14.0f }, juce::Justification::centredLeft, 8.5f, 0.18f);
    g.setFont (script (30.0f));
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText ("DubMastor", juce::Rectangle<float> (620.0f, 510.0f, 196.0f, 40.0f).translated (1.0f, 1.5f), juce::Justification::centredRight);
    g.setColour (col::ink);
    g.drawText ("DubMastor", juce::Rectangle<float> (620.0f, 510.0f, 196.0f, 40.0f), juce::Justification::centredRight);
    g.setColour (col::jamGold);
    g.drawLine (690.0f, 546.0f, 816.0f, 543.0f, 2.5f);
}

void Panel::drawKey (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, bool on, bool down, bool green)
{
    r = r.reduced (2.0f);
    if (green)
    {
        g.setColour (juce::Colour (0xff1a1714)); g.fillRoundedRectangle (r, 6.0f);
        if (on)
        {
            const float ph = std::fmod (anim * 1.25f, 1.0f);
            g.setColour (juce::Colour (0xff5cff7a).withAlpha (0.35f * (1.0f - ph)));
            g.drawRoundedRectangle (r.expanded (2.0f + 5.0f * ph), 8.0f, 2.0f);
        }
        auto cap = r.reduced (4.0f).translated (0, down ? 1.5f : 0.0f);
        juce::ColourGradient cg (col::jamGreen, cap.getX(), cap.getY(), col::jamGreen.darker (0.75f), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (cg); g.fillRoundedRectangle (cap, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.2f)); g.fillRoundedRectangle (cap.withHeight (cap.getHeight() * 0.38f).reduced (4, 2), 3.0f);
        lamp (g, juce::Rectangle<float> (12, 12).withCentre ({ cap.getX() + 18.0f, cap.getCentreY() }), on, juce::Colour (0xff5cff7a));
        g.setColour (col::jamGold); g.setFont (stencil (14.0f, 0.14f));
        g.drawText (text, cap.withTrimmedLeft (30.0f), juce::Justification::centred);
        return;
    }
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r.translated (0, 2.5f), 4.0f);
    auto cap = r.translated (0, (on || down) ? 1.5f : 0.0f);
    juce::ColourGradient cg (juce::Colour (on ? 0xff2c2925 : 0xff3a3632), cap.getX(), cap.getY(), juce::Colour (0xff141210), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (cg); g.fillRoundedRectangle (cap, 4.0f);
    if (on)
    {
        g.setColour (col::amber.withAlpha (0.8f));
        g.fillRoundedRectangle (cap.withTop (cap.getBottom() - 3.0f).reduced (cap.getWidth() * 0.12f, 0.0f).translated (0, -2.0f), 1.5f);
    }
    lamp (g, juce::Rectangle<float> (9, 9).withCentre ({ cap.getX() + 14.0f, cap.getCentreY() }), on);
    g.setColour (on ? col::amber : col::cream.withAlpha (0.66f)); g.setFont (stencil (11.5f, 0.12f));
    g.drawText (text, cap.withTrimmedLeft (22.0f), juce::Justification::centred);
}

void Panel::paint (juce::Graphics& g)
{
    if (! background.isValid()) renderBackground();
    g.drawImage (background, getLocalBounds().toFloat());

    // tom
    darkGlass (g, keyGlass);
    g.setFont (stencil (30.0f, 0.2f));
    if (st.learned && st.keyRoot >= 0)
        glowText (g, juce::String (kNotes[st.keyRoot]) + (st.keyMinor ? " MENOR" : " MAIOR"), keyGlass, juce::Justification::centred);
    else
    {
        g.setFont (stencil (14.0f, 0.16f));
        glowText (g, st.signal ? u8 ("APRENDENDO O TOM\xe2\x80\xa6") : u8 ("D\xc3\x8a PLAY NO RIDDIM"), keyGlass, juce::Justification::centred, col::amber, 0.7f);
    }
    if (st.hold)
    {
        g.setFont (stencil (9.0f, 0.2f));
        glowText (g, "TRAVADO", keyGlass.reduced (8.0f, 6.0f), juce::Justification::topRight, juce::Colour (0xffff7a5c));
    }
    darkGlass (g, chordGlass);
    g.setFont (stencil (12.0f, 0.16f));
    glowText (g, "ACORDE", chordGlass.reduced (12.0f, 0), juce::Justification::centredLeft, col::amber, 0.6f);
    g.setFont (stencil (22.0f, 0.12f));
    glowText (g, st.signal && st.chordRoot >= 0 ? juce::String (kNotes[st.chordRoot]) + (st.chordMinor ? "m" : "") : juce::String ("--"),
              chordGlass.reduced (14.0f, 0), juce::Justification::centredRight);
    drawKey (g, keyHold, "TRAVAR TOM", st.hold, pressed == 0);
    drawKey (g, keyRelearn, "APRENDER DE NOVO", false, pressed == 1);

    // cromagrama
    darkGlass (g, chromaGlass);
    const auto bars0 = chromaGlass.reduced (10.0f, 8.0f).withTrimmedBottom (16.0f);
    const float bw = bars0.getWidth() / 12.0f;
    const uint16_t chordM = st.signal && st.chordRoot >= 0 ? triad (st.chordRoot, st.chordMinor) : 0;
    for (int i = 0; i < 12; ++i)
    {
        auto b = juce::Rectangle<float> (bars0.getX() + bw * (float) i + 3.0f, bars0.getY(), bw - 6.0f, bars0.getHeight());
        g.setColour (juce::Colour (0xff1b2418)); g.fillRect (b);
        const float h = juce::jlimit (0.0f, 1.0f, bars[(size_t) i]) * b.getHeight();
        const bool inChord = (chordM >> i) & 1;
        juce::ColourGradient mg (inChord ? col::amber : juce::Colour (0xff3be070), b.getX(), b.getBottom() - h, juce::Colour (0xff1d3a1d), b.getX(), b.getBottom(), false);
        g.setGradientFill (mg); g.fillRect (b.withTop (b.getBottom() - h));
        g.setColour (juce::Colour (0xff0e0c0a));
        for (float y = b.getY(); y < b.getBottom(); y += 4.0f) g.fillRect (b.getX(), y, b.getWidth(), 1.5f);
        g.setColour (inChord ? col::amber : col::cream.withAlpha (0.6f));
        g.setFont (stencil (9.5f, 0.0f));
        g.drawText (kNotes[i], juce::Rectangle<float> (b.getX() - 4.0f, chromaGlass.getBottom() - 20.0f, b.getWidth() + 8.0f, 12.0f), juce::Justification::centred);
    }

    // link
    darkGlass (g, linkGlass);
    lamp (g, juce::Rectangle<float> (10, 10).withCentre ({ linkGlass.getX() + 16.0f, linkGlass.getCentreY() }), st.learned, juce::Colour (0xff5cff7a));
    g.setFont (stencil (12.0f, 0.14f));
    glowText (g, st.learned ? u8 ("PUBLICANDO  \xe2\x86\x92  MASTOR TUNE (SEM SIDECHAIN)") : u8 ("ESPERANDO O RIDDIM TOCAR"),
              linkGlass.withTrimmedLeft (32.0f), juce::Justification::centredLeft, col::amber, st.learned ? 1.0f : 0.6f);
    const bool listen = proc.apvts.getRawParameterValue ("listen")->load() > 0.5f;
    drawKey (g, keyListen, "ESCUTAR RIDDIM", listen, pressed == 2, true);

    // teclado: escala aprendida (creme) e acorde do momento (âmbar)
    const uint16_t scaleM = st.learned && st.keyRoot >= 0 ? mt::scaleMask (st.keyRoot, st.keyMinor ? mt::Menor : mt::Maior) : 0x0fff;
    const int octaves = 3, whites = 7 * octaves;
    const float ww = pianoR.getWidth() / (float) whites;
    static const int wh[7] = { 0, 2, 4, 5, 7, 9, 11 };
    static const std::pair<int, float> bl[5] = { { 1, 1.0f }, { 3, 2.0f }, { 6, 4.0f }, { 8, 5.0f }, { 10, 6.0f } };
    for (int o = 0; o < octaves; ++o)
        for (int i = 0; i < 7; ++i)
        {
            const int pc = wh[i];
            auto k = juce::Rectangle<float> (pianoR.getX() + ww * (float) (o * 7 + i), pianoR.getY(), ww, pianoR.getHeight()).reduced (0.6f, 0);
            const bool inS = (scaleM >> pc) & 1, inC = (chordM >> pc) & 1;
            juce::Colour top = inC ? juce::Colour (0xfff3d9a0) : inS ? juce::Colour (0xfff3ecd8) : juce::Colour (0xff9c9282);
            juce::Colour bot = inC ? juce::Colour (0xffe7b860) : inS ? juce::Colour (0xffdccfae) : juce::Colour (0xff7a7062);
            juce::ColourGradient kg (top, k.getX(), k.getY(), bot, k.getX(), k.getBottom(), false);
            g.setGradientFill (kg); g.fillRoundedRectangle (k, 2.0f);
            g.setColour (col::ink.withAlpha (0.5f)); g.drawRoundedRectangle (k, 2.0f, 0.8f);
            if (inC) { g.setColour (col::amber); g.fillEllipse (juce::Rectangle<float> (8, 8).withCentre ({ k.getCentreX(), k.getBottom() - 10.0f })); }
        }
    for (int o = 0; o < octaves; ++o)
        for (auto [pc, pos] : bl)
        {
            auto k = juce::Rectangle<float> (pianoR.getX() + ww * ((float) (o * 7) + pos) - ww * 0.3f, pianoR.getY(), ww * 0.6f, pianoR.getHeight() * 0.6f);
            const bool inS = (scaleM >> pc) & 1, inC = (chordM >> pc) & 1;
            juce::ColourGradient kg (inC ? juce::Colour (0xff7a4a10) : inS ? juce::Colour (0xff2a2520) : juce::Colour (0xff5a5148),
                                     k.getX(), k.getY(), inC ? juce::Colour (0xff3a2206) : juce::Colour (0xff0d0b09), k.getX(), k.getBottom(), false);
            g.setGradientFill (kg); g.fillRoundedRectangle (k, 2.0f);
        }

    // aviso
    g.setColour (col::ink);
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawFittedText (u8 ("Coloque no MASTER (ou no grupo da base). O Mastor Tune na faixa da voz recebe o tom e os acordes sozinho: n\xc3\xa3o precisa de sidechain. "
                          "Teclas apagadas = fora da escala."),
                      infoBox.toNearestInt(), juce::Justification::centredLeft, 2, 0.9f);
}
} // namespace mksui
