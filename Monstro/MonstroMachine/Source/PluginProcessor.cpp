#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <regex>

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout MonstroProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (const String& id, const String& name, float def) { return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, 0.0f, 1.0f, def); };
    auto onOff = [] (const String& id, const String& name, bool def) { return std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def); };

    StringArray kitNames;
    for (auto& k : mm::kits()) kitNames.add ("Vol." + String (k.vol) + " " + String::fromUTF8 (k.name));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "kit", 1 }, "Kit", kitNames, 0));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "pkit", 1 }, "Kit do produtor", 0, 7, 0));

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
    if (! keepSeqs) { loadProducerGroove(); setParam ("seq", 0.0f); }
    selectPk (0);
}

void MonstroProcessor::loadProducerGroove()
{
    const auto g = mm::groove (kit());
    for (int s = 0; s < mm::numSeqs; ++s)
        for (int p = 0; p < mm::numPads; ++p)
            for (int st = 0; st < mm::numSteps; ++st) setStep (s, p, st, g[(size_t) s][(size_t) p][(size_t) st]);
    setParam ("swing", (float) mm::prof (kit()).sw);
    styleOverride = {};
}

int MonstroProcessor::kitCountOf (int producer) const
{
    const auto it = lib.find (producer);
    return it != lib.end() && ! it->second.empty() ? jmin (8, (int) it->second.size()) : mm::kitCountF (producer);
}

String MonstroProcessor::kitLabel (int producer, int n) const
{
    const auto it = lib.find (producer);
    if (it == lib.end() || n < 0 || n >= (int) it->second.size()) return {};
    return it->second[(size_t) n].name;
}

void MonstroProcessor::selectPk (int n)
{
    const int c = jmax (1, kitCount());
    n = ((n % c) + c) % c;
    setParam ("pkit", (float) n);
    for (auto& f : famFiles) f.clear();
    for (int p = 0; p < mm::numPads; ++p) clearSample (p);
    if (hasLib (kit())) { applyUserKit(); return; }
    const auto v = mm::kitVars (kit(), n);
    for (int p = 0; p < mm::numPads; ++p) setParam (pid ("var", p), (float) v[(size_t) p]);
}

// o kit da pasta vai para os pads: KICK 2, PERC 2… pegam o próximo arquivo da família
void MonstroProcessor::applyUserKit()
{
    const auto it = lib.find (kit());
    if (it == lib.end() || it->second.empty()) return;
    const auto& uk = it->second[(size_t) jmin (pk(), (int) it->second.size() - 1)];
    famFiles = uk.files;
    std::array<int, mm::numFams> used {};
    for (int p = 0; p < mm::numPads; ++p)
    {
        const int fam = mm::padFam()[(size_t) p];
        const auto& L = famFiles[(size_t) fam];
        const int j = used[(size_t) fam]++;
        clearSample (p);
        if (L.isEmpty()) { setParam (pid ("var", p), 0.0f); continue; }
        setParam (pid ("var", p), (float) (j % L.size()));
        loadSample (p, L[j % L.size()]);
    }
}

void MonstroProcessor::loadStyle (const String& styleKey)
{
    const auto& pats = mm::patterns (styleKey.toStdString());
    for (int s = 0; s < mm::numSeqs; ++s)
        for (int p = 0; p < mm::numPads; ++p)
            for (int st = 0; st < mm::numSteps; ++st) setStep (s, p, st, pats[(size_t) s][(size_t) p][(size_t) st]);
    styleOverride = styleKey;   // groove de um estilo (o do produtor volta com loadProducerGroove)
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
    for (auto& f : famFiles) f.clear();
    for (int p = 0; p < mm::numPads; ++p) clearSample (p);
}

void MonstroProcessor::forgetFolders()
{
    lib.clear(); folders.clear();
    selectPk (0);
}

