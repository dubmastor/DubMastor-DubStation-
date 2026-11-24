#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "RiddimBank.h"   // banco único de riddims da Dub Station (RiddimBank/Source)

using namespace mro;

const juce::StringArray& ReggaeOrganProcessor::drawbarNames()
{
    static const juce::StringArray names { "16'", "5 1/3'", "8'", "4'", "2 2/3'", "2'", "1 3/5'", "1 1/3'", "1'" };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout ReggaeOrganProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (const char* id, const char* name, float def)
    { return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def); };
    auto onOff = [] (const char* id, const char* name, bool def)
    { return std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def); };
    auto choice = [] (const juce::String& id, const juce::String& name, StringArray items, int def)
    { return std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def); };

    const Musician& m = musician (MusMittoo);

    // ---- transporte (BPM na contagem com a caixa no 2 e no 4)
    l.add (onOff ("groove", "Groove ligado", true));
    l.add (onOff ("play", "Play (sem a DAW)", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM (sem a DAW)", NormalisableRange<float> (50.0f, 140.0f, 1.0f), m.bpm));

    // ---- músico e época
    {
        StringArray mus, eps;
        for (int i = 0; i < NumMusicians; ++i) mus.add (musician (i).name);
        for (int i = 0; i < numEpochs; ++i) eps.add (epochName (i));
        l.add (choice ("musico", "Musico", mus, MusMittoo));
        l.add (choice ("epoca", "Epoca", eps, 0));
    }

    // ---- a grade (16 semicolcheias, mão direita e mão esquerda)
    {
        const mro::Grid g = gridFrom (m.rh, m.lh);
        const StringArray lv { "-", "fantasma", "acento" };
        for (int i = 0; i < 16; ++i)
            l.add (choice (gridId (true, i), "Mao direita " + String (i + 1), lv, g.rh[i]));
        for (int i = 0; i < 16; ++i)
            l.add (choice (gridId (false, i), "Mao esquerda " + String (i + 1), lv, g.lh[i]));
    }

    // ---- ajustes do groove
    l.add (choice ("beat", "Batida", { "One Drop", "Rockers", "Steppers" }, m.beat));
    l.add (choice ("nota", "Duracao da nota", { "1/32", "1/64" }, 1));
    l.add (pct ("swing", "Swing", m.swing));
    l.add (pct ("human", "Humanizar", m.human));
    l.add (pct ("ghost", "Ghost (nota fantasma)", m.ghost));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "seed", 1 }, "Variacao", 1, 64, 1));

    // ---- camadas
    l.add (onOff ("bubbleOn", "Bubble", true));
    l.add (onOff ("chopOn", "Chop", true));
    l.add (onOff ("fillsOn", "Fills", true));
    l.add (pct ("bubbleLvl", "Bubble volume", 0.85f));
    l.add (pct ("chopLvl", "Chop + Fills volume", 0.75f));
    l.add (choice ("fillStyle", "Estilo dos fills", { "Licks Mittoo", "Melodia Wright", "Acordes Downie", "Fragmentos Gladdy" }, m.fillStyle));
    l.add (choice ("fillEvery", "Fill a cada", { "2 compassos", "4 compassos", "8 compassos" }, 1));

    // ---- acordes (só maior e menor, durações iguais às do banco de riddims)
    l.add (choice ("chordSrc", "Acordes", { "Do plugin", "Do teclado (MIDI)" }, 0));
    {
        StringArray roots, durs;
        for (int r = 0; r < 12; ++r) roots.add (rootName (r));
        for (int d = 0; d < numChordDurations; ++d) durs.add (chordDurationName (d));
        const int defRoot[numSlots] = { 9, 2, 9, 4 };
        for (int i = 0; i < numSlots; ++i)
        {
            const String n (i + 1), lbl = "Acorde " + n + " ";
            l.add (choice ("root" + n, lbl + "tonica", roots, defRoot[i]));
            l.add (std::make_unique<AudioParameterBool> (ParameterID { "minor" + n, 1 }, lbl + "menor", true));
            l.add (choice ("dur" + n, lbl + "duracao", durs, 1));
            if (i > 0) l.add (std::make_unique<AudioParameterBool> (ParameterID { "on" + n, 1 }, lbl + "ligado", true));
        }
    }
    l.add (choice ("reps", "Repetir no MIDI", { "x1", "x2", "x4" }, 1));

    // ---- registros (dois organistas: mão esquerda e mão direita)
    for (int i = 0; i < 9; ++i)
        l.add (std::make_unique<AudioParameterInt> (ParameterID { "bdb" + String (i + 1), 1 }, "Bubble drawbar " + drawbarNames()[i], 0, 8, m.bubbleDb[i]));
    for (int i = 0; i < 9; ++i)
        l.add (std::make_unique<AudioParameterInt> (ParameterID { "cdb" + String (i + 1), 1 }, "Chop drawbar " + drawbarNames()[i], 0, 8, m.chopDb[i]));

    // ---- som
    l.add (onOff ("perc", "Percussao (mao direita)", m.perc));
    l.add (pct ("drive", "Drive", m.drive));
    l.add (pct ("click", "Key click", 0.35f));
    l.add (onOff ("echoOn", "Eco de fita", false));
    l.add (pct ("echoVol", "Eco volume", 0.45f));
    l.add (pct ("echoInt", "Eco intensidade", 0.40f));
    l.add (pct ("echoRate", "Eco velocidade", 0.50f));
    l.add (pct ("reverb", "Reverb", 0.15f));
    l.add (pct ("master", "Master", 0.80f));
    l.add (onOff ("maxOn", "Maximizer", true));
    // ---- Leslie de dois rotores (1.1)
    {
        StringArray names;
        for (int i = 0; i < NumLesliePresets; ++i) names.add (String::fromUTF8 (lesliePreset (i).name));
        l.add (choice ("lesPreset", "Leslie preset", names, leslieFromMusician (m.leslie)));
    }
    l.add (choice ("lesSpeed", "Leslie velocidade", { "Lenta", "Rapida" }, m.leslie == 2 ? 1 : 0));
    l.add (onOff ("lesBrake", "Leslie freio", false));
    l.add (pct ("lesDepth", "Leslie profundidade", m.leslie ? lesliePreset (leslieFromMusician (m.leslie)).depth : 0.8f));
    l.add (choice ("instr", "Instrumento", { "Hammond B3", "Rhodes Stage", "Rhodes Suitcase", "Wurlitzer 200A", "FM EP (DX7)" }, 0));
    // CORES do visor e canais do MIDI: por acorde (igual ao MRP-80: acorde 1..4 = canal 1..4) ou por camada (1 bubble, 2 chop, 3 fills)
    l.add (choice ("colorBy", "Cores do MIDI", { "Por acorde", "Por camada" }, 0));
    return l;
}

