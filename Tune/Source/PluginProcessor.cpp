#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const float kDivBeats[] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
}

const juce::StringArray& MastorTuneProcessor::keyNames()
{
    static const juce::StringArray k { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return k;
}

const juce::StringArray& MastorTuneProcessor::scaleNames()
{
    static const juce::StringArray s = []
    {
        juce::StringArray a;
        const char* names[] = { "Maior", "Menor", u8"Dórico", u8"Mixolídio", u8"Frígio", u8"Lídio", u8"Lócrio",
                                "Harm. menor", "Penta maior", "Penta menor", "Blues", u8"Cromática" };
        for (auto* n : names) a.add (juce::String::fromUTF8 (n));
        return a;
    }();
    return s;
}

const juce::StringArray& MastorTuneProcessor::divisionNames()
{
    static const juce::StringArray names { "1/16", "1/8 T", "1/8", "1/8 D", "1/4", "1/4 D", "1/2" };
    return names;
}

MastorTuneProcessor::MastorTuneProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Riddim", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "MastorTune", createLayout())
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            params[rp->getParameterID().toStdString()] = apvts.getRawParameterValue (rp->getParameterID());

    engine.overrideAt = [this] (int64_t t) -> int
    {
        if (! blockPlaying) return -1;
        const double ppq = blockPpq + (double) (t - blockStart) / sr * blockBpm / 60.0;
        return notes.overrideAt (ppq);
    };
    engine.onHop = [this] (const mt::HopInfo& h)
    {
        sungMidi.store (h.midi);
        targetNote.store (h.target);
        if (! blockPlaying) return;
        HopFrame f;
        f.ppq = blockPpq + (double) (h.time - blockStart) / sr * blockBpm / 60.0;
        f.midi = h.midi; f.target = h.target; f.doubt = h.doubt;
        const auto scope = frameFifo.write (1);
        if (scope.blockSize1 > 0) frameBuf[(size_t) scope.startIndex1] = f;
        else if (scope.blockSize2 > 0) frameBuf[(size_t) scope.startIndex2] = f;
    };
    startTimerHz (20);
}

MastorTuneProcessor::~MastorTuneProcessor() { stopTimer(); }

