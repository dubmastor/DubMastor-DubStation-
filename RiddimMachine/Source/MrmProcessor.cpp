#include "MrmProcessor.h"
#include "MrmEditor.h"

namespace
{
    constexpr int kTicksPerQuarter = 960;
    constexpr int kLengths[MrmProcessor::numLengths] = { 4, 8, 16, 32, 64 };

    void setParam (juce::AudioProcessorValueTreeState& s, const char* id, float plainValue)
    {
        if (auto* p = s.getParameter (id))
        {
            const float v = p->convertTo0to1 (plainValue);
            if (std::abs (p->getValue() - v) > 1.0e-6f)
                p->setValueNotifyingHost (v);
        }
    }
    float getParam (const juce::AudioProcessorValueTreeState& s, const char* id)
    {
        return s.getRawParameterValue (id)->load();
    }

    // satura só o que passa de 0,85: a soma da bateria com o piano nunca estoura
    inline float softClip (float x)
    {
        const float a = std::abs (x), knee = 0.85f;
        if (a <= knee) return x;
        return std::copysign (knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee)), x);
    }
}

int MrmProcessor::lengthBars (int index) { return kLengths[juce::jlimit (0, numLengths - 1, index)]; }

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MrmProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "play", 1 }, "Play riddim", false));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM",
                                                       NormalisableRange<float> (60.0f, 180.0f, 1.0f), 75.0f));
    StringArray lengths;
    for (int i = 0; i < numLengths; ++i) lengths.add (String (kLengths[i]) + " compassos");
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "riddimBars", 1 }, "Tamanho do riddim", lengths, 2));
    return layout;
}

MrmProcessor::MrmProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MRM0420Params", createLayout())
{
    chords.setPlayHead (&chordsHead);
    drums.setPlayHead (&drumsHead);

    // o BPM do MRM manda nos dois geradores desde o começo
    setParam (chords.apvts, "bpm", raw ("bpm"));
    setParam (drums.apvts,  "bpm", raw ("bpm"));
    startTimerHz (15);
}

MrmProcessor::~MrmProcessor()
{
    stopTimer();
}

bool MrmProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void MrmProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    const int bs = juce::jmax (512, samplesPerBlock);
    for (auto* p : { (juce::AudioProcessor*) &chords, (juce::AudioProcessor*) &drums })
    {
        p->setRateAndBufferSizeDetails (sr, bs);
        p->prepareToPlay (sr, bs);
    }
    chordsBuf.setSize (2, bs);
    drumsBuf.setSize (2, bs);
    for (auto* m : { &chordsMidi, &chordsMidiB, &drumsMidi, &inputA, &inputB })
        m->ensureSize (4096);
    wasMaster = false;
    masterPpq = 0.0;
}

void MrmProcessor::releaseResources()
{
    chords.releaseResources();
    drums.releaseResources();
}

int MrmProcessor::riddimBars() const     { return lengthBars ((int) raw ("riddimBars")); }
double MrmProcessor::introBeats() const  { return getParam (drums.apvts, "intro") > 0.5f ? 4.0 : 0.0; }

