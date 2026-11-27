#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "RiddimPresets.h"
#include "../../RiddimBank/Source/RiddimBank.h"

namespace
{
    juce::String rootId (int i) { return "root" + juce::String (i); }
    juce::String qualId (int i) { return "qual" + juce::String (i); }
    juce::String durId  (int i) { return "dur"  + juce::String (i); }
    juce::String onId   (int i) { return "on"   + juce::String (i); }

    const int exportRepsTable[] = { 1, 2, 4, 8 };
    constexpr int kMidiChannel = 1;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout BassMachineProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray roots, quals, durs, styles, timbres;
    for (int r = 0; r < 12; ++r) roots.add (mbm::rootName (r));
    for (int q = 0; q < mbm::NumQualities; ++q) quals.add (mbm::qualityLongName (q));
    for (int d = 0; d < mbm::numDurations; ++d) durs.add (mbm::durationName (d));
    for (int s = 0; s < mbm::NumStyles; ++s) styles.add (mbm::styleName (s));
    for (int t = 0; t < mbm::NumTimbres; ++t) timbres.add (mbm::timbre (t).name);

    // Harmonia padrão: Am (1/2 comp.) | Dm (1/2 comp.); acordes 3 e 4 desligados
    const int  defRoot[numSlots] = { 9, 2, 4, 7 };
    const int  defQual[numSlots] = { mbm::Min, mbm::Min, mbm::Min7, mbm::Maj };
    const int  defDur [numSlots] = { 0, 0, 1, 1 };   // 1/2 compasso cada
    const bool defOn  [numSlots] = { true, true, false, false };

    for (int i = 0; i < numSlots; ++i)
    {
        const String n = "Acorde " + String (i + 1);
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { rootId (i), 1 }, n + " tonica",  roots, defRoot[i]));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { qualId (i), 1 }, n + " tipo",    quals, defQual[i]));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { durId (i),  1 }, n + " duracao", durs,  defDur[i]));
        if (i > 0)   // o acorde 1 está sempre ligado
            layout.add (std::make_unique<AudioParameterBool> (ParameterID { onId (i), 1 }, n + " ligado", defOn[i]));
    }

    // estilo e linha
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Estilo", styles, mbm::FamilyMan));
    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "seed", 1 }, "Ideia", 1, 999, 1));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "density", 1 }, "Densidade", 0.0f, 1.0f, 0.6f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "variation", 1 }, "Variacao", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "swing", 1 }, "Swing", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "base", 1 }, "Registro", 28, 40, 31));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "velocity", 1 }, "Velocity", 0.1f, 1.0f, 0.85f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "gate", 1 }, "Sustentacao", 0.0f, 1.0f, 0.7f));

    // preview
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "preview", 1 }, "Play (preview)", false));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "bpm", 1 }, "BPM interno", NormalisableRange<float> (50.0f, 180.0f, 1.0f), 75.0f));

    // som interno
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "sound", 1 }, "Som interno", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "timbre", 1 }, "Timbre", timbres, mbm::RootsSine));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "inGain", 1 }, "Ganho de entrada", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                                                        AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "tone", 1 }, "Tone", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "drive", 1 }, "Drive", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "volume", 1 }, "Volume", 0.0f, 1.0f, 0.75f));

    // acompanhamento do preview
    StringArray drumStyles, keysModes;
    for (int d = 0; d < mbm::NumDrumStyles; ++d) drumStyles.add (mbm::drumStyleName (d));
    for (int k = 0; k < mbm::NumKeysModes; ++k) keysModes.add (mbm::keysModeName (k));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "drumsOn", 1 }, "Bateria", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "drumStyle", 1 }, "Bateria estilo", drumStyles, mbm::DrumOneDrop));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "drumVol", 1 }, "Bateria volume", 0.0f, 1.0f, 0.7f));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "keysOn", 1 }, "Teclado Roots Keys", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "keysMode", 1 }, "Teclado instrumento", keysModes, mbm::KeysPiano));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "keysVol", 1 }, "Teclado volume", 0.0f, 1.0f, 0.6f));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "exportReps", 1 }, "Repeticoes no export",
                                                        StringArray { "1x", "2x", "4x", "8x" }, 1));
    // chimbal da bateria de acompanhamento: 1/8 (como antes) ou 1/16
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "hat16", 1 }, "Hi-hat 1/16", false));
    return layout;
}

