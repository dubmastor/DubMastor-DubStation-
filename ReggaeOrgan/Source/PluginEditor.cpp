#include "PluginEditor.h"
#include "SkinTextures.h"
#include "BinaryData.h"
#include "RiddimBankMenu.h"

using namespace vintage;

namespace
{
constexpr int kW = 1000, kH = 800;

inline juce::String u8 (const char* t) { return juce::String::fromUTF8 (t); }

const juce::Rectangle<int> kPlate   { 415, 16, 170, 40 };
const juce::Rectangle<int> kMus     { 24, 82, 952, 52 };
const juce::Rectangle<int> kContent { 24, 214, 952, 412 };
const juce::Rectangle<int> kGridPan { 24, 214, 640, 412 };
const juce::Rectangle<int> kSide    { 676, 214, 300, 412 };
const juce::Rectangle<int> kBubCon  { 24, 214, 278, 412 };
const juce::Rectangle<int> kChopCon { 310, 214, 446, 412 };   // drawbars + a Leslie da mão direita
const juce::Rectangle<int> kLeslie  { 590, 214, 166, 412 };   // dentro do console da mão direita
const juce::Rectangle<int> kSound   { 764, 214, 212, 412 };
constexpr int kDbPitch = 29;
const juce::Rectangle<int> kKeysBed { 24, 638, 952, 150 };

const juce::Colour lcdBlue   { 0xff0b2236 };
const juce::Colour lcdCyan   { 0xff8fe3ff };
const juce::Colour colBubble { 0xffe0a040 };   // mão esquerda
const juce::Colour colChop   { 0xffd9752e };   // mão direita
const juce::Colour colFill   { 0xfff3e6c4 };   // fills

juce::Colour layerColour (int l) { return l == mro::LayerBubble ? colBubble : (l == mro::LayerChop ? colChop : colFill); }

juce::Colour drawbarColour (int i)
{
    switch (i)
    {
        case 0: case 1: return juce::Colour (0xff6b3418);
        case 4: case 6: case 7: return juce::Colour (0xff141414);
        default: return ivory;
    }
}

juce::String versionText()
{
    auto parts = juce::StringArray::fromTokens (JucePlugin_VersionString, ".", "");
    return parts[0] + "." + (parts.size() > 1 ? parts[1] : juce::String ("0"));
}

juce::String yearsOf (const char* who)
{
    auto s = u8 (who);
    const int i = s.indexOf (u8 ("\xc2\xb7"));
    return i >= 0 ? s.substring (i + 1).trim() : s;
}
} // namespace

// =====================================================================  Drawbar
// ---------------------------------------------------------------- curva do EQ (1.3)
static const char* const kEqIds[4][2] = { { "eqHpf", nullptr }, { "eqLowF", "eqLowG" }, { "eqMidF", "eqMidG" }, { "eqHighF", "eqHighG" } };

float EqCurve::xOf (double f) const
{
    const auto r = plot();
    return r.getX() + r.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0));
}
double EqCurve::fOf (float x) const
{
    const auto r = plot();
    return 20.0 * std::pow (1000.0, juce::jlimit (0.0, 1.0, (double) ((x - r.getX()) / r.getWidth())));
}
float EqCurve::yOf (double db) const
{
    const auto r = plot();
    return r.getCentreY() - (float) (db / 18.0) * r.getHeight() * 0.5f;
}
double EqCurve::dbOf (float y) const
{
    const auto r = plot();
    return juce::jlimit (-15.0, 15.0, (double) ((r.getCentreY() - y) / (r.getHeight() * 0.5f) * 18.0f));
}
juce::Point<float> EqCurve::node (int k) const
{
    if (k == 0) return { xOf (proc.get ("eqHpf")), yOf (0.0) };
    return { xOf (proc.get (kEqIds[k][0])), yOf (proc.get (kEqIds[k][1])) };
}
int EqCurve::nodeAt (juce::Point<float> p) const
{
    int best = -1;
    float bd = 14.0f;
    for (int k = 0; k < 4; ++k)
    {
        const float d = node (k).getDistanceFrom (p);
        if (d < bd) { bd = d; best = k; }
    }
    return best;
}
void EqCurve::gesture (bool begin)
{
    if (dragNode < 0) return;
    for (auto* id : kEqIds[dragNode])
        if (id != nullptr)
            if (auto* prm = proc.apvts.getParameter (id)) { if (begin) prm->beginChangeGesture(); else prm->endChangeGesture(); }
}
void EqCurve::mouseDown (const juce::MouseEvent& e)
{
    dragNode = nodeAt (e.position);
    if (dragNode >= 0) { if (proc.get ("eqOn") < 0.5f) proc.setParam ("eqOn", 1.0f); gesture (true); }
}
void EqCurve::mouseDrag (const juce::MouseEvent& e)
{
    if (dragNode < 0) return;
    auto set = [this] (const char* id, float v)
    {
        if (auto* prm = proc.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };
    set (kEqIds[dragNode][0], (float) fOf (e.position.x));
    if (kEqIds[dragNode][1] != nullptr) set (kEqIds[dragNode][1], (float) dbOf (e.position.y));
    repaint();
}
void EqCurve::mouseUp (const juce::MouseEvent&) { gesture (false); dragNode = -1; }
void EqCurve::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int k = nodeAt (e.position);
    if (k == 0) proc.setParam ("eqHpf", 20.0f);
    else if (k > 0) proc.setParam (kEqIds[k][1], 0.0f);
    repaint();
}
void EqCurve::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (nodeAt (e.position) != 2) return;
    proc.setParam ("eqMidQ", juce::jlimit (0.3f, 4.0f, proc.get ("eqMidQ") * (w.deltaY > 0 ? 1.12f : 1.0f / 1.12f)));
    repaint();
}
void EqCurve::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (lcdBlue);
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
    const auto r = plot();
    g.setFont (VintageLookAndFeel::mono (8.5f, false));
    // grade: frequências e ganhos
    for (double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
    {
        const float x = xOf (f);
        g.setColour (lcdCyan.withAlpha (0.12f));
        g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
        g.setColour (lcdCyan.withAlpha (0.5f));
        const juce::String t = f >= 1000.0 ? juce::String ((int) (f / 1000.0)) + "k" : juce::String ((int) f);
        g.drawText (t, (int) x - 16, (int) r.getBottom() + 2, 32, 12, juce::Justification::centred);
    }
    for (int db : { -12, -6, 0, 6, 12 })
    {
        const float y = yOf (db);
        g.setColour (lcdCyan.withAlpha (db == 0 ? 0.3f : 0.12f));
        g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
        g.setColour (lcdCyan.withAlpha (0.5f));
        g.drawText ((db > 0 ? "+" : "") + juce::String (db), 0, (int) y - 6, 26, 12, juce::Justification::centredRight);
    }
    // a curva
    const mro::EqParams ep = proc.getEq();
    juce::Path curve, fill;
    for (int i = 0; i <= (int) r.getWidth(); i += 2)
    {
        const float x = r.getX() + (float) i;
        const float y = juce::jlimit (r.getY(), r.getBottom(), yOf (mro::OrganEQ::responseDb (ep, fOf (x)) - ep.out));   // a SAÍDA vai escrita no canto
        if (i == 0) { curve.startNewSubPath (x, y); fill.startNewSubPath (x, yOf (0)); }
        curve.lineTo (x, y);
        fill.lineTo (x, y);
    }
    fill.lineTo (r.getRight(), yOf (0));
    fill.closeSubPath();
    const juce::Colour c = ep.on ? juce::Colour (0xffffb347) : lcdCyan.withAlpha (0.4f);
    g.setColour (c.withAlpha (0.15f));
    g.fillPath (fill);
    g.setColour (c);
    g.strokePath (curve, juce::PathStrokeType (2.0f));
    // os pontos
    const char* names[4] = { "CORTE", "GRAVES", "M\xc3\x89" "DIOS", "AGUDOS" };
    for (int k = 0; k < 4; ++k)
    {
        const auto p = node (k);
        g.setColour (k == dragNode ? juce::Colours::white : c);
        g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (p));
        g.setColour (lcdBlue);
        g.drawEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (p), 1.5f);
        g.setColour (lcdCyan.withAlpha (0.8f));
        g.drawText (juce::String::fromUTF8 (names[k]), (int) p.x - 30, (int) p.y - 22, 60, 11, juce::Justification::centred);
    }
    if (ep.on && std::abs (ep.out) > 0.05f)
    {
        g.setColour (lcdCyan.withAlpha (0.8f));
        g.setFont (VintageLookAndFeel::mono (9.5f));
        g.drawText (u8 ("SA\xc3\x8d" "DA ") + (ep.out > 0 ? "+" : "") + juce::String (ep.out, 1) + " dB",
                    r.toNearestInt().reduced (4), juce::Justification::topRight);
    }
    if (! ep.on)
    {
        g.setColour (lcdCyan.withAlpha (0.7f));
        g.setFont (VintageLookAndFeel::mono (11.0f));
        g.drawText ("EQ DESLIGADO", r.toNearestInt(), juce::Justification::centredTop);
    }
}

