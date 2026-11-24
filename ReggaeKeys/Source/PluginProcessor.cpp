#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../../ReggaeChordMachine/Source/PluginProcessor.h"   // CHORD MACHINE (MRCM-1240)

namespace
{
// Presets de fábrica — drawbars na ordem 16' 5⅓' 8' 4' 2⅔' 2' 1⅗' 1⅓' 1'
const Preset kPresets[] = {
    // ---------------- PIANO (só mexe nos parâmetros do piano)
    //  nome                         modo  brilho decay chorus nívelP   (campos do órgão ignorados)                                                 reverb
    { "Piano: M1 Classic",              0, 0.65f, 0.55f, 0.50f, 0.80f, { 0,0,0,0,0,0,0,0,0 }, false, true, true, 0, 0, 0, 0, false, 0.20f },
    { "Piano: M1 Dub Skank",            0, 0.85f, 0.20f, 0.65f, 0.85f, { 0,0,0,0,0,0,0,0,0 }, false, true, true, 0, 0, 0, 0, false, 0.35f },
    { "Piano: M1 Bright House",         0, 1.00f, 0.45f, 0.55f, 0.80f, { 0,0,0,0,0,0,0,0,0 }, false, true, true, 0, 0, 0, 0, false, 0.25f },
    { "Piano: M1 Soft Ballad",          0, 0.35f, 0.80f, 0.40f, 0.85f, { 0,0,0,0,0,0,0,0,0 }, false, true, true, 0, 0, 0, 0, false, 0.30f },
    // ---------------- ORGAN (só mexe nos parâmetros do órgão)
    //  nome                         modo  (campos do piano ignorados)   drawbars                     perc  3ª    fast  click  drive  lesMix nívelO lesFast reverb
    { "Organ: Reggae Bubble",           1, 0, 0, 0, 0,                  { 8,8,8,0,0,0,0,0,0 }, false, true, true, 0.40f, 0.25f, 0.85f, 0.75f, false, 0.18f },
    { "Organ: Roots Percussion",        1, 0, 0, 0, 0,                  { 8,8,8,0,0,0,0,0,0 }, true,  true, true, 0.50f, 0.30f, 0.80f, 0.75f, false, 0.18f },
    { "Organ: Dub Dark",                1, 0, 0, 0, 0,                  { 8,0,8,0,0,0,0,0,0 }, false, true, true, 0.25f, 0.15f, 0.90f, 0.80f, false, 0.40f },
    { "Organ: Gospel Leslie Fast",      1, 0, 0, 0, 0,                  { 8,8,8,8,0,0,0,0,8 }, false, true, true, 0.45f, 0.40f, 0.90f, 0.65f, true,  0.25f },
};
constexpr int kNumPianoPresets = 4;
constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
} // namespace

