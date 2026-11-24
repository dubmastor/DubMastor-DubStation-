#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using mpg::kSteps;

namespace
{
    StringArray names (const std::vector<mpg::Line>& v) { StringArray a; for (auto& x : v) a.add (String::fromUTF8 (x.name)); return a; }
    StringArray names (const std::vector<mpg::Shape>& v) { StringArray a; for (auto& x : v) a.add (String::fromUTF8 (x.name)); return a; }
    AudioParameterFloatAttributes pct() { return AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + "%"; }); }
    NormalisableRange<float> freqRange() { NormalisableRange<float> r (20.0f, 20000.0f); r.setSkewForCentre (1000.0f); return r; }
}

AudioProcessorValueTreeState::ParameterLayout PickGuitarProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto ch = [&] (const char* id, const char* name, StringArray items, int def) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, String::fromUTF8 (name), items, def)); };
    auto fl = [&] (const char* id, const char* name, float lo, float hi, float def, float step = 0.0f)
    { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, String::fromUTF8 (name), NormalisableRange<float> (lo, hi, step), def, pct())); };
    auto on = [&] (const char* id, const char* name, bool def) { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, String::fromUTF8 (name), def)); };

    const mpg::Params f;   // padrão de fábrica
    StringArray keys; for (int i = 0; i < 12; ++i) keys.add (mpg::noteName (i));
    StringArray scales; for (int i = 0; i < mpg::kNumScales; ++i) scales.add (String::fromUTF8 (mpg::scaleName (i)));
    StringArray musicos; for (auto& p : mpg::presets()) musicos.add (String::fromUTF8 (p.name));
    StringArray syncs; for (int i = 0; i < mpg::kNumSync; ++i) syncs.add (String::fromUTF8 (mpg::echoSyncName (i)));

    // linha
    ch ("mode", "Modo", { "Follow", "Groove", "Counter" }, f.mode);
    ch ("src", "Entrada do baixo", { String::fromUTF8 ("MIDI (MRB-69 · MRM-0420 · teclado)"), "Linha guia" }, f.source);
    ch ("line", "Linha guia", names (mpg::lines()), f.line);
    ch ("oct", "Oitava da palheta", { String::fromUTF8 ("Uníssono"), "+1 oitava", "+2 oitavas" }, f.oct / 12);
    ch ("shape", "Desenho do Counter", names (mpg::shapes()), f.shape);
    ch ("key", "Tom", keys, f.key);
    ch ("scale", "Escala", scales, f.scale);
    ch ("musico", "Guitarrista", musicos, mpg::kDefaultPreset);
    for (int i = 0; i < kSteps; ++i)
        ch (("g" + String (i + 1)).toRawUTF8(), ("Grade " + String (i + 1)).toRawUTF8(), { String::fromUTF8 ("pausa"), "abafada", "acento", "aberta", "pica-pau" },
            String (kCells).indexOfChar (f.grid[(size_t) i]));
    // toque
    fl ("gate", "Duração", 8, 100, f.gate, 1);
    fl ("human", "Humano", 0, 60, f.human, 1);
    fl ("swing", "Swing", 0, 60, f.swing, 1);
    fl ("acc", "Acento", 0, 100, f.acc, 1);
    // pica-pau
    ch ("art", "Notas", { String::fromUTF8 ("Mantém"), "Pica-pau" }, f.art);
    ch ("rep", "Repetições por semicolcheia", { "3 · tercinas", "2", "4" }, indexFromRep (f.rep));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "dose", 1 }, "Dose do pica-pau", NormalisableRange<float> (0.0f, 10.0f, 0.5f), f.dose,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + "%"; })));
    fl ("ppSwing", "Swing do pica-pau", 0, 100, f.ppSwing, 1);
    // timbre
    ch ("pickup", "Captador", { String::fromUTF8 ("Braço"), "Meio", "Ponte" }, f.pickup);
    fl ("mute", "Abafado", 0, 100, f.mute, 1);
    fl ("pos", "Posição da palheta", 0, 100, f.pos, 1);
    fl ("atk", "Ataque", 0, 100, f.atk, 1);
    fl ("drive", "Drive", 0, 100, f.drive, 1);
    fl ("tone", "Tom do amp", 0, 100, f.tone, 1);
    fl ("body", "Corpo", 0, 100, f.body, 1);
    // EQ
    on ("eqOn", "EQ", f.eqOn);
    for (int i = 0; i < mpg::kEqBands; ++i)
    {
        const String n = String::fromUTF8 (mpg::eqName (i));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { "eqF" + String (i + 1), 1 }, "EQ " + n + " Hz", freqRange(), f.eq[(size_t) i].f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { "eqG" + String (i + 1), 1 }, "EQ " + n + " dB", NormalisableRange<float> (-15.0f, 15.0f, 0.5f), f.eq[(size_t) i].g));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { "eqQ" + String (i + 1), 1 }, "EQ " + n + " Q", NormalisableRange<float> (0.3f, 8.0f), f.eq[(size_t) i].q));
    }
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bk", 1 }, "Mastor Big Knob", NormalisableRange<float> (-100.0f, 100.0f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            if (std::abs (v) < 1.5f) return String ("NEUTRO");
            const float hz = v < 0 ? mpg::bigKnobLP (v) : mpg::bigKnobHP (v);
            return String (v < 0 ? "LPF " : "HPF ") + (hz < 1000 ? String (roundToInt (hz)) + " Hz" : String (hz / 1000.0f, 1) + " kHz");
        })));
    // saturador e Tape Echo (padrão de fábrica: os dois em bypass)
    on ("satOn", "Saturador", false);
    ch ("satType", "Tipo do saturador", { "Fita", String::fromUTF8 ("Válvula"), "Transistor" }, f.satType);
    fl ("satDrive", "Drive do saturador", 0, 100, f.satDrive, 1);
    fl ("satMix", "Mix do saturador", 0, 100, f.satMix, 1);
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "satOut", 1 }, String::fromUTF8 ("Saída do saturador"), NormalisableRange<float> (-12.0f, 6.0f, 0.1f), 0.0f));
    on ("echoOn", "Tape Echo", false);
    ch ("echoSync", "Tempo do eco", syncs, f.echoSync);
    fl ("echoFb", "Repetições", 0, 100, f.echoFb, 1);
    fl ("echoMix", "Mix do eco", 0, 100, f.echoMix, 1);
    fl ("spring", "Mola", 0, 100, f.spring, 1);
    // saída e transporte
    fl ("master", "Master", 0, 100, f.master, 1);
    on ("power", "Power", true);
    on ("guideBass", "Baixo guia", true);
    on ("play", "Play (sem a DAW)", false);
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM (sem a DAW)", NormalisableRange<float> (50.0f, 100.0f, 1.0f), 76.0f));
    return l;
}