// ---------------------------------------------------------------- pasta do usuário (mesma regra da web: buildLib)
namespace
{
    // família pelo nome (arquivo e pasta de cima), na mesma ordem da web (FAMFILE)
    int famOfFile (const std::string& s)
    {
        static const std::vector<std::pair<int, std::regex>> rules {
            { mm::VOX, std::regex ("vox|vocal|chant|voz", std::regex::icase) },
            { mm::OPENHAT, std::regex ("open|\\boh\\b", std::regex::icase) },
            { mm::KICK, std::regex ("kick|bumbo|\\bbd\\b", std::regex::icase) },
            { mm::SNARE, std::regex ("snare|caixa|\\bsd\\b", std::regex::icase) },
            { mm::CLAP, std::regex ("clap|palma|snap", std::regex::icase) },
            { mm::RIM, std::regex ("rim", std::regex::icase) },
            { mm::HIHAT, std::regex ("hat|\\bhh\\b|hihat", std::regex::icase) },
            { mm::CYMBAL, std::regex ("crash|cym|ride", std::regex::icase) },
            { mm::B808, std::regex ("808|bass|baixo", std::regex::icase) },
            { mm::SHAKER, std::regex ("shak", std::regex::icase) },
            { mm::PERC, std::regex ("perc|conga|bongo|tom|fill", std::regex::icase) },
            { mm::FX, std::regex ("fx|efeito|riser|impact", std::regex::icase) } };
        for (auto& [f, re] : rules) if (std::regex_search (s, re)) return f;
        return -1;
    }
    // minúsculas, sem acento e só letras e números (como o norm da web)
    std::string norm (const String& s)
    {
        static const char* latin = "aaaaaaaceeeeiiiidnooooo/ouuuuytsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
        std::string out;
        for (auto c : s.toLowerCase())
        {
            juce_wchar ch = c;
            if (ch >= 0xC0 && ch <= 0xFF) ch = (juce_wchar) latin[ch - 0xC0];
            if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) out += (char) ch;
        }
        return out;
    }
    struct Who { int ki = -1, at = -1; };
    Who producerOf (const StringArray& segs)
    {
        static std::vector<std::pair<std::string, int>> names;
        if (names.empty())
            for (int ki = 0; ki < mm::numKits(); ++ki)
                names.push_back ({ norm (String (std::regex_replace (std::string (mm::kits()[(size_t) ki].name), std::regex ("\\s*vol\\.?\\s*\\d+", std::regex::icase), "")).trim()), ki });
        const std::string path = norm (segs.joinIntoString ("/"));
        for (int i = 0; i < segs.size() - 1; ++i)
        {
            std::vector<std::string> w;
            for (auto& t : StringArray::fromTokens (segs[i], " _-.()[]\t", "")) { const auto n = norm (t); if (! n.empty()) w.push_back (n); }
            if (w.empty()) continue;
            std::vector<std::string> joins;
            for (size_t a = 0; a < w.size(); ++a) { std::string s; for (size_t b = a; b < w.size(); ++b) { s += w[b]; joins.push_back (s); } }
            std::vector<int> c;
            for (auto& [n, ki] : names) if (n.size() >= 3 && std::find (joins.begin(), joins.end(), n) != joins.end()) c.push_back (ki);
            if (c.empty()) continue;
            for (int ki : c) if (path.find ("vol" + std::to_string (mm::kits()[(size_t) ki].vol)) != std::string::npos) return { ki, i };
            return { c[0], i };
        }
        return {};
    }
}