//==============================================================================
BassMachineProcessor::BassMachineProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BassMachine", createLayout())
{
    for (auto* p : getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            apvts.addParameterListener (withId->paramID, this);

    pending.reserve (1024);
    accPending.reserve (4096);
}

BassMachineProcessor::~BassMachineProcessor()
{
    for (auto* p : getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            apvts.removeParameterListener (withId->paramID, this);
}

bool BassMachineProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void BassMachineProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    synth.prepare (sampleRate, samplesPerBlock);
    monoBuf.setSize (1, juce::jmax (1, samplesPerBlock), false, true, false);
    setLatencySamples (synth.getLatencySamples());
    acousticDrums.prepare (sampleRate);
    digitalDrums.prepare (sampleRate);
    keys.prepare (sampleRate);
    dirty.store (true);
}

//==============================================================================
std::vector<mbm::ChordSlot> BassMachineProcessor::getSlots() const
{
    std::vector<mbm::ChordSlot> slots;
    for (int i = 0; i < numSlots; ++i)
    {
        mbm::ChordSlot s;
        s.root    = (int) param (rootId (i));
        s.quality = juce::jlimit (0, mbm::NumQualities - 1, (int) param (qualId (i)));
        const bool on = (i == 0) || param (onId (i)) > 0.5f;
        s.beats   = on ? mbm::durationBeats ((int) param (durId (i))) : 0.0;
        slots.push_back (s);
    }
    return slots;
}

mbm::Settings BassMachineProcessor::getSettings() const
{
    mbm::Settings s;
    s.style     = (int) param ("style");
    s.seed      = (int) param ("seed");
    s.density   = param ("density");
    s.variation = param ("variation");
    s.swing     = param ("swing");
    s.base      = (int) param ("base");
    s.velocity  = param ("velocity");
    s.gate      = param ("gate");
    s.strict    = strictReggae.load();
    return s;
}

void BassMachineProcessor::newIdea()
{
    if (auto* p = apvts.getParameter ("seed"))
    {
        const int current = (int) param ("seed");
        int next = current;
        juce::Random rng;
        while (next == current) next = 1 + rng.nextInt (999);
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) next));
        p->endChangeGesture();
    }
}

double BassMachineProcessor::getExportBpm() const
{
    const double h = hostBpm.load();
    return h > 0.0 ? h : (double) param ("bpm");
}

uint64_t BassMachineProcessor::settingsKey() const
{
    uint64_t h = 1469598103934665603ull;   // FNV-1a
    auto mix = [&h] (int64_t v) { for (int i = 0; i < 8; ++i) { h ^= (uint64_t) ((v >> (i * 8)) & 0xff); h *= 1099511628211ull; } };
    for (const char* id : { "style", "seed", "density", "variation", "swing", "base", "velocity", "gate" })
        mix ((int64_t) std::llround (param (id) * 10000.0f));
    mix (strictReggae.load() ? 1 : 0);
    for (int i = 0; i < numSlots; ++i)
    {
        mix ((int64_t) param (rootId (i))); mix ((int64_t) param (qualId (i))); mix ((int64_t) param (durId (i)));
        mix (i == 0 ? 1 : (param (onId (i)) > 0.5f ? 1 : 0));
    }
    return h;
}

