#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DMSamples.h"
#include "GroovePresets.h"
#include "../../RiddimBank/Source/RiddimBank.h"

namespace
{
    juce::String u8s (const char* s) { return juce::String::fromUTF8 (s); }

    // parâmetros que mudam o padrão (os outros só mudam o som ou a saída)
    const char* const kParamIds[] = { "style", "bars", "hatPattern", "swing", "swingAll", "dynamics", "drummer", "backbeat", "kit", "kickBeats", "snareBeats",
                                      "rndKick", "rndSnare", "rndHat", "rndPerc", "fill", "intro", "seed" };
    constexpr int kTicksPerQuarter = 960;
}

const DubDrumsProcessor::Piece DubDrumsProcessor::pieces[DubDrumsProcessor::numPieces] =
{
    { "kick",  "Bumbo",      dubdrums::gm::Kick },
    { "side",  "Aro",        dubdrums::gm::SideStick },
    { "snare", "Caixa",      dubdrums::gm::Snare },
    { "hhc",   "HH fechado", dubdrums::gm::ClosedHat },
    { "hho",   "HH aberto",  dubdrums::gm::OpenHat },
    { "tom1",  "Tom 1",      dubdrums::gm::TomHigh },
    { "tom2",  "Tom 2",      dubdrums::gm::TomMid },
    { "tom3",  "Surdo",      dubdrums::gm::TomLow },
    { "crash", "Prato",      dubdrums::gm::Crash },
};

juce::AudioProcessorValueTreeState::ParameterLayout DubDrumsProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto choice = [&] (const char* id, const char* name, std::initializer_list<const char*> items, int def)
    {
        StringArray list;
        for (auto* item : items) list.add (u8s (item));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, u8s (name), list, def));
    };
    auto percent = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, u8s (name),
                                                           NormalisableRange<float> (0.0f, 100.0f, 1.0f), def,
                                                           AudioParameterFloatAttributes().withLabel ("%")));
    };

    choice ("style", "Estilo", { "Rockers", "One Drop", "Boom Bap", "Dembow" }, 0);
    // bumbo e caixa principais por colcheia (8 posições, bit 0 = "1"); o estilo carrega o padrão dele
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "kickBeats", 1 },  "Bumbo (colcheias)", 0, 255,
                                                     dubdrums::defaultKickMask (dubdrums::Style::Rockers)));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "snareBeats", 1 }, "Caixa (colcheias)", 0, 255,
                                                     dubdrums::defaultSnareMask (dubdrums::Style::Rockers)));
    choice ("bars", "Compassos", { "1", "2", "3", "4" }, 3);
    choice ("hatPattern", "Hi-hat", { "Colcheias", "Semicolcheias", "Contratempo", "Tercinas", "Flying cymbal", "Abre no 1" }, 1);   // preset inicial: 1/16
    choice ("swing", "Swing do hi-hat",
            { "Reto", "Leve (54%)", "Médio (58%)", "Pesado (62%)", "Shuffle (tercina)" }, 0);
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "swingAll", 1 }, "Swing em tudo", true));
    choice ("dynamics", "Dinâmica",
            { "Suave", "Média", "Forte", "Acentuada", "Crescendo", "Respirando" }, 1);

    percent ("rndKick",  "Aleatoriedade bumbo",  15.0f);
    percent ("rndSnare", "Aleatoriedade caixa",  25.0f);
    percent ("rndHat",   "Aleatoriedade hi-hat", 30.0f);
    percent ("rndPerc",  "Aleatoriedade tons",   20.0f);

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "fill", 1 }, u8s ("Virada no último compasso"), false));
    layout.add (std::make_unique<AudioParameterInt>  (ParameterID { "seed", 1 }, "Semente", 1, 9999, 1));
    layout.add (std::make_unique<AudioParameterInt>  (ParameterID { "channel", 1 }, "Canal MIDI", 1, 16, 10));

    // 1.0: baterista, introdução, preview / som interno e mapa de notas
    choice ("drummer", "Baterista", { "Style Scott", "Santa Davis", "Carlton Barrett", "Sly Dunbar", "Digikal" }, 0);   // preset inicial: Style Scott
    choice ("backbeat", "Batida principal", { "Caixa", "Aro", "Caixa + aro" }, 0);
    choice ("kit", "Kit", { "Anos 70 (samples DM)", "Digital (King Jammy / Sleng Teng)" }, 0);
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "intro", 1 }, u8s ("Virada de introdução"), false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "preview", 1 }, "Preview", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "colorMidi", 1 }, "MIDI colorido (um canal por compasso)", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "sound", 1 }, "Som interno", true));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM do preview",
                                                       NormalisableRange<float> (50.0f, 180.0f, 1.0f), 75.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume",
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    auto noteName = [] (int v, int) { return MidiMessage::getMidiNoteName (v, true, true, 3) + " (" + String (v) + ")"; };
    for (const auto& pc : pieces)
        layout.add (std::make_unique<AudioParameterInt> (ParameterID { String ("note_") + pc.id, 1 },
                                                         u8s ("Nota ") + u8s (pc.name), 0, 127, pc.gmNote,
                                                         AudioParameterIntAttributes().withStringFromValueFunction (noteName)));

    return layout;
}