ReggaeOrganProcessor::ReggaeOrganProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (auto& x : lit) x.store (false);
}

bool ReggaeOrganProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

const juce::String ReggaeOrganProcessor::getProgramName (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) NumMusicians)) return {};
    return juce::String (musician (index).who);
}

void ReggaeOrganProcessor::setParam (const juce::String& id, float value)
{
    if (auto* prm = apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
        prm->endChangeGesture();
    }
}

// ---------------------------------------------------------------- os três caminhos
void ReggaeOrganProcessor::applyMusician (int mIndex)
{
    if (! juce::isPositiveAndBelow (mIndex, (int) NumMusicians)) return;
    const Musician& m = musician (mIndex);
    clearEdits();
    setParam ("musico", (float) mIndex);
    const Grid g = gridFrom (m.rh, m.lh);
    for (int i = 0; i < 16; ++i)
    {
        setParam (gridId (true, i), (float) g.rh[i]);
        setParam (gridId (false, i), (float) g.lh[i]);
    }
    setParam ("beat", (float) m.beat);
    setParam ("swing", m.swing);
    setParam ("human", m.human);
    setParam ("ghost", m.ghost);
    setParam ("fillStyle", (float) m.fillStyle);
    setParam ("fillEvery", m.fillEvery == 2 ? 0.0f : (m.fillEvery == 4 ? 1.0f : 2.0f));
    setParam ("bpm", m.bpm);
    for (int i = 0; i < 9; ++i)
    {
        setParam ("bdb" + juce::String (i + 1), (float) m.bubbleDb[i]);
        setParam ("cdb" + juce::String (i + 1), (float) m.chopDb[i]);
    }
    {
        const int lp = leslieFromMusician (m.leslie);
        setParam ("lesPreset", (float) lp);
        setParam ("lesSpeed", m.leslie == 2 ? 1.0f : 0.0f);
        setParam ("lesBrake", 0.0f);
        if (lp > 0) setParam ("lesDepth", lesliePreset (lp).depth);
    }
    setParam ("perc", m.perc ? 1.0f : 0.0f);
    setParam ("drive", m.drive);
    apvts.state.setProperty ("epochSel", -1, nullptr);
}

