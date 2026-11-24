#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout OrchHitProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    // centre < 0: linear; senão o meio do botão cai nesse ponto (tempos curtos com mais resolução)
    auto range = [] (const char* id, const char* name, float lo, float hi, float def, float centre = -1.0f)
    {
        NormalisableRange<float> r (lo, hi);
        if (centre > 0.0f) r.setSkewForCentre (lo + (hi - lo) * centre);
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def);
    };
    auto onOff = [] (const char* id, const char* name, bool def) { return std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def); };
    StringArray tim; for (auto& p : moh::presets()) tim.add (String::fromUTF8 (p.lcd));
    StringArray lens; for (int i = 0; i < moh::kNumLengths; ++i) lens.add (moh::lengthName (i));
    StringArray grv; for (auto& g : moh::grooves()) grv.add (String::fromUTF8 (g.name));

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "timbre", 1 }, "Timbre", tim, 0));
    l.add (range ("attack", "Ataque (s)", 0.001f, 0.3f, 0.002f, 0.15f));
    l.add (range ("decay", "Decaimento (s)", 0.01f, 1.0f, 0.25f, 0.25f));
    l.add (range ("sustain", "Sustentacao", 0.0f, 1.0f, 0.6f));
    l.add (range ("release", "Release (s)", 0.01f, 1.0f, 0.06f, 0.2f));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "noteLen", 1 }, "Tamanho da nota", lens, 0));   // 1/64 = padrão
    l.add (onOff ("twoHands", "Duas maos", true));
    l.add (range ("hand", "Mao direita (oitava)", 0.0f, 1.0f, 0.75f));
    l.add (range ("vol", "Volume", 0.0f, 1.0f, 0.8f));
    l.add (range ("reverb", "Reverb plate", 0.0f, 1.0f, 0.25f));
    l.add (onOff ("echoOn", "Tape echo", false));    // padrão de fábrica do kit: começa desligado
    l.add (onOff ("satOn", "Saturador", false));     // padrão de fábrica do kit: começa desligado
    l.add (onOff ("machineOn", "Toca com a DAW", false));
    l.add (onOff ("preview", "Preview", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM do preview", NormalisableRange<float> (50.0f, 160.0f, 1.0f), 84.0f));
    l.add (onOff ("drums", "Bateria do preview", true));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "groove", 1 }, "Groove", grv, 0));
    return l;
}

OrchHitProcessor::OrchHitProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MOH88", createLayout())
{
    for (auto& p : moh::presets()) samples.push_back (moh::renderHit (p));
    std::atomic_store (&grid, std::shared_ptr<const moh::Grid> (std::make_shared<moh::Grid> (moh::grooveGrid (0))));   // o groove selecionado já desenhado
}

bool OrchHitProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void OrchHitProcessor::setParam (const String& id, float v)
{
    if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void OrchHitProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    sampler.prepare (sr);
    drums.prepare (sr);
    reverb.setSampleRate (sr);
    Reverb::Parameters rp; rp.roomSize = 0.55f; rp.damping = 0.45f; rp.wetLevel = 1.0f; rp.dryLevel = 0.0f; rp.width = 1.0f;
    reverb.setParameters (rp); reverb.reset();
    echoBuf.assign ((size_t) (sr * 2.5) + 1, 0.0f); echoPos = 0;
    echoHp.set (moh::Biquad::HP, 300, .707, 0, sr); echoLp.set (moh::Biquad::LP, 2800, .707, 0, sr);
    const auto n = (size_t) jmax (1, samplesPerBlock);
    hitBuf.assign (n, 0); drumBuf.assign (n, 0); revL.assign (n, 0); revR.assign (n, 0);
    evs.reserve (1024);
    previewBeat = 0; lastBlockEndBeat = -1; wasPlaying = false;
}

// ---------------------------------------------------------------- grade
moh::Grid OrchHitProcessor::getGrid() const
{
    auto g = std::atomic_load (&grid);
    return g != nullptr ? *g : moh::Grid {};
}

void OrchHitProcessor::setGrid (const moh::Grid& g, bool editedByUser)
{
    std::atomic_store (&grid, std::shared_ptr<const moh::Grid> (std::make_shared<moh::Grid> (g)));
    custom = editedByUser;
}

void OrchHitProcessor::loadGroove (int i)
{
    setParam ("groove", (float) i);
    setGrid (moh::grooveGrid (i, jmax (1, moh::lengthTicks ((int) get ("noteLen")))), false);
}