DubDrumsProcessor::DubDrumsProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DubDrums", createLayout())
{
    for (auto* id : kParamIds)
        apvts.addParameterListener (id, this);

    loadSamples();
    regenerate();
    startTimerHz (20);
}

// Kit SAMPLES DM: WAVs embutidos (Source/DMSamples.h). Mono, sem o silêncio do começo, normalizados.
void DubDrumsProcessor::loadSamples()
{
    struct S { int note; const unsigned char* data; size_t size; float gain; };
    using namespace dubdrums;
    const S list[] = {
        { gm::Kick,      dmsamples::kick,    dmsamples::kickSize,    1.50f },
        { gm::Snare,     dmsamples::snare,   dmsamples::snareSize,   1.25f },
        { gm::SideStick, dmsamples::rim,     dmsamples::rimSize,     1.00f },
        { gm::ClosedHat, dmsamples::hat,     dmsamples::hatSize,     0.72f },
        { gm::OpenHat,   dmsamples::openHat, dmsamples::openHatSize, 0.66f },
    };
    juce::WavAudioFormat wav;
    for (const auto& it : list)
    {
        std::unique_ptr<juce::AudioFormatReader> r (wav.createReaderFor (new juce::MemoryInputStream (it.data, it.size, false), true));
        if (r == nullptr || r->lengthInSamples <= 0) continue;
        const int n = (int) r->lengthInSamples, chs = (int) r->numChannels;
        juce::AudioBuffer<float> buf (chs, n);
        r->read (&buf, 0, n, 0, true, true);
        std::vector<float> mono ((size_t) n);
        float peak = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            float v = 0.0f;
            for (int c = 0; c < chs; ++c) v += buf.getSample (c, i);
            mono[(size_t) i] = v / (float) chs;
            peak = std::max (peak, std::abs (mono[(size_t) i]));
        }
        if (peak <= 0.0f) continue;
        int start = 0;                                   // corta o silêncio antes do ataque
        while (start < n && std::abs (mono[(size_t) start]) < peak * 0.02f) ++start;
        start = std::max (0, start - 8);
        std::vector<float> data (mono.begin() + start, mono.end());
        for (auto& v : data) v /= peak;
        const int fade = std::min ((int) data.size(), 64);  // final sem clique
        for (int i = 0; i < fade; ++i) data[data.size() - 1 - (size_t) i] *= (float) i / (float) fade;
        synth.setSample (it.note, std::move (data), r->sampleRate, it.gain);
    }
}

DubDrumsProcessor::~DubDrumsProcessor()
{
    stopTimer();
    for (auto* id : kParamIds)
        apvts.removeParameterListener (id, this);
}

bool DubDrumsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void DubDrumsProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0 ? sampleRate : 44100.0;
    wasPlaying = wasPreviewing = false;
    held.fill (false);
    synth.prepare (sr);
}

// ------------------------------------------------------------------ parâmetros -> padrão
void DubDrumsProcessor::parameterChanged (const juce::String&, float)
{
    dirty = true; // pode vir da thread de áudio (automação): só marca, gera depois na thread da interface
}

void DubDrumsProcessor::timerCallback()
{
    if (dirty.exchange (false))
        regenerate();
}