void ReggaeOrganProcessor::applyEpoch (int e)
{
    if (! juce::isPositiveAndBelow (e, numEpochs)) return;
    const Epoch& ep = epoch (e);
    applyMusician (ep.musician);
    setParam ("beat", (float) ep.beat);
    setParam ("bpm", ep.bpm);
    if (ep.feel >= 0) applyFeel (ep.feel, DenEA);
    setParam ("epoca", (float) e);
    apvts.state.setProperty ("epochSel", e, nullptr);
}

int ReggaeOrganProcessor::getEpoch() const
{
    const int e = apvts.state.getProperty ("epochSel", -1);
    return juce::isPositiveAndBelow (e, numEpochs) ? e : -1;
}

void ReggaeOrganProcessor::applyBankEntry (int index)
{
    if (index < 0 || index >= riddimbank::numEntries) return;
    const auto& e = riddimbank::entries[index];
    if (e.numChords <= 0) return;   // harmonia ainda pendente
    applyEpoch (e.style);           // o organista da época da gravação
    for (int i = 0; i < numSlots; ++i)
    {
        const juce::String n (i + 1);
        const bool used = i < e.numChords;
        if (used)
        {
            setParam ("root" + n, (float) e.chords[i].root);
            setParam ("minor" + n, e.chords[i].minor ? 1.0f : 0.0f);
            setParam ("dur" + n, (float) juce::jlimit (0, numChordDurations - 1, e.chords[i].dur));
        }
        if (i > 0) setParam ("on" + n, used ? 1.0f : 0.0f);
    }
    // kit da bateria do banco: 0 ROCKERS, 1 ONE DROP, 2 BOOM BAP, 3 DEMBOW
    setParam ("beat", (float) (e.kit == 0 ? BeatRockers : (e.kit == 1 ? BeatOneDrop : BeatSteppers)));
    setParam ("bpm", (float) e.bpm);
    setParam ("chordSrc", 0.0f);
    apvts.state.setProperty ("bank", index, nullptr);
}

int ReggaeOrganProcessor::getBankEntry() const
{
    const int b = apvts.state.getProperty ("bank", -1);
    return b >= 0 && b < riddimbank::numEntries ? b : -1;
}

void ReggaeOrganProcessor::applyFeel (int feel, int density)
{
    const Grid g = makeGrid (feel, density);
    for (int i = 0; i < 16; ++i)
    {
        setParam (gridId (true, i), (float) g.rh[i]);
        setParam (gridId (false, i), (float) g.lh[i]);
    }
}

void ReggaeOrganProcessor::setGridCell (bool rightHand, int step, int level)
{
    if (juce::isPositiveAndBelow (step, 16)) setParam (gridId (rightHand, step), (float) juce::jlimit (0, 2, level));
}

