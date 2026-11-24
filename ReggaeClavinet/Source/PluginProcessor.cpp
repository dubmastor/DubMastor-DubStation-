#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "RiddimBank.h"

using namespace juce;

namespace
{
// timbres = ajustes do instrumento (o eco nunca liga sozinho: Tape Echo começa sempre desligado)
struct Timbre { const char* name; const char* lcd; bool f[4]; bool cd, ab; float bright, decay, mute; bool wah, ph, m1; };
const Timbre timbres[6] = {
    { "M1",     "M1 Clav",         { true, true, false, false },  true,  true,  0.95f, 0.45f, 0.12f, false, false, true },   // estilo do Clav do Korg M1 (timbre inicial)
    { "D6",     "D6 Funky Reggae", { true, false, true, false },  true,  false, 0.85f, 0.50f, 0.08f, false, false, false },
    { "MUTE",   "D6 Muted Chop",   { false, true, true, false },  false, true,  0.75f, 0.40f, 0.55f, false, false, false },
    { "WAH",    "Wah Skank",       { true, true, false, false },  false, true,  0.90f, 0.45f, 0.20f, true,  false, false },
    { "PHASER", "Phase Riff",      { false, true, true, false },  true,  true,  0.60f, 0.55f, 0.10f, false, true,  false },
    { "DUB",    "Dark Dub Clav",   { false, false, true, true },  true,  false, 0.40f, 0.60f, 0.20f, false, true,  false } };
const char* const filterIds[4] = { "fBrill", "fTreble", "fMedium", "fSoft" };
}

juce::String timbreLcdName (int t) { return timbres[jlimit (0, 5, t)].lcd; }
juce::String timbreName (int t) { return timbres[jlimit (0, 5, t)].name; }

AudioProcessorValueTreeState::ParameterLayout ReggaeClavinetProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (const char* id, const char* name, float def) { return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, 0.0f, 1.0f, def); };
    auto onOff = [] (const char* id, const char* name, bool def) { return std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def); };
    auto choice = [] (const String& id, const String& name, const StringArray& items, int def) { return std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def); };

    // ---- INSTRUMENTO
    l.add (choice ("timbre", "Timbre", { "M1", "D6", "MUTE", "WAH", "PHASER", "DUB" }, 0));
    l.add (onOff ("m1", "Brilho M1", true));
    l.add (onOff ("fBrill", "Filtro BRILL.", true));
    l.add (onOff ("fTreble", "Filtro TREBLE", true));
    l.add (onOff ("fMedium", "Filtro MEDIUM", false));
    l.add (onOff ("fSoft", "Filtro SOFT", false));
    l.add (onOff ("pickCD", "Captador C/D", true));
    l.add (onOff ("pickAB", "Captador A/B", true));
    l.add (pct ("bright", "Brilho", 0.95f));
    l.add (pct ("decay", "Decay", 0.45f));
    l.add (pct ("mute", "Mute (la)", 0.12f));
    l.add (pct ("vol", "Volume", 0.80f));
    l.add (onOff ("wahOn", "Auto-wah", false));
    l.add (pct ("wahSens", "Wah sensibilidade", 0.6f));
    l.add (pct ("wahRange", "Wah faixa", 0.7f));
    l.add (onOff ("phOn", "Phaser", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "phRate", 1 }, "Phaser rate", NormalisableRange<float> (0.05f, 2.0f, 0.01f), 0.4f));
    l.add (pct ("phDepth", "Phaser depth", 0.55f));
    l.add (onOff ("echoOn", "Tape echo", false));   // padrão de fábrica do kit: começa desligado
    l.add (choice ("echoTime", "Eco tempo", { "1/8", "3/16", "1/4", "3/8" }, 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "echoFb", 1 }, "Eco feedback", 0.0f, 0.85f, 0.4f));
    l.add (pct ("echoMix", "Eco volume", 0.45f));
    l.add (onOff ("maxOn", "Maximizer", true));
    l.add (pct ("reverb", "Reverb", 0.2f));
    l.add (pct ("master", "Master", 0.8f));

    // ---- CLAVINET MACHINE
    l.add (onOff ("machineOn", "Toca com a DAW", false));
    l.add (onOff ("preview", "Preview", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM do preview", NormalisableRange<float> (60.0f, 100.0f, 1.0f), 76.0f));
    l.add (choice ("src", "Acordes", { "Do plugin", "MIDI da trilha" }, 0));
    StringArray roots; for (int r = 0; r < 12; ++r) roots.add (mrcl::rootName (r));
    StringArray durs; for (int d = 0; d < 6; ++d) durs.add (mrcl::durName (d));
    const int defRoot[4] = { 9, 2, 7, 4 }; const bool defMinor[4] = { true, true, false, true };
    for (int i = 0; i < numSlots; ++i)
    {
        const String n (i + 1);
        l.add (choice ("root" + n, "Acorde " + n + " tonica", roots, defRoot[i]));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { "minor" + n, 1 }, "Acorde " + n + " menor", defMinor[i]));
        l.add (choice ("dur" + n, "Acorde " + n + " duracao", durs, 1));
        if (i > 0) l.add (std::make_unique<AudioParameterBool> (ParameterID { "on" + n, 1 }, "Acorde " + n + " ligado", true));
    }
    StringArray mus; for (auto& m : mrcl::musicians()) mus.add (m.name);
    l.add (choice ("musician", "Clavinetista", mus, 0));
    l.add (choice ("use", "Sugestao em uso", { "A SKANK DUPLO", "B CONTRAPONTO", "C RIFF EM OITAVAS" }, 0));
    l.add (choice ("fill", "Fill", { "SEM", "PUXADA", "RAJADA", "SUBIDA" }, 0));
    l.add (choice ("fillEvery", "Fill a cada", { "4 compassos", "8 compassos" }, 0));
    l.add (choice ("nota", "Notas", { "1/64", "1/32" }, 0));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "center", 1 }, "Registro", 17, 77, 29));   // C0 (nomes do Cubase)
    l.add (pct ("density", "Densidade", 0.8f));
    l.add (pct ("swing", "Swing", 0.15f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "velocity", 1 }, "Velocity", 0.2f, 1.0f, 0.85f));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "seed", 1 }, "Semente", 1, 999999, 1));
    l.add (choice ("reps", "Repetir no MIDI", { "x1", "x2", "x4" }, 1));
    return l;
}

