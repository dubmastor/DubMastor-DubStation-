#include "MrmProcessor.h"
#include "../../RiddimBank/Source/RiddimBank.h"
#include "MrmEditor.h"

namespace
{
    constexpr int kTicksPerQuarter = 960;
    constexpr int kLengths[MrmProcessor::numLengths] = { 4, 8, 16, 32, 64 };
    constexpr int kBassLiveChannel = 2;     // MIDI ao vivo: skank 1, baixo 2, bubble 3, bateria 10

    void setParam (juce::AudioProcessorValueTreeState& s, const juce::String& id, float plainValue)
    {
        if (auto* p = s.getParameter (id))
        {
            const float v = p->convertTo0to1 (plainValue);
            if (std::abs (p->getValue() - v) > 1.0e-6f)
                p->setValueNotifyingHost (v);
        }
    }
    float getParam (const juce::AudioProcessorValueTreeState& s, const juce::String& id)
    {
        return s.getRawParameterValue (id)->load();
    }

    // satura só o que passa de 0,85: a soma das máquinas nunca estoura
    inline float softClip (float x)
    {
        const float a = std::abs (x), knee = 0.85f;
        if (a <= knee) return x;
        return std::copysign (knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee)), x);
    }
}

int MrmProcessor::lengthBars (int index) { return kLengths[juce::jlimit (0, numLengths - 1, index)]; }

const char* MrmProcessor::muteId (int m)
{
    static const char* ids[NumMixer] = { "muteDrums", "muteChords", "muteBass", "muteOrgan" };
    return ids[juce::jlimit (0, NumMixer - 1, m)];
}

bool MrmProcessor::isAudible (int m) const { return isMachineOn (m) && raw (muteId (m)) < 0.5f; }

const char* MrmProcessor::machineOnId (int m)
{
    static const char* ids[NumMixer] = { "drumsOn", "chordsOn", "bassOn", "organOn" };
    return ids[juce::jlimit (0, NumMixer - 1, m)];
}

bool MrmProcessor::isMachineOn (int m) const { return raw (machineOnId (m)) > 0.5f; }

const char* MrmProcessor::volId (int m)  { static const char* ids[NumMixer] = { "volDrums", "volChords", "volBass", "volOrgan" };    return ids[juce::jlimit (0, NumMixer - 1, m)]; }
const char* MrmProcessor::panId (int m)  { static const char* ids[NumMixer] = { "panDrums", "panChords", "panBass", "panOrgan" };    return ids[juce::jlimit (0, NumMixer - 1, m)]; }
const char* MrmProcessor::soloId (int m) { static const char* ids[NumMixer] = { "soloDrums", "soloChords", "soloBass", "soloOrgan" }; return ids[juce::jlimit (0, NumMixer - 1, m)]; }

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MrmProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "play", 1 }, "Play riddim", false));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM",
                                                       NormalisableRange<float> (50.0f, 180.0f, 1.0f), 75.0f));
    StringArray lengths;
    for (int i = 0; i < numLengths; ++i) lengths.add (String (kLengths[i]) + " compassos");
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "riddimBars", 1 }, "Tamanho do riddim", lengths, 2));
    layout.add (std::make_unique<AudioParameterInt>   (ParameterID { "seed", 1 }, "Semente", 1, 999, 420));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "drumsOn", 1 },  "Bateria (MRDM-12)", true));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "chordsOn", 1 }, "Harmonia (MRCM-1240)", true));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "bassOn", 1 },   "Baixo (MRBM-69)", true));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "fxOn", 1 },     "Control Room (Big Knob no master)", true));
    // CONTROL ROOM: mute de cada máquina (só o som; o MIDI continua)
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "muteDrums", 1 },  "Mute DRUMS", false));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "muteChords", 1 }, "Mute RIDDIM (harmonia)", false));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "muteBass", 1 },   "Mute BASS", false));
    // 2.8 · ÓRGÃO (MRO-68): liga junto com as outras máquinas (projetos antigos abrem com ele desligado)
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "organOn", 1 },    "Orgao (MRO-68)", true));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "muteOrgan", 1 },  "Mute ORGAO", false));

    // 2.7 · MIXER: volume, pan e solo de cada máquina (o baixo começa 2 dB mais baixo, como na versão para celular)
    const char* mixNames[NumMixer] = { "BATERIA", "RIDDIM (harmonia)", "BAIXO", "ORGAO" };
    for (int m = 0; m < NumMixer; ++m)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { volId (m), 1 }, String ("Volume ") + mixNames[m],
                        NormalisableRange<float> (-60.0f, 6.0f, 0.1f, 2.5f), m == Bass ? -2.0f : (m == Organ ? -3.0f : 0.0f),
                        AudioParameterFloatAttributes().withLabel ("dB")));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { panId (m), 1 }, String ("Pan ") + mixNames[m],
                        NormalisableRange<float> (-1.0f, 1.0f, 0.01f), m == Organ ? -0.15f : 0.0f));
        layout.add (std::make_unique<AudioParameterBool>  (ParameterID { soloId (m), 1 }, String ("Solo ") + mixNames[m], false));
    }

    // 2.7 · SIRENE com o Mastor Tape Echo MTE-12 (o eco começa LIGADO, nos padrões de fábrica do MTE-12)
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "sirenFire", 1 }, "Sirene FIRE", false));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "sirenHold", 1 }, "Sirene HOLD", false));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { "sirEchoOn", 1 }, "Sirene Tape Echo", true));
    StringArray modes;
    for (int i = 1; i <= 12; ++i) modes.add (String (i));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "sirEchoMode", 1 }, "Sirene Tape Echo Mode", modes, 6));   // Mode 7
    auto unit = [&layout] (const char* id, const char* name, float def)
    { layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f, 0.001f), def)); };
    unit ("sirEchoInput",  "Sirene Echo Input",        0.5f);
    unit ("sirEchoRate",   "Sirene Echo Repeat Rate",  0.5f);
    unit ("sirEchoInt",    "Sirene Echo Intensity",    0.45f);
    unit ("sirEchoVol",    "Sirene Echo Volume",       0.6f);
    unit ("sirEchoRev",    "Sirene Echo Reverb",       0.3f);
    unit ("sirEchoWow",    "Sirene Echo Wow & Flutter", 0.35f);
    unit ("sirEchoAge",    "Sirene Echo Tape Age",     0.3f);
    return layout;
}

MrmProcessor::MrmProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MRM0420Params", createLayout())
{
    chords.setPlayHead (&chordsHead);
    drums.setPlayHead (&drumsHead);
    bass.setPlayHead (&bassHead);
    knob.setPlayHead (&knobHead);
    organ.setPlayHead (&organHead);
    // o órgão segue a harmonia do riddim (não o teclado) e o PLAY dele fica com o MRM
    setParam (organ.apvts, "chordSrc", 0.0f);
    setParam (organ.apvts, "play", 0.0f);

    // preset inicial: o baixo toca junto com a bateria e a harmonia de verdade (o acompanhamento interno
    // dele começa desligado, mas fica à mão na aba BAIXO) e o TAPE ECHO começa desligado
    setParam (bass.apvts, "drumsOn", 0.0f);
    setParam (bass.apvts, "keysOn", 0.0f);
    setParam (knob.apvts, "echoOn", 0.0f);

    // reggae "de livro" no riddim: caixa só no 2 e no 4 e baixo sempre dentro do acorde
    drums.setStrictReggae (true);
    bass.setStrictReggae (true);

    // a sirene do MRM substitui a do Big Knob (o FIRE e o HOLD do Big Knob ficam desligados aqui dentro)
    setParam (knob.apvts, "sirFire", 0.0f);
    setParam (knob.apvts, "sirHold", 0.0f);
    arrAudio = std::make_shared<const mrm::Arrangement> (arrangement);

    buildLinks();
    syncLinks();
    startTimerHz (20);
}

