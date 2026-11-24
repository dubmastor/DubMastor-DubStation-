#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Musicians.h"

using namespace juce;

namespace
{
    NormalisableRange<float> skewed (float lo, float hi, float centre)
    {
        NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }
    AudioParameterFloatAttributes fmt (std::function<String (float)> f)
    {
        return AudioParameterFloatAttributes().withStringFromValueFunction ([f] (float v, int) { return f (v); });
    }
    String pct (float v) { return String (roundToInt (v * 100.0f)) + "%"; }
    String hz (float v)  { return v >= 1000.0f ? String (v / 1000.0f, v >= 10000.0f ? 0 : 1) + "k" : String (roundToInt (v)); }
    String ms (float v)  { return String (roundToInt (v)) + " ms"; }
    String db (float v)  { return (v > 0 ? "+" : "") + String (v, 1) + " dB"; }
    String panTxt (float v) { return std::abs (v) < 0.04f ? String ("C") : (v < 0 ? "E" : "D") + String (roundToInt (std::abs (v) * 100.0f)); }
}

StringArray HornsProcessor::instIds()
{
    StringArray a;
    for (const auto& d : horns::instruments()) a.add (d.id);
    return a;
}

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout HornsProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;
    const horns::Params def;

    StringArray keys, scales, harms, octs { "-2", "-1", "0", "+1" }, syncs;
    const char* pt[12] = { "Dó", "Dó#", "Ré", "Ré#", "Mi", "Fá", "Fá#", "Sol", "Sol#", "Lá", "Lá#", "Si" };
    for (int i = 0; i < 12; ++i) keys.add (String (horns::noteName (i)) + " (" + HU (pt[i]) + ")");
    for (auto* s : { "Menor natural", "Maior", "Dórico", "Mixolídio", "Pentatônica menor", "Pentatônica maior", "Blues" }) scales.add (HU (s));
    for (auto* h : { "Uníssono", "3ª acima", "3ª abaixo", "5ª abaixo", "5ª acima", "6ª abaixo", "4ª acima" }) harms.add (HU (h));
    for (const auto& s : horns::syncDivs()) syncs.add (s.name);

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Tom", keys, def.key));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Escala", scales, def.scale));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "master", 1 }, "Master", NormalisableRange<float> (0, 1), def.master, fmt (pct)));

    const auto ids = instIds();
    for (int i = 0; i < horns::kNumInst; ++i)
    {
        const auto& d = horns::instruments()[(size_t) i];
        const auto& s = def.inst[(size_t) i];
        const String id = ids[i], nm = String (d.name).toLowerCase();
        layout.add (std::make_unique<AudioParameterBool>   (ParameterID { id + "_on", 1 },   nm + " no naipe", s.on));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { id + "_lvl", 1 },  nm + " volume", NormalisableRange<float> (0, 1), s.level, fmt (pct)));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { id + "_pan", 1 },  nm + " pan", NormalisableRange<float> (-1, 1), s.pan, fmt (panTxt)));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id + "_oct", 1 },  nm + " oitava", octs, s.oct + 2));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id + "_harm", 1 }, nm + " harmonia", harms, s.harm));
    }

    // timbre anos 70
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bright", 1 },  "Brilho",      NormalisableRange<float> (0, 1), def.bright, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "breath", 1 },  "Sopro",       NormalisableRange<float> (0, 1), def.breath, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "vib", 1 },     "Vibrato",     NormalisableRange<float> (0, 1), def.vib, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "scoop", 1 },   "Scoop",       NormalisableRange<float> (0, 1), def.scoop, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 },  "Ataque",      NormalisableRange<float> (0.3f, 2.5f), def.attack,
                                                       fmt ([] (float v) { return String (v, 1) + "x"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release",     NormalisableRange<float> (0, 1), def.release,
                                                       fmt ([] (float v) { return ms ((0.05f + v * 0.45f) * 1000.0f); })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 },   HU ("Saturação de fita"), NormalisableRange<float> (0, 1), def.drive, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "band", 1 },    "Banda (agudo)", skewed (2500, 18000, 8000), def.band, fmt (hz)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "lowcut", 1 },  "Corte grave", skewed (20, 500, 90), def.lowcut, fmt (hz)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "hiss", 1 },    "Chiado de fita", NormalisableRange<float> (0, 1), def.hiss, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "human", 1 },   "Humanizar",   NormalisableRange<float> (0, 1), def.human, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "width", 1 },   HU ("Abertura estéreo"), NormalisableRange<float> (0, 1), def.width, fmt (pct)));
    // v1.2
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "growl", 1 },   "Rouco (growl)", NormalisableRange<float> (0, 1), def.growl, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "room", 1 },    HU ("Sala do estúdio"), NormalisableRange<float> (0, 1), def.room, fmt (pct)));

    // Mastor Tape Echo
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "echoOn", 1 },  "Echo ligado", def.echoOn));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "sync", 1 },    "Echo sync", syncs, def.sync));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "echoTime", 1 }, "Echo tempo livre", skewed (40, 1500, 375), def.echoMs, fmt (ms)));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "fb", 1 },      "Echo intensidade", NormalisableRange<float> (0, 0.95f), def.fb, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "send", 1 },    "Echo envio", NormalisableRange<float> (0, 1), def.send, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "ret", 1 },     "Echo volume", NormalisableRange<float> (0, 1.2f), def.ret, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "tone", 1 },    "Echo agudos", skewed (800, 12000, 3200), def.tone, fmt (hz)));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "bass", 1 },    "Echo corte grave", skewed (40, 1200, 220), def.bass, fmt (hz)));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "wow", 1 },     "Wow & flutter", NormalisableRange<float> (0, 1), def.wow, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "sat", 1 },     HU ("Echo saturação"), NormalisableRange<float> (0, 1), def.sat, fmt (pct)));
    for (int h = 0; h < 3; ++h)
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { "head" + String (h + 1), 1 }, HU ("Cabeça ") + String (h + 1), def.head[(size_t) h]));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "throw", 1 },   "Dub throw", false));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "selfOsc", 1 }, "Auto-oscilar", false));

    // EQ gráfico + Filtro Tubby
    const char* bands[10] = { "31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "16k" };
    for (int b = 0; b < 10; ++b)
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "eq" + String (b), 1 }, "EQ " + String (bands[b]),
                                                           NormalisableRange<float> (-12, 12, 0.5f), 0.0f, fmt (db)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tubby", 1 },    HU ("Filtro Tubby frequência"), skewed (20, 2000, 200), def.tubby, fmt (hz)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tubbyRes", 1 }, HU ("Filtro Tubby ressonância"), NormalisableRange<float> (0, 1), def.tubbyRes, fmt (pct)));

    // frases
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "phrasePlay", 1 }, "Tocar frase", false));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 },        "BPM interno", NormalisableRange<float> (50, 160, 1), 74.0f,
                                                       fmt ([] (float v) { return String (roundToInt (v)) + " BPM"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "swing", 1 },      "Swing", NormalisableRange<float> (0, 1), 0.1f, fmt (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "phraseVel", 1 },  HU ("Força da frase"), NormalisableRange<float> (0.2f, 1), 0.8f, fmt (pct)));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "guide", 1 },      "Guia one drop", true));
    return layout;
}