Grid ReggaeOrganProcessor::getGrid() const
{
    Grid g;
    for (int i = 0; i < 16; ++i)
    {
        g.rh[i] = (uint8_t) juce::jlimit (0, 2, (int) get (gridId (true, i)));
        g.lh[i] = (uint8_t) juce::jlimit (0, 2, (int) get (gridId (false, i)));
    }
    return g;
}

void ReggaeOrganProcessor::newVariation()
{
    const int cur = (int) get ("seed");
    int s = cur;
    juce::Random rnd;
    while (s == cur) s = 1 + rnd.nextInt (64);
    clearEdits();
    setParam ("seed", (float) s);
}

// ---------------------------------------------------------------- leitura dos parâmetros
std::vector<Chord> ReggaeOrganProcessor::getChords() const
{
    std::vector<Chord> v;
    for (int i = 0; i < numSlots; ++i)
    {
        const juce::String n (i + 1);
        if (i > 0 && get ("on" + n) < 0.5f) continue;
        Chord c;
        c.root  = (int) get ("root" + n);
        c.minor = get ("minor" + n) > 0.5f;
        c.beats = chordDurationBeats ((int) get ("dur" + n));
        v.push_back (c);
    }
    return v;
}

std::vector<int> ReggaeOrganProcessor::getChordSlots() const
{
    std::vector<int> v;
    for (int i = 0; i < numSlots; ++i)
        if (i == 0 || get ("on" + juce::String (i + 1)) > 0.5f) v.push_back (i);
    return v;
}

int ReggaeOrganProcessor::chordSlotForBeat (double beat) const
{
    if (get ("chordSrc") > 0.5f) return -1;
    const auto chords = getChords();
    const auto slots = getChordSlots();
    const int k = chordIndexForBeat (chords, beat);
    return k >= 0 && k < (int) slots.size() ? slots[(size_t) k] : -1;
}

bool ReggaeOrganProcessor::colourByChord() const
{
    return get ("colorBy") < 0.5f && get ("chordSrc") < 0.5f;
}

Settings ReggaeOrganProcessor::getSettings() const
{
    static const int everyTable[3] = { 2, 4, 8 };
    Settings s;
    s.grid      = getGrid();
    s.beat      = (int) get ("beat");
    s.noteSize  = (int) get ("nota");
    s.swing     = get ("swing");
    s.human     = get ("human");
    s.ghost     = get ("ghost");
    s.seed      = (int) get ("seed");
    s.bubbleOn  = get ("bubbleOn") > 0.5f;
    s.chopOn    = get ("chopOn") > 0.5f;
    s.fillsOn   = get ("fillsOn") > 0.5f;
    s.fillStyle = (int) get ("fillStyle");
    s.fillEvery = everyTable[juce::jlimit (0, 2, (int) get ("fillEvery"))];
    return s;
}

SoundParams ReggaeOrganProcessor::getSound() const
{
    SoundParams sp;
    for (int i = 0; i < 9; ++i)
    {
        sp.bubbleDb[i] = get ("bdb" + juce::String (i + 1));
        sp.chopDb[i]   = get ("cdb" + juce::String (i + 1));
    }
    sp.bubbleLevel   = get ("bubbleLvl");
    sp.ghostLevel    = get ("ghost");
    sp.chopLevel     = get ("chopLvl");
    sp.lesPreset     = (int) get ("lesPreset");
    sp.lesSpeed      = (int) get ("lesSpeed");
    sp.lesBrake      = get ("lesBrake") > 0.5f;
    sp.lesDepth      = get ("lesDepth");
    sp.perc          = get ("perc") > 0.5f;
    sp.drive         = get ("drive");
    sp.click         = get ("click");
    sp.echoOn        = get ("echoOn") > 0.5f;
    sp.echoVol       = get ("echoVol");
    sp.echoIntensity = get ("echoInt");
    sp.echoRate      = get ("echoRate");
    sp.reverb        = get ("reverb");
    sp.master        = get ("master");
    sp.maxOn         = get ("maxOn") > 0.5f;
    sp.instr         = (int) get ("instr");
    return sp;
}