const juce::StringArray& RootsKeysProcessor::drawbarNames()
{
    static const juce::StringArray names { "16'", "5 1/3'", "8'", "4'", "2 2/3'", "2'", "1 3/5'", "1 1/3'", "1'" };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout RootsKeysProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (const char* id, const char* name, float def)
    { return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def); };

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode", StringArray { "Piano", "Organ" }, 0));

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "pianoType", 1 }, "Piano Timbre",
                                                   StringArray { "M1", "Grand Piano", "MT-40", "FM EP", "Yamaha CP-80", "Yamaha C7", "M1 Grand Piano",
                                                                 "M1 Piano 16 Comp", "Honky Tonk", "Casio CZ-101",
                                                                 "Rhodes Stage", "Rhodes Suitcase", "Wurlitzer 200A" }, 7));   // de fábrica: o Piano 16 do Korg M1 comprimido (novos timbres sempre no fim da lista: projetos antigos guardam o índice)
    l.add (pct ("pianoBright", "Piano Brightness", 0.65f));
    l.add (pct ("pianoDecay",  "Piano Decay",      0.55f));
    l.add (pct ("pianoChorus", "Piano Chorus",     0.50f));
    l.add (pct ("pianoLevel",  "Piano Level",      0.80f));

    const int defDb[9] = { 8, 8, 8, 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < 9; ++i)
        l.add (std::make_unique<AudioParameterInt> (ParameterID { "db" + String (i + 1), 1 },
                                                    "Drawbar " + drawbarNames()[i], 0, 8, defDb[i]));

    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "percOn", 1 },   "Percussion", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "percHarm", 1 }, "Perc Harmonic", StringArray { "2nd", "3rd" }, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "percDecay", 1 },"Perc Decay", StringArray { "Fast", "Slow" }, 0));
    l.add (pct ("click",      "Key Click",    0.35f));
    l.add (pct ("drive",      "Overdrive",    0.20f));
    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "leslieFast", 1 }, "Leslie Fast", false));
    l.add (pct ("leslieMix",  "Leslie Mix",   0.85f));
    l.add (pct ("organLevel", "Organ Level",  0.70f));

    // Tape Echo — FitaEco TE-12 (mesmo DSP do Dub Station 2.1)
    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "echoOn", 1 }, "Echo On", false));
    {
        StringArray modes;
        for (int i = 1; i <= 12; ++i) modes.add ("Mode " + String (i));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { "echoMode", 1 }, "Echo Mode", modes, 6));
    }
    l.add (pct ("echoRate",      "Echo Repeat Rate", 0.50f));
    l.add (pct ("echoIntensity", "Echo Intensity",   0.45f));
    l.add (pct ("echoVol",       "Echo Volume",      0.60f));
    l.add (pct ("echoSpring",    "Echo Spring",      0.30f));
    l.add (pct ("echoWow",       "Echo Wow/Flutter", 0.35f));
    l.add (pct ("echoAge",       "Echo Tape Age",    0.30f));

    // Flanger estilo dub (Mad Professor)
    l.add (std::make_unique<AudioParameterBool>  (ParameterID { "flOn", 1 }, "Flanger On", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "flRate", 1 }, "Flanger Rate",
                                                  NormalisableRange<float> (0.02f, 4.0f, 0.0f, 0.35f), 0.12f,
                                                  AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (pct ("flDepth",  "Flanger Depth",  0.80f));
    l.add (pct ("flManual", "Flanger Manual", 0.30f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "flFeedback", 1 }, "Flanger Feedback",
                                                  NormalisableRange<float> (-0.95f, 0.95f), 0.65f));
    l.add (pct ("flMix",    "Flanger Mix",    0.50f));

    l.add (pct ("reverb", "Reverb", 0.20f));
    l.add (pct ("master", "Master", 0.80f));

    // Maximizer
    l.add (std::make_unique<AudioParameterBool>  (ParameterID { "maxOn", 1 }, "Maximizer On", true));
    l.add (std::make_unique<AudioParameterBool>  (ParameterID { "gtrOn", 1 }, "Guitar Skank On", false));   // sempre começa desligado
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "maxGain", 1 }, "Maximizer Gain",
                                                  NormalisableRange<float> (0.0f, 18.0f, 0.1f), 3.0f,
                                                  AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "maxCeiling", 1 }, "Maximizer Ceiling",
                                                  NormalisableRange<float> (-12.0f, 0.0f, 0.1f), -0.3f,
                                                  AudioParameterFloatAttributes().withLabel ("dB")));

    // CHORD MACHINE (MRCM-1240): os acordes tocam junto com a DAW (o PREVIEW da aba toca sempre). No fim da lista:
    // os parâmetros antigos não mudam de lugar.
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "machineOn", 1 }, "Chord Machine On", false));
    return l;
}

RootsKeysProcessor::RootsKeysProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout()),
      machine (std::make_unique<DubChordsProcessor>())
{
    for (int i = 0; i < 9; ++i)
        dbParam[i] = apvts.getRawParameterValue ("db" + juce::String (i + 1));
    // instância nova: os acordes da machine tocam no som deste instrumento (SOM INTERNO da machine desligado)
    if (auto* prm = machine->apvts.getParameter ("sound")) prm->setValueNotifyingHost (0.0f);
}

