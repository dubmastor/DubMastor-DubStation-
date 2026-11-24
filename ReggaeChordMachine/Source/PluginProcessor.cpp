#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../../RiddimBank/Source/RiddimBank.h"

namespace
{
    juce::String rootId (int i) { return "root" + juce::String (i); }
    juce::String qualId (int i) { return "qual" + juce::String (i); }
    juce::String durId  (int i) { return "dur"  + juce::String (i); }
    juce::String onId   (int i) { return "on"   + juce::String (i); }
    juce::String invId  (int i) { return "inv"  + juce::String (i); }

    const int exportRepsTable[] = { 1, 2, 4, 8 };
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout DubChordsProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray roots, quals, durs;
    for (int r = 0; r < 12; ++r) roots.add (dub::rootName (r));
    quals.add ("maior"); quals.add ("menor");   // dub::Maj = 0, dub::Min = 1
    for (int d = 0; d < dub::numDurations; ++d) durs.add (dub::durationName (d));

    // Progressão padrão: Am (1 comp.) | Dm (1 comp.); acordes 3 (Em) e 4 (G) desligados
    const int  defRoot[numSlots] = { 9, 2, 4, 7 };
    const int  defQual[numSlots] = { dub::Min, dub::Min, dub::Min, dub::Maj };
    const int  defDur [numSlots] = { 0, 0, 0, 0 };            // 1/2 compasso cada
    const bool defOn  [numSlots] = { true, true, false, false };

    for (int i = 0; i < numSlots; ++i)
    {
        const String n = "Acorde " + String (i + 1);
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { rootId (i), 1 }, n + " tonica",   roots, defRoot[i]));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { qualId (i), 1 }, n + " tipo",     quals, defQual[i]));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { durId (i),  1 }, n + " duracao",  durs,  defDur[i]));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { invId (i), 1 }, n + " inversao",
                                                            StringArray { "AUTO", "FUND", juce::String::fromUTF8 ("1\xc2\xaa INV"), juce::String::fromUTF8 ("2\xc2\xaa INV") }, 0));
        if (i > 0)   // o acorde 1 está sempre ligado
            layout.add (std::make_unique<AudioParameterBool> (ParameterID { onId (i), 1 }, n + " ligado", defOn[i]));
    }

    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "center", 1 }, "Registro", 48, 72, 60));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "velocity", 1 }, "Velocity", 0.1f, 1.0f, 0.8f));
    {
        StringArray sizes;
        for (int n = 0; n < dub::numNoteSizes; ++n) sizes.add (dub::noteSizeName (n));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "noteSize", 1 }, "Tamanho da nota", sizes, 0));   // 1/32
    }
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "bubble", 1 }, "Bubble (orgao)", false));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "swing", 1 }, "Swing", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "preview", 1 }, "Preview (clock interno)", false));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "bpm", 1 }, "BPM interno", NormalisableRange<float> (50.0f, 180.0f, 1.0f), 75.0f));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "sound", 1 }, "Som interno", true));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "volume", 1 }, "Volume", 0.0f, 1.0f, 0.75f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "pianoLvl", 1 }, "Piano M1", 0.0f, 1.0f, 0.8f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "organLvl", 1 }, "Orgao Hammond", 0.0f, 1.0f, 0.6f));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "exportReps", 1 }, "Repeticoes no export",
                                                        StringArray { "1x", "2x", "4x", "8x" }, 1));
    // PIANISTAS (chops inspirados nos tecladistas jamaicanos) e a nota de 1/64. No fim da lista: os antigos não mudam de lugar.
    {
        StringArray pn;
        for (int p = 0; p < dub::NumPianists; ++p) pn.add (dub::pianistName (p));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "pianist", 1 }, "Pianista", pn, dub::PnClassico));
    }
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "nota64", 1 }, "Nota 1/64", false));
    {   // FILLS (seletor separado do pianista): no fim de cada frase de 4 compassos
        StringArray fl;
        for (int f = 0; f < dub::NumFills; ++f) fl.add (dub::fillName (f));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "pianoFill", 1 }, "Fill", fl, dub::FillNone));
    }
    return layout;
}

