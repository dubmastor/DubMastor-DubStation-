#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
// Presets de fábrica — drawbars na ordem 16' 5⅓' 8' 4' 2⅔' 2' 1⅗' 1⅓' 1'
const Preset kPresets[] = {
    // ---------------- PIANO (só mexe nos parâmetros do piano)
    //  nome                         modo  brilho decay chorus nívelP   (campos do órgão ignorados)                                                 reverb
    { "Piano: M1 Classic",              0, 0.65f, 0.55f, 0.50f, 0.80f, { 0,0,0,0,0,0,0,0,0 }, false, true, true, 0, 0, 0, 0, false, 0.20f },
    { "Piano: M1 Dub Skank",            0, 0.85f, 0.20f, 0.65f, 0.85f, { 0,0,0,0,0,0,0,0,0 }, false, true, true, 0, 0, 0, 0, false, 0.35f },
    { "Piano: M1 Bright House",         0, 1.00f, 0.45f, 0.55f, 0.80f, { 0,0,0,0,0,0,0,0,0 }, false, true, true, 0, 0, 0, 0, false, 0.25f },
    { "Piano: M1 Soft Ballad",          0, 0.35f, 0.80f, 0.40f, 0.85f, { 0,0,0,0,0,0,0,0,0 }, false, true, true, 0, 0, 0, 0, false, 0.30f },
    // ---------------- ORGAN (só mexe nos parâmetros do órgão)
    //  nome                         modo  (campos do piano ignorados)   drawbars                     perc  3ª    fast  click  drive  lesMix nívelO lesFast reverb
    { "Organ: Reggae Bubble",           1, 0, 0, 0, 0,                  { 8,8,8,0,0,0,0,0,0 }, false, true, true, 0.40f, 0.25f, 0.85f, 0.75f, false, 0.18f },
    { "Organ: Roots Percussion",        1, 0, 0, 0, 0,                  { 8,8,8,0,0,0,0,0,0 }, true,  true, true, 0.50f, 0.30f, 0.80f, 0.75f, false, 0.18f },
    { "Organ: Dub Dark",                1, 0, 0, 0, 0,                  { 8,0,8,0,0,0,0,0,0 }, false, true, true, 0.25f, 0.15f, 0.90f, 0.80f, false, 0.40f },
    { "Organ: Gospel Leslie Fast",      1, 0, 0, 0, 0,                  { 8,8,8,8,0,0,0,0,8 }, false, true, true, 0.45f, 0.40f, 0.90f, 0.65f, true,  0.25f },
};
constexpr int kNumPianoPresets = 4;
constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
} // namespace

