#include "BatteryProcessor.h"
#include "BatteryEditor.h"
#include "BatterySamples.h"

using namespace dbr;

namespace
{
    constexpr float kMinDb = -36.0f;

    void setParamPlain (juce::RangedAudioParameter& p, float plain)
    {
        const float v = p.convertTo0to1 (plain);
        if (std::abs (p.getValue() - v) > 1.0e-6f)
        {
            p.beginChangeGesture();
            p.setValueNotifyingHost (v);
            p.endChangeGesture();
        }
    }

    // satura só o que passa de 0,85: a soma dos pads nunca estoura
    inline float softClip (float x)
    {
        const float a = std::abs (x), knee = 0.85f;
        if (a <= knee) return x;
        return std::copysign (knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee)), x);
    }

    // interpolação cúbica (Hermite) entre as amostras
    inline float hermite (const float* d, int n, double pos)
    {
        const int i = (int) pos;
        const float f = (float) (pos - i);
        auto at = [&] (int k) { return d[juce::jlimit (0, n - 1, k)]; };
        const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout BatteryProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto ms = [] (float lo, float hi) { NormalisableRange<float> r (lo, hi, 0.1f); r.setSkewForCentre (lo + (hi - lo) * 0.08f); return r; };

    for (int i = 0; i < numPads; ++i)
    {
        const String pre = "Pad " + String (i + 1).paddedLeft ('0', 2) + " ";
        auto id = [i] (P p) { return ParameterID { pid (i, p), 1 }; };
        auto group = std::make_unique<AudioProcessorParameterGroup> ("pad" + String (i + 1), pre + pads[i].name, "|");
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::level), pre + "Volume", NormalisableRange<float> (kMinDb, 12.0f, 0.1f), 0.0f,
                                                               AudioParameterFloatAttributes().withLabel ("dB")));
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::pan), pre + "Pan", NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::tune), pre + u8"Afinação", NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f,
                                                               AudioParameterFloatAttributes().withLabel ("st")));
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::start), pre + u8"Início", NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.0f));
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::end), pre + "Fim", NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 1.0f));
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::attack), pre + "Attack", ms (0.0f, 2000.0f), 0.0f,
                                                               AudioParameterFloatAttributes().withLabel ("ms")));
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::hold), pre + "Hold", ms (0.0f, 2000.0f), 0.0f,
                                                               AudioParameterFloatAttributes().withLabel ("ms")));
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::decay), pre + "Decay", ms (5.0f, 5000.0f), 400.0f,
                                                               AudioParameterFloatAttributes().withLabel ("ms")));
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::sustain), pre + "Sustain", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::release), pre + "Release", ms (5.0f, 5000.0f), 120.0f,
                                                               AudioParameterFloatAttributes().withLabel ("ms")));
        NormalisableRange<float> cut (60.0f, 20000.0f, 1.0f);
        cut.setSkewForCentre (1500.0f);
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::cutoff), pre + "Filtro", cut, 20000.0f,
                                                               AudioParameterFloatAttributes().withLabel ("Hz")));
        group->addChild (std::make_unique<AudioParameterFloat> (id (P::velocity), pre + "Velocity", NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.75f));
        group->addChild (std::make_unique<AudioParameterBool>  (id (P::reverse), pre + "Reverse", false));
        group->addChild (std::make_unique<AudioParameterChoice> (id (P::mode), pre + "Modo", modeNames, 0));
        group->addChild (std::make_unique<AudioParameterChoice> (id (P::choke), pre + "Choke", chokeNames, pads[i].chokeGroup));
        layout.add (std::move (group));
    }
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "masterLevel", 1 }, "Volume geral",
                                                       NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tape", 1 }, "Fita (saturação)",
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.15f));
    return layout;
}