//==============================================================================
DubChordsProcessor::DubChordsProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DubChords", createLayout())
{

    for (auto* p : getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            apvts.addParameterListener (withId->paramID, this);

    pending.reserve (8192);
    startTimerHz (30);
}

DubChordsProcessor::~DubChordsProcessor()
{
    stopTimer();
    for (auto* p : getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            apvts.removeParameterListener (withId->paramID, this);
}

bool DubChordsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void DubChordsProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const int maxBlock = std::max (samplesPerBlock, 8192);
    sound.prepare (sampleRate, maxBlock);
    renderL.assign ((size_t) maxBlock, 0.0f);
    renderR.assign ((size_t) maxBlock, 0.0f);
    pending.reserve (8192);
    // fora do tempo real: pode montar o loop aqui mesmo
    loop = getCurrentLoop (loopBeats);
    displayLoop.store (loopBeats);
    hasIncoming.store (false);
    dirty.store (false);
    for (auto& ch : active) for (bool& b : ch) b = false;
    wasPlaying = false;
    pianoGain = param ("pianoLvl") * param ("volume");
    organGain = param ("organLvl") * param ("volume");
}

//==============================================================================
std::vector<dub::ChordSlot> DubChordsProcessor::getSlots() const
{
    std::vector<dub::ChordSlot> slots;
    for (int i = 0; i < numSlots; ++i)
    {
        dub::ChordSlot s;
        s.root    = (int) param (rootId (i));
        s.quality = juce::jlimit (0, 1, (int) param (qualId (i)));
        const bool on = (i == 0) || param (onId (i)) > 0.5f;
        s.beats   = on ? dub::durationBeats ((int) param (durId (i))) : 0.0;
        s.inversion = (int) param (invId (i)) - 1;   // 0 = AUTO -> -1
        slots.push_back (s);
    }
    return slots;
}

dub::Settings DubChordsProcessor::getSettings() const
{
    dub::Settings s;
    s.pattern    = dub::SkankOffbeat;   // skank sempre no contratempo
    s.centerNote = (int) param ("center");
    s.velocity   = param ("velocity");
    s.noteSize   = param ("nota64") > 0.5f ? dub::NoteSize64 : 0;   // 1/32 (padrão) ou 1/64 ("noteSize" ficou só para projetos antigos)
    s.pianist    = juce::jlimit (0, dub::NumPianists - 1, (int) param ("pianist"));
    s.fill       = juce::jlimit (0, dub::NumFills - 1, (int) param ("pianoFill"));
    s.bubble     = param ("bubble") > 0.5f;
    s.swing      = param ("swing");
    return s;
}

juce::String DubChordsProcessor::getProgressionText() const
{
    juce::StringArray parts;
    for (const auto& s : getSlots())
        if (s.beats > 0)
        {
            const double bars = s.beats / 4.0;
            parts.add (juce::String (dub::chordLabel (s)) + " (" + juce::String (bars, bars < 1.0 ? 1 : 0) + ")");
        }
    return parts.isEmpty() ? juce::String ("Nenhum acorde ativo")
                           : parts.joinIntoString ("  >  ");
}

dub::Voicings DubChordsProcessor::getVoicings() const
{
    return dub::voiceProgression (getSlots(), getSettings().centerNote);
}

double DubChordsProcessor::getExportBpm() const
{
    const double h = hostBpm.load();
    return h > 0.0 ? h : (double) param ("bpm");
}

// Mexer nos controles durante o PREVIEW não pode estalar nem prender nota: o loop novo é montado
// aqui (thread de mensagens, pode alocar) e só trocado de lugar na thread de áudio (swap, sem alocar).
void DubChordsProcessor::timerCallback()
{
    if (dirty.exchange (false))
        publishLoop();
}

void DubChordsProcessor::publishLoop()
{
    double beats = 0.0;
    auto fresh = getCurrentLoop (beats);
    {
        const juce::SpinLock::ScopedLockType lock (swapLock);
        incoming.swap (fresh);            // o loop antigo que estava esperando é liberado aqui, fora do áudio
        incomingBeats = beats;
        hasIncoming.store (true);
    }
    displayLoop.store (beats);
}

bool DubChordsProcessor::takeNewLoop()
{
    if (! hasIncoming.load()) return false;
    const juce::SpinLock::ScopedTryLockType lock (swapLock);
    if (! lock.isLocked()) return false;  // a thread de mensagens está entregando: pega no próximo bloco
    loop.swap (incoming);
    loopBeats = incomingBeats;
    hasIncoming.store (false);
    return true;
}

void DubChordsProcessor::allNotesOff (juce::MidiBuffer& midi, int sample)
{
    for (int ch = 1; ch <= 16; ++ch)
        for (int n = 0; n < 128; ++n)
            if (active[ch][n])
            {
                midi.addEvent (juce::MidiMessage::noteOff (ch, n), sample);
                active[ch][n] = false;
            }
}

//==============================================================================
void DubChordsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0) return;

    // render offline (bounce) pode não deixar a thread de mensagens rodar: monta aqui mesmo
    if (isNonRealtime() && dirty.exchange (false))
        publishLoop();
    const bool loopChanged = takeNewLoop();

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
        // Salto no transporte OU loop novo (mexeu nos acordes/padrão durante o preview):
        // solta tudo o que estava tocando antes de seguir com o loop atual
        if (wasPlaying && (loopChanged || std::abs (ppq - expectedPpq) > 1.0e-3))
            allNotesOff (midi, 0);

        const double end = ppq + blockBeats;
        pending.clear();

        auto collect = [&] (double t, bool on, const dub::NoteEvent& e)
        {
            double occ = t + std::ceil ((ppq - t) / loopBeats) * loopBeats;
            while (occ < end)
            {
                if (occ >= ppq)
                {
                    const int s = juce::jlimit (0, numSamples - 1, (int) ((occ - ppq) / blockBeats * numSamples));
                    pending.push_back ({ s, on, e.channel, e.note, e.velocity });
                }
                occ += loopBeats;
            }
        };

        for (const auto& e : loop)
        {
            collect (e.start, true, e);
            collect (e.start + e.length, false, e);
        }

        // note-offs antes de note-ons na mesma amostra
        std::sort (pending.begin(), pending.end(), [] (const Pending& a, const Pending& b)
        {
            if (a.sample != b.sample) return a.sample < b.sample;
            return (int) a.on < (int) b.on;
        });

        for (const auto& p : pending)
        {
            bool& isOn = active[p.channel][p.note];
            if (p.on)
            {
                if (isOn) midi.addEvent (juce::MidiMessage::noteOff (p.channel, p.note), p.sample);
                midi.addEvent (juce::MidiMessage::noteOn (p.channel, p.note, (juce::uint8) p.vel), p.sample);
                isOn = true;
            }
            else if (isOn)
            {
                midi.addEvent (juce::MidiMessage::noteOff (p.channel, p.note), p.sample);
                isOn = false;
            }
        }

        wasPlaying = true;
        expectedPpq = end;
        if (! hostPlaying) internalPpq = end;
        displayPos.store (std::fmod (std::max (0.0, ppq), loopBeats));
    }

    // --- Som interno (o MIDI gerado continua saindo para a DAW) ---
    const bool soundOn = param ("sound") > 0.5f;
    if (soundOn != soundWasOn)
    {
        sound.allNotesOff();      // nada fica preso ao desligar/religar o SOM INTERNO
        soundWasOn = soundOn;
    }
    if (soundOn)
        renderSound (buffer, midi);
}