const juce::StringArray& RootsKeysProcessor::drawbarNames()
{
    static const juce::StringArray names { "16'", "5 1/3'", "8'", "4'", "2 2/3'", "2'", "1 3/5'", "1 1/3'", "1'" };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout RootsKeysProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (const char* id, const char* name, float def)
    { return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def); };

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode", StringArray { "Piano", "Organ" }, 0));

    l.add (pct ("pianoBright", "Piano Brightness", 0.65f));
    l.add (pct ("pianoDecay",  "Piano Decay",      0.55f));
    l.add (pct ("pianoChorus", "Piano Chorus",     0.50f));
    l.add (pct ("pianoLevel",  "Piano Level",      0.80f));

    const int defDb[9] = { 8, 8, 8, 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < 9; ++i)
        l.add (std::make_unique<AudioParameterInt> (ParameterID { "db" + String (i + 1), 1 },
                                                    "Drawbar " + drawbarNames()[i], 0, 8, defDb[i]));

    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "percOn", 1 },   "Percussion", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "percHarm", 1 }, "Perc Harmonic", StringArray { "2nd", "3rd" }, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "percDecay", 1 },"Perc Decay", StringArray { "Fast", "Slow" }, 0));
    l.add (pct ("click",      "Key Click",    0.35f));
    l.add (pct ("drive",      "Overdrive",    0.20f));
    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "leslieFast", 1 }, "Leslie Fast", false));
    l.add (pct ("leslieMix",  "Leslie Mix",   0.85f));
    l.add (pct ("organLevel", "Organ Level",  0.70f));

    // Tape Echo — FitaEco TE-12 (mesmo DSP do Dub Station 2.1)
    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "echoOn", 1 }, "Echo On", false));
    {
        StringArray modes;
        for (int i = 1; i <= 12; ++i) modes.add ("Mode " + String (i));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { "echoMode", 1 }, "Echo Mode", modes, 6));
    }
    l.add (pct ("echoRate",      "Echo Repeat Rate", 0.50f));
    l.add (pct ("echoIntensity", "Echo Intensity",   0.45f));
    l.add (pct ("echoVol",       "Echo Volume",      0.60f));
    l.add (pct ("echoSpring",    "Echo Spring",      0.30f));
    l.add (pct ("echoWow",       "Echo Wow/Flutter", 0.35f));
    l.add (pct ("echoAge",       "Echo Tape Age",    0.30f));

    // Flanger estilo dub (Mad Professor)
    l.add (std::make_unique<AudioParameterBool>  (ParameterID { "flOn", 1 }, "Flanger On", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "flRate", 1 }, "Flanger Rate",
                                                  NormalisableRange<float> (0.02f, 4.0f, 0.0f, 0.35f), 0.12f,
                                                  AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (pct ("flDepth",  "Flanger Depth",  0.80f));
    l.add (pct ("flManual", "Flanger Manual", 0.30f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "flFeedback", 1 }, "Flanger Feedback",
                                                  NormalisableRange<float> (-0.95f, 0.95f), 0.65f));
    l.add (pct ("flMix",    "Flanger Mix",    0.50f));

    // ---------------- Dub Chords (aba de harmonia)
    {
        StringArray roots, quals, durs, pats;
        for (int r = 0; r < 12; ++r) roots.add (dub::rootName (r));
        quals = { "Maior", "menor", "7", "m7", "maj7", "m6", "sus2", "sus4", "dim", "m9" };
        for (int d = 0; d < dub::numDurations; ++d) durs.add (dub::durationName (d));
        pats.add ("Auto (segue o instrumento)");
        for (int pt = 0; pt < dub::NumPatterns; ++pt) pats.add (dub::patternName (pt));
        const int defRoot[4] = { 9, 2, 7, 0 }, defQual[4] = { 1, 1, 0, 0 };
        l.add (std::make_unique<AudioParameterBool>  (ParameterID { "dcOn", 1 }, "Dub Chords: tocar com a DAW", false));
        l.add (std::make_unique<AudioParameterBool>  (ParameterID { "dcPreview", 1 }, "Dub Chords: Preview", false));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { "dcBpm", 1 }, "Dub Chords: BPM do preview",
                                                      NormalisableRange<float> (60.0f, 180.0f, 1.0f), 75.0f));
        for (int i = 0; i < dcSlots; ++i)
        {
            const auto n = String (i), lbl = "Acorde " + String (i + 1) + " ";
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { "dcRoot" + n, 1 }, lbl + "tonica", roots, defRoot[i]));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { "dcQual" + n, 1 }, lbl + "tipo", quals, defQual[i]));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { "dcDur" + n, 1 }, lbl + "duracao", durs, 2));
            if (i > 0)
                l.add (std::make_unique<AudioParameterBool> (ParameterID { "dcAct" + n, 1 }, lbl + "ligado", i == 1));
        }
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { "dcPattern", 1 }, "Dub Chords: padrao", pats, 0));
        l.add (std::make_unique<AudioParameterInt>   (ParameterID { "dcRegister", 1 }, "Dub Chords: registro (mao esquerda)", 48, 72, 60));
        l.add (pct ("dcVel",   "Dub Chords: velocity", 0.80f));
        StringArray sizes;
        for (int sz = 0; sz < dub::numNoteSizes; ++sz) sizes.add (dub::noteSizeName (sz));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { "dcNoteSize", 1 }, "Dub Chords: tamanho da nota", sizes, 0));
        l.add (pct ("dcSwing", "Dub Chords: swing", 0.0f));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { "dcReps", 1 }, "Dub Chords: repetir no MIDI",
                                                       StringArray { "x1", "x2", "x4", "x8" }, 1));
    }

    l.add (pct ("reverb", "Reverb", 0.20f));
    l.add (pct ("master", "Master", 0.80f));

    // Maximizer
    l.add (std::make_unique<AudioParameterBool>  (ParameterID { "maxOn", 1 }, "Maximizer On", true));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "maxGain", 1 }, "Maximizer Gain",
                                                  NormalisableRange<float> (0.0f, 18.0f, 0.1f), 3.0f,
                                                  AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "maxCeiling", 1 }, "Maximizer Ceiling",
                                                  NormalisableRange<float> (-12.0f, 0.0f, 0.1f), -0.3f,
                                                  AudioParameterFloatAttributes().withLabel ("dB")));
    return l;
}