std::vector<mbm::NoteEvent> BassMachineProcessor::getCurrentLine (double& lb) const
{
    {
        const juce::SpinLock::ScopedLockType sl (editLock);
        if (editsOn && editsKey == settingsKey()) { lb = editsLoopBeats; return edits; }
    }
    return mbm::buildLoop (getSlots(), getSettings(), lb);
}

void BassMachineProcessor::setEdits (std::vector<mbm::NoteEvent> ev, double lb)
{
    if (lb <= 0.0) return;
    for (auto& e : ev)
    {
        e.note = juce::jlimit (24, 72, e.note);
        e.velocity = juce::jlimit (1, 127, e.velocity);
        e.start = juce::jlimit (0.0, lb - 0.0625, e.start);
    }
    mbm::monophonic (ev, lb);
    {
        const juce::SpinLock::ScopedLockType sl (editLock);
        edits = std::move (ev);
        editsLoopBeats = lb;
        editsKey = settingsKey();
        editsOn = true;
    }
    dirty.store (true);
}

bool BassMachineProcessor::hasEdits() const
{
    const juce::SpinLock::ScopedLockType sl (editLock);
    return editsOn && editsKey == settingsKey();
}

void BassMachineProcessor::clearEdits()
{
    {
        const juce::SpinLock::ScopedLockType sl (editLock);
        editsOn = false;
        edits.clear();
    }
    dirty.store (true);
}

void BassMachineProcessor::dropStaleEdits()
{
    bool stale;
    {
        const juce::SpinLock::ScopedLockType sl (editLock);
        stale = editsOn && editsKey != settingsKey();
    }
    if (stale) clearEdits();
}

void BassMachineProcessor::rebuild()
{
    const auto slots = getSlots();
    const auto s = getSettings();
    loop = mbm::buildLoop (slots, s, loopBeats);
    {
        // linha editada pela interface (se ainda vale para estes controles)
        const juce::SpinLock::ScopedTryLockType tl (editLock);
        if (! tl.isLocked()) dirty.store (true);
        else if (editsOn && editsKey == settingsKey()) { loop = edits; loopBeats = editsLoopBeats; }
    }
    accLoop.clear();
    if (param ("drumsOn") > 0.5f)
        accLoop = mbm::buildDrums (loopBeats, (int) param ("drumStyle"), s.swing, param ("hat16") > 0.5f);
    if (param ("keysOn") > 0.5f)
    {
        const auto k = mbm::buildKeys (slots, (int) param ("keysMode"), s.swing);
        accLoop.insert (accLoop.end(), k.begin(), k.end());
    }
    std::stable_sort (accLoop.begin(), accLoop.end(), [] (const mbm::AccEvent& a, const mbm::AccEvent& b) { return a.start < b.start; });
}

void BassMachineProcessor::accompOff()
{
    keys.allNotesOff();
    for (auto& row : keysActive) for (auto& a : row) a = false;
}

void BassMachineProcessor::allNotesOff (juce::MidiBuffer& midi, int sample)
{
    accompOff();
    for (int n = 0; n < 128; ++n)
        if (active[n])
        {
            midi.addEvent (juce::MidiMessage::noteOff (kMidiChannel, n), sample);
            active[n] = false;
        }
}

//==============================================================================
// Ouvir a tônica: a fundamental do acorde no registro da linha (REGISTRO), com a VELOCITY da linha
void BassMachineProcessor::auditionStart (int slot)
{
    if (! juce::isPositiveAndBelow (slot, numSlots)) return;
    const int root = juce::roundToInt (param ("root" + juce::String (slot)));
    const int base = juce::roundToInt (param ("base"));
    auditionReqNote.store (juce::jlimit (0, 127, base + ((root - base) % 12 + 12) % 12));
    auditionReqVel.store (juce::jlimit (30, 127, juce::roundToInt (param ("velocity") * 127.0f)));
    auditionReq.store (1);
}

void BassMachineProcessor::auditionStop() { auditionReq.store (2); }

