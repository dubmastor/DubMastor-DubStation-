#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace { std::atomic<double> lastBpm { 75.0 }; }

AudioProcessorValueTreeState::ParameterLayout ReggaePercussionProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (const String& id, const String& name, float def) { return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, 0.0f, 1.0f, def); };
    auto onOff = [] (const String& id, const String& name, bool def) { return std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def); };
    auto range = [] (const String& id, const String& name, float lo, float hi, float def, float skewCentre = 0.0f)
    {
        NormalisableRange<float> r (lo, hi);
        if (skewCentre > 0) r.setSkewForCentre (skewCentre);
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def);
    };

    StringArray snds;
    for (auto& s : mrp::sounds()) snds.add (String::fromUTF8 (s.name) + " " + String::fromUTF8 (s.sub));

    // ---- as 6 peças
    for (int t = 0; t < mrp::numTracks; ++t)
    {
        const auto& d = mrp::trackDefaults()[(size_t) t];
        const String n (t + 1);
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { tid ("snd", t), 1 }, "Peca " + n + " som", snds, mrp::soundIndex (d.soundKey)));
        l.add (onOff (tid ("mute", t), "Peca " + n + " mudo", false));
        l.add (onOff (tid ("solo", t), "Peca " + n + " solo", false));
        l.add (pct (tid ("dec", t), "Peca " + n + " duracao", d.dec));
        l.add (pct (tid ("vol", t), "Peca " + n + " volume", d.vol));
        l.add (range (tid ("pan", t), "Peca " + n + " pan", -1.0f, 1.0f, d.pan));
    }

    // ---- PERC MACHINE
    l.add (onOff ("machineOn", "Toca com a DAW", true));
    l.add (onOff ("preview", "Preview", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM do preview", NormalisableRange<float> (56.0f, 100.0f, 1.0f), 75.0f));
    l.add (range ("swing", "Swing", 0.5f, 0.7f, 0.58f));
    l.add (pct ("human", "Humanizar", 0.4f));
    l.add (onOff ("guide", "Guia caixa + palheta", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "bars", 1 }, "Compassos", StringArray { "1", "2", "4", "8" }, 2));

    // ---- efeitos (padrão de fábrica: saturador e Tape Echo em bypass)
    l.add (onOff ("eqOn", "EQ", true));
    l.add (range ("hpf", "EQ HPF", 20.0f, 2000.0f, 30.0f, 200.0f));
    l.add (range ("bellF", "EQ bell freq", 200.0f, 12000.0f, 2500.0f, 2000.0f));
    l.add (range ("bellG", "EQ bell ganho", -12.0f, 12.0f, 0.0f));
    l.add (range ("lpf", "EQ LPF", 1000.0f, 20000.0f, 18000.0f, 6000.0f));
    l.add (onOff ("satOn", "Saturador", false));
    l.add (pct ("drive", "Drive", 0.3f));
    l.add (onOff ("echoOn", "Tape echo", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "echoTime", 1 }, "Eco tempo", StringArray { "1/8 pontuada", "1/8", "1/4", "1/4 pontuada" }, 0));
    l.add (range ("echoFb", "Eco feedback", 0.0f, 0.85f, 0.45f));
    l.add (pct ("echoMix", "Eco mix", 0.3f));
    l.add (pct ("big", "Mastor Big Knob (saida)", 0.8f));

    // ---- MIDI
    l.add (onOff ("midiColor", "MIDI colorido", true));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "midiCh", 1 }, "Canal MIDI", 1, 16, 10));
    l.add (onOff ("midiSwing", "Swing no MIDI", true));
    return l;
}

ReggaePercussionProcessor::ReggaePercussionProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MRP77", createLayout())
{
    for (auto& tr : pat) for (auto& s : tr) s.store (0);
    for (auto& h : padHits) h.store (0);
    for (auto& h : trackHits) h.store (0);
    for (auto& v : trackVel) v.store (0.0f);
    resetNotes();
    loadGroove (0);
}

void ReggaePercussionProcessor::resetNotes()
{
    for (int i = 0; i < mrp::numPads; ++i) padNote[(size_t) i].store (mrp::pads()[(size_t) i].note);
}

bool ReggaePercussionProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void ReggaePercussionProcessor::setParam (const String& id, float v)
{
    if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void ReggaePercussionProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    synth.prepare ((float) sampleRate);
    fx.prepare ((float) sampleRate);
    previewBeat = 0; wasPreview = false;
    evs.reserve ((size_t) std::max (256, samplesPerBlock));
}

// ---------------------------------------------------------------- grade
int ReggaePercussionProcessor::numBars() const { static const int b[4] = { 1, 2, 4, 8 }; return b[jlimit (0, 3, (int) get ("bars"))]; }