// ---------------------------------------------------------------- Leslie animada
void LeslieCabinet::paint (juce::Graphics& g)
{
    const auto& L = proc.leslieState();
    const auto& P = mro::lesliePreset (juce::roundToInt (proc.get ("lesPreset")));
    const bool on = P.on;
    const float W = (float) getWidth(), H = (float) getHeight();
    const float aH = L.hornAngle.load(), aD = L.drumAngle.load();
    const float rH = L.hornRate.load(), rD = L.drumRate.load();

    // gabinete de madeira com veios
    juce::ColourGradient wood (juce::Colour (0xff5b3519), 0, 0, juce::Colour (0xff4a2a12), W, 0, false);
    wood.addColour (0.5, juce::Colour (0xff7a4a22));
    g.setGradientFill (wood);
    g.fillRoundedRectangle (4, 2, W - 8, H - 4, 8.0f);
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    for (float x = 9; x < W - 9; x += 6) g.drawLine (x, 4, x + 2, H - 4, 1.0f);

    const float wx = 16, ww = W - 32, wh = (H - 48) / 2;
    const float yTop = 10, yBot = 10 + wh + 8;
    for (float y : { yTop, yBot }) { g.setColour (juce::Colour (0xff120c08)); g.fillRoundedRectangle (wx, y, ww, wh, 6.0f); }
    const float cx = W * 0.5f, cyH = yTop + wh * 0.5f, cyD = yBot + wh * 0.5f, R = ww * 0.5f;
    const juce::Colour amber (0xffffc061);

    // tambor (graves): cilindro com as duas aberturas girando
    g.setColour (juce::Colour (0xff3a2a1c));
    g.fillEllipse (cx - R * 0.8f, cyD - wh * 0.32f, R * 1.6f, wh * 0.64f);
    for (int i = 0; i < 2; ++i)
    {
        const float a = aD + (float) i * juce::MathConstants<float>::pi, x = std::cos (a), z = std::sin (a);
        const float ew = 8.0f + 10.0f * std::abs (z), eh = wh * 0.36f;
        g.setColour (amber.withAlpha (on ? 0.25f + 0.55f * juce::jmax (0.0f, z) : 0.12f));
        g.fillEllipse (cx + x * R * 0.62f - ew * 0.5f, cyD + z * 6.0f - eh * 0.5f, ew, eh);
    }
    g.setColour (juce::Colour (0xff6b4a2a));
    g.drawEllipse (cx - R * 0.8f, cyD - wh * 0.32f, R * 1.6f, wh * 0.64f, 1.5f);

    // corneta (agudos): a boca de verdade e o contrapeso
    const float hx = std::cos (aH), hz = std::sin (aH);
    auto horn = [&] (float sgn, bool real)
    {
        const float x = cx + sgn * hx * R * 0.82f, y = cyH + sgn * hz * wh * 0.22f;
        const bool near = sgn * hz > 0;
        g.setColour (real ? juce::Colour (0xffc9b48a) : juce::Colour (0xff6f6250));
        g.drawLine (cx, cyH, x, y, near ? 5.0f : 3.5f);
        const float bw = real ? (near ? 22.0f : 14.0f) : 10.0f, bh = real ? (near ? 24.0f : 16.0f) : 11.0f;
        g.setColour (real ? (on ? amber.withAlpha (near ? 1.0f : 0.55f) : juce::Colour (0xff7d6c52)) : juce::Colour (0xff4d4335));
        g.fillEllipse (x - bw * 0.5f, y - bh * 0.5f, bw, bh);
    };
    if (hz > 0) { horn (-1, false); horn (1, true); } else { horn (1, true); horn (-1, false); }
    g.setColour (juce::Colour (0xff2a2018));
    g.fillEllipse (cx - 7, cyH - 7, 14, 14);

    // venezianas por cima das janelas
    g.setColour (juce::Colour (0xff7a4a22).withAlpha (0.55f));
    for (float y0 : { yTop, yBot })
        for (float y = y0 + 7; y < y0 + wh - 2; y += 10) g.drawLine (wx + 2, y, wx + ww - 2, y, 2.5f);

    // lâmpada (acende mais com a velocidade) e o nome do preset
    const float spin = juce::jmin (1.0f, juce::jmax (rH / 6.7f, rD / 5.8f));
    g.setColour (on ? juce::Colour::fromRGB ((juce::uint8) (120 + 130 * spin), (juce::uint8) (60 + 40 * (1 - spin)), 30) : juce::Colour (0xff444444));
    g.fillEllipse (W - 26, H - 24, 10, 10);
    g.setColour (juce::Colour (0xffefe6cf));
    g.setFont (VintageLookAndFeel::mono (9.5f));
    g.drawText (juce::String::fromUTF8 (P.name), 14, (int) H - 28, (int) W - 44, 16, juce::Justification::centredLeft);
}

void OrganDrawbar::paint (juce::Graphics& g)
{
    const float cx = (float) getWidth() * 0.5f;
    g.setColour (ivoryDark);
    g.setFont (VintageLookAndFeel::mono (getWidth() < 34 ? 8.0f : 9.5f));
    g.drawText (name, -4, 0, getWidth() + 8, 14, juce::Justification::centred);
    const float top = 18.0f;
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (cx - 9.0f, top, 18.0f, 8.0f, 2.0f);
    const float len = (float) value * step();
    if (value > 0)
    {
        g.setColour (ivoryDark);
        g.fillRect (cx - 7.0f, top + 4.0f, 14.0f, len);
        g.setColour (brownInk);
        g.setFont (VintageLookAndFeel::mono (9.0f));
        for (int n = 1; n <= value; ++n)
            g.drawText (juce::String (n), juce::Rectangle<float> (cx - 7.0f, top + 4.0f + (float) (n - 1) * step(), 14.0f, step()),
                        juce::Justification::centred);
    }
    const float tw = juce::jmin (30.0f, (float) getWidth() - 2.0f);
    auto tab = juce::Rectangle<float> (cx - tw * 0.5f, top + 4.0f + len, tw, 24.0f);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (tab.translated (0, 2.0f), 5.0f);
    g.setColour (tabColour);
    g.fillRoundedRectangle (tab, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillRoundedRectangle (tab.reduced (4.0f, 0).withHeight (3.0f).translated (0, 3.0f), 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (tab, 5.0f, 1.0f);
}

// =====================================================================  RIDDIM (banco único)
void RiddimVisor::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 4.0f);
    auto in = r.reduced (2.5f);
    g.setColour (lcdBlue);
    g.fillRoundedRectangle (in, 3.0f);
    g.setColour (lcdCyan.withAlpha (0.05f));
    for (float y = in.getY(); y < in.getBottom(); y += 2.0f) g.drawHorizontalLine ((int) y, in.getX(), in.getRight());

    auto arrows = in.removeFromRight (52.0f);
    g.setColour (lcdCyan.withAlpha (0.8f));
    g.setFont (VintageLookAndFeel::mono (13.0f));
    g.drawText (u8 ("\xe2\x97\x80"), arrows.removeFromLeft (26.0f), juce::Justification::centred);
    g.drawText (u8 ("\xe2\x96\xb6"), arrows, juce::Justification::centred);

    const int b = proc.getBankEntry();
    auto txt = in.reduced (10.0f, 0.0f);
    if (b < 0)
    {
        g.setColour (lcdCyan);
        g.setFont (VintageLookAndFeel::mono (12.0f));
        g.drawText (u8 ("RIDDIM \xe2\x96\xbe  escolha uma m\xc3\xbasica do banco (356 riddims, harmonia e BPM)"), txt, juce::Justification::centredLeft);
        return;
    }
    const auto& e = riddimbank::entries[b];
    g.setColour (lcdCyan);
    g.setFont (VintageLookAndFeel::mono (12.0f));
    g.drawText (riddimbank::title (e).toUpperCase(), txt.removeFromTop (txt.getHeight() * 0.55f), juce::Justification::bottomLeft);
    g.setColour (lcdCyan.withAlpha (0.7f));
    g.setFont (VintageLookAndFeel::mono (10.0f, false));
    const auto line2 = riddimbank::showCredits (e) ? riddimbank::creditsText (e)
                      : juce::String (riddimbank::styles[e.style]) + "   " + riddimbank::chordsText (e) + (e.harmony == 1 ? " ~" : "")
                        + "   " + juce::String (e.bpm) + " BPM   " + riddimbank::lights (e);
    g.drawText (line2, txt, juce::Justification::topLeft);
}

void RiddimVisor::mouseDown (const juce::MouseEvent& e)
{
    if (e.x > getWidth() - 54) { step (e.x > getWidth() - 28 ? 1 : -1); return; }
    showMenu();
}

void RiddimVisor::step (int dir)
{
    int i = proc.getBankEntry();
    for (int k = 0; k < riddimbank::numEntries; ++k)
    {
        i = (i + dir + riddimbank::numEntries) % riddimbank::numEntries;
        if (riddimbank::usable (i)) { proc.applyBankEntry (i); repaint(); return; }
    }
}

void RiddimVisor::showMenu()
{
    juce::PopupMenu menu;
    riddimbank::addMenus (menu, proc.getBankEntry());
    juce::Component::SafePointer<RiddimVisor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safe] (int r)
                        {
                            if (safe == nullptr || r == 0) return;
                            if (r == riddimbank::toggleCompleteId) { riddimbank::onlyComplete() = ! riddimbank::onlyComplete(); safe->showMenu(); return; }
                            if (const int i = riddimbank::entryFromMenuResult (r); i >= 0) { safe->proc.applyBankEntry (i); safe->repaint(); }
                        });
}

// =====================================================================  GRADE
juce::Rectangle<float> GrooveGrid::cell (int row, int step) const
{
    const float labelW = 112.0f, top = 20.0f;
    const float w = ((float) getWidth() - labelW) / 16.0f, h = ((float) getHeight() - top) / 3.0f;
    return { labelW + (float) step * w, top + (float) row * h, w, h };
}

bool GrooveGrid::cellAt (juce::Point<float> p, bool& rightHand, int& step) const
{
    for (int row = 1; row <= 2; ++row)
        for (int s = 0; s < 16; ++s)
            if (cell (row, s).contains (p)) { rightHand = (row == 1); step = s; return true; }
    return false;
}

void GrooveGrid::mouseDown (const juce::MouseEvent& e)
{
    bool rh = true; int st = 0;
    if (! cellAt (e.position, rh, st)) { paintLevel = -1; return; }
    const auto g = proc.getGrid();
    const int cur = rh ? g.rh[st] : g.lh[st];
    // vazio → acento → fantasma → vazio  (botão direito apaga)
    paintLevel = e.mods.isPopupMenu() ? mro::Off : (cur == mro::Off ? mro::Accent : (cur == mro::Accent ? mro::Ghost : mro::Off));
    paintRight = rh;
    proc.setGridCell (rh, st, paintLevel);
    repaint();
}

void GrooveGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (paintLevel < 0) return;
    bool rh = true; int st = 0;
    if (cellAt (e.position, rh, st) && rh == paintRight)
    {
        const auto g = proc.getGrid();
        if ((rh ? g.rh[st] : g.lh[st]) != paintLevel) { proc.setGridCell (rh, st, paintLevel); repaint(); }
    }
}