void BassMachineProcessor::runAudition (juce::MidiBuffer& midi)
{
    auditionOut.clear();
    const int req = auditionReq.exchange (0);
    if (req == 0) return;
    auto add = [&] (const juce::MidiMessage& m) { midi.addEvent (m, 0); auditionOut.addEvent (m, 0); };
    if (auditionNote >= 0) add (juce::MidiMessage::noteOff (kMidiChannel, auditionNote));
    auditionNote = -1;
    if (req != 1) return;
    auditionNote = auditionReqNote.load();
    add (juce::MidiMessage::noteOn (kMidiChannel, auditionNote, (juce::uint8) auditionReqVel.load()));
}

// Notas da linha soando no fim do bloco, por acorde (a interface pinta o teclado da tônica com elas)
void BassMachineProcessor::publishPlaying()
{
    juce::uint32 m[numSlots] {};
    for (int n = 0; n < 128; ++n)
        if (active[n] && juce::isPositiveAndBelow ((int) activeSlot[n], numSlots))
            m[activeSlot[n]] |= 1u << (n % 12);
    for (int i = 0; i < numSlots; ++i) playMask[(size_t) i].store (m[i]);
}

void BassMachineProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int numSamples = buffer.getNumSamples();

    if (dirty.exchange (false))
        rebuild();

    runAudition (midi);   // ouvir a tônica (teclado clicado na interface)

    // --- Posição de transporte ---
    double bpm = 0.0, ppq = 0.0;
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())         bpm = *b;
            if (auto p = pos->getPpqPosition()) ppq = *p;
            hostPlaying = pos->getIsPlaying();
        }
    if (bpm > 0.0) hostBpm.store (bpm);

    // PLAY: toca o loop no BPM do knob quando a DAW está parada
    bool playing = hostPlaying;
    if (! hostPlaying && param ("preview") > 0.5f)
    {
        bpm = (double) param ("bpm");
        ppq = internalPpq;
        playing = true;
    }
    if (param ("preview") < 0.5f || hostPlaying)
        internalPpq = 0.0;

    if (bpm <= 0.0) bpm = 120.0;
    const double blockBeats = (double) numSamples / sr * bpm / 60.0;

    if (! playing || loopBeats <= 0.0)
    {
        if (wasPlaying) allNotesOff (midi, 0);
        wasPlaying = false;
        displayPos.store (-1.0);
    }
    else
    {
        // salto no transporte (loop da DAW, reposicionamento): limpa notas presas
        if (wasPlaying && std::abs (ppq - expectedPpq) > 1.0e-3)
            allNotesOff (midi, 0);

        const double end = ppq + blockBeats;
        pending.clear();
        accPending.clear();

        auto collect = [&] (double t, bool on, const mbm::NoteEvent& e)
        {
            double occ = t + std::ceil ((ppq - t) / loopBeats) * loopBeats;
            while (occ < end)
            {
                if (occ >= ppq)
                {
                    const int s = juce::jlimit (0, numSamples - 1, (int) ((occ - ppq) / blockBeats * numSamples));
                    pending.push_back ({ s, on, e.note, e.velocity, e.slot });
                }
                occ += loopBeats;
            }
        };

        for (const auto& e : loop)
        {
            collect (e.start, true, e);
            collect (e.start + e.length, false, e);
        }

        // acompanhamento (bateria e teclado): só o som interno, não sai como MIDI
        auto collectAcc = [&] (double t, bool on, const mbm::AccEvent& e)
        {
            double occ = t + std::ceil ((ppq - t) / loopBeats) * loopBeats;
            for (; occ < end; occ += loopBeats)
                if (occ >= ppq)
                    accPending.push_back ({ juce::jlimit (0, numSamples - 1, (int) ((occ - ppq) / blockBeats * numSamples)),
                                            on, e.target, e.note, e.velocity });
        };
        for (const auto& e : accLoop)
        {
            collectAcc (e.start, true, e);
            if (e.target == mbm::AccPiano || e.target == mbm::AccOrgan)
                collectAcc (e.start + e.length, false, e);
        }
        std::sort (accPending.begin(), accPending.end(), [] (const AccPending& a, const AccPending& b)
        {
            if (a.sample != b.sample) return a.sample < b.sample;
            return (int) a.on < (int) b.on;
        });

        // note-offs antes de note-ons na mesma amostra
        std::sort (pending.begin(), pending.end(), [] (const Pending& a, const Pending& b)
        {
            if (a.sample != b.sample) return a.sample < b.sample;
            return (int) a.on < (int) b.on;
        });

        for (const auto& p : pending)
        {
            bool& isOn = active[p.note];
            if (p.on)
            {
                if (isOn) midi.addEvent (juce::MidiMessage::noteOff (kMidiChannel, p.note), p.sample);
                midi.addEvent (juce::MidiMessage::noteOn (kMidiChannel, p.note, (juce::uint8) p.vel), p.sample);
                isOn = true;
                activeSlot[p.note] = (signed char) p.slot;
                if (juce::isPositiveAndBelow (p.slot, numSlots))
                    hitMask[(size_t) p.slot].fetch_or (1u << (p.note % 12));
            }
            else if (isOn)
            {
                midi.addEvent (juce::MidiMessage::noteOff (kMidiChannel, p.note), p.sample);
                isOn = false;
            }
        }

        wasPlaying = true;
        expectedPpq = end;
        if (! hostPlaying) internalPpq = end;
        displayPos.store (std::fmod (std::max (0.0, ppq), loopBeats));
    }
    publishPlaying();

    // --- Som interno (o MIDI gerado continua saindo para a DAW) ---
    synth.set ((int) param ("timbre"), param ("tone"), param ("drive"));
    synth.setInputGainDb (param ("inGain"));
    renderSynth (buffer, midi);

    if (param ("sound") < 0.5f) buffer.clear();

    // osciloscópio: só o baixo
    int sw = scopeWrite.load (std::memory_order_relaxed);
    const float* l = buffer.getReadPointer (0);
    const float* r = buffer.getReadPointer (buffer.getNumChannels() > 1 ? 1 : 0);
    for (int i = 0; i < numSamples; ++i)
    {
        scopeBuf[(size_t) sw] = 0.5f * (l[i] + r[i]);
        sw = (sw + 1) & (kScopeSize - 1);
    }
    scopeWrite.store (sw, std::memory_order_release);

    // bateria e teclado por cima do baixo; VOLUME é o master do preview
    const bool keysOn = param ("keysOn") > 0.5f;
    if (lastKeysOn && ! keysOn) accompOff();
    lastKeysOn = keysOn;
    if (! playing) accPending.clear();
    renderAccomp (buffer, numSamples);
    buffer.applyGain (param ("volume"));

    // limitador suave de segurança: acima de 0,8 o pico entra num joelho de tanh e nunca passa de 1
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float a = std::abs (d[i]);
            if (a > 0.8f) d[i] = std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), d[i]);
        }
    }
}

