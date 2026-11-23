// VintageLookAndFeel.h — dois visuais para o Dub Station Roots Keys
//  skin 0 = PIANO: teclado anos 80 em plástico preto texturizado, visor LCD verde
//  skin 1 = ORGAN: gabinete de madeira estilo Hammond com detalhes em marfim
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vintage
{
// piano anos 80
const juce::Colour plasticText { 0xffdcdcdc };
const juce::Colour plasticMuted { 0xff8c8c90 };
const juce::Colour plasticBtn  { 0xff242426 };
const juce::Colour ledRed      { 0xffff3b30 };
const juce::Colour ledOff      { 0xff4a1a17 };
const juce::Colour lcdBg       { 0xff8fb96a };
const juce::Colour lcdInk      { 0xff1d2b12 };
const juce::Colour keyGreen    { 0xff3ad07a };
// órgão madeira + marfim
const juce::Colour ivory       { 0xffefe6cf };
const juce::Colour ivoryDark   { 0xffd9ccad };
const juce::Colour brownInk    { 0xff3e2110 };
const juce::Colour console     { 0xff15110d };
const juce::Colour knobBlack   { 0xff1a1512 };
const juce::Colour rockerRed   { 0xffa32a1e };
const juce::Colour keyOrange   { 0xffd9752e };
} // namespace vintage