RootsKeysProcessor::RootsKeysProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < 9; ++i)
        dbParam[i] = apvts.getRawParameterValue ("db" + juce::String (i + 1));
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (rp->getParameterID().startsWith ("dc") || rp->getParameterID() == "mode")
                apvts.addParameterListener (rp->getParameterID(), this);
}

RootsKeysProcessor::~RootsKeysProcessor()
{
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            apvts.removeParameterListener (rp->getParameterID(), this);
}

bool RootsKeysProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void RootsKeysProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateHz = sampleRate;
    dcWasPlaying = false;
    dcInternalPpq = 0.0;
    std::fill (std::begin (dcActive), std::end (dcActive), false);
    dcPending.reserve (512);
    dcDirty.store (true);
    engine.prepare (sampleRate);
    setLatencySamples (engine.latencySamples());   // lookahead do maximizer
    keyboardState.reset();
}

void RootsKeysProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
    if (buffer.getNumChannels() < 2) { buffer.clear(); return; }

    // parâmetros -> motor
    rk::Params& p = engine.params;
    p.mode        = (int) get ("mode");
    p.pianoBright = get ("pianoBright");
    p.pianoDecay  = get ("pianoDecay");
    p.pianoChorus = get ("pianoChorus");
    p.pianoLevel  = get ("pianoLevel");
    for (int i = 0; i < 9; ++i)
        p.drawbar[i] = dbParam[i]->load();
    p.percOn     = get ("percOn") > 0.5f;
    p.perc3rd    = get ("percHarm") > 0.5f;
    p.percFast   = get ("percDecay") < 0.5f;
    p.click      = get ("click");
    p.drive      = get ("drive");
    p.leslieFast = get ("leslieFast") > 0.5f;
    p.leslieMix  = get ("leslieMix");
    p.organLevel = get ("organLevel");
    p.reverb     = get ("reverb");
    p.master     = get ("master");
    p.echoOn        = get ("echoOn") > 0.5f;
    p.echoMode      = (int) get ("echoMode") + 1;
    p.echoRate      = get ("echoRate");
    p.echoIntensity = get ("echoIntensity");
    p.echoVol       = get ("echoVol");
    p.echoSpring    = get ("echoSpring");
    p.echoWow       = get ("echoWow");
    p.echoAge       = get ("echoAge");
    p.flOn          = get ("flOn") > 0.5f;
    p.flRate        = get ("flRate");
    p.flDepth       = get ("flDepth");
    p.flManual      = get ("flManual");
    p.flFeedback    = get ("flFeedback");
    p.flMix         = get ("flMix");
    p.maxOn         = get ("maxOn") > 0.5f;
    p.maxGainDb     = get ("maxGain");
    p.maxCeilDb     = get ("maxCeiling");

    // aba DUB CHORDS: gera o skank/bubble sincronizado com a DAW e toca no próprio instrumento
    runDubChords (midi, n);

    // junta as notas do teclado da tela e atualiza as teclas pintadas
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    int pos = 0;

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int t = juce::jlimit (0, n, meta.samplePosition);
        if (t > pos) { engine.render (L + pos, R + pos, t - pos); pos = t; }

        if (m.isNoteOn())                       engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())                 engine.noteOff (m.getNoteNumber());
        else if (m.isSustainPedalOn())          engine.setSustain (true);
        else if (m.isSustainPedalOff())         engine.setSustain (false);
        else if (m.isController() && m.getControllerNumber() == 1)
        {
            engine.setModWheelLeslie (m.getControllerValue() >= 64);   // mod wheel = Leslie rápido
            modWheel.store ((float) m.getControllerValue() / 127.0f);
        }
        else if (m.isPitchWheel())
            pitchWheel.store ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            engine.allNotesOff();
    }
    if (pos < n) engine.render (L + pos, R + pos, n - pos);

    // medidores (média quadrática do bloco) para o VU
    float sl = 0, sr = 0;
    for (int i = 0; i < n; ++i) { sl += L[i] * L[i]; sr += R[i] * R[i]; }
    if (n > 0) { meterL.store (sl / (float) n); meterR.store (sr / (float) n); }
    gainReduction.store (engine.gainReductionDb());
}

// =====================================================================  DUB CHORDS
dub::ChordSlot RootsKeysProcessor::getDcSlot (int i) const
{
    const auto n = juce::String (i);
    dub::ChordSlot s;
    s.root    = (int) apvts.getRawParameterValue ("dcRoot" + n)->load();
    s.quality = (int) apvts.getRawParameterValue ("dcQual" + n)->load();
    s.beats   = dub::durationBeats ((int) apvts.getRawParameterValue ("dcDur" + n)->load());
    return s;
}