dubdrums::Settings DubDrumsProcessor::readSettings() const
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    dubdrums::Settings s;
    s.style      = (dubdrums::Style)      juce::roundToInt (get ("style"));
    s.bars       = juce::roundToInt (get ("bars")) + 1;
    s.hatPattern = (dubdrums::HatPattern) juce::roundToInt (get ("hatPattern"));
    s.swing      = (dubdrums::Swing)      juce::roundToInt (get ("swing"));
    s.swingAll   = get ("swingAll") > 0.5f;
    s.dynamics   = (dubdrums::Dynamics)   juce::roundToInt (get ("dynamics"));
    s.randKick   = get ("rndKick")  / 100.0f;
    s.randSnare  = get ("rndSnare") / 100.0f;
    s.randHat    = get ("rndHat")   / 100.0f;
    s.randPerc   = get ("rndPerc")  / 100.0f;
    s.fill       = get ("fill") > 0.5f;
    s.intro      = get ("intro") > 0.5f;
    s.drummer    = (dubdrums::Drummer)    juce::roundToInt (get ("drummer"));
    s.backbeat   = (dubdrums::Backbeat)   juce::roundToInt (get ("backbeat"));
    s.machine    = juce::roundToInt (get ("kit")) == 1;
    s.kickMask   = (uint8_t) juce::jlimit (0, 255, juce::roundToInt (get ("kickBeats")));
    s.snareMask  = (uint8_t) juce::jlimit (0, 255, juce::roundToInt (get ("snareBeats")));
    s.seed       = (uint32_t) juce::roundToInt (get ("seed"));
    s.strict     = strictReggae.load();
    return s;
}

namespace
{
    const juce::Identifier kEdits ("EDITS"), kNote ("N");

    juce::ValueTree patternToTree (const dubdrums::Pattern& p, const juce::String& key)
    {
        juce::ValueTree t (kEdits);
        t.setProperty ("key", key, nullptr);
        t.setProperty ("len", p.lengthInBeats, nullptr);
        t.setProperty ("introLen", p.introLengthInBeats, nullptr);
        auto add = [&] (const dubdrums::NoteEvent& e, bool intro)
        {
            juce::ValueTree n (kNote);
            n.setProperty ("b", e.beat, nullptr);   n.setProperty ("l", e.length, nullptr);
            n.setProperty ("n", e.note, nullptr);   n.setProperty ("v", e.velocity, nullptr);
            n.setProperty ("i", intro, nullptr);
            t.appendChild (n, nullptr);
        };
        for (auto& e : p.intro)  add (e, true);
        for (auto& e : p.events) add (e, false);
        return t;
    }

    dubdrums::Pattern treeToPattern (const juce::ValueTree& t)
    {
        dubdrums::Pattern p;
        p.lengthInBeats = juce::jlimit (4.0, 16.0, (double) t.getProperty ("len", 4.0));
        p.introLengthInBeats = juce::jlimit (0.0, 4.0, (double) t.getProperty ("introLen", 0.0));
        for (const auto& n : t)
        {
            dubdrums::NoteEvent e;
            const bool intro = (bool) n.getProperty ("i", false);
            const double lim = intro ? p.introLengthInBeats : p.lengthInBeats;
            e.beat = (double) n.getProperty ("b", 0.0);
            if (e.beat < 0.0 || e.beat >= lim) continue;
            e.length   = juce::jlimit (0.01, 4.0, (double) n.getProperty ("l", 0.1));
            e.note     = juce::jlimit (0, 127, (int) n.getProperty ("n", 36));
            e.velocity = juce::jlimit (1, 127, (int) n.getProperty ("v", 100));
            e.bar      = intro ? -1 : (int) std::floor (e.beat / 4.0);
            (intro ? p.intro : p.events).push_back (e);
        }
        auto byBeat = [] (const dubdrums::NoteEvent& a, const dubdrums::NoteEvent& b) { return a.beat < b.beat; };
        std::stable_sort (p.intro.begin(), p.intro.end(), byBeat);
        std::stable_sort (p.events.begin(), p.events.end(), byBeat);
        return p;
    }
}

// Tudo que muda o padrão gerado (as edições na grade valem só para esta combinação)
juce::String DubDrumsProcessor::settingsKey() const
{
    const auto s = readSettings();
    juce::String k;
    k << (int) s.style << "/" << (int) s.kickMask << "/" << (int) s.snareMask << "/" << s.bars << "/" << (int) s.hatPattern
      << "/" << (int) s.swing << "/" << (int) s.swingAll << "/" << (int) s.dynamics << "/" << juce::roundToInt (s.randKick * 100.0f)
      << "/" << juce::roundToInt (s.randSnare * 100.0f) << "/" << juce::roundToInt (s.randHat * 100.0f) << "/" << juce::roundToInt (s.randPerc * 100.0f)
      << "/" << (int) s.drummer << "/" << (int) s.backbeat << "/" << (int) s.machine << "/" << (int) s.fill << "/" << (int) s.intro << "/" << (int) s.seed;
    if (s.strict) k << "/strict";
    return k;
}

