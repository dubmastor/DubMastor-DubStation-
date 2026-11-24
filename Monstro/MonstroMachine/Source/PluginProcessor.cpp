#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout MonstroProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (const String& id, const String& name, float def) { return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, 0.0f, 1.0f, def); };
    auto onOff = [] (const String& id, const String& name, bool def) { return std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def); };

    StringArray kitNames;
    for (auto& k : mm::kits()) kitNames.add ("Vol." + String (k.vol) + " " + String::fromUTF8 (k.name));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "kit", 1 }, "Kit", kitNames, 0));

    // ---- sequenciador
    l.add (onOff ("machineOn", "Toca com a DAW", true));
    l.add (onOff ("preview", "Play do plugin", false));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "bpm", 1 }, "BPM", 60, 180, mm::kits()[0].bpm));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "swing", 1 }, "Swing", 50, 75, 54));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "seq", 1 }, "SEQ", StringArray { "A", "B", "C", "D" }, 0));

    // ---- pads
    for (int p = 0; p < mm::numPads; ++p)
    {
        const String n = "Pad " + String (p + 1) + " ";
        l.add (std::make_unique<AudioParameterInt> (ParameterID { pid ("var", p), 1 }, n + "timbre", 0, 999, 0));
        l.add (std::make_unique<AudioParameterInt> (ParameterID { pid ("tune", p), 1 }, n + "tune", -12, 12, 0));
        l.add (std::make_unique<AudioParameterInt> (ParameterID { pid ("dec", p), 1 }, n + "decay %", 25, 200, 100));
        l.add (pct (pid ("vol", p), n + "volume", 1.0f));
        l.add (onOff (pid ("mute", p), n + "mudo", false));
    }

    // ---- efeitos (padrão de fábrica: saturador e Tape Echo em bypass)
    l.add (onOff ("satOn", "Saturador", false));
    l.add (pct ("drive", "Drive", 0.35f));
    l.add (onOff ("echoOn", "Tape echo", false));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "echoSteps", 1 }, "Eco tempo (passos)", 1, 8, 3));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "echoFb", 1 }, "Eco feedback", 0.0f, 0.85f, 0.45f));
    l.add (pct ("echoMix", "Eco mix", 0.3f));
    l.add (pct ("master", "Master", 0.8f));
    return l;
}

MonstroProcessor::MonstroProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MM2500", createLayout())
{
    for (auto& s : seqs) for (auto& p : s) for (auto& x : p) x.store (0);
    for (auto& h : padHits) h.store (0);
    cachedVar.fill (-1);
    selectKit (0);
}

bool MonstroProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void MonstroProcessor::setParam (const String& id, float v)
{
    if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void MonstroProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    synth.prepare (sampleRate);
    fx.prepare (sampleRate);
    previewBeat = 0; wasPreview = false;
    evs.reserve ((size_t) jmax (256, samplesPerBlock));
}

// ---------------------------------------------------------------- kit e grooves
void MonstroProcessor::selectKit (int index, bool keepSeqs)
{
    index = (index % mm::numKits() + mm::numKits()) % mm::numKits();
    const auto& k = mm::kits()[(size_t) index];
    setParam ("kit", (float) index);
    setParam ("bpm", (float) k.bpm);
    clearUserKit();
    for (int p = 0; p < mm::numPads; ++p) setParam (pid ("var", p), 0.0f);
    if (! keepSeqs) { styleOverride = {}; loadStyle (k.style); setParam ("seq", 0.0f); styleOverride = {}; }
}

void MonstroProcessor::loadStyle (const String& styleKey)
{
    const auto& pats = mm::patterns (styleKey.toStdString());
    for (int s = 0; s < mm::numSeqs; ++s)
        for (int p = 0; p < mm::numPads; ++p)
            for (int st = 0; st < mm::numSteps; ++st) setStep (s, p, st, pats[(size_t) s][(size_t) p][(size_t) st]);
    styleOverride = styleKey == String (mm::kits()[(size_t) kit()].style) ? String() : styleKey;
}

int MonstroProcessor::notesIn (int s) const
{
    int n = 0;
    for (int p = 0; p < mm::numPads; ++p) for (int st = 0; st < mm::numSteps; ++st) n += getStep (s, p, st) ? 1 : 0;
    return n;
}

void MonstroProcessor::clearSeq (int s) { for (int p = 0; p < mm::numPads; ++p) for (int st = 0; st < mm::numSteps; ++st) setStep (s, p, st, 0); }

void MonstroProcessor::pushUndo()
{
    std::vector<uint8_t> snap;
    snap.reserve (mm::numSeqs * mm::numPads * mm::numSteps);
    for (auto& s : seqs) for (auto& p : s) for (auto& x : p) snap.push_back (x.load());
    undoStack.push_back (std::move (snap));
    if (undoStack.size() > 50) undoStack.erase (undoStack.begin());
}