//==============================================================================
// Toca o MIDI do bloco no som interno (piano M1 no canal 1, Hammond no canal 3)
void DubChordsProcessor::renderSound (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    const int n = buffer.getNumSamples();
    if ((int) renderL.size() < n) { renderL.resize ((size_t) n); renderR.resize ((size_t) n); }   // só se o host mandar bloco gigante
    std::fill (renderL.begin(), renderL.begin() + n, 0.0f);
    std::fill (renderR.begin(), renderR.begin() + n, 0.0f);

    // volumes com rampa ao longo do bloco (girar PIANO M1 / HAMMOND / volume não estala)
    const float pTo = param ("pianoLvl") * param ("volume");
    const float oTo = param ("organLvl") * param ("volume");
    const float pFrom = pianoGain, oFrom = organGain;
    auto renderRange = [&] (int from, int to)
    {
        for (int s0 = from; s0 < to; )
        {
            const int len = std::min (to - s0, sound.maxBlock());
            const float a0 = (float) s0 / (float) n, a1 = (float) (s0 + len) / (float) n;
            sound.render (renderL.data() + s0, renderR.data() + s0, len,
                          pFrom + (pTo - pFrom) * a0, pFrom + (pTo - pFrom) * a1,
                          oFrom + (oTo - oFrom) * a0, oFrom + (oTo - oFrom) * a1);
            s0 += len;
        }
    };

    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, n, meta.samplePosition);
        if (at > pos) { renderRange (pos, at); pos = at; }
        const auto m = meta.getMessage();
        if (m.isNoteOn())                                sound.noteOn (m.getChannel(), m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())                          sound.noteOff (m.getChannel(), m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) sound.allNotesOff();
    }
    if (pos < n) renderRange (pos, n);
    pianoGain = pTo;
    organGain = oTo;

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, renderL.data(), n);
        buffer.copyFrom (1, 0, renderR.data(), n);
    }
    else if (buffer.getNumChannels() == 1)
    {
        for (int i = 0; i < n; ++i) renderL[(size_t) i] = 0.5f * (renderL[(size_t) i] + renderR[(size_t) i]);
        buffer.copyFrom (0, 0, renderL.data(), n);
    }
}