void DubDrumsProcessor::regenerate()
{
    generated = dubdrums::generate (readSettings());
    auto edits = apvts.state.getChildWithName (kEdits);
    if (edits.isValid() && edits.getProperty ("key").toString() == settingsKey())
    {
        publish (treeToPattern (edits));      // projeto salvo com edições: volta como estava
        return;
    }
    if (edits.isValid())
        apvts.state.removeChild (edits, nullptr);   // mexeu num controle do gerador: as edições saem
    publish (generated);
}

void DubDrumsProcessor::publish (const dubdrums::Pattern& p)
{
    {
        const juce::SpinLock::ScopedLockType lock (displayLock);
        display = p;
    }
    auto copy = p;   // aloca aqui, na thread da interface
    {
        const juce::SpinLock::ScopedLockType lock (pendingLock);
        std::swap (pending, copy);
        pendingReady = true;
    }
    ++patternVersion;
    // copy (o pendente antigo) é liberado aqui, fora da thread de áudio
}

dubdrums::Pattern DubDrumsProcessor::getPatternCopy() const
{
    const juce::SpinLock::ScopedLockType lock (displayLock);
    return display;
}

void DubDrumsProcessor::storeEdits (const dubdrums::Pattern& p)
{
    auto old = apvts.state.getChildWithName (kEdits);
    if (old.isValid()) apvts.state.removeChild (old, nullptr);
    apvts.state.appendChild (patternToTree (p, settingsKey()), nullptr);
    publish (p);
}

bool DubDrumsProcessor::hasEdits() const   { return apvts.state.getChildWithName (kEdits).isValid(); }

void DubDrumsProcessor::clearEdits()
{
    auto old = apvts.state.getChildWithName (kEdits);
    if (! old.isValid()) return;
    apvts.state.removeChild (old, nullptr);
    publish (generated);
}

//==============================================================================
// GROOVES: presets de músicas de cada baterista
namespace
{
    // valores "de verdade" de cada controle no groove (escolhas pelo índice; bars é o número de compassos)
    std::vector<std::pair<const char*, float>> grooveValues (const dubdrums::GroovePreset& g)
    {
        return { { "drummer", (float) g.drummer }, { "style", (float) g.style }, { "kickBeats", (float) g.kick },
                 { "snareBeats", (float) g.snare }, { "backbeat", (float) g.backbeat }, { "hatPattern", (float) g.hat },
                 { "swing", (float) g.swing }, { "swingAll", g.swingAll ? 1.0f : 0.0f }, { "dynamics", (float) g.dyn },
                 { "rndKick", (float) g.rndKick }, { "rndSnare", (float) g.rndSnare }, { "rndHat", (float) g.rndHat },
                 { "rndPerc", (float) g.rndPerc }, { "fill", g.fill ? 1.0f : 0.0f }, { "intro", g.intro ? 1.0f : 0.0f },
                 { "kit", (float) g.kit }, { "bars", (float) (g.bars - 1) }, { "seed", (float) g.seed }, { "bpm", (float) g.bpm } };
    }
}

void DubDrumsProcessor::loadGroove (int index)
{
    if (! juce::isPositiveAndBelow (index, dubdrums::numGroovePresets)) return;
    for (auto [id, value] : grooveValues (dubdrums::groovePresets[index]))
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            p->endChangeGesture();
        }
    apvts.state.setProperty ("groove", index, nullptr);
    apvts.state.setProperty ("bank", -1, nullptr);
    clearEdits();
    dirty = true;
}

void DubDrumsProcessor::applyBankEntry (int index)
{
    if (! juce::isPositiveAndBelow (index, riddimbank::numEntries)) return;
    const auto& e = riddimbank::entries[index];
    loadGroove (e.groove);
    if (auto* p = apvts.getParameter ("bpm"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) e.bpm));
        p->endChangeGesture();
    }
    apvts.state.setProperty ("bank", index, nullptr);
}