bool ReggaePercussionProcessor::isAudible (int t) const
{
    bool anySolo = false;
    for (int i = 0; i < mrp::numTracks; ++i) anySolo = anySolo || get (tid ("solo", i)) > 0.5f;
    return get (tid ("mute", t)) < 0.5f && (! anySolo || get (tid ("solo", t)) > 0.5f);
}

void ReggaePercussionProcessor::loadGroove (int index)
{
    const auto& gs = mrp::grooves();
    index = jlimit (0, (int) gs.size() - 1, index);
    const auto& g = gs[(size_t) index];
    for (int t = 0; t < mrp::numTracks; ++t)
        for (int s = 0; s < 128; ++s) setStep (t, s, mrp::stepValue (g.p[t], s % 16));
    setParam ("swing", g.swing / 100.0f);
    grooveIndex = index;
}

void ReggaePercussionProcessor::clearPattern()
{
    for (int t = 0; t < mrp::numTracks; ++t) for (int s = 0; s < 128; ++s) setStep (t, s, 0);
    grooveIndex = (int) mrp::grooves().size() - 1;   // "Vazio"
}

String ReggaePercussionProcessor::grooveName() const { return mrp::grooves()[(size_t) jlimit (0, (int) mrp::grooves().size() - 1, grooveIndex.load())].name; }

void ReggaePercussionProcessor::cycleSound (int t, int dir)
{
    setParam (tid ("snd", t), (float) ((soundOf (t) + dir + mrp::numSounds) % mrp::numSounds));
    playPad (mrp::padOf (soundOf (t), true), 0.95f);
}

void ReggaePercussionProcessor::playPad (int pad, float vel)
{
    int s1, n1, s2, n2;
    uiFifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) uiQueue[(size_t) s1] = { pad, vel };
    else if (n2 > 0) uiQueue[(size_t) s2] = { pad, vel };
    uiFifo.finishedWrite (n1 + n2);
}

// ---------------------------------------------------------------- amostras
bool ReggaePercussionProcessor::loadSample (int t, const File& f)
{
    AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr || r->lengthInSamples <= 0) return false;
    const int len = (int) std::min<int64> (r->lengthInSamples, (int64) (r->sampleRate * 10.0));
    AudioBuffer<float> b ((int) std::min<unsigned> (2u, r->numChannels), len);
    r->read (&b, 0, len, 0, true, true);
    auto d = std::make_shared<mrp::SampleData>();
    d->rate = r->sampleRate;
    d->l.assign (b.getReadPointer (0), b.getReadPointer (0) + len);
    if (b.getNumChannels() > 1) d->r.assign (b.getReadPointer (1), b.getReadPointer (1) + len);
    std::atomic_store (&samples[(size_t) t], std::shared_ptr<const mrp::SampleData> (d));
    sampleFile[(size_t) t] = f;
    return true;
}

void ReggaePercussionProcessor::clearSample (int t)
{
    std::atomic_store (&samples[(size_t) t], std::shared_ptr<const mrp::SampleData>());
    sampleFile[(size_t) t] = File();
}

// ---------------------------------------------------------------- MIDI para a DAW
bool ReggaePercussionProcessor::writeMidiFile (const File& file, int track, int bar) const
{
    std::array<std::array<uint8_t, 128>, mrp::numTracks> p {};
    std::array<int, mrp::numTracks> snd {}; std::array<bool, mrp::numTracks> aud {}; std::array<int, mrp::numPads> notes {};
    for (int t = 0; t < mrp::numTracks; ++t)
    {
        for (int s = 0; s < 128; ++s) p[(size_t) t][(size_t) s] = (uint8_t) getStep (t, s);
        snd[(size_t) t] = soundOf (t); aud[(size_t) t] = isAudible (t);
    }
    for (int i = 0; i < mrp::numPads; ++i) notes[(size_t) i] = padNote[(size_t) i].load();
    const auto ns = mrp::buildNotes (p, snd, aud, notes, numBars(), bar, track, get ("midiColor") > 0.5f,
                                     (int) get ("midiCh"), get ("midiSwing") > 0.5f, get ("swing"));
    if (ns.empty()) return false;
    const int tpq = 480;
    const double bpm = get ("machineOn") > 0.5f ? lastBpm.load() : (double) get ("bpm");
    MidiMessageSequence meta, tr;
    meta.addEvent (MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / jlimit (30.0, 300.0, bpm))), 0.0);
    meta.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    tr.addEvent (MidiMessage::textMetaEvent (3, "Mastor Reggae Percussion MRP-77"), 0.0);
    for (const auto& e : ns)
    {
        tr.addEvent (MidiMessage::noteOn (e.ch, e.note, (uint8) e.vel), (double) e.tick);
        tr.addEvent (MidiMessage::noteOff (e.ch, e.note), (double) (e.tick + e.len));
    }
    tr.sort(); tr.updateMatchedPairs();
    MidiFile mf; mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (meta); mf.addTrack (tr);
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    FileOutputStream out (file);
    return out.openedOk() && mf.writeTo (out);
}