//==============================================================================
BatteryProcessor::BatteryProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DubBatteryReggae", createLayout())
{
    for (int i = 0; i < numPads; ++i)
        for (int p = 0; p < (int) P::count; ++p)
            raw[(size_t) i][(size_t) p] = apvts.getRawParameterValue (pid (i, (P) p));
    for (auto& v : playPos) v.store (-1.0f);

    juce::WavAudioFormat wav;
    for (int i = 0; i < factory::numSamples; ++i)
    {
        const auto& e = factory::samples[i];
        std::unique_ptr<juce::AudioFormatReader> r (wav.createReaderFor (new juce::MemoryInputStream (e.data, e.size, false), true));
        factory.push_back (r != nullptr ? makeSample (*r, e.name, "f:" + juce::String (i)) : nullptr);
    }
    for (int i = 0; i < numPads; ++i) setFactorySample (i, i);

    drums.setPlayHead (&drumsHead);
    setParamPlain (*drums.apvts.getParameter ("sound"), 0.0f);    // o som sai pelos pads do Battery
    startTimerHz (10);
}

BatteryProcessor::~BatteryProcessor()
{
    stopTimer();
}

bool BatteryProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void BatteryProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    const int bs = juce::jmax (512, samplesPerBlock);
    drums.setRateAndBufferSizeDetails (sr, bs);
    drums.prepareToPlay (sr, bs);
    drumsBuf.setSize (2, bs);
    drumsMidi.ensureSize (4096);
    events.ensureSize (8192);
    uiNotes.reset (sr);
    for (auto& v : voices) { v.active = false; v.sample.reset(); }
    wasGroove = false;
}

void BatteryProcessor::releaseResources()
{
    drums.releaseResources();
}

//==============================================================================
// Samples
dbr::SamplePtr BatteryProcessor::makeSample (juce::AudioFormatReader& r, juce::String name, juce::String ref)
{
    const int n = (int) juce::jmin<juce::int64> (r.lengthInSamples, (juce::int64) (r.sampleRate * 60.0));   // até 60 s
    if (n <= 0) return nullptr;
    auto s = std::make_shared<Sample>();
    const int chs = juce::jlimit (1, 2, (int) r.numChannels);
    s->audio.setSize (chs, n);
    r.read (&s->audio, 0, n, 0, true, chs > 1);
    s->rate = r.sampleRate > 0 ? r.sampleRate : 44100.0;
    s->name = name.toUpperCase();
    s->ref  = ref;

    constexpr int kPeaks = 1024;
    s->peaks.assign (kPeaks, 0.0f);
    float top = 0.0f;
    for (int k = 0; k < kPeaks; ++k)
    {
        const int a = (int) ((juce::int64) k * n / kPeaks), b = juce::jmax (a + 1, (int) ((juce::int64) (k + 1) * n / kPeaks));
        float m = 0.0f;
        for (int c = 0; c < chs; ++c)
            m = juce::jmax (m, s->audio.getMagnitude (c, a, juce::jmin (b, n) - a));
        s->peaks[(size_t) k] = m;
        top = juce::jmax (top, m);
    }
    if (top > 0.0f) for (auto& p : s->peaks) p /= top;
    return s;
}

void BatteryProcessor::setPadSample (int pad, dbr::SamplePtr s)
{
    if (s == nullptr) return;
    auto old = std::atomic_exchange (&padSamples[(size_t) pad], s);
    if (old != nullptr) graveyard.push_back (std::move (old));   // pode estar tocando: libera depois
}

void BatteryProcessor::setFactorySample (int pad, int index)
{
    if (juce::isPositiveAndBelow (index, (int) factory.size()))
        setPadSample (pad, factory[(size_t) index]);
}

bool BatteryProcessor::loadSampleFile (int pad, const juce::File& f)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr) return false;
    auto s = makeSample (*r, f.getFileNameWithoutExtension(), f.getFullPathName());
    if (s == nullptr) return false;
    setPadSample (pad, s);
    return true;
}