int DubDrumsProcessor::getBankEntry() const
{
    const int b = apvts.state.getProperty ("bank", -1);
    return juce::isPositiveAndBelow (b, riddimbank::numEntries) ? b : -1;
}

int DubDrumsProcessor::getGroove() const
{
    const int g = apvts.state.getProperty ("groove", -1);
    return juce::isPositiveAndBelow (g, dubdrums::numGroovePresets) ? g : -1;
}

bool DubDrumsProcessor::grooveIntact() const
{
    const int g = getGroove();
    if (g < 0) return false;
    for (auto [id, value] : grooveValues (dubdrums::groovePresets[g]))
        if (auto* p = apvts.getRawParameterValue (id))
            if (std::abs (p->load() - value) > 0.5f) return false;
    return true;
}

namespace
{
    // nota da peça perto da posição clicada (a nota desenhada ocupa ~2/3 da semicolcheia)
    dubdrums::NoteEvent* findNote (dubdrums::Pattern& p, int gmNote, double beat)
    {
        dubdrums::NoteEvent* best = nullptr;
        double bestD = 0.17;
        auto scan = [&] (std::vector<dubdrums::NoteEvent>& evs, double offset)
        {
            for (auto& e : evs)
            {
                if (e.note != gmNote) continue;
                const double d = std::abs (beat - (offset + e.beat + 0.085));
                if (d < bestD) { bestD = d; best = &e; }
            }
        };
        scan (p.intro, 0.0);
        scan (p.events, p.introLengthInBeats);
        return best;
    }

    int defaultVelocity (int gmNote, double beatInBar)
    {
        using namespace dubdrums;
        const double frac = beatInBar - std::floor (beatInBar);
        switch (gmNote)
        {
            case gm::Kick:      return 108;
            case gm::Snare:     return 108;
            case gm::SideStick: return 100;
            case gm::ClosedHat: return frac < 0.01 ? 80 : std::abs (frac - 0.5) < 0.01 ? 98 : 58;
            case gm::OpenHat:   return 98;
            case gm::Crash:     return 104;
            default:            return 96;
        }
    }
}

bool DubDrumsProcessor::noteAt (int gmNote, double beat) const
{
    auto p = getPatternCopy();
    return findNote (p, gmNote, beat) != nullptr;
}

bool DubDrumsProcessor::toggleNote (int gmNote, double beat, bool add)
{
    auto p = getPatternCopy();
    const double total = p.introLengthInBeats + p.lengthInBeats;
    if (beat < 0.0 || beat >= total) return false;

    if (auto* e = findNote (p, gmNote, beat))
    {
        if (add) return false;
        auto erase = [&] (std::vector<dubdrums::NoteEvent>& evs)
        {
            for (auto it = evs.begin(); it != evs.end(); ++it)
                if (&*it == e) { evs.erase (it); return true; }
            return false;
        };
        if (! erase (p.intro)) erase (p.events);
    }
    else
    {
        if (! add) return false;
        const double snapped = std::floor (beat * 4.0) / 4.0;   // cai na semicolcheia clicada
        const bool intro = snapped < p.introLengthInBeats;
        dubdrums::NoteEvent ne;
        ne.beat = intro ? snapped : snapped - p.introLengthInBeats;
        ne.note = gmNote;
        ne.length = gmNote == dubdrums::gm::OpenHat ? 0.4 : gmNote == dubdrums::gm::Crash ? 1.0 : 0.1;
        ne.velocity = defaultVelocity (gmNote, ne.beat);
        ne.bar = intro ? -1 : (int) std::floor (ne.beat / 4.0);
        auto& evs = intro ? p.intro : p.events;
        evs.insert (std::upper_bound (evs.begin(), evs.end(), ne,
                                      [] (const dubdrums::NoteEvent& a, const dubdrums::NoteEvent& b) { return a.beat < b.beat; }), ne);
    }
    storeEdits (p);
    return true;
}

void DubDrumsProcessor::changeVelocity (int gmNote, double beat, int delta)
{
    auto p = getPatternCopy();
    if (auto* e = findNote (p, gmNote, beat))
    {
        const int v = juce::jlimit (1, 127, e->velocity + delta);
        if (v == e->velocity) return;
        e->velocity = v;
        storeEdits (p);
    }
}