void GrooveGrid::paint (juce::Graphics& g)
{
    const auto grid = proc.getGrid();
    const int beat = (int) proc.get ("beat");
    const int now = proc.statusStep.load();
    const char* sub[4] = { "", "e", "&", "a" };

    // cabeçalho: 1 e & a  2 e & a ... com a caixa marcada no 2 e no 4
    for (int s = 0; s < 16; ++s)
    {
        auto c = cell (0, s).withY (0.0f).withHeight (18.0f);
        g.setFont (VintageLookAndFeel::mono (s % 4 == 0 ? 12.0f : 10.0f, s % 4 == 0));
        g.setColour (mro::snareAt (s) ? juce::Colour (0xffa32a1e) : brownInk.withAlpha (s % 4 == 0 ? 1.0f : 0.6f));
        g.drawText (s % 4 == 0 ? juce::String (s / 4 + 1) : juce::String (sub[s % 4]), c, juce::Justification::centred);
    }

    const juce::String rowNames[3] = { "BATERIA", u8 ("M\xc3\x83O DIREITA"), u8 ("M\xc3\x83O ESQUERDA") };
    const char* rowSub[3]   = { "CAIXA NO 2 E NO 4", "ACORDE \xc2\xb7 CHOP", "GRAVE \xc2\xb7 BUBBLE" };
    for (int row = 0; row < 3; ++row)
    {
        auto lab = juce::Rectangle<float> (0.0f, cell (row, 0).getY(), 108.0f, cell (row, 0).getHeight());
        g.setColour (brownInk);
        g.setFont (VintageLookAndFeel::mono (10.5f));
        g.drawText (rowNames[row], lab.withTrimmedBottom (lab.getHeight() * 0.45f), juce::Justification::bottomLeft);
        g.setColour (brownInk.withAlpha (0.55f));
        g.setFont (VintageLookAndFeel::mono (8.5f, false));
        g.drawText (u8 (rowSub[row]), lab.withTrimmedTop (lab.getHeight() * 0.58f), juce::Justification::topLeft);

        for (int s = 0; s < 16; ++s)
        {
            auto c = cell (row, s).reduced (2.0f, 4.0f);
            const bool beatHead = s % 4 == 0;
            g.setColour (row == 0 ? juce::Colour (0xffe6dcc2) : (beatHead ? juce::Colour (0xffd9ccad) : juce::Colour (0xffe9dfc6)));
            g.fillRoundedRectangle (c, 3.0f);
            if (s == now) { g.setColour (keyOrange.withAlpha (0.35f)); g.fillRoundedRectangle (c.expanded (1.5f), 3.0f); }

            if (row == 0)
            {
                // bateria de referência (só desenho)
                if (mro::snareAt (s))
                {
                    g.setColour (juce::Colour (0xffa32a1e));
                    g.fillEllipse (c.withSizeKeepingCentre (14.0f, 14.0f).translated (0.0f, mro::kickAt (beat, s) ? -6.0f : 0.0f));
                }
                if (mro::kickAt (beat, s))
                {
                    g.setColour (console);
                    g.fillEllipse (c.withSizeKeepingCentre (14.0f, 14.0f).translated (0.0f, mro::snareAt (s) ? 7.0f : 0.0f));
                }
                if (s % 2 == 0) { g.setColour (brownInk.withAlpha (0.35f)); g.drawLine (c.getCentreX() - 3, c.getY() + 4, c.getCentreX() + 3, c.getY() + 4, 1.5f); }
                continue;
            }
            const int lv = row == 1 ? grid.rh[s] : grid.lh[s];
            if (lv == mro::Off) continue;
            const auto col = row == 1 ? colChop : colBubble;
            if (lv == mro::Accent)
            {
                g.setColour (col);
                g.fillRoundedRectangle (c.reduced (1.0f), 3.0f);
                g.setColour (console);
                g.setFont (VintageLookAndFeel::mono (11.0f));
                g.drawText (">", c, juce::Justification::centred);
            }
            else
            {
                g.setColour (col.withAlpha (0.45f));
                g.fillRoundedRectangle (c.reduced (5.0f, 7.0f), 3.0f);
                g.setColour (col.darker (0.4f));
                g.drawRoundedRectangle (c.reduced (5.0f, 7.0f), 3.0f, 1.0f);
            }
        }
    }
    // separadores dos tempos
    g.setColour (brownInk.withAlpha (0.45f));
    for (int b = 1; b < 4; ++b)
    {
        const float x = cell (0, b * 4).getX();
        g.drawLine (x, 2.0f, x, (float) getHeight(), 1.2f);
    }
}

// =====================================================================  VISOR MIDI
void OrganRoll::update()
{
    if (drag != Drag::None) { repaint(); return; }
    const auto l = proc.getCurrentLoop();
    events = l.events;
    bars = l.bars;
    loopBeats = 4.0 * bars;
    chords = proc.getChords();
    computeGeometry();
    repaint();
}

void OrganRoll::computeGeometry()
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    area = r.reduced (8.0f, 6.0f);
    header = area.removeFromTop (16.0f);
    keysCol = area.removeFromLeft (30.0f);
    int mn = 127, mx = 0;
    for (const auto& e : events) { mn = std::min (mn, e.note); mx = std::max (mx, e.note); }
    if (events.empty()) { mn = 43; mx = 79; }
    lo = juce::jmax (24, mn - 2); hi = juce::jmin (100, mx + 2);
    rowH = area.getHeight() / (float) (hi - lo + 1);
}

double OrganRoll::beatAt (float x) const { return loopBeats * (double) ((x - area.getX()) / juce::jmax (1.0f, area.getWidth())); }
int OrganRoll::pitchAt (float y) const { return juce::jlimit (lo, hi, lo + (int) std::floor ((area.getBottom() - y) / rowH)); }

juce::Rectangle<float> OrganRoll::noteRect (const mro::Event& e) const
{
    auto xOf = [&] (double beat) { return area.getX() + (float) (beat / loopBeats) * area.getWidth(); };
    return { xOf (e.start), area.getBottom() - (float) (e.note - lo + 1) * rowH, juce::jmax (3.0f, xOf (e.start + e.length) - xOf (e.start)), rowH };
}

int OrganRoll::noteAt (juce::Point<float> p, bool& onRightEdge) const
{
    onRightEdge = false;
    for (int i = (int) events.size() - 1; i >= 0; --i)
    {
        const auto nr = noteRect (events[(size_t) i]).expanded (1.0f, 1.5f);
        if (nr.contains (p)) { onRightEdge = p.x > nr.getRight() - juce::jmin (5.0f, nr.getWidth() * 0.4f); return i; }
    }
    return -1;
}

void OrganRoll::commit() { proc.setEdits (events, juce::jmax (1, bars)); }

void OrganRoll::mouseMove (const juce::MouseEvent& e)
{
    bool edge = false;
    const int i = area.contains (e.position) ? noteAt (e.position, edge) : -1;
    setMouseCursor (i < 0 ? juce::MouseCursor::CrosshairCursor : (edge ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::DraggingHandCursor));
}

void OrganRoll::mouseDown (const juce::MouseEvent& e)
{
    if (loopBeats <= 0.0 || ! area.expanded (0, 2).contains (e.position)) return;
    bool edge = false;
    const int i = noteAt (e.position, edge);
    dragChanged = false;
    dragFrom = e.position;
    if (e.mods.isPopupMenu())
    {
        if (i >= 0) { events.erase (events.begin() + i); commit(); }
        return;
    }
    if (i >= 0) { dragIndex = i; dragOrig = events[(size_t) i]; drag = edge ? Drag::Resize : Drag::Move; return; }
    // espaço vazio: nota de 1/32 na semicolcheia mais próxima, na camada do PINCEL
    const double grid = e.mods.isShiftDown() ? 0.125 : 0.25;
    const double start = juce::jlimit (0.0, loopBeats - grid, std::floor (beatAt (e.position.x) / grid) * grid);
    mro::Event n;
    n.start = start; n.length = 0.125; n.note = pitchAt (e.position.y); n.velocity = 100; n.layer = brush; n.accent = true;
    events.push_back (n);
    dragIndex = (int) events.size() - 1;
    dragOrig = events.back();
    drag = Drag::Resize;
    dragChanged = true;
    repaint();
}

void OrganRoll::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == Drag::None || ! juce::isPositiveAndBelow (dragIndex, (int) events.size())) return;
    const double grid = e.mods.isShiftDown() ? 0.125 : 0.25;
    const double dBeat = beatAt (e.position.x) - beatAt (dragFrom.x);
    auto& n = events[(size_t) dragIndex];
    if (drag == Drag::Move)
    {
        n.start = juce::jlimit (0.0, loopBeats - 0.125, std::round ((dragOrig.start + dBeat) / grid) * grid);
        n.note  = juce::jlimit (lo, hi, dragOrig.note + (int) std::round ((dragFrom.y - e.position.y) / rowH));
    }
    else
        n.length = juce::jlimit (0.125, loopBeats, std::round ((dragOrig.length + dBeat) / 0.125) * 0.125);
    dragChanged = dragChanged || ! juce::exactlyEqual (n.start, dragOrig.start) || n.note != dragOrig.note || ! juce::exactlyEqual (n.length, dragOrig.length);
    repaint();
}

void OrganRoll::mouseUp (const juce::MouseEvent&)
{
    if (drag != Drag::None && dragChanged) commit();
    drag = Drag::None;
    dragIndex = -1;
    update();
}

void OrganRoll::mouseDoubleClick (const juce::MouseEvent& e)
{
    bool edge = false;
    const int i = noteAt (e.position, edge);
    if (i < 0) return;
    events.erase (events.begin() + i);
    drag = Drag::None;
    commit();
    update();
}

