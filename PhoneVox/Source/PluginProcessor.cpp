#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const float kDivBeats[] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
}

const juce::StringArray& RadioFoneProcessor::divisionNames()
{
    static const juce::StringArray names { "1/16", "1/8 T", "1/8", "1/8 D", "1/4", "1/4 D", "1/2" };
    return names;
}

const juce::StringArray& RadioFoneProcessor::modeNames()
{
    static const juce::StringArray names { juce::String::fromUTF8 ("Rádio AM"), "Telefone", "Megafone" };
    return names;
}

RadioFoneProcessor::RadioFoneProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "RadioFone", createLayout())
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            params[rp->getParameterID().toStdString()] = apvts.getRawParameterValue (rp->getParameterID());
    curveVoice.prepare (48000.0);
}

juce::AudioProcessorValueTreeState::ParameterLayout RadioFoneProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    using Attr = AudioParameterFloatAttributes;
    auto pct = [] { return Attr().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; }); };
    auto db  = [] { return Attr().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; }); };
    auto hz  = [] { return Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; }); };
    auto ms  = [] { return Attr().withLabel ("ms").withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; }); };
    auto logRange = [] (float lo, float hi) { NormalisableRange<float> r (lo, hi); r.setSkewForCentre (std::sqrt (lo * hi)); return r; };

    // --- APARELHO
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "mode", 1 }, "Aparelho", modeNames(), 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tune", 1 }, "Sintonia", NormalisableRange<float> (-100.0f, 100.0f), 0.0f,
                 Attr().withStringFromValueFunction ([] (float v, int) { return (v > 0.5f ? "+" : "") + String (roundToInt (v)); })));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "band", 1 }, "Banda", NormalisableRange<float> (0.0f, 100.0f), 50.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Drive", NormalisableRange<float> (0.0f, 100.0f), 35.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "noise", 1 }, "Chiado", NormalisableRange<float> (0.0f, 100.0f), 20.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "defect", 1 }, "Defeito", NormalisableRange<float> (0.0f, 100.0f), 15.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0.0f, 100.0f), 100.0f, pct()));

    // --- ECO DE FITA (o mesmo do Big Knob)
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "echoSync", 1 }, "Echo Sync", true));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "echoDiv", 1 }, "Echo Time (Sync)", divisionNames(), 5));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoMs", 1 }, "Repeat Rate (ms)", logRange (40.0f, 1500.0f), 400.0f, ms()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoFb", 1 }, "Echo Feedback", NormalisableRange<float> (0.0f, 120.0f), 55.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoTone", 1 }, "Echo Tone", logRange (500.0f, 8000.0f), 2500.0f, hz()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoWow", 1 }, "Echo Wow", NormalisableRange<float> (0.0f, 100.0f), 25.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoSend", 1 }, "Echo Send", NormalisableRange<float> (0.0f, 100.0f), 0.0f, pct()));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "echoHeads", 1 }, "Echo Heads",
                                                        StringArray { "1", "2", "3", "1+2", "2+3", "1+3", "1+2+3" }, 0));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "echoPP", 1 }, "Ping Pong", false));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "throw", 1 }, "Throw", false));

    // --- MAXIMIZER (o mesmo do Big Knob)
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "maxOn", 1 }, "Maximizer On", true));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "maxThresh", 1 }, "Maximizer Threshold",
                                                        NormalisableRange<float> (-20.0f, 0.0f), 0.0f, db()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "maxCeil", 1 }, "Maximizer Ceiling",
                                                        NormalisableRange<float> (-3.0f, 0.0f), -0.3f, db()));

    // --- MASTER
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "dry", 1 }, "Dry", NormalisableRange<float> (0.0f, 100.0f), 100.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-24.0f, 12.0f), 0.0f, db()));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "bypass", 1 }, "Bypass", false));
    return { p.begin(), p.end() };
}

bool RadioFoneProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    const auto in = layouts.getMainInputChannelSet();
    // voz mono entrando numa pista estéreo também vale (mono -> estéreo)
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void RadioFoneProcessor::prepareToPlay (double sampleRate, int)
{
    voice.prepare (sampleRate);
    echo.prepare (sampleRate);
    maximizer.prepare (sampleRate);
    setLatencySamples (maximizer.getLatency());
    for (auto& d : bypassDelay) d.assign ((size_t) juce::jmax (1, maximizer.getLatency()), 0.0f);
    bypassIdx = 0;
    {
        const juce::SpinLock::ScopedLockType sl (curveLock);
        curveVoice.prepare (48000.0);
    }
    for (auto* s : { &dryGain, &outGain })
        s->reset (sampleRate, 0.02);
    bypassMix.reset (sampleRate, 0.03);
    bypassMix.setCurrentAndTargetValue (getP ("bypass") > 0.5f ? 1.0f : 0.0f);
}

void RadioFoneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels(), n = buffer.getNumSamples();
    if (numOut == 0 || n == 0) return;
    if (numIn == 1 && numOut > 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);   // mono -> estéreo
    for (int ch = std::max (numIn, numIn == 1 ? 2 : 0); ch < numOut; ++ch)
        buffer.clear (ch, 0, n);

    // andamento do Cubase
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = juce::jlimit (30.0, 300.0, *b);
    displayBpm.store ((float) bpm);

    rf::VoiceSettings vs;
    vs.mode   = juce::jlimit (0, 2, (int) getP ("mode"));
    vs.tune   = getP ("tune") / 100.0f;
    vs.band   = getP ("band") / 100.0f;
    vs.drive  = getP ("drive") / 100.0f;
    vs.noise  = getP ("noise") / 100.0f;
    vs.defect = getP ("defect") / 100.0f;
    vs.mix    = getP ("mix") / 100.0f;
    voice.setSettings (vs);

    dub::EchoSettings es;
    const int div = juce::jlimit (0, 6, (int) getP ("echoDiv"));
    es.delaySec = getP ("echoSync") > 0.5f ? kDivBeats[div] * 60.0f / (float) bpm : getP ("echoMs") / 1000.0f;
    es.feedback = getP ("echoFb") / 100.0f;
    es.toneHz   = getP ("echoTone");
    es.wow      = getP ("echoWow") / 100.0f;
    es.send     = getP ("throw") > 0.5f ? 1.0f : getP ("echoSend") / 100.0f;
    es.pingPong = getP ("echoPP") > 0.5f;
    static const int headMasks[] = { 1, 2, 4, 3, 6, 5, 7 };
    es.heads    = headMasks[juce::jlimit (0, 6, (int) getP ("echoHeads"))];
    echo.setSettings (es);
    echoDelay.store (es.delaySec);

    dub::MaximizerSettings ms;
    ms.on = getP ("maxOn") > 0.5f;
    ms.pushDb = -getP ("maxThresh");
    ms.ceilingDb = getP ("maxCeil");
    maximizer.setSettings (ms);
    float pk[2] = { 0, 0 }, grMin = 0.0f;

    dryGain.setTargetValue (getP ("dry") / 100.0f);
    outGain.setTargetValue (juce::Decibels::decibelsToGain (getP ("out")));
    bypassMix.setTargetValue (getP ("bypass") > 0.5f ? 1.0f : 0.0f);

    float* L = buffer.getWritePointer (0);
    float* R = numOut > 1 ? buffer.getWritePointer (1) : nullptr;
    float sumSq = 0.0f, echoSq = 0.0f;
    int sw = scopeWrite.load (std::memory_order_relaxed);

    // cadeia: entrada (mono) -> aparelho -> seco + eco de fita -> saída
    for (int i = 0; i < n; ++i)
    {
        const float xl = L[i], xr = R ? R[i] : xl;
        const float v = voice.processSample (0.5f * (xl + xr));
        float el, er;
        echo.processSample (v, v, el, er);
        const float dg = dryGain.getNextValue(), og = outGain.getNextValue(), m = bypassMix.getNextValue();
        float yl = (v * dg + el) * og, yr = (v * dg + er) * og;
        maximizer.processSample (yl, yr);
        grMin = std::min (grMin, maximizer.getLastGainReductionDb());
        // bypass: mistura suave com a entrada atrasada na mesma latência do maximizer
        {
            const int N = (int) bypassDelay[0].size();
            const float dl = bypassDelay[0][(size_t) bypassIdx], dr = bypassDelay[1][(size_t) bypassIdx];
            bypassDelay[0][(size_t) bypassIdx] = xl;
            bypassDelay[1][(size_t) bypassIdx] = xr;
            if (++bypassIdx >= N) bypassIdx = 0;
            yl = yl * (1.0f - m) + dl * m;
            yr = yr * (1.0f - m) + dr * m;
        }
        pk[0] = std::max (pk[0], std::fabs (yl)); pk[1] = std::max (pk[1], std::fabs (yr));
        sumSq += 0.5f * (yl * yl + yr * yr);
        scopeBuf[(size_t) sw] = 0.5f * (yl + yr);
        sw = (sw + 1) & (kScopeSize - 1);
        echoSq += 0.5f * (el * el + er * er);
        if (R) { L[i] = yl; R[i] = yr; }
        else     L[i] = 0.5f * (yl + yr);
    }
    scopeWrite.store (sw, std::memory_order_release);
    outRms.store (std::sqrt (sumSq / (float) n));
    outPeak[0].store (pk[0]); outPeak[1].store (pk[1]);
    grDb.store (grMin);
    echoRms.store (std::sqrt (echoSq / (float) n));
}

void RadioFoneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void RadioFoneProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

void RadioFoneProcessor::getVoiceCurve (const float* hz, float* db, int n)
{
    rf::VoiceSettings vs;
    vs.mode = juce::jlimit (0, 2, (int) getP ("mode"));
    vs.tune = getP ("tune") / 100.0f;
    vs.band = getP ("band") / 100.0f;
    vs.drive = getP ("drive") / 100.0f;
    vs.defect = getP ("defect") / 100.0f;
    vs.mix = getP ("mix") / 100.0f;
    const juce::SpinLock::ScopedLockType sl (curveLock);
    curveVoice.setSettings (vs);
    curveVoice.curveDb (hz, db, n);
}

float RadioFoneProcessor::getVoicePeakHz()
{
    rf::VoiceSettings vs;
    vs.mode = juce::jlimit (0, 2, (int) getP ("mode"));
    vs.tune = getP ("tune") / 100.0f;
    vs.band = getP ("band") / 100.0f;
    vs.defect = getP ("defect") / 100.0f;
    const juce::SpinLock::ScopedLockType sl (curveLock);
    curveVoice.setSettings (vs);
    return curveVoice.peakHz();
}

juce::AudioProcessorEditor* RadioFoneProcessor::createEditor() { return new RadioFoneEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RadioFoneProcessor(); }
