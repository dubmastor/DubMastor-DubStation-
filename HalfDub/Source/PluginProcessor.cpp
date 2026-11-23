#include "PluginProcessor.h"
#include "PluginEditor.h"

const juce::StringArray& HalfDubProcessor::speedNames()
{
    static const juce::StringArray n { "1/1", "2/3", "1/2", "1/3", "1/4", "REV" };
    return n;
}
const juce::StringArray& HalfDubProcessor::bandSpeedNames()
{
    static const juce::StringArray n { "VEL", "1/1", "2/3", "1/2", "1/3", "1/4", "REV", "OFF" };
    return n;
}
const juce::StringArray& HalfDubProcessor::loopNames()
{
    static const juce::StringArray n { "1/16", "1/8", "1/4", "1/2", "1 BAR", "2 BARS", "4 BARS", "8 BARS" };
    return n;
}
const juce::StringArray& HalfDubProcessor::divisionNames()
{
    static const juce::StringArray n { "1/16", "1/8 T", "1/8", "1/8 D", "1/4", "1/4 D", "1/2" };
    return n;
}

HalfDubProcessor::HalfDubProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "HalfDub", createLayout())
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            params[rp->getParameterID().toStdString()] = apvts.getRawParameterValue (rp->getParameterID());
}

juce::AudioProcessorValueTreeState::ParameterLayout HalfDubProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    using Attr = AudioParameterFloatAttributes;
    auto pct = [] { return Attr().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; }); };
    auto db  = [] { return Attr().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; }); };
    auto hz  = [] { return Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; }); };
    auto beats = [] { return Attr().withStringFromValueFunction ([] (float v, int) { return v < 0.01f ? String ("imediato") : String (v, 2) + " tempos"; }); };
    auto logRange = [] (float lo, float hi, float centre) { NormalisableRange<float> r (lo, hi); r.setSkewForCentre (centre); return r; };
    auto add = [&p] (auto* x) { p.push_back (std::unique_ptr<RangedAudioParameter> (x)); };

    // --- DROP e velocidade (preset INIT: efeito ligado, 1/2, fita)
    add (new AudioParameterBool  (ParameterID { "drop", 1 }, "Drop", true));
    add (new AudioParameterBool  (ParameterID { "latch", 1 }, "Drop Travado", true));
    add (new AudioParameterChoice(ParameterID { "speed", 1 }, "Velocidade", speedNames(), 2));
    add (new AudioParameterChoice(ParameterID { "motor", 1 }, "Motor", StringArray { "Fita", "Groove" }, 0));
    add (new AudioParameterFloat (ParameterID { "pwrDown", 1 }, "Power Down", NormalisableRange<float> (0.0f, 4.0f), 0.0f, beats()));
    add (new AudioParameterFloat (ParameterID { "pwrUp", 1 }, "Power Up", NormalisableRange<float> (0.0f, 4.0f), 0.0f, beats()));
    add (new AudioParameterChoice(ParameterID { "smoothMode", 1 }, "Smooth Mode", StringArray { "Perc", "Sust", "FX", "Swell" }, 1));

    // --- BANDAS
    const char* bandNames[3] = { "Grave", "Medio", "Agudo" };
    for (int b = 0; b < 3; ++b)
    {
        const String n (bandNames[b]);
        add (new AudioParameterChoice(ParameterID { bandId (b, "Speed"), 1 }, n + " Velocidade", bandSpeedNames(), 0));
        add (new AudioParameterFloat (ParameterID { bandId (b, "Mix"), 1 }, n + " Mix", NormalisableRange<float> (0.0f, 100.0f), 100.0f, pct()));
        add (new AudioParameterFloat (ParameterID { bandId (b, "Smooth"), 1 }, n + " Smooth", NormalisableRange<float> (0.0f, 100.0f), 30.0f, pct()));
        add (new AudioParameterChoice(ParameterID { bandId (b, "Loop"), 1 }, n + " Loop", loopNames(), 4));
        add (new AudioParameterBool  (ParameterID { bandId (b, "Solo"), 1 }, n + " Solo", false));
    }

    // --- SOUND SYSTEM
    add (new AudioParameterBool  (ParameterID { "padRewind", 1 }, "Rewind", false));
    add (new AudioParameterBool  (ParameterID { "padStop", 1 }, "Tape Stop", false));
    add (new AudioParameterBool  (ParameterID { "padReverse", 1 }, "Reverse", false));
    add (new AudioParameterBool  (ParameterID { "padSpin", 1 }, "Spin-Back", false));
    add (new AudioParameterFloat (ParameterID { "padDur", 1 }, "Duracao", NormalisableRange<float> (0.0f, 100.0f), 50.0f, pct()));
    add (new AudioParameterBool  (ParameterID { "padSync", 1 }, "Pads Sync", true));

    // --- FITA
    add (new AudioParameterFloat (ParameterID { "wow", 1 }, "Wow/Flutter", NormalisableRange<float> (0.0f, 100.0f), 20.0f, pct()));
    add (new AudioParameterFloat (ParameterID { "sat", 1 }, "Saturacao", NormalisableRange<float> (0.0f, 100.0f), 30.0f, pct()));

    // --- ECO DE FITA
    add (new AudioParameterBool  (ParameterID { "echoOn", 1 }, "Eco Ligado", true));
    add (new AudioParameterChoice(ParameterID { "echoDiv", 1 }, "Eco Tempo", divisionNames(), 5));
    add (new AudioParameterFloat (ParameterID { "echoFb", 1 }, "Eco Intensidade", NormalisableRange<float> (0.0f, 110.0f), 45.0f, pct()));
    add (new AudioParameterFloat (ParameterID { "echoSend", 1 }, "Eco Envio", NormalisableRange<float> (0.0f, 100.0f), 25.0f, pct()));
    add (new AudioParameterFloat (ParameterID { "washout", 1 }, "Washout", NormalisableRange<float> (0.0f, 100.0f), 60.0f, pct()));
    add (new AudioParameterBool  (ParameterID { "echoPP", 1 }, "Ping Pong", false));
    add (new AudioParameterBool  (ParameterID { "echoMte", 1 }, "MTE-12 Tape", false));

    // --- EQ GRÁFICO (preset HPF)
    add (new AudioParameterBool  (ParameterID { "eqByp", 1 }, "EQ Bypass", false));
    add (new AudioParameterChoice(ParameterID { "eqPos", 1 }, "EQ Onde", StringArray { "Sinal lento", "Saida total" }, 0));
    add (new AudioParameterFloat (ParameterID { "eqGain", 1 }, "EQ Ganho", NormalisableRange<float> (-12.0f, 12.0f), 0.0f, db()));
    add (new AudioParameterFloat (ParameterID { "eqHp", 1 }, "EQ HPF", logRange (20.0f, 2000.0f, 200.0f), 90.0f, hz()));
    add (new AudioParameterFloat (ParameterID { "eqLp", 1 }, "EQ LPF", logRange (200.0f, 20000.0f, 2000.0f), 20000.0f, hz()));
    const float fDef[4] = { 100.0f, 500.0f, 2000.0f, 7000.0f }, qDef[4] = { 1.0f, 1.2f, 1.2f, 1.0f };
    for (int k = 0; k < 4; ++k)
    {
        const String s (k + 1);
        add (new AudioParameterFloat (ParameterID { "eqF" + s, 1 }, "EQ " + s + " Freq", logRange (20.0f, 20000.0f, 632.0f), fDef[k], hz()));
        add (new AudioParameterFloat (ParameterID { "eqG" + s, 1 }, "EQ " + s + " Ganho", NormalisableRange<float> (-12.0f, 12.0f), 0.0f, db()));
        add (new AudioParameterFloat (ParameterID { "eqQ" + s, 1 }, "EQ " + s + " Q", logRange (0.3f, 4.0f, 1.1f), qDef[k]));
    }

    // --- MASTOR BIG KNOB (começa em OFF)
    add (new AudioParameterFloat (ParameterID { "bkCut", 1 }, "Big Knob", NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    add (new AudioParameterBool  (ParameterID { "bkStep", 1 }, "Big Knob Stepped", true));
    add (new AudioParameterBool  (ParameterID { "bkLow", 1 }, "Big Knob Low Pass", false));
    add (new AudioParameterBool  (ParameterID { "bk24", 1 }, "Big Knob 24 dB", false));
    add (new AudioParameterFloat (ParameterID { "bkRes", 1 }, "Big Knob Ressonancia", NormalisableRange<float> (0.0f, 100.0f), 25.0f, pct()));
    add (new AudioParameterFloat (ParameterID { "bkDrive", 1 }, "Big Knob Drive", NormalisableRange<float> (0.0f, 100.0f), 0.0f, pct()));
    add (new AudioParameterFloat (ParameterID { "bkMix", 1 }, "Big Knob Mix", NormalisableRange<float> (0.0f, 100.0f), 100.0f, pct()));
    add (new AudioParameterBool  (ParameterID { "bkByp", 1 }, "Big Knob Bypass", false));

    // --- SAÍDA
    add (new AudioParameterFloat (ParameterID { "dryWet", 1 }, "Dry/Wet", NormalisableRange<float> (0.0f, 100.0f), 100.0f, pct()));
    add (new AudioParameterFloat (ParameterID { "out", 1 }, "Saida", NormalisableRange<float> (-24.0f, 12.0f), 0.0f, db()));
    add (new AudioParameterBool  (ParameterID { "bypass", 1 }, "Bypass", false));
    return { p.begin(), p.end() };
}

bool HalfDubProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    const auto in = layouts.getMainInputChannelSet();
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void HalfDubProcessor::prepareToPlay (double sampleRate, int)
{
    engine.setParams (readParams());
    engine.prepare (sampleRate);
    setLatencySamples (0);
}

hd::EngineParams HalfDubProcessor::readParams()
{
    hd::EngineParams e;
    e.drop = getP ("drop") > 0.5f || midiDrop.load();
    e.speed = juce::jlimit (0, 5, (int) getP ("speed"));
    e.groove = getP ("motor") > 0.5f;
    e.powerDownBeats = getP ("pwrDown");
    e.powerUpBeats = getP ("pwrUp");
    e.smoothMode = juce::jlimit (0, 3, (int) getP ("smoothMode"));
    for (int b = 0; b < 3; ++b)
    {
        e.bandSpeed[b] = juce::jlimit (0, 7, (int) getP (bandId (b, "Speed")));
        e.bandMix[b] = getP (bandId (b, "Mix")) / 100.0f;
        e.bandSmooth[b] = getP (bandId (b, "Smooth")) / 100.0f;
        e.bandLoop[b] = juce::jlimit (0, 7, (int) getP (bandId (b, "Loop")));
        e.bandSolo[b] = getP (bandId (b, "Solo")) > 0.5f;
    }
    // pad: MIDI tem prioridade, depois os botões
    int pad = midiPad;
    if (pad == 0)
    {
        const char* ids[4] = { "padRewind", "padStop", "padReverse", "padSpin" };
        for (int k = 0; k < 4 && pad == 0; ++k)
            if (getP (ids[k]) > 0.5f) pad = k + 1;
    }
    e.pad = pad;
    e.padDur = getP ("padDur") / 100.0f;
    e.padSync = getP ("padSync") > 0.5f;
    e.wow = getP ("wow") / 100.0f;
    e.sat = getP ("sat") / 100.0f;
    e.echoOn = getP ("echoOn") > 0.5f;
    e.echoDiv = juce::jlimit (0, 6, (int) getP ("echoDiv"));
    e.echoFb = getP ("echoFb") / 100.0f;
    e.echoSend = getP ("echoSend") / 100.0f;
    e.washout = getP ("washout") / 100.0f;
    e.echoPP = getP ("echoPP") > 0.5f;
    e.echoMte = getP ("echoMte") > 0.5f;
    e.eqBypass = getP ("eqByp") > 0.5f;
    e.eqOnTotal = getP ("eqPos") > 0.5f;
    e.eq.gainDb = getP ("eqGain");
    e.eq.hp = getP ("eqHp");
    e.eq.lp = getP ("eqLp");
    for (int k = 0; k < 4; ++k)
    {
        const juce::String s (k + 1);
        e.eq.f[k] = getP ("eqF" + s);
        e.eq.g[k] = getP ("eqG" + s);
        e.eq.q[k] = getP ("eqQ" + s);
    }
    e.bkCut = getP ("bkCut");
    e.bkStepped = getP ("bkStep") > 0.5f;
    e.bkLowPass = getP ("bkLow") > 0.5f;
    e.bk24 = getP ("bk24") > 0.5f;
    e.bkRes = getP ("bkRes") / 100.0f;
    e.bkDrive = getP ("bkDrive") / 100.0f;
    e.bkMix = getP ("bkMix") / 100.0f;
    e.bkBypass = getP ("bkByp") > 0.5f;
    e.dryWet = getP ("dryWet") / 100.0f;
    e.outDb = getP ("out");
    e.bypass = getP ("bypass") > 0.5f;
    return e;
}

void HalfDubProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels(), n = buffer.getNumSamples();
    if (numOut == 0 || n == 0) return;
    if (numIn == 1 && numOut > 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);   // mono -> estéreo
    for (int ch = std::max (numIn, numIn == 1 ? 2 : 0); ch < numOut; ++ch)
        buffer.clear (ch, 0, n);

    // MIDI: C1 D1 E1 F1 = pads, C2 = DROP (enquanto segura)
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (msg.isNoteOn())
        {
            for (int k = 0; k < 4; ++k) if (msg.getNoteNumber() == kPadNotes[k]) midiPad = k + 1;
            if (msg.getNoteNumber() == kDropNote) midiDrop.store (true);
        }
        else if (msg.isNoteOff())
        {
            for (int k = 0; k < 4; ++k) if (msg.getNoteNumber() == kPadNotes[k] && midiPad == k + 1) midiPad = 0;
            if (msg.getNoteNumber() == kDropNote) midiDrop.store (false);
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) { midiPad = 0; midiDrop.store (false); }
    }

    hd::HostClock clk;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) clk.bpm = *b;
            if (auto q = pos->getPpqPosition()) clk.ppq = *q;
            clk.playing = pos->getIsPlaying() && pos->getPpqPosition().hasValue();
        }

    engine.setParams (readParams());
    float* L = buffer.getWritePointer (0);
    float* R = numOut > 1 ? buffer.getWritePointer (1) : nullptr;
    if (R != nullptr)
        engine.process (L, R, n, clk);
    else
    {
        juce::HeapBlock<float> tmp ((size_t) n);
        std::copy (L, L + n, tmp.get());
        engine.process (L, tmp.get(), n, clk);
        for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + tmp[i]);
    }
}

void HalfDubProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HalfDubProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* HalfDubProcessor::createEditor() { return new HalfDubEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HalfDubProcessor(); }