MrmProcessor::~MrmProcessor()
{
    stopTimer();
    if (exportThread != nullptr) exportThread->stopThread (8000);
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
    knob.setPlayConfigDetails (2, 2, sr, bs);
    organ.setPlayConfigDetails (0, 2, sr, bs);
    for (auto* p : { (juce::AudioProcessor*) &chords, (juce::AudioProcessor*) &drums, (juce::AudioProcessor*) &bass, (juce::AudioProcessor*) &knob, (juce::AudioProcessor*) &organ })
    {
        p->setRateAndBufferSizeDetails (sr, bs);
        p->prepareToPlay (sr, bs);
    }
    for (auto* b : { &chordsBuf, &drumsBuf, &bassBuf, &organBuf, &mixBuf, &fxBuf, &sirenBuf, &sirenWet })
        b->setSize (2, bs);
    for (auto* m : { &chordsMidi, &drumsMidi, &bassMidi, &scratchMidi, &spanMidi, &spanIn })
        m->ensureSize (4096);
    siren.prepare (sr);
    sirenEcho.prepare (sr);
    sirenEchoMix = raw ("sirEchoOn") > 0.5f ? 1.0f : 0.0f;
    wasMaster = false;
    masterPpq = 0.0;
    for (int m = 0; m < NumMixer; ++m) gains[m] = targetGain (m, true);
    fxGain = raw ("fxOn") > 0.5f ? 1.0f : 0.0f;
}

void MrmProcessor::releaseResources()
{
    chords.releaseResources();
    drums.releaseResources();
    bass.releaseResources();
    knob.releaseResources();
    organ.releaseResources();
}

int MrmProcessor::riddimBars() const     { return lengthBars ((int) raw ("riddimBars")); }
double MrmProcessor::introBeats() const  { return getParam (drums.apvts, "intro") > 0.5f ? 4.0 : 0.0; }

//==============================================================================
float MrmProcessor::targetGain (int m, bool sectionOn) const
{
    if (! sectionOn || ! isAudible (m)) return 0.0f;
    bool anySolo = false;
    for (int k = 0; k < NumMixer; ++k) anySolo = anySolo || raw (soloId (k)) > 0.5f;
    if (anySolo && raw (soloId (m)) < 0.5f) return 0.0f;
    return juce::Decibels::decibelsToGain (raw (volId (m)), -59.9f);
}

void MrmProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) { midi.clear(); return; }

    if (chordsBuf.getNumSamples() < n)          // bloco maior que o anunciado (raro)
        for (auto* b : { &chordsBuf, &drumsBuf, &bassBuf, &organBuf, &mixBuf, &fxBuf, &sirenBuf, &sirenWet })
            b->setSize (2, n, false, false, true);

    // --- relógio: DAW tocando, ou PLAY RIDDIM com a DAW parada
    juce::Optional<juce::AudioPlayHead::PositionInfo> hostPos;
    if (auto* ph = getPlayHead()) hostPos = ph->getPosition();
    const bool dawPlaying = hostPos.hasValue() && hostPos->getIsPlaying() && hostPos->getPpqPosition().hasValue();
    if (hostPos.hasValue())
        if (auto b = hostPos->getBpm(); b.hasValue() && *b > 0.0) hostBpm.store (*b);
    hostPlaying.store (dawPlaying);

    const bool master = raw ("play") > 0.5f && ! dawPlaying;
    if (master && ! wasMaster) masterPpq = 0.0;
    // PLAY RIDDIM tocando o arranjo: tocar num quadrado pula para aquela parte
    if (const int jump = arrJump.exchange (-1); jump >= 0 && master)
        if (const auto a = std::atomic_load (&arrAudio))
        {
            double start = 0.0;
            for (int k = 0; k < jump && k < (int) a->order.size(); ++k)
                if (const auto* sec = a->sectionAt (k)) start += sec->bars * 4.0;
            masterPpq = introBeats() + start;
        }
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
    if (! pos.getBpm().hasValue() || *pos.getBpm() <= 0.0) pos.setBpm ((double) raw ("bpm"));

    const bool playing = master || dawPlaying;
    const double bpm = *pos.getBpm();
    const double beatsPerSample = bpm / 60.0 / sr;
    const double blockBeats = (double) n * beatsPerSample;
    const double songBeat0 = playing ? *pos.getPpqPosition() : 0.0;
    playBeat.store (playing ? juce::jmax (0.0, songBeat0) : -1.0);

    for (auto* b : { &drumsBuf, &chordsBuf, &bassBuf, &organBuf }) b->clear (0, n);
    drumsMidi.clear(); chordsMidi.clear(); bassMidi.clear();

    // --- ARRANJO: a música vai parte por parte; cada parte reinicia a harmonia e escolhe quem toca.
    // O bloco é dividido nas viradas de parte para cada máquina mudar na amostra certa.
    const auto arr = std::atomic_load (&arrAudio);
    const bool arrOn = arr != nullptr && arr->active() && playing;
    const std::array<bool, NumMachines> all { true, true, true };
    std::array<bool, NumMachines> lastMask = all;
    if (! arrOn)
    {
        arrSlot.store (-1); arrBar.store (-1); arrEnded.store (false);
        renderSpan (0, n, pos, playing, midi, all, master);
    }
    else
    {
        const double intro = introBeats();
        int off = 0;
        while (off < n)
        {
            const double sb = songBeat0 + off * beatsPerSample;
            auto p = pos;
            bool spanPlaying = true;
            std::array<bool, NumMachines> mask = all;
            double boundary;                                  // próxima virada (em semínimas da música)
            if (sb < intro) { p.setPpqPosition (sb); boundary = intro; arrSlot.store (-1); }
            else
            {
                const auto w = arr->find (sb - intro);
                if (w.ended) { spanPlaying = false; mask = { false, false, false }; boundary = 1.0e12; arrSlot.store (-1); arrEnded.store (true); }
                else
                {
                    const double total = arr->totalBars() * 4.0;
                    const double wrapped = arr->loop ? std::fmod (sb - intro, total) : sb - intro;
                    const double local = wrapped - w.start;
                    p.setPpqPosition (intro + local);
                    boundary = sb + (w.end - wrapped);
                    if (const auto* sec = arr->sectionAt (w.slot)) mask = sec->on;
                    arrSlot.store (w.slot); arrBar.store ((int) (local / 4.0)); arrEnded.store (false);
                }
            }
            p.setIsPlaying (spanPlaying);
            int len = n - off;
            if (boundary < songBeat0 + blockBeats)
                len = juce::jlimit (1, n - off, (int) std::ceil ((boundary - sb) / beatsPerSample));
            renderSpan (off, len, p, spanPlaying, midi, mask, master);
            lastMask = mask;
            off += len;
        }
    }
    if (master) masterPpq += blockBeats;

    // --- mistura: volume, pan e solo do MIXER; máquina desligada (ou fora da parte) some com rampa, sem estalo
    // o órgão toca onde a harmonia (RIDDIM) toca
    const juce::AudioBuffer<float>* srcs[NumMixer] = { &drumsBuf, &chordsBuf, &bassBuf, &organBuf };
    mixBuf.clear (0, 0, n);
    mixBuf.clear (1, 0, n);
    for (int m = 0; m < NumMixer; ++m)
    {
        const float pan = raw (panId (m));
        const float pl = pan > 0.0f ? 1.0f - pan : 1.0f, pr = pan < 0.0f ? 1.0f + pan : 1.0f;
        const float target = targetGain (m, lastMask[(size_t) (m == Organ ? Chords : m)]);
        const float g0 = gains[m];
        const float l0 = panL[m], r0 = panR[m];
        gains[m] = target; panL[m] = pl; panR[m] = pr;
        if (g0 <= 0.0f && target <= 0.0f) { meters[(size_t) m].store (meters[(size_t) m].load() * 0.7f); continue; }
        mixBuf.addFromWithRamp (0, 0, srcs[m]->getReadPointer (0), n, g0 * l0, target * pl);
        mixBuf.addFromWithRamp (1, 0, srcs[m]->getReadPointer (1), n, g0 * r0, target * pr);
        const float pk = juce::jmax (srcs[m]->getMagnitude (0, 0, n), srcs[m]->getMagnitude (1, 0, n)) * target;
        meters[(size_t) m].store (juce::jmax (pk, meters[(size_t) m].load() * std::pow (0.05f, (float) n / (float) sr)));
    }

    // --- SIRENE do MRM (oscilador da DUB SIREN do Big Knob) pelo Mastor Tape Echo MTE-12, antes do master
    {
        auto& k = knob.apvts;
        mrm::Siren::Settings st;
        st.pitchHz = getParam (k, "sirPitch"); st.rateHz = getParam (k, "sirRate");
        st.depth = getParam (k, "sirDepth") / 100.0f; st.level = getParam (k, "sirLevel") / 100.0f;
        st.wave = juce::jlimit (0, 4, juce::roundToInt (getParam (k, "sirWave")));
        st.gate = raw ("sirenFire") > 0.5f || raw ("sirenHold") > 0.5f;
        siren.set (st);
        // a fita só roda com a sirene tocando e até 10 s depois (o rabo do eco); parada, não soma chiado no riddim
        if (siren.silent()) sirenIdle += n; else sirenIdle = 0;
        const double idleSec = (double) sirenIdle / sr;
        if (idleSec < 10.0)
        {
            auto* sl = sirenBuf.getWritePointer (0);
            auto* sr2 = sirenBuf.getWritePointer (1);
            for (int i = 0; i < n; ++i) sl[i] = siren.next();
            fitaeco::Params ep;
            ep.input = raw ("sirEchoInput"); ep.mode = juce::jlimit (1, 12, juce::roundToInt (raw ("sirEchoMode")) + 1);
            ep.repeatRate = raw ("sirEchoRate"); ep.intensity = raw ("sirEchoInt"); ep.echoVol = raw ("sirEchoVol");
            ep.reverbVol = raw ("sirEchoRev"); ep.wowFlutter = raw ("sirEchoWow"); ep.tapeAge = raw ("sirEchoAge");
            sirenEcho.setParams (ep);
            auto* wetL = sirenWet.getWritePointer (0);
            auto* wetR = sirenWet.getWritePointer (1);
            sirenEcho.process (sl, sl, wetL, wetR, n);
            const float target = raw ("sirEchoOn") > 0.5f ? 1.0f : 0.0f, e0 = sirenEchoMix;
            sirenEchoMix = target;
            // nos últimos 2 s o rabo some devagar (sem estalo)
            const float f0 = (float) juce::jlimit (0.0, 1.0, (10.0 - idleSec) / 2.0);
            const float f1 = (float) juce::jlimit (0.0, 1.0, (10.0 - idleSec - n / sr) / 2.0);
            for (int i = 0; i < n; ++i)
            {
                const float g = e0 + (target - e0) * (float) i / (float) n;
                const float fade = f0 + (f1 - f0) * (float) i / (float) n;
                const float d = sl[i];
                sl[i] = (d + g * (wetL[i] - d)) * fade;
                sr2[i] = (d + g * (wetR[i] - d)) * fade;
            }
            mixBuf.addFrom (0, 0, sirenBuf, 0, 0, n);
            mixBuf.addFrom (1, 0, sirenBuf, 1, 0, n);
        }
        else if (sirenIdle - n < (juce::int64) (10.0 * sr) + n)
            sirenEcho.reset();   // fita limpa para o próximo disparo
    }

    // --- CONTROL ROOM: Big Knob no master (sempre processa, para o eco não cortar; liga e desliga com rampa)
    {
        for (int ch = 0; ch < 2; ++ch) fxBuf.copyFrom (ch, 0, mixBuf, ch, 0, n);
        juce::AudioBuffer<float> fxView (fxBuf.getArrayOfWritePointers(), 2, 0, n);
        knobHead.info = pos;
        emptyMidi.clear();
        knob.processBlock (fxView, emptyMidi);
        const float target = raw ("fxOn") > 0.5f ? 1.0f : 0.0f;
        const float f0 = fxGain;
        fxGain = target;
        if (f0 > 0.0f || target > 0.0f)
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* dst = mixBuf.getWritePointer (ch);
                const auto* wet = fxBuf.getReadPointer (ch);
                for (int i = 0; i < n; ++i)
                {
                    const float g = f0 + (target - f0) * (float) i / (float) n;
                    dst[i] += g * (wet[i] - dst[i]);
                }
            }
    }

    // --- saída
    const int outCh = buffer.getNumChannels();
    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float l = softClip (mixBuf.getSample (0, i)), r = softClip (mixBuf.getSample (1, i));
        peak = juce::jmax (peak, std::abs (l), std::abs (r));
        if (outCh == 1) buffer.setSample (0, i, 0.5f * (l + r));
        else { buffer.setSample (0, i, l); buffer.setSample (1, i, r); }
    }
    const float lvl = outLevel.load();
    outLevel.store (juce::jmax (peak, lvl * std::pow (0.05f, (float) n / (float) sr)));

    // --- MIDI ao vivo: skank 1, baixo 2, bubble 3, bateria no canal do MRDM-12 (10).
    // No PLAY RIDDIM (DAW parada) nada sai, como no PREVIEW das máquinas separadas.
    // No ARRANJO só sai o MIDI das máquinas que tocam na parte (renderSpan já filtrou).
    midi.clear();
    if (! master)
    {
        if (isMachineOn (Chords)) midi.addEvents (chordsMidi, 0, n, 0);
        if (isMachineOn (Drums))  midi.addEvents (drumsMidi, 0, n, 0);
        if (isMachineOn (Bass))
            for (const auto meta : bassMidi)
            {
                auto m = meta.getMessage();
                if (m.getChannel() > 0) m.setChannel (kBassLiveChannel);
                midi.addEvent (m, meta.samplePosition);
            }
    }
}