//==============================================================================
HornsProcessor::HornsProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MastorDubHorns", createLayout())
{
    const auto& p = horns::stabPatterns().front();
    phraseShared = horns::makePhrase (p, true, 9, 0);
    phraseShared.events.reserve (horns::kMaxEvents);
    phraseAudio.events.reserve (horns::kMaxEvents);
    phraseAudio.name.reserve (128);
    sched.reserve (1024);
    applyMusico (musicos::defaultPreset, false);   // abre no som do "Tubby · naipe no eco"...
    // ...mas o Tape Echo e a saturação de fita começam desligados (padrão de fábrica do kit)
    if (auto* e = apvts.getParameter ("echoOn")) e->setValueNotifyingHost (0.0f);
    if (auto* d = apvts.getParameter ("drive"))  d->setValueNotifyingHost (0.0f);
}

void HornsProcessor::applyMusico (int index, bool preview)
{
    if (index < 0 || index >= musicos::numPresets) return;
    const auto& pr = musicos::presets[index];
    auto set = [this] (const String& id, float plain)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (plain)); p->endChangeGesture();
        }
    };
    const auto ids = instIds();
    for (int k = 0; k < horns::kNumInst; ++k)
    {
        const auto& sc = pr.sec[k];
        set (ids[k] + "_on", (float) sc.on); set (ids[k] + "_lvl", sc.lvl); set (ids[k] + "_pan", sc.pan);
        set (ids[k] + "_oct", (float) (sc.oct + 2)); set (ids[k] + "_harm", (float) sc.harm);
    }
    for (int k = 0; k < 14; ++k) set (musicos::timbreIds[k], pr.t[k]);
    set ("echoOn", (float) pr.echoOn); set ("sync", (float) pr.sync); set ("fb", pr.fb); set ("send", pr.send); set ("tone", pr.tone);
    for (int b = 0; b < 10; ++b) set ("eq" + String (b), pr.eq[b]);
    apvts.state.setProperty ("musico", index, nullptr);
    if (preview) previewNote (60 + (int) param ("key"), pr.artic);
}