juce::AudioProcessorValueTreeState::ParameterLayout MastorTuneProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    using Attr = AudioParameterFloatAttributes;
    auto pct = [] { return Attr().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; }); };
    auto db  = [] { return Attr().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; }); };
    auto hz  = [] { return Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; }); };
    auto ms  = [] { return Attr().withLabel ("ms").withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; }); };
    auto logRange = [] (float lo, float hi) { NormalisableRange<float> r (lo, hi); r.setSkewForCentre (std::sqrt (lo * hi)); return r; };

    // --- AFINAÇÃO
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "amount", 1 }, "Clean / Robo", NormalisableRange<float> (0.0f, 100.0f), 25.0f,
                 Attr().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + (v < 35 ? " (clean)" : v < 70 ? " (firme)" : " (hard-tune)"); })));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "key", 1 }, "Tom", keyNames(), 9));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "scale", 1 }, "Escala", scaleNames(), 1));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "autoKey", 1 }, "Tom automatico", true));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "listen", 1 }, "Escutar Riddim", true));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "voice", 1 }, "Voz", StringArray { "Grave", String::fromUTF8 (u8"Média"), "Aguda" }, 1));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0.0f, 100.0f), 100.0f, pct()));

    // --- EQ (gráfico com pontos)
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "eqOn", 1 }, "EQ On", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "eqHp", 1 }, "EQ HP", logRange (20.0f, 2000.0f), 20.0f,
                 Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v < 21.0f ? String ("OFF") : v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; })));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "eqLp", 1 }, "EQ LP", logRange (1000.0f, 20000.0f), 20000.0f,
                 Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v > 19500.0f ? String ("OFF") : String (v / 1000.0f, 1) + " kHz"; })));
    const float defF[3] = { 250.0f, 1000.0f, 4000.0f };
    for (int i = 0; i < 3; ++i)
    {
        const String n (i + 1);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "eq" + n + "f", 1 }, "EQ " + n + " Freq", logRange (40.0f, 16000.0f), defF[i], hz()));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "eq" + n + "g", 1 }, "EQ " + n + " Gain", NormalisableRange<float> (-12.0f, 12.0f), 0.0f, db()));
    }

    // --- SATURADOR
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "satOn", 1 }, "Saturador On", false));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "satMode", 1 }, "Saturador", StringArray { "Fita", String::fromUTF8 (u8"Válvula"), "Sound System" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "satDrive", 1 }, "Drive", NormalisableRange<float> (0.0f, 100.0f), 35.0f, pct()));

    // --- BIG KNOB (o filtro do Mastor Big Knob)
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "bkOn", 1 }, "Big Knob On", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "bkKnob", 1 }, "Big Knob", NormalisableRange<float> (0.0f, 1.0f), 0.3f,
                 Attr().withStringFromValueFunction ([] (float v, int) { const float f = dub::knobToFrequency (v, true); return f < 21.0f ? String ("OFF") : f >= 1000.0f ? String (f / 1000.0f, 1) + " kHz" : String (roundToInt (f)) + " Hz"; })));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "bkMode", 1 }, "Big Knob Mode", StringArray { "High Pass", "Low Pass" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "bkRes", 1 }, "Big Knob Resonance", NormalisableRange<float> (0.0f, 1.0f), 0.25f,
                 Attr().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; })));

    // --- TAPE ECHO (o mesmo do Mastor Tape Echo / Big Knob)
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "echoOn", 1 }, "Tape Echo On", false));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "echoSync", 1 }, "Echo Sync", true));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "echoDiv", 1 }, "Echo Time (Sync)", divisionNames(), 5));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoMs", 1 }, "Echo Time (ms)", logRange (40.0f, 1500.0f), 400.0f, ms()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoFb", 1 }, "Echo Feedback", NormalisableRange<float> (0.0f, 110.0f), 45.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoMix", 1 }, "Echo Mix", NormalisableRange<float> (0.0f, 100.0f), 30.0f, pct()));

    // --- MAXIMIZER
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "maxOn", 1 }, "Maximizer On", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "maxThresh", 1 }, "Maximizer Threshold", NormalisableRange<float> (-20.0f, 0.0f), -6.0f, db()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "maxCeil", 1 }, "Maximizer Ceiling", NormalisableRange<float> (-3.0f, 0.0f), -0.3f, db()));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "maxMode", 1 }, "Maximizer Mode", StringArray { "Transparente", "Punch", "Vintage" }, 0));

    // --- SAÍDA
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-24.0f, 12.0f), 0.0f, db()));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "bypass", 1 }, "Bypass", false));
    return { p.begin(), p.end() };
}

bool MastorTuneProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    const auto in = layouts.getMainInputChannelSet();
    if (! (in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo())))
        return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo())
            return false;
    }
    return true;
}

void MastorTuneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate > 0 ? sampleRate : 44100.0;
    engine.prepare (sr);
    ear.prepare (sr);
    eq.prepare (sr);
    sat.prepare (sr);
    bigKnob.prepare (sr, 2);
    echo.prepare (sr);
    maximizer.prepare (sr);
    loudness.prepare (sr);
    totalLatency = engine.latency() + maximizer.getLatency();
    setLatencySamples (totalLatency);
    for (auto& d : bypassDelay) d.assign ((size_t) juce::jmax (1, totalLatency), 0.0f);
    bypassIdx = 0;
    work.setSize (2, juce::jmax (1, samplesPerBlock));
    dryCopy.setSize (2, juce::jmax (1, samplesPerBlock));
    bkBuf.setSize (2, juce::jmax (1, samplesPerBlock));
    for (auto* s : { &eqMix, &satMix, &bkMix, &outGain })
        s->reset (sr, 0.02);
    eqMix.setCurrentAndTargetValue (getP ("eqOn") > 0.5f ? 1.0f : 0.0f);
    satMix.setCurrentAndTargetValue (getP ("satOn") > 0.5f ? 1.0f : 0.0f);
    bkMix.setCurrentAndTargetValue (getP ("bkOn") > 0.5f ? 1.0f : 0.0f);
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getP ("out")));
    bypassMix.reset (sr, 0.03);
    bypassMix.setCurrentAndTargetValue (getP ("bypass") > 0.5f ? 1.0f : 0.0f);
    samplesDone = 0;
    for (int c = 0; c < 2; ++c) punchFast[c] = punchSlow[c] = 0.0f;
}

void MastorTuneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    auto mainIn = getBusBuffer (buffer, true, 0);
    auto mainOut = getBusBuffer (buffer, false, 0);
    const int numIn = mainIn.getNumChannels(), numOut = mainOut.getNumChannels();
    if (numOut == 0 || n == 0) return;
    if (work.getNumSamples() < n) { work.setSize (2, n, false, false, true); dryCopy.setSize (2, n, false, false, true); bkBuf.setSize (2, n, false, false, true); }

    // --- MIDI de outra máquina de harmonia: notas seguradas = acorde
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (msg.isNoteOn()) heldNotes[(size_t) msg.getNoteNumber()] = true;
        else if (msg.isNoteOff()) heldNotes[(size_t) msg.getNoteNumber()] = false;
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) heldNotes.fill (false);
    }
    midi.clear();
    int midiMask = 0, held = 0;
    for (int i = 0; i < 128; ++i) if (heldNotes[(size_t) i]) { midiMask |= 1 << (i % 12); ++held; }
    midiHeld.store (held >= 2);

    // --- transporte
    double bpm = 120.0, ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = juce::jlimit (30.0, 300.0, *b);
            if (auto q = pos->getPpqPosition()) { ppq = *q; playing = pos->getIsPlaying(); }
        }
    hostBpm.store (bpm); hostPlaying.store (playing); playPpq.store (ppq);
    blockPpq = ppq; blockBpm = bpm; blockPlaying = playing; blockStart = samplesDone;

    // --- escuta do riddim (entrada lateral)
    const bool listen = getP ("listen") > 0.5f;
    bool scConnected = false;
    if (getBusCount (true) > 1)
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0 && getBus (true, 1)->isEnabled())
        {
            scConnected = true;
            if (listen)
                for (int i = 0; i < n; ++i)
                {
                    float x = sc.getSample (0, i);
                    if (sc.getNumChannels() > 1) x = 0.5f * (x + sc.getSample (1, i));
                    ear.push (x);
                }
        }
    }
    riddimConn.store (scConnected);
    const bool riddimOk = listen && scConnected && ear.hasSignal();
    riddimSig.store (riddimOk);

    // --- harmonia usada agora: MRM > MIDI > riddim > manual
    int source = SrcManual, chordMask = 0, chordRoot = -1; bool chordMinor = false;
    int keyRoot = juce::jlimit (0, 11, (int) getP ("key")), scale = juce::jlimit (0, 11, (int) getP ("scale"));
    const bool autoKey = getP ("autoKey") > 0.5f;
    if (mrmPlaying.load() && mrmChordMask.load() != 0)
    {
        source = SrcMrm; chordMask = mrmChordMask.load(); chordRoot = usedChordRoot.load(); chordMinor = usedChordMinor.load();
    }
    else if (held >= 2)
    {
        source = SrcMidi; chordMask = midiMask;
    }
    else if (riddimOk)
    {
        source = SrcRiddim; chordRoot = ear.getChordRoot(); chordMinor = ear.getChordMinor();
        chordMask = chordRoot >= 0 ? mt::triadMask (chordRoot, chordMinor) : 0;
    }
    if (autoKey && (scale == mt::Maior || scale == mt::Menor))
    {
        if (mrmOpen.load() && mrmKeyRoot.load() >= 0) { keyRoot = mrmKeyRoot.load(); scale = mrmKeyMinor.load() ? mt::Menor : mt::Maior; if (source == SrcManual) source = SrcMrm; }
        else if (riddimOk && ear.getKeyRoot() >= 0)    { keyRoot = ear.getKeyRoot(); scale = ear.getKeyMinor() ? mt::Menor : mt::Maior; }
    }
    usedSource.store (source); usedKey.store (keyRoot); usedScale.store (scale);
    if (source != SrcMrm) { usedChordRoot.store (chordRoot); usedChordMinor.store (chordMinor); }

    mt::TuneSettings ts;
    ts.amount  = getP ("amount") / 100.0f;
    ts.allowed = mt::scaleMask (keyRoot, scale);
    ts.chord   = (uint16_t) chordMask;
    ts.voice   = juce::jlimit (0, 2, (int) getP ("voice"));
    ts.mix     = getP ("mix") / 100.0f;
    engine.setSettings (ts);

    // --- módulos
    mt::EqSettings es;
    es.hp = getP ("eqHp"); es.lp = getP ("eqLp");
    for (int i = 0; i < 3; ++i) { const auto nm = std::to_string (i + 1); es.f[i] = getP (("eq" + nm + "f").c_str()); es.g[i] = getP (("eq" + nm + "g").c_str()); }
    eq.setSettings (es);
    sat.set ((int) getP ("satMode"), getP ("satDrive") / 100.0f);
    eqMix.setTargetValue (getP ("eqOn") > 0.5f ? 1.0f : 0.0f);
    satMix.setTargetValue (getP ("satOn") > 0.5f ? 1.0f : 0.0f);
    bkMix.setTargetValue (getP ("bkOn") > 0.5f ? 1.0f : 0.0f);
    outGain.setTargetValue (juce::Decibels::decibelsToGain (getP ("out")));
    bypassMix.setTargetValue (getP ("bypass") > 0.5f ? 1.0f : 0.0f);

    dub::Settings bk;
    bk.knob = getP ("bkKnob"); bk.stepped = true; bk.highPass = (int) getP ("bkMode") == 0;
    bk.steep = false; bk.resonance = getP ("bkRes"); bk.driveDb = 0.0f; bk.mix = 1.0f; bk.outputDb = 0.0f;
    bigKnob.setSettings (bk);

    const bool echoOn = getP ("echoOn") > 0.5f;
    dub::EchoSettings ec;
    const int div = juce::jlimit (0, 6, (int) getP ("echoDiv"));
    ec.delaySec = getP ("echoSync") > 0.5f ? kDivBeats[div] * 60.0f / (float) bpm : getP ("echoMs") / 1000.0f;
    ec.feedback = getP ("echoFb") / 100.0f;
    ec.toneHz = 2500.0f; ec.wow = 0.25f; ec.heads = 1; ec.pingPong = false;
    ec.send = echoOn ? 1.0f : 0.0f;
    echo.setSettings (ec);
    const float echoMix = getP ("echoMix") / 100.0f;

    const bool maxOn = getP ("maxOn") > 0.5f;
    const int maxMode = (int) getP ("maxMode");
    dub::MaximizerSettings mx;
    mx.on = maxOn; mx.pushDb = -getP ("maxThresh"); mx.ceilingDb = getP ("maxCeil");
    maximizer.setSettings (mx);

    // --- 1. afinação (voz mono) + entrada guardada para o bypass
    float inPk = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        float l = numIn > 0 ? mainIn.getSample (0, i) : 0.0f;
        float r = numIn > 1 ? mainIn.getSample (1, i) : l;
        dryCopy.setSample (0, i, l); dryCopy.setSample (1, i, r);
        const float x = 0.5f * (l + r);
        inPk = std::max (inPk, std::abs (x));
        const float y = engine.process (x);
        work.setSample (0, i, y); work.setSample (1, i, y);
    }
    samplesDone += n;
    inDb.store (juce::Decibels::gainToDecibels (inPk, -100.0f));

    // --- 2. EQ + saturador
    for (int i = 0; i < n; ++i)
    {
        const float ge = eqMix.getNextValue(), gs = satMix.getNextValue();
        for (int c = 0; c < 2; ++c)
        {
            float x = work.getSample (c, i);
            const float xe = eq.process (c, x);
            x += ge * (xe - x);
            const float xs = sat.process (c, x);
            x += gs * (xs - x);
            work.setSample (c, i, x);
        }
    }

    // --- 3. Big Knob (filtro King Tubby): processa uma cópia e mistura suave
    {
        if (bkMix.isSmoothing() || bkMix.getTargetValue() > 0.0f)
        {
            bkBuf.copyFrom (0, 0, work, 0, 0, n);
            bkBuf.copyFrom (1, 0, work, 1, 0, n);
            bigKnob.process (bkBuf.getArrayOfWritePointers(), 2, n);
            for (int i = 0; i < n; ++i)
            {
                const float g = bkMix.getNextValue();
                for (int c = 0; c < 2; ++c)
                    work.setSample (c, i, work.getSample (c, i) + g * (bkBuf.getSample (c, i) - work.getSample (c, i)));
            }
        }
        else bkMix.skip (n);
    }

    // --- 4. tape echo + maximizer + saída + bypass
    float pk[2] { 0, 0 };
    const float fastA = 1.0f - std::exp (-1.0f / (0.001f * (float) sr)), slowA = 1.0f - std::exp (-1.0f / (0.03f * (float) sr));
    for (int i = 0; i < n; ++i)
    {
        float l = work.getSample (0, i), r = work.getSample (1, i);
        float el = 0, er = 0;
        echo.processSample (l, r, el, er);
        l += echoMix * el; r += echoMix * er;

        if (maxOn && maxMode == 1)        // PUNCH: realça o ataque das sílabas antes do limiter
        {
            float* v[2] = { &l, &r };
            for (int c = 0; c < 2; ++c)
            {
                const float a = std::abs (*v[c]);
                punchFast[c] += fastA * (a - punchFast[c]);
                punchSlow[c] += slowA * (a - punchSlow[c]);
                const float boost = juce::jlimit (1.0f, 1.6f, 1.0f + 1.5f * (punchFast[c] - punchSlow[c]) / (punchSlow[c] + 1e-4f));
                *v[c] *= boost;
            }
        }
        else if (maxOn && maxMode == 2)   // VINTAGE: saturação macia de fita antes do limiter
        {
            l = std::tanh (l * 1.4f) / 1.4f * 1.25f;
            r = std::tanh (r * 1.4f) / 1.4f * 1.25f;
        }
        maximizer.processSample (l, r);
        const float og = outGain.getNextValue();
        l *= og; r *= og;
        loudness.processSample (l, r);

        // bypass: mistura suave com a entrada original atrasada na mesma latência
        const int N = (int) bypassDelay[0].size();
        const float dl = bypassDelay[0][(size_t) bypassIdx], dr = bypassDelay[1][(size_t) bypassIdx];
        bypassDelay[0][(size_t) bypassIdx] = dryCopy.getSample (0, i);
        bypassDelay[1][(size_t) bypassIdx] = dryCopy.getSample (1, i);
        if (++bypassIdx >= N) bypassIdx = 0;
        const float bm = bypassMix.getNextValue();
        l += bm * (dl - l); r += bm * (dr - r);

        mainOut.setSample (0, i, l);
        if (numOut > 1) mainOut.setSample (1, i, r);
        pk[0] = std::max (pk[0], std::abs (l)); pk[1] = std::max (pk[1], std::abs (r));
    }
    for (int c = 0; c < 2; ++c) outPk[c].store (std::max (pk[c], outPk[c].load() * 0.85f));
    grDb.store (maximizer.getLastGainReductionDb());
    lufsS.store (loudness.getShortTermLufs());
}

