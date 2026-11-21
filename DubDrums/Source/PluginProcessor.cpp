#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    choice ("style", "Estilo", { "Rockers", "One Drop", "Boom Bap" }, 0);
    // bumbo e caixa principais por colcheia (8 posições, bit 0 = "1"); o estilo carrega o padrão dele
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "kickBeats", 1 },  "Bumbo (colcheias)", 0, 255,
                                                     dubdrums::defaultKickMask (dubdrums::Style::Rockers)));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "snareBeats", 1 }, "Caixa (colcheias)", 0, 255,
                                                     dubdrums::defaultSnareMask (dubdrums::Style::Rockers)));
    choice ("bars", "Compassos", { "1", "2", "3", "4" }, 3);
    choice ("hatPattern", "Hi-hat", { "Colcheias", "Semicolcheias", "Contratempo", "Tercinas" }, 0);
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
    choice ("drummer", "Baterista", { "Style Scott", "Santa Davis", "Carlton Barrett" }, 2);
    choice ("backbeat", "Batida principal", { "Caixa", "Aro", "Caixa + aro" }, 0);
    choice ("kit", "Kit", { "Acustico (anos 70)", "Digital (King Jammy / Sleng Teng)" }, 0);
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "intro", 1 }, u8s ("Virada de introdução"), false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "preview", 1 }, "Preview", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "colorMidi", 1 }, "MIDI colorido (um canal por compasso)", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "sound", 1 }, "Som interno", true));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM do preview",
                                                       NormalisableRange<float> (60.0f, 180.0f, 1.0f), 75.0f));
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

    regenerate();
    startTimerHz (20);
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
    return s;
}

void DubDrumsProcessor::regenerate()
{
    auto p = dubdrums::generate (readSettings());
    {
        const juce::SpinLock::ScopedLockType lock (patternLock);
        std::swap (pattern, p);
    }
    ++patternVersion;
    // p (o padrão antigo) é liberado aqui, fora da thread de áudio
}

dubdrums::Pattern DubDrumsProcessor::getPatternCopy() const
{
    const juce::SpinLock::ScopedLockType lock (patternLock);
    return pattern;
}

void DubDrumsProcessor::applyStyleDefaults (int styleIndex)
{
    const auto st = (dubdrums::Style) juce::jlimit (0, 2, styleIndex);
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
}

// ------------------------------------------------------------------ áudio / MIDI
int DubDrumsProcessor::mapNote (int gmNote) const
{
    for (const auto& pc : pieces)
        if (pc.gmNote == gmNote)
            return juce::jlimit (0, 127, juce::roundToInt (apvts.getRawParameterValue (juce::String ("note_") + pc.id)->load()));
    return gmNote;
}

// Linha do tempo: [intro (toca uma vez)] [loop] [loop] [loop] ...
template <typename Fn>
void DubDrumsProcessor::forEachEventIn (double start, double end, Fn&& fn) const
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
    synth.setKit (apvts.getRawParameterValue ("kit")->load() > 0.5f ? dubdrums::DrumSynth::Kit::Digital
                                                                   : dubdrums::DrumSynth::Kit::Acoustic);

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

    // PREVIEW: com a DAW parada, o plugin toca sozinho no BPM do painel, começando pela intro
    const bool preview = previewOn && ! playing;
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
        midi.addEvent (juce::MidiMessage::allNotesOff (channel), 0);
    wasPlaying = playing;

    if (! playing && ! preview)
    {
        playheadBeat = -1.0;
        synth.render (buffer.getWritePointer (0), buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr,
                      0, numSamples, volume);   // deixa os pratos terminarem
        return;
    }

    const double blockBeats = (double) numSamples / sr * bpm / 60.0;
    if (preview) previewPos += blockBeats;

    const juce::SpinLock::ScopedTryLockType lock (patternLock);
    if (! lock.isLocked())
        return; // a interface está trocando o padrão neste instante

    const double intro = pattern.introLengthInBeats, loopLen = pattern.lengthInBeats;
    {
        double pos = std::max (0.0, ppq);
        if (pos >= intro) pos = intro + std::fmod (pos - intro, loopLen);
        playheadBeat = pos;
    }

    struct Trigger { int offset; int note; int vel; };
    std::array<Trigger, 256> triggers;
    int numTriggers = 0;
    const bool makeSound = preview || soundOn;

    forEachEventIn (ppq, ppq + blockBeats, [&] (double t, const dubdrums::NoteEvent& e, bool off)
    {
        const int offset = juce::jlimit (0, numSamples - 1, (int) ((t - ppq) / blockBeats * numSamples));
        if (playing)
            midi.addEvent (off ? juce::MidiMessage::noteOff (channel, mapNote (e.note))
                               : juce::MidiMessage::noteOn (channel, mapNote (e.note), (juce::uint8) e.velocity), offset);
        if (makeSound && ! off && numTriggers < (int) triggers.size())
            triggers[(size_t) numTriggers++] = { offset, e.note, e.velocity };
    });

    // bateria interna: renderiza em pedaços entre os toques
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
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
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i) d[i] = std::tanh (d[i]);
    }
}

// ------------------------------------------------------------------ arquivo MIDI
juce::String DubDrumsProcessor::suggestedFileName() const
{
    const auto s = readSettings();
    juce::String name = juce::String ("DubDrums_") + dubdrums::styleName (s.style)
                      + "_" + dubdrums::drummerName (s.drummer)
                      + "_" + juce::String (s.bars) + "comp" + (s.intro ? "_intro" : "") + "_s" + juce::String ((int) s.seed);
    return name.replaceCharacter (' ', '-') + ".mid";
}

bool DubDrumsProcessor::writeMidiFile (const juce::File& target) const
{
    const auto p = getPatternCopy();
    const int channel = juce::jlimit (1, 16, juce::roundToInt (apvts.getRawParameterValue ("channel")->load()));
    const double intro = p.introLengthInBeats;

    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, "Dub Drums"), 0.0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);

    // marcadores com o nome de cada compasso (aparecem na régua de DAWs que importam marcadores)
    if (intro > 0.0)
        seq.addEvent (juce::MidiMessage::textMetaEvent (6, "Intro"), 0.0);
    for (int bar = 0; bar < juce::roundToInt (p.lengthInBeats / 4.0); ++bar)
        seq.addEvent (juce::MidiMessage::textMetaEvent (6, "Compasso " + juce::String (bar + 1)), (intro + bar * 4.0) * kTicksPerQuarter);

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
