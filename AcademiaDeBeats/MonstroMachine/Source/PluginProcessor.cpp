#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

juce::AudioProcessorValueTreeState::ParameterLayout MonstroProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "kit", 1 }, "Kit", 0, (int) mm::kits().size() - 1, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "seq", 1 }, "Sequencia", juce::StringArray { "A", "B", "C", "D" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "bpm", 1 }, "BPM (sem DAW)", juce::NormalisableRange<float> (60.f, 180.f, 1.f), 80.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "swing", 1 }, "Swing", juce::NormalisableRange<float> (50.f, 75.f, 1.f), 54.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "vol", 1 }, "Volume", juce::NormalisableRange<float> (0.f, 1.f), .8f));
    // padrão de fábrica do kit: saturador e Tape Echo começam desligados
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "satOn", 1 }, "Saturador", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "satDrive", 1 }, "Saturador Drive", juce::NormalisableRange<float> (0.f, 1.f), .35f));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "echoOn", 1 }, "Tape Echo", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "echoTime", 1 }, "Echo Tempo (passos)", juce::NormalisableRange<float> (1.f, 8.f, 1.f), 3.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "echoFb", 1 }, "Echo Feedback", juce::NormalisableRange<float> (0.f, .85f), .45f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "echoMix", 1 }, "Echo Mix", juce::NormalisableRange<float> (0.f, 1.f), .3f));
    for (int i = 0; i < mm::kPads; ++i)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "tune" + juce::String (i), 1 }, "Pad " + juce::String (i + 1) + " Tune",
                                                                  juce::NormalisableRange<float> (-12.f, 12.f, 1.f), 0.f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dec" + juce::String (i), 1 }, "Pad " + juce::String (i + 1) + " Decay",
                                                                  juce::NormalisableRange<float> (25.f, 200.f, 1.f), 100.f));
    }
    return { p.begin(), p.end() };
}

MonstroProcessor::MonstroProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MonstroMachineState", createLayout())
{
    pKit = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter ("kit"));
    pSeq = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("seq"));
    bpmP = apvts.getRawParameterValue ("bpm");       swingP = apvts.getRawParameterValue ("swing");
    volP = apvts.getRawParameterValue ("vol");       satOnP = apvts.getRawParameterValue ("satOn");
    satDrvP = apvts.getRawParameterValue ("satDrive"); echoOnP = apvts.getRawParameterValue ("echoOn");
    echoTP = apvts.getRawParameterValue ("echoTime"); echoFbP = apvts.getRawParameterValue ("echoFb");
    echoMixP = apvts.getRawParameterValue ("echoMix");
    for (int i = 0; i < mm::kPads; ++i)
    {
        tuneP[i] = apvts.getRawParameterValue ("tune" + juce::String (i));
        decP[i]  = apvts.getRawParameterValue ("dec" + juce::String (i));
    }
    formats.registerBasicFormats();
    // abre no kit do DubMastor com o groove de reggae (caixa no 2 e no 4)
    auto pats = mm::stylePatterns (mm::kits()[0].style);
    for (int s = 0; s < mm::kSeqs; ++s) writeGrid (s, pats[(size_t) s]);
}

bool MonstroProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void MonstroProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    engine.prepare (sampleRate);
    lastPos = -1;
}

void MonstroProcessor::writeGrid (int seq, const mm::Grid& g)
{
    for (int p = 0; p < mm::kPads; ++p)
        for (int s = 0; s < mm::kSteps; ++s) grid[seq][p][s].store (g[(size_t) p][(size_t) s]);
}

mm::Grid MonstroProcessor::seqGrid (int seq) const
{
    mm::Grid g {};
    for (int p = 0; p < mm::kPads; ++p)
        for (int s = 0; s < mm::kSteps; ++s) g[(size_t) p][(size_t) s] = grid[seq][p][s].load();
    return g;
}

void MonstroProcessor::pushUndo()
{
    for (int s = 0; s < mm::kSeqs; ++s) history.push_back (seqGrid (s));
    while (history.size() > (size_t) (50 * mm::kSeqs)) history.erase (history.begin(), history.begin() + mm::kSeqs);
}

bool MonstroProcessor::undo()
{
    if (history.size() < (size_t) mm::kSeqs) return false;
    for (int s = mm::kSeqs - 1; s >= 0; --s) { writeGrid (s, history.back()); history.pop_back(); }
    return true;
}

void MonstroProcessor::clearSeq (int seq) { pushUndo(); writeGrid (seq, mm::Grid {}); }