ReggaeClavinetProcessor::ReggaeClavinetProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MRCL71", createLayout())
{
    rebuildLines();
    startTimerHz (20);
}

ReggaeClavinetProcessor::~ReggaeClavinetProcessor() { stopTimer(); }

bool ReggaeClavinetProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void ReggaeClavinetProcessor::setParam (const String& id, float v)
{
    if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void ReggaeClavinetProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    ins.prepare ((float) sampleRate);
    numOffs = 0; sampleClock = 0; previewBeat = 0; wasPlaying = false; held.fill (false);
    evs.reserve (1024);
}

// ---------------------------------------------------------------- CLAVINET MACHINE
std::vector<mrcl::Chord> ReggaeClavinetProcessor::getChords() const
{
    std::vector<mrcl::Chord> v;
    for (int i = 0; i < numSlots; ++i)
    {
        const String n (i + 1);
        if (i > 0 && get ("on" + n) < 0.5f) continue;
        mrcl::Chord c;
        c.slot = i; c.root = (int) get ("root" + n); c.minor = get ("minor" + n) > 0.5f;
        c.beats = mrcl::durBeats[jlimit (0, 5, (int) get ("dur" + n))];
        v.push_back (c);
    }
    return v;
}

mrcl::Settings ReggaeClavinetProcessor::getSettings() const
{
    mrcl::Settings s;
    s.musician = (int) get ("musician"); s.fill = (int) get ("fill"); s.fillEvery = get ("fillEvery") > 0.5f ? 8 : 4;
    s.nota = (int) get ("nota"); s.center = (int) get ("center"); s.density = get ("density"); s.swing = get ("swing");
    s.velocity = get ("velocity"); s.seed = (uint32_t) get ("seed");
    return s;
}

String ReggaeClavinetProcessor::settingsKey() const
{
    String k;
    for (const char* id : { "musician", "fill", "fillEvery", "nota", "center", "density", "swing", "velocity", "seed", "src" })
        k << id << '=' << String (get (id), 3) << ';';
    for (auto& c : getChords()) k << c.slot << ':' << c.root << ':' << (int) c.minor << ':' << c.beats << ';';
    return k;
}