// um trecho do bloco com o mesmo relógio e a mesma parte do arranjo
void MrmProcessor::renderSpan (int off, int len, const juce::AudioPlayHead::PositionInfo& pos, bool playing,
                               const juce::MidiBuffer& input, const std::array<bool, NumMachines>& sectionOn, bool master)
{
    juce::ignoreUnused (master);
    const double bpm = pos.getBpm().hasValue() ? *pos.getBpm() : (double) raw ("bpm");
    const double spanBeats = (double) len / sr * bpm / 60.0;
    spanIn.clear();
    spanIn.addEvents (input, off, len, -off);

    // --- bateria: no relógio direto (intro no começo, depois o loop)
    {
        juce::AudioBuffer<float> view (drumsBuf.getArrayOfWritePointers(), 2, off, len);
        spanMidi.clear();
        drumsHead.info = pos;
        drums.processBlock (view, spanMidi);
        if (sectionOn[Drums]) drumsMidi.addEvents (spanMidi, 0, len, off);
    }
    // --- harmonia e baixo: entram depois da intro da bateria
    {
        float* ptrs[2] = { chordsBuf.getWritePointer (0) + off, chordsBuf.getWritePointer (1) + off };
        juce::AudioBuffer<float> view (ptrs, 2, len);
        processDelayed (chords, chordsHead, view, spanMidi, scratchMidi, pos, playing, spanBeats, &spanIn, len);
        if (sectionOn[Chords]) chordsMidi.addEvents (spanMidi, 0, len, off);
    }
    {
        float* ptrs[2] = { bassBuf.getWritePointer (0) + off, bassBuf.getWritePointer (1) + off };
        juce::AudioBuffer<float> view (ptrs, 2, len);
        processDelayed (bass, bassHead, view, spanMidi, scratchMidi, pos, playing, spanBeats, nullptr, len);
        if (sectionOn[Bass]) bassMidi.addEvents (spanMidi, 0, len, off);
    }
    // --- órgão (MRO-68): entra junto com a harmonia, depois da intro; o som vai para a mistura (o órgão não manda MIDI)
    {
        float* ptrs[2] = { organBuf.getWritePointer (0) + off, organBuf.getWritePointer (1) + off };
        juce::AudioBuffer<float> view (ptrs, 2, len);
        processDelayed (organ, organHead, view, spanMidi, scratchMidi, pos, playing, spanBeats, nullptr, len);
        spanMidi.clear();
    }
}

void MrmProcessor::processDelayed (juce::AudioProcessor& proc, SubPlayHead& head, juce::AudioBuffer<float>& out,
                                   juce::MidiBuffer& outMidi, juce::MidiBuffer& scratch,
                                   const juce::AudioPlayHead::PositionInfo& pos, bool playing, double blockBeats,
                                   const juce::MidiBuffer* input, int n)
{
    outMidi.clear();
    const double intro = introBeats();
    const double shifted = playing ? *pos.getPpqPosition() - intro : 0.0;

    auto run = [&] (juce::AudioPlayHead::PositionInfo p, int start, int len, juce::MidiBuffer& m)
    {
        head.info = p;
        juce::AudioBuffer<float> view (out.getArrayOfWritePointers(), 2, start, len);
        proc.processBlock (view, m);
    };

    if (! playing || intro <= 0.0 || shifted >= 0.0)
    {
        auto p = pos;
        if (playing) p.setPpqPosition (shifted);
        if (input != nullptr) outMidi.addEvents (*input, 0, n, 0);
        run (p, 0, n, outMidi);
        return;
    }

    // ainda na intro: a máquina fica parada (o MIDI do teclado continua tocando o piano)
    auto still = pos;
    still.setIsPlaying (false);
    const int k = shifted + blockBeats <= 0.0 ? n
                : juce::jlimit (1, n - 1, (int) std::ceil (-shifted / blockBeats * (double) n));

    if (input != nullptr) outMidi.addEvents (*input, 0, k, 0);
    run (still, 0, k, outMidi);
    if (k >= n) return;

    // a intro acabou no meio do bloco: a máquina começa no compasso 1, na amostra certa
    auto go = pos;
    go.setPpqPosition (0.0);
    scratch.clear();
    if (input != nullptr) scratch.addEvents (*input, k, n - k, -k);
    run (go, k, n - k, scratch);
    outMidi.addEvents (scratch, 0, n - k, k);
}

//==============================================================================
// Harmonia e BPM ligados entre as máquinas
void MrmProcessor::buildLinks()
{
    auto identity = [] (float v) { return juce::roundToInt (v); };
    auto plain    = [] (int c) { return (float) c; };
    auto boolIn   = [] (float v) { return v > 0.5f ? 1 : 0; };
    auto bpmIn    = [] (float v) { return juce::jlimit (50, 180, juce::roundToInt (v)); };

    // BPM: um só para todos
    links.push_back ({ { { &apvts, "bpm", bpmIn, plain }, { &chords.apvts, "bpm", bpmIn, plain },
                         { &drums.apvts, "bpm", bpmIn, plain }, { &bass.apvts, "bpm", bpmIn, plain } }, {} });
    // o BPM do MRO-68 só vale no PLAY dele (vai até 140); tocando, o órgão segue o relógio do MRM
    links.push_back ({ { { &apvts, "bpm", bpmIn, plain }, { &organ.apvts, "bpm", [] (float v) { return juce::roundToInt (v); }, plain } }, {} });

    // Harmonia: a mesma para o MRCM-1240 e o MRBM-69 (o MRCM-1240 só tem maior e menor:
    // um acorde "7" do baixo vira maior e um "m7" vira menor)
    auto chordQualIn = [] (float v) { return juce::roundToInt (v) == dub::Min ? 1 : 0; };
    auto bassQualIn  = [] (float v) { return mbm::isMinorQuality (juce::roundToInt (v)) ? 1 : 0; };
    auto chordQualOut = [] (int c) { return (float) (c == 1 ? dub::Min : dub::Maj); };
    auto bassQualOut  = [] (int c) { return (float) (c == 1 ? mbm::Min : mbm::Maj); };
    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
    {
        const juce::String n (i);
        const juce::String o (i + 1);   // no MRO-68 os acordes contam de 1 a 4 (mesmas tônicas e durações)
        links.push_back ({ { { &chords.apvts, "root" + n, identity, plain }, { &bass.apvts, "root" + n, identity, plain }, { &organ.apvts, "root" + o, identity, plain } }, {} });
        links.push_back ({ { { &chords.apvts, "dur" + n,  identity, plain }, { &bass.apvts, "dur" + n,  identity, plain }, { &organ.apvts, "dur" + o, identity, plain } }, {} });
        links.push_back ({ { { &chords.apvts, "qual" + n, chordQualIn, chordQualOut }, { &bass.apvts, "qual" + n, bassQualIn, bassQualOut }, { &organ.apvts, "minor" + o, boolIn, plain } }, {} });
        if (i > 0)
            links.push_back ({ { { &chords.apvts, "on" + n, boolIn, plain }, { &bass.apvts, "on" + n, boolIn, plain }, { &organ.apvts, "on" + o, boolIn, plain } }, {} });
    }
    for (auto& l : links) l.last.assign (l.points.size(), std::numeric_limits<int>::min());
}

