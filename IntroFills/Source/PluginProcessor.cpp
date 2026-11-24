#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>

using namespace juce;

namespace
{
    struct ProgramRef { int drummer, fill; };
    const std::vector<ProgramRef>& programs()
    {
        static const std::vector<ProgramRef> p = [] {
            std::vector<ProgramRef> v;
            for (int d = 0; d < (int) mif::drummers().size(); ++d)
                for (int f = 0; f < (int) mif::drummers()[(size_t) d].fills.size(); ++f) v.push_back ({ d, f });
            return v;
        }();
        return p;
    }
}

IntroFillsProcessor::IntroFillsProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MIF", createLayout())
{
    // o kit: samples DM do Mastor Reggae Drums MRD-12 + tons, surdo e timbal (os mesmos da versão para celular)
    for (int p = 0; p < mif::NumPieces; ++p)
    {
        const String file = String (mif::piece (p).file) + ".wav";
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
            if (file == BinaryData::originalFilenames[i])
            {
                int size = 0;
                if (auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size))
                {
                    mif::Sample s;
                    if (mif::readWav (data, (size_t) size, s)) sampler.setSample (p, std::move (s));
                }
            }
    }
    rebuild();
    startTimerHz (15);
}

IntroFillsProcessor::~IntroFillsProcessor() { stopTimer(); }

AudioProcessorValueTreeState::ParameterLayout IntroFillsProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray names;
    for (auto& d : mif::drummers()) names.add (d.name);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "drummer", 1 }, "Baterista", names, 0));
    l.add (std::make_unique<AudioParameterInt>    (ParameterID { "fill", 1 }, "Variacao", 0, 7, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Groove", StringArray { "One drop", "Rockers", "Steppers", "Dub", "Rocksteady" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "size", 1 }, "Tamanho", StringArray { "Anacruse", "1/2 compasso", "1 compasso", "2 compassos" }, 2));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "land", 1 }, "Chegada", StringArray { "Com o estalo", "Sem o estalo" }, 0));
    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "intensity", 1 }, "Intensidade", 0.0f, 1.0f, 0.72f));
    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "human", 1 }, "Humanizar", 0.0f, 1.0f, 1.0f));
    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "swing", 1 }, "Swing", 0.5f, 0.75f, 0.64f));
    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "ghost", 1 }, "Ghost", 0.0f, 1.0f, 0.2f));
    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "loop", 1 }, "Loop", false));
    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "play", 1 }, "Preview", false));
    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "bpm", 1 }, "BPM do preview", NormalisableRange<float> (50.0f, 160.0f, 1.0f), 74.0f));
    // padrão de fábrica do kit: Tape Echo e saturador começam desligados
    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "echoOn", 1 }, "Tape Echo", false));
    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "satOn", 1 }, "Saturador", false));
    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "volume", 1 }, "Volume", 0.0f, 1.0f, 0.8f));
    return l;
}

bool IntroFillsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == AudioChannelSet::stereo() || out == AudioChannelSet::mono();
}

void IntroFillsProcessor::setParam (const String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
    if (MessageManager::getInstanceWithoutCreating() != nullptr && MessageManager::getInstance()->isThisTheMessageThread())
        rebuild();   // a interface muda: a sequência nova já vale no próximo bloco (o timer cobre a automação)
}

int IntroFillsProcessor::drummer() const { return jlimit (0, (int) mif::drummers().size() - 1, (int) get ("drummer")); }
int IntroFillsProcessor::fill() const
{
    return jlimit (0, (int) mif::drummers()[(size_t) drummer()].fills.size() - 1, (int) get ("fill"));
}

void IntroFillsProcessor::selectDrummer (int d) { setParam ("drummer", (float) d); setParam ("fill", 0.0f); }

void IntroFillsProcessor::stepFill (int delta)
{
    const int n = (int) mif::drummers()[(size_t) drummer()].fills.size();
    setParam ("fill", (float) ((fill() + delta + n) % n));
}