void OrganRoll::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (lcdBlue);
    g.fillRoundedRectangle (r.reduced (2.0f), 4.0f);
    if (area.isEmpty()) computeGeometry();
    if (loopBeats <= 0.0) return;

    auto yOf = [&] (int n) { return area.getBottom() - (float) (n - lo + 1) * rowH; };
    auto xOf = [&] (double beat) { return area.getX() + (float) (beat / loopBeats) * area.getWidth(); };
    for (int n = lo; n <= hi; ++n)
    {
        const int pc = n % 12;
        if (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10) { g.setColour (juce::Colours::black.withAlpha (0.25f)); g.fillRect (area.getX(), yOf (n), area.getWidth(), rowH); }
        if (pc == 0)
        {
            g.setColour (lcdCyan.withAlpha (0.5f));
            g.setFont (VintageLookAndFeel::mono (8.5f, false));
            g.drawText (juce::MidiMessage::getMidiNoteName (n, true, true, 4), keysCol.withY (yOf (n) - 4.0f).withHeight (rowH + 8.0f), juce::Justification::centredLeft);
        }
    }
    // acordes no cabeçalho (os tempos de cada acorde)
    if (! chords.empty() && proc.get ("chordSrc") < 0.5f)
    {
        int prevRoot = -1; bool prevMin = false; double startB = 0.0;
        auto flush = [&] (double endB)
        {
            if (prevRoot < 0) return;
            g.setColour (lcdCyan);
            g.setFont (VintageLookAndFeel::mono (10.0f));
            g.drawText (juce::String (mro::rootName (prevRoot)) + (prevMin ? "m" : ""), juce::Rectangle<float> (xOf (startB) + 4.0f, header.getY(), xOf (endB) - xOf (startB) - 4.0f, header.getHeight()), juce::Justification::centredLeft);
            g.setColour (lcdCyan.withAlpha (0.35f));
            g.drawVerticalLine ((int) xOf (startB), header.getY(), area.getBottom());
        };
        for (int b = 0; b < (int) loopBeats; ++b)
        {
            const auto& c = mro::chordForBeat (chords, b);
            if (c.root != prevRoot || c.minor != prevMin || b == 0) { flush (b); prevRoot = c.root; prevMin = c.minor; startB = b; }
        }
        flush (loopBeats);
    }
    for (double b = 0.25; b < loopBeats; b += 0.25)
    {
        const bool bar = std::fmod (b, 4.0) < 1e-6, beat = std::fmod (b, 1.0) < 1e-6;
        g.setColour (lcdCyan.withAlpha (bar ? 0.3f : (beat ? 0.12f : 0.04f)));
        g.drawVerticalLine ((int) xOf (b), area.getY(), area.getBottom());
    }
    // caixa no 2 e no 4: marquinha vermelha embaixo
    g.setColour (juce::Colour (0xffe5484d).withAlpha (0.7f));
    for (double b = 1.0; b < loopBeats; b += 2.0) g.fillRect (xOf (b) - 1.5f, area.getBottom() - 4.0f, 3.0f, 4.0f);

    const double pos = proc.playPos.load();
    for (int i = 0; i < (int) events.size(); ++i)
    {
        const auto& e = events[(size_t) i];
        auto nr = noteRect (e).reduced (0.0f, rowH > 5.0f ? 0.5f : 0.0f);
        const bool now = pos >= 0.0 && pos >= e.start && pos < e.start + e.length;
        const float a = 0.45f + 0.55f * (float) e.velocity / 127.0f;
        g.setColour (layerColour (e.layer).withAlpha (now ? 1.0f : a));
        g.fillRect (nr);
        if (now || (i == dragIndex && drag != Drag::None)) { g.setColour (juce::Colours::white); g.drawRect (nr, 1.0f); }
    }
    if (pos >= 0.0)
    {
        g.setColour (lcdCyan);
        g.drawVerticalLine ((int) xOf (pos), area.getY(), area.getBottom());
    }
    // legenda
    g.setFont (VintageLookAndFeel::mono (9.5f));
    auto leg = juce::Rectangle<float> (area.getRight() - 300.0f, header.getY(), 300.0f, header.getHeight());
    const char* names[3] = { "BUBBLE", "CHOP", "FILLS" };
    for (int k = 2; k >= 0; --k)
    {
        auto it = leg.removeFromRight (90.0f);
        g.setColour (layerColour (k));
        g.fillRect (it.removeFromLeft (10.0f).withSizeKeepingCentre (10.0f, 6.0f));
        g.drawText (names[k], it.withTrimmedLeft (4.0f), juce::Justification::centredLeft);
    }
    if (proc.hasEdits())
    {
        g.setColour (juce::Colour (0xffffc061));
        g.drawText ("MIDI EDITADO", juce::Rectangle<float> (area.getX() + 4.0f, area.getBottom() - 16.0f, 140.0f, 14.0f), juce::Justification::centredLeft);
    }
}

// =====================================================================  arrastar MIDI
juce::File MidiDragChip::makeFile (const juce::File& dir) const
{
    juce::String prog;
    for (auto& ch : proc.getChords()) prog << mro::rootName (ch.root) << (ch.minor ? "m" : "") << " ";
    const char* part[4] = { "", " Bubble", " Chop", " Fills" };
    auto f = dir.getChildFile ("Reggae Organ " + juce::String (mro::musician (proc.getCurrentProgram()).name) + part[layer + 1] + " "
                               + prog.trim() + " " + juce::String (juce::roundToInt (proc.exportBpm())) + "bpm.mid");
    proc.writeMidiFile (f, layer);
    return f;
}

void MidiDragChip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (dragging ? keyOrange : console);
    g.fillRoundedRectangle (r.reduced (2.0f), 3.0f);
    if (layer >= 0) { g.setColour (layerColour (layer)); g.fillRect (r.reduced (2.0f).removeFromLeft (6.0f)); }
    g.setColour (dragging ? console : ivory);
    g.setFont (VintageLookAndFeel::mono (10.5f));
    g.drawText (u8 ("\xe2\x87\xa3 ") + text, r, juce::Justification::centred);
}

void MidiDragChip::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true; repaint();
    auto f = makeFile (juce::File::getSpecialLocation (juce::File::tempDirectory));
    if (f.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (f.getFullPathName()), false, this,
                                                                    [this] { dragging = false; repaint(); });
    else { dragging = false; repaint(); }
}

void MidiDragChip::mouseUp (const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() >= 6) { dragging = false; repaint(); return; }
    auto f = makeFile (juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Mastor Reggae Organ"));
    if (onSaved) onSaved (f.existsAsFile() ? juce::String ("MIDI SALVO EM DOCUMENTOS/MASTOR REGGAE ORGAN") : u8 ("N\xc3\x83O CONSEGUI SALVAR O MIDI"));
}