void ReggaeClavinetProcessor::rebuildLines()
{
    auto set = std::make_shared<LineSet>();
    const auto s = getSettings();
    const auto cs = getChords();
    for (int st = 0; st < 3; ++st)
    {
        set->l[(size_t) st][24] = mrcl::generate (st, cs, s);
        for (int r = 0; r < 12; ++r)
            for (int m = 0; m < 2; ++m)
            {
                std::vector<mrcl::Chord> one { { 0, r, m == 1, 4 } };
                set->l[(size_t) st][(size_t) (r * 2 + m)] = mrcl::generate (st, one, s);
            }
    }
    std::atomic_store (&lines, std::shared_ptr<const LineSet> (set));
}

void ReggaeClavinetProcessor::timerCallback()
{
    const auto key = settingsKey();
    if (key != lastKey)
    {
        lastKey = key;
        rebuildLines();
        if (hasEdits() && editsKey != key) clearEdits();   // mudou a harmonia ou o músico: volta para a sugestão
    }
}

mrcl::Line ReggaeClavinetProcessor::getSuggestion (int style) const
{
    auto ls = std::atomic_load (&lines);
    if (ls == nullptr) return {};
    return ls->l[(size_t) jlimit (0, 2, style)][24];
}

mrcl::Line ReggaeClavinetProcessor::getCurrentLine() const
{
    if (auto e = std::atomic_load (&edits)) return *e;
    return getSuggestion ((int) get ("use"));
}

void ReggaeClavinetProcessor::setEdits (const mrcl::Line& l)
{
    editsKey = settingsKey();
    std::atomic_store (&edits, std::shared_ptr<const mrcl::Line> (std::make_shared<mrcl::Line> (l)));
}

void ReggaeClavinetProcessor::clearEdits() { std::atomic_store (&edits, std::shared_ptr<const mrcl::Line>()); }

void ReggaeClavinetProcessor::newIdeas()
{
    clearEdits();
    setParam ("seed", (float) Random::getSystemRandom().nextInt ({ 1, 999999 }));
}

void ReggaeClavinetProcessor::applyTimbre (int t)
{
    const auto& T = timbres[jlimit (0, 5, t)];
    setParam ("timbre", (float) t);
    for (int k = 0; k < 4; ++k) setParam (filterIds[k], T.f[k] ? 1.0f : 0.0f);
    setParam ("pickCD", T.cd ? 1.0f : 0.0f); setParam ("pickAB", T.ab ? 1.0f : 0.0f);
    setParam ("bright", T.bright); setParam ("decay", T.decay); setParam ("mute", T.mute);
    setParam ("wahOn", T.wah ? 1.0f : 0.0f); setParam ("phOn", T.ph ? 1.0f : 0.0f); setParam ("m1", T.m1 ? 1.0f : 0.0f);
}

void ReggaeClavinetProcessor::applyBankEntry (int i)
{
    if (i < 0 || i >= riddimbank::numEntries) return;
    const auto& e = riddimbank::entries[i];
    if (e.numChords <= 0) return;
    bankEntry = i;
    for (int k = 0; k < numSlots; ++k)
    {
        const String n (k + 1);
        if (k < e.numChords)
        {
            setParam ("root" + n, (float) e.chords[k].root);
            setParam ("minor" + n, e.chords[k].minor ? 1.0f : 0.0f);
            setParam ("dur" + n, (float) jlimit (0, 5, e.chords[k].dur));
            if (k > 0) setParam ("on" + n, 1.0f);
        }
        else if (k > 0) setParam ("on" + n, 0.0f);
    }
    setParam ("bpm", (float) jlimit (60, 100, e.bpm));
    setParam ("src", 0.0f);
}

double ReggaeClavinetProcessor::exportBpm() const { return get ("bpm"); }

bool ReggaeClavinetProcessor::writeMidiFile (const File& file) const
{
    const auto line = getCurrentLine();
    if (line.events.empty()) return false;
    static const int repsTable[3] = { 1, 2, 4 };
    const int reps = repsTable[jlimit (0, 2, (int) get ("reps"))], tpq = 960;
    MidiMessageSequence meta, tr;
    meta.addEvent (MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / exportBpm())), 0.0);
    meta.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    tr.addEvent (MidiMessage::textMetaEvent (3, "Mastor Reggae Clavinet MRCL-71"), 0.0);
    // cada acorde no seu canal (acorde 1..4 = canal 1..4): na DAW cada acorde aparece numa cor
    for (int r = 0; r < reps; ++r)
        for (const auto& e : line.events)
        {
            const double t0 = (e.start + 4.0 * line.bars * r) * tpq;
            const int ch = jlimit (1, 16, e.slot + 1);
            tr.addEvent (MidiMessage::noteOn (ch, e.note, (uint8) e.vel), t0);
            tr.addEvent (MidiMessage::noteOff (ch, e.note), t0 + e.length * tpq);
        }
    tr.sort(); tr.updateMatchedPairs();
    MidiFile mf; mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (meta); mf.addTrack (tr);
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    FileOutputStream out (file);
    return out.openedOk() && mf.writeTo (out);
}