void BassMachineProcessor::renderAccomp (juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    const float drumGain = 1.1f * param ("drumVol");
    keys.master = 2.2f * param ("keysVol");
    keys.pianoLevel = 0.8f;
    keys.organLevel = 0.55f;
    float scratchR[512];   // o motor do teclado soma em estéreo: em mono, soma num rascunho

    // o motor do teclado soma em estéreo: em mono, soma num rascunho e copia

    auto renderSeg = [&] (int from, int to)
    {
        if (to <= from) return;
        acousticDrums.render (L, R, from, to - from, drumGain);
        digitalDrums.render (L, R, from, to - from, drumGain);
        for (int p = from; p < to; p += 512)
        {
            const int n = juce::jmin (512, to - p);
            keys.render (L + p, R != nullptr ? R + p : scratchR, n);
        }
    };

    int pos = 0;
    for (const auto& a : accPending)
    {
        renderSeg (pos, a.sample);
        pos = juce::jmax (pos, a.sample);
        switch (a.target)
        {
            case mbm::AccAcousticDrum: acousticDrums.trigger (a.note, a.vel); break;
            case mbm::AccDigitalDrum:  digitalDrums.trigger (a.note, a.vel); break;
            default:
            {
                const int ch = a.target == mbm::AccOrgan ? 3 : 1;
                bool& act = keysActive[a.target == mbm::AccOrgan ? 1 : 0][juce::jlimit (0, 127, a.note)];
                if (a.on) { if (act) keys.noteOff (ch, a.note); keys.noteOn (ch, a.note, (float) a.vel / 127.0f); act = true; }
                else if (act) { keys.noteOff (ch, a.note); act = false; }
                break;
            }
        }
    }
    renderSeg (pos, numSamples);
}