int HornsProcessor::currentMusico() const
{
    return jlimit (-1, musicos::numPresets - 1, (int) apvts.state.getProperty ("musico", -1));
}

bool HornsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == AudioChannelSet::mono() || out == AudioChannelSet::stereo();
}

void HornsProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    horns::Params P;
    fillParams (P, getBpmForDisplay());
    engine.setParams (P);
    engine.prepare (sampleRate, std::max (512, samplesPerBlock));
    bufL.assign ((size_t) std::max (8192, samplesPerBlock), 0.0f);
    bufR.assign ((size_t) std::max (8192, samplesPerBlock), 0.0f);
    std::fill (std::begin (activePhrase), std::end (activePhrase), false);
    wasPlaying = false;
    internalPpq = 0.0;
}

double HornsProcessor::getBpmForDisplay() const
{
    const double h = hostBpm.load();
    return h > 0.0 ? h : (double) param ("bpm");
}

void HornsProcessor::fillParams (horns::Params& P, double bpm) const
{
    P.key = (int) param ("key");
    P.scale = (int) param ("scale");
    P.master = param ("master");
    const auto ids = instIds();
    for (int i = 0; i < horns::kNumInst; ++i)
    {
        auto& s = P.inst[(size_t) i];
        const String id = ids[i];
        s.on = param (id + "_on") > 0.5f;
        s.level = param (id + "_lvl");
        s.pan = param (id + "_pan");
        s.oct = (int) param (id + "_oct") - 2;
        s.harm = (int) param (id + "_harm");
    }
    P.bright = param ("bright"); P.breath = param ("breath"); P.vib = param ("vib"); P.scoop = param ("scoop");
    P.attack = param ("attack"); P.release = param ("release"); P.drive = param ("drive"); P.band = param ("band");
    P.lowcut = param ("lowcut"); P.hiss = param ("hiss"); P.human = param ("human"); P.width = param ("width");
    P.growl = param ("growl"); P.room = param ("room");
    P.echoOn = param ("echoOn") > 0.5f; P.sync = (int) param ("sync"); P.echoMs = param ("echoTime");
    P.fb = param ("fb"); P.send = param ("send"); P.ret = param ("ret"); P.tone = param ("tone"); P.bass = param ("bass");
    P.wow = param ("wow"); P.sat = param ("sat");
    P.head = { param ("head1") > 0.5f, param ("head2") > 0.5f, param ("head3") > 0.5f };
    if (! (P.head[0] || P.head[1] || P.head[2])) P.head[0] = true;
    P.throwOn = param ("throw") > 0.5f;
    P.selfOsc = param ("selfOsc") > 0.5f;
    for (int b = 0; b < 10; ++b) P.eq[(size_t) b] = param ("eq" + String (b));
    P.tubby = param ("tubby"); P.tubbyRes = param ("tubbyRes");
    P.bpm = bpm > 0.0 ? bpm : 74.0;
}

