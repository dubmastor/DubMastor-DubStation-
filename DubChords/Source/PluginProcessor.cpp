#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    StringArray roots, quals, durs, pats;
    for (int r = 0; r < 12; ++r) roots.add (dub::rootName (r));
    quals.add ("maior"); quals.add ("menor");   // dub::Maj = 0, dub::Min = 1
    for (int d = 0; d < dub::numDurations; ++d) durs.add (dub::durationName (d));
    for (int p = 0; p < dub::NumPatterns; ++p) pats.add (dub::patternName (p));

    // Progressão padrão: Am (2 comp.) | Dm (1) | Em (1/2) | G (1/2)
    const int  defRoot[numSlots] = { 9, 2, 4, 7 };
    const int  defQual[numSlots] = { dub::Min, dub::Min, dub::Min, dub::Maj };
    const int  defDur [numSlots] = { 2, 1, 0, 0 };            // 2 comp. | 1 comp. | 1/2 | 1/2
    const bool defOn  [numSlots] = { true, true, true, true };

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

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "pattern", 1 }, "Padrao", pats, 0));
    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "center", 1 }, "Registro", 48, 72, 60));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "velocity", 1 }, "Velocity", 0.1f, 1.0f, 0.8f));
    {
        StringArray sizes;
        for (int n = 0; n < dub::numNoteSizes; ++n) sizes.add (dub::noteSizeName (n));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "noteSize", 1 }, "Tamanho da nota", sizes, 1));
    }
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "bubble", 1 }, "Bubble (orgao)", true));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "swing", 1 }, "Swing", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "preview", 1 }, "Preview (clock interno)", false));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "bpm", 1 }, "BPM interno", NormalisableRange<float> (55.0f, 180.0f, 1.0f), 75.0f));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "sound", 1 }, "Som interno", true));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "volume", 1 }, "Volume", 0.0f, 1.0f, 0.75f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "pianoLvl", 1 }, "Piano M1", 0.0f, 1.0f, 0.8f));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "organLvl", 1 }, "Orgao Hammond", 0.0f, 1.0f, 0.6f));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "exportReps", 1 }, "Repeticoes no export",
                                                        StringArray { "1x", "2x", "4x", "8x" }, 1));
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

    pending.reserve (2048);
}

DubChordsProcessor::~DubChordsProcessor()
{
    for (auto* p : getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            apvts.removeParameterListener (withId->paramID, this);
}

bool DubChordsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void DubChordsProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    sound.prepare (sampleRate);
    renderL.assign (8192, 0.0f);
    renderR.assign (8192, 0.0f);
    dirty.store (true);
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
    s.pattern    = (int) param ("pattern");
    s.centerNote = (int) param ("center");
    s.velocity   = param ("velocity");
    s.noteSize   = (int) param ("noteSize");
    s.bubble     = param ("bubble") > 0.5f;
    s.swing      = param ("swing");
    s.bass       = false;   // baixo guia removido na 1.3
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

void DubChordsProcessor::rebuild()
{
    loop = dub::buildLoop (getSlots(), getSettings(), loopBeats);
    displayLoop.store (loopBeats);
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

    if (dirty.exchange (false))
        rebuild();

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
        // Salto no transporte (loop da DAW, reposicionamento): limpa notas presas
        if (wasPlaying && std::abs (ppq - expectedPpq) > 1.0e-3)
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
        sound.killAll();          // nada fica preso ao desligar/religar o SOM INTERNO
        soundWasOn = soundOn;
    }
    if (soundOn)
        renderSound (buffer, midi);
}

//==============================================================================
// Toca o MIDI do bloco no som interno (piano M1 no canal 1, baixo no 2, Hammond no 3)
void DubChordsProcessor::renderSound (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    sound.pianoLevel = param ("pianoLvl");
    sound.organLevel = param ("organLvl");
    sound.master     = param ("volume");

    const int n = buffer.getNumSamples();
    if ((int) renderL.size() < n) { renderL.resize ((size_t) n); renderR.resize ((size_t) n); }   // só se o host mandar bloco gigante
    std::fill (renderL.begin(), renderL.begin() + n, 0.0f);
    std::fill (renderR.begin(), renderR.begin() + n, 0.0f);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, n, meta.samplePosition);
        if (at > pos) { sound.render (renderL.data() + pos, renderR.data() + pos, at - pos); pos = at; }
        const auto m = meta.getMessage();
        if (m.isNoteOn())                         sound.noteOn (m.getChannel(), m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())                   sound.noteOff (m.getChannel(), m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) sound.allNotesOff();
    }
    if (pos < n) sound.render (renderL.data() + pos, renderR.data() + pos, n - pos);

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
bool DubChordsProcessor::writeMidiFile (const juce::File& file) const
{
    double lb = 0.0;
    const auto events = dub::buildLoop (getSlots(), getSettings(), lb);
    if (lb <= 0.0 || events.empty()) return false;

    const int tpq = 960;
    const int reps = exportRepsTable[juce::jlimit (0, 3, (int) param ("exportReps"))];
    const double bpm = getExportBpm();

    // Uma faixa por instrumento; dentro de cada faixa, cada acorde usa o seu canal MIDI
    // (acorde 1 = canal 1 ... acorde 4 = canal 4). Na DAW, colorir as notas por canal
    // (Cubase: Cores > Canal; REAPER e FL Studio também) deixa cada acorde de uma cor.
    juce::MidiMessageSequence meta, skank, bubble, bass;
    meta.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)), 0.0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);

    auto trackName = [] (const juce::String& n) { return juce::MidiMessage::textMetaEvent (3, n); };
    skank.addEvent  (trackName ("Dub Chords - Skank (piano)"), 0.0);
    bubble.addEvent (trackName ("Dub Chords - Bubble (orgao)"), 0.0);
    bass.addEvent   (trackName ("Dub Chords - Baixo guia"), 0.0);

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
            auto& seq = e.channel == dub::ChBass ? bass : e.channel == dub::ChBubble ? bubble : skank;
            const int ch = juce::jlimit (1, 16, e.slot + 1);
            seq.addEvent (juce::MidiMessage::noteOn (ch, e.note, (juce::uint8) e.velocity), on);
            seq.addEvent (juce::MidiMessage::noteOff (ch, e.note), off);
        }

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    meta.sort();
    mf.addTrack (meta);
    for (auto* seq : { &skank, &bubble, &bass })
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
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DubChordsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    dirty.store (true);
}

juce::AudioProcessorEditor* DubChordsProcessor::createEditor() { return new DubChordsEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DubChordsProcessor(); }