//==============================================================================
// PRESETS de harmonia
void DubChordsProcessor::applyPreset (int index)
{
    if (index < 0 || index >= dubpresets::numPresets) return;
    const auto& pr = dubpresets::presets[index];
    auto set = [this] (const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (v));
            p->endChangeGesture();
        }
    };
    for (int i = 0; i < numSlots; ++i)
    {
        const bool used = i < pr.numChords;
        if (used)
        {
            set (rootId (i), (float) pr.chords[i].root);
            set (qualId (i), (float) (pr.chords[i].minor ? dub::Min : dub::Maj));
            set (durId (i),  (float) pr.chords[i].dur);
            set (invId (i),  0.0f);   // AUTO
        }
        if (i > 0) set (onId (i), used ? 1.0f : 0.0f);
    }
    lastPreset.store (index);
    apvts.state.setProperty ("bank", riddimbank::fromMrcm[index], nullptr);
}

// BANCO ÚNICO: a harmonia da música (só os acordes que ela tem ficam ligados) e o BPM dela
void DubChordsProcessor::applyBankEntry (int index)
{
    if (index < 0 || index >= riddimbank::numEntries) return;
    const auto& e = riddimbank::entries[index];
    if (e.numChords <= 0) return;   // harmonia ainda pendente
    auto set = [this] (const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (v));
            p->endChangeGesture();
        }
    };
    for (int i = 0; i < numSlots; ++i)
    {
        const bool used = i < e.numChords;
        if (used)
        {
            set (rootId (i), (float) e.chords[i].root);
            set (qualId (i), (float) (e.chords[i].minor ? dub::Min : dub::Maj));
            set (durId (i),  (float) e.chords[i].dur);
            set (invId (i),  0.0f);   // AUTO
        }
        if (i > 0) set (onId (i), used ? 1.0f : 0.0f);
    }
    set ("bpm", (float) e.bpm);
    lastPreset.store (e.mrcm);
    apvts.state.setProperty ("bank", index, nullptr);
}

int DubChordsProcessor::getBankEntry() const
{
    const int b = apvts.state.getProperty ("bank", -1);
    if (b >= 0 && b < riddimbank::numEntries) return b;
    const int p = lastPreset.load();
    return p >= 0 && p < dubpresets::numPresets ? riddimbank::fromMrcm[p] : -1;
}

//==============================================================================
// MIDI editável
juce::String DubChordsProcessor::settingsKey() const
{
    juce::String k;
    for (int i = 0; i < numSlots; ++i)
    {
        k << (int) param (rootId (i)) << ',' << (int) param (qualId (i)) << ',' << (int) param (durId (i)) << ','
          << (int) param (invId (i)) << ',' << (i == 0 ? 1 : (param (onId (i)) > 0.5f ? 1 : 0)) << ';';
    }
    k << 0 << ',' << (int) param ("center") << ',' << juce::roundToInt (param ("velocity") * 1000.0f) << ','
      << (param ("bubble") > 0.5f ? 1 : 0) << ',' << juce::roundToInt (param ("swing") * 1000.0f)
      << ',' << (int) param ("pianist") << ',' << (param ("nota64") > 0.5f ? 1 : 0) << ',' << (int) param ("pianoFill");
    return k;
}

std::vector<dub::NoteEvent> DubChordsProcessor::getCurrentLoop (double& lb) const
{
    {
        const juce::SpinLock::ScopedLockType lock (editLock);
        if (editsOn && editsKey == settingsKey()) { lb = editsLoopBeats; return edits; }
    }
    return dub::buildLoop (getSlots(), getSettings(), lb);
}

void DubChordsProcessor::setEdits (std::vector<dub::NoteEvent> ev, double lb)
{
    std::sort (ev.begin(), ev.end(), [] (const dub::NoteEvent& a, const dub::NoteEvent& b) { return a.start < b.start; });
    {
        const juce::SpinLock::ScopedLockType lock (editLock);
        edits = std::move (ev);
        editsLoopBeats = lb;
        editsKey = settingsKey();
        editsOn = true;
    }
    dirty.store (true);
}

bool DubChordsProcessor::hasEdits() const
{
    const juce::SpinLock::ScopedLockType lock (editLock);
    return editsOn && editsKey == settingsKey();
}

