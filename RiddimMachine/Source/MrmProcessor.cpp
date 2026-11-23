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
    static const char* ids[NumMachines] = { "muteDrums", "muteChords", "muteBass" };
    return ids[juce::jlimit (0, NumMachines - 1, m)];
}

bool MrmProcessor::isAudible (int m) const { return isMachineOn (m) && raw (muteId (m)) < 0.5f; }

const char* MrmProcessor::machineOnId (int m)
{
    static const char* ids[NumMachines] = { "drumsOn", "chordsOn", "bassOn" };
    return ids[juce::jlimit (0, NumMachines - 1, m)];
}

bool MrmProcessor::isMachineOn (int m) const { return raw (machineOnId (m)) > 0.5f; }

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

    // preset inicial: o baixo toca junto com a bateria e a harmonia de verdade (o acompanhamento interno
    // dele começa desligado, mas fica à mão na aba BAIXO) e o TAPE ECHO começa desligado
    setParam (bass.apvts, "drumsOn", 0.0f);
    setParam (bass.apvts, "keysOn", 0.0f);
    setParam (knob.apvts, "echoOn", 0.0f);

    // reggae "de livro" no riddim: caixa só no 2 e no 4 e baixo sempre dentro do acorde
    drums.setStrictReggae (true);
    bass.setStrictReggae (true);

    buildLinks();
    syncLinks();
    startTimerHz (20);
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
    knob.setPlayConfigDetails (2, 2, sr, bs);
    for (auto* p : { (juce::AudioProcessor*) &chords, (juce::AudioProcessor*) &drums, (juce::AudioProcessor*) &bass, (juce::AudioProcessor*) &knob })
    {
        p->setRateAndBufferSizeDetails (sr, bs);
        p->prepareToPlay (sr, bs);
    }
    for (auto* b : { &chordsBuf, &drumsBuf, &bassBuf, &mixBuf, &fxBuf })
        b->setSize (2, bs);
    for (auto* m : { &chordsMidi, &drumsMidi, &bassMidi, &scratchMidi })
        m->ensureSize (4096);
    wasMaster = false;
    masterPpq = 0.0;
    for (int m = 0; m < NumMachines; ++m) gains[m] = isAudible (m) ? 1.0f : 0.0f;
    fxGain = raw ("fxOn") > 0.5f ? 1.0f : 0.0f;
}

void MrmProcessor::releaseResources()
{
    chords.releaseResources();
    drums.releaseResources();
    bass.releaseResources();
    knob.releaseResources();
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
        for (auto* b : { &chordsBuf, &drumsBuf, &bassBuf, &mixBuf, &fxBuf })
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
    const double blockBeats = (double) n / sr * bpm / 60.0;
    playBeat.store (playing ? juce::jmax (0.0, *pos.getPpqPosition()) : -1.0);

    // --- bateria: no relógio direto (intro no começo, depois o loop)
    juce::AudioBuffer<float> drumsView (drumsBuf.getArrayOfWritePointers(), 2, 0, n);
    drumsMidi.clear();
    drumsHead.info = pos;
    drums.processBlock (drumsView, drumsMidi);

    // --- harmonia e baixo: entram depois da intro da bateria
    processDelayed (chords, chordsHead, chordsBuf, chordsMidi, scratchMidi, pos, playing, blockBeats, &midi, n);
    processDelayed (bass,   bassHead,   bassBuf,   bassMidi,   scratchMidi, pos, playing, blockBeats, nullptr, n);
    if (master) masterPpq += blockBeats;

    // --- mistura (máquina desligada some com uma rampa curta, sem estalo)
    const juce::AudioBuffer<float>* srcs[NumMachines] = { &drumsBuf, &chordsBuf, &bassBuf };
    mixBuf.clear (0, 0, n);
    mixBuf.clear (1, 0, n);
    for (int m = 0; m < NumMachines; ++m)
    {
        const float target = isAudible (m) ? 1.0f : 0.0f;
        const float g0 = gains[m];
        gains[m] = target;
        if (g0 <= 0.0f && target <= 0.0f) continue;
        for (int ch = 0; ch < 2; ++ch)
            mixBuf.addFromWithRamp (ch, 0, srcs[m]->getReadPointer (ch), n, g0, target);
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

    // Harmonia: a mesma para o MRCM-1240 e o MRBM-69 (o MRCM-1240 só tem maior e menor:
    // um acorde "7" do baixo vira maior e um "m7" vira menor)
    auto chordQualIn = [] (float v) { return juce::roundToInt (v) == dub::Min ? 1 : 0; };
    auto bassQualIn  = [] (float v) { return mbm::isMinorQuality (juce::roundToInt (v)) ? 1 : 0; };
    auto chordQualOut = [] (int c) { return (float) (c == 1 ? dub::Min : dub::Maj); };
    auto bassQualOut  = [] (int c) { return (float) (c == 1 ? mbm::Min : mbm::Maj); };
    for (int i = 0; i < DubChordsProcessor::numSlots; ++i)
    {
        const juce::String n (i);
        links.push_back ({ { { &chords.apvts, "root" + n, identity, plain }, { &bass.apvts, "root" + n, identity, plain } }, {} });
        links.push_back ({ { { &chords.apvts, "dur" + n,  identity, plain }, { &bass.apvts, "dur" + n,  identity, plain } }, {} });
        links.push_back ({ { { &chords.apvts, "qual" + n, chordQualIn, chordQualOut }, { &bass.apvts, "qual" + n, bassQualIn, bassQualOut } }, {} });
        if (i > 0)
            links.push_back ({ { { &chords.apvts, "on" + n, boolIn, plain }, { &bass.apvts, "on" + n, boolIn, plain } }, {} });
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
    juce::AudioProcessorValueTreeState* subs[NumMachines] = { &drums.apvts, &chords.apvts, &bass.apvts };
    bool started = false;
    for (int m = 0; m < NumMachines; ++m)
    {
        const bool on = getParam (*subs[m], "preview") > 0.5f;
        if (on && ! lastPreview[m]) started = true;
    }
    if (raw ("play") > 0.5f && started)
        setParam (apvts, "play", 0.0f);
    else if (raw ("play") > 0.5f || hostPlaying.load())
        for (auto* s : subs)
            if (getParam (*s, "preview") > 0.5f) setParam (*s, "preview", 0.0f);
    for (int m = 0; m < NumMachines; ++m)
        lastPreview[m] = getParam (*subs[m], "preview") > 0.5f;
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
void MrmProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::XmlElement root ("MRM0420");
    root.setAttribute ("version", JucePlugin_VersionString);
    root.setAttribute ("tab", currentTab.load());
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
    copyXmlToBinary (root, dest);
}

void MrmProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName ("MRM0420")) return;

    setCurrentTab (xml->getIntAttribute ("tab", 0));
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

    // o BPM salvo do MRM manda em todas; a harmonia do MRCM-1240 manda no baixo
    for (auto& l : links) std::fill (l.last.begin(), l.last.end(), std::numeric_limits<int>::min());
    lastSeed = juce::roundToInt (raw ("seed"));   // a semente de cada máquina já veio no estado dela
}

juce::AudioProcessorEditor* MrmProcessor::createEditor() { return new MrmEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MrmProcessor(); }