void BatteryProcessor::loadSampleRef (int pad, const juce::String& ref)
{
    if (ref.startsWith ("f:"))
        setFactorySample (pad, ref.substring (2).getIntValue());
    else if (! ref.isEmpty() && ! loadSampleFile (pad, juce::File (ref)))
        setFactorySample (pad, pad);                 // arquivo sumiu: volta o timbre de fábrica do pad
}

int BatteryProcessor::factoryIndexOf (int pad) const
{
    auto s = getSample (pad);
    return s != nullptr && s->ref.startsWith ("f:") ? s->ref.substring (2).getIntValue() : -1;
}

int BatteryProcessor::numFactorySamples()                  { return factory::numSamples; }
juce::String BatteryProcessor::factorySampleName (int i)   { return factory::samples[juce::jlimit (0, factory::numSamples - 1, i)].name; }

//==============================================================================
// Pads pela interface
void BatteryProcessor::triggerPad (int pad, int velocity)
{
    auto m = juce::MidiMessage::noteOn (1, firstNote + pad, (juce::uint8) juce::jlimit (1, 127, velocity));
    m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    uiNotes.addMessageToQueue (m);
}

void BatteryProcessor::releasePad (int pad)
{
    auto m = juce::MidiMessage::noteOff (1, firstNote + pad);
    m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    uiNotes.addMessageToQueue (m);
}

bool BatteryProcessor::isGroovePlaying() const
{
    return drums.apvts.getRawParameterValue ("preview")->load() > 0.5f;
}

void BatteryProcessor::setGroovePlaying (bool on)
{
    setParamPlain (*drums.apvts.getParameter ("preview"), on ? 1.0f : 0.0f);
}

//==============================================================================
// Motor do sampler
void BatteryProcessor::startVoice (int pad, int velocity)
{
    auto s = std::atomic_load (&padSamples[(size_t) pad]);
    if (s == nullptr || s->audio.getNumSamples() < 4) return;

    // choke: o pad novo corta os outros do mesmo grupo (hi-hat fechado corta o aberto)
    const int group = juce::roundToInt (padValue (pad, P::choke));
    if (group > 0)
        for (auto& v : voices)
            if (v.active && juce::roundToInt (padValue (v.pad, P::choke)) == group && v.stage != Voice::Choke)
            {
                v.stage = Voice::Choke;
                v.stageLeft = (float) (sr * 0.004);
            }

    Voice* slot = nullptr;
    for (auto& v : voices) if (! v.active) { slot = &v; break; }
    if (slot == nullptr)                                 // sem voz livre: rouba a mais antiga
    {
        slot = &voices[0];
        for (auto& v : voices) if (v.age < slot->age) slot = &v;
    }
    auto& v = *slot;
    const int n = s->audio.getNumSamples();
    float st = padValue (pad, P::start), en = padValue (pad, P::end);
    if (en < st) std::swap (st, en);
    v.first = juce::jlimit (0, n - 2, (int) (st * (float) n));
    v.last  = juce::jlimit (v.first + 2, n, (int) (en * (float) n));
    v.reverse = padValue (pad, P::reverse) > 0.5f;
    v.gate = juce::roundToInt (padValue (pad, P::mode)) == 1;
    v.pos = v.reverse ? (double) (v.last - 1) : (double) v.first;
    v.step = s->rate / sr * std::pow (2.0, (double) padValue (pad, P::tune) / 12.0);
    v.sample = std::move (s);
    v.pad = pad;
    v.note = firstNote + pad;
    v.released = false;
    const float sens = padValue (pad, P::velocity), vel = (float) velocity / 127.0f;
    v.gain = (1.0f - sens) + sens * vel * vel;
    v.stage = Voice::Attack;
    v.stageLeft = (float) (padValue (pad, P::attack) * 0.001 * sr);
    v.env = v.stageLeft < 1.0f ? 1.0f : 0.0f;
    for (int c = 0; c < 2; ++c) v.ic1[c] = v.ic2[c] = 0.0f;
    v.age = ++voiceCounter;
    v.active = true;

    hitVel[(size_t) pad].store (vel);
    hitCount[(size_t) pad].fetch_add (1);
}