void MrmProcessor::syncLinks()
{
    for (auto& l : links)
    {
        std::vector<int> now;
        for (auto& p : l.points) now.push_back (p.toCommon (getParam (*p.state, p.id)));

        // quem mudou desde a última vez manda; na primeira vez manda o primeiro (o MRM / o MRCM-1240)
        int source = -1;
        for (size_t i = 0; i < now.size() && source < 0; ++i)
            if (now[i] != l.last[i]) source = (int) i;
        if (l.last[0] == std::numeric_limits<int>::min()) source = 0;

        if (source >= 0)
            for (size_t i = 0; i < now.size(); ++i)
                if ((int) i != source && now[i] != now[(size_t) source])
                {
                    auto& p = l.points[i];
                    setParam (*p.state, p.id, p.fromCommon (now[(size_t) source]));
                }

        for (size_t i = 0; i < now.size(); ++i)
            l.last[i] = l.points[i].toCommon (getParam (*l.points[i].state, l.points[i].id));
    }
}

void MrmProcessor::timerCallback()
{
    syncLinks();
    if (! offlineRender) publishLink();

    // semente do JACKPOT: vale para a bateria e para o baixo
    const int seed = juce::roundToInt (raw ("seed"));
    if (seed != lastSeed)
    {
        lastSeed = seed;
        setParam (drums.apvts, "seed", (float) seed);
        setParam (bass.apvts,  "seed", (float) seed);
    }

    // um relógio por vez: PREVIEW de uma máquina ligado durante o PLAY RIDDIM para o PLAY RIDDIM;
    // PLAY RIDDIM ligado ou DAW tocando desligam os PREVIEWs separados
    // (o PREVIEW do órgão é o PLAY do MRO-68)
    juce::AudioProcessorValueTreeState* subs[NumMixer] = { &drums.apvts, &chords.apvts, &bass.apvts, &organ.apvts };
    const char* prevId[NumMixer] = { "preview", "preview", "preview", "play" };
    bool started = false;
    for (int m = 0; m < NumMixer; ++m)
    {
        const bool on = getParam (*subs[m], prevId[m]) > 0.5f;
        if (on && ! lastPreview[m]) started = true;
    }
    if (raw ("play") > 0.5f && started)
        setParam (apvts, "play", 0.0f);
    else if (raw ("play") > 0.5f || hostPlaying.load())
        for (int m = 0; m < NumMixer; ++m)
            if (getParam (*subs[m], prevId[m]) > 0.5f) setParam (*subs[m], prevId[m], 0.0f);
    for (int m = 0; m < NumMixer; ++m)
        lastPreview[m] = getParam (*subs[m], prevId[m]) > 0.5f;
    // o órgão segue sempre a harmonia do riddim
    if (getParam (organ.apvts, "chordSrc") > 0.5f) setParam (organ.apvts, "chordSrc", 0.0f);
}

// harmonia e acorde do momento para o Mastor Tune MT-0420 (ver MrmLink.h)
void MrmProcessor::publishLink()
{
    mrmlink::Data d {};
    const auto slots = chords.getSlots();
    d.numSlots = (juce::int32) juce::jmin (4, (int) slots.size());
    double total = 0.0;
    for (int i = 0; i < d.numSlots; ++i)
    {
        const auto& s = slots[(size_t) i];
        d.root[i]  = ((s.root % 12) + 12) % 12;
        d.minor[i] = s.quality == dub::Min ? 1 : 0;
        d.on[i]    = s.beats > 0.0 ? 1 : 0;
        d.beats[i] = (float) s.beats;
        total += s.beats;
    }
    const double pos = chords.getPlayPositionInLoop();
    d.posBeats = (float) pos;
    d.playing  = pos >= 0.0 ? 1 : 0;
    d.curSlot  = -1;
    if (pos >= 0.0 && total > 0.0)
    {
        double acc = 0.0;
        for (int i = 0; i < d.numSlots; ++i)
        {
            if (! d.on[i]) continue;
            acc += d.beats[i];
            if (pos < acc) { d.curSlot = i; break; }
        }
    }
    d.bpm = getParam (apvts, "bpm");
    linkWriter.write (d);
}

void MrmProcessor::newRiddim()
{
    const int cur = juce::roundToInt (raw ("seed"));
    int s = cur;
    juce::Random rng;
    while (s == cur) s = 1 + rng.nextInt (999);
    if (auto* p = apvts.getParameter ("seed"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) s));
        p->endChangeGesture();
    }
    timerCallback();   // bateria e baixo recebem a semente na hora
}

void MrmProcessor::newLine (int machine)
{
    auto& st = machine == Drums ? drums.apvts : bass.apvts;
    const int cur = juce::roundToInt (getParam (st, "seed"));
    int s = cur;
    juce::Random rng;
    while (s == cur) s = 1 + rng.nextInt (999);
    setParam (st, "seed", (float) s);    // a semente do JACKPOT não muda: a outra máquina fica como está
}