// ---------------------------------------------------------------- áudio
void ReggaeClavinetProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    // parâmetros do som
    ins.clav.pickupCD = get ("pickCD") > 0.5f; ins.clav.pickupAB = get ("pickAB") > 0.5f;
    ins.clav.bright = get ("bright"); ins.clav.decay = get ("decay"); ins.clav.mute = get ("mute");
    for (int k = 0; k < 4; ++k) ins.fx.filters[(size_t) k] = get (filterIds[k]) > 0.5f;
    ins.fx.volume = get ("vol");
    ins.fx.wahOn = get ("wahOn") > 0.5f; ins.fx.wahSens = get ("wahSens"); ins.fx.wahRange = get ("wahRange");
    ins.fx.phOn = get ("phOn") > 0.5f; ins.fx.phRate = get ("phRate"); ins.fx.phDepth = get ("phDepth");
    ins.fx.echoOn = get ("echoOn") > 0.5f; ins.fx.echoTime = (int) get ("echoTime"); ins.fx.echoFb = get ("echoFb"); ins.fx.echoMix = get ("echoMix");
    ins.fx.m1 = get ("m1") > 0.5f;
    ins.fx.maxOn = get ("maxOn") > 0.5f; ins.fx.reverb = get ("reverb"); ins.fx.master = get ("master");

    // transporte: a DAW (TOCA COM A DAW) ou o PREVIEW
    const bool machineOn = get ("machineOn") > 0.5f, preview = get ("preview") > 0.5f, fromMidi = get ("src") > 0.5f;
    bool playing = false, hostPlaying = false; double beat0 = 0.0, bpm = get ("bpm");
    if (machineOn)
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                if (pos->getBpm()) bpm = *pos->getBpm();
                if (pos->getIsPlaying() && pos->getPpqPosition()) { playing = hostPlaying = true; beat0 = *pos->getPpqPosition(); }
            }
    if (! playing && preview) { playing = true; beat0 = previewBeat; }
    ins.fx.bpm = bpm;
    const double spb = 60.0 / std::max (30.0, bpm), beatsPerSample = 1.0 / (spb * sr);
    const double beat1 = beat0 + n * beatsPerSample;

    // eventos do bloco: (amostra, nota, força; força < 0 = soltar)
    evs.clear();

    // MIDI que chega: toca o clavinet; com a machine seguindo o MIDI da trilha, vira o acorde da linha
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const bool chordInput = machineOn && fromMidi;
        if (m.isNoteOn()) { if (chordInput) held[(size_t) m.getNoteNumber()] = true; else evs.push_back ({ meta.samplePosition, m.getNoteNumber(), m.getFloatVelocity() }); }
        else if (m.isNoteOff()) { if (chordInput) held[(size_t) m.getNoteNumber()] = false; else evs.push_back ({ meta.samplePosition, m.getNoteNumber(), -1.0f }); }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) { held.fill (false); ins.allOff(); }
    }
    if (machineOn && fromMidi)
    {
        mrcl::Chord c;
        bool h[128]; for (int i = 0; i < 128; ++i) h[i] = held[(size_t) i];
        if (mrcl::detectChord (h, c)) { midiChordRoot = c.root; midiChordMinor = c.minor ? 1 : 0; }
    }

    if (playing != wasPlaying || (playing && std::abs (beat0 - lastBlockEndBeat) > 0.02))
    {   // parou, começou ou pulou: solta o que a linha estava tocando
        for (int i = 0; i < numOffs; ++i) ins.noteOff (offs[(size_t) i].note);
        numOffs = 0;
    }
    wasPlaying = playing;
    lastBlockEndBeat = beat1;

    if (playing)
    {
        mrcl::Line line;
        auto ed = std::atomic_load (&edits);
        auto ls = std::atomic_load (&lines);
        const int use = (int) get ("use");
        const int mr = midiChordRoot.load();
        const bool lineFromMidi = fromMidi && machineOn && mr >= 0;
        const mrcl::Line* lp = nullptr;
        if (ed != nullptr && ! lineFromMidi) lp = ed.get();
        else if (ls != nullptr) lp = &ls->l[(size_t) jlimit (0, 2, use)][(size_t) (lineFromMidi ? mr * 2 + midiChordMinor.load() : 24)];
        if (lp != nullptr && ! lp->events.empty())
        {
            const double L = lp->bars * 4.0;
            for (const auto& e : lp->events)
            {
                double t = e.start + std::ceil ((beat0 - e.start) / L - 1e-12) * L;
                for (; t < beat1; t += L)
                {
                    const int at = jlimit (0, n - 1, (int) ((t - beat0) / beatsPerSample));
                    evs.push_back ({ at, e.note, (float) e.vel / 127.0f });
                    if (numOffs < (int) offs.size())
                        offs[(size_t) numOffs++] = { sampleClock + at + (int64_t) std::max (1.0, e.length * spb * sr), e.note };
                }
            }
            playBeat = std::fmod (beat0, L);
        }
    }
    else playBeat = -1.0;
    if (preview && ! hostPlaying) previewBeat = beat1;
    if (! preview) previewBeat = 0.0;

    // soltas que caem neste bloco
    for (int i = 0; i < numOffs;)
    {
        if (offs[(size_t) i].at < sampleClock + n)
        {
            evs.push_back ({ (int) jlimit<int64_t> (0, n - 1, offs[(size_t) i].at - sampleClock), offs[(size_t) i].note, -1.0f });
            offs[(size_t) i] = offs[(size_t) --numOffs];
        }
        else ++i;
    }
    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.at < b.at || (a.at == b.at && a.vel < b.vel); });   // solta antes de tocar

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    std::vector<float> tmpR;
    float* r = R; if (r == nullptr) { tmpR.assign ((size_t) n, 0.0f); r = tmpR.data(); }
    int pos = 0;
    size_t ei = 0;
    for (;;)
    {
        while (ei < evs.size() && evs[ei].at <= pos)
        {
            if (evs[ei].vel < 0) ins.noteOff (evs[ei].note); else ins.noteOn (evs[ei].note, evs[ei].vel);
            ++ei;
        }
        const int next = ei < evs.size() ? std::min (n, evs[ei].at) : n;
        if (next > pos) ins.render (L + pos, r + pos, next - pos);
        pos = next;
        if (pos >= n) break;
    }
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
    sampleClock += n;
}