bool MonstroProcessor::undo()
{
    if (undoStack.empty()) return false;
    size_t i = 0;
    for (auto& s : seqs) for (auto& p : s) for (auto& x : p) x.store (undoStack.back()[i++]);
    undoStack.pop_back();
    return true;
}

// ---------------------------------------------------------------- timbres
bool MonstroProcessor::hasUserFiles (int pad) const { return ! famFiles[(size_t) mm::padFam()[(size_t) pad]].isEmpty(); }

int MonstroProcessor::varCount (int pad) const
{
    const int fam = mm::padFam()[(size_t) pad];
    if (! famFiles[(size_t) fam].isEmpty()) return famFiles[(size_t) fam].size();
    return mm::famCount (kit(), fam);
}

void MonstroProcessor::setVariation (int pad, int v)
{
    const int n = jmax (1, varCount (pad));
    v = ((v % n) + n) % n;
    setParam (pid ("var", pad), (float) v);
    const auto& files = famFiles[(size_t) mm::padFam()[(size_t) pad]];
    if (! files.isEmpty()) loadSample (pad, files[v]);
    playPad (pad, 0.9f);
}

String MonstroProcessor::timbreName (int pad) const
{
    if (hasSample (pad)) return sampleFile[(size_t) pad].getFileNameWithoutExtension();
    return String (mm::famNames()[(size_t) mm::padFam()[(size_t) pad]]) + " " + String (variation (pad) + 1).paddedLeft ('0', 2);
}

// ---------------------------------------------------------------- amostras
bool MonstroProcessor::loadSample (int pad, const File& f)
{
    AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr || r->lengthInSamples <= 0) return false;
    const int len = (int) std::min<int64> (r->lengthInSamples, (int64) (r->sampleRate * 12.0));
    AudioBuffer<float> b ((int) jlimit (1u, 2u, r->numChannels), len);
    r->read (&b, 0, len, 0, true, true);
    auto d = std::make_shared<mm::SampleData>();
    d->rate = r->sampleRate;
    d->l.assign (b.getReadPointer (0), b.getReadPointer (0) + len);
    if (b.getNumChannels() > 1) d->r.assign (b.getReadPointer (1), b.getReadPointer (1) + len);
    std::atomic_store (&samples[(size_t) pad], std::shared_ptr<const mm::SampleData> (d));
    sampleFile[(size_t) pad] = f;
    return true;
}

void MonstroProcessor::clearSample (int pad)
{
    std::atomic_store (&samples[(size_t) pad], std::shared_ptr<const mm::SampleData>());
    sampleFile[(size_t) pad] = File();
}

void MonstroProcessor::clearUserKit()
{
    userFolder = File();
    for (auto& f : famFiles) f.clear();
    for (int p = 0; p < mm::numPads; ++p) clearSample (p);
}

int MonstroProcessor::loadFolder (const File& dir)
{
    if (! dir.isDirectory()) return 0;
    auto files = dir.findChildFiles (File::findFiles, true, "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    files.sort();
    std::array<Array<File>, mm::numFams> byFam;
    for (auto& f : files)
    {
        // a família vem do nome do arquivo e, se não der, da pasta (Kicks/, Snares/…)
        int fam = mm::famOfContent (" " + f.getFileNameWithoutExtension().toStdString());
        if (fam < 0) fam = mm::famOfContent (" " + f.getParentDirectory().getFileName().toStdString());
        if (fam < 0 && f.getFileName().containsIgnoreCase ("hh")) fam = mm::HIHAT;
        if (fam >= 0) byFam[(size_t) fam].add (f);
    }
    int found = 0;
    for (auto& a : byFam) found += a.size();
    if (found == 0) return 0;
    for (int p = 0; p < mm::numPads; ++p) clearSample (p);
    famFiles = byFam;
    userFolder = dir;
    int loaded = 0;
    std::array<int, mm::numFams> used {};
    for (int p = 0; p < mm::numPads; ++p)
    {
        const int fam = mm::padFam()[(size_t) p];
        const auto& L = famFiles[(size_t) fam];
        if (L.isEmpty()) { setParam (pid ("var", p), 0.0f); continue; }
        const int v = used[(size_t) fam]++ % L.size();   // KICK 2, PERC 2… pegam o próximo arquivo da família
        setParam (pid ("var", p), (float) v);
        if (loadSample (p, L[v])) ++loaded;
    }
    return loaded;
}

void MonstroProcessor::playPad (int pad, float vel)
{
    int s1, n1, s2, n2;
    uiFifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) uiQueue[(size_t) s1] = { pad, vel };
    else if (n2 > 0) uiQueue[(size_t) s2] = { pad, vel };
    uiFifo.finishedWrite (n1 + n2);
}