void BatteryProcessor::noteOff (int pad)
{
    for (auto& v : voices)
        if (v.active && v.pad == pad && v.gate && ! v.released && v.stage != Voice::Choke)
        {
            v.released = true;
            v.stage = Voice::Release;
        }
}

void BatteryProcessor::renderVoices (float* l, float* r, int start, int num)
{
    if (num <= 0) return;
    for (auto& v : voices)
    {
        if (! v.active) continue;
        const int pad = v.pad;
        const float levelDb = padValue (pad, P::level);
        const float g = levelDb <= kMinDb + 0.05f ? 0.0f : juce::Decibels::decibelsToGain (levelDb) * v.gain;
        const float pan = padValue (pad, P::pan);
        const float gl = g * std::cos ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * juce::MathConstants<float>::sqrt2;
        const float gr = g * std::sin ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * juce::MathConstants<float>::sqrt2;

        const float att = (float) (padValue (pad, P::attack)  * 0.001 * sr);
        const float hld = (float) (padValue (pad, P::hold)    * 0.001 * sr);
        const float dec = (float) (padValue (pad, P::decay)   * 0.001 * sr);
        const float sus = padValue (pad, P::sustain);
        const float rel = (float) (padValue (pad, P::release) * 0.001 * sr);
        const float decCoef = std::exp (-5.0f / juce::jmax (1.0f, dec));      // chega no sustain em ~decay
        const float relCoef = std::exp (-6.9f / juce::jmax (1.0f, rel));      // -60 dB em ~release

        const float cutoff = padValue (pad, P::cutoff);
        const bool filt = cutoff < 19500.0f;
        const float fg = std::tan (juce::MathConstants<float>::pi * juce::jmin (cutoff, (float) (sr * 0.45)) / (float) sr);
        const float fk = 1.0f / 0.75f;
        const float a1 = 1.0f / (1.0f + fg * (fg + fk)), a2 = fg * a1, a3 = fg * a2;

        const auto& buf = v.sample->audio;
        const int n = buf.getNumSamples();
        const float* d0 = buf.getReadPointer (0);
        const float* d1 = buf.getReadPointer (juce::jmin (1, buf.getNumChannels() - 1));
        const bool stereo = buf.getNumChannels() > 1;
        const float declick = (float) (sr * 0.0015);

        for (int i = start; i < start + num; ++i)
        {
            // envelope AHDSR
            switch (v.stage)
            {
                case Voice::Attack:
                    if (v.stageLeft >= 1.0f) { v.env = juce::jmin (1.0f, v.env + (1.0f - v.env) / v.stageLeft); v.stageLeft -= 1.0f; }
                    else { v.env = 1.0f; v.stage = Voice::Hold; v.stageLeft = hld; }
                    break;
                case Voice::Hold:
                    if ((v.stageLeft -= 1.0f) <= 0.0f) { v.stage = Voice::Decay; v.stageLeft = dec; }
                    break;
                case Voice::Decay:
                    v.env = sus + (v.env - sus) * decCoef;
                    if ((v.stageLeft -= 1.0f) <= 0.0f) v.stage = Voice::Sustain;
                    break;
                case Voice::Sustain:
                    v.env = sus + (v.env - sus) * decCoef;
                    break;
                case Voice::Release:
                    v.env *= relCoef;
                    break;
                case Voice::Choke:
                    v.env -= v.env / juce::jmax (1.0f, v.stageLeft);
                    v.stageLeft -= 1.0f;
                    break;
            }
            if (v.env < 1.0e-4f && (v.stage == Voice::Release || v.stage == Voice::Choke
                                    || (v.stage == Voice::Sustain && sus < 1.0e-3f)))
            { v.active = false; break; }

            // fim da região: termina com um fade curto (sem clique)
            const double left = v.reverse ? v.pos - (double) v.first : (double) v.last - 1.0 - v.pos;
            if (left <= 0.0) { v.active = false; break; }
            float edge = juce::jmin (1.0f, (float) (left / v.step) / declick);
            const double done = v.reverse ? (double) (v.last - 1) - v.pos : v.pos - (double) v.first;
            if (v.first > 0 || v.reverse) edge = juce::jmin (edge, (float) (done / v.step + 1.0) / declick);

            float sl = hermite (d0, n, v.pos), sr2 = stereo ? hermite (d1, n, v.pos) : sl;
            if (filt)
            {
                float* in[2] = { &sl, &sr2 };
                for (int c = 0; c < 2; ++c)
                {
                    const float v3 = *in[c] - v.ic2[c];
                    const float v1 = a1 * v.ic1[c] + a2 * v3;
                    const float v2 = v.ic2[c] + a2 * v.ic1[c] + a3 * v3;
                    v.ic1[c] = 2.0f * v1 - v.ic1[c];
                    v.ic2[c] = 2.0f * v2 - v.ic2[c];
                    *in[c] = v2;
                }
            }
            const float e = v.env * edge;
            l[i] += sl * e * gl;
            if (r != nullptr) r[i] += sr2 * e * gr;
            v.pos += v.reverse ? -v.step : v.step;
        }
        if (! v.active) v.sample.reset();
    }
}