// =====================================================================  Editor
ReggaeOrganEditor::ReggaeOrganEditor (ReggaeOrganProcessor& p)
    : AudioProcessorEditor (&p), proc (p), riddim (p), grid (p), roll (p), keyboard (p.keyboardState, p)
{
    plateFace = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoBlackRegular_ttf, (size_t) BinaryData::ArchivoBlackRegular_ttfSize);
    titleFace = juce::Typeface::createSystemTypefaceFor (BinaryData::RootsKeysDisplayBoldItalic_ttf, (size_t) BinaryData::RootsKeysDisplayBoldItalic_ttfSize);
    lnf.setSkin (1);   // visual do órgão do MRK-80: madeira + marfim
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;

    // ---- cabeçalho
    for (auto* b : { &grooveBtn, &playBtn }) { b->getProperties().set ("style", "rocker"); addAndMakeVisible (b); }
    grooveAtt = std::make_unique<APVTS::ButtonAttachment> (s, "groove", grooveBtn);
    playAtt   = std::make_unique<APVTS::ButtonAttachment> (s, "play", playBtn);
    setupKnob (bpm, "bpm", "BPM");

    // ---- MÚSICOS (teclas invertidas do B3)
    for (int i = 0; i < mro::NumMusicians; ++i)
    {
        musBtn[i].setButtonText (mro::musician (i).name);
        musBtn[i].getProperties().set ("style", "reverseKey");
        musBtn[i].setClickingTogglesState (false);
        musBtn[i].setTooltip (u8 (mro::musician (i).who));
        musBtn[i].onClick = [this, i] { proc.applyMusician (i); };
        addAndMakeVisible (musBtn[i]);
    }
    // ---- ÉPOCAS
    epochBox.setTextWhenNothingSelected (u8 ("\xc3\x89POCA \xe2\x96\xbe"));
    for (int e = 0; e < mro::numEpochs; ++e)
        epochBox.addItem (juce::String (mro::epochName (e)) + "  " + mro::epochYears (e), e + 1);
    epochBox.onChange = [this] { const int e = epochBox.getSelectedId() - 1; if (e >= 0 && e != proc.getEpoch()) proc.applyEpoch (e); };
    addAndMakeVisible (epochBox);
    // ---- RIDDIMS
    addAndMakeVisible (riddim);
    jackpot.getProperties().set ("style", "chip");
    jackpot.setTooltip (u8 ("Outra varia\xc3\xa7\xc3\xa3o do groove (humaniza\xc3\xa7\xc3\xa3o e frases dos fills)"));
    jackpot.onClick = [this] { proc.newVariation(); jackpotFlash = 18; };
    addAndMakeVisible (jackpot);

    setupChips (tabs, {}, { "GROOVE", "MIDI", u8 ("\xc3\x93RG\xc3\x83O"), "EQ" });
    tabs.onPick = [this] (int i) { setPage (i); };
    tabs.current = [this] { return page; };
    status.setFont (VintageLookAndFeel::mono (11.0f));
    status.setColour (juce::Label::textColourId, lcdCyan);
    status.setColour (juce::Label::backgroundColourId, lcdBlue);
    status.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (status);

    // ---- aba GROOVE
    addAndMakeVisible (grid);
    setupChips (feel, {}, { "SKA", "ROCKSTEADY", "REGGAE" });
    feel.onPick = [this] (int i) { quickFeel = i; proc.applyFeel (quickFeel, quickDensity); };
    feel.current = [this] { return quickFeel; };
    setupChips (density, {}, { u8 ("S\xc3\x93 e"), u8 ("S\xc3\x93 a"), "e + a" });
    density.onPick = [this] (int i) { quickDensity = i; proc.applyFeel (quickFeel, quickDensity); };
    density.current = [this] { return quickDensity; };
    setupChips (beat, "beat", { "ONE DROP", "ROCKERS", "STEPPERS" });
    setupChips (nota, "nota", { "1/32", "1/16", "1/8" });
    setupKnob (swing, "swing", "Swing");
    setupKnob (human, "human", "Human");
    setupKnob (ghost, "ghost", "Ghost");
    setupKnob (seed,  "seed",  u8 ("VARIA\xc3\x87\xc3\x83O"));
    {
        const char* ids[3] = { "bubbleOn", "chopOn", "fillsOn" };
        const char* names[3] = { "BUBBLE", "CHOP", "FILLS" };
        for (int i = 0; i < 3; ++i)
        {
            layerBtn[i].setButtonText (names[i]);
            layerBtn[i].getProperties().set ("style", "rocker");
            addAndMakeVisible (layerBtn[i]);
            layerAtt[i] = std::make_unique<APVTS::ButtonAttachment> (s, ids[i], layerBtn[i]);
        }
    }
    setupKnob (bubbleLvl, "bubbleLvl", "Volume");
    setupKnob (chopLvl,   "chopLvl",   "Volume");
    setupChips (fillStyle, "fillStyle", { "MITTOO", "WRIGHT", "DOWNIE", "GLADDY" });
    setupChips (fillEvery, "fillEvery", { "2", "4", "8" });
    setupChips (chordSrc, "chordSrc", { "DO PLUGIN", "DO TECLADO" });

    // ---- aba MIDI
    for (int i = 0; i < ReggaeOrganProcessor::numSlots; ++i)
    {
        const juce::String n (i + 1);
        for (int r = 0; r < 12; ++r) rootBox[i].addItem (mro::rootName (r), r + 1);
        addAndMakeVisible (rootBox[i]);
        rootAtt[i] = std::make_unique<APVTS::ComboBoxAttachment> (s, "root" + n, rootBox[i]);
        for (int d = 0; d < mro::numChordDurations; ++d) durBox[i].addItem (mro::chordDurationName (d), d + 1);
        addAndMakeVisible (durBox[i]);
        durAtt[i] = std::make_unique<APVTS::ComboBoxAttachment> (s, "dur" + n, durBox[i]);
        majBtn[i].setButtonText ("MAIOR"); minBtn[i].setButtonText ("menor");
        for (auto* b : { &majBtn[i], &minBtn[i] }) { b->getProperties().set ("style", "chip"); b->setClickingTogglesState (false); addAndMakeVisible (b); }
        majBtn[i].onClick = [this, n] { proc.setParam ("minor" + n, 0.0f); };
        minBtn[i].onClick = [this, n] { proc.setParam ("minor" + n, 1.0f); };
        slotOn[i].setButtonText ("ACORDE " + n);
        slotOn[i].getProperties().set ("style", "rocker");
        addAndMakeVisible (slotOn[i]);
        if (i > 0) slotOnAtt[i] = std::make_unique<APVTS::ButtonAttachment> (s, "on" + n, slotOn[i]);
        else { slotOn[i].setToggleState (true, juce::dontSendNotification); slotOn[i].setEnabled (false); }
    }
    addAndMakeVisible (roll);
    setupChips (brush, {}, { "BUBBLE", "CHOP", "FILLS" });
    brush.onPick = [this] (int i) { roll.brush = i; };
    brush.current = [this] { return roll.brush; };
    dragAll = std::make_unique<MidiDragChip> (proc, -1, "TUDO (3 TRILHAS)");
    dragLayer[0] = std::make_unique<MidiDragChip> (proc, mro::LayerBubble, "BUBBLE");
    dragLayer[1] = std::make_unique<MidiDragChip> (proc, mro::LayerChop, "CHOP");
    dragLayer[2] = std::make_unique<MidiDragChip> (proc, mro::LayerFill, "FILLS");
    for (auto* d : { dragAll.get(), dragLayer[0].get(), dragLayer[1].get(), dragLayer[2].get() })
    {
        d->onSaved = [this] (const juce::String& m) { savedMsg = m; };
        addAndMakeVisible (d);
    }
    repsBox.addItem ("x1", 1); repsBox.addItem ("x2", 2); repsBox.addItem ("x4", 3);
    addAndMakeVisible (repsBox);
    repsAtt = std::make_unique<APVTS::ComboBoxAttachment> (s, "reps", repsBox);
    backToGroove.setButtonText ("VOLTAR AO GROOVE");
    backToGroove.getProperties().set ("style", "chip");
    backToGroove.setTooltip (u8 ("Descarta o MIDI editado e volta a tocar o groove da grade"));
    backToGroove.onClick = [this] { proc.clearEdits(); roll.update(); };
    addAndMakeVisible (backToGroove);

    // ---- aba ÓRGÃO
    for (int i = 0; i < 9; ++i)
    {
        bubbleDb[i] = std::make_unique<OrganDrawbar> (*s.getParameter ("bdb" + juce::String (i + 1)), ReggaeOrganProcessor::drawbarNames()[i], drawbarColour (i));
        chopDb[i]   = std::make_unique<OrganDrawbar> (*s.getParameter ("cdb" + juce::String (i + 1)), ReggaeOrganProcessor::drawbarNames()[i], drawbarColour (i));
        addAndMakeVisible (*bubbleDb[i]);
        addAndMakeVisible (*chopDb[i]);
    }
    // Leslie: o preset liga a velocidade e a profundidade dele (como na versão para celular)
    addAndMakeVisible (cabinet);
    setupChips (lesPreset, {}, { "SEM LESLIE", u8 ("122 CL\xc3\x81SS."), "ROOTS", "147 GRAVE", "CHORALE", "DUB TREM." });
    lesPreset.current = [this] { return juce::roundToInt (proc.get ("lesPreset")); };
    lesPreset.onPick = [this] (int i)
    {
        proc.setParam ("lesPreset", (float) i);
        const auto& P = mro::lesliePreset (i);
        if (P.on) { proc.setParam ("lesSpeed", (float) P.speed); proc.setParam ("lesDepth", P.depth); proc.setParam ("lesBrake", 0.0f); }
    };
    {
        const char* tips[mro::NumLesliePresets] = { "som direto", "a Leslie 122 do Hammond B3", "lenta, macia no chop",
                                                    "Leslie 147: tambor mais presente", "meio-termo, coral", "r\xc3\xa1pida e funda" };
        for (int i = 0; i < mro::NumLesliePresets; ++i)
            lesPreset.buttons[i]->setTooltip (juce::String::fromUTF8 (mro::lesliePreset (i).name) + ": " + juce::String::fromUTF8 (tips[i]));
    }
    setupChips (lesSpeed, {}, { "LENTA", u8 ("R\xc3\x81PIDA") });
    lesSpeed.current = [this] { return juce::roundToInt (proc.get ("lesSpeed")); };
    lesSpeed.onPick = [this] (int i) { proc.setParam ("lesSpeed", (float) i); proc.setParam ("lesBrake", 0.0f); };
    lesBrake.getProperties().set ("style", "chip");
    lesBrake.setTooltip ("Para os rotores (clique de novo para soltar)");
    lesBrake.onClick = [this] { proc.setParam ("lesBrake", proc.get ("lesBrake") > 0.5f ? 0.0f : 1.0f); };
    addAndMakeVisible (lesBrake);
    setupKnob (lesDepth, "lesDepth", "Profund.");
    lesDepth.label.setColour (juce::Label::textColourId, ivoryDark);
    lesRead.setFont (VintageLookAndFeel::mono (8.5f, false));
    lesRead.setJustificationType (juce::Justification::centred);
    lesRead.setColour (juce::Label::textColourId, ivoryDark);
    addAndMakeVisible (lesRead);
    setupChips (instr, "instr", { "B3", "RHODES", "SUITC.", "WURLI", "DX EP" });
    for (auto* b : { &percBtn, &echoBtn, &maxBtn }) { b->getProperties().set ("style", "rocker"); addAndMakeVisible (b); }
    percAtt = std::make_unique<APVTS::ButtonAttachment> (s, "perc", percBtn);
    echoAtt = std::make_unique<APVTS::ButtonAttachment> (s, "echoOn", echoBtn);
    maxAtt  = std::make_unique<APVTS::ButtonAttachment> (s, "maxOn", maxBtn);
    setupKnob (drive,    "drive",    "Drive");
    setupKnob (click,    "click",    "Click");
    setupKnob (echoVol,  "echoVol",  "Eco Vol");
    setupKnob (echoInt,  "echoInt",  "Eco Rep");
    setupKnob (echoRate, "echoRate", "Eco Vel");
    setupKnob (reverb,   "reverb",   "Reverb");
    setupKnob (master,   "master",   "Master");
    vu.setColours (juce::Colour (0xfff7efd9), juce::Colour (0xff2a2420), juce::Colour (0xff2a231e));
    addAndMakeVisible (vu);

    // ---- aba EQ (1.3)
    addAndMakeVisible (eqCurve);
    eqOnBtn.getProperties().set ("style", "rocker");
    addAndMakeVisible (eqOnBtn);
    eqOnAtt = std::make_unique<APVTS::ButtonAttachment> (s, "eqOn", eqOnBtn);
    setupKnob (eqOut,   "eqOut",   "Ganho");
    eqOut.slider.setDoubleClickReturnValue (true, 0.0);
    setupKnob (eqHpf,   "eqHpf",   "Corte");
    setupKnob (eqLowG,  "eqLowG",  "Ganho");
    setupKnob (eqLowF,  "eqLowF",  "Freq.");
    setupKnob (eqMidG,  "eqMidG",  "Ganho");
    setupKnob (eqMidF,  "eqMidF",  "Freq.");
    setupKnob (eqMidQ,  "eqMidQ",  "Q");
    setupKnob (eqHighG, "eqHighG", "Ganho");
    setupKnob (eqHighF, "eqHighF", "Freq.");
    for (auto* k : { &eqLowG, &eqMidG, &eqHighG }) k->slider.setDoubleClickReturnValue (true, 0.0);
    eqHpf.slider.setDoubleClickReturnValue (true, 20.0);
    {
        juce::StringArray names;
        for (int i = 0; i < mro::NumEqPresets; ++i) names.add (u8 (mro::eqPreset (i).name));
        setupChips (eqPresets, {}, names);
        for (int i = 0; i < mro::NumEqPresets; ++i) eqPresets.buttons[i]->setTooltip (u8 (mro::eqPreset (i).sub));
    }
    eqPresets.current = [this] { return proc.getEqPreset(); };
    eqPresets.onPick = [this] (int i) { proc.applyEqPreset (i); };
    {
        juce::StringArray names;
        for (int i = 0; i < mro::NumMidniteTimbres; ++i) names.add (u8 (mro::midniteTimbre (i).name));
        setupChips (midniteChips, {}, names);
        for (int i = 0; i < mro::NumMidniteTimbres; ++i)
            midniteChips.buttons[i]->setTooltip (u8 (mro::midniteTimbre (i).sub)
                                                 + u8 (". Troca registros, Leslie, drive, click, reverb, eco e EQ; o groove continua o do m\xc3\xbasico."));
        midniteChips.current = [this] { return proc.midniteIndex(); };
        midniteChips.onPick = [this] (int i) { proc.applyMidnite (i); };
    }
    midniteInfo.setText (u8 ("Ron Benjamin, tecladista e diretor musical do Midnite (St. Croix, Ilhas Virgens, 1989-2015). "
                             "Os discos Unpolished e Ras Mek Peace foram gravados ao vivo, direto em fita de dois canais, "
                             "sem reverb, sem eco e sem EQ: um som cru, simples e pesado, com o baixo na frente.\n\n"
                             "MIDNITE, o original: opaco e escuro, bubble oco (8' + 4' e um pouco de 2 2/3'), chop sem percuss\xc3\xa3o e sem key click, "
                             "Leslie lenta e discreta, seco; o EQ corta o grave profundo, encorpa o m\xc3\xa9" "dio-grave e tira m\xc3\xa9" "dio e agudo.\n"
                             "ABAFADO: mais escuro e redondo. CORPO: mais m\xc3\xa9" "dio-grave e o 16' cheio. AO VIVO: Leslie 122 lenta, um pouco de sala e mais drive. "
                             "Todos com o volume igualado na SA\xc3\x8d" "DA.\n\n"
                             "Nenhuma fonte diz qual \xc3\xb3rg\xc3\xa3o ele usava: os registros s\xc3\xa3o uma aproxima\xc3\xa7\xc3\xa3o de ouvido."),
                         juce::dontSendNotification);
    midniteInfo.setFont (VintageLookAndFeel::mono (9.5f, false));
    midniteInfo.setColour (juce::Label::textColourId, brownInk);
    midniteInfo.setJustificationType (juce::Justification::topLeft);
    midniteInfo.setMinimumHorizontalScale (1.0f);
    addAndMakeVisible (midniteInfo);

    // ---- teclado
    keyboard.setAvailableRange (36, 96);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setWantsKeyboardFocus (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, ivory);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, console);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xffa89a82));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, keyOrange.withAlpha (0.92f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, keyOrange.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, brownInk);
    addAndMakeVisible (keyboard);

    setSize (kW, kH);
    setPage (0);
    timerCallback();
    startTimerHz (30);
}