horns::Params HornsProcessor::currentParams() const
{
    horns::Params P;
    fillParams (P, getBpmForDisplay());
    return P;
}

//==============================================================================
horns::Phrase HornsProcessor::getPhrase() const
{
    const SpinLock::ScopedLockType sl (phraseLock);
    return phraseShared;
}

void HornsProcessor::setPhrase (const horns::Phrase& p)
{
    auto copy = p;
    if ((int) copy.events.size() > horns::kMaxEvents) copy.events.resize ((size_t) horns::kMaxEvents);
    copy.length = jlimit (4, 64, copy.length);
    for (auto& e : copy.events)
    {
        e.start = jlimit (0, copy.length - 1, e.start);
        e.dur = jlimit (1, 16, e.dur);
        e.note = e.note < 0 ? -1 : jlimit (0, 127, e.note);
        e.artic = jlimit (0, horns::kNumArtic - 1, e.artic);
    }
    horns::sortEvents (copy);
    copy.events.reserve (horns::kMaxEvents);
    {
        const SpinLock::ScopedLockType sl (phraseLock);
        phraseShared = std::move (copy);
    }
    ++phraseVersion;
}

bool HornsProcessor::popPickedNote (int& note)
{
    int got = 0;
    pickFifo.read (1).forEach ([&] (int i) { note = pickBuf[(size_t) i]; got = 1; });
    return got != 0;
}

void HornsProcessor::previewNote (int note, int artic)
{
    previewFifo.write (1).forEach ([&] (int i) { previewBuf[(size_t) i] = { note, artic }; });
}

//==============================================================================
void HornsProcessor::renderClicks (float* L, float* R, int n)
{
    const float twoPiOverSr = MathConstants<float>::twoPi / (float) sr;
    for (auto& c : clicks)
    {
        if (c.env < 1.0e-4f && c.phase >= 0.0f) continue;
        for (int i = 0; i < n; ++i)
        {
            if (c.phase < 0.0f) { c.phase += 1.0f; if (c.phase < 0.0f) continue; c.phase = 0.0f; }  // atraso até a amostra do clique
            const float s = std::sin (c.phase) * c.env * 0.35f;
            c.phase += c.freq * twoPiOverSr;
            if (c.freq > 70.0f && c.decay > 0.9995f) c.freq = std::max (45.0f, c.freq * 0.9996f);   // bumbo: queda de pitch
            c.env *= c.decay;
            L[i] += s; R[i] += s;
        }
    }
}