// ---------------------------------------------------------------- MIDI editado
ReggaeOrganProcessor::Loop ReggaeOrganProcessor::getCurrentLoop() const
{
    if (auto e = std::atomic_load (&edits)) return *e;
    Loop l;
    l.events = buildLoop (getChords(), getSettings(), l.bars);
    return l;
}

void ReggaeOrganProcessor::setEdits (const std::vector<Event>& events, int bars)
{
    auto l = std::make_shared<Loop>();
    l->events = events;
    std::sort (l->events.begin(), l->events.end(), [] (const Event& a, const Event& b) { return a.start < b.start; });
    l->bars = juce::jmax (1, bars);
    std::atomic_store (&edits, std::shared_ptr<const Loop> (l));
}

void ReggaeOrganProcessor::clearEdits() { std::atomic_store (&edits, std::shared_ptr<const Loop>()); }

// ---------------------------------------------------------------- áudio
void ReggaeOrganProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateHz = sampleRate;
    trio.prepare (sampleRate);
    trio.setParams (getSound());
    setLatencySamples (trio.latencySamples());
    player.reset();
    cmds.reserve (1024);
    std::fill (std::begin (held), std::end (held), false);
    haveKeysChord = false;
    previewPpq = 0.0;
    keyboardState.reset();
    for (auto& x : lit) x.store (false);
}

void ReggaeOrganProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
    if (buffer.getNumChannels() < 2) { buffer.clear(); return; }

    trio.setParams (getSound());
    const Settings s = getSettings();
    const bool fromKeys = get ("chordSrc") > 0.5f;

    // teclado da tela junto com o MIDI da DAW
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    // transporte: DAW tocando, ou o PLAY interno com a DAW parada
    double bpm = 0.0, ppq = 0.0;
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())         bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            hostPlaying = pos->getIsPlaying();
        }
    if (bpm > 0.0) hostBpm.store (bpm);
    const bool preview = get ("play") > 0.5f && ! hostPlaying;
    if (preview) { bpm = (double) get ("bpm"); ppq = previewPpq; }
    else previewPpq = 0.0;
    const bool running = get ("groove") > 0.5f && (hostPlaying || preview) && bpm > 0.0;

    cmds.clear();

    // MIDI que chega: no modo "acordes do teclado" com o groove rodando ele só escolhe o acorde;
    // nos outros casos toca o órgão da mão direita normalmente
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int t = juce::jlimit (0, n - 1, meta.samplePosition);
        if (m.isNoteOn())       held[m.getNoteNumber()] = true;
        else if (m.isNoteOff()) held[m.getNoteNumber()] = false;
        else if (m.isAllNotesOff() || m.isAllSoundOff()) std::fill (std::begin (held), std::end (held), false);

        if (running && fromKeys) continue;
        if (m.isNoteOn())       cmds.push_back ({ t, OrganTrio::Chop, m.getNoteNumber(), (int) m.getVelocity(), -1, true });
        else if (m.isNoteOff()) cmds.push_back ({ t, OrganTrio::Chop, m.getNoteNumber(), 0, -1, false });
        else if (m.isAllNotesOff() || m.isAllSoundOff()) trio.allNotesOff();
        else if (m.isController() && m.getControllerNumber() == 1)
            trio.engine (OrganTrio::Chop).setModWheel ((float) m.getControllerValue() / 127.0f);
    }
    {
        Chord c;
        if (detectChord (held, c)) { keysChord = c; haveKeysChord = true; }
    }

    const auto chords = getChords();
    const auto ed = std::atomic_load (&edits);
    const int lb = ed ? ed->bars : loopBars (chords, s);
    auto chordsOf = [&] (int bar, Chord out[4])
    {
        if (fromKeys) { for (int i = 0; i < 4; ++i) out[i] = haveKeysChord ? keysChord : chords.front(); }
        else chordsOfBar (chords, bar, out);
    };
    auto eventsFor = [&] (int bar) -> std::vector<Event>
    {
        if (ed && ! fromKeys) return eventsOfBar (ed->events, ed->bars, bar);
        Chord bc[4];
        chordsOf (bar, bc);
        return barEvents (bar, bc, s);
    };

    if (running)
    {
        const size_t first = cmds.size();
        player.process (ppq, bpm, sampleRateHz, n, eventsFor, cmds);
        for (size_t i = first; i < cmds.size(); ++i)
            if (cmds[i].layer >= 0) lit[cmds[i].note].store (cmds[i].on);
        const double p = std::max (0.0, ppq);
        const int bar = (int) std::floor (p / 4.0);
        Chord bc[4];
        chordsOf (bar, bc);
        const int beatInBar = juce::jlimit (0, 3, (int) std::floor (p - 4.0 * bar));
        statusBar.store (fromKeys ? bar : bar % juce::jmax (1, lb));
        statusRoot.store (bc[beatInBar].root);
        statusMinor.store (bc[beatInBar].minor);
        statusStep.store (juce::jlimit (0, 15, (int) std::floor ((p - 4.0 * bar) * 4.0)));
        playPos.store (fromKeys ? -1.0 : std::fmod (p, 4.0 * juce::jmax (1, lb)));
        if (preview) previewPpq = ppq + (double) n / sampleRateHz * bpm / 60.0;
    }
    else
    {
        if (player.isRunning())
        {
            player.stop (cmds, 0);
            for (auto& x : lit) x.store (false);
        }
        statusStep.store (-1);
        playPos.store (-1.0);
    }
    statusRunning.store (running);

    std::stable_sort (cmds.begin(), cmds.end(), [] (const NoteCmd& a, const NoteCmd& b)
    {
        if (a.sample != b.sample) return a.sample < b.sample;
        return (int) a.on < (int) b.on;
    });

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    int pos = 0;
    for (const auto& c : cmds)
    {
        if (c.sample > pos) { trio.render (L + pos, R + pos, c.sample - pos); pos = c.sample; }
        if (c.on) trio.engine (c.engine).noteOn (c.note, (float) c.velocity / 127.0f);
        else      trio.engine (c.engine).noteOff (c.note);
    }
    if (pos < n) trio.render (L + pos, R + pos, n - pos);

    float ms = 0;
    for (int i = 0; i < n; ++i) ms += 0.5f * (L[i] * L[i] + R[i] * R[i]);
    if (n > 0) meter.store (ms / (float) n);
}

double ReggaeOrganProcessor::exportBpm() const
{
    const double h = hostBpm.load();
    return h > 0.0 ? h : (double) get ("bpm");
}

bool ReggaeOrganProcessor::writeMidiFile (const juce::File& file, int layer) const
{
    const Loop loop = getCurrentLoop();
    if (loop.bars <= 0 || loop.events.empty()) return false;

    static const int repsTable[3] = { 1, 2, 4 };
    const int reps = repsTable[juce::jlimit (0, 2, (int) get ("reps"))];
    const int tpq = 960;

    juce::MidiMessageSequence meta, tr[NumLayers];
    meta.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / exportBpm())), 0.0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    const char* names[NumLayers] = { "Reggae Organ - Bubble (mao esquerda)", "Reggae Organ - Chop (mao direita)", "Reggae Organ - Fills" };
    for (int k = 0; k < NumLayers; ++k) tr[k].addEvent (juce::MidiMessage::textMetaEvent (3, names[k]), 0.0);

    // uma trilha por camada; o canal MIDI dá a cor na DAW:
    //   CORES POR ACORDE  -> acorde 1..4 = canal 1..4 (igual ao MRP-80 e às outras machines)
    //   CORES POR CAMADA  -> 1 bubble, 2 chop, 3 fills
    const bool byChord = colourByChord();
    for (int r = 0; r < reps; ++r)
        for (const auto& e : loop.events)
        {
            if (layer >= 0 && e.layer != layer) continue;
            const double t0 = (e.start + 4.0 * loop.bars * r) * tpq;
            const int slot = byChord ? chordSlotForBeat (e.start) : -1;
            const int ch = slot >= 0 ? slot + 1 : e.layer + 1;
            tr[e.layer].addEvent (juce::MidiMessage::noteOn (ch, e.note, (juce::uint8) e.velocity), t0);
            tr[e.layer].addEvent (juce::MidiMessage::noteOff (ch, e.note), t0 + e.length * tpq);
        }

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (meta);
    int added = 0;
    for (auto& t : tr)
    {
        t.sort();
        t.updateMatchedPairs();
        if (t.getNumEvents() > 1) { mf.addTrack (t); ++added; }
    }
    if (added == 0) return false;
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    juce::FileOutputStream out (file);
    return out.openedOk() && mf.writeTo (out);
}