//==============================================================================
void BatteryProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) return;
    if (drumsBuf.getNumSamples() < n) drumsBuf.setSize (2, n, false, false, true);

    // --- relógio: DAW tocando, ou PLAY GROOVE (preview do Dub Drums) com a DAW parada
    juce::Optional<juce::AudioPlayHead::PositionInfo> hostPos;
    if (auto* ph = getPlayHead()) hostPos = ph->getPosition();
    const bool dawPlaying = hostPos.hasValue() && hostPos->getIsPlaying() && hostPos->getPpqPosition().hasValue();
    const bool groove = isGroovePlaying() && ! dawPlaying;
    if (groove && ! wasGroove) groovePpq = 0.0;
    wasGroove = groove;

    juce::AudioPlayHead::PositionInfo pos;
    if (groove)
    {
        pos.setIsPlaying (true);
        pos.setBpm ((double) drums.apvts.getRawParameterValue ("bpm")->load());
        pos.setPpqPosition (groovePpq);
        pos.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
    }
    else if (hostPos.hasValue())
        pos = *hostPos;
    drumsHead.info = pos;

    juce::AudioBuffer<float> drumsView (drumsBuf.getArrayOfWritePointers(), 2, 0, n);
    drumsMidi.clear();
    drums.processBlock (drumsView, drumsMidi);       // o som interno dele fica desligado: só o MIDI interessa
    if (groove)
    {
        const double bpm = pos.getBpm().hasValue() ? *pos.getBpm() : 75.0;
        groovePpq += (double) n / sr * bpm / 60.0;
    }

    // --- eventos que tocam os pads: MIDI da DAW + groove do Dub Drums + cliques na interface
    events.clear();
    events.addEvents (midi, 0, n, 0);
    events.addEvents (drumsMidi, 0, n, 0);
    {
        juce::MidiBuffer ui;
        uiNotes.removeNextBlockOfMessages (ui, n);
        events.addEvents (ui, 0, n, 0);
    }

    auto* l = buffer.getWritePointer (0);
    auto* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    int at = 0;
    for (const auto meta : events)
    {
        const auto m = meta.getMessage();
        const int pad = m.getNoteNumber() - firstNote;
        if (! (m.isNoteOnOrOff() && juce::isPositiveAndBelow (pad, numPads))) continue;
        const int t = juce::jlimit (0, n - 1, meta.samplePosition);
        if (t > at) { renderVoices (l, r, at, t - at); at = t; }
        if (m.isNoteOn()) startVoice (pad, m.getVelocity());
        else              noteOff (pad);
    }
    renderVoices (l, r, at, n - at);

    // posição tocando no sample de cada pad (para o visor)
    std::array<float, numPads> newest; newest.fill (-1.0f);
    std::array<juce::uint64, numPads> ages {};
    for (auto& v : voices)
        if (v.active && v.age > ages[(size_t) v.pad] && v.sample != nullptr)
        {
            ages[(size_t) v.pad] = v.age;
            newest[(size_t) v.pad] = (float) (v.pos / (double) v.sample->audio.getNumSamples());
        }
    for (int i = 0; i < numPads; ++i) playPos[(size_t) i].store (newest[(size_t) i]);

    // --- MASTER: fita (saturação suave), volume e teto
    const float tape = apvts.getRawParameterValue ("tape")->load();
    const float master = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("masterLevel")->load());
    const float drive = 1.0f + tape * 4.0f, norm = 1.0f / std::tanh (drive);
    float peak = 0.0f;
    for (int c = 0; c < buffer.getNumChannels(); ++c)
    {
        auto* d = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            float x = d[i];
            if (tape > 0.001f) x = (1.0f - tape) * x + tape * std::tanh (x * drive) * norm * 0.8f;
            x = softClip (x * master);
            d[i] = x;
            peak = juce::jmax (peak, std::abs (x));
        }
    }
    outLevel.store (juce::jmax (peak, outLevel.load() * 0.92f));

    // --- MIDI que sai: o groove do Dub Drums com a DAW tocando (como no MRDM-12)
    midi.clear();
    if (dawPlaying) midi.addEvents (drumsMidi, 0, n, 0);
}