//==============================================================================
void MastorTuneProcessor::timerCallback()
{
    updateHarmony();

    // análises novas -> notas
    const int ready = frameFifo.getNumReady();
    if (ready > 0)
    {
        const auto scope = frameFifo.read (ready);
        bool ch = false;
        if (scope.blockSize1 > 0) ch |= notes.addFrames (frameBuf.data() + scope.startIndex1, scope.blockSize1);
        if (scope.blockSize2 > 0) ch |= notes.addFrames (frameBuf.data() + scope.startIndex2, scope.blockSize2);
        if (ch) notesChanged = true;
    }
    else if (! hostPlaying.load())
    {
        if (notes.closeCurrent()) notesChanged = true;
    }
}

void MastorTuneProcessor::updateHarmony()
{
    mrmlink::Data d;
    if (mrmReader.read (d))
    {
        mrmOpen.store (true);
        mrmPlaying.store (d.playing != 0 && d.curSlot >= 0);
        // tom a partir da harmonia inteira (cada acorde pesa pela duração)
        std::array<float, 12> c {};
        bool any = false;
        for (int i = 0; i < juce::jmin (4, (int) d.numSlots); ++i)
        {
            if (! d.on[i]) continue;
            any = true;
            const uint16_t m = mt::triadMask (d.root[i], d.minor[i] != 0);
            for (int k = 0; k < 12; ++k) if (m & (1u << k)) c[(size_t) k] += juce::jmax (0.5f, d.beats[i]) * (k == d.root[i] % 12 ? 1.4f : 1.0f);
        }
        if (any)
        {
            bool minor = false;
            const int root = mt::RiddimEar::keyFromChroma (c, minor);
            mrmKeyRoot.store (root); mrmKeyMinor.store (minor);
        }
        if (d.curSlot >= 0 && d.curSlot < 4)
        {
            mrmChordMask.store (mt::triadMask (d.root[d.curSlot], d.minor[d.curSlot] != 0));
            if (usedSource.load() == SrcMrm) { usedChordRoot.store (d.root[d.curSlot] % 12); usedChordMinor.store (d.minor[d.curSlot] != 0); }
        }
        else mrmChordMask.store (0);
    }
    else
    {
        mrmOpen.store (false); mrmPlaying.store (false); mrmChordMask.store (0); mrmKeyRoot.store (-1);
    }
}