// ---------------------------------------------------------------- MIDI para a DAW
double MonstroProcessor::currentBpm() const
{
    const double h = hostBpm.load();
    return get ("machineOn") > 0.5f && h > 0 ? h : (double) get ("bpm");
}

bool MonstroProcessor::writeMidiFile (const File& file, int seqIndex) const
{
    const int tpq = 480, step = tpq / 4;
    const double swing = (get ("swing") - 50.0) / 50.0;
    MidiMessageSequence meta, tr;
    meta.addEvent (MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / jlimit (30.0, 300.0, currentBpm()))), 0.0);
    meta.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    tr.addEvent (MidiMessage::textMetaEvent (3, "Monstro Machine MM-2500"), 0.0);
    int bar = 0, count = 0;
    for (int s = 0; s < mm::numSeqs; ++s)
    {
        if (seqIndex >= 0 && s != seqIndex) continue;
        for (int p = 0; p < mm::numPads; ++p)
        {
            if (get (pid ("mute", p)) > 0.5f) continue;
            for (int st = 0; st < mm::numSteps; ++st)
            {
                const int v = getStep (s, p, st);
                if (! v) continue;
                const int tick = bar * 16 * step + st * step + (st % 2 ? (int) std::round (swing * step) : 0);
                tr.addEvent (MidiMessage::noteOn (10, mm::midiNotes()[(size_t) p], (uint8) (v == 2 ? 120 : 90)), (double) tick);
                tr.addEvent (MidiMessage::noteOff (10, mm::midiNotes()[(size_t) p]), (double) (tick + step / 2));
                ++count;
            }
        }
        ++bar;
    }
    if (count == 0) return false;
    tr.addEvent (MidiMessage::endOfTrack(), (double) (bar * 16 * step));
    tr.sort(); tr.updateMatchedPairs();
    MidiFile mf; mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (meta); mf.addTrack (tr);
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    FileOutputStream out (file);
    return out.openedOk() && mf.writeTo (out);
}

File MonstroProcessor::tempMidi (int seqIndex) const
{
    String name ("Monstro " + String::fromUTF8 (mm::kits()[(size_t) kit()].name));
    name << (seqIndex < 0 ? String (" SEQ ABCD") : " SEQ " + String::charToString ((juce_wchar) ('A' + seqIndex)));
    name << " " << (int) std::round (currentBpm()) << "bpm";
    return File::getSpecialLocation (File::tempDirectory).getChildFile ("MM-2500").getChildFile (File::createLegalFileName (name) + ".mid");
}

// ---------------------------------------------------------------- áudio
void MonstroProcessor::refreshCache()
{
    const int k = kit();
    if (k != cachedKit) { ch = mm::charOf (k); cachedKit = k; cachedVar.fill (-1); }
    for (int p = 0; p < mm::numPads; ++p)
    {
        const int v = variation (p);
        if (v != cachedVar[(size_t) p]) { vars[(size_t) p] = mm::varOf (k, p, v); cachedVar[(size_t) p] = v; }
    }
}

void MonstroProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0 || buffer.getNumChannels() < 2) { midi.clear(); return; }
    refreshCache();

    // transporte: a DAW (TOCA COM A DAW) ou o PLAY do plugin
    const bool machineOn = get ("machineOn") > 0.5f, preview = get ("preview") > 0.5f;
    bool playing = false, hostPlaying = false; double beat0 = 0.0, bpm = get ("bpm");
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (pos->getBpm()) hostBpm = *pos->getBpm();
            if (machineOn)
            {
                if (pos->getBpm()) bpm = *pos->getBpm();
                if (pos->getIsPlaying() && pos->getPpqPosition()) { playing = hostPlaying = true; beat0 = *pos->getPpqPosition(); }
            }
        }
    if (! playing && preview) { if (! wasPreview) previewBeat = 0.0; playing = true; beat0 = previewBeat; bpm = get ("bpm"); }
    wasPreview = preview;
    const double spb = 60.0 / jmax (30.0, bpm), bps = 1.0 / (spb * sr), beat1 = beat0 + n * bps;

    mm::FxSettings fs;
    fs.satOn = get ("satOn") > 0.5f; fs.drive = get ("drive");
    fs.echoOn = get ("echoOn") > 0.5f; fs.echoSteps = (int) get ("echoSteps");
    fs.echoFb = get ("echoFb"); fs.echoMix = get ("echoMix"); fs.master = get ("master");
    fs.stepDur = spb / 4.0;

    evs.clear();
    // MIDI que chega: as notas dos pads (padrão GM, como a MPC)
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (! m.isNoteOn()) continue;
        for (int p = 0; p < mm::numPads; ++p)
            if (mm::midiNotes()[(size_t) p] == m.getNoteNumber()) { evs.push_back ({ meta.samplePosition, p, m.getFloatVelocity(), -1 }); break; }
    }
    midi.clear();
    {
        int s1, n1, s2, n2;
        uiFifo.prepareToRead (uiFifo.getNumReady(), s1, n1, s2, n2);
        for (int i = 0; i < n1 + n2; ++i)
        {
            const auto [pad, vel] = uiQueue[(size_t) (i < n1 ? s1 + i : s2 + i - n1)];
            evs.push_back ({ 0, jlimit (0, mm::numPads - 1, pad), vel, -1 });
        }
        uiFifo.finishedRead (n1 + n2);
    }

    if (playing)
    {
        const int sq = seq();
        const double swing = (get ("swing") - 50.0) / 50.0;   // atraso dos passos pares (2, 4…) em fração do passo
        const int64 kStart = (int64) std::floor (beat0 * 4.0) - 1, kEnd = (int64) std::ceil (beat1 * 4.0) + 1;
        for (int64 k = kStart; k <= kEnd; ++k)
        {
            const int st = (int) (((k % 16) + 16) % 16);
            const double eb = k / 4.0 + (st % 2 ? swing / 4.0 : 0.0);
            if (eb < beat0 || eb >= beat1) continue;
            const int at = jlimit (0, n - 1, (int) ((eb - beat0) / bps));
            for (int p = 0; p < mm::numPads; ++p)
            {
                const int v = getStep (sq, p, st);
                if (v && get (pid ("mute", p)) < 0.5f) evs.push_back ({ at, p, v == 2 ? 1.0f : 0.75f, st });
            }
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
            const auto smp = std::atomic_load (&samples[(size_t) e.pad]);
            synth.hit (e.pad, cachedKit, ch, vars[(size_t) e.pad], e.vel * get (pid ("vol", e.pad)),
                       get (pid ("tune", e.pad)), get (pid ("dec", e.pad)), smp);
            padHits[(size_t) e.pad].fetch_add (1);
        }
        const int next = ei < evs.size() ? jmin (n, evs[ei].at) : n;
        if (next > pos) synth.render (L + pos, R + pos, next - pos);
        pos = next;
        if (pos >= n) break;
    }
    fx.process (L, R, n, fs);
    outLevel = jmax (buffer.getMagnitude (0, 0, n), buffer.getMagnitude (1, 0, n));
}

// ---------------------------------------------------------------- estado (com os SEQs e as amostras)
void MonstroProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("MONSTRO"), nullptr);
    ValueTree g ("MONSTRO");
    for (int s = 0; s < mm::numSeqs; ++s)
    {
        String t;
        for (int p = 0; p < mm::numPads; ++p) for (int st = 0; st < mm::numSteps; ++st) t << getStep (s, p, st);
        g.setProperty ("seq" + String (s), t, nullptr);
    }
    for (int p = 0; p < mm::numPads; ++p) g.setProperty (pid ("sample", p), sampleFile[(size_t) p].getFullPathName(), nullptr);
    g.setProperty ("folder", userFolder.getFullPathName(), nullptr);
    g.setProperty ("style", styleOverride, nullptr);
    state.addChild (g, -1, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void MonstroProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = ValueTree::fromXml (*xml);
    auto g = tree.getChildWithName ("MONSTRO");
    tree.removeChild (g, nullptr);
    apvts.replaceState (tree);
    if (! g.isValid()) return;
    for (int s = 0; s < mm::numSeqs; ++s)
    {
        const String t = g.getProperty ("seq" + String (s)).toString();
        for (int p = 0; p < mm::numPads; ++p)
            for (int st = 0; st < mm::numSteps; ++st)
            {
                const int i = p * mm::numSteps + st;
                setStep (s, p, st, i < t.length() ? jlimit (0, 2, (int) (t[i] - '0')) : 0);
            }
    }
    styleOverride = g.getProperty ("style").toString();
    // a pasta do usuário (para o ◀ ▶) e o som de cada pad, sem mexer nos timbres escolhidos
    userFolder = File(); for (auto& f : famFiles) f.clear();
    const File folder (g.getProperty ("folder").toString());
    if (g.getProperty ("folder").toString().isNotEmpty() && folder.isDirectory())
    {
        Array<float> keep;
        for (int p = 0; p < mm::numPads; ++p) keep.add (get (pid ("var", p)));
        loadFolder (folder);
        for (int p = 0; p < mm::numPads; ++p) setParam (pid ("var", p), keep[p]);
    }
    for (int p = 0; p < mm::numPads; ++p)
    {
        const String path = g.getProperty (pid ("sample", p)).toString();
        if (path.isNotEmpty() && File (path).existsAsFile()) loadSample (p, File (path)); else clearSample (p);
    }
}

AudioProcessorEditor* MonstroProcessor::createEditor() { return new MonstroEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MonstroProcessor(); }