File ReggaePercussionProcessor::tempMidi (int track, int bar) const
{
    String name ("MRP-77 ");
    if (track >= 0) name << String::fromUTF8 (mrp::sounds()[(size_t) soundOf (track)].name) << " ";
    name << grooveName();
    if (bar >= 0) name << " C" << (bar + 1);
    name = File::createLegalFileName (name);
    return File::getSpecialLocation (File::tempDirectory).getChildFile ("MRP-77").getChildFile (name + ".mid");
}

// ---------------------------------------------------------------- áudio
void ReggaePercussionProcessor::trigger (int track, int snd, float vel, bool lg, int, float stepDur)
{
    float dec = 0.3f, vol = 0.7f, pan = 0.0f;
    std::shared_ptr<const mrp::SampleData> smp;
    if (track >= 0)
    {
        dec = get (tid ("dec", track)); vol = get (tid ("vol", track)); pan = get (tid ("pan", track));
        smp = std::atomic_load (&samples[(size_t) track]);
        trackHits[(size_t) track].fetch_add (1); trackVel[(size_t) track].store (vel);
    }
    synth.hit (snd, vel, lg, dec, vol, pan, stepDur, smp);
    padHits[(size_t) mrp::padOf (snd, lg)].fetch_add (1);
}

void ReggaePercussionProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0 || buffer.getNumChannels() < 2) { midi.clear(); return; }

    mrp::FxSettings fs;
    fs.eqOn = get ("eqOn") > 0.5f; fs.hpf = get ("hpf"); fs.bellF = get ("bellF"); fs.bellG = get ("bellG"); fs.lpf = get ("lpf");
    fs.satOn = get ("satOn") > 0.5f; fs.drive = get ("drive");
    static const int echoSteps[4] = { 3, 2, 4, 6 };
    fs.echoOn = get ("echoOn") > 0.5f; fs.echoSteps = echoSteps[jlimit (0, 3, (int) get ("echoTime"))];
    fs.echoFb = get ("echoFb"); fs.echoMix = get ("echoMix"); fs.big = get ("big");

    // transporte: a DAW (TOCA COM A DAW) ou o PLAY do plugin
    const bool machineOn = get ("machineOn") > 0.5f, preview = get ("preview") > 0.5f;
    bool playing = false, hostPlaying = false; double beat0 = 0.0, bpm = get ("bpm");
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (pos->getBpm()) lastBpm = *pos->getBpm();
            if (machineOn)
            {
                if (pos->getBpm()) bpm = *pos->getBpm();
                if (pos->getIsPlaying() && pos->getPpqPosition()) { playing = hostPlaying = true; beat0 = *pos->getPpqPosition(); }
            }
        }
    if (! playing && preview) { if (! wasPreview) previewBeat = 0.0; playing = true; beat0 = previewBeat; bpm = get ("bpm"); }
    wasPreview = preview;
    fs.bpm = (float) bpm;
    const double spb = 60.0 / jmax (30.0, bpm), bps = 1.0 / (spb * sr), beat1 = beat0 + n * bps;
    const float stepDur = (float) (spb / 4.0);

    evs.clear();
    // MIDI que chega: cada nota toca o pad dela (as notas do MRD-12 não tocam nada aqui)
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (! m.isNoteOn()) continue;
        for (int p = 0; p < mrp::numPads; ++p)
            if (padNote[(size_t) p].load() == m.getNoteNumber())
            {
                const auto& pd = mrp::pads()[(size_t) p];
                int track = -1;
                for (int t = 0; t < mrp::numTracks; ++t) if (soundOf (t) == pd.snd) { track = t; break; }
                evs.push_back ({ meta.samplePosition, track, pd.snd, m.getFloatVelocity(), pd.lg == 1 || (pd.lg < 0 && m.getFloatVelocity() > 0.8f), false, -1 });
                break;
            }
    }
    midi.clear();
    // toques na tela
    {
        int s1, n1, s2, n2;
        uiFifo.prepareToRead (uiFifo.getNumReady(), s1, n1, s2, n2);
        auto add = [&] (int i)
        {
            const auto [pad, vel] = uiQueue[(size_t) i];
            const auto& pd = mrp::pads()[(size_t) jlimit (0, mrp::numPads - 1, pad)];
            int track = -1;
            for (int t = 0; t < mrp::numTracks; ++t) if (soundOf (t) == pd.snd) { track = t; break; }
            evs.push_back ({ 0, track, pd.snd, vel, pd.lg == 1, false, -1 });
        };
        for (int i = 0; i < n1; ++i) add (s1 + i);
        for (int i = 0; i < n2; ++i) add (s2 + i);
        uiFifo.finishedRead (n1 + n2);
    }

    if (playing)
    {
        const int total = numBars() * 16;
        const double swing = get ("swing"), h = get ("human");
        const bool guide = get ("guide") > 0.5f;
        const int64 kStart = (int64) std::floor (beat0 * 4.0) - 1, kEnd = (int64) std::ceil (beat1 * 4.0) + 1;
        for (int64 k = kStart; k <= kEnd; ++k)
        {
            const int st = (int) (((k % total) + total) % total), s16 = st % 16;
            const double eb = k / 4.0 + (s16 % 2 ? (swing * 2.0 - 1.0) / 4.0 : 0.0);
            if (eb < beat0 || eb >= beat1) continue;
            const int at = (int) ((eb - beat0) / bps);
            for (int t = 0; t < mrp::numTracks; ++t)
            {
                const int v = getStep (t, st);
                if (! v || ! isAudible (t)) continue;
                const float vel = (float) ((v == 2 ? 1.0 : 0.45) * (1.0 - h * 0.15 * rng.next()));
                const int jit = (int) ((rng.next() - 0.5) * h * 0.01 * sr);
                evs.push_back ({ jlimit (0, n - 1, at + jit), t, soundOf (t), vel, v == 2, false, st });
            }
            if (guide) evs.push_back ({ jlimit (0, n - 1, at), -1, 0, 1.0f, false, true, s16 });
            playStep = st;
        }
        if (preview && ! hostPlaying) previewBeat = beat1;
    }
    else playStep = -1;

    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.at < b.at; });
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    int pos = 0; size_t ei = 0;
    for (;;)
    {
        while (ei < evs.size() && evs[ei].at <= pos)
        {
            const auto& e = evs[ei++];
            if (e.guide) synth.guide (e.step, stepDur);
            else trigger (e.track, e.snd, e.vel, e.lg, e.at, stepDur);
        }
        const int next = ei < evs.size() ? std::min (n, evs[ei].at) : n;
        if (next > pos) synth.render (L + pos, R + pos, next - pos);
        pos = next;
        if (pos >= n) break;
    }
    fx.process (L, R, n, fs);
    outLevel = std::max (mrp::FxChain::levelOf (L, n), mrp::FxChain::levelOf (R, n));
}