MastorTuneProcessor::Status MastorTuneProcessor::getStatus() const
{
    Status s;
    s.source = usedSource.load();
    s.mrmOpen = mrmOpen.load(); s.mrmPlaying = mrmPlaying.load();
    s.riddimSignal = riddimSig.load(); s.riddimConnected = riddimConn.load(); s.midiChord = midiHeld.load();
    s.keyRoot = usedKey.load(); s.scale = usedScale.load(); s.keyAuto = getP ("autoKey") > 0.5f;
    s.chordRoot = usedChordRoot.load(); s.chordMinor = usedChordMinor.load();
    s.sungMidi = sungMidi.load(); s.target = targetNote.load();
    s.inDb = inDb.load(); s.outPeak[0] = outPk[0].load(); s.outPeak[1] = outPk[1].load();
    s.grDb = grDb.load(); s.lufs = lufsS.load();
    return s;
}

static void setParamValue (juce::AudioProcessorValueTreeState& st, const char* id, float v)
{
    if (auto* p = st.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

void MastorTuneProcessor::chooseKey (int root)
{
    // fixa também a escala que estava valendo (ex.: a que o riddim achou)
    setParamValue (apvts, "scale", (float) usedScale.load());
    setParamValue (apvts, "key", (float) juce::jlimit (0, 11, root));
    setParamValue (apvts, "autoKey", 0.0f);
}

void MastorTuneProcessor::chooseScale (int scale)
{
    setParamValue (apvts, "key", (float) usedKey.load());
    setParamValue (apvts, "scale", (float) juce::jlimit (0, 11, scale));
    if (scale != mt::Maior && scale != mt::Menor) setParamValue (apvts, "autoKey", 0.0f);
}

void MastorTuneProcessor::setAuto (bool on) { setParamValue (apvts, "autoKey", on ? 1.0f : 0.0f); }

//==============================================================================
void MastorTuneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.removeChild (st.getChildWithName ("Notes"), nullptr);
    st.appendChild (notes.toTree(), nullptr);
    if (auto xml = st.createXml())
        copyXmlToBinary (*xml, destData);
}

void MastorTuneProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto t = juce::ValueTree::fromXml (*xml);
            notes.fromTree (t.getChildWithName ("Notes"));
            t.removeChild (t.getChildWithName ("Notes"), nullptr);
            apvts.replaceState (t);
            notesChanged = true;
        }
}

juce::AudioProcessorEditor* MastorTuneProcessor::createEditor() { return new MastorTuneEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MastorTuneProcessor(); }