std::map<int, std::vector<MonstroProcessor::UserKit>> MonstroProcessor::buildLib (const File& dir) const
{
    static const std::regex skip ("loop|melod|midi|preset|stem", std::regex::icase);
    static const std::regex generic ("^(drums?|one ?shots?|samples?|sounds?|wav|audio|kit|drum ?kit|monstro.*|vol.*)$", std::regex::icase);
    auto files = dir.findChildFiles (File::findFiles, true, "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    std::map<int, std::map<String, std::vector<std::pair<int, File>>>> by;
    for (auto& f : files)
    {
        // caminho como o da web: a pasta escolhida + o caminho dentro dela
        const String rel = dir.getFileName() + "/" + f.getRelativePathFrom (dir).replaceCharacter ('\\', '/');
        if (std::regex_search (rel.toStdString(), skip)) continue;
        StringArray segs; segs.addTokens (rel, "/", "");
        const int fam = famOfFile ((segs.size() > 1 ? segs[segs.size() - 2] + "/" : String()).toStdString() + segs[segs.size() - 1].toStdString());
        if (fam < 0) continue;
        const auto who = producerOf (segs);
        const int ki = who.ki >= 0 ? who.ki : kit();
        String key;
        for (int i = who.ki >= 0 ? who.at + 1 : 1; i < segs.size() - 1; ++i)
        {
            const String d = segs[i].trim();
            if (famOfFile (d.toStdString()) >= 0 || std::regex_match (d.toStdString(), generic)) continue;
            key = d; break;
        }
        by[ki][key].push_back ({ fam, f });
    }
    std::map<int, std::vector<UserKit>> out;
    for (auto& [ki, groups] : by)
    {
        auto pack = [] (std::vector<std::pair<int, File>> list)
        {
            std::sort (list.begin(), list.end(), [] (auto& a, auto& b) { return a.second.getFileName().compareIgnoreCase (b.second.getFileName()) < 0; });
            std::array<Array<File>, mm::numFams> byFam;
            for (auto& [fam, f] : list) byFam[(size_t) fam].add (f);
            return byFam;
        };
        std::vector<UserKit> kits;
        if (groups.size() == 1)
        {
            const auto& [key, list] = *groups.begin();
            const auto packed = pack (list);
            const int kk = packed[mm::KICK].size(), ss = packed[mm::SNARE].size() + packed[mm::CLAP].size();
            const int n = jmax (1, jmin (8, jmin (kk ? (kk + 1) / 2 : 8, ss ? ss : 8)));
            for (int j = 0; j < n; ++j)
            {
                UserKit u;
                u.name = key.isNotEmpty() ? key + (n > 1 ? " " + String (j + 1) : String()) : "PASTA " + String (j + 1);
                for (int f = 0; f < mm::numFams; ++f)
                {
                    for (int i = j; i < packed[(size_t) f].size(); i += n) u.files[(size_t) f].add (packed[(size_t) f][i]);
                    if (u.files[(size_t) f].isEmpty()) u.files[(size_t) f] = packed[(size_t) f];
                }
                kits.push_back (u);
            }
        }
        else
            for (auto& [key, list] : groups) { UserKit u; u.name = key.isNotEmpty() ? key : "SOLTOS"; u.files = pack (list); kits.push_back (u); }
        if (kits.size() > 8) kits.resize (8);
        out[ki] = kits;
    }
    return out;
}

int MonstroProcessor::loadFolder (const File& dir)
{
    if (! dir.isDirectory()) return 0;
    auto found = buildLib (dir);
    if (found.empty()) { lastLoadProducers = 0; return 0; }
    int kits = 0;
    for (auto& [ki, v] : found) { lib[ki] = v; kits += (int) v.size(); }
    lastLoadProducers = (int) found.size();
    folders.addIfNotAlreadyThere (dir.getFullPathName());
    if (found.count (kit())) selectPk (0);
    else selectKit (found.begin()->first);
    return kits;
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
    g.setProperty ("folders", folders.joinIntoString ("\n"), nullptr);
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
    // as pastas do usuário (kits por produtor, para o ◀ ▶) e o som de cada pad, sem mexer nos timbres escolhidos
    lib.clear(); folders.clear(); for (auto& f : famFiles) f.clear();
    StringArray saved; saved.addLines (g.getProperty ("folders").toString());
    if (g.getProperty ("folder").toString().isNotEmpty()) saved.add (g.getProperty ("folder").toString());   // projetos da 1.0
    for (auto& path : saved)
    {
        const File folder (path);
        if (path.isEmpty() || ! folder.isDirectory()) continue;
        for (auto& [ki, v] : buildLib (folder)) lib[ki] = v;
        folders.addIfNotAlreadyThere (path);
    }
    if (const auto it = lib.find (kit()); it != lib.end() && ! it->second.empty())
        famFiles = it->second[(size_t) jmin (pk(), (int) it->second.size() - 1)].files;
    for (int p = 0; p < mm::numPads; ++p)
    {
        const String path = g.getProperty (pid ("sample", p)).toString();
        if (path.isNotEmpty() && File (path).existsAsFile()) loadSample (p, File (path)); else clearSample (p);
    }
}

AudioProcessorEditor* MonstroProcessor::createEditor() { return new MonstroEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MonstroProcessor(); }
