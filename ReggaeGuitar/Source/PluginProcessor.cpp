#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace mrg;

juce::AudioProcessorValueTreeState::ParameterLayout ReggaeGuitarProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (const String& id, const String& name, float def)
    { return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def); };
    auto onOff = [] (const String& id, const String& name, bool def)
    { return std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def); };
    auto choice = [] (const String& id, const String& name, StringArray items, int def)
    { return std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def); };
    auto names = [] (const char* (*fn) (int), int n) { StringArray a; for (int i = 0; i < n; ++i) a.add (String::fromUTF8 (fn (i))); return a; };

    const Player& def = player (5);   // Chinna Smith

    // ---- transporte (BPM na contagem com a caixa no 2 e no 4)
    l.add (onOff ("groove", "Groove ligado", true));
    l.add (onOff ("play", "Play (sem a DAW)", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM (sem a DAW)", NormalisableRange<float> (55.0f, 100.0f, 1.0f), (float) def.tracks[0].bpm));
    l.add (onOff ("band", "Acompanhamento (aro, bumbo e baixo)", false));

    { StringArray p; for (int i = 0; i < NumPlayers; ++i) p.add (player (i).name); l.add (choice ("player", "Guitarrista", p, 5)); }

    // ---- grade principal e golpe
    for (int i = 0; i < 16; ++i)
        l.add (choice (stepId (i), "Passo " + String (i + 1), { "-", "skank", "chop" }, (i % 4 == 2) ? 1 : 0));
    l.add (choice ("stroke", "Golpe", names (strokeName, NumStrokes), def.stroke));
    l.add (choice ("mute", "Abafamento", names (muteName, NumMutes), def.mute));
    l.add (pct ("lag", "Atraso (laid back)", 0.45f));
    l.add (pct ("strum", "Strum", 0.35f));
    l.add (pct ("dur", "Duracao", 0.3f));

    // ---- variação
    l.add (onOff ("varOn", "Variacao ligada", true));
    l.add (choice ("varType", "Variacao", names (varName, NumVarTypes), def.var));
    l.add (choice ("varEvery", "Variacao a cada", { "2 compassos", "4 compassos", "8 compassos" }, 1));
    l.add (choice ("varStart", "Variacao comeca no tempo", { "1", "2", "3", "4" }, varPreset (def.var).start - 1));
    l.add (pct ("varInt", "Variacao intensidade", 0.8f));
    for (int i = 0; i < 16; ++i)
        l.add (choice (fillId (i), "Variacao passo " + String (i + 1), { "-", "skank", "bang-bang", "longo", "chop", "nota" }, varPreset (def.var).s[i]));

    // ---- acordes
    l.add (choice ("chordSrc", "Acordes", { "Do plugin", "Do teclado (MIDI)" }, 0));
    {
        StringArray roots { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int defRoot[numSlots] = { 9, 9, 2, 4 };
        for (int i = 0; i < numSlots; ++i)
        {
            const String n (i + 1);
            l.add (choice ("root" + n, "Acorde " + n + " tonica", roots, defRoot[i]));
            l.add (onOff ("minor" + n, "Acorde " + n + " menor", true));
        }
    }
    l.add (choice ("art", "Articulacao (keyswitch)", names (artName, NumArts), 0));

    // ---- guitarra e ampli
    l.add (choice ("guitar", "Guitarra", names (guitarName, NumGuitars), def.guitar));
    l.add (choice ("pickup", "Captador", names (pickupName, NumPickups), def.pickup));
    l.add (pct ("gvol", "Volume da guitarra", 0.8f));
    l.add (pct ("tone", "Tone", 0.6f));
    l.add (choice ("amp", "Ampli", names (ampName, NumAmps), def.amp));
    l.add (pct ("treble", "Treble", 0.4f));
    l.add (pct ("middle", "Middle", 1.0f));
    l.add (pct ("bass", "Bass", 0.6f));
    l.add (pct ("reverb", "Reverb", 0.3f));

    // ---- efeitos (padrão de fábrica: Tape Echo e saturador em bypass)
    l.add (onOff ("wahOn", "Wah", false));            l.add (pct ("wah", "Wah posicao", 0.45f));
    l.add (onOff ("phOn", "Phaser", false));          l.add (pct ("ph", "Phaser velocidade", 0.4f));
    l.add (onOff ("chOn", "Chorus", def.chorus));     l.add (pct ("ch", "Chorus profundidade", 0.5f));
    l.add (onOff ("molaOn", "Mola", true));           l.add (pct ("mola", "Mola volume", 0.35f));
    l.add (onOff ("echoOn", "Tape Echo", false));     l.add (pct ("echo", "Tape Echo repeticoes", 0.35f));
    l.add (onOff ("satOn", "Saturador", false));      l.add (pct ("sat", "Saturador drive", 0.4f));
    l.add (pct ("out", "Saida", 0.7f));
    return l;
}

ReggaeGuitarProcessor::ReggaeGuitarProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MRG68", createLayout()), voices ((size_t) numVoices)
{
}

bool ReggaeGuitarProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ReggaeGuitarProcessor::setParam (const juce::String& id, float value)
{
    if (auto* prm = apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
        prm->endChangeGesture();
    }
}

void ReggaeGuitarProcessor::applyPlayer (int index)
{
    const Player& p = player (index);
    setParam ("player", (float) juce::jlimit (0, NumPlayers - 1, index));
    setParam ("guitar", (float) p.guitar);
    setParam ("pickup", (float) p.pickup);
    setParam ("amp", (float) p.amp);
    setParam ("stroke", (float) p.stroke);
    setParam ("mute", (float) p.mute);
    for (int i = 0; i < 16; ++i)
        setParam (stepId (i), (i % 4 == 2) ? 1.0f : ((p.chop && (i == 7 || i == 15)) ? 2.0f : 0.0f));
    setParam ("wahOn", p.wah ? 1.0f : 0.0f);
    setParam ("phOn", p.phaser ? 1.0f : 0.0f);
    setParam ("chOn", p.chorus ? 1.0f : 0.0f);
    setParam ("bpm", (float) p.tracks[0].bpm);
    trackIndex.store (0);
    loadVariation (p.var);
}

void ReggaeGuitarProcessor::setTrack (int t)
{
    const Player& p = player ((int) get ("player"));
    const int i = ((t % p.numTracks) + p.numTracks) % p.numTracks;
    trackIndex.store (i);
    setParam ("bpm", (float) p.tracks[i].bpm);
}

void ReggaeGuitarProcessor::loadVariation (int type)
{
    setParam ("varType", (float) type);
    if (type < 0 || type >= VarEditada) return;
    const VarPreset& v = varPreset (type);
    setParam ("varStart", (float) (v.start - 1));
    for (int i = 0; i < 16; ++i) setParam (fillId (i), (float) v.s[i]);
}

void ReggaeGuitarProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    chain.prepare (sr);
    band.sr = sr;
    busL.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    busR.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    for (auto& v : voices) v.active = false;
    lastStep = -1; lastPpq = -1.0; internalPpq = 0.0;
}

mrg::GrooveSettings ReggaeGuitarProcessor::grooveSettings() const
{
    GrooveSettings s;
    for (int i = 0; i < 16; ++i) { s.steps[i] = (int) get (stepId (i)); s.fill[i] = (int) get (fillId (i)); }
    s.stroke = (int) get ("stroke");
    s.mute = (int) get ("mute");
    s.lag = get ("lag"); s.strum = get ("strum"); s.dur = get ("dur");
    s.vary = get ("varOn") > 0.5f;
    const int ev = (int) get ("varEvery");
    s.every = ev == 0 ? 2 : (ev == 1 ? 4 : 8);
    s.start = (int) get ("varStart") + 1;
    s.intensity = get ("varInt");
    s.band = get ("band") > 0.5f;
    return s;
}

mrg::SoundParams ReggaeGuitarProcessor::soundParams (double bpm) const
{
    SoundParams p;
    p.guitar = (int) get ("guitar"); p.pickup = (int) get ("pickup"); p.amp = (int) get ("amp");
    p.gvol = get ("gvol"); p.tone = get ("tone");
    p.treble = get ("treble"); p.middle = get ("middle"); p.bass = get ("bass"); p.reverb = get ("reverb");
    p.wahOn = get ("wahOn") > 0.5f; p.wah = get ("wah");
    p.phOn = get ("phOn") > 0.5f; p.ph = get ("ph");
    p.chOn = get ("chOn") > 0.5f; p.ch = get ("ch");
    p.molaOn = get ("molaOn") > 0.5f; p.mola = get ("mola");
    p.echoOn = get ("echoOn") > 0.5f; p.echo = get ("echo");
    p.satOn = get ("satOn") > 0.5f; p.sat = get ("sat");
    p.out = get ("out");
    p.bpm = bpm;
    return p;
}

std::array<int, 4> ReggaeGuitarProcessor::chordForBar (long long bar) const
{
    if ((int) get ("chordSrc") == 1 && haveKeysChord) return keysChord;
    const juce::String n ((int) (bar % numSlots) + 1);
    return chordVoicing ((int) get ("root" + n), get ("minor" + n) > 0.5f);
}

std::array<int, 4> ReggaeGuitarProcessor::chordFromNote (int note) const
{
    // tecla da zona de acordes: a tônica; o tipo vem do campo harmônico de Lá menor (C, Dm, Em, F, G, Am; B vira Em)
    const int pc = note % 12;
    bool minor = false; int root = pc;
    switch (pc)
    {
        case 9: case 2: case 4: minor = true; break;
        case 11: root = 4; minor = true; break;
        default: break;
    }
    return chordVoicing (root, minor);
}

mrg::StringVoice& ReggaeGuitarProcessor::freeVoice()
{
    StringVoice* best = &voices[0];
    for (auto& v : voices)
    {
        if (! v.active) return v;
        if (! v.held && v.age > best->age) best = &v;
    }
    return *best;
}

void ReggaeGuitarProcessor::strum (const std::array<int, 4>& chord, float vel, int offset, double holdSec, int mute,
                                   bool up, bool top3, double spread, double dbl)
{
    std::vector<int> notes (chord.begin(), chord.end());
    std::sort (notes.begin(), notes.end());
    if (top3) notes.erase (notes.begin());
    if (up) std::reverse (notes.begin(), notes.end());
    const double sp = (spread >= 0 ? spread : 0.002 + get ("strum") * 0.008) * (1.2 - 0.4 * vel);
    const double pickupPos = pickupModel ((int) get ("pickup")).pos;
    const float dark = mute == MuPalm ? 1.0f : 0.0f;
    const double ring = mute == MuAberto ? 0.998 : 0.9965, muteG = mute == MuMaoEsq ? 0.7 : 0.8;
    for (int take = 0; take < 2; ++take)      // duas guitarras abertas em estéreo
    {
        const int takeOff = offset + (take ? (int) (dbl * sr) : 0);
        const float pan = take ? 0.35f : -0.35f, tv = take ? vel * 0.96f : vel;
        for (size_t i = 0; i < notes.size(); ++i)
        {
            const float v = tv * (1.0f - 0.09f * (float) i) * (0.92f + 0.08f * rnd.nextFloat());
            const double hold = std::max (0.008, holdSec - (double) i * sp * 0.5);
            auto& voice = freeVoice();
            voice.start (sr, notes[i], v, (int) (hold * sr), takeOff + (int) ((double) i * sp * sr),
                         0.12 + 0.03 * rnd.nextFloat(), pickupPos, dark, ring, muteG, pan, ++seed, -1, false);
        }
    }
}

void ReggaeGuitarProcessor::playLead (int note, float vel, int offset, double holdSec, bool sustain, int mute, float pan)
{
    auto& voice = freeVoice();
    voice.start (sr, note, vel, (int) (holdSec * sr), offset, 0.15, pickupModel ((int) get ("pickup")).pos,
                 mute == MuPalm ? 1.0f : 0.0f, 0.998, 0.8, pan, ++seed, note, sustain);
}

void ReggaeGuitarProcessor::doAction (const Action& a, const std::array<int, 4>& chord, int sampleOffset, double bpm)
{
    const double spb = 60.0 / bpm;
    const int off = sampleOffset + (int) (a.offsetBeats * spb * sr);
    switch (a.kind)
    {
        case Action::Strum:    strum (chord, a.vel, off, a.holdSec, a.mute, a.up, a.top3, a.spread, a.dbl); break;
        case Action::Lead:     playLead (a.note, a.vel, off, a.holdSec, false, MuPalm); break;
        case Action::Kick:     band.hit (0, 1.0f, off); break;
        case Action::Rim:      band.hit (1, 1.0f, off); break;
        case Action::Hat:      band.hit (2, a.vel, off); break;
        case Action::BassNote: band.note (chord[0] - 12 + a.note, a.beats * 1.0, spb, off); break;
    }
}

void ReggaeGuitarProcessor::handleMidi (const juce::MidiMessage& m, int offset, bool grooveRunning)
{
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();
        const float v = m.getFloatVelocity();
        if (n < 36)
        {   // keyswitch: articulação
            setParam ("art", (float) (n % NumArts));
            return;
        }
        if (n < 60)
        {   // zona de acordes: skank
            const auto ch = chordFromNote (n);
            if (grooveRunning && (int) get ("chordSrc") == 1) { keysChord = ch; haveKeysChord = true; return; }
            keysChord = ch; haveKeysChord = true;
            if (v < 0.45f) { strum (ch, 0.5f, offset, 0.014, MuMaoEsq, false, false, -1, 0.012); return; }
            if ((int) get ("stroke") == StBangBang)
            {
                std::vector<Action> acts;
                bangBang (acts, v, 0.0, 60.0 / juce::jlimit (55.0, 100.0, (double) get ("bpm")));
                for (auto& a : acts) doAction (a, ch, offset, juce::jlimit (55.0, 100.0, (double) get ("bpm")));
                return;
            }
            const int art = (int) get ("art");
            const int mute = art == ArtMaoEsq ? MuMaoEsq : art == ArtPalm || art == ArtTremolo ? MuPalm
                           : art == ArtSustentada ? MuAberto : (int) get ("mute");
            strum (ch, v, offset, holdFor (mute, get ("dur")), mute, false, false, -1, 0.012);
            return;
        }
        // zona de notas: picking e lead
        const int art = (int) get ("art");
        if (art == ArtTremolo)
        {
            const double spb = 60.0 / juce::jlimit (55.0, 100.0, (double) get ("bpm"));
            for (int k = 0; k < 3; ++k) playLead (n, v * (1.0f - 0.15f * (float) k), offset + (int) (k * spb / 8.0 * sr), 0.05, false, MuPalm, 0.0f);
            return;
        }
        const bool sustain = art == ArtSustentada || (int) get ("mute") == MuAberto;
        playLead (n, v, offset, holdFor (art == ArtPalm ? MuPalm : (int) get ("mute"), get ("dur")), sustain,
                  art == ArtPalm ? MuPalm : MuSeco, 0.0f);
    }
    else if (m.isNoteOff())
    {
        for (auto& v : voices) if (v.active && v.held && v.tag == m.getNoteNumber()) v.release();
    }
    else if (m.isPitchWheel())
    {
        bend = (m.getPitchWheelValue() - 8192) / 8192.0 * 2.0;   // ±2 semitons
    }
    else if (m.isController() && m.getControllerNumber() == 1)
    {
        vibDepth = m.getControllerValue() / 127.0 * 0.35;        // até 35 cents
    }
}

void ReggaeGuitarProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    if ((int) busL.size() < n) { busL.resize ((size_t) n); busR.resize ((size_t) n); }

    // ---- relógio: a DAW manda; sem a DAW, o botão PLAY usa o BPM do plugin
    double bpm = juce::jlimit (55.0, 100.0, (double) get ("bpm"));
    bool playing = false; double ppq = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (pos->getIsPlaying())
            {
                playing = true;
                if (auto b = pos->getBpm()) bpm = juce::jlimit (40.0, 200.0, *b);
                if (auto p = pos->getPpqPosition()) ppq = *p;
            }
        }
    if (! playing && get ("play") > 0.5f)
    {
        playing = true; ppq = internalPpq;
        internalPpq += n / sr * bpm / 60.0;
    }
    else if (get ("play") < 0.5f) internalPpq = 0.0;
    const bool grooveRunning = playing && get ("groove") > 0.5f;
    running.store (grooveRunning);

    chain.update (soundParams (bpm));
    const GrooveSettings gs = grooveSettings();

    // ---- MIDI (teclado): tratado no início do bloco, com o deslocamento de cada evento
    for (const auto meta : midi) handleMidi (meta.getMessage(), meta.samplePosition, grooveRunning);

    // ---- groove: cada semicolcheia que começa dentro deste bloco
    if (grooveRunning)
    {
        const double beatsPerSample = bpm / 60.0 / sr;
        const double endPpq = ppq + n * beatsPerSample;
        if (lastPpq < 0 || ppq < lastPpq - 0.01 || ppq > lastPpq + 1.0) lastStep = (long long) std::ceil (ppq * 4.0 - 1e-9) - 1;
        for (long long st = lastStep + 1; st * 0.25 < endPpq; ++st)
        {
            const double stPpq = st * 0.25;
            if (stPpq < ppq - 1e-9) continue;
            const int off = juce::jlimit (0, n - 1, (int) ((stPpq - ppq) / beatsPerSample));
            const auto chord = chordForBar (st / 16);
            for (const auto& a : grooveStep (gs, st, bpm)) doAction (a, chord, off, bpm);
            lastStep = st;
            statusStep.store ((int) (st % 16));
            statusBar.store ((int) ((st / 16) % gs.every));
        }
        lastPpq = endPpq;
    }
    else { lastStep = -1; lastPpq = -1.0; statusStep.store (-1); statusBar.store (-1); }

    // ---- bend e vibrato nas notas de lead seguradas
    vibPhase += n / sr * 5.5; if (vibPhase > 1.0) vibPhase -= std::floor (vibPhase);
    const double cents = bend * 100.0 + vibDepth * 100.0 * std::sin (2.0 * kPi * vibPhase);
    for (auto& v : voices) if (v.active && v.held) v.setPitch (sr, std::pow (2.0, cents / 1200.0));

    // ---- síntese
    float* L = busL.data(); float* R = busR.data();
    std::fill (L, L + n, 0.0f); std::fill (R, R + n, 0.0f);
    for (auto& v : voices)
        if (v.active)
            for (int i = 0; i < n; ++i) v.tick (L[i], R[i]);
    chain.process (L, R, n);
    const float bandGain = gs.band ? 0.5f * get ("out") : 0.0f;
    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    int w = scopeWrite.load();
    for (int i = 0; i < n; ++i)
    {
        const float b = band.tick() * bandGain;
        outL[i] = L[i] + b;
        if (outR) outR[i] = R[i] + b;
        scope[w] = outL[i]; w = (w + 1) % scopeSize;
    }
    scopeWrite.store (w);
}

void ReggaeGuitarProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("track", trackIndex.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void ReggaeGuitarProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto st = juce::ValueTree::fromXml (*xml);
            trackIndex.store ((int) st.getProperty ("track", 0));
            apvts.replaceState (st);
        }
}

juce::AudioProcessorEditor* ReggaeGuitarProcessor::createEditor() { return new ReggaeGuitarEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ReggaeGuitarProcessor(); }