mif::Fill IntroFillsProcessor::currentFill() const
{
    const int key = drummer() * 16 + fill();
    {
        std::lock_guard<std::mutex> lk (editsLock);
        auto it = edits.find (key);
        if (it != edits.end()) return it->second;
    }
    return mif::drummers()[(size_t) drummer()].fills[(size_t) fill()].ev;
}

bool IntroFillsProcessor::isEdited() const
{
    std::lock_guard<std::mutex> lk (editsLock);
    return edits.count (drummer() * 16 + fill()) > 0;
}

void IntroFillsProcessor::toggleCell (int p, int step)
{
    auto f = currentFill();
    auto it = std::find_if (f.begin(), f.end(), [&] (const mif::Ev& x) { return x.p == p && (int) std::lround (x.s) == step; });
    if (it != f.end()) f.erase (it);
    else { f.push_back (mif::e (step, p, 2)); audition (p); }
    {
        std::lock_guard<std::mutex> lk (editsLock);
        edits[drummer() * 16 + fill()] = f;
        ++editsVersion;
    }
    rebuild();
}

void IntroFillsProcessor::resetFill()
{
    {
        std::lock_guard<std::mutex> lk (editsLock);
        edits.erase (drummer() * 16 + fill());
        ++editsVersion;
    }
    rebuild();
}

void IntroFillsProcessor::audition (int p, int vel) { pendingAuditionVel = vel; pendingAudition = p; }

double IntroFillsProcessor::previewBpm() const
{
    const double h = hostBpm.load();
    return h > 0 ? h : (double) get ("bpm");
}

mif::Settings IntroFillsProcessor::settings() const
{
    mif::Settings st;
    st.drummer = drummer(); st.fill = fill();
    st.style = (int) get ("style"); st.size = (int) get ("size"); st.land = (int) get ("land");
    st.intensity = get ("intensity"); st.human = get ("human"); st.swing = get ("swing"); st.ghost = get ("ghost");
    st.bpm = previewBpm();
    return st;
}

String IntroFillsProcessor::settingsKey() const
{
    const auto st = settings();
    int ev;
    { std::lock_guard<std::mutex> lk (editsLock); ev = editsVersion; }
    return String (st.drummer) + ":" + String (st.fill) + ":" + String (st.style) + ":" + String (st.size) + ":" + String (st.land) + ":"
         + String (st.intensity, 3) + ":" + String (st.human, 3) + ":" + String (st.swing, 3) + ":" + String (st.ghost, 3) + ":"
         + String (st.bpm, 1) + ":" + String (ev);
}

void IntroFillsProcessor::rebuild()
{
    const auto st = settings();
    mif::Rng rng;
    auto q = mif::buildSequence (st, currentFill(), rng);
    auto player = std::make_shared<mif::SeqPlayer>();
    player->notes = mif::perform (st, q, rng);
    player->lengthBeats = q.total / 4.0;
    player->firstGrooveBeat = q.g0 / 4.0;
    std::atomic_store (&seq, std::shared_ptr<const mif::SeqPlayer> (player));
    lastKey = settingsKey();
}

void IntroFillsProcessor::timerCallback()
{
    if (settingsKey() != lastKey) rebuild();
}

void IntroFillsProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateHz = sampleRate;
    sampler.prepare (sampleRate);
    hits.reserve (4096);
    previewBeat = 0; lastHostBeat = -1; wasRunning = false;
}

void IntroFillsProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    // notas que chegam: tocam as peças do kit (notas do MRD-12)
    MidiBuffer in; in.swapWith (midi);
    hits.clear();
    for (const auto m : in)
    {
        const auto msg = m.getMessage();
        if (msg.isNoteOn())
            if (const int p = mif::pieceForNote (msg.getNoteNumber()); p >= 0) hits.push_back ({ m.samplePosition, p, msg.getVelocity() });
    }
    if (const int a = pendingAudition.exchange (-1); a >= 0) hits.push_back ({ 0, a, pendingAuditionVel.load() });

    // tempo: o da DAW quando ela toca; senão o PREVIEW com o BPM do plugin
    bool hostPlaying = false; double bpm = 0, ppq = 0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            hostPlaying = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    hostBpm = bpm;
    const bool preview = get ("play") > 0.5f && ! hostPlaying;
    if (preview && ! wasRunning) previewBeat = 0;
    if (! preview && ! hostPlaying) wasRunning = false;
    if (preview) { bpm = (double) get ("bpm"); ppq = previewBeat; }
    const bool running = (hostPlaying || preview) && bpm > 0;
    const bool loop = get ("loop") > 0.5f;
    const double human = get ("human");

    auto player = std::atomic_load (&seq);
    if (running && player && player->lengthBeats > 0)
    {
        const double beatsPerSample = bpm / 60.0 / sampleRateHz;
        const double len = player->lengthBeats;
        double b0 = ppq;
        if (loop) b0 = std::fmod (std::max (0.0, b0), len);
        const double b1 = b0 + n * beatsPerSample;
        auto addWindow = [&] (double from, double to, double shiftBeats)
        {
            // a primeira janela de uma passada também toca os enfeites de antes do 0 (drag/flam)
            const bool startOfPass = from <= 1e-9;
            for (auto& note : player->notes)
            {
                const double nb = note.beat;
                if ((nb >= from && nb < to) || (startOfPass && nb < 0 && from <= 0))
                {
                    const int off = jlimit (0, n - 1, (int) ((std::max (nb, from) - from + shiftBeats) / beatsPerSample));
                    hits.push_back ({ off, note.p, note.vel });
                }
            }
        };
        if (b0 < len)
        {
            if (b1 <= len) addWindow (b0, b1, 0);
            else
            {
                addWindow (b0, len, 0);
                if (loop) addWindow (0, b1 - len, len - b0);
            }
            playStep.store (std::fmod (b0, len) * 4.0);
        }
        else playStep.store (-1.0);
        if (preview)
        {
            previewBeat += n * beatsPerSample;
            if (loop && previewBeat >= len) previewBeat -= len;
        }
        wasRunning = true;
    }
    else playStep.store (-1.0);

    // toca em ordem, com precisão de amostra, e manda as mesmas notas para a saída MIDI (canal 10)
    std::stable_sort (hits.begin(), hits.end(), [] (const Hit& a, const Hit& b) { return a.offset < b.offset; });
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    float tmpR[2048];
    const bool echoOn = get ("echoOn") > 0.5f, satOn = get ("satOn") > 0.5f;
    const float vol = get ("volume");
    const double fxBpm = bpm > 0 ? bpm : (double) get ("bpm");
    int pos = 0; size_t k = 0;
    while (pos < n)
    {
        while (k < hits.size() && hits[k].offset <= pos)
        {
            sampler.hit (hits[k].piece, hits[k].vel, human);
            const int note = mif::piece (hits[k].piece).note;
            midi.addEvent (MidiMessage::noteOn (10, note, (uint8) jlimit (1, 127, hits[k].vel)), hits[k].offset);
            midi.addEvent (MidiMessage::noteOff (10, note), jmin (n - 1, hits[k].offset + 1));
            ++k;
        }
        const int next = k < hits.size() ? jmin (n, hits[k].offset) : n;
        const int len = jmin (next - pos, 2048);
        if (len <= 0) { ++pos; continue; }
        float* rp = R ? R + pos : tmpR;
        sampler.render (L + pos, rp, len, echoOn, satOn, fxBpm, vol);
        if (! R) for (int i = 0; i < len; ++i) L[pos + i] = 0.5f * (L[pos + i] + tmpR[i]);
        pos += len;
    }
    meter.store (buffer.getMagnitude (0, n));
}