bool OrchHitProcessor::writeMidiFile (const File& file) const
{
    const auto g = getGrid();
    if (g.empty()) return false;
    const int tpq = 960;
    MidiMessageSequence meta, tr;
    meta.addEvent (MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / get ("bpm"))), 0.0);
    meta.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    tr.addEvent (MidiMessage::textMetaEvent (3, "Mastor Orch Hit MOH-88"), 0.0);
    for (auto& n : g)
    {
        const double t0 = n.tick / 16.0 * tpq;   // 16 ticks de 1/64 = 1 tempo
        tr.addEvent (MidiMessage::noteOn (1, n.note, (uint8) 110), t0);
        tr.addEvent (MidiMessage::noteOff (1, n.note), t0 + n.len / 16.0 * tpq);
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
void OrchHitProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    if ((int) hitBuf.size() < n) { hitBuf.resize ((size_t) n); drumBuf.resize ((size_t) n); revL.resize ((size_t) n); revR.resize ((size_t) n); }

    sampler.setSample (&samples[(size_t) jlimit (0, moh::numPresets() - 1, (int) get ("timbre"))]);
    sampler.env = { get ("attack"), get ("decay"), get ("sustain"), get ("release") };
    sampler.twoHands = get ("twoHands") > 0.5f; sampler.hand = get ("hand");

    // transporte: a DAW (TOCA COM A DAW) ou o PREVIEW
    const bool machineOn = get ("machineOn") > 0.5f, preview = get ("preview") > 0.5f;
    bool playing = false, hostPlaying = false; double beat0 = 0.0, bpm = get ("bpm");
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (pos->getBpm()) bpm = *pos->getBpm();
            if (machineOn && pos->getIsPlaying() && pos->getPpqPosition()) { playing = hostPlaying = true; beat0 = *pos->getPpqPosition(); }
        }
    if (! playing && preview) { playing = true; beat0 = previewBeat; }
    bpm = jlimit (30.0, 300.0, bpm);
    const double spb = 60.0 / bpm, beatsPerSample = 1.0 / (spb * sr), beat1 = beat0 + n * beatsPerSample;
    const auto tickSamples = (int64_t) std::max (1.0, spb / 16.0 * sr);
    const int lenTicks = moh::lengthTicks ((int) get ("noteLen"));

    evs.clear();
    // MIDI que chega (DAW ou teclado): o TAMANHO DA NOTA corta a nota; em LIVRE vale o note off
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn()) evs.push_back ({ meta.samplePosition, m.getNoteNumber(), m.getFloatVelocity(), lenTicks > 0 ? lenTicks * tickSamples : -1, 0 });
        else if (m.isNoteOff() && lenTicks == 0) evs.push_back ({ meta.samplePosition, m.getNoteNumber(), 0, 0, 1 });
        else if (m.isAllNotesOff() || m.isAllSoundOff()) sampler.allOff();
    }

    // a grade em loop (16 tempos = 256 ticks de 1/64)
    if (playing)
    {
        const auto g = std::atomic_load (&grid);
        const bool withDrums = ! hostPlaying && get ("drums") > 0.5f;
        if (wasPlaying && std::abs (beat0 - lastBlockEndBeat) > 0.02) { /* pulou: as notas da grade já se soltam sozinhas */ }
        for (int64_t k = (int64_t) std::ceil (beat0 * 16.0 - 1e-9); k < beat1 * 16.0; ++k)
        {
            const int at = jlimit (0, n - 1, (int) ((k / 16.0 - beat0) / beatsPerSample));
            const int tk = (int) (((k % moh::kTotalTicks) + moh::kTotalTicks) % moh::kTotalTicks);
            if (withDrums) evs.push_back ({ at, tk % moh::kTicksPerBar, 0, 0, 2 });
            if (g != nullptr)
                for (auto& note : *g)
                    if (note.tick == tk) evs.push_back ({ at, note.note, 0.9f, std::max<int64_t> (1, note.len * tickSamples), 0 });
        }
        playBeat = std::fmod (beat0, 16.0);
    }
    else playBeat = -1.0;
    wasPlaying = playing; lastBlockEndBeat = beat1;
    if (preview && ! hostPlaying) previewBeat = beat1;
    if (! preview) previewBeat = 0.0;
    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.at < b.at; });

    std::fill (hitBuf.begin(), hitBuf.begin() + n, 0.0f);
    std::fill (drumBuf.begin(), drumBuf.begin() + n, 0.0f);
    int pos = 0; size_t ei = 0;
    for (;;)
    {
        while (ei < evs.size() && evs[ei].at <= pos)
        {
            const auto& e = evs[ei++];
            if (e.kind == 0) sampler.noteOn (e.note, e.vel, e.len);
            else if (e.kind == 1) sampler.noteOff (e.note);
            else drums.trigger (e.note);
        }
        const int next = ei < evs.size() ? std::min (n, evs[ei].at) : n;
        if (next > pos) { sampler.render (hitBuf.data() + pos, next - pos); drums.render (drumBuf.data() + pos, next - pos); }
        pos = next;
        if (pos >= n) break;
    }

    // efeitos: saturador (começa desligado), reverb plate (envio), tape echo (começa desligado)
    const bool sat = get ("satOn") > 0.5f, echo = get ("echoOn") > 0.5f;
    const float rev = get ("reverb"), vol = get ("vol");
    for (int i = 0; i < n; ++i)
    {
        float x = hitBuf[(size_t) i] * 0.55f;
        if (sat) x = 0.35f * x + 0.8f * std::tanh (3.0f * x) / std::tanh (3.0f);
        hitBuf[(size_t) i] = x;
        revL[(size_t) i] = revR[(size_t) i] = x * rev;
    }
    reverb.processStereo (revL.data(), revR.data(), n);
    const int echoDelay = jlimit (1, (int) echoBuf.size() - 1, (int) (0.75 * spb * sr));   // colcheia pontuada
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        const float x = hitBuf[(size_t) i];
        int rp = echoPos - echoDelay; if (rp < 0) rp += (int) echoBuf.size();
        const float d = (float) echoHp.process (echoLp.process (echoBuf[(size_t) rp]));
        echoBuf[(size_t) echoPos] = (echo ? x * 0.5f : 0.0f) + d * 0.55f;
        if (++echoPos >= (int) echoBuf.size()) echoPos = 0;
        auto clip = [] (float v) { const float a = std::abs (v); return a <= 0.8f ? v : std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), v); };
        const float common = x + d * 0.6f + drumBuf[(size_t) i] * 0.45f;
        L[i] = clip ((common + revL[(size_t) i]) * vol);
        if (R != nullptr) R[i] = clip ((common + revR[(size_t) i]) * vol);
    }
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
}