PickGuitarProcessor::PickGuitarProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MPG11", createLayout())
{
    for (auto& e : edits) e.store (0);
}

const String PickGuitarProcessor::getProgramName (int index)
{
    if (index < 0 || index >= getNumPrograms()) return {};
    return String::fromUTF8 (mpg::presets()[(size_t) index].name);
}

void PickGuitarProcessor::setParam (const String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
}

void PickGuitarProcessor::setCell (int i, char c)
{
    const int idx = String (kCells).indexOfChar (c);
    setParam ("g" + String (i + 1), (float) jmax (0, idx));
}

void PickGuitarProcessor::cycleCell (int i)
{
    const int idx = String (kCells).indexOfChar (cell (i));
    setParam ("g" + String (i + 1), (float) ((idx + 1) % 5));
}

void PickGuitarProcessor::toggleEdit (int cyc)
{
    const auto p = makeParams();
    std::array<bool, mpg::kCycle> autoSet {};
    mpg::autoPicaPau (p.grid, p.dose, autoSet);
    const bool now = mpg::isPicaPau (p, autoSet, cyc);
    auto& e = edits[(size_t) cyc];
    if (now) e.store (e.load() == 1 ? 0 : 2);
    else     e.store (e.load() == 2 ? 0 : 1);
}

void PickGuitarProcessor::applyMusician (int index)
{
    index = jlimit (0, getNumPrograms() - 1, index);
    mpg::Params p = makeParams();
    mpg::applyPreset (p, index);
    const auto& P = mpg::presets()[(size_t) index];
    setParam ("musico", (float) index);
    setParam ("mode", (float) p.mode);
    setParam ("line", (float) p.line);
    setParam ("oct", (float) (p.oct / 12));
    setParam ("shape", (float) p.shape);
    setParam ("pickup", (float) p.pickup);
    setParam ("mute", p.mute); setParam ("pos", p.pos); setParam ("atk", p.atk);
    setParam ("drive", p.drive); setParam ("tone", p.tone); setParam ("body", p.body);
    setParam ("human", p.human); setParam ("acc", p.acc); setParam ("gate", p.gate); setParam ("swing", p.swing);
    setParam ("echoSync", (float) p.echoSync); setParam ("echoFb", p.echoFb); setParam ("echoMix", p.echoMix); setParam ("spring", p.spring);
    setParam ("art", (float) p.art); setParam ("rep", (float) indexFromRep (p.rep)); setParam ("ppSwing", p.ppSwing);
    setGrid (P.grid);
    clearEdits();
}

void PickGuitarProcessor::applyEqPreset (int index)
{
    const auto& s = mpg::eqPresets()[(size_t) jlimit (0, (int) mpg::eqPresets().size() - 1, index)].set;
    for (int i = 0; i < mpg::kEqBands; ++i)
    {
        setParam ("eqF" + String (i + 1), s[(size_t) i].f);
        setParam ("eqG" + String (i + 1), s[(size_t) i].g);
        setParam ("eqQ" + String (i + 1), s[(size_t) i].q);
    }
    setParam ("eqOn", 1.0f);
}