class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    int skin = 0;   // 0 piano, 1 órgão

    static juce::Font mono (float size, bool bold = true)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size,
                                              bold ? juce::Font::bold : juce::Font::plain));
    }

    void setSkin (int s)
    {
        using namespace vintage;
        skin = s;
        const bool p = (s == 0);
        setColour (juce::Label::textColourId, p ? plasticText : ivory);
        setColour (juce::ComboBox::textColourId, p ? lcdInk : ivory);
        setColour (juce::ComboBox::backgroundColourId, p ? lcdBg : knobBlack);
        setColour (juce::ComboBox::arrowColourId, p ? lcdInk : ivory);
        setColour (juce::PopupMenu::backgroundColourId, p ? juce::Colour (0xff1b1b1d) : juce::Colour (0xff2a1a0e));
        setColour (juce::PopupMenu::textColourId, p ? plasticText : ivory);
        setColour (juce::PopupMenu::headerTextColourId, p ? lcdBg : ivoryDark);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, p ? lcdBg : ivory);
        setColour (juce::PopupMenu::highlightedTextColourId, p ? lcdInk : brownInk);
        setColour (juce::BubbleComponent::backgroundColourId, p ? lcdBg : ivory);
        setColour (juce::BubbleComponent::outlineColourId, p ? lcdInk : brownInk);
        setColour (juce::TooltipWindow::textColourId, p ? lcdInk : brownInk);
    }

    // ---------------------------------------------------------------- knobs
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        using namespace vintage;
        const float radius = (float) juce::jmin (w, h) * 0.5f - 6.0f;
        if (radius < 5.0f) return;
        const juce::Point<float> c ((float) x + (float) w * 0.5f, (float) y + (float) h * 0.5f);
        auto pt = [c] (float a, float r) { return c.translated (r * std::sin (a), -r * std::cos (a)); };

        const auto tick = s.findColour (juce::Slider::rotarySliderOutlineColourId);
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            g.setColour (tick.withAlpha (i % 5 == 0 ? 0.9f : 0.6f));
            g.drawLine (juce::Line<float> (pt (a, radius + 1.5f), pt (a, radius + (i % 5 == 0 ? 5.5f : 4.0f))), 1.1f);
        }
        const float r = radius - 1.0f;
        const float a = startAngle + pos * (endAngle - startAngle);
        if (skin == 0)
        {
            // botão preto canelado anos 80
            g.setColour (juce::Colour (0xff050506));
            g.fillEllipse (juce::Rectangle<float> (r * 2 + 4, r * 2 + 4).withCentre (c));
            g.setColour (juce::Colour (0xff2a2a2c));
            g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
            g.setColour (juce::Colour (0xff1c1c1e));
            for (int i = 0; i < 24; ++i)
            {
                const float b = (float) i / 24.0f * juce::MathConstants<float>::twoPi;
                g.drawLine (juce::Line<float> (pt (b, r * 0.82f), pt (b, r)), 1.0f);
            }
            g.setColour (juce::Colour (0xff4a4a4e));
            g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), 1.0f);
            g.setColour (juce::Colour (0xff323235));
            g.fillEllipse (juce::Rectangle<float> (r * 1.5f, r * 1.5f).withCentre (c));
            g.setColour (juce::Colour (0xfff2f2f2));
            g.drawLine (juce::Line<float> (pt (a, r * 0.2f), pt (a, r * 0.95f)), 2.0f);
        }
        else
        {
            g.setColour (knobBlack);
            g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
            g.setColour (juce::Colour (0xff2a231e));
            g.fillEllipse (juce::Rectangle<float> (r * 1.25f, r * 1.25f).withCentre (c));
            g.setColour (ivory);
            g.drawLine (juce::Line<float> (pt (a, r * 0.22f), pt (a, r * 0.95f)), 2.5f);
        }
    }

    // ---------------------------------------------------------------- faders (piano anos 80)
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                           float minPos, float maxPos, juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, minPos, maxPos, style, s);
            return;
        }
        using namespace vintage;
        const float cx = (float) x + (float) w * 0.5f;
        const float top = (float) y, bottom = (float) (y + h);   // área do trilho (já descontado o cursor)
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (cx - 2.5f, top - 4.0f, 5.0f, bottom - top + 8.0f, 2.0f);
        g.setColour (juce::Colour (0xff6a6a6e));
        for (int t = 0; t <= 10; ++t)
        {
            const float ty = bottom - (float) t * (bottom - top) / 10.0f;
            g.drawLine (cx + 11.0f, ty, cx + (t % 5 ? 15.0f : 19.0f), ty, 1.0f);
            g.drawLine (cx - 11.0f, ty, cx - (t % 5 ? 15.0f : 19.0f), ty, 1.0f);
        }
        auto cap = juce::Rectangle<float> (26.0f, 16.0f).withCentre ({ cx, sliderPos });
        g.setColour (juce::Colour (0xff050506));
        g.fillRoundedRectangle (cap.translated (0, 1.5f), 2.0f);
        g.setColour (juce::Colour (0xff2e2e31));
        g.fillRoundedRectangle (cap, 2.0f);
        g.setColour (juce::Colour (0xff47474b));
        g.drawLine (cap.getX() + 2, cap.getY() + 1, cap.getRight() - 2, cap.getY() + 1, 1.0f);
        g.setColour (juce::Colour (0xfff2f2f2));
        g.drawLine (cap.getX() + 3, cap.getCentreY(), cap.getRight() - 3, cap.getCentreY(), 1.6f);
    }

    // ---------------------------------------------------------------- chaves
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool) override
    {
        using namespace vintage;
        const bool on = b.getToggleState();
        const auto style = b.getProperties()["style"].toString();
        auto r = b.getLocalBounds().toFloat();

        if (style == "led")   // botão de membrana com LED (anos 80)
        {
            auto box = r.reduced (1.0f);
            g.setColour (juce::Colours::black);
            g.fillRoundedRectangle (box.translated (0, 1.5f), 2.0f);
            g.setColour (on ? juce::Colour (0xff3a3a3d) : plasticBtn);
            g.fillRoundedRectangle (box, 2.0f);
            g.setColour (juce::Colours::white.withAlpha (highlighted ? 0.12f : 0.06f));
            g.drawLine (box.getX() + 2, box.getY() + 1, box.getRight() - 2, box.getY() + 1, 1.0f);
            g.setColour (on ? ledRed : ledOff);
            g.fillEllipse (box.getX() + 7.0f, box.getCentreY() - 3.0f, 6.0f, 6.0f);
            g.setColour (plasticText);
            g.setFont (mono (11.0f));
            g.drawText (b.getButtonText(), box.withTrimmedLeft (14.0f), juce::Justification::centred);
            return;
        }
        if (style == "tab")   // tab de percussão do Hammond (marfim)
        {
            auto slot = r.withHeight (r.getHeight() - 16.0f).reduced (2.0f, 0.0f);
            g.setColour (juce::Colour (0xff2a231e));
            g.fillRoundedRectangle (slot, 3.0f);
            auto tab = slot.reduced (3.0f).withHeight (slot.getHeight() * 0.52f);
            if (on) tab.setY (slot.getBottom() - 3.0f - tab.getHeight());
            g.setColour (on ? juce::Colour (0xfffbf7ec) : ivoryDark);
            g.fillRoundedRectangle (tab, 3.0f);
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.drawRoundedRectangle (tab, 3.0f, 1.0f);
            g.setColour (brownInk);
            g.setFont (mono (9.5f));
            g.drawText (b.getButtonText(), r.withTop (r.getBottom() - 14.0f), juce::Justification::centred);
            return;
        }
        if (style == "halfmoon")   // chave meia-lua da Leslie
        {
            auto area = r.withTrimmedBottom (16.0f);
            const float rad = juce::jmin (area.getWidth() * 0.5f, area.getHeight()) - 2.0f;
            const juce::Point<float> c (area.getCentreX(), area.getBottom());
            juce::Path half;
            half.addPieSegment (c.x - rad, c.y - rad, rad * 2, rad * 2, -juce::MathConstants<float>::halfPi,
                                juce::MathConstants<float>::halfPi, 0.0f);
            g.setColour (juce::Colour (0xff2a231e));
            g.fillPath (half);
            juce::Path lever;
            const float a0 = on ? 0.0f : -juce::MathConstants<float>::halfPi;
            lever.addPieSegment (c.x - rad, c.y - rad, rad * 2, rad * 2, a0, a0 + juce::MathConstants<float>::halfPi, 0.0f);
            g.setColour (juce::Colour (0xffb8862e));
            g.fillPath (lever);
            g.setFont (mono (9.5f));
            g.setColour (on ? juce::Colour (0xff8a7a62) : brownInk);
            g.drawText ("SLOW", r.withTop (r.getBottom() - 14.0f).withWidth (r.getWidth() * 0.5f), juce::Justification::centred);
            g.setColour (on ? brownInk : juce::Colour (0xff8a7a62));
            g.drawText ("FAST", r.withTop (r.getBottom() - 14.0f).withTrimmedLeft (r.getWidth() * 0.5f), juce::Justification::centred);
            return;
        }
        // "rocker": chave basculante vermelha (órgão)
        auto sw = r.removeFromLeft (20.0f).withSizeKeepingCentre (20.0f, juce::jmin (r.getHeight(), 34.0f));
        g.setColour (juce::Colour (0xff2a231e));
        g.fillRoundedRectangle (sw, 3.0f);
        auto half = sw.reduced (2.0f).withHeight (sw.getHeight() * 0.5f - 2.0f);
        if (! on) half.setY (sw.getCentreY());
        g.setColour (on ? rockerRed : juce::Colour (0xff5a4a40));
        g.fillRoundedRectangle (half, 2.0f);
        g.setColour (skin == 0 ? plasticText : brownInk);
        g.setFont (mono (11.0f));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (8.0f), juce::Justification::centredLeft);
    }

    // ---------------------------------------------------------------- botões de preset / modo
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down) override
    {
        using namespace vintage;
        const auto style = b.getProperties()["style"].toString();
        const bool on = b.getToggleState();
        auto r = b.getLocalBounds().toFloat();
        if (style == "reverseKey")   // teclas de preset invertidas do B3
        {
            g.setColour (juce::Colours::black);
            g.fillRect (r);
            g.setColour (on ? ivory : console);
            g.fillRect (r.reduced (1.0f).withTrimmedBottom (down ? 0.0f : 2.0f));
            return;
        }
        if (style == "chip")   // botão pequeno (notas, salvar, abas): aceso na cor de destaque
        {
            const bool p = (skin == 0);
            g.setColour (juce::Colours::black);
            g.fillRoundedRectangle (r.reduced (0.5f), 3.0f);
            const auto onCol = p ? lcdBg : keyOrange;
            const auto offCol = p ? juce::Colour (0xff2a2a2c) : juce::Colour (0xff2a231e);
            g.setColour ((on ? onCol : offCol).brighter (highlighted ? 0.12f : 0.0f).darker (down ? 0.2f : 0.0f));
            g.fillRoundedRectangle (r.reduced (1.5f), 2.5f);
            return;
        }
        if (style == "pill")   // chave de modo do órgão
        {
            g.setColour (on ? ivory : juce::Colours::transparentBlack);
            g.fillRoundedRectangle (r.reduced (2.0f), r.getHeight() * 0.5f - 2.0f);
            return;
        }
        // "membrane" — botão de plástico anos 80 com LED
        auto box = r.reduced (1.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (box.translated (0, 1.5f), 2.0f);
        g.setColour ((on ? juce::Colour (0xff3a3a3d) : plasticBtn).brighter (highlighted ? 0.08f : 0.0f).darker (down ? 0.2f : 0.0f));
        g.fillRoundedRectangle (box, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawLine (box.getX() + 2, box.getY() + 1, box.getRight() - 2, box.getY() + 1, 1.0f);
        g.setColour (on ? ledRed : ledOff);
        g.fillEllipse (box.getX() + 8.0f, box.getCentreY() - 3.0f, 6.0f, 6.0f);
        if (on)
        {
            g.setColour (ledRed.withAlpha (0.25f));
            g.fillEllipse (box.getX() + 5.0f, box.getCentreY() - 6.0f, 12.0f, 12.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        using namespace vintage;
        const auto style = b.getProperties()["style"].toString();
        const bool on = b.getToggleState();
        auto r = b.getLocalBounds().toFloat();
        g.setFont (mono (style == "reverseKey" ? 10.0f : 11.0f));
        if (style == "reverseKey")      g.setColour (on ? console : ivory);
        else if (style == "chip")       { g.setFont (mono (10.0f)); g.setColour (on ? (skin == 0 ? lcdInk : console) : (skin == 0 ? plasticText : ivory)); }
        else if (style == "pill")       g.setColour (on ? brownInk : juce::Colour (0xffb89a74));
        else                            { g.setColour (plasticText); r.removeFromLeft (14.0f); }
        g.drawText (b.getButtonText(), r, juce::Justification::centred);
    }

    // ---------------------------------------------------------------- caixas de seleção
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        using namespace vintage;
        auto r = juce::Rectangle<float> ((float) w, (float) h);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (r, 3.0f);
        auto in = r.reduced (2.5f);
        g.setColour (skin == 0 ? lcdBg : knobBlack);
        g.fillRoundedRectangle (in, 2.0f);
        juce::Path arrow;
        const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
        g.setColour (skin == 0 ? lcdInk : ivory);
        g.fillPath (arrow);
    }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return mono (12.0f); }
    juce::Font getPopupMenuFont() override { return mono (13.0f); }
};
