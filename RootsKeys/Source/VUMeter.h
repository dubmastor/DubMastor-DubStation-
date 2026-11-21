// VUMeter.h — VU analógico com ponteiro e balística clássica de 300 ms
// As cores do mostrador mudam com o visual (verde no piano anos 80, marfim no órgão)
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VUMeter : public juce::Component
{
public:
    explicit VUMeter (juce::String channelName) : name (std::move (channelName)) {}

    void setColours (juce::Colour faceColour, juce::Colour inkColour, juce::Colour bezelColour)
    {
        face = faceColour; ink = inkColour; bezel = bezelColour;
        repaint();
    }

    // meanSquare = média quadrática do sinal; chamado ~30x por segundo pela interface
    void update (float meanSquare, float dtSeconds)
    {
        // 0 VU = -18 dBFS RMS (padrão de estúdio)
        const float db = 10.0f * std::log10 (meanSquare + 1.0e-12f) + 18.0f;
        const float target = dbToPos (juce::jlimit (-22.0f, 4.0f, db));
        const float coef = 1.0f - std::exp (-dtSeconds / 0.3f);
        const float old = needle;
        needle += (target - needle) * coef;
        const float oldLed = peakLed;
        peakLed = db > 2.0f ? 1.0f : peakLed * 0.85f;
        if (std::abs (old - needle) > 0.0005f || std::abs (oldLed - peakLed) > 0.01f)
            repaint();
    }

    void setNeedle (float pos) { needle = pos; repaint(); }   // usado nas capturas de tela

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const juce::Colour red (0xffb3261e);

        g.setColour (bezel);
        g.fillRoundedRectangle (b, 5.0f);
        auto f = b.reduced (5.0f);
        g.setColour (face);
        g.fillRoundedRectangle (f, 3.0f);

        const juce::Point<float> pivot (f.getCentreX(), f.getBottom() + f.getHeight() * 0.4f);
        const float rArc = f.getHeight() * 1.08f;
        auto posToAngle = [] (float p) { return -0.75f + p * 1.5f; };
        auto pt = [&] (float a, float r) { return pivot.translated (r * std::sin (a), -r * std::cos (a)); };

        const float zeroPos = dbToPos (0.0f);
        juce::Path black, redArc;
        black.addCentredArc (pivot.x, pivot.y, rArc, rArc, 0.0f, posToAngle (0.0f), posToAngle (zeroPos), true);
        redArc.addCentredArc (pivot.x, pivot.y, rArc, rArc, 0.0f, posToAngle (zeroPos), posToAngle (1.0f), true);
        g.setColour (ink);
        g.strokePath (black, juce::PathStrokeType (1.3f));
        g.setColour (red);
        g.strokePath (redArc, juce::PathStrokeType (3.5f));

        const float marks[] = { -20, -10, -7, -5, -3, -1, 0, 1, 2, 3 };
        g.setFont (juce::FontOptions (9.0f));
        for (float m : marks)
        {
            const float a = posToAngle (dbToPos (m));
            g.setColour (m > 0 ? red : ink);
            g.drawLine (juce::Line<float> (pt (a, rArc), pt (a, rArc + 4.0f)), 1.1f);
            if (m == -20 || m == -10 || m == -5 || m == 0 || m == 3)
                g.drawText (m > 0 ? "+" + juce::String ((int) m) : juce::String ((int) m),
                            juce::Rectangle<float> (24.0f, 10.0f).withCentre (pt (a, rArc + 11.0f)),
                            juce::Justification::centred);
        }

        g.setColour (ink);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("VU", f.withTrimmedTop (f.getHeight() * 0.5f).withTrimmedBottom (3.0f), juce::Justification::centred);
        g.setFont (juce::FontOptions (9.0f));
        g.drawText (name, f.reduced (5.0f, 2.0f), juce::Justification::bottomLeft);

        g.setColour (red.withAlpha (0.25f + 0.75f * peakLed));
        g.fillEllipse (f.getRight() - 11.0f, f.getBottom() - 11.0f, 7.0f, 7.0f);

        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (f.toNearestInt());
            const float a = posToAngle (needle);
            g.setColour (juce::Colour (0xff141210));
            g.drawLine (juce::Line<float> (pt (a, f.getHeight() * 0.55f), pt (a, rArc + 4.0f)), 1.5f);
        }

        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRoundedRectangle (f.withHeight (f.getHeight() * 0.35f), 3.0f);
    }

private:
    juce::String name;
    juce::Colour face { 0xfff3e2b6 }, ink { 0xff2a2420 }, bezel { 0xff0b0907 };
    float needle = 0.0f, peakLed = 0.0f;

    static float dbToPos (float db)
    {
        const float lo = std::pow (10.0f, -20.0f / 20.0f), hi = std::pow (10.0f, 3.0f / 20.0f);
        return juce::jlimit (0.0f, 1.0f, (std::pow (10.0f, db / 20.0f) - lo) / (hi - lo));
    }
};