static void setRiddimParam (juce::AudioProcessorValueTreeState& s, const juce::String& id, float v)
{
    if (auto* p = s.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

// PRESET de riddim: a harmonia da música, o BPM sugerido, o estilo da bateria e os músicos das máquinas
// mais próximos de quem gravou (baterista do MRDM-12 e baixista do MRBM-69). Skank sempre no contratempo.
void MrmProcessor::applyRiddimPreset (int index)
{
    if (index < 0 || index >= dubpresets::numPresets) return;
    const auto& pr = dubpresets::presets[index];
    chords.applyPreset (index);
    setRiddimParam (apvts, "bpm", (float) pr.bpm);
    setRiddimParam (drums.apvts, "style", (float) pr.kit);
    drums.applyStyleDefaults (pr.kit);
    setRiddimParam (drums.apvts, "drummer", (float) pr.drummer);
    setRiddimParam (drums.apvts, "backbeat", 0.0f);   // caixa na batida principal
    setRiddimParam (drums.apvts, "fill", 0.0f);
    setRiddimParam (bass.apvts, "style", (float) pr.bassist);
    syncLinks();       // o baixo e a bateria recebem harmonia e BPM novos agora
}

// BANCO ÚNICO DE RIDDIMS: a música inteira (harmonia conferida, BPM, kit, baterista e baixista mais próximos
// de quem gravou) nas três máquinas de uma vez. Caixa na batida principal, como nos presets.
void MrmProcessor::applyBankRiddim (int index)
{
    if (index < 0 || index >= riddimbank::numEntries || riddimbank::entries[index].numChords <= 0) return;
    const auto& e = riddimbank::entries[index];
    chords.applyBankEntry (index);
    setRiddimParam (apvts, "bpm", (float) e.bpm);
    setRiddimParam (drums.apvts, "style", (float) e.kit);
    drums.applyStyleDefaults (e.kit);
    setRiddimParam (drums.apvts, "drummer", (float) e.drummer);
    setRiddimParam (drums.apvts, "backbeat", 0.0f);   // caixa na batida principal
    setRiddimParam (drums.apvts, "fill", 0.0f);
    setRiddimParam (bass.apvts, "style", (float) e.bassStyle);
    setRiddimParam (bass.apvts, "timbre", (float) e.timbre);   // timbre da época: ROOTS, SUPER TONE (digital e ragga) ou 808 (dancehall)
    syncLinks();       // o baixo e a bateria recebem harmonia e BPM novos agora
}

int MrmProcessor::currentBankRiddim() const { return chords.getBankEntry(); }

// ALAVANCA do JACKPOT: um riddim novo inteiro, sempre reggae. Sorteia uma música do BANCO ÚNICO com harmonia
// e kit de reggae (ROCKERS ou ONE DROP: caixa no 2 e no 4), o hi-hat e a semente. Tudo aparece na tela; é só apertar PLAY.
void MrmProcessor::randomRiddim()
{
    juce::Random rng;
    juce::Array<int> pool;
    for (int i = 0; i < riddimbank::numEntries; ++i)
        if (riddimbank::entries[i].numChords > 0 && riddimbank::entries[i].kit <= 1) pool.add (i);
    if (pool.isEmpty()) applyRiddimPreset (rng.nextInt (dubpresets::numPresets));
    else applyBankRiddim (pool[rng.nextInt (pool.size())]);
    setRiddimParam (drums.apvts, "hatPattern", (float) rng.nextInt (3));   // colcheia, semicolcheia ou contratempo
    newRiddim();       // a semente nova gira os rolos do JACKPOT
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

juce::String MrmProcessor::suggestedMachineName (int m) const
{
    juce::StringArray prog;
    for (const auto& s : chords.getSlots())
        if (s.beats > 0) prog.add (juce::String (dub::chordLabel (s)));
    juce::String name;
    if (m == Drums)
        name = "MRM-0420 Bateria " + juce::String (dubdrums::styleName ((dubdrums::Style) juce::roundToInt (getParam (drums.apvts, "style"))))
             + " " + dubdrums::drummerName ((dubdrums::Drummer) juce::roundToInt (getParam (drums.apvts, "drummer")));
    else if (m == Chords)
        name = "MRM-0420 Harmonia " + prog.joinIntoString ("-");
    else
        name = "MRM-0420 Baixo " + juce::String (mbm::styleName (juce::roundToInt (getParam (bass.apvts, "style")))) + " " + prog.joinIntoString ("-");
    return juce::File::createLegalFileName (name.replaceCharacter ('#', 's')) + ".mid";
}

bool MrmProcessor::writeMachineFile (int m, const juce::File& f) const
{
    if (m == Drums)  return drums.writeMidiFile (f);
    if (m == Chords) return chords.writeMidiFile (f);
    return bass.writeMidiFile (f);
}

bool MrmProcessor::writeRiddimFile (const juce::File& file) const
{
    const double bpm = hostBpm.load() > 0.0 ? hostBpm.load() : (double) raw ("bpm");
    const auto pattern = drums.getPatternCopy();
    const double intro = pattern.introLengthInBeats;
    const double end = intro + riddimBars() * 4.0;
    const auto tick = [] (double beat) { return std::round (beat * kTicksPerQuarter); };

    juce::MidiMessageSequence meta, drumTrack, skank, bubble, bassTrack;
    auto trackName = [] (const juce::String& t) { return juce::MidiMessage::textMetaEvent (3, t); };
    meta.addEvent (trackName ("Mastor Riddim Machine MRM-0420"), 0.0);
    meta.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)), 0.0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    drumTrack.addEvent (trackName ("MRM-0420 Bateria (MRDM-12)"), 0.0);
    skank.addEvent     (trackName ("MRM-0420 Skank (MRCM-1240)"), 0.0);
    bubble.addEvent    (trackName ("MRM-0420 Bubble (MRCM-1240)"), 0.0);
    bassTrack.addEvent (trackName ("MRM-0420 Baixo (MRBM-69)"), 0.0);
    if (intro > 0.0) meta.addEvent (juce::MidiMessage::textMetaEvent (6, "Intro"), 0.0);

    // nota que passaria do fim do riddim é cortada no fim
    auto addNote = [&] (juce::MidiMessageSequence& seq, int ch, int note, int vel, double on, double len)
    {
        if (on >= end - 1.0e-9 || on < 0.0) return;
        seq.addEvent (juce::MidiMessage::noteOn (ch, juce::jlimit (0, 127, note), (juce::uint8) juce::jlimit (1, 127, vel)), tick (on));
        seq.addEvent (juce::MidiMessage::noteOff (ch, juce::jlimit (0, 127, note)), tick (juce::jmin (on + juce::jmax (0.01, len), end)));
    };

    // --- bateria: mesmo canal / MIDI colorido / mapa de notas do MRDM-12 (com as edições da grade)
    if (isMachineOn (Drums))
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

    // --- harmonia (com as edições), depois da intro, um canal por acorde (colorir por canal na DAW)
    const auto slots = chords.getSlots();
    double chordLoop = 0.0;
    const auto chordEvents = chords.getCurrentLoop (chordLoop);
    if (chordLoop > 0.0)
        for (double t0 = intro; t0 < end - 1.0e-9; t0 += chordLoop)
        {
            double p = t0;
            for (const auto& sl : slots)
            {
                if (sl.beats <= 0) continue;
                if (p < end - 1.0e-9) meta.addEvent (juce::MidiMessage::textMetaEvent (6, juce::String (dub::chordLabel (sl))), tick (p));
                p += sl.beats;
            }
            if (isMachineOn (Chords))
                for (const auto& e : chordEvents)
                    addNote (e.channel == dub::ChBubble ? bubble : skank, juce::jlimit (1, 16, e.slot + 1),
                             e.note, e.velocity, t0 + e.start, e.length);
        }

    // --- baixo (com as edições), depois da intro, um canal por acorde como a harmonia
    if (isMachineOn (Bass))
    {
        double bassLoop = 0.0;
        const auto line = bass.getCurrentLine (bassLoop);
        if (bassLoop > 0.0)
            for (double t0 = intro; t0 < end - 1.0e-9; t0 += bassLoop)
                for (const auto& e : line)
                    addNote (bassTrack, juce::jlimit (1, 16, e.slot + 1), e.note, e.velocity, t0 + e.start, e.length);
    }

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (kTicksPerQuarter);
    for (auto* seq : { &meta, &drumTrack, &skank, &bubble, &bassTrack })
    {
        if (seq != &meta && seq->getNumEvents() <= 1) continue;   // faixa vazia (máquina desligada, bubble desligado)
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
juce::XmlElement MrmProcessor::buildState (bool withShow)
{
    juce::XmlElement root ("MRM0420");
    root.setAttribute ("version", JucePlugin_VersionString);
    root.setAttribute ("tab", currentTab.load());
    root.setAttribute ("tabs", numTabs);   // 2.8: a aba ÓRGÃO entrou depois da RIDDIM
    if (auto x = apvts.copyState().createXml()) root.addChildElement (x.release());

    // cada máquina salva o estado completo dela (com o MIDI editado), dentro de uma etiqueta própria
    auto store = [&root] (juce::AudioProcessor& p, const char* tag)
    {
        juce::MemoryBlock mb;
        p.getStateInformation (mb);
        auto* e = root.createNewChildElement (tag);
        e->setAttribute ("data", mb.toBase64Encoding());
    };
    store (chords, "MRCM1240");
    store (drums,  "MRDM12");
    store (bass,   "MRBM69");
    store (knob,   "BIGKNOB");
    store (organ,  "MRO68");
    {
        const juce::ScopedLock sl (arrLock);
        if (auto x = arrangement.toTree().createXml()) root.addChildElement (x.release());
    }
    if (withShow)
        if (auto x = showTree.createXml()) root.addChildElement (x.release());
    return root;
}

void MrmProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    copyXmlToBinary (buildState (true), dest);
}

void MrmProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName ("MRM0420")) return;

    {
        int t = xml->getIntAttribute ("tab", 0);
        if (xml->getIntAttribute ("tabs", 8) < 9 && t >= TabOrgan) ++t;   // projetos até a 2.7 (sem a aba ÓRGÃO)
        setCurrentTab (t);
    }
    if (auto* x = xml->getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*x));

    auto restore = [&xml] (juce::AudioProcessor& p, const char* tag)
    {
        if (auto* e = xml->getChildByName (tag))
        {
            juce::MemoryBlock mb;
            if (mb.fromBase64Encoding (e->getStringAttribute ("data")) && mb.getSize() > 0)
                p.setStateInformation (mb.getData(), (int) mb.getSize());
        }
    };
    restore (chords, "MRCM1240");
    restore (drums,  "MRDM12");
    restore (bass,   "MRBM69");
    restore (knob,   "BIGKNOB");
    // ÓRGÃO (2.8+): projetos antigos abrem com ele desligado, para soar igual ao que era
    if (xml->getChildByName ("MRO68") != nullptr) restore (organ, "MRO68");
    else setParam (apvts, "organOn", 0.0f);
    setParam (organ.apvts, "chordSrc", 0.0f);
    setParam (organ.apvts, "play", 0.0f);

    // projetos até a 2.6: a sirene era a do Big Knob; o HOLD ligado passa para a sirene do MRM
    if (getParam (knob.apvts, "sirHold") > 0.5f) setParam (apvts, "sirenHold", 1.0f);
    setParam (knob.apvts, "sirFire", 0.0f);
    setParam (knob.apvts, "sirHold", 0.0f);

    // ARRANJO (2.7+); projetos antigos ganham o arranjo de fábrica, desligado
    if (auto* a = xml->getChildByName ("ARRANJO")) setArrangement (mrm::Arrangement::fromTree (juce::ValueTree::fromXml (*a)));
    else setArrangement (mrm::Arrangement::makeDefault());
    // SHOW: só troca se vier no estado (carregar um riddim do SHOW não apaga o SHOW)
    if (auto* sh = xml->getChildByName ("SHOW")) showTree = juce::ValueTree::fromXml (*sh);

    // o BPM salvo do MRM manda em todas; a harmonia do MRCM-1240 manda no baixo
    for (auto& l : links) std::fill (l.last.begin(), l.last.end(), std::numeric_limits<int>::min());
    lastSeed = juce::roundToInt (raw ("seed"));   // a semente de cada máquina já veio no estado dela
}