ReggaeOrganEditor::~ReggaeOrganEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ReggaeOrganEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& text)
{
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setPopupDisplayEnabled (true, true, this);
    k.slider.setColour (juce::Slider::rotarySliderOutlineColourId, brownInk);
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<APVTS::SliderAttachment> (proc.apvts, paramId, k.slider);
    k.label.setText (text.toUpperCase(), juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (VintageLookAndFeel::mono (10.0f));
    k.label.setColour (juce::Label::textColourId, brownInk);
    k.label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k.label);
}

void ReggaeOrganEditor::placeKnob (Knob& k, juce::Point<int> c, int size, int labelW)
{
    k.slider.setBounds (juce::Rectangle<int> (size, size).withCentre (c));
    k.label.setBounds (juce::Rectangle<int> (labelW, 14).withCentre ({ c.x, c.y + size / 2 + 9 }));
}

void ReggaeOrganEditor::setupChips (Chips& c, const juce::String& paramId, const juce::StringArray& names)
{
    c.paramId = paramId;
    for (int i = 0; i < names.size(); ++i)
    {
        auto* b = c.buttons.add (new juce::TextButton (names[i]));
        b->getProperties().set ("style", "chip");
        b->setClickingTogglesState (false);
        b->onClick = [this, &c, i]
        {
            if (c.paramId.isEmpty()) { if (c.onPick) c.onPick (i); }
            else proc.setParam (c.paramId, (float) i);
            refreshChips (c);
        };
        addAndMakeVisible (b);
    }
}

void ReggaeOrganEditor::refreshChips (Chips& c)
{
    const int v = c.paramId.isEmpty() ? (c.current ? c.current() : -1) : juce::roundToInt (proc.get (c.paramId));
    for (int i = 0; i < c.buttons.size(); ++i) c.buttons[i]->setToggleState (i == v, juce::dontSendNotification);
}

void ReggaeOrganEditor::layoutChips (Chips& c, juce::Rectangle<int> area, int gap)
{
    const int n = c.buttons.size();
    const int w = (area.getWidth() - gap * (n - 1)) / n;
    for (int i = 0; i < n; ++i) c.buttons[i]->setBounds (area.getX() + i * (w + gap), area.getY(), w, area.getHeight());
}

std::vector<juce::Component*> ReggaeOrganEditor::pageComponents (int p)
{
    std::vector<juce::Component*> v;
    auto addK = [&] (Knob& k) { v.push_back (&k.slider); v.push_back (&k.label); };
    auto addC = [&] (Chips& c) { for (auto* b : c.buttons) v.push_back (b); };
    if (p == 0)
    {
        v.push_back (&grid);
        addC (feel); addC (density); addC (beat); addC (nota); addC (fillStyle); addC (fillEvery); addC (chordSrc);
        for (auto* k : { &swing, &human, &ghost, &seed, &bubbleLvl, &chopLvl }) addK (*k);
        for (auto& b : layerBtn) v.push_back (&b);
    }
    else if (p == 1)
    {
        for (int i = 0; i < ReggaeOrganProcessor::numSlots; ++i)
            for (juce::Component* c : { (juce::Component*) &rootBox[i], (juce::Component*) &durBox[i], (juce::Component*) &majBtn[i],
                                        (juce::Component*) &minBtn[i], (juce::Component*) &slotOn[i] })
                v.push_back (c);
        v.push_back (&roll);
        addC (brush);
        for (auto* d : { dragAll.get(), dragLayer[0].get(), dragLayer[1].get(), dragLayer[2].get() }) v.push_back (d);
        v.push_back (&repsBox);
        v.push_back (&backToGroove);
    }
    else if (p == 3)
    {
        v.push_back (&eqCurve);
        v.push_back (&eqOnBtn);
        for (auto* k : { &eqOut, &eqHpf, &eqLowG, &eqLowF, &eqMidG, &eqMidF, &eqMidQ, &eqHighG, &eqHighF }) addK (*k);
        addC (eqPresets);
        addC (midniteChips);
        v.push_back (&midniteInfo);
    }
    else
    {
        for (int i = 0; i < 9; ++i) { v.push_back (bubbleDb[i].get()); v.push_back (chopDb[i].get()); }
        v.push_back (&cabinet);
        addC (lesPreset);
        addC (lesSpeed);
        v.push_back (&lesBrake);
        addK (lesDepth);
        v.push_back (&lesRead);
        addC (instr);
        for (auto* b : { &percBtn, &echoBtn, &maxBtn }) v.push_back (b);
        for (auto* k : { &drive, &click, &echoVol, &echoInt, &echoRate, &reverb, &master }) addK (*k);
        v.push_back (&vu);
    }
    return v;
}

void ReggaeOrganEditor::setPage (int p)
{
    page = juce::jlimit (0, 3, p);
    for (int k = 0; k < 4; ++k)
        for (auto* c : pageComponents (k)) c->setVisible (k == page);
    refreshChips (tabs);
    if (page == 1) roll.update();
    repaint();
}

void ReggaeOrganEditor::timerCallback()
{
    {
        const int newLook = juce::jlimit (0, 4, (int) proc.apvts.getRawParameterValue ("instr")->load());
        if (newLook != look) { look = newLook; repaint(); }
    }
    for (auto* c : { &tabs, &feel, &density, &beat, &nota, &fillStyle, &fillEvery, &chordSrc, &lesPreset, &lesSpeed, &instr, &brush, &eqPresets, &midniteChips })
        refreshChips (*c);
    if (page == 3) eqCurve.repaint();
    lesBrake.setToggleState (proc.get ("lesBrake") > 0.5f, juce::dontSendNotification);
    if (page == 2)
    {
        cabinet.repaint();
        const auto& L = proc.leslieState();
        const bool on = mro::lesliePreset (juce::roundToInt (proc.get ("lesPreset"))).on;
        lesRead.setText (on ? "CORNETA " + juce::String (L.hornRate.load(), 2) + "  TAMBOR " + juce::String (L.drumRate.load(), 2) + " rot/s"
                            : juce::String ("SOM DIRETO"), juce::dontSendNotification);
    }
    const int mus = proc.getCurrentProgram();
    for (int i = 0; i < mro::NumMusicians; ++i) musBtn[i].setToggleState (i == mus, juce::dontSendNotification);
    const int ep = proc.getEpoch();
    if (epochBox.getSelectedId() != ep + 1)
    {
        if (ep >= 0) epochBox.setSelectedId (ep + 1, juce::dontSendNotification);
        else epochBox.setSelectedId (0, juce::dontSendNotification);
    }

    const bool fromKeys = proc.get ("chordSrc") > 0.5f;
    for (int i = 0; i < ReggaeOrganProcessor::numSlots; ++i)
    {
        const bool minor = proc.get ("minor" + juce::String (i + 1)) > 0.5f;
        majBtn[i].setToggleState (! minor, juce::dontSendNotification);
        minBtn[i].setToggleState (minor, juce::dontSendNotification);
        const bool on = i == 0 || proc.get ("on" + juce::String (i + 1)) > 0.5f;
        for (juce::Component* c : { (juce::Component*) &rootBox[i], (juce::Component*) &durBox[i], (juce::Component*) &majBtn[i], (juce::Component*) &minBtn[i] })
            c->setAlpha (on && ! fromKeys ? 1.0f : 0.4f);
    }
    backToGroove.setEnabled (proc.hasEdits());
    backToGroove.setToggleState (proc.hasEdits(), juce::dontSendNotification);

    if (jackpotFlash > 0) { --jackpotFlash; jackpot.setToggleState ((jackpotFlash / 3) % 2 == 0, juce::dontSendNotification); }
    else jackpot.setToggleState (false, juce::dontSendNotification);

    juce::String st;
    const auto& m = mro::musician (mus);
    if (proc.statusRunning.load())
    {
        st << (fromKeys ? juce::String ("TECLADO") : "COMPASSO " + juce::String (proc.statusBar.load() + 1))
           << "   " << mro::rootName (proc.statusRoot.load()) << (proc.statusMinor.load() ? "m" : "")
           << "   " << m.name << " / " << mro::beatName ((int) proc.get ("beat"))
           << (proc.hasEdits() ? juce::String ("   MIDI EDITADO") : juce::String());
    }
    else if (savedMsg.isNotEmpty()) st = savedMsg;
    else st = u8 (m.who) + (fromKeys ? u8 ("  \xc2\xb7  SEGURE UM ACORDE E D\xc3\x8a PLAY") : u8 ("  \xc2\xb7  D\xc3\x8a PLAY AQUI OU NA DAW"));
    status.setText (st, juce::dontSendNotification);

    grid.repaint();
    riddim.repaint();
    if (page == 1) roll.update();
    vu.update (proc.meter.load(), 1.0f / 30.0f);
    keyboard.repaint();
}