// voz do Dub Bass com precisão de amostra: renderiza os trechos entre os eventos MIDI,
// depois a saturação e o maximizer do Dub Bass
void BassMachineProcessor::renderSynth (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    const int numSamples = buffer.getNumSamples();
    if (monoBuf.getNumSamples() < numSamples) monoBuf.setSize (1, numSamples, false, false, true);
    float* mono = monoBuf.getWritePointer (0);
    int pos = 0;

    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, numSamples, meta.samplePosition);
        if (t > pos) { synth.renderVoice (mono, pos, t); pos = t; }
        const auto m = meta.getMessage();
        if (m.isNoteOn())                                { synth.noteOn (m.getNoteNumber(), m.getFloatVelocity()); soundingNote.store (m.getNoteNumber()); }
        else if (m.isNoteOff())                          { synth.noteOff (m.getNoteNumber()); if (soundingNote.load() == m.getNoteNumber()) soundingNote.store (-1); }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) { synth.allNotesOff(); soundingNote.store (-1); }
    }
    if (pos < numSamples) synth.renderVoice (mono, pos, numSamples);
    synth.finish (monoBuf, numSamples);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, monoBuf, 0, 0, numSamples);
}

//==============================================================================
bool BassMachineProcessor::writeMidiFile (const juce::File& file) const
{
    double lb = 0.0;
    const auto s = getSettings();
    const auto events = getCurrentLine (lb);
    if (lb <= 0.0 || events.empty()) return false;

    const int tpq = 960;
    const int reps = exportRepsTable[juce::jlimit (0, 3, (int) param ("exportReps"))];
    const double bpm = getExportBpm();

    juce::MidiMessageSequence meta, bass;
    meta.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)), 0.0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    bass.addEvent (juce::MidiMessage::textMetaEvent (3, juce::String ("Reggae Bass Machine - ") + mbm::styleName (s.style)
                                                          + " ideia " + juce::String (s.seed)), 0.0);

    for (int r = 0; r < reps; ++r)
        for (const auto& e : events)
        {
            const double on  = (e.start + r * lb) * tpq;
            const double off = (e.start + e.length + r * lb) * tpq;
            bass.addEvent (juce::MidiMessage::noteOn (kMidiChannel, e.note, (juce::uint8) e.velocity), on);
            bass.addEvent (juce::MidiMessage::noteOff (kMidiChannel, e.note), off);
        }
    bass.sort();
    bass.updateMatchedPairs();

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (meta);
    mf.addTrack (bass);

    file.getParentDirectory().createDirectory();
    file.deleteFile();
    juce::FileOutputStream out (file);
    if (! out.openedOk()) return false;
    return mf.writeTo (out);
}