//==============================================================================
void MrmProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) return;

    if (chordsBuf.getNumSamples() < n)          // bloco maior que o anunciado (raro)
    {
        chordsBuf.setSize (2, n, false, false, true);
        drumsBuf.setSize (2, n, false, false, true);
    }

    // --- relógio: DAW tocando, ou PLAY RIDDIM com a DAW parada
    juce::Optional<juce::AudioPlayHead::PositionInfo> hostPos;
    if (auto* ph = getPlayHead()) hostPos = ph->getPosition();
    const bool dawPlaying = hostPos.hasValue() && hostPos->getIsPlaying() && hostPos->getPpqPosition().hasValue();
    if (hostPos.hasValue())
        if (auto b = hostPos->getBpm(); b.hasValue() && *b > 0.0) hostBpm.store (*b);
    hostPlaying.store (dawPlaying);

    const bool master = raw ("play") > 0.5f && ! dawPlaying;
    if (master && ! wasMaster) masterPpq = 0.0;
    wasMaster = master;
    masterPlaying.store (master);

    juce::AudioPlayHead::PositionInfo pos;
    if (master)
    {
        pos.setIsPlaying (true);
        pos.setBpm ((double) raw ("bpm"));
        pos.setPpqPosition (masterPpq);
        pos.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
    }
    else if (hostPos.hasValue())
        pos = *hostPos;

    const bool playing = master || dawPlaying;
    const double bpm = pos.getBpm().hasValue() && *pos.getBpm() > 0.0 ? *pos.getBpm() : 120.0;
    const double blockBeats = (double) n / sr * bpm / 60.0;
    const double ppq = playing ? *pos.getPpqPosition() : 0.0;
    playBeat.store (playing ? juce::jmax (0.0, ppq) : -1.0);

    // --- bateria: no relógio direto (intro no começo, depois o loop)
    juce::AudioBuffer<float> drumsView (drumsBuf.getArrayOfWritePointers(), 2, 0, n);
    drumsMidi.clear();
    drumsHead.info = pos;
    drums.processBlock (drumsView, drumsMidi);

    // --- harmonia: entra depois da intro da bateria
    processChords (pos, playing, blockBeats, midi, n);

    if (master) masterPpq += blockBeats;

    // --- saída: soma dos dois, com teto suave
    const int outCh = buffer.getNumChannels();
    for (int ch = 0; ch < outCh; ++ch)
    {
        auto* out = buffer.getWritePointer (ch);
        if (outCh == 1)
        {
            for (int i = 0; i < n; ++i)
                out[i] = softClip (0.5f * (chordsBuf.getSample (0, i) + chordsBuf.getSample (1, i)
                                           + drumsBuf.getSample (0, i) + drumsBuf.getSample (1, i)));
        }
        else
        {
            const int src = juce::jmin (ch, 1);
            const float* a = chordsBuf.getReadPointer (src);
            const float* d = drumsBuf.getReadPointer (src);
            for (int i = 0; i < n; ++i) out[i] = softClip (a[i] + d[i]);
        }
    }

    // --- MIDI ao vivo: skank canal 1, bubble 3, bateria no canal do Dub Drums (10)
    // No PLAY RIDDIM (DAW parada) nada sai, como no PREVIEW dos plugins separados.
    midi.clear();
    if (! master)
    {
        midi.addEvents (chordsMidi, 0, n, 0);
        midi.addEvents (drumsMidi, 0, n, 0);
    }
}

void MrmProcessor::processChords (const juce::AudioPlayHead::PositionInfo& pos, bool playing, double blockBeats,
                                  const juce::MidiBuffer& input, int n)
{
    chordsMidi.clear();
    const double intro = introBeats();
    const double shifted = playing ? *pos.getPpqPosition() - intro : 0.0;

    auto run = [this] (juce::AudioPlayHead::PositionInfo p, int start, int len, juce::MidiBuffer& m)
    {
        chordsHead.info = p;
        juce::AudioBuffer<float> view (chordsBuf.getArrayOfWritePointers(), 2, start, len);
        chords.processBlock (view, m);
    };

    if (! playing || intro <= 0.0 || shifted >= 0.0)
    {
        auto p = pos;
        if (playing) p.setPpqPosition (shifted);
        chordsMidi.addEvents (input, 0, n, 0);
        run (p, 0, n, chordsMidi);
        return;
    }

    // ainda na intro: os acordes ficam parados (o MIDI do teclado continua tocando o piano)
    auto still = pos;
    still.setIsPlaying (false);
    const int k = shifted + blockBeats <= 0.0 ? n
                : juce::jlimit (1, n - 1, (int) std::ceil (-shifted / blockBeats * (double) n));

    chordsMidi.addEvents (input, 0, k, 0);
    run (still, 0, k, chordsMidi);
    if (k >= n) return;

    // a intro acabou no meio do bloco: os acordes começam no compasso 1, na amostra certa
    auto go = pos;
    go.setPpqPosition (0.0);
    chordsMidiB.clear();
    chordsMidiB.addEvents (input, k, n - k, -k);
    run (go, k, n - k, chordsMidiB);
    chordsMidi.addEvents (chordsMidiB, 0, n - k, k);
}

