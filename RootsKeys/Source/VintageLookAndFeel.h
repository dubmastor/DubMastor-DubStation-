// VintageLookAndFeel.h — visual retrô anos 70/80 para o Roots Keys
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vintage
{
const juce::Colour wood      { 0xff4a2c17 };
const juce::Colour woodDark  { 0xff2e1a0d };
const juce::Colour cream     { 0xffefe4c8 };
const juce::Colour mustard   { 0xffd9a23a };
const juce::Colour orange    { 0xffc8562a };
const juce::Colour brown     { 0xff5b3a24 };
const juce::Colour charcoal  { 0xff262422 };
const juce::Colour panelDark { 0xff1c1a18 };
const juce::Colour leather   { 0xff3b2618 };
const juce::Colour lcdBg     { 0xffa9c07c };
const juce::Colour lcdText   { 0xff1f2b14 };
const juce::Colour red       { 0xffb3261e };
const juce::Colour silver    { 0xffb9b2a3 };
} // namespace vintage

class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel()
    {
        using namespace vintage;
        setColour (juce::Slider::textBoxTextColourId, cream.withAlpha (0.8f));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, cream);
        setColour (juce::ComboBox::backgroundColourId, lcdBg);
        setColour (juce::ComboBox::textColourId, lcdText);
        setColour (juce::ComboBox::arrowColourId, lcdText);
        setColour (juce::ComboBox::outlineColourId, lcdText);
        setColour (juce::PopupMenu::backgroundColourId, panelDark);
        setColour (juce::PopupMenu::textColourId, cream);
        setColour (juce::PopupMenu::headerTextColourId, mustard);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, mustard);
        setColour (juce::PopupMenu::highlightedTextColourId, panelDark);
        setColour (juce::ToggleButton::textColourId, cream);
    }

    static juce::Font monoFont (float size)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::bold));
    }

    // Knob com saia e marcações (estilo amplificador/sintetizador vintage)
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        using namespace vintage;
        const float radius = (float) juce::jmin (w, h) * 0.5f - 9.0f;
        if (radius < 6.0f) return;
        const juce::Point<float> c ((float) x + (float) w * 0.5f, (float) y + (float) h * 0.5f);
        auto pointAt = [c] (float angle, float r)
        { return c.translated (r * std::sin (angle), -r * std::cos (angle)); };

        // marcações 0–10
        g.setColour (cream.withAlpha (0.75f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            g.drawLine (juce::Line<float> (pointAt (a, radius + 3.0f), pointAt (a, radius + (i % 5 == 0 ? 8.0f : 6.0f))),
                        i % 5 == 0 ? 1.6f : 1.0f);
        }

        // saia
        g.setColour (charcoal);
        g.fillEllipse (juce::Rectangle<float> (radius * 2, radius * 2).withCentre (c));
        g.setColour (silver.withAlpha (0.7f));
        g.drawEllipse (juce::Rectangle<float> (radius * 2, radius * 2).withCentre (c), 1.5f);

        // capa do knob
        const float capR = radius * 0.66f;
        g.setColour (juce::Colour (0xff121110));
        g.fillEllipse (juce::Rectangle<float> (capR * 2, capR * 2).withCentre (c));
        g.setColour (juce::Colour (0xff3a3430));
        g.drawEllipse (juce::Rectangle<float> (capR * 2, capR * 2).withCentre (c), 1.0f);

        // ponteiro
        const float a = startAngle + pos * (endAngle - startAngle);
        g.setColour (orange);
        g.drawLine (juce::Line<float> (pointAt (a, radius * 0.22f), pointAt (a, radius * 0.97f)), 3.0f);
    }

    // Drawbars: haste creme numerada + tampa colorida
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
        const float rodW = 10.0f;

        // fenda
        g.setColour (juce::Colour (0xff0b0907));
        g.fillRoundedRectangle (cx - rodW * 0.5f - 2.0f, (float) y, rodW + 4.0f, (float) h, 3.0f);

        // haste exposta (da tampa até o fundo) com números 1–8
        const float bottom = (float) (y + h);
        g.setColour (cream.darker (0.1f));
        g.fillRect (cx - rodW * 0.5f, sliderPos, rodW, bottom - sliderPos);
        g.setColour (brown);
        g.setFont (monoFont (9.0f));
        const float step = (minPos - maxPos) / 8.0f;
        for (int i = 1; i <= 8; ++i)
        {
            const float ty = minPos - step * (float) i + step * 0.5f;
            if (ty > sliderPos + 6.0f && ty < bottom - 2.0f)
                g.drawText (juce::String (i), juce::Rectangle<float> (cx - rodW * 0.5f, ty - 5.0f, rodW, 10.0f),
                            juce::Justification::centred);
        }

        // tampa
        const juce::Colour tab = s.findColour (juce::Slider::thumbColourId);
        const float tabW = juce::jmin ((float) w - 4.0f, 30.0f);
        auto tabRect = juce::Rectangle<float> (tabW, 18.0f).withCentre ({ cx, sliderPos });
        g.setColour (tab);
        g.fillRoundedRectangle (tabRect, 4.0f);
        g.setColour (tab.brighter (0.4f));
        g.drawLine (tabRect.getX() + 4.0f, tabRect.getY() + 4.0f, tabRect.getRight() - 4.0f, tabRect.getY() + 4.0f, 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (tabRect, 4.0f, 1.0f);
    }

    // Chave basculante (rocker) vermelha
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool) override
    {
        using namespace vintage;
        const bool on = b.getToggleState();
        auto r = b.getLocalBounds().toFloat();
        auto sw = r.removeFromLeft (46.0f).reduced (2.0f, 3.0f);

        g.setColour (juce::Colour (0xff0b0907));
        g.fillRoundedRectangle (sw, 3.0f);
        auto half = on ? sw.withTrimmedRight (sw.getWidth() * 0.45f) : sw.withTrimmedLeft (sw.getWidth() * 0.45f);
        g.setColour (on ? red : juce::Colour (0xff4a3f38));
        g.fillRoundedRectangle (half.reduced (1.5f), 2.5f);
        g.setColour (cream.withAlpha (on ? 1.0f : 0.5f));
        g.setFont (monoFont (10.0f));
        g.drawText (on ? "ON" : "OFF", half, juce::Justification::centred);

        g.setColour (cream.withAlpha (highlighted ? 1.0f : 0.9f));
        g.setFont (monoFont (13.0f));
        g.drawText (b.getButtonText().toUpperCase(), r.withTrimmedLeft (8.0f), juce::Justification::centredLeft);
    }

    // ComboBox como visor LCD esverdeado (anos 80)
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        using namespace vintage;
        auto r = juce::Rectangle<float> ((float) w, (float) h);
        g.setColour (juce::Colour (0xff0b0907));
        g.fillRoundedRectangle (r, 3.0f);
        auto lcd = r.reduced (2.5f);
        g.setColour (lcdBg);
        g.fillRoundedRectangle (lcd, 2.0f);
        g.setColour (lcdText.withAlpha (0.25f));
        g.drawRoundedRectangle (lcd.reduced (1.0f), 2.0f, 1.0f);

        juce::Path arrow;
        const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
        g.setColour (lcdText);
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return monoFont (14.0f); }
    juce::Font getPopupMenuFont() override { return monoFont (14.0f); }
};
