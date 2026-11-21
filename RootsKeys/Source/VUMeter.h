// VUMeter.h — VU analógico com ponteiro e balística clássica de 300 ms
#pragma once

#include "VintageLookAndFeel.h"

class VUMeter : public juce::Component
{
public:
    explicit VUMeter (juce::String channelName) : name (std::move (channelName)) {}

    // meanSquare = média quadrática do sinal; chamado ~30x por segundo pela interface
    void update (float meanSquare, float dtSeconds)
    {
        // 0 VU = -18 dBFS RMS (padrão de estúdio)
        const float db = 10.0f * std::log10 (meanSquare + 1.0e-12f) + 18.0f;
        const float target = dbToPos (juce::jlimit (-22.0f, 4.0f, db));
        const float coef = 1.0f - std::exp (-dtSeconds / 0.3f);
        needle += (target - needle) * coef;
        peakLed = db > 2.0f ? 1.0f : peakLed * 0.85f;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace vintage;
        auto b = getLocalBounds().toFloat();

        // moldura preta
        g.setColour (juce::Colour (0xff0b0907));
        g.fillRoundedRectangle (b, 5.0f);
        auto face = b.reduced (5.0f);

        // mostrador creme iluminado
        g.setColour (juce::Colour (0xfff3e2b6));
        g.fillRoundedRectangle (face, 3.0f);

        const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.35f);
        const float rArc = face.getHeight() * 1.05f;
        const float aMin = -0.78f, aMax = 0.78f;   // radianos
        auto posToAngle = [&] (float pos) { return aMin + pos * (aMax - aMin); };
        auto pt = [&] (float a, float r) { return pivot.translated (r * std::sin (a), -r * std::cos (a)); };

        // arco: preto até 0 VU, vermelho acima
        const float zeroPos = dbToPos (0.0f);
        juce::Path black, red;
        black.addCentredArc (pivot.x, pivot.y, rArc, rArc, 0.0f, posToAngle (0.0f), posToAngle (zeroPos), true);
        red.addCentredArc (pivot.x, pivot.y, rArc, rArc, 0.0f, posToAngle (zeroPos), posToAngle (1.0f), true);
        g.setColour (juce::Colour (0xff2a2420));
        g.strokePath (black, juce::PathStrokeType (1.5f));
        g.setColour (vintage::red);
        g.strokePath (red, juce::PathStrokeType (4.0f));

        // marcações
        const float marks[] = { -20, -10, -7, -5, -3, -1, 0, 1, 2, 3 };
        g.setFont (VintageLookAndFeel::monoFont (9.0f));
        for (float m : marks)
        {
            const float a = posToAngle (dbToPos (m));
            g.setColour (m > 0 ? vintage::red : juce::Colour (0xff2a2420));
            g.drawLine (juce::Line<float> (pt (a, rArc), pt (a, rArc + 5.0f)), 1.2f);
            if (m == -20 || m == -10 || m == -5 || m == 0 || m == 3)
            {
                const auto lp = pt (a, rArc + 12.0f);
                g.drawText (m > 0 ? "+" + juce::String ((int) m) : juce::String ((int) m),
                            juce::Rectangle<float> (24.0f, 10.0f).withCentre (lp), juce::Justification::centred);
            }
        }

        g.setColour (juce::Colour (0xff2a2420));
        g.setFont (VintageLookAndFeel::monoFont (13.0f));
        g.drawText ("VU", face.withTrimmedTop (face.getHeight() * 0.45f).withTrimmedBottom (4.0f), juce::Justification::centred);
        g.setFont (VintageLookAndFeel::monoFont (9.0f));
        g.drawText (name, face.reduced (6.0f, 3.0f), juce::Justification::bottomLeft);

        // LED de pico
        g.setColour (vintage::red.withAlpha (0.25f + 0.75f * peakLed));
        g.fillEllipse (face.getRight() - 12.0f, face.getBottom() - 12.0f, 7.0f, 7.0f);

        // ponteiro
        g.saveState();
        g.reduceClipRegion (face.toNearestInt());
        const float a = posToAngle (needle);
        g.setColour (juce::Colour (0xff141210));
        g.drawLine (juce::Line<float> (pt (a, face.getHeight() * 0.3f), pt (a, rArc + 6.0f)), 1.6f);
        g.restoreState();

        // vidro
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.35f), 3.0f);
    }

private:
    juce::String name;
    float needle = 0.0f, peakLed = 0.0f;

    // escala VU é quase linear em tensão
    static float dbToPos (float db)
    {
        const float lo = std::pow (10.0f, -20.0f / 20.0f), hi = std::pow (10.0f, 3.0f / 20.0f);
        return juce::jlimit (0.0f, 1.0f, (std::pow (10.0f, db / 20.0f) - lo) / (hi - lo));
    }
};