// ---------------------------------------------------------------- estado (com a grade e os MIDIs do usuário)
static String notesToString (const std::vector<moh::GridNote>& v)
{
    StringArray rows; for (auto& n : v) rows.add (String (n.tick) + "," + String (n.len) + "," + String (n.note));
    return rows.joinIntoString (";");
}
static std::vector<moh::GridNote> notesFromString (const String& s)
{
    std::vector<moh::GridNote> v;
    for (auto& row : StringArray::fromTokens (s, ";", ""))
    {
        auto f = StringArray::fromTokens (row, ",", "");
        if (f.size() >= 3) v.push_back ({ jlimit (0, moh::kTotalTicks - 1, f[0].getIntValue()), jlimit (1, moh::kTotalTicks, f[1].getIntValue()), jlimit (0, 127, f[2].getIntValue()) });
    }
    return v;
}

void OrchHitProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    for (auto id : { "GRID", "CLIPS" }) state.removeChild (state.getChildWithName (id), nullptr);
    ValueTree g ("GRID");
    g.setProperty ("notes", notesToString (getGrid()), nullptr);
    g.setProperty ("custom", custom.load(), nullptr);
    state.addChild (g, -1, nullptr);
    ValueTree c ("CLIPS");
    for (auto& u : userClips) { ValueTree t ("CLIP"); t.setProperty ("name", u.name, nullptr); t.setProperty ("notes", notesToString (u.notes), nullptr); c.addChild (t, -1, nullptr); }
    state.addChild (c, -1, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void OrchHitProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            auto g = tree.getChildWithName ("GRID"), c = tree.getChildWithName ("CLIPS");
            tree.removeChild (g, nullptr); tree.removeChild (c, nullptr);
            apvts.replaceState (tree);
            if (g.isValid()) setGrid (notesFromString (g.getProperty ("notes").toString()), (bool) g.getProperty ("custom", false));
            else setGrid (moh::grooveGrid ((int) get ("groove")), false);
            userClips.clear();
            for (auto t : c) userClips.push_back ({ t.getProperty ("name").toString(), notesFromString (t.getProperty ("notes").toString()) });
        }
}

AudioProcessorEditor* OrchHitProcessor::createEditor() { return new OrchHitEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OrchHitProcessor(); }