// ---------------------------------------------------------------- MIDI
String IntroFillsProcessor::midiFileName() const
{
    const auto& d = mif::drummers()[(size_t) drummer()];
    auto s = String ("MIF_") + d.name + "_" + d.fills[(size_t) fill()].name + "_" + String (roundToInt (previewBpm())) + "bpm.mid";
    return File::createLegalFileName (s.replaceCharacter (' ', '_'));
}

bool IntroFillsProcessor::writeMidiFile (const File& f) const
{
    auto st = settings();
    mif::Rng rng;
    auto q = mif::buildSequence (st, currentFill(), rng);
    rng.seed = 4242;
    const auto& d = mif::drummers()[(size_t) st.drummer];
    auto bytes = mif::makeMidiFile (mif::perform (st, q, rng), st.bpm,
                                    std::string ("MIF ") + d.name + " - " + d.fills[(size_t) st.fill].name);
    f.getParentDirectory().createDirectory();
    return f.replaceWithData (bytes.data(), bytes.size());
}

// ---------------------------------------------------------------- presets e estado
int IntroFillsProcessor::getNumPrograms() { return (int) programs().size(); }
int IntroFillsProcessor::getCurrentProgram()
{
    for (int i = 0; i < (int) programs().size(); ++i)
        if (programs()[(size_t) i].drummer == drummer() && programs()[(size_t) i].fill == fill()) return i;
    return 0;
}
void IntroFillsProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= (int) programs().size()) return;
    setParam ("drummer", (float) programs()[(size_t) index].drummer);
    setParam ("fill", (float) programs()[(size_t) index].fill);
}
const String IntroFillsProcessor::getProgramName (int index)
{
    if (index < 0 || index >= (int) programs().size()) return {};
    const auto& d = mif::drummers()[(size_t) programs()[(size_t) index].drummer];
    return String (d.name) + " - " + d.fills[(size_t) programs()[(size_t) index].fill].name;
}

void IntroFillsProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("play", false, nullptr);
    ValueTree ed ("EDITS");
    {
        std::lock_guard<std::mutex> lk (editsLock);
        for (auto& [key, f] : edits)
        {
            ValueTree fv ("FILL"); fv.setProperty ("key", key, nullptr);
            for (auto& x : f)
            {
                ValueTree ev ("EV");
                ev.setProperty ("s", x.s, nullptr); ev.setProperty ("p", x.p, nullptr); ev.setProperty ("v", x.v, nullptr);
                if (x.drag) ev.setProperty ("drag", true, nullptr);
                if (x.flam) ev.setProperty ("flam", true, nullptr);
                fv.appendChild (ev, nullptr);
            }
            ed.appendChild (fv, nullptr);
        }
    }
    state.removeChild (state.getChildWithName ("EDITS"), nullptr);
    state.appendChild (ed, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void IntroFillsProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto state = ValueTree::fromXml (*xml);
    std::map<int, mif::Fill> loaded;
    for (auto fv : state.getChildWithName ("EDITS"))
    {
        mif::Fill f;
        for (auto ev : fv)
        {
            mif::Ev x { (double) ev.getProperty ("s"), jlimit (0, mif::NumPieces - 1, (int) ev.getProperty ("p")), jlimit (0, 3, (int) ev.getProperty ("v")) };
            x.drag = ev.getProperty ("drag", false); x.flam = ev.getProperty ("flam", false);
            f.push_back (x);
        }
        loaded[(int) fv.getProperty ("key")] = f;
    }
    state.removeChild (state.getChildWithName ("EDITS"), nullptr);
    apvts.replaceState (state);
    setParam ("play", 0.0f);
    {
        std::lock_guard<std::mutex> lk (editsLock);
        edits = std::move (loaded);
        ++editsVersion;
    }
    rebuild();
}

AudioProcessorEditor* IntroFillsProcessor::createEditor() { return new IntroFillsEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new IntroFillsProcessor(); }