// ---------------------------------------------------------------- estado (com o MIDI editado)
void ReggaeClavinetProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("EDITS"), nullptr);
    state.setProperty ("bank", bankEntry.load(), nullptr);
    if (auto e = std::atomic_load (&edits))
    {
        ValueTree t ("EDITS");
        t.setProperty ("bars", e->bars, nullptr);
        StringArray rows;
        for (auto& ev : e->events) rows.add (String (ev.start, 5) + "," + String (ev.length, 5) + "," + String (ev.note) + "," + String (ev.vel) + "," + String (ev.slot));
        t.setProperty ("notes", rows.joinIntoString (";"), nullptr);
        state.addChild (t, -1, nullptr);
    }
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void ReggaeClavinetProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            auto t = tree.getChildWithName ("EDITS");
            bankEntry = (int) tree.getProperty ("bank", -1);
            tree.removeChild (t, nullptr);
            apvts.replaceState (tree);
            lastKey = settingsKey();
            rebuildLines();
            clearEdits();
            if (t.isValid())
            {
                mrcl::Line l; l.bars = (int) t.getProperty ("bars", 2);
                for (auto& row : StringArray::fromTokens (t.getProperty ("notes").toString(), ";", ""))
                {
                    auto f = StringArray::fromTokens (row, ",", "");
                    if (f.size() < 5) continue;
                    l.events.push_back ({ f[0].getDoubleValue(), f[1].getDoubleValue(), f[2].getIntValue(), f[3].getIntValue(), f[4].getIntValue() });
                }
                if (! l.events.empty()) setEdits (l);
            }
        }
}

AudioProcessorEditor* ReggaeClavinetProcessor::createEditor() { return new ReggaeClavinetEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ReggaeClavinetProcessor(); }