//==============================================================================
// Thread da interface: BPM ligado e um só relógio tocando por vez
void MrmProcessor::timerCallback()
{
    const float m = raw ("bpm");
    const float c = getParam (chords.apvts, "bpm");
    const float d = getParam (drums.apvts, "bpm");

    float target = -1.0f;
    auto changed = [] (float a, float b) { return std::abs (a - b) > 0.25f; };   // BPM anda de 1 em 1
    if (changed (m, lastBpm))             target = m;
    else if (changed (c, lastChordsBpm))  target = c;
    else if (changed (d, lastDrumsBpm))   target = d;
    if (target > 0.0f)
    {
        target = juce::jlimit (60.0f, 180.0f, std::round (target));
        setParam (apvts,        "bpm", target);
        setParam (chords.apvts, "bpm", target);
        setParam (drums.apvts,  "bpm", target);
    }
    lastBpm       = raw ("bpm");
    lastChordsBpm = getParam (chords.apvts, "bpm");
    lastDrumsBpm  = getParam (drums.apvts, "bpm");

    // PREVIEW de um gerador ligado enquanto o PLAY RIDDIM toca: o PLAY RIDDIM para.
    // PLAY RIDDIM ligado ou DAW tocando: os PREVIEWs separados desligam.
    const bool cp = getParam (chords.apvts, "preview") > 0.5f;
    const bool dp = getParam (drums.apvts, "preview") > 0.5f;
    if (raw ("play") > 0.5f && ((cp && ! lastChordsPreview) || (dp && ! lastDrumsPreview)))
        setParam (apvts, "play", 0.0f);
    else if (raw ("play") > 0.5f || hostPlaying.load())
    {
        if (cp) setParam (chords.apvts, "preview", 0.0f);
        if (dp) setParam (drums.apvts,  "preview", 0.0f);
    }
    lastChordsPreview = getParam (chords.apvts, "preview") > 0.5f;
    lastDrumsPreview  = getParam (drums.apvts, "preview") > 0.5f;
}

//==============================================================================
juce::String MrmProcessor::suggestedRiddimName() const
{
    juce::StringArray prog;
    for (const auto& s : chords.getSlots())
        if (s.beats > 0) prog.add (juce::String (dub::chordLabel (s)));
    const auto style = (dubdrums::Style) juce::roundToInt (getParam (drums.apvts, "style"));
    const double b = hostBpm.load() > 0.0 ? hostBpm.load() : (double) raw ("bpm");
    juce::String name = "MRM-0420 Riddim " + prog.joinIntoString ("-") + " " + dubdrums::styleName (style)
                      + " " + juce::String (juce::roundToInt (b)) + "bpm";
    return juce::File::createLegalFileName (name.replaceCharacter ('#', 's')) + ".mid";
}