RootsKeysProcessor::~RootsKeysProcessor() = default;

bool RootsKeysProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void RootsKeysProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    machine->setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);
    machine->prepareToPlay (sampleRate, samplesPerBlock);
    machineBuf.setSize (2, juce::jmax (1, samplesPerBlock));
    machineMidi.ensureSize (2048);
    engine.prepare (sampleRate);
    setLatencySamples (engine.latencySamples());   // lookahead do maximizer
    keyboardState.reset();
}

void RootsKeysProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
    if (buffer.getNumChannels() < 2) { buffer.clear(); return; }

    // rodas mexidas na tela
    {
        const float pb = uiPitchReq.exchange (kNoWheelReq);
        if (pb > kNoWheelReq + 1.0f) { engine.setPitchBend (pb); pitchWheel.store (pb); }
        const float mw = uiModReq.exchange (kNoWheelReq);
        if (mw > kNoWheelReq + 1.0f) { engine.setModWheel (mw); modWheel.store (mw); }
    }

    // parâmetros -> motor
    rk::Params& p = engine.params;
    p.mode        = (int) get ("mode");
    p.pianoType   = (int) get ("pianoType");
    p.pianoBright = get ("pianoBright");
    p.pianoDecay  = get ("pianoDecay");
    p.pianoChorus = get ("pianoChorus");
    p.pianoLevel  = get ("pianoLevel");
    for (int i = 0; i < 9; ++i)
        p.drawbar[i] = dbParam[i]->load();
    p.percOn     = get ("percOn") > 0.5f;
    p.perc3rd    = get ("percHarm") > 0.5f;
    p.percFast   = get ("percDecay") < 0.5f;
    p.click      = get ("click");
    p.drive      = get ("drive");
    p.leslieFast = get ("leslieFast") > 0.5f;
    p.leslieMix  = get ("leslieMix");
    p.organLevel = get ("organLevel");
    p.reverb     = get ("reverb");
    p.master     = get ("master");
    p.echoOn        = get ("echoOn") > 0.5f;
    p.gtrOn         = get ("gtrOn") > 0.5f;
    p.echoMode      = (int) get ("echoMode") + 1;
    p.echoRate      = get ("echoRate");
    p.echoIntensity = get ("echoIntensity");
    p.echoVol       = get ("echoVol");
    p.echoSpring    = get ("echoSpring");
    p.echoWow       = get ("echoWow");
    p.echoAge       = get ("echoAge");
    p.flOn          = get ("flOn") > 0.5f;
    p.flRate        = get ("flRate");
    p.flDepth       = get ("flDepth");
    p.flManual      = get ("flManual");
    p.flFeedback    = get ("flFeedback");
    p.flMix         = get ("flMix");
    p.maxOn         = get ("maxOn") > 0.5f;
    p.maxGainDb     = get ("maxGain");
    p.maxCeilDb     = get ("maxCeiling");

    // aba CHORD MACHINE: o MRCM-1240 gera o skank/bubble sincronizado com a DAW
    runMachine (midi, n);

    // junta as notas do teclado da tela e atualiza as teclas pintadas
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    int pos = 0;

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int t = juce::jlimit (0, n, meta.samplePosition);
        if (t > pos) { engine.render (L + pos, R + pos, t - pos); pos = t; }

        if (m.isNoteOn())                       engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())                 engine.noteOff (m.getNoteNumber());
        else if (m.isSustainPedalOn())          engine.setSustain (true);
        else if (m.isSustainPedalOff())         engine.setSustain (false);
        else if (m.isController() && m.getControllerNumber() == 1)
        {
            // mod wheel: vibrato no piano, Leslie rápido no órgão (acima da metade)
            const float v = (float) m.getControllerValue() / 127.0f;
            engine.setModWheel (v);
            modWheel.store (v);
        }
        else if (m.isPitchWheel())
        {
            const float v = juce::jlimit (-1.0f, 1.0f, (float) (m.getPitchWheelValue() - 8192) / 8192.0f);
            engine.setPitchBend (v);
            pitchWheel.store (v);
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            engine.allNotesOff();
    }
    if (pos < n) engine.render (L + pos, R + pos, n - pos);

    // SOM INTERNO da machine ligado: o piano M1 + Hammond dela somados à saída
    for (int ch = 0; ch < 2; ++ch)
        buffer.addFrom (ch, 0, machineBuf, ch, 0, n);

    // medidores (média quadrática do bloco) para o VU
    float sl = 0, sr = 0;
    for (int i = 0; i < n; ++i) { sl += L[i] * L[i]; sr += R[i] * R[i]; }
    if (n > 0) { meterL.store (sl / (float) n); meterR.store (sr / (float) n); }
    gainReduction.store (engine.gainReductionDb());
}