// ---------------------------------------------------------------------  fundo
void ReggaeOrganEditor::renderBackground (juce::Graphics& g)
{
    // gabinete conforme o INSTRUMENTO: madeira do Hammond, tolex preto do Rhodes, coral do Wurlitzer, preto anos 80 do DX
    if (look == 0)
    {
        static const juce::Image wood = skin::makeWood (kW, kH);
        g.drawImageAt (wood, 0, 0);
    }
    else
    {
        static const juce::Colour bodies[5] = { juce::Colour (0xff5b3519), juce::Colour (0xff1b1a19), juce::Colour (0xff1b1a19),
                                                juce::Colour (0xffa4472f), juce::Colour (0xff232022) };
        g.fillAll (bodies[juce::jlimit (0, 4, look)]);
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        for (int y = 0; y < kH; y += 3) g.drawHorizontalLine (y, 0.0f, (float) kW);
        g.setColour (juce::Colours::black.withAlpha (look == 3 ? 0.08f : 0.18f));
        for (int x = 0; x < kW; x += 4) g.drawVerticalLine (x, 0.0f, (float) kH);
    }
    g.setColour (juce::Colours::black.withAlpha (look == 0 ? 0.12f : 0.3f));
    g.fillRect (0, 0, kW, 74);
    if (look == 1 || look == 2)
    {   // chapa de alumínio e feltro vermelho do Rhodes
        g.setColour (juce::Colour (0xffbfbfbb));
        g.fillRect (0, 75, kW, 2);
        g.setColour (juce::Colour (0xff8e2a20));
        g.fillRect (0, 77, kW, 3);
    }
    else if (look == 4)
    {   // friso bordô do teclado FM anos 80
        g.setColour (juce::Colour (0xff7a2234));
        g.fillRect (0, 75, kW, 3);
    }
    g.setColour (brownInk);
    g.fillRect (0, 73, kW, 2);

    g.setColour (ivory);
    g.setFont (juce::FontOptions (juce::Font::getDefaultSerifFontName(), 15.0f, juce::Font::italic));
    g.drawText ("DubMastor", 30, 6, 120, 18, juce::Justification::centredLeft);
    // nome e, logo depois, a versão (padrão da linha)
    g.setFont (juce::FontOptions (titleFace).withHeight (34.0f));
    const juce::String titleText ("Reggae Organ");
    g.drawText (titleText, 26, 22, 300, 44, juce::Justification::centredLeft);
    const int tw = juce::GlyphArrangement::getStringWidthInt (juce::Font (juce::FontOptions (titleFace).withHeight (34.0f)), titleText);
    g.setFont (VintageLookAndFeel::mono (13.0f));
    g.drawText (versionText(), 26 + tw + 10, 38, 60, 18, juce::Justification::centredLeft);
    g.setColour (ivory.withAlpha (0.7f));
    g.setFont (VintageLookAndFeel::mono (9.5f, false));
    g.drawText ("MRO-68", 26 + tw + 10, 24, 80, 14, juce::Justification::centredLeft);
    {
        static const char* badges[5] = { "B3", "STAGE 73", "SUITCASE 73", "200A", "FM EP" };
        g.setColour (ivory.withAlpha (0.85f));
        g.setFont (VintageLookAndFeel::mono (10.0f));
        g.drawText (badges[juce::jlimit (0, 4, look)], 26 + tw + 70, 38, 120, 18, juce::Justification::centredLeft);
    }

    // placa DUB STATION (latão dourado, igual à do MRK-80)
    {
        auto r = kPlate.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r.expanded (2.0f).translated (0.0f, 2.5f), 9.0f);
        g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff8a6418), r.getY() - 2.0f, juce::Colour (0xff4a3208), r.getBottom() + 2.0f));
        g.fillRoundedRectangle (r.expanded (2.0f), 9.0f);
        juce::ColourGradient gold (juce::Colour (0xfffff3c4), 0.0f, r.getY(), juce::Colour (0xfff3d98a), 0.0f, r.getBottom(), false);
        gold.addColour (0.35, juce::Colour (0xffe2b956));
        gold.addColour (0.52, juce::Colour (0xffa9781f));
        gold.addColour (0.70, juce::Colour (0xffd8a943));
        g.setGradientFill (gold);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.42f), 8.0f);
        g.setColour (juce::Colour (0xff6b4a10).withAlpha (0.6f));
        g.drawRoundedRectangle (r.reduced (3.0f), 6.0f, 1.0f);
        g.setFont (juce::FontOptions (plateFace).withHeight (17.0f));
        g.setColour (juce::Colour (0xfffff8dc).withAlpha (0.7f));
        g.drawText ("DUB STATION", r.translated (0.0f, 1.0f), juce::Justification::centred);
        g.setColour (juce::Colour (0xff3a2606));
        g.drawText ("DUB STATION", r, juce::Justification::centred);
    }

    // trilho de marfim do transporte
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRoundedRectangle (610.0f, 22.0f, 366.0f, 40.0f, 20.0f);
    g.setColour (ivory);
    g.fillRoundedRectangle (610.0f, 20.0f, 366.0f, 40.0f, 20.0f);

    // MÚSICOS: época de cada um sob a tecla
    g.setColour (ivory);
    g.setFont (VintageLookAndFeel::mono (10.0f));
    g.drawText (u8 ("M\xc3\x9aSICOS"), kMus.getX(), kMus.getY() - 2, 200, 12, juce::Justification::centredLeft);
    g.setFont (VintageLookAndFeel::mono (8.0f, false));
    g.setColour (ivory.withAlpha (0.8f));
    const int mw = (kMus.getWidth() - 7 * 6) / 8;
    for (int i = 0; i < mro::NumMusicians; ++i)
        g.drawText (yearsOf (mro::musician (i).who), kMus.getX() + i * (mw + 6), kMus.getY() + 42, mw, 12, juce::Justification::centred);

    auto ivoryPanel = [&] (juce::Rectangle<int> r, const juce::String& title, const juce::String& sub)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (r.toFloat().translated (0, 2.0f), 5.0f);
        g.setColour (ivory);
        g.fillRoundedRectangle (r.toFloat(), 5.0f);
        g.setFont (VintageLookAndFeel::mono (11.0f));
        g.setColour (brownInk);
        g.drawText (title, r.getX() + 14, r.getY() + 6, 300, 14, juce::Justification::centredLeft);
        if (sub.isNotEmpty())
        {
            g.setColour (brownInk.withAlpha (0.6f));
            g.setFont (VintageLookAndFeel::mono (9.5f, false));
            g.drawText (sub, r.getX() + 14, r.getY() + 6, r.getWidth() - 28, 14, juce::Justification::centredRight);
        }
        g.setColour (brownInk.withAlpha (0.3f));
        g.drawHorizontalLine (r.getY() + 24, (float) r.getX() + 14, (float) r.getRight() - 14);
    };
    auto smallLabel = [&] (const juce::String& t, int x, int y, int w = 300)
    {
        g.setFont (VintageLookAndFeel::mono (9.5f));
        g.setColour (brownInk.withAlpha (0.8f));
        g.drawText (t, x, y, w, 12, juce::Justification::centredLeft);
    };
    auto consolePanel = [&] (juce::Rectangle<int> r, const juce::String& title, const juce::String& sub)
    {
        g.setColour (console);
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
        g.setColour (brownInk);
        g.drawRoundedRectangle (r.toFloat(), 4.0f, 2.0f);
        g.setColour (ivory);
        g.setFont (VintageLookAndFeel::mono (11.0f));
        g.drawText (title, r.getX() + 12, r.getY() + 8, r.getWidth() - 24, 14, juce::Justification::centredLeft);
        g.setColour (ivoryDark.withAlpha (0.7f));
        g.setFont (VintageLookAndFeel::mono (9.0f, false));
        g.drawText (sub, r.getX() + 12, r.getY() + 24, r.getWidth() - 24, 12, juce::Justification::centredLeft);
    };

    if (page == 0)
    {
        ivoryPanel (kGridPan, "GRADE DO GROOVE", u8 ("1 COMPASSO \xc2\xb7 CAIXA NO 2 E NO 4 \xc2\xb7 CLIQUE: ACENTO \xe2\x86\x92 FANTASMA \xe2\x86\x92 VAZIO"));
        const int x = kGridPan.getX() + 14;
        smallLabel ("ATALHOS DE FEEL", x, 434);
        smallLabel (u8 ("M\xc3\x83O ESQUERDA NO"), x + 312, 434);
        smallLabel ("BATIDA", x, 480);
        smallLabel (u8 ("DURA\xc3\x87\xc3\x83O DA NOTA"), x + 312, 480);
        ivoryPanel (kSide, "CAMADAS", "");
        smallLabel ("FILLS NO ESTILO DE", kSide.getX() + 14, 396);
        smallLabel ("FILL A CADA (COMPASSOS)", kSide.getX() + 14, 450);
        smallLabel ("ACORDES", kSide.getX() + 14, 504);
        g.setFont (VintageLookAndFeel::mono (8.5f, false));
        g.setColour (brownInk.withAlpha (0.6f));
        g.drawText (u8 ("DO PLUGIN: ABA MIDI  \xc2\xb7  DO TECLADO: SEGURE O ACORDE"), kSide.getX() + 14, 550, 280, 12, juce::Justification::centredLeft);
    }
    else if (page == 1)
    {
        ivoryPanel (kContent, "ACORDES E MIDI", u8 ("EDITE NO VISOR \xc2\xb7 ARRASTE PARA A DAW (CANAL 1 BUBBLE, 2 CHOP, 3 FILLS)"));
        smallLabel ("PINCEL (NOTA NOVA)", kContent.getX() + 14, 572);
        smallLabel ("ARRASTE PARA A DAW", kContent.getX() + 300, 572);
    }
    else if (page == 3)
    {
        ivoryPanel (kGridPan, "EQUALIZADOR", u8 ("SA\xc3\x8d" "DA DO \xc3\x93RG\xc3\x83O \xc2\xb7 ARRASTE OS PONTOS \xc2\xb7 RODA = Q DO M\xc3\x89" "DIO"));
        const int x = kGridPan.getX() + 14, y = kGridPan.getY() + 252;
        smallLabel ("CORTE", x + 70, y);
        smallLabel (u8 ("SA\xc3\x8d" "DA"), x + 12, y + 34);
        smallLabel ("GRAVES", x + 168, y);
        smallLabel (u8 ("M\xc3\x89" "DIOS"), x + 318, y);
        smallLabel ("AGUDOS", x + 498, y);
        g.setColour (brownInk.withAlpha (0.25f));
        for (int gx : { x + 146, x + 296, x + 476 }) g.drawVerticalLine (gx, (float) y, (float) y + 96);
        smallLabel ("PRESETS DO EQ", x, kGridPan.getY() + 362);
        ivoryPanel (kSide, "TIMBRES MIDNITE", "RON BENJAMIN");
    }
    else
    {
        consolePanel (kBubCon, u8 ("M\xc3\x83O ESQUERDA \xc2\xb7 BUBBLE"), u8 ("SECO E GRAVE: S\xc3\x93 O 8' E UM POUCO DE 4'"));
        consolePanel (kChopCon, u8 ("M\xc3\x83O DIREITA \xc2\xb7 CHOP + FILLS"), u8 ("CHEIO (888), PERC E ECO  \xc2\xb7  LESLIE \xe2\x86\x92"));
        g.setColour (ivory);
        g.setFont (VintageLookAndFeel::mono (11.0f));
        g.drawText ("LESLIE", kLeslie.getX(), kLeslie.getY() + 8, 150, 14, juce::Justification::centredLeft);
        g.setColour (ivoryDark.withAlpha (0.7f));
        g.setFont (VintageLookAndFeel::mono (9.0f, false));
        g.drawText ("CORNETA + TAMBOR", kLeslie.getX(), kLeslie.getY() + 24, 150, 12, juce::Justification::centredLeft);
        g.setColour (ivoryDark.withAlpha (0.25f));
        g.drawVerticalLine (kLeslie.getX() - 8, (float) kLeslie.getY() + 10, (float) kLeslie.getBottom() - 10);
        ivoryPanel (kSound, "SOM", "");
        smallLabel (u8 ("INSTRUMENTO (DRAWBARS S\xc3\x93 NO B3)"), kSound.getX() + 14, kSound.getY() + 358, 230);
    }

    g.setColour (brownInk);
    g.fillRoundedRectangle (kKeysBed.toFloat(), 6.0f);
    g.setColour (juce::Colour (0xff7a1a14));
    g.fillRect (34, 646, 932, 5);   // feltro
    g.setColour (juce::Colour (0xffa89a82));
    g.setFont (VintageLookAndFeel::mono (9.0f, false));
    g.drawText ("AS TECLAS", 34, 690, 110, 12, juce::Justification::centred);
    g.drawText ("ACENDEM COM", 34, 704, 110, 12, juce::Justification::centred);
    g.drawText ("O GROOVE", 34, 718, 110, 12, juce::Justification::centred);
}