void DubDrumsProcessor::applyStyleDefaults (int styleIndex)
{
    const auto st = (dubdrums::Style) juce::jlimit (0, 3, styleIndex);
    auto set = [this] (const char* id, int v)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) v));
            p->endChangeGesture();
        }
    };
    set ("kickBeats",  dubdrums::defaultKickMask (st));
    set ("snareBeats", dubdrums::defaultSnareMask (st));
}

void DubDrumsProcessor::newVariation()
{
    if (auto* p = apvts.getParameter ("seed"))
    {
        const int newSeed = juce::Random::getSystemRandom().nextInt ({ 1, 10000 });
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) newSeed));
        p->endChangeGesture();
    }
    dirty = false;
    regenerate();              // gera já, para o preview recomeçar com o padrão novo
    restartRequested = true;
}

// ------------------------------------------------------------------ áudio / MIDI
int DubDrumsProcessor::mapNote (int gmNote) const
{
    for (const auto& pc : pieces)
        if (pc.gmNote == gmNote)
            return juce::jlimit (0, 127, juce::roundToInt (apvts.getRawParameterValue (juce::String ("note_") + pc.id)->load()));
    return gmNote;
}

int DubDrumsProcessor::unmapNote (int outNote) const
{
    for (const auto& pc : pieces)
        if (mapNote (pc.gmNote) == outNote) return pc.gmNote;
    return outNote;
}

// Linha do tempo: [intro (toca uma vez)] [loop] [loop] [loop] ...
template <typename Fn>
void DubDrumsProcessor::forEachEventIn (const dubdrums::Pattern& pattern, double start, double end, Fn&& fn)
{
    const double intro = pattern.introLengthInBeats, loopLen = pattern.lengthInBeats;
    for (int pass = 0; pass < 2; ++pass)      // note-offs antes dos note-ons
    {
        const bool off = pass == 0;
        for (const auto& e : pattern.intro)
        {
            const double t = e.beat + (off ? e.length : 0.0);
            if (t >= start && t < end) fn (t, e, off);
        }
        if (loopLen <= 0.0) continue;
        for (const auto& e : pattern.events)
        {
            const double t0 = intro + e.beat + (off ? e.length : 0.0);
            const double k = std::max (0.0, std::ceil ((start - t0) / loopLen));
            for (double t = t0 + k * loopLen; t < end; t += loopLen)
                if (t >= start) fn (t, e, off);
        }
    }
}

void DubDrumsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    midi.clear();   // a entrada MIDI não é usada; a saída é só a bateria

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0) return;
    const int channel = juce::jlimit (1, 16, juce::roundToInt (apvts.getRawParameterValue ("channel")->load()));
    const bool soundOn   = apvts.getRawParameterValue ("sound")->load() > 0.5f;
    const bool previewOn = apvts.getRawParameterValue ("preview")->load() > 0.5f;
    const float volume   = apvts.getRawParameterValue ("volume")->load();
    const int kitIndex   = juce::roundToInt (apvts.getRawParameterValue ("kit")->load());
    synth.setKit (kitIndex == 1 ? dubdrums::DrumSynth::Kit::Digital : dubdrums::DrumSynth::Kit::Acoustic);

    bool playing = false;
    double ppq = 0.0, bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) ppq = *p; else playing = false;
            if (auto b = pos->getBpm())         bpm = *b;
        }
    hostPlaying = playing;

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    // troca o padrão tocando pelo pendente (nunca espera a interface: se ela estiver escrevendo, tenta no próximo bloco)
    auto adoptPending = [this]() -> bool
    {
        if (! pendingReady.load()) return false;
        const juce::SpinLock::ScopedTryLockType lock (pendingLock);
        if (! lock.isLocked()) return false;
        std::swap (active, pending);
        pendingReady = false;
        return true;
    };

    // PREVIEW: com a DAW parada, o plugin toca sozinho no BPM do painel, começando pela intro
    const bool preview = previewOn && ! playing;
    bool starting = (preview && ! wasPreviewing) || (playing && ! wasPlaying);
    if (restartRequested.exchange (false) && preview && wasPreviewing)
    {
        // NOVA VARIAÇÃO com o preview tocando: para tudo e o loop novo começa do início
        previewPos = 0.0;
        synth.stopAll();
        starting = true;
    }
    if (preview)
    {
        if (! wasPreviewing) { previewPos = 0.0; synth.stopAll(); }
        ppq = previewPos;
        bpm = apvts.getRawParameterValue ("bpm")->load();
    }
    else if (wasPreviewing)
        synth.stopAll();
    wasPreviewing = preview;
    previewing = preview;

    if (playing != wasPlaying && ! playing)
    {
        midi.addEvent (juce::MidiMessage::allNotesOff (channel), 0);
        held.fill (false);
    }
    wasPlaying = playing;

    if (! playing && ! preview)
    {
        adoptPending();   // parado: troca na hora
        playheadBeat = -1.0;
        synth.render (left, right, 0, numSamples, volume);   // deixa os pratos terminarem
        outLevel = outLevel.load() * 0.8f;
        return;
    }
    if (starting)
        adoptPending();   // começando a tocar: já entra com o padrão novo

    bpm = juce::jlimit (20.0, 400.0, bpm);
    const double blockBeats = (double) numSamples / sr * bpm / 60.0;
    if (preview) previewPos += blockBeats;
    const double blockEnd = ppq + blockBeats;

    struct Trigger { int offset; int note; int vel; };
    std::array<Trigger, 256> triggers;
    int numTriggers = 0;
    const bool makeSound = preview || soundOn;
    auto offsetOf = [&] (double t) { return juce::jlimit (0, numSamples - 1, (int) ((t - ppq) / blockBeats * numSamples)); };

    auto schedule = [&] (double from, double to)
    {
        if (to <= from) return;
        forEachEventIn (active, from, to, [&] (double t, const dubdrums::NoteEvent& e, bool off)
        {
            const int offset = offsetOf (t);
            const int n = mapNote (e.note);
            if (playing)
            {
                if (off)
                {
                    if (held[(size_t) n]) { midi.addEvent (juce::MidiMessage::noteOff (channel, n), offset); held[(size_t) n] = false; }
                }
                else
                {
                    if (held[(size_t) n]) midi.addEvent (juce::MidiMessage::noteOff (channel, n), offset);
                    midi.addEvent (juce::MidiMessage::noteOn (channel, n, (juce::uint8) e.velocity), offset);
                    held[(size_t) n] = true;
                }
            }
            if (makeSound && ! off && numTriggers < (int) triggers.size())
                triggers[(size_t) numTriggers++] = { offset, e.note, e.velocity };
        });
    };

    // Com um padrão novo esperando, toca o antigo até pouco antes da virada do próximo tempo e troca ali.
    // (a margem pega as notas adiantadas do baterista que pertencem ao tempo seguinte)
    constexpr double kMargin = 0.06;
    double from = std::max (0.0, ppq);
    if (pendingReady.load())
    {
        const double switchAt = std::ceil (from + kMargin) - kMargin;
        if (switchAt < blockEnd)
        {
            schedule (from, switchAt);
            if (adoptPending())
            {
                const int at = offsetOf (switchAt);
                for (int n = 0; n < 128; ++n)
                    if (held[(size_t) n]) { midi.addEvent (juce::MidiMessage::noteOff (channel, n), at); held[(size_t) n] = false; }
            }
            from = switchAt;
        }
    }
    schedule (from, blockEnd);

    {
        const double intro = active.introLengthInBeats, loopLen = active.lengthInBeats;
        double pos = std::max (0.0, ppq);
        if (pos >= intro && loopLen > 0.0) pos = intro + std::fmod (pos - intro, loopLen);
        playheadBeat = pos;
    }

    // bateria interna: renderiza em pedaços entre os toques
    std::sort (triggers.begin(), triggers.begin() + numTriggers, [] (const Trigger& a, const Trigger& b) { return a.offset < b.offset; });
    int pos = 0;
    for (int i = 0; i < numTriggers; ++i)
    {
        const int at = triggers[(size_t) i].offset;
        if (at > pos) { synth.render (left, right, pos, at - pos, volume); pos = at; }
        synth.trigger (triggers[(size_t) i].note, triggers[(size_t) i].vel);
    }
    synth.render (left, right, pos, numSamples - pos, volume);

    // limitador suave para nunca estourar
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i) { d[i] = std::tanh (d[i]); peak = std::max (peak, std::abs (d[i])); }
    }
    outLevel = std::max (peak, outLevel.load() * 0.82f);   // nível com queda suave para o cone do alto-falante
}