mpg::Params PickGuitarProcessor::makeParams() const
{
    mpg::Params p;
    auto i = [&] (const char* id) { return (int) get (id); };
    p.mode = i ("mode"); p.source = i ("src"); p.line = i ("line"); p.oct = 12 * i ("oct"); p.shape = i ("shape");
    p.key = i ("key"); p.scale = i ("scale");
    p.gate = get ("gate"); p.human = get ("human"); p.swing = get ("swing"); p.acc = get ("acc");
    p.art = i ("art"); p.rep = repFromIndex (i ("rep")); p.dose = get ("dose"); p.ppSwing = get ("ppSwing");
    p.pickup = i ("pickup"); p.mute = get ("mute"); p.pos = get ("pos"); p.atk = get ("atk");
    p.drive = get ("drive"); p.tone = get ("tone"); p.body = get ("body");
    p.eqOn = get ("eqOn") > 0.5f;
    for (int b = 0; b < mpg::kEqBands; ++b)
        p.eq[(size_t) b] = { get ("eqF" + String (b + 1)), get ("eqG" + String (b + 1)), get ("eqQ" + String (b + 1)) };
    p.bk = get ("bk");
    p.satOn = get ("satOn") > 0.5f; p.satType = i ("satType"); p.satDrive = get ("satDrive"); p.satMix = get ("satMix"); p.satOut = get ("satOut");
    p.echoOn = get ("echoOn") > 0.5f; p.echoSync = i ("echoSync"); p.echoFb = get ("echoFb"); p.echoMix = get ("echoMix"); p.spring = get ("spring");
    p.throwOn = throwOn.load();
    p.master = get ("master"); p.power = get ("power") > 0.5f; p.guideBass = get ("guideBass") > 0.5f;
    for (int c = 0; c < kSteps; ++c) p.grid[(size_t) c] = cell (c);
    for (int c = 0; c < mpg::kCycle; ++c) p.edits[(size_t) c] = edits[(size_t) c].load();
    return p;
}

bool PickGuitarProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == AudioChannelSet::stereo() || out == AudioChannelSet::mono();
}

void PickGuitarProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate;
    engine.prepare (sampleRate);
    engine.setParams (makeParams());
    midiIn.clear(); midiIn.reserve (256);
    work.setSize (2, jmax (1, samplesPerBlock));
    previewPpq = 0.0;
}

void PickGuitarProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) return;
    if (work.getNumSamples() < n) work.setSize (2, n, false, false, true);
    work.clear();

    // teclado da tela junto com o MIDI da DAW (a nota do baixo)
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    midiIn.clear();
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn()) midiIn.push_back ({ jlimit (0, n - 1, meta.samplePosition), m.getNoteNumber(), m.getFloatVelocity(), true });
    }

    // transporte: a DAW tocando, ou o PLAY interno com a DAW parada
    mpg::Transport t;
    bool hostPlaying = false;
    double bpm = 0.0, ppq = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            hostPlaying = pos->getIsPlaying();
        }
    const bool preview = get ("play") > 0.5f && ! hostPlaying;
    if (preview) { bpm = (double) get ("bpm"); ppq = previewPpq; previewPpq += (double) n / sampleRateHz * bpm / 60.0; }
    else previewPpq = 0.0;
    t.playing = (hostPlaying || preview) && bpm > 0.0;
    t.ppq = ppq; t.bpm = bpm > 0.0 ? bpm : (double) get ("bpm");

    engine.setParams (makeParams());
    engine.process (work.getWritePointer (0), work.getWritePointer (1), n, t, midiIn);

    const int outCh = buffer.getNumChannels();
    if (outCh >= 2) { buffer.copyFrom (0, 0, work, 0, 0, n); buffer.copyFrom (1, 0, work, 1, 0, n); }
    else if (outCh == 1) { buffer.addFrom (0, 0, work, 0, 0, n, 0.5f); buffer.addFrom (0, 0, work, 1, 0, n, 0.5f); }

    // VU e osciloscópio
    const float* l = work.getReadPointer (0);
    const float* r = work.getReadPointer (1);
    float ms = 0.0f;
    int w = scopeWrite.load();
    for (int i = 0; i < n; ++i)
    {
        const float m = 0.5f * (l[i] + r[i]);
        ms += m * m;
        scope[(size_t) w] = m;
        w = (w + 1) % kScope;
    }
    scopeWrite.store (w);
    meter.store (0.8f * meter.load() + 0.2f * (ms / (float) n));
}

void PickGuitarProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    StringArray e;
    for (int i = 0; i < mpg::kCycle; ++i) e.add (String (edits[(size_t) i].load()));
    state.setProperty ("ppEdits", e.joinIntoString (""), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void PickGuitarProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            const String e = tree.getProperty ("ppEdits").toString();
            for (int i = 0; i < mpg::kCycle; ++i) edits[(size_t) i].store ((int8_t) (i < e.length() ? jlimit (0, 2, e[i] - '0') : 0));
            apvts.replaceState (tree);
        }
}

AudioProcessorEditor* PickGuitarProcessor::createEditor() { return new PickGuitarEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PickGuitarProcessor(); }