void ReggaeOrganEditor::paint (juce::Graphics& g)
{
    if (background.isNull() || backgroundPage != page || backgroundLook != look)
    {
        background = juce::Image (juce::Image::RGB, kW, kH, true);
        juce::Graphics bg (background);
        renderBackground (bg);
        backgroundPage = page;
        backgroundLook = look;
    }
    g.drawImageAt (background, 0, 0);
}

void ReggaeOrganEditor::resized()
{
    // cabeçalho
    grooveBtn.setBounds (628, 23, 104, 34);
    playBtn.setBounds (742, 23, 90, 34);
    placeKnob (bpm, { 900, 34 }, 30, 60);
    bpm.label.setBounds (928, 27, 44, 14);

    // músicos, época, riddim, jackpot, abas
    {
        const int mw = (kMus.getWidth() - 7 * 6) / 8;
        for (int i = 0; i < mro::NumMusicians; ++i)
            musBtn[i].setBounds (kMus.getX() + i * (mw + 6), kMus.getY() + 10, mw, 30);
    }
    epochBox.setBounds (24, 142, 230, 32);
    riddim.setBounds (262, 140, 566, 36);
    jackpot.setBounds (836, 142, 140, 32);
    layoutChips (tabs, { 24, 184, 400, 24 }, 6);
    status.setBounds (434, 184, 542, 24);

    // aba GROOVE
    {
        const int x = kGridPan.getX() + 14, w = kGridPan.getWidth() - 28;
        grid.setBounds (x, kGridPan.getY() + 32, w, 184);
        layoutChips (feel,    { x, 448, 300, 24 }, 4);
        layoutChips (density, { x + 312, 448, w - 312, 24 }, 4);
        layoutChips (beat,    { x, 494, 300, 24 }, 4);
        layoutChips (nota,    { x + 312, 494, w - 312, 24 }, 4);
        Knob* ks[4] = { &swing, &human, &ghost, &seed };
        for (int i = 0; i < 4; ++i) placeKnob (*ks[i], { x + 60 + i * 150, 572 }, 44, 90);

        const int sx = kSide.getX() + 14, sw = kSide.getWidth() - 28;
        layerBtn[0].setBounds (sx, kSide.getY() + 34, 120, 30);
        placeKnob (bubbleLvl, { kSide.getRight() - 56, kSide.getY() + 52 }, 38, 70);
        layerBtn[1].setBounds (sx, kSide.getY() + 90, 120, 30);
        placeKnob (chopLvl, { kSide.getRight() - 56, kSide.getY() + 108 }, 38, 70);
        layerBtn[2].setBounds (sx, kSide.getY() + 146, 120, 30);
        layoutChips (fillStyle, { sx, 410, sw, 24 }, 4);
        layoutChips (fillEvery, { sx, 464, sw, 24 }, 4);
        layoutChips (chordSrc,  { sx, 518, sw, 24 }, 4);
    }

    // aba MIDI
    {
        const int x = kContent.getX() + 14, w = kContent.getWidth() - 28;
        const int colW = (w - 3 * 12) / 4;
        for (int i = 0; i < ReggaeOrganProcessor::numSlots; ++i)
        {
            const int cx = x + i * (colW + 12), y = kContent.getY() + 32;
            slotOn[i].setBounds (cx, y, 104, 24);
            rootBox[i].setBounds (cx + 108, y, colW - 108, 24);
            majBtn[i].setBounds (cx, y + 28, 50, 22);
            minBtn[i].setBounds (cx + 52, y + 28, 50, 22);
            durBox[i].setBounds (cx + 108, y + 28, colW - 108, 22);
        }
        roll.setBounds (x, kContent.getY() + 88, w, 268);
        layoutChips (brush, { x, 586, 270, 26 }, 4);
        dragAll->setBounds (x + 286, 586, 160, 26);
        dragLayer[0]->setBounds (x + 452, 586, 90, 26);
        dragLayer[1]->setBounds (x + 548, 586, 90, 26);
        dragLayer[2]->setBounds (x + 644, 586, 90, 26);
        repsBox.setBounds (x + 740, 586, 54, 26);
        backToGroove.setBounds (x + w - 128, 586, 128, 26);
    }

    // aba ÓRGÃO
    for (int i = 0; i < 9; ++i)
    {
        bubbleDb[i]->setBounds (kBubCon.getX() + 8 + i * kDbPitch, kBubCon.getY() + 52, kDbPitch, 330);
        chopDb[i]->setBounds (kChopCon.getX() + 8 + i * kDbPitch, kChopCon.getY() + 52, kDbPitch, 330);
    }
    {
        const int sx = kSound.getX() + 14, sw = kSound.getWidth() - 28;
        percBtn.setBounds (sx, kSound.getY() + 34, 60, 30);
        echoBtn.setBounds (sx + 62, kSound.getY() + 34, 64, 30);
        maxBtn.setBounds (sx + 128, kSound.getY() + 34, 56, 30);
        vu.setBounds (sx + 12, kSound.getY() + 82, sw - 24, 64);
        Knob* r1[4] = { &drive, &click, &reverb, &master };
        for (int i = 0; i < 4; ++i) placeKnob (*r1[i], { sx + 21 + i * 48, kSound.getY() + 196 }, 34, 48);
        Knob* r2[3] = { &echoVol, &echoInt, &echoRate };
        for (int i = 0; i < 3; ++i) placeKnob (*r2[i], { sx + 45 + i * 48, kSound.getY() + 286 }, 34, 48);
        layoutChips (instr, { sx, kSound.getY() + 372, sw, 24 }, 2);
    }
    {
        const int lx = kLeslie.getX(), ly = kLeslie.getY(), lw = kLeslie.getWidth() - 10;
        cabinet.setBounds (lx, ly + 40, lw, 176);
        for (int i = 0; i < mro::NumLesliePresets; ++i)
            lesPreset.buttons[i]->setBounds (lx + (i % 2) * (lw / 2 + 2), ly + 222 + (i / 2) * 26, lw / 2 - 2, 23);
        layoutChips (lesSpeed, { lx, ly + 302, lw, 23 }, 4);
        lesBrake.setBounds (lx, ly + 340, lw / 2 - 4, 23);
        placeKnob (lesDepth, { lx + lw * 3 / 4, ly + 348 }, 30, 70);
        lesRead.setBounds (lx - 4, ly + 388, lw + 8, 14);
    }

    // aba EQ
    {
        const int x = kGridPan.getX() + 14, w = kGridPan.getWidth() - 28, y = kGridPan.getY() + 252;
        eqCurve.setBounds (x, kGridPan.getY() + 32, w, 212);
        eqOnBtn.setBounds (x, y + 2, 56, 30);
        placeKnob (eqOut, { x + 28, y + 62 }, 34, 56);
        placeKnob (eqHpf,  { x + 96, y + 44 }, 38, 60);
        placeKnob (eqLowG, { x + 186, y + 44 }, 38, 60);
        placeKnob (eqLowF, { x + 252, y + 44 }, 38, 60);
        placeKnob (eqMidG, { x + 330, y + 44 }, 38, 60);
        placeKnob (eqMidF, { x + 384, y + 44 }, 38, 60);
        placeKnob (eqMidQ, { x + 436, y + 44 }, 38, 50);
        placeKnob (eqHighG, { x + 520, y + 44 }, 38, 60);
        placeKnob (eqHighF, { x + 580, y + 44 }, 38, 60);
        layoutChips (eqPresets, { x, kGridPan.getY() + 376, w, 26 }, 4);
        const int sx = kSide.getX() + 14, sw = kSide.getWidth() - 28;
        for (int i = 0; i < mro::NumMidniteTimbres; ++i)
            midniteChips.buttons[i]->setBounds (sx + (i % 2) * (sw / 2 + 3), kSide.getY() + 34 + (i / 2) * 34, sw / 2 - 3, 30);
        midniteInfo.setBounds (sx - 4, kSide.getY() + 110, sw + 8, kSide.getHeight() - 120);
    }

    keyboard.setBounds (152, 653, 812, 127);
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f);
}