//==============================================================================
void BatteryProcessor::timerCallback()
{
    // samples trocados: libera quando nenhuma voz usa mais
    graveyard.erase (std::remove_if (graveyard.begin(), graveyard.end(),
                                     [] (const SamplePtr& s) { return s.use_count() <= 1; }),
                     graveyard.end());
    // o som interno do Dub Drums fica sempre desligado: a bateria toca nos pads
    if (drums.apvts.getRawParameterValue ("sound")->load() > 0.5f)
        setParamPlain (*drums.apvts.getParameter ("sound"), 0.0f);
}

//==============================================================================
// Presets
juce::File BatteryProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Dub Battery Reggae").getChildFile ("Presets");
}

int BatteryProcessor::getNumFactoryPresets() const { return (int) factoryPresets().size(); }

juce::StringArray BatteryProcessor::getPresetNames() const
{
    juce::StringArray names;
    for (auto& f : factoryPresets()) names.add (f.name);
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.dbrpreset");
    files.sort();
    for (auto& f : files) names.add (f.getFileNameWithoutExtension().toUpperCase());
    return names;
}

void BatteryProcessor::resetPadsToDefault()
{
    for (int i = 0; i < numPads; ++i)
    {
        for (int p = 0; p < (int) P::count; ++p)
        {
            auto& prm = padParam (i, (P) p);
            setParamPlain (prm, prm.convertFrom0to1 (prm.getDefaultValue()));
        }
        setFactorySample (i, i);
    }
}

juce::ValueTree BatteryProcessor::presetTree() const
{
    juce::ValueTree t ("DBRPreset");
    t.setProperty ("name", presetName, nullptr);
    for (int i = 0; i < numPads; ++i)
    {
        juce::ValueTree pad ("Pad");
        pad.setProperty ("index", i, nullptr);
        if (auto s = getSample (i)) pad.setProperty ("sample", s->ref, nullptr);
        for (int p = 0; p < (int) P::count; ++p)
            pad.setProperty (paramSuffix ((P) p), padValue (i, (P) p), nullptr);
        t.appendChild (pad, nullptr);
    }
    return t;
}