void DubChordsProcessor::clearEdits()
{
    {
        const juce::SpinLock::ScopedLockType lock (editLock);
        editsOn = false;
        edits.clear();
    }
    dirty.store (true);
}

//==============================================================================
bool DubChordsProcessor::writeMidiFile (const juce::File& file) const
{
    double lb = 0.0;
    const auto events = getCurrentLoop (lb);
    if (lb <= 0.0 || events.empty()) return false;

    const int tpq = 960;
    const int reps = exportRepsTable[juce::jlimit (0, 3, (int) param ("exportReps"))];
    const double bpm = getExportBpm();

    // Uma faixa por instrumento; dentro de cada faixa, cada acorde usa o seu canal MIDI
    // (acorde 1 = canal 1 ... acorde 4 = canal 4). Na DAW, colorir as notas por canal
    // (Cubase: Cores > Canal; REAPER e FL Studio também) deixa cada acorde de uma cor.
    juce::MidiMessageSequence meta, skank, bubble;
    meta.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)), 0.0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);

    auto trackName = [] (const juce::String& n) { return juce::MidiMessage::textMetaEvent (3, n); };
    skank.addEvent  (trackName ("MRCM-1240 - Skank (piano)"), 0.0);
    bubble.addEvent (trackName ("MRCM-1240 - Bubble (orgao)"), 0.0);

    // marcadores com o nome de cada acorde (aparecem na régua/marker track da DAW)
    const auto slots = getSlots();
    for (int r = 0; r < reps; ++r)
    {
        double pos = 0.0;
        for (const auto& sl : slots)
        {
            if (sl.beats <= 0) continue;
            meta.addEvent (juce::MidiMessage::textMetaEvent (6, juce::String (dub::chordLabel (sl))), (pos + r * lb) * tpq);
            pos += sl.beats;
        }
    }

    for (int r = 0; r < reps; ++r)
        for (const auto& e : events)
        {
            const double on  = (e.start + r * lb) * tpq;
            const double off = (e.start + e.length + r * lb) * tpq;
            auto& seq = e.channel == dub::ChBubble ? bubble : skank;
            const int ch = juce::jlimit (1, 16, e.slot + 1);
            seq.addEvent (juce::MidiMessage::noteOn (ch, e.note, (juce::uint8) e.velocity), on);
            seq.addEvent (juce::MidiMessage::noteOff (ch, e.note), off);
        }

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    meta.sort();
    mf.addTrack (meta);
    for (auto* seq : { &skank, &bubble })
        if (seq->getNumEvents() > 1)
        {
            seq->sort();
            seq->updateMatchedPairs();
            mf.addTrack (*seq);
        }

    file.getParentDirectory().createDirectory();
    file.deleteFile();
    juce::FileOutputStream out (file);
    if (! out.openedOk()) return false;
    return mf.writeTo (out);
}

//==============================================================================
void DubChordsProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    {   // MIDI editado (só se ainda vale para os controles atuais)
        const juce::SpinLock::ScopedLockType lock (editLock);
        if (editsOn && editsKey == settingsKey())
        {
            juce::ValueTree ed ("Edits");
            ed.setProperty ("loopBeats", editsLoopBeats, nullptr);
            for (const auto& e : edits)
            {
                juce::ValueTree n ("N");
                n.setProperty ("s", e.start, nullptr);    n.setProperty ("l", e.length, nullptr);
                n.setProperty ("n", e.note, nullptr);     n.setProperty ("v", e.velocity, nullptr);
                n.setProperty ("c", e.channel, nullptr);  n.setProperty ("k", e.slot, nullptr);
                ed.appendChild (n, nullptr);
            }
            state.appendChild (ed, nullptr);
        }
    }
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DubChordsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            auto ed = tree.getChildWithName ("Edits");
            if (ed.isValid()) tree.removeChild (ed, nullptr);
            apvts.replaceState (tree);
            if (ed.isValid())
            {
                std::vector<dub::NoteEvent> ev;
                for (const auto& n : ed)
                    ev.push_back ({ (double) n.getProperty ("s"), (double) n.getProperty ("l"), (int) n.getProperty ("n"),
                                    (int) n.getProperty ("v"), (int) n.getProperty ("c"), (int) n.getProperty ("k") });
                setEdits (std::move (ev), (double) ed.getProperty ("loopBeats"));
            }
            else clearEdits();
        }
    dirty.store (true);
}

juce::AudioProcessorEditor* DubChordsProcessor::createEditor() { return new DubChordsEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DubChordsProcessor(); }