// ------------------------------------------------------------------ arquivo MIDI
juce::String DubDrumsProcessor::suggestedFileName (int bar) const
{
    const auto s = readSettings();
    juce::String name = juce::String ("MRDM-12_") + dubdrums::styleName (s.style)
                      + "_" + dubdrums::drummerName (s.drummer);
    if (bar == kAllBars) name << "_" << s.bars << "comp" << (s.intro ? "_intro" : "");
    else                 name << (bar < 0 ? juce::String ("_intro") : "_compasso" + juce::String (bar + 1));
    name << "_s" << (int) s.seed;
    return name.replaceCharacter (' ', '-') + ".mid";
}

bool DubDrumsProcessor::writeMidiFile (const juce::File& target, int onlyBar) const
{
    auto p = getPatternCopy();
    if (onlyBar != kAllBars)
    {
        // só um compasso (arrastado pela faixa dele no PADRÃO): vira um arquivo de 4 tempos começando no 0
        std::vector<dubdrums::NoteEvent> one;
        for (auto e : (onlyBar < 0 ? p.intro : p.events))
            if (e.bar == onlyBar) { e.beat -= onlyBar < 0 ? 0.0 : onlyBar * 4.0; one.push_back (e); }
        p.intro.clear();
        p.introLengthInBeats = 0.0;
        p.events = std::move (one);
        p.lengthInBeats = 4.0;
    }
    const int channel = juce::jlimit (1, 16, juce::roundToInt (apvts.getRawParameterValue ("channel")->load()));
    const double intro = p.introLengthInBeats;

    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, onlyBar == kAllBars ? juce::String ("MRDM-12")
                                                     : onlyBar < 0 ? juce::String ("MRDM-12 Intro")
                                                                   : "MRDM-12 Compasso " + juce::String (onlyBar + 1)), 0.0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);

    // marcadores com o nome de cada compasso (aparecem na régua de DAWs que importam marcadores)
    if (onlyBar != kAllBars)
        seq.addEvent (juce::MidiMessage::textMetaEvent (6, onlyBar < 0 ? juce::String ("Intro") : "Compasso " + juce::String (onlyBar + 1)), 0.0);
    else
    {
        if (intro > 0.0)
            seq.addEvent (juce::MidiMessage::textMetaEvent (6, "Intro"), 0.0);
        for (int bar = 0; bar < juce::roundToInt (p.lengthInBeats / 4.0); ++bar)
            seq.addEvent (juce::MidiMessage::textMetaEvent (6, "Compasso " + juce::String (bar + 1)), (intro + bar * 4.0) * kTicksPerQuarter);
    }

    // MIDI colorido (como no Dub Chords): compasso 1 no canal 1 ... compasso 4 no canal 4, intro no canal 5.
    // Na DAW, colorir as notas por canal (Cubase: Key Editor > Cores > Canal) deixa cada compasso de uma cor.
    const bool colorMidi = apvts.getRawParameterValue ("colorMidi")->load() > 0.5f;
    auto channelFor = [&] (int bar) { return ! colorMidi ? channel : (bar < 0 ? 5 : juce::jlimit (1, 4, bar + 1)); };

    auto addNotes = [&] (const std::vector<dubdrums::NoteEvent>& evs, double offset)
    {
        for (const auto& e : evs)
        {
            const int n = mapNote (e.note), ch = channelFor (e.bar);
            seq.addEvent (juce::MidiMessage::noteOn (ch, n, (juce::uint8) e.velocity),
                          std::round ((offset + e.beat) * kTicksPerQuarter));
            seq.addEvent (juce::MidiMessage::noteOff (ch, n),
                          std::round ((offset + e.beat + e.length) * kTicksPerQuarter));
        }
    };
    addNotes (p.intro, 0.0);
    addNotes (p.events, intro);

    seq.sort();
    seq.updateMatchedPairs();
    seq.addEvent (juce::MidiMessage::endOfTrack(), (intro + p.lengthInBeats) * kTicksPerQuarter);

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (kTicksPerQuarter);
    mf.addTrack (seq);

    target.deleteFile();
    juce::FileOutputStream out (target);
    if (! out.openedOk())
        return false;
    mf.writeTo (out);
    out.flush();
    return target.existsAsFile();
}

// ------------------------------------------------------------------ estado
void DubDrumsProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void DubDrumsProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    dirty = true;
}

juce::AudioProcessorEditor* DubDrumsProcessor::createEditor()
{
    return new DubDrumsEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DubDrumsProcessor();
}