void BatteryProcessor::applyPresetTree (const juce::ValueTree& t)
{
    for (auto pad : t)
    {
        if (! pad.hasType ("Pad")) continue;
        const int i = pad.getProperty ("index", -1);
        if (! juce::isPositiveAndBelow (i, numPads)) continue;
        for (int p = 0; p < (int) P::count; ++p)
            if (pad.hasProperty (paramSuffix ((P) p)))
                setParamPlain (padParam (i, (P) p), (float) pad.getProperty (paramSuffix ((P) p)));
        loadSampleRef (i, pad.getProperty ("sample").toString());
    }
}

void BatteryProcessor::loadPreset (int index)
{
    const auto names = getPresetNames();
    if (! juce::isPositiveAndBelow (index, names.size())) return;
    const int nf = getNumFactoryPresets();
    resetPadsToDefault();
    if (index < nf)
    {
        factoryPresets()[(size_t) index].apply ([this] (int pad, P p, float v) { setParamPlain (padParam (pad, p), v); });
    }
    else
    {
        auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.dbrpreset");
        files.sort();
        if (auto xml = juce::parseXML (files[index - nf]))
            applyPresetTree (juce::ValueTree::fromXml (*xml));
    }
    presetIndex = index;
    presetName = names[index];
}

bool BatteryProcessor::savePreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim().toUpperCase());
    if (clean.isEmpty()) return false;
    auto dir = userPresetFolder();
    dir.createDirectory();
    presetName = clean;
    auto xml = presetTree().createXml();
    if (xml == nullptr || ! xml->writeTo (dir.getChildFile (clean + ".dbrpreset"))) return false;
    presetIndex = getPresetNames().indexOf (clean);
    return true;
}

//==============================================================================
void BatteryProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::ValueTree root ("DubBatteryReggaeState");
    root.setProperty ("version", "1.0", nullptr);
    root.setProperty ("presetIndex", presetIndex, nullptr);
    root.setProperty ("presetName", presetName, nullptr);
    root.setProperty ("selectedPad", selectedPad, nullptr);
    root.setProperty ("tab", currentTab, nullptr);
    root.appendChild (apvts.copyState(), nullptr);
    juce::ValueTree samples ("Samples");
    for (int i = 0; i < numPads; ++i)
        if (auto s = getSample (i)) samples.setProperty ("s" + juce::String (i), s->ref, nullptr);
    root.appendChild (samples, nullptr);
    juce::MemoryBlock drumsState;
    drums.getStateInformation (drumsState);
    root.setProperty ("drums", drumsState.toBase64Encoding(), nullptr);
    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, dest);
}

void BatteryProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName ("DubBatteryReggaeState")) return;
    auto root = juce::ValueTree::fromXml (*xml);
    auto params = root.getChildWithName (apvts.state.getType());
    if (params.isValid()) apvts.replaceState (params);
    auto samples = root.getChildWithName ("Samples");
    for (int i = 0; i < numPads; ++i)
        loadSampleRef (i, samples.getProperty ("s" + juce::String (i), "f:" + juce::String (i)).toString());
    presetIndex = root.getProperty ("presetIndex", 0);
    presetName  = root.getProperty ("presetName", "ROOTS DM").toString();
    selectedPad = juce::jlimit (0, numPads - 1, (int) root.getProperty ("selectedPad", 0));
    currentTab  = juce::jlimit (0, 1, (int) root.getProperty ("tab", 0));
    juce::MemoryBlock drumsState;
    if (drumsState.fromBase64Encoding (root.getProperty ("drums").toString()) && drumsState.getSize() > 0)
        drums.setStateInformation (drumsState.getData(), (int) drumsState.getSize());
}

//==============================================================================
juce::AudioProcessorEditor* BatteryProcessor::createEditor()
{
    return new BatteryEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BatteryProcessor();
}