// =====================================================================  CHORD MACHINE (MRCM-1240)
void RootsKeysProcessor::runMachine (juce::MidiBuffer& midi, int numSamples)
{
    machine->setPlayHead (getPlayHead());
    machine->setNonRealtime (isNonRealtime());
    machineBuf.setSize (2, numSamples, false, false, true);
    machineBuf.clear();
    machineMidi.clear();
    machine->processBlock (machineBuf, machineMidi);   // limpa o buffer e escreve o som interno (se ligado)

    const bool heard = get ("machineOn") > 0.5f || machine->apvts.getRawParameterValue ("preview")->load() > 0.5f;
    const bool ownSound = machine->apvts.getRawParameterValue ("sound")->load() > 0.5f;
    if (! heard) { machineBuf.clear(); return; }
    if (ownSound) return;   // as notas já tocaram no som da machine (machineBuf vai para a saída)

    // SOM INTERNO desligado: as notas (skank no canal 1, bubble no 3) tocam no piano/órgão deste instrumento
    for (const auto meta : machineMidi)
    {
        auto m = meta.getMessage();
        if (m.isNoteOnOrOff() || m.isAllNotesOff() || m.isAllSoundOff())
        {
            m.setChannel (1);
            midi.addEvent (m, meta.samplePosition);
        }
    }
}

// Projetos do Roots Keys 2.x: a aba DUB CHORDS antiga vira a CHORD MACHINE com os mesmos acordes
// (os tipos com tétrade ou suspensão viram o maior ou o menor mais perto, porque a machine só tem os dois).
void RootsKeysProcessor::migrateOldDubChords (const juce::ValueTree& old)
{
    auto value = [&old] (const juce::String& id, double def)
    {
        for (const auto& c : old)
            if (c.hasType ("PARAM") && c.getProperty ("id").toString() == id)
                return (double) c.getProperty ("value", def);
        return def;
    };
    bool any = false;
    for (const auto& c : old)
        if (c.hasType ("PARAM") && c.getProperty ("id").toString().startsWith ("dc")) { any = true; break; }
    if (! any) return;

    auto set = [this] (const juce::String& id, double v)
    {
        if (auto* p = machine->apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) v));
    };
    //                       Maior menor 7  m7 maj7 m6 sus2 sus4 dim m9
    static const int toMinor[10] = { 0, 1, 0, 1, 0, 1, 0, 0, 1, 1 };
    const int defRoot[4] = { 9, 2, 7, 0 }, defQual[4] = { 1, 1, 0, 0 };
    for (int i = 0; i < 4; ++i)
    {
        const auto n = juce::String (i);
        set ("root" + n, value ("dcRoot" + n, defRoot[i]));
        set ("qual" + n, toMinor[juce::jlimit (0, 9, (int) value ("dcQual" + n, defQual[i]))]);
        set ("dur" + n, value ("dcDur" + n, 2));
        set ("inv" + n, 0);
        if (i > 0) set ("on" + n, value ("dcAct" + n, i == 1 ? 1 : 0));
    }
    set ("center",   value ("dcRegister", 60));
    set ("velocity", juce::jlimit (0.1, 1.0, value ("dcVel", 0.8)));
    set ("noteSize", value ("dcNoteSize", 0));
    set ("swing",    value ("dcSwing", 0.0));
    set ("bpm",      value ("dcBpm", 75.0));
    set ("exportReps", value ("dcReps", 1));
    // padrão: 0 = Auto (segue o instrumento: órgão = bubble), 4 = Bubble (órgão)
    const int pattern = (int) value ("dcPattern", 0);
    set ("bubble", (pattern == 4 || (pattern == 0 && value ("mode", 0) > 0.5)) ? 1 : 0);
    set ("preview", 0);
    set ("sound", 0);
    apvts.getParameter ("machineOn")->setValueNotifyingHost (value ("dcOn", 0) > 0.5 ? 1.0f : 0.0f);
}