//==============================================================================
// ARRANJO
mrm::Arrangement MrmProcessor::getArrangement() const
{
    const juce::ScopedLock sl (arrLock);
    return arrangement;
}

void MrmProcessor::setArrangement (const mrm::Arrangement& a)
{
    {
        const juce::ScopedLock sl (arrLock);
        arrangement = a;
    }
    std::atomic_store (&arrAudio, std::shared_ptr<const mrm::Arrangement> (std::make_shared<const mrm::Arrangement> (a)));
}

//==============================================================================
// SHOW: 10 lugares, cada um com o estado inteiro do MRM (sem o SHOW) em base64
namespace
{
    juce::ValueTree showSlotTree (juce::ValueTree& show, int i, bool create)
    {
        for (auto c : show) if ((int) c.getProperty ("slot", -1) == i) return c;
        if (! create) return {};
        juce::ValueTree c ("SLOT");
        c.setProperty ("slot", i, nullptr);
        show.appendChild (c, nullptr);
        return c;
    }
}

MrmProcessor::ShowSlot MrmProcessor::getShowSlot (int i) const
{
    ShowSlot s;
    auto t = showSlotTree (const_cast<juce::ValueTree&> (showTree), i, false);
    if (t.isValid() && t.hasProperty ("data"))
    {
        s.used = true;
        s.name = t.getProperty ("name").toString();
        s.info = t.getProperty ("info").toString();
    }
    return s;
}

void MrmProcessor::saveShowSlot (int i, const juce::String& name)
{
    if (i < 0 || i >= numShowSlots) return;
    juce::MemoryBlock mb;
    copyXmlToBinary (buildState (false), mb);
    auto t = showSlotTree (showTree, i, true);
    t.setProperty ("name", name.isNotEmpty() ? name : "Riddim " + juce::String (i + 1), nullptr);
    juce::String chordsTxt;
    for (const auto& sl : chords.getSlots())
        if (sl.beats > 0) chordsTxt << (chordsTxt.isEmpty() ? "" : " ") << dub::chordLabel (sl);
    t.setProperty ("info", juce::String (juce::roundToInt (raw ("bpm"))) + " BPM  " + chordsTxt, nullptr);
    t.setProperty ("data", mb.toBase64Encoding(), nullptr);
    showCurrent = i;
}

bool MrmProcessor::loadShowSlot (int i)
{
    auto t = showSlotTree (showTree, i, false);
    if (! t.isValid()) return false;
    juce::MemoryBlock mb;
    if (! mb.fromBase64Encoding (t.getProperty ("data").toString()) || mb.getSize() == 0) return false;
    const int tab = getCurrentTab();
    setStateInformation (mb.getData(), (int) mb.getSize());
    setCurrentTab (tab);
    showCurrent = i;
    return true;
}

void MrmProcessor::renameShowSlot (int i, const juce::String& name)
{
    auto t = showSlotTree (showTree, i, false);
    if (t.isValid() && name.isNotEmpty()) t.setProperty ("name", name, nullptr);
}

void MrmProcessor::clearShowSlot (int i)
{
    auto t = showSlotTree (showTree, i, false);
    if (t.isValid()) showTree.removeChild (t, nullptr);
    if (showCurrent == i) showCurrent = -1;
}

bool MrmProcessor::exportShow (const juce::File& f) const
{
    auto xml = showTree.createXml();
    if (xml == nullptr) return false;
    xml->setAttribute ("app", "MRM-0420");
    return xml->writeTo (f);
}

bool MrmProcessor::importShow (const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName ("SHOW")) return false;
    showTree = juce::ValueTree::fromXml (*xml);
    showCurrent = -1;
    return true;
}

//==============================================================================
// EXPORTAR A MÚSICA: uma cópia do MRM toca o arranjo inteiro fora do tempo real (DAW de mentira)
// e grava o WAV 24 bits e o MIDI de cada máquina (só as partes em que ela toca).
double MrmProcessor::songSeconds() const
{
    const auto a = getArrangement();
    const double bpm = juce::jmax (30.0, (double) raw ("bpm"));
    return (introBeats() + a.totalBars() * 4.0) * 60.0 / bpm;
}

juce::String MrmProcessor::getExportMessage() const
{
    const juce::ScopedLock sl (exportLock);
    return exportMessage;
}