// ---------------------------------------------------------------- estado (com a grade, as notas e as amostras)
void ReggaePercussionProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("GRADE"), nullptr);
    ValueTree g ("GRADE");
    for (int t = 0; t < mrp::numTracks; ++t)
    {
        String s; for (int i = 0; i < 128; ++i) s << getStep (t, i);
        g.setProperty (tid ("t", t), s, nullptr);
        g.setProperty (tid ("sample", t), sampleFile[(size_t) t].getFullPathName(), nullptr);
    }
    StringArray ns; for (auto& x : padNote) ns.add (String (x.load()));
    g.setProperty ("notes", ns.joinIntoString (","), nullptr);
    g.setProperty ("groove", grooveIndex.load(), nullptr);
    state.addChild (g, -1, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void ReggaePercussionProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            auto g = tree.getChildWithName ("GRADE");
            tree.removeChild (g, nullptr);
            apvts.replaceState (tree);
            if (g.isValid())
            {
                for (int t = 0; t < mrp::numTracks; ++t)
                {
                    const String s = g.getProperty (tid ("t", t)).toString();
                    for (int i = 0; i < 128; ++i) setStep (t, i, i < s.length() ? jlimit (0, 2, (int) (s[i] - '0')) : 0);
                    const String path = g.getProperty (tid ("sample", t)).toString();
                    if (path.isNotEmpty() && File (path).existsAsFile()) loadSample (t, File (path)); else clearSample (t);
                }
                auto ns = StringArray::fromTokens (g.getProperty ("notes").toString(), ",", "");
                if (ns.size() == mrp::numPads) for (int i = 0; i < mrp::numPads; ++i) padNote[(size_t) i] = jlimit (0, 127, ns[i].getIntValue());
                grooveIndex = (int) g.getProperty ("groove", 0);
            }
        }
}

AudioProcessorEditor* ReggaePercussionProcessor::createEditor() { return new ReggaePercussionEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ReggaePercussionProcessor(); }