juce::AudioProcessorEditor* RootsKeysProcessor::createEditor()
{
    return new RootsKeysEditor (*this);
}

// ---------------------------------------------------------------- presets
int RootsKeysProcessor::getNumPrograms() { return kNumPresets; }

const juce::String RootsKeysProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, kNumPresets) ? juce::String (kPresets[index].name) : juce::String();
}

void RootsKeysProcessor::setParam (const juce::String& id, float value)
{
    if (auto* prm = apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
}

void RootsKeysProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPresets)) return;
    currentProgram = index;
    const Preset& pr = kPresets[index];
    setParam ("mode", (float) pr.mode);

    if (pr.mode == 0)   // preset de piano: não toca nos parâmetros do órgão
    {
        setParam ("pianoType", 0.0f);   // os presets de piano são do timbre M1
        setParam ("pianoBright", pr.pianoBright);
        setParam ("pianoDecay", pr.pianoDecay);
        setParam ("pianoChorus", pr.pianoChorus);
        setParam ("pianoLevel", pr.pianoLevel);
    }
    else                // preset de órgão: não toca nos parâmetros do piano
    {
        for (int i = 0; i < 9; ++i) setParam ("db" + juce::String (i + 1), (float) pr.drawbars[i]);
        setParam ("percOn", pr.percOn ? 1.0f : 0.0f);
        setParam ("percHarm", pr.perc3rd ? 1.0f : 0.0f);
        setParam ("percDecay", pr.percFast ? 0.0f : 1.0f);
        setParam ("click", pr.click);
        setParam ("drive", pr.drive);
        setParam ("leslieMix", pr.leslieMix);
        setParam ("organLevel", pr.organLevel);
        setParam ("leslieFast", pr.leslieFast ? 1.0f : 0.0f);
    }
    setParam ("reverb", pr.reverb);
}

int RootsKeysProcessor::getNumPianoPresets() { return kNumPianoPresets; }

// ---------------------------------------------------------------- estado
void RootsKeysProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
    {
        juce::MemoryBlock m;   // a CHORD MACHINE vai junto, num filho que o Roots Keys antigo ignora
        machine->getStateInformation (m);
        xml->createNewChildElement ("ChordMachine")->setAttribute ("data", m.toBase64Encoding());
        copyXmlToBinary (*xml, destData);
    }
}

void RootsKeysProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            bool hadMachine = false;
            if (auto* cm = xml->getChildByName ("ChordMachine"))
            {
                juce::MemoryBlock m;
                if (m.fromBase64Encoding (cm->getStringAttribute ("data")) && m.getSize() > 0)
                {
                    machine->setStateInformation (m.getData(), (int) m.getSize());
                    hadMachine = true;
                }
                xml->removeChildElement (cm, true);
            }
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
            if (! hadMachine) migrateOldDubChords (tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RootsKeysProcessor();
}