bool RootsKeysProcessor::isDcSlotOn (int i) const
{
    return i == 0 || apvts.getRawParameterValue ("dcAct" + juce::String (i))->load() > 0.5f;
}

std::vector<dub::ChordSlot> RootsKeysProcessor::getDcSlots() const
{
    std::vector<dub::ChordSlot> v;
    for (int i = 0; i < dcSlots; ++i)
        if (isDcSlotOn (i)) v.push_back (getDcSlot (i));
    return v;
}

int RootsKeysProcessor::effectiveDcPattern() const
{
    const int pat = (int) get ("dcPattern");
    if (pat > 0) return pat - 1;
    return (int) get ("mode") == 1 ? dub::Bubble : dub::SkankOffbeat;
}

dub::Settings RootsKeysProcessor::getDcSettings() const
{
    dub::Settings s;
    s.pattern    = effectiveDcPattern();
    s.centerNote = (int) get ("dcRegister");
    s.velocity   = get ("dcVel");
    s.noteSize   = (int) get ("dcNoteSize");
    s.swing      = get ("dcSwing");
    return s;
}

double RootsKeysProcessor::getDcExportBpm() const
{
    const double h = dcHostBpm.load();
    return h > 0.0 ? h : (double) get ("dcBpm");
}

void RootsKeysProcessor::applyRiddim (int index)
{
    const auto& presets = dub::riddimPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    const auto& r = presets[(size_t) index];
    auto durIndex = [] (double beats)
    {
        int best = 0;
        for (int d = 0; d < dub::numDurations; ++d)
            if (std::abs (dub::durationBeats (d) - beats) < std::abs (dub::durationBeats (best) - beats)) best = d;
        return best;
    };
    for (int i = 0; i < dcSlots; ++i)
    {
        const auto n = juce::String (i);
        if (i < r.numChords)
        {
            setParam ("dcRoot" + n, (float) r.chords[i].root);
            setParam ("dcQual" + n, (float) r.chords[i].quality);
            setParam ("dcDur" + n, (float) durIndex (r.chords[i].beats));
        }
        if (i > 0) setParam ("dcAct" + n, i < r.numChords ? 1.0f : 0.0f);
    }
}

void RootsKeysProcessor::dcAllNotesOff (juce::MidiBuffer& midi, int sample)
{
    for (int nn = 0; nn < 128; ++nn)
        if (dcActive[nn])
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, nn), sample);
            dcActive[nn] = false;
        }
}

void RootsKeysProcessor::runDubChords (juce::MidiBuffer& midi, int numSamples)
{
    double bpm = 0.0, ppq = 0.0;
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())         bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            hostPlaying = pos->getIsPlaying();
        }
    if (bpm > 0.0) dcHostBpm.store (bpm);
    dcHostPlaying.store (hostPlaying);

    const bool enabled = get ("dcOn") > 0.5f;
    const bool preview = get ("dcPreview") > 0.5f;
    bool playing = enabled && hostPlaying;
    if (! hostPlaying && preview)
    {
        bpm = (double) get ("dcBpm");
        ppq = dcInternalPpq;
        playing = true;
    }
    if (! preview || hostPlaying) dcInternalPpq = 0.0;
    if (bpm <= 0.0) bpm = 120.0;

    if (dcDirty.exchange (false))
        dcLoop = dub::buildLoop (getDcSlots(), getDcSettings(), dcLoopBeats);

    const double blockBeats = (double) numSamples / sampleRateHz * bpm / 60.0;
    if (! playing || dcLoopBeats <= 0.0)
    {
        if (dcWasPlaying) dcAllNotesOff (midi, 0);
        dcWasPlaying = false;
        dcPlayPos.store (-1.0);
        return;
    }

    // salto no transporte (loop da DAW, reposicionamento): solta as notas presas
    if (dcWasPlaying && std::abs (ppq - dcExpectedPpq) > 1.0e-3)
        dcAllNotesOff (midi, 0);

    const double end = ppq + blockBeats;
    dcPending.clear();
    auto collect = [&] (double t, bool on, const dub::NoteEvent& e)
    {
        double occ = t + std::ceil ((ppq - t) / dcLoopBeats) * dcLoopBeats;
        while (occ < end)
        {
            if (occ >= ppq)
                dcPending.push_back ({ juce::jlimit (0, numSamples - 1, (int) ((occ - ppq) / blockBeats * numSamples)), on, e.note, e.velocity });
            occ += dcLoopBeats;
        }
    };
    for (const auto& e : dcLoop)
        if (e.channel == 1)
        {
            collect (e.start, true, e);
            collect (e.start + e.length, false, e);
        }
    std::sort (dcPending.begin(), dcPending.end(), [] (const DcPending& a, const DcPending& b)
    {
        if (a.sample != b.sample) return a.sample < b.sample;
        return (int) a.on < (int) b.on;   // note-off antes de note-on
    });
    for (const auto& pe : dcPending)
    {
        bool& isOn = dcActive[pe.note];
        if (pe.on)
        {
            if (isOn) midi.addEvent (juce::MidiMessage::noteOff (1, pe.note), pe.sample);
            midi.addEvent (juce::MidiMessage::noteOn (1, pe.note, (juce::uint8) pe.vel), pe.sample);
            isOn = true;
        }
        else if (isOn)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, pe.note), pe.sample);
            isOn = false;
        }
    }
    dcWasPlaying = true;
    dcExpectedPpq = end;
    if (! hostPlaying) dcInternalPpq = end;
    dcPlayPos.store (std::fmod (std::max (0.0, ppq), dcLoopBeats));
}