//==============================================================================
void HornsProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) return;
    if ((int) bufL.size() < n) { bufL.resize ((size_t) n); bufR.resize ((size_t) n); }

    sched.clear();

    // MIDI da DAW: notas, sustain (CC64) = dub throw, all notes off
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int s = jlimit (0, n - 1, meta.samplePosition);
        if (m.isNoteOnOrOff()) keyboardState.processNextMidiEvent (m);
        if (m.isNoteOn())
        {
            sched.push_back ({ s, 1, m.getNoteNumber(), m.getFloatVelocity(), horns::Ten, 0 });
            pickFifo.write (1).forEach ([&] (int i) { pickBuf[(size_t) i] = m.getNoteNumber(); });
        }
        else if (m.isNoteOff())                           sched.push_back ({ s, 0, m.getNoteNumber(), 0.0f, 0, 0 });
        else if (m.isSustainPedalOn())                    cc64Throw = true;
        else if (m.isSustainPedalOff())                   cc64Throw = false;
        else if (m.isAllNotesOff() || m.isAllSoundOff())  sched.push_back ({ s, 2, 0, 0.0f, 0, 0 });
    }
    // teclado na tela: injeta as notas clicadas (as da DAW acendem as teclas acima)
    MidiBuffer kb;
    keyboardState.processNextMidiBuffer (kb, 0, n, true);
    for (const auto meta : kb)
    {
        const auto m = meta.getMessage();
        const int s = jlimit (0, n - 1, meta.samplePosition);
        if (m.isNoteOn())       sched.push_back ({ s, 1, m.getNoteNumber(), m.getFloatVelocity(), horns::Ten, 3 });
        else if (m.isNoteOff()) sched.push_back ({ s, 0, m.getNoteNumber(), 0.0f, 0, 3 });
    }
    // prévia de notas escolhidas no editor
    previewFifo.read (previewFifo.getNumReady()).forEach ([&] (int i)
    {
        const auto [note, artic] = previewBuf[(size_t) i];
        sched.push_back ({ 0, 1, note, 0.85f, artic, 2 });
        for (auto& o : previewOffs)
            if (o.note < 0) { o.note = note; o.samplesLeft = (int) (sr * (artic == horns::Stac ? 0.12 : 0.4)); break; }
    });
    for (auto& o : previewOffs)
        if (o.note >= 0)
        {
            if (o.samplesLeft < n) { sched.push_back ({ std::max (0, o.samplesLeft), 0, o.note, 0.0f, 0, 2 }); o.note = -1; }
            else o.samplesLeft -= n;
        }

    // transporte
    double bpm = 0.0, ppq = 0.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())         bpm = *b;
            if (auto p = pos->getPpqPosition()) ppq = *p;
            playing = pos->getIsPlaying();
        }
    hostPlaying.store (playing);
    if (bpm > 0.0) hostBpm.store (bpm);

    const bool phraseOn = param ("phrasePlay") > 0.5f;
    bool internal = false;
    if (! playing && phraseOn) { bpm = (double) param ("bpm"); ppq = internalPpq; internal = true; }
    if (! phraseOn || playing) internalPpq = 0.0;
    const double bpmEff = bpm > 0.0 ? bpm : getBpmForDisplay();

    horns::Params P;
    fillParams (P, bpmEff);
    P.throwOn = P.throwOn || cc64Throw;
    engine.setParams (P);

    // copia a frase editada (sem travar o áudio)
    if (phraseVersion.load() != phraseVersionAudio)
    {
        const SpinLock::ScopedTryLockType tl (phraseLock);
        if (tl.isLocked())
        {
            phraseAudio.name = phraseShared.name;
            phraseAudio.length = phraseShared.length;
            phraseAudio.events = phraseShared.events;
            phraseVersionAudio = phraseVersion.load();
        }
    }

    const double loopBeats = phraseAudio.length * 0.25;
    const bool phrasePlaying = phraseOn && (playing || internal) && loopBeats > 0.0;
    if (! phrasePlaying)
    {
        if (wasPlaying) sched.push_back ({ 0, 2, 0, 0.0f, 0, 1 });
        wasPlaying = false;
        playStep.store (-1);
    }
    else
    {
        if (wasPlaying && std::abs (ppq - expectedPpq) > 1.0e-3)   // salto no transporte / loop da DAW
            sched.push_back ({ 0, 2, 0, 0.0f, 0, 1 });

        const double blockBeats = (double) n / sr * bpmEff / 60.0;
        const double end = ppq + blockBeats;
        const float vel = param ("phraseVel");
        const double swing = param ("swing");

        auto collect = [&] (double t, int type, int note, int artic)
        {
            double occ = t + std::ceil ((ppq - t) / loopBeats) * loopBeats;
            while (occ < end)
            {
                if (occ >= ppq)
                    sched.push_back ({ jlimit (0, n - 1, (int) ((occ - ppq) / blockBeats * n)), type, note, vel, artic, 1 });
                occ += loopBeats;
            }
        };
        for (const auto& e : phraseAudio.events)
        {
            if (e.note < 0 || e.note > 127) continue;
            const double on = e.start * 0.25 + ((e.start & 1) ? swing * 0.25 * 0.45 : 0.0);
            const double len = e.dur * 0.25 * (e.artic == horns::Stac ? 0.45 : 0.9);
            collect (on, 1, e.note, e.artic);
            collect (on + len, 0, e.note, e.artic);
        }

        // guia one drop no clock interno: bumbo e aro juntos no 2 e no 4 (contagem da Dub Station)
        if (internal && param ("guide") > 0.5f)
        {
            for (double occ = std::ceil (ppq * 4.0 - 1.0e-9) / 4.0; occ < end; occ += 0.25)
            {
                const int s16 = (int) std::llround (occ * 4.0) % 16;
                const bool accent = s16 == 4 || s16 == 12, tick = (s16 % 4) == 0 || (s16 % 4) == 2;
                if (! accent && ! tick) continue;
                const int at = jlimit (0, n - 1, (int) ((occ - ppq) / blockBeats * n));
                auto fire = [&] (float freq, float env, float decaySec)
                {
                    for (auto& c : clicks)
                        if (c.env < 1.0e-4f && c.phase >= 0.0f)
                        { c.freq = freq; c.env = env; c.decay = std::exp (-1.0f / (decaySec * (float) sr)); c.phase = (float) -at; return; }
                };
                if (accent) { fire (2000.0f, 0.5f, 0.012f); fire (110.0f, 1.2f, 0.09f); }
                else        fire ((s16 % 4) == 0 ? 3000.0f : 5200.0f, (s16 % 4) == 0 ? 0.14f : 0.08f, 0.006f);
            }
        }

        playStep.store ((int) (std::fmod (std::max (0.0, ppq), loopBeats) / 0.25));
        wasPlaying = true;
        expectedPpq = end;
        if (internal) internalPpq = end;
    }

    // offs e "todas off" antes dos ons na mesma amostra
    std::stable_sort (sched.begin(), sched.end(), [] (const SchedEv& a, const SchedEv& b)
    {
        if (a.sample != b.sample) return a.sample < b.sample;
        return (a.type == 1 ? 1 : 0) < (b.type == 1 ? 1 : 0);
    });

    int pos = 0;
    for (const auto& e : sched)
    {
        const int at = jlimit (0, n, e.sample);
        if (at > pos) { engine.render (bufL.data() + pos, bufR.data() + pos, at - pos); pos = at; }
        if (e.type == 1)
        {
            if (e.note < 0 || e.note > 127) continue;
            if (e.src == 1) { if (activePhrase[e.note]) engine.noteOff (e.note, 1); activePhrase[e.note] = true; }
            engine.noteOn (e.note, e.vel, e.artic, e.src);
        }
        else if (e.type == 0)
        {
            if (e.note < 0 || e.note > 127) continue;
            if (e.src == 1) { if (! activePhrase[e.note]) continue; activePhrase[e.note] = false; }
            engine.noteOff (e.note, e.src);
        }
        else
        {
            engine.allNotesOff (e.src == 0 ? -1 : e.src);
            if (e.src == 1 || e.src == 0) std::fill (std::begin (activePhrase), std::end (activePhrase), false);
        }
    }
    if (pos < n) engine.render (bufL.data() + pos, bufR.data() + pos, n - pos);
    renderClicks (bufL.data(), bufR.data(), n);

    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += 0.5f * (bufL[(size_t) i] * bufL[(size_t) i] + bufR[(size_t) i] * bufR[(size_t) i]);
    vuLevel.store (std::sqrt (sum / (float) n));
    echoMsNow.store (engine.currentEchoMs());

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, bufL.data(), n);
        buffer.copyFrom (1, 0, bufR.data(), n);
    }
    else if (buffer.getNumChannels() == 1)
    {
        for (int i = 0; i < n; ++i) bufL[(size_t) i] = 0.5f * (bufL[(size_t) i] + bufR[(size_t) i]);
        buffer.copyFrom (0, 0, bufL.data(), n);
    }
}