bool MrmProcessor::writeRiddimFile (const juce::File& file) const
{
    const double bpm = hostBpm.load() > 0.0 ? hostBpm.load() : (double) raw ("bpm");
    const auto pattern = drums.getPatternCopy();
    const double intro = pattern.introLengthInBeats;
    const double end = intro + riddimBars() * 4.0;
    const auto tick = [] (double beat) { return std::round (beat * kTicksPerQuarter); };

    juce::MidiMessageSequence meta, drumTrack, skank, bubble, bass;   // baixo guia: só se o Dub Chords mandar
    auto trackName = [] (const juce::String& t) { return juce::MidiMessage::textMetaEvent (3, t); };
    meta.addEvent (trackName ("Mastor Riddim Machine MRM-0420"), 0.0);
    meta.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)), 0.0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    drumTrack.addEvent (trackName ("MRM-0420 Bateria (Dub Drums)"), 0.0);
    skank.addEvent     (trackName ("MRM-0420 Skank (piano)"), 0.0);
    bubble.addEvent    (trackName ("MRM-0420 Bubble (orgao)"), 0.0);
    bass.addEvent      (trackName ("MRM-0420 Baixo guia"), 0.0);
    if (intro > 0.0) meta.addEvent (juce::MidiMessage::textMetaEvent (6, "Intro"), 0.0);

    // nota que passaria do fim do riddim é cortada no fim
    auto addNote = [&] (juce::MidiMessageSequence& seq, int ch, int note, int vel, double on, double len)
    {
        if (on >= end - 1.0e-9) return;
        seq.addEvent (juce::MidiMessage::noteOn (ch, note, (juce::uint8) juce::jlimit (1, 127, vel)), tick (on));
        seq.addEvent (juce::MidiMessage::noteOff (ch, note), tick (juce::jmin (on + len, end)));
    };

    // --- bateria: mesmo canal / MIDI colorido / mapa de notas do Dub Drums
    {
        const int channel = juce::jlimit (1, 16, juce::roundToInt (getParam (drums.apvts, "channel")));
        const bool colour = getParam (drums.apvts, "colorMidi") > 0.5f;
        auto chFor = [&] (int bar) { return ! colour ? channel : (bar < 0 ? 5 : juce::jlimit (1, 4, bar + 1)); };
        for (const auto& e : pattern.intro)
            addNote (drumTrack, chFor (e.bar), drums.mapNote (e.note), e.velocity, e.beat, e.length);
        if (pattern.lengthInBeats > 0.0)
            for (double t0 = intro; t0 < end - 1.0e-9; t0 += pattern.lengthInBeats)
                for (const auto& e : pattern.events)
                    addNote (drumTrack, chFor (e.bar), drums.mapNote (e.note), e.velocity, t0 + e.beat, e.length);
    }

    // --- harmonia: depois da intro, um canal por acorde (colorir por canal na DAW), como no Dub Chords
    {
        double loopBeats = 0.0;
        const auto slots = chords.getSlots();
        const auto events = dub::buildLoop (slots, chords.getSettings(), loopBeats);
        if (loopBeats > 0.0)
            for (double t0 = intro; t0 < end - 1.0e-9; t0 += loopBeats)
            {
                double p = t0;
                for (const auto& sl : slots)
                {
                    if (sl.beats <= 0) continue;
                    if (p < end - 1.0e-9) meta.addEvent (juce::MidiMessage::textMetaEvent (6, juce::String (dub::chordLabel (sl))), tick (p));
                    p += sl.beats;
                }
                for (const auto& e : events)
                {
                    auto& seq = e.channel == dub::ChBass ? bass : e.channel == dub::ChBubble ? bubble : skank;
                    addNote (seq, juce::jlimit (1, 16, e.slot + 1), e.note, e.velocity, t0 + e.start, e.length);
                }
            }
    }

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (kTicksPerQuarter);
    for (auto* seq : { &meta, &drumTrack, &skank, &bubble, &bass })
    {
        if (seq != &meta && seq->getNumEvents() <= 1) continue;   // faixa vazia (ex.: bubble desligado)
        seq->sort();
        seq->updateMatchedPairs();
        seq->addEvent (juce::MidiMessage::endOfTrack(), tick (end));
        mf.addTrack (*seq);
    }

    file.getParentDirectory().createDirectory();
    file.deleteFile();
    {
        juce::FileOutputStream out (file);
        if (! out.openedOk() || ! mf.writeTo (out)) return false;
        out.flush();
    }
    return file.existsAsFile();
}

//==============================================================================
void MrmProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::XmlElement root ("MRM0420");
    root.setAttribute ("version", JucePlugin_VersionString);
    root.setAttribute ("tab", currentTab.load());
    if (auto x = apvts.copyState().createXml())        root.addChildElement (x.release());
    if (auto x = chords.apvts.copyState().createXml()) root.addChildElement (x.release());
    if (auto x = drums.apvts.copyState().createXml())  root.addChildElement (x.release());
    copyXmlToBinary (root, dest);
}

void MrmProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName ("MRM0420")) return;

    currentTab.store (juce::jlimit (0, 1, xml->getIntAttribute ("tab", 0)));
    if (auto* x = xml->getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*x));

    auto restore = [] (juce::AudioProcessor& p, const juce::XmlElement* x)
    {
        if (x == nullptr) return;
        juce::MemoryBlock mb;
        copyXmlToBinary (*x, mb);
        p.setStateInformation (mb.getData(), (int) mb.getSize());
    };
    restore (chords, xml->getChildByName (chords.apvts.state.getType()));
    restore (drums,  xml->getChildByName (drums.apvts.state.getType()));

    // o BPM salvo do MRM manda nos dois
    lastBpm = -1.0f;
}

juce::AudioProcessorEditor* MrmProcessor::createEditor() { return new MrmEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MrmProcessor(); }