struct MrmProcessor::ExportJob : juce::Thread
{
    ExportJob (MrmProcessor& o, juce::File w, std::unique_ptr<MrmProcessor> c)
        : juce::Thread ("MRM-0420 exportar"), owner (o), wav (std::move (w)), copy (std::move (c)) {}

    struct Head : juce::AudioPlayHead
    {
        double ppq = 0.0, bpm = 75.0;
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            p.setIsPlaying (true);
            p.setBpm (bpm);
            p.setPpqPosition (ppq);
            p.setTimeSignature (TimeSignature { 4, 4 });
            return p;
        }
    };

    void say (const juce::String& m) { const juce::ScopedLock sl (owner.exportLock); owner.exportMessage = m; }

    void run() override
    {
        constexpr double rate = 44100.0;
        constexpr int bs = 512;
        const auto arr = copy->getArrangement();
        Head head;
        head.bpm = juce::jmax (30.0, (double) copy->raw ("bpm"));
        copy->setPlayHead (&head);
        copy->setRateAndBufferSizeDetails (rate, bs);
        copy->prepareToPlay (rate, bs);

        const double songBeats = copy->introBeats() + arr.totalBars() * 4.0;
        const juce::int64 songLen = (juce::int64) std::ceil (songBeats * 60.0 / head.bpm * rate);
        const juce::int64 total = songLen + (juce::int64) (rate * 4.0);   // 4 s de rabo do eco

        juce::AudioBuffer<float> all (2, (int) total);
        juce::AudioBuffer<float> block (2, bs);
        juce::MidiBuffer midi;
        juce::MidiMessageSequence seqs[17];   // por canal
        const double beatsPerSample = head.bpm / 60.0 / rate;
        for (juce::int64 pos = 0; pos < total && ! threadShouldExit(); pos += bs)
        {
            const int len = (int) juce::jmin ((juce::int64) bs, total - pos);
            juce::AudioBuffer<float> view (block.getArrayOfWritePointers(), 2, 0, len);
            head.ppq = (double) pos * beatsPerSample;
            midi.clear();
            copy->processBlock (view, midi);
            for (int ch = 0; ch < 2; ++ch) all.copyFrom (ch, (int) pos, view, ch, 0, len);
            if (pos < songLen)
                for (const auto meta : midi)
                {
                    const auto m = meta.getMessage();
                    if (! (m.isNoteOn() || m.isNoteOff())) continue;
                    const double beat = (double) (pos + meta.samplePosition) * beatsPerSample;
                    seqs[juce::jlimit (1, 16, m.getChannel())].addEvent (m, std::round (beat * kTicksPerQuarter));
                }
            if ((pos / bs) % 64 == 0)
            {
                owner.exportProgress.store (0.95f * (float) pos / (float) total);
                say (juce::String::fromUTF8 ("Gravando a m\xc3\xba" "sica\xe2\x80\xa6 ") + juce::String (juce::roundToInt (100.0 * pos / total)) + "%");
            }
        }
        copy->releaseResources();
        if (threadShouldExit()) { finish ({}); return; }

        // corta o silêncio do fim (deixa 1/4 s)
        int end = (int) total;
        while (end > (int) songLen && all.getMagnitude (0, end - 1, 1) < 1.0e-4f && all.getMagnitude (1, end - 1, 1) < 1.0e-4f) --end;
        end = juce::jmin ((int) total, end + (int) (rate / 4));

        say ("Salvando o WAV...");
        bool ok = false;
        wav.deleteFile();
        if (auto out = std::unique_ptr<juce::OutputStream> (wav.createOutputStream()))
        {
            juce::WavAudioFormat fmt;
            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (fmt.createWriterFor (out.get(), rate, 2, 24, {}, 0)))
            {
                out.release();
                ok = w->writeFromAudioSampleBuffer (all, 0, end);
            }
        }

        // MIDI do arranjo: uma faixa por canal (skank 1, baixo 2, bubble 3, bateria 10)
        juce::MidiFile mf;
        mf.setTicksPerQuarterNote (kTicksPerQuarter);
        juce::MidiMessageSequence meta;
        meta.addEvent (juce::MidiMessage::textMetaEvent (3, "Mastor Riddim Machine MRM-0420 (arranjo)"), 0.0);
        meta.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / head.bpm)), 0.0);
        meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
        double acc = copy->introBeats();
        for (int k = 0; k < (int) arr.order.size(); ++k)
            if (const auto* sec = arr.sectionAt (k))
            {
                meta.addEvent (juce::MidiMessage::textMetaEvent (6, sec->name), std::round (acc * kTicksPerQuarter));
                acc += sec->bars * 4.0;
            }
        mf.addTrack (meta);
        for (int ch = 1; ch <= 16; ++ch)
        {
            auto& sq = seqs[ch];
            if (sq.getNumEvents() == 0) continue;
            const juce::String nm = ch == 1 ? "MRM-0420 Skank (MRK-80)" : ch == 2 ? "MRM-0420 Baixo (MRB-69)" : ch == 3 ? "MRM-0420 Bubble (MRK-80)"
                                  : "MRM-0420 Bateria (MRD-12) canal " + juce::String (ch);
            sq.addEvent (juce::MidiMessage::textMetaEvent (3, nm), 0.0);
            sq.sort();
            sq.updateMatchedPairs();
            mf.addTrack (sq);
        }
        const auto mid = wav.withFileExtension ("mid");
        mid.deleteFile();
        { juce::FileOutputStream os (mid); if (os.openedOk()) mf.writeTo (os); }

        finish (ok ? juce::String::fromUTF8 ("Pronto: ") + wav.getFileName() + " + " + mid.getFileName()
                   : juce::String::fromUTF8 ("N\xc3\xa3o consegui salvar o WAV"));
    }

    void finish (const juce::String& msg)
    {
        say (msg);
        owner.exportProgress.store (-1.0f);
    }

    MrmProcessor& owner;
    juce::File wav;
    std::unique_ptr<MrmProcessor> copy;
};

bool MrmProcessor::startSongExport (const juce::File& wav)
{
    if (exportProgress.load() >= 0.0f) return false;
    if (exportThread != nullptr) exportThread->stopThread (2000);
    exportProgress.store (0.0f);
    {
        const juce::ScopedLock sl (exportLock);
        exportMessage = juce::String::fromUTF8 ("Gravando a m\xc3\xba" "sica\xe2\x80\xa6");
    }
    // a cópia é montada aqui, na thread de mensagens, com o estado de agora e o arranjo ligado do começo ao fim
    auto copy = std::make_unique<MrmProcessor>();
    copy->offlineRender = true;
    copy->stopTimer();
    {
        juce::MemoryBlock mb;
        copyXmlToBinary (buildState (false), mb);
        copy->setStateInformation (mb.getData(), (int) mb.getSize());
    }
    auto arr = copy->getArrangement();
    arr.on = true; arr.loop = false;
    copy->setArrangement (arr);
    setParam (copy->apvts, "play", 0.0f);
    for (int i = 0; i < 4; ++i) copy->syncLinks();
    exportThread = std::make_unique<ExportJob> (*this, wav, std::move (copy));
    exportThread->startThread();
    return true;
}

juce::AudioProcessorEditor* MrmProcessor::createEditor() { return new MrmEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MrmProcessor(); }