//==============================================================================
bool HornsProcessor::writeMidiFile (const File& file) const
{
    const auto ph = getPhrase();
    bool any = false;
    for (const auto& e : ph.events) any = any || e.note >= 0;
    if (! any) return false;

    const int tpq = 960;
    const double bpm = getBpmForDisplay();
    const double swing = param ("swing");
    const int vel = jlimit (1, 127, roundToInt (param ("phraseVel") * 127.0f));
    const auto P = currentParams();

    MidiMessageSequence meta;
    meta.addEvent (MidiMessage::tempoMetaEvent (roundToInt (60000000.0 / bpm)), 0.0);
    meta.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    meta.addEvent (MidiMessage::textMetaEvent (6, String (ph.name) + " - " + horns::noteName (P.key) + " " + horns::scales()[(size_t) P.scale].name), 0.0);

    auto build = [&] (const String& name, int channel, int inst) -> MidiMessageSequence
    {
        MidiMessageSequence seq;
        seq.addEvent (MidiMessage::textMetaEvent (3, name), 0.0);
        for (const auto& e : ph.events)
        {
            if (e.note < 0) continue;
            const int note = inst < 0 ? e.note
                                      : horns::sectionNote (e.note, P.inst[(size_t) inst].harm, P.inst[(size_t) inst].oct, inst, P.key, P.scale);
            const double on = e.start * 0.25 + ((e.start & 1) ? swing * 0.25 * 0.45 : 0.0);
            const double len = e.dur * 0.25 * (e.artic == horns::Stac ? 0.45 : 0.9);
            seq.addEvent (MidiMessage::noteOn (channel, note, (uint8) vel), on * tpq);
            seq.addEvent (MidiMessage::noteOff (channel, note), (on + len) * tpq);
        }
        seq.sort();
        seq.updateMatchedPairs();
        return seq;
    };

    MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (meta);
    mf.addTrack (build ("Mastor Dub Horns - frase (melodia)", 1, -1));
    for (int i = 0; i < horns::kNumInst; ++i)
        if (P.inst[(size_t) i].on)
            mf.addTrack (build ("Mastor Dub Horns - " + String (horns::instruments()[(size_t) i].name).toLowerCase(), i + 2, i));

    file.getParentDirectory().createDirectory();
    file.deleteFile();
    FileOutputStream out (file);
    if (! out.openedOk()) return false;
    return mf.writeTo (out);
}