void MonstroProcessor::loadStyle (int style)
{
    pushUndo();
    auto pats = mm::stylePatterns (style);
    for (int s = 0; s < mm::kSeqs; ++s) writeGrid (s, pats[(size_t) s]);
}

void MonstroProcessor::selectKit (int index)
{
    const int n = (int) mm::kits().size();
    index = ((index % n) + n) % n;
    const auto& k = mm::kits()[(size_t) index];
    pKit->setValueNotifyingHost (pKit->convertTo0to1 ((float) index));
    if (auto* b = apvts.getParameter ("bpm")) b->setValueNotifyingHost (b->convertTo0to1 ((float) k.bpm));
    forcedStyle = -1;
    clearSamples();
    loadStyle (k.style);
    pSeq->setValueNotifyingHost (0.f);
}

void MonstroProcessor::hitPad (int pad, float vel)
{
    selectedPad = pad;
    pendingHit[pad].store (juce::jlimit (1, 127, (int) (vel * 127)));
}

void MonstroProcessor::recordHit (int pad, double pos16)
{
    const int s = ((int) std::lround (pos16) % mm::kSteps + mm::kSteps) % mm::kSteps;
    grid[pSeq->getIndex()][pad][s].store (1);
}

void MonstroProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    buffer.clear();

    engine.setKit (pKit->get());
    engine.master = volP->load();
    engine.satOn = satOnP->load() > .5f;  engine.satDrive = satDrvP->load();
    engine.echoOn = echoOnP->load() > .5f; engine.echoSteps = echoTP->load();
    engine.echoFb = echoFbP->load();      engine.echoMix = echoMixP->load();
    for (int i = 0; i < mm::kPads; ++i) { engine.tune[i] = tuneP[i]->load(); engine.decay[i] = decP[i]->load(); }

    // transporte: segue a DAW quando ela toca; senão o PLAY interno no BPM do plugin
    double bpm = bpmP->load();
    bool hostPlaying = false; double hostPpq = 0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) if (*b > 0 && pos->getIsPlaying()) bpm = *b;
            hostPlaying = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) hostPpq = *q; else hostPlaying = false;
        }
    hostBpm = bpm;
    engine.bpm = bpm;
    const double per16 = bpm / 60.0 * 4.0 / sr;   // semicolcheias por sample
    const bool run = hostPlaying || internalPlay.load();
    double pos0 = 0;
    if (hostPlaying) pos0 = hostPpq * 4.0;
    else if (run) pos0 = internalPos;
    if (run && ! wasRunning && ! hostPlaying) { internalPos = 0; pos0 = 0; }
    if (! run) internalPos = 0;
    const double pos1 = pos0 + per16 * n;

    if (run)
    {
        const int seq = pSeq->getIndex();
        const double swingOff = (swingP->load() - 50.0) / 50.0;  // atraso das semicolcheias pares (em passos)
        const bool fresh = ! wasRunning || std::abs (pos0 - lastPos) > 1.0;
        for (long k = (long) std::floor (pos0) - 1; k <= (long) std::ceil (pos1); ++k)
        {
            const double t = (double) k + ((k & 1) ? swingOff : 0.0);
            if (t < pos0 - (fresh ? 1e-6 : 0.0) || t >= pos1) continue;
            const int step = (int) (((k % mm::kSteps) + mm::kSteps) % mm::kSteps);
            const int off = juce::jlimit (0, n - 1, (int) ((t - pos0) / per16));
            for (int p = 0; p < mm::kPads; ++p)
                if (const int v = grid[seq][p][step].load()) engine.trigger (p, v == 2 ? 1.0f : .75f, off);
        }
        playStep = ((int) std::floor (pos0) % mm::kSteps + mm::kSteps) % mm::kSteps;
        lastPos = pos1;
        if (! hostPlaying) internalPos = pos1;
    }
    else playStep = -1;
    running = run; wasRunning = run;

    // MIDI da DAW: notas 36–51 tocam os pads 1–16
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (! msg.isNoteOn()) continue;
        const int pad = msg.getNoteNumber() - mm::kFirstNote;
        if (pad < 0 || pad >= mm::kPads) continue;
        engine.trigger (pad, msg.getFloatVelocity(), m.samplePosition);
        if (recording && run) recordHit (pad, pos0 + per16 * m.samplePosition);
    }
    // toques na tela
    for (int p = 0; p < mm::kPads; ++p)
        if (const int v = pendingHit[p].exchange (0))
        {
            engine.trigger (p, v / 127.f, 0);
            if (recording && run) recordHit (p, pos0);
        }

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    if (R != nullptr) engine.render (L, R, n);
    else
    {
        juce::HeapBlock<float> tmp ((size_t) n, true);
        engine.render (L, tmp.get(), n);
    }
}