bool RootsKeysProcessor::writeDcMidiFile (const juce::File& file, int slot) const
{
    std::vector<dub::ChordSlot> slots;
    if (slot >= 0) slots.push_back (getDcSlot (slot));
    else           slots = getDcSlots();
    double lb = 0.0;
    const auto events = dub::buildLoop (slots, getDcSettings(), lb);
    if (lb <= 0.0 || events.empty()) return false;

    static const int repsTable[4] = { 1, 2, 4, 8 };
    const int reps = repsTable[juce::jlimit (0, 3, (int) get ("dcReps"))];
    const int tpq = 960;
    const double bpm = getDcExportBpm();

    juce::MidiMessageSequence meta, chords;
    meta.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)), 0.0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    chords.addEvent (juce::MidiMessage::textMetaEvent (3, "Roots Keys - Skank"), 0.0);

    for (int r = 0; r < reps; ++r)
        for (const auto& e : events)
        {
            auto& seq = chords;
            seq.addEvent (juce::MidiMessage::noteOn (e.channel, e.note, (juce::uint8) e.velocity), (e.start + r * lb) * tpq);
            seq.addEvent (juce::MidiMessage::noteOff (e.channel, e.note), (e.start + e.length + r * lb) * tpq);
        }
    chords.sort(); chords.updateMatchedPairs();

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (meta);
    if (chords.getNumEvents() > 1) mf.addTrack (chords);

    file.getParentDirectory().createDirectory();
    file.deleteFile();
    juce::FileOutputStream out (file);
    return out.openedOk() && mf.writeTo (out);
}

juce::AudioProcessorEditor* RootsKeysProcessor::createEditor()
{
    return new RootsKeysEditor (*this);
}

// ---------------------------------------------------------------- presets
int RootsKeysProcessor::getNumPrograms() { return kNumPresets; }

const juce::String RootsKeysProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, kNumPresets) ? juce::String (kPresets[index].name) : juce::String();
}

void RootsKeysProcessor::setParam (const juce::String& id, float value)
{
    if (auto* prm = apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
}

void RootsKeysProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPresets)) return;
    currentProgram = index;
    const Preset& pr = kPresets[index];
    setParam ("mode", (float) pr.mode);

    if (pr.mode == 0)   // preset de piano: não toca nos parâmetros do órgão
    {
        setParam ("pianoBright", pr.pianoBright);
        setParam ("pianoDecay", pr.pianoDecay);
        setParam ("pianoChorus", pr.pianoChorus);
        setParam ("pianoLevel", pr.pianoLevel);
    }
    else                // preset de órgão: não toca nos parâmetros do piano
    {
        for (int i = 0; i < 9; ++i) setParam ("db" + juce::String (i + 1), (float) pr.drawbars[i]);
        setParam ("percOn", pr.percOn ? 1.0f : 0.0f);
        setParam ("percHarm", pr.perc3rd ? 1.0f : 0.0f);
        setParam ("percDecay", pr.percFast ? 0.0f : 1.0f);
        setParam ("click", pr.click);
        setParam ("drive", pr.drive);
        setParam ("leslieMix", pr.leslieMix);
        setParam ("organLevel", pr.organLevel);
        setParam ("leslieFast", pr.leslieFast ? 1.0f : 0.0f);
    }
    setParam ("reverb", pr.reverb);
}

int RootsKeysProcessor::getNumPianoPresets() { return kNumPianoPresets; }

// ---------------------------------------------------------------- estado
void RootsKeysProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void RootsKeysProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RootsKeysProcessor();
}
