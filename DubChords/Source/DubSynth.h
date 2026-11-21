// Som interno simples para ouvir o riddim sem carregar outro instrumento
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

struct DubSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class DubVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<DubSound*> (s) != nullptr; }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        freq   = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        level  = velocity * 0.12f;
        isBass = midiNote < 50;
        phase  = 0.0;

        adsr.setSampleRate (getSampleRate());
        if (isBass) adsr.setParameters ({ 0.005f, 0.35f, 0.8f, 0.08f });
        else        adsr.setParameters ({ 0.002f, 0.16f, 0.30f, 0.05f });
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff) adsr.noteOff();
        else { adsr.reset(); clearCurrentNote(); }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! isVoiceActive()) return;

        const double inc = juce::MathConstants<double>::twoPi * freq / getSampleRate();

        for (int i = 0; i < numSamples; ++i)
        {
            const float env = adsr.getNextSample();
            const double p = phase;
            double s;
            if (isBass) s = std::sin (p) + 0.25 * std::sin (2.0 * p);
            else        s = std::sin (p) + 0.5 * std::sin (2.0 * p) + 0.3 * std::sin (3.0 * p) + 0.12 * std::sin (5.0 * p);

            const float v = (float) s * env * level * (isBass ? 2.2f : 1.0f);
            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.addSample (ch, startSample + i, v);

            phase += inc;
            if (phase > juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;

            if (! adsr.isActive()) { clearCurrentNote(); break; }
        }
    }

private:
    juce::ADSR adsr;
    double freq = 440.0, phase = 0.0;
    float level = 0.0f;
    bool isBass = false;
};