//==============================================================================
void BassMachineProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    {
        // linha editada vai junto no projeto
        const juce::SpinLock::ScopedLockType sl (editLock);
        if (editsOn && editsKey == settingsKey())
        {
            juce::ValueTree ed ("EDITS");
            ed.setProperty ("loopBeats", editsLoopBeats, nullptr);
            for (const auto& e : edits)
            {
                juce::ValueTree n ("N");
                n.setProperty ("s", e.start, nullptr);
                n.setProperty ("l", e.length, nullptr);
                n.setProperty ("n", e.note, nullptr);
                n.setProperty ("v", e.velocity, nullptr);
                ed.appendChild (n, nullptr);
            }
            state.appendChild (ed, nullptr);
        }
    }
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BassMachineProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            auto ed = tree.getChildWithName ("EDITS");
            if (ed.isValid()) tree.removeChild (ed, nullptr);
            apvts.replaceState (tree);
            if (ed.isValid())
            {
                std::vector<mbm::NoteEvent> ev;
                for (const auto& n : ed)
                    ev.push_back ({ (double) n.getProperty ("s"), (double) n.getProperty ("l"), (int) n.getProperty ("n"),
                                    (int) n.getProperty ("v"), 0, false });
                setEdits (std::move (ev), (double) ed.getProperty ("loopBeats"));
            }
            else clearEdits();
        }
    dirty.store (true);
}

juce::AudioProcessorEditor* BassMachineProcessor::createEditor() { return new BassMachineEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BassMachineProcessor(); }

//==============================================================================
void BassMachineProcessor::loadRiddim (int index)
{
    if (index < 0 || index >= mbmriddims::numRiddims) return;
    const auto& r = mbmriddims::riddims[index];

    auto set = [this] (const juce::String& id, float plainValue)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
    };

    for (int i = 0; i < numSlots; ++i)
    {
        if (i < r.numChords)
        {
            set (rootId (i), (float) r.chords[i].root);
            set (qualId (i), (float) r.chords[i].quality);
            set (durId (i),  (float) r.chords[i].dur);
        }
        if (i > 0) set (onId (i), i < r.numChords ? 1.0f : 0.0f);
    }
    set ("style", (float) r.style);
    set ("density", r.density);
    set ("variation", r.variation);
    set ("swing", r.swing);
    set ("seed", 1.0f);
    set ("bpm", (float) r.bpm);
    set ("drumStyle", (float) r.drum);
    set ("drumsOn", 1.0f);
    set ("timbre", (float) r.timbre);
    clearEdits();
    apvts.state.setProperty ("riddim", index, nullptr);
    apvts.state.setProperty ("bank", -1, nullptr);
    dirty.store (true);
}

void BassMachineProcessor::applyBankEntry (int index)
{
    if (index < 0 || index >= riddimbank::numEntries) return;
    const auto& e = riddimbank::entries[index];
    if (e.numChords <= 0) return;   // harmonia ainda pendente

    auto set = [this] (const juce::String& id, float plainValue)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
    };

    if (e.mrbm >= 0 && e.mrbm < mbmriddims::numRiddims)
        loadRiddim (e.mrbm);       // o baixo do riddim (baixista, densidade, timbre, bateria)
    else
    {
        set ("style", (float) e.bassStyle);
        set ("density", e.density);
        set ("variation", e.variation);
        set ("swing", e.swing);
        set ("seed", 1.0f);
        set ("drumStyle", (float) e.bassDrum);
        set ("drumsOn", 1.0f);
        set ("timbre", (float) e.timbre);
        clearEdits();
        apvts.state.setProperty ("riddim", -1, nullptr);
    }
    // a harmonia e o BPM do banco valem sobre os do riddim (conferidos com as gravações)
    for (int i = 0; i < numSlots; ++i)
    {
        if (i < e.numChords)
        {
            set (rootId (i), (float) e.chords[i].root);
            set (qualId (i), (float) (e.chords[i].minor ? mbm::Min : mbm::Maj));
            set (durId (i),  (float) e.chords[i].dur);
        }
        if (i > 0) set (onId (i), i < e.numChords ? 1.0f : 0.0f);
    }
    set ("bpm", (float) e.bpm);
    apvts.state.setProperty ("bank", index, nullptr);
    dirty.store (true);
}