//==============================================================================
void HornsProcessor::getStateInformation (MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.removeChild (st.getChildWithName ("PHRASE"), nullptr);
    const auto ph = getPhrase();
    ValueTree pt ("PHRASE");
    pt.setProperty ("name", String::fromUTF8 (ph.name.c_str()), nullptr);
    pt.setProperty ("length", ph.length, nullptr);
    for (const auto& e : ph.events)
    {
        ValueTree ev ("EV");
        ev.setProperty ("s", e.start, nullptr);
        ev.setProperty ("d", e.dur, nullptr);
        ev.setProperty ("n", e.note, nullptr);
        ev.setProperty ("a", e.artic, nullptr);
        ev.setProperty ("g", e.degree, nullptr);
        pt.appendChild (ev, nullptr);
    }
    st.appendChild (pt, nullptr);
    st.setProperty ("version", MASTOR_HORNS_VERSION, nullptr);
    if (auto xml = st.createXml())
        copyXmlToBinary (*xml, destData);
}

void HornsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto st = ValueTree::fromXml (*xml);
    const auto pt = st.getChildWithName ("PHRASE");
    if (pt.isValid())
    {
        horns::Phrase ph;
        ph.name = pt.getProperty ("name").toString().toStdString();
        ph.length = (int) pt.getProperty ("length", 32);
        for (const auto& ev : pt)
        {
            if ((int) ph.events.size() >= horns::kMaxEvents) break;
            horns::PhraseEvent e;
            e.start = (int) ev.getProperty ("s", 0);
            e.dur = (int) ev.getProperty ("d", 2);
            e.note = (int) ev.getProperty ("n", -1);
            e.artic = (int) ev.getProperty ("a", 0);
            e.degree = (int) ev.getProperty ("g", 0);
            ph.events.push_back (e);
        }
        setPhrase (ph);
        st.removeChild (pt, nullptr);
    }
    apvts.replaceState (st);
}

AudioProcessorEditor* HornsProcessor::createEditor() { return new HornsEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HornsProcessor(); }