// ---------- samples do usuário ----------
bool MonstroProcessor::loadSampleFile (int pad, const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> r (formats.createReaderFor (f));
    if (r == nullptr || r->lengthInSamples < 2) return false;
    const int len = (int) juce::jmin<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 20));
    juce::AudioBuffer<float> b ((int) juce::jmax (1u, juce::jmin (2u, r->numChannels)), len);
    r->read (&b, 0, len, 0, true, true);
    auto s = std::make_shared<mm::Sample>();
    s->rate = r->sampleRate;
    s->name = f.getFileNameWithoutExtension().toStdString();
    s->l.assign (b.getReadPointer (0), b.getReadPointer (0) + len);
    s->l.push_back (0);
    if (b.getNumChannels() > 1) { s->r.assign (b.getReadPointer (1), b.getReadPointer (1) + len); s->r.push_back (0); }
    std::atomic_store (&engine.samples[pad], std::shared_ptr<const mm::Sample> (s));
    samplePaths[pad] = f.getFullPathName();
    return true;
}

int MonstroProcessor::loadFolder (const juce::File& dir)
{
    auto files = dir.findChildFiles (juce::File::findFiles, true, "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
    files.sort();
    struct Rule { std::vector<int> pads; const char* words; };
    const Rule rules[] = { { { 0, 1 }, "kick|bumbo|bd" }, { { 2 }, "snare|caixa|sd" }, { { 3 }, "clap|palma|snap" }, { { 4 }, "rim" },
                           { { 6 }, "open|oh" }, { { 5 }, "hat|hh|hihat" }, { { 7 }, "crash|cym|ride" }, { { 12, 13 }, "808|bass|baixo" },
                           { { 11 }, "shak" }, { { 8, 9, 10 }, "perc|conga|bongo|tom" }, { { 14 }, "fx|efeito|riser|impact" }, { { 15 }, "vox|vocal|chant|voz" } };
    juce::Array<juce::File> used;
    int count = 0;
    for (auto& rule : rules)
    {
        auto words = juce::StringArray::fromTokens (rule.words, "|", "");
        size_t next = 0;
        for (auto& f : files)
        {
            if (next >= rule.pads.size()) break;
            if (used.contains (f)) continue;
            const auto path = f.getRelativePathFrom (dir).toLowerCase();
            bool match = false;
            for (auto& w : words) if (path.contains (w)) { match = true; break; }
            if (! match) continue;
            used.add (f);
            if (loadSampleFile (rule.pads[next], f)) { ++next; ++count; }
        }
    }
    return count;
}

void MonstroProcessor::clearSamples()
{
    for (int i = 0; i < mm::kPads; ++i) { std::atomic_store (&engine.samples[i], std::shared_ptr<const mm::Sample>()); samplePaths[i] = {}; }
}

juce::String MonstroProcessor::sampleName (int pad) const
{
    if (auto s = std::atomic_load (&engine.samples[pad])) return juce::String::fromUTF8 (s->name.c_str());
    return mm::padName (pad);
}

// ---------- estado ----------
void MonstroProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    juce::String g;
    for (int s = 0; s < mm::kSeqs; ++s)
        for (int p = 0; p < mm::kPads; ++p)
            for (int k = 0; k < mm::kSteps; ++k) g << (int) grid[s][p][k].load();
    state.setProperty ("grid", g, nullptr);
    for (int i = 0; i < mm::kPads; ++i) state.setProperty ("sample" + juce::String (i), samplePaths[i], nullptr);
    state.setProperty ("logoA", logoA, nullptr);
    state.setProperty ("logoM", logoM, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void MonstroProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto state = juce::ValueTree::fromXml (*xml);
    apvts.replaceState (state);
    const auto g = state.getProperty ("grid").toString();
    if (g.length() == mm::kSeqs * mm::kPads * mm::kSteps)
    {
        int c = 0;
        for (int s = 0; s < mm::kSeqs; ++s)
            for (int p = 0; p < mm::kPads; ++p)
                for (int k = 0; k < mm::kSteps; ++k) grid[s][p][k].store ((uint8_t) juce::jlimit (0, 2, g[c++] - '0'));
    }
    for (int i = 0; i < mm::kPads; ++i)
    {
        const juce::File f (state.getProperty ("sample" + juce::String (i)).toString());
        if (f.existsAsFile()) loadSampleFile (i, f);
    }
    logoA = state.getProperty ("logoA").toString();
    logoM = state.getProperty ("logoM").toString();
}

juce::AudioProcessorEditor* MonstroProcessor::createEditor() { return new MonstroEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MonstroProcessor(); }