juce::AudioProcessorEditor* ReggaeOrganProcessor::createEditor()
{
    return new ReggaeOrganEditor (*this);
}

// ---------------------------------------------------------------- estado (com o MIDI editado)
void ReggaeOrganProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("EDITS"), nullptr);
    if (auto e = std::atomic_load (&edits))
    {
        juce::ValueTree t ("EDITS");
        t.setProperty ("bars", e->bars, nullptr);
        juce::StringArray rows;
        for (const auto& ev : e->events)
            rows.add (juce::String (ev.start, 5) + "," + juce::String (ev.length, 5) + "," + juce::String (ev.note) + ","
                      + juce::String (ev.velocity) + "," + juce::String (ev.layer) + "," + (ev.accent ? "1" : "0"));
        t.setProperty ("notes", rows.joinIntoString (";"), nullptr);
        state.appendChild (t, nullptr);
    }
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ReggaeOrganProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            auto t = tree.getChildWithName ("EDITS");
            clearEdits();
            if (t.isValid())
            {
                std::vector<Event> ev;
                for (auto& row : juce::StringArray::fromTokens (t.getProperty ("notes").toString(), ";", ""))
                {
                    auto f = juce::StringArray::fromTokens (row, ",", "");
                    if (f.size() < 6) continue;
                    Event e;
                    e.start = f[0].getDoubleValue(); e.length = f[1].getDoubleValue(); e.note = f[2].getIntValue();
                    e.velocity = f[3].getIntValue(); e.layer = juce::jlimit (0, 2, f[4].getIntValue()); e.accent = f[5] == "1";
                    ev.push_back (e);
                }
                if (! ev.empty()) setEdits (ev, (int) t.getProperty ("bars", 1));
                tree.removeChild (t, nullptr);
            }
            // projetos da 1.0: a Leslie antiga (desligada / lenta / rápida) vira o preset 122 CLÁSSICA
            juce::ValueTree oldLes, newLes;
            for (auto c : tree)
                if (c.hasType ("PARAM"))
                {
                    if (c.getProperty ("id").toString() == "leslie")    oldLes = c;
                    if (c.getProperty ("id").toString() == "lesPreset") newLes = c;
                }
            bool hadColorBy = false;
            for (const auto& c : tree) hadColorBy |= c.getProperty ("id").toString() == "colorBy";
            apvts.replaceState (tree);
            if (! hadColorBy)   // projeto de antes das cores por acorde: continua com as cores/canais por camada
                if (auto* prm = apvts.getParameter ("colorBy")) prm->setValueNotifyingHost (1.0f);
            if (oldLes.isValid() && ! newLes.isValid())
            {
                const int v = juce::roundToInt ((double) oldLes.getProperty ("value", 0));
                const int lp = leslieFromMusician (v);
                setParam ("lesPreset", (float) lp);
                setParam ("lesSpeed", v == 2 ? 1.0f : 0.0f);
                setParam ("lesDepth", lp > 0 ? lesliePreset (lp).depth : 0.8f);
            }
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReggaeOrganProcessor();
}
