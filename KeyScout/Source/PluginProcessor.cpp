#include "PluginProcessor.h"
#include "PluginEditor.h"

KeyScoutProcessor::KeyScoutProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MastorKeyScout", createLayout())
{
    for (auto& c : chroma) c.store (0.0f);
    startTimerHz (20);
}

KeyScoutProcessor::~KeyScoutProcessor() { stopTimer(); }

juce::AudioProcessorValueTreeState::ParameterLayout KeyScoutProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "hold", 1 }, "Travar tom", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "listen", 1 }, "Escutar", true));
    return { p.begin(), p.end() };
}

bool KeyScoutProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

void KeyScoutProcessor::prepareToPlay (double sampleRate, int)
{
    ear.prepare (sampleRate > 0 ? sampleRate : 44100.0);
    setLatencySamples (0);
}

void KeyScoutProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int n = buffer.getNumSamples(), nc = juce::jmin (2, getTotalNumInputChannels());
    if (n == 0 || nc == 0) return;
    if (relearnFlag.exchange (false)) ear.relearn();
    float pk = 0.0f;
    const bool listen = apvts.getRawParameterValue ("listen")->load() > 0.5f;
    for (int i = 0; i < n; ++i)
    {
        float x = buffer.getSample (0, i);
        if (nc > 1) x = 0.5f * (x + buffer.getSample (1, i));
        pk = juce::jmax (pk, std::abs (x));
        if (listen) ear.push (x);
    }
    // o áudio passa sem mudança
    level.store (juce::Decibels::gainToDecibels (pk, -100.0f));
    sig.store (listen && ear.hasSignal());
    learned.store (ear.hasKey());
    keyRoot.store (ear.getKeyRoot()); keyMin.store (ear.getKeyMinor());
    chordRoot.store (ear.getChordRoot()); chordMin.store (ear.getChordMinor());
    const auto& c = ear.getShortChroma();
    for (int i = 0; i < 12; ++i) chroma[(size_t) i].store (c[(size_t) i]);
}

KeyScoutProcessor::Status KeyScoutProcessor::getStatus() const
{
    Status s;
    s.signal = sig.load(); s.learned = learned.load();
    s.hold = apvts.getRawParameterValue ("hold")->load() > 0.5f;
    s.keyRoot = keyRoot.load(); s.keyMinor = keyMin.load();
    if (s.hold && heldRoot >= 0) { s.keyRoot = heldRoot; s.keyMinor = heldMinor; s.learned = true; }
    s.chordRoot = chordRoot.load(); s.chordMinor = chordMin.load();
    for (int i = 0; i < 12; ++i) s.chroma[(size_t) i] = chroma[(size_t) i].load();
    s.levelDb = level.load();
    return s;
}

void KeyScoutProcessor::timerCallback()
{
    const bool hold = apvts.getRawParameterValue ("hold")->load() > 0.5f;
    if (hold && ! wasHold) { heldRoot = learned.load() ? keyRoot.load() : -1; heldMinor = keyMin.load(); }
    wasHold = hold;
    const auto st = getStatus();
    scoutlink::Data d {};
    d.signal = st.signal ? 1 : 0;
    d.keyRoot = st.learned ? st.keyRoot : -1;
    d.keyMinor = st.keyMinor ? 1 : 0;
    d.chordRoot = st.signal ? st.chordRoot : -1;
    d.chordMinor = st.chordMinor ? 1 : 0;
    d.hold = hold ? 1 : 0;
    for (int i = 0; i < 12; ++i) d.chroma[i] = st.chroma[(size_t) i];
    writer.write (d);
}

void KeyScoutProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.setProperty ("heldRoot", heldRoot, nullptr);
    st.setProperty ("heldMinor", heldMinor, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void KeyScoutProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto t = juce::ValueTree::fromXml (*xml);
            heldRoot = t.getProperty ("heldRoot", -1);
            heldMinor = t.getProperty ("heldMinor", false);
            wasHold = (float) t.getChildWithProperty ("id", "hold").getProperty ("value", 0.0f) > 0.5f;
            apvts.replaceState (t);
        }
}

juce::AudioProcessorEditor* KeyScoutProcessor::createEditor() { return new KeyScoutEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KeyScoutProcessor(); }
