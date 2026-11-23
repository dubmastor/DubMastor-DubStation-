#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::String fmtDb  (float v, int) { return juce::String (v, 1) + " dB"; }
    juce::String fmtSt  (float v, int) { return juce::String (v, 1) + " st"; }
    juce::String fmtQ   (float v, int) { return juce::String (v, 2); }
    juce::String fmtPct (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
    juce::String fmtMs  (float v, int)
    {
        return v < 1000.0f ? juce::String (v, v < 10.0f ? 1 : 0) + " ms"
                           : juce::String (v / 1000.0f, 2) + " s";
    }
    juce::String fmtHz (float v, int)
    {
        return v < 1000.0f ? juce::String (juce::roundToInt (v)) + " Hz"
                           : juce::String (v / 1000.0f, 1) + " kHz";
    }

    // Curvas de saturação (aplicadas em 2x oversampling)
    inline float shape (float x, int type) noexcept
    {
        switch (type)
        {
            case 0: // TUBE — assimétrica, harmônicos pares
            {
                constexpr float bias = 0.22f;
                static const float tb = std::tanh (bias);
                return std::tanh (x + bias) - tb;
            }
            case 3: // FITA — joelho macio de fita magnética (comprime antes de distorcer)
            {
                constexpr float p = 2.4f;
                const float ax = std::abs (x * 1.15f);
                const float y = (x * 1.15f) / std::pow (1.0f + std::pow (ax, p), 1.0f / p);
                return y + 0.04f * y * y;                         // um pouco de 2º harmônico da cabeça
            }
            case 1: // SOFT — compressão suave, harmônicos ímpares
                return (2.0f / juce::MathConstants<float>::pi) * std::atan (x * 1.3f);

            default: // FUZZ — clip duro
                return juce::jlimit (-1.0f, 1.0f, x * 1.5f) * 0.9f;
        }
    }
}

//==============================================================================
DubBassAudioProcessor::DubBassAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DubBassState", createLayout())
{
    auto get = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };

    prm.mode = get (ParamID::mode);           prm.harm2 = get (ParamID::harm2);
    prm.harm3 = get (ParamID::harm3);         prm.glide = get (ParamID::glide);
    prm.pitchAmt = get (ParamID::pitchAmt);   prm.pitchTime = get (ParamID::pitchTime);
    prm.attack = get (ParamID::attack);       prm.decay = get (ParamID::decay);
    prm.sustain = get (ParamID::sustain);     prm.release = get (ParamID::release);
    prm.cutoff = get (ParamID::cutoff);       prm.reso = get (ParamID::reso);
    prm.drvOn = get (ParamID::drvOn);         prm.drvType = get (ParamID::drvType);
    prm.drive = get (ParamID::drive);         prm.drvTone = get (ParamID::drvTone);
    prm.drvMix = get (ParamID::drvMix);       prm.eqOn = get (ParamID::eqOn);
    prm.revOn = get (ParamID::revOn);         prm.revSize = get (ParamID::revSize);
    prm.revDamp = get (ParamID::revDamp);     prm.revWidth = get (ParamID::revWidth);
    prm.revLowCut = get (ParamID::revLowCut); prm.revMix = get (ParamID::revMix);
    prm.boost = get (ParamID::boost);         prm.maxBypass = get (ParamID::maxBypass);
    prm.maxGain = get (ParamID::maxGain);     prm.maxCeiling = get (ParamID::maxCeiling);
    prm.maxRelease = get (ParamID::maxRelease);
    prm.output = get (ParamID::output);

    for (int b = 0; b < numEqBands; ++b)
        prm.eq[(size_t) b] = get (eqParamID (b));
}

juce::AudioProcessorValueTreeState::ParameterLayout DubBassAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&layout] (const String& id, const String& name, NormalisableRange<float> range,
                               float def, String (*fmt) (float, int))
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def,
                        AudioParameterFloatAttributes().withStringFromValueFunction (fmt)));
    };
    auto addBool = [&layout] (const String& id, const String& name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };
    auto addChoice = [&layout] (const String& id, const String& name, const StringArray& items, int def)
    {
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };

    // OSC
    addChoice (ParamID::mode, "Mode", { "Bass", "808", "Super Tone" }, 0);   // ordem dos valores fixa (compatível); a tela mostra BASS / SUPER TONE / 808
    addFloat  (ParamID::harm2,     "Harmonic 2",  { 0.0f, 1.0f }, 0.15f, fmtPct);
    addFloat  (ParamID::harm3,     "Harmonic 3",  { 0.0f, 1.0f }, 0.08f, fmtPct);
    addFloat  (ParamID::glide,     "Glide",       skewed (0.0f, 500.0f, 80.0f), 20.0f, fmtMs);
    addFloat  (ParamID::pitchAmt,  "808 Pitch",   { 0.0f, 24.0f }, 12.0f, fmtSt);
    addFloat  (ParamID::pitchTime, "808 Time",    skewed (5.0f, 500.0f, 80.0f), 70.0f, fmtMs);

    // AMP / FILTER
    addFloat (ParamID::attack,  "Attack",    skewed (0.5f, 2000.0f, 50.0f), 3.0f, fmtMs);
    addFloat (ParamID::decay,   "Decay",     skewed (5.0f, 5000.0f, 500.0f), 600.0f, fmtMs);
    addFloat (ParamID::sustain, "Sustain",   { 0.0f, 1.0f }, 0.85f, fmtPct);
    addFloat (ParamID::release, "Release",   skewed (5.0f, 5000.0f, 300.0f), 180.0f, fmtMs);
    addFloat (ParamID::cutoff,  "Cutoff",    skewed (40.0f, 10000.0f, 800.0f), 900.0f, fmtHz);
    addFloat (ParamID::reso,    "Resonance", skewed (0.5f, 6.0f, 1.2f), 0.8f, fmtQ);

    // DRIVE
    addBool   (ParamID::drvOn,   "Drive On", false);   // válvula começa desligada
    addChoice (ParamID::drvType, "Drive Type", { "Tube", "Soft", "Fuzz", "Fita" }, 0);
    addFloat  (ParamID::drive,   "Drive",      { 0.0f, 36.0f }, 6.0f, fmtDb);
    addFloat  (ParamID::drvTone, "Drive Tone", skewed (400.0f, 16000.0f, 2500.0f), 5000.0f, fmtHz);
    addFloat  (ParamID::drvMix,  "Drive Mix",  { 0.0f, 1.0f }, 0.5f, fmtPct);

    // EQ gráfico
    addBool (ParamID::eqOn, "EQ On", true);
    for (int b = 0; b < numEqBands; ++b)
    {
        const auto f = eqFrequencies[(size_t) b];
        const auto name = "EQ " + (f < 1000.0f ? String (roundToInt (f)) + " Hz" : String (roundToInt (f / 1000.0f)) + " kHz");
        addFloat (eqParamID (b), name, { -12.0f, 12.0f, 0.1f }, 0.0f, fmtDb);
    }

    // REVERB
    addBool  (ParamID::revOn,     "Reverb On", false);  // reverb começa desligado
    addFloat (ParamID::revSize,   "Reverb Size",    { 0.0f, 1.0f }, 0.6f, fmtPct);
    addFloat (ParamID::revDamp,   "Reverb Damping", { 0.0f, 1.0f }, 0.5f, fmtPct);
    addFloat (ParamID::revWidth,  "Reverb Width",   { 0.0f, 1.0f }, 1.0f, fmtPct);
    addFloat (ParamID::revLowCut, "Reverb Low Cut", skewed (20.0f, 2000.0f, 300.0f), 250.0f, fmtHz);
    addFloat (ParamID::revMix,    "Reverb Mix",     { 0.0f, 1.0f }, 0.12f, fmtPct);

    // BASS BOOST (60 Hz +4 dB, pré-maximizer)
    addBool (ParamID::boost, "Bass Boost", false);

    // MAXIMIZER
    addBool  (ParamID::maxBypass,  "Maximizer Bypass", false);
    addFloat (ParamID::maxGain,    "Maximizer Gain",    { 0.0f, 24.0f, 0.1f }, 3.0f, fmtDb);
    addFloat (ParamID::maxCeiling, "Maximizer Ceiling", { -12.0f, 0.0f, 0.1f }, -0.3f, fmtDb);
    addFloat (ParamID::maxRelease, "Maximizer Release", skewed (10.0f, 1000.0f, 150.0f), 120.0f, fmtMs);

    // OUTPUT
    addFloat (ParamID::output, "Output", { -30.0f, 0.0f, 0.1f }, 0.0f, fmtDb);

    return layout;
}

bool DubBassAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

//==============================================================================
void DubBassAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    keyboardState.reset();
    const int  nCh   = juce::jlimit (1, 2, getTotalNumOutputChannels());
    const auto block = (juce::uint32) juce::jmax (1, samplesPerBlock);

    // Gerador
    voice.prepare (sampleRate);
    updateVoiceParams();
    voiceFilter.prepare ({ sampleRate, block, 1 });
    voiceFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    cutoffSm.reset (sampleRate, 0.02);
    cutoffSm.setCurrentAndTargetValue (prm.cutoff->load());

    const juce::dsp::ProcessSpec spec { sampleRate, block, (juce::uint32) nCh };

    // Drive
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) nCh, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampler->initProcessing ((size_t) block);
    osSampleRate = sampleRate * 2.0;
    driveGainSm.reset (osSampleRate, 0.03);
    driveMixSm.reset (osSampleRate, 0.03);
    driveGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (prm.drive->load()));
    driveMixSm.setCurrentAndTargetValue (prm.drvOn->load() > 0.5f ? prm.drvMix->load() : 0.0f);
    toneState.fill (0.0f); dcX.fill (0.0f); dcY.fill (0.0f);

    // EQ (coeficientes antes do prepare, para os filtros já nascerem com a ordem certa)
    lastEqGain.fill (1000.0f);
    updateEq();
    for (auto& band : eqBands) { band.prepare (spec); band.reset(); }

    // Bass boost
    boostDbSm.reset (sampleRate, 0.08);
    boostDbSm.setCurrentAndTargetValue (prm.boost->load() > 0.5f ? boostGainDb : 0.0f);
    boostNeedsUpdate = true;
    updateBoost (0);
    boostFilter.prepare (spec);
    boostFilter.reset();

    // Head bump da fita
    headBumpDbSm.reset (sampleRate, 0.08);
    headBumpDbSm.setCurrentAndTargetValue (0.0f);
    *headBump.state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (sampleRate, 90.0f, 0.8f, 1.0f);
    headBumpNeedsUpdate = true;
    headBump.prepare (spec);
    headBump.reset();

    // Reverb
    reverb.prepare (spec);
    reverb.reset();
    reverbHP.prepare (spec);
    reverbHP.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    reverbBuffer.setSize (nCh, (int) block, false, true, false);
    reverbWasOn = false;

    // Maximizer + saída
    maximizer.prepare (sampleRate, nCh);
    outGainSm.reset (sampleRate, 0.05);
    outGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (prm.output->load()));

    setLatencySamples (maximizer.getLatencySamples()
                       + (int) std::round (oversampler->getLatencyInSamples()));
}

//==============================================================================
void DubBassAudioProcessor::updateVoiceParams()
{
    BassVoice::Params vp;
    vp.mode        = (BassVoice::Mode) juce::jlimit (0, 2, juce::roundToInt (prm.mode->load()));
    vp.harm2       = prm.harm2->load();
    vp.harm3       = prm.harm3->load();
    vp.glideMs     = prm.glide->load();
    vp.pitchAmt    = prm.pitchAmt->load();
    vp.pitchTimeMs = prm.pitchTime->load();
    vp.attackMs    = prm.attack->load();
    vp.decayMs     = prm.decay->load();
    vp.sustain     = prm.sustain->load();
    vp.releaseMs   = prm.release->load();
    voice.setParams (vp);
}

void DubBassAudioProcessor::handleMidi (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
        voice.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
    else if (msg.isNoteOff())
        voice.noteOff (msg.getNoteNumber());
    else if (msg.isPitchWheel())
        voice.setPitchBend ((float) (msg.getPitchWheelValue() - 8192) / 8192.0f * 2.0f);
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        voice.allNotesOff();
}

void DubBassAudioProcessor::renderVoice (float* out, int start, int end)
{
    for (int i = start; i < end; ++i)
    {
        voiceFilter.setCutoffFrequency (cutoffSm.getNextValue());
        out[i] = voiceFilter.processSample (0, voice.process());
    }
}

void DubBassAudioProcessor::processDrive (juce::dsp::AudioBlock<float>& block)
{
    const bool on = prm.drvOn->load() > 0.5f;
    {
        float pk = 0.0f;
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            for (size_t i = 0; i < block.getNumSamples(); ++i)
                pk = std::max (pk, std::abs (block.getSample ((int) ch, (int) i)));
        driveLevel.store (on ? pk * juce::Decibels::decibelsToGain (prm.drive->load()) : 0.0f);
    }

    auto up = oversampler->processSamplesUp (block);
    driveGainSm.setTargetValue (juce::Decibels::decibelsToGain (prm.drive->load()));
    driveMixSm.setTargetValue (on ? prm.drvMix->load() : 0.0f);

    const auto n   = up.getNumSamples();
    const auto nCh = std::min<size_t> (up.getNumChannels(), 2);

    if (! on && ! driveMixSm.isSmoothing())
    {
        driveGainSm.skip ((int) n);
    }
    else
    {
        const int   type      = juce::roundToInt (prm.drvType->load());
        const auto  twoPi     = juce::MathConstants<double>::twoPi;
        const float toneCoeff = (float) (1.0 - std::exp (-twoPi * prm.drvTone->load() / osSampleRate));
        const float dcR       = (float) (1.0 - twoPi * 8.0 / osSampleRate);

        for (size_t i = 0; i < n; ++i)
        {
            const float g   = driveGainSm.getNextValue();
            const float mix = driveMixSm.getNextValue();

            for (size_t ch = 0; ch < nCh; ++ch)
            {
                float* d = up.getChannelPointer (ch);
                const float x = d[i];

                float y = shape (x * g, type);
                toneState[ch] += toneCoeff * (y - toneState[ch]);   // TONE (low-pass)
                y = toneState[ch];

                const float dc = y - dcX[ch] + dcR * dcY[ch];        // DC blocker
                dcX[ch] = y;
                dcY[ch] = dc;

                d[i] = x + mix * (dc - x);
            }
        }
    }

    oversampler->processSamplesDown (block);
}

void DubBassAudioProcessor::updateEq()
{
    const bool on = prm.eqOn->load() > 0.5f;
    const double sr = getSampleRateSafe();

    for (int b = 0; b < numEqBands; ++b)
    {
        const float gainDb = on ? prm.eq[(size_t) b]->load() : 0.0f;
        if (juce::exactlyEqual (gainDb, lastEqGain[(size_t) b]))
            continue;

        lastEqGain[(size_t) b] = gainDb;
        const float f = std::min (eqFrequencies[(size_t) b], (float) (sr * 0.45));
        *eqBands[(size_t) b].state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (
                                         sr, f, eqQ, juce::Decibels::decibelsToGain (gainDb));
    }
}

void DubBassAudioProcessor::updateBoost (int numSamples)
{
    boostDbSm.setTargetValue (prm.boost->load() > 0.5f ? boostGainDb : 0.0f);

    if (boostNeedsUpdate || boostDbSm.isSmoothing())
    {
        const float db = boostDbSm.skip (numSamples);
        *boostFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (
                                 getSampleRateSafe(), boostFreq, boostQ, juce::Decibels::decibelsToGain (db));
        boostNeedsUpdate = boostDbSm.isSmoothing();
    }
}

void DubBassAudioProcessor::processReverb (juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (prm.revOn->load() < 0.5f)
    {
        reverbWasOn = false;
        return;
    }

    if (! reverbWasOn)
    {
        reverb.reset();
        reverbHP.reset();
        reverbWasOn = true;
    }

    juce::Reverb::Parameters rp;
    rp.roomSize   = prm.revSize->load();
    rp.damping    = prm.revDamp->load();
    rp.width      = prm.revWidth->load();
    rp.wetLevel   = 1.0f;
    rp.dryLevel   = 0.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    const int nCh = std::min (buffer.getNumChannels(), reverbBuffer.getNumChannels());
    if (reverbBuffer.getNumSamples() < numSamples)
        reverbBuffer.setSize (reverbBuffer.getNumChannels(), numSamples, false, false, true);

    for (int ch = 0; ch < nCh; ++ch)
        reverbBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    juce::dsp::AudioBlock<float> wet (reverbBuffer.getArrayOfWritePointers(), (size_t) nCh, (size_t) numSamples);
    juce::dsp::ProcessContextReplacing<float> ctx (wet);
    reverb.process (ctx);

    reverbHP.setCutoffFrequency (prm.revLowCut->load());
    reverbHP.process (ctx);

    const float mix = prm.revMix->load();
    for (int ch = 0; ch < nCh; ++ch)
        buffer.addFrom (ch, 0, reverbBuffer, ch, 0, numSamples, mix);
}

//==============================================================================
// Cadeia: OSC -> FILTRO -> AMP -> DRIVE -> EQ GRÁFICO -> REVERB -> BASS BOOST -> MAXIMIZER -> OUTPUT
//==============================================================================
void DubBassAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = std::min (buffer.getNumChannels(), 2);
    buffer.clear();

    if (numSamples == 0 || numChannels == 0)
        return;

    // teclas clicadas na interface entram no MIDI; o MIDI da DAW acende as teclas
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    // 1) Sintetizador mono com MIDI sample-accurate
    updateVoiceParams();
    cutoffSm.setTargetValue (prm.cutoff->load());
    voiceFilter.setResonance (prm.reso->load());

    auto* mono = buffer.getWritePointer (0);
    int pos = 0;

    for (const auto metadata : midi)
    {
        const int t = juce::jlimit (0, numSamples, metadata.samplePosition);
        renderVoice (mono, pos, t);
        pos = t;
        handleMidi (metadata.getMessage());
    }
    renderVoice (mono, pos, numSamples);

    for (int ch = 1; ch < numChannels; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numChannels, (size_t) numSamples);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // 2) Distorção / saturação harmônica
    processDrive (block);

    // 2b) FITA: "head bump" de grave do gravador (+2.5 dB em 90 Hz) quando a fita está ligada
    {
        const bool tapeOn = prm.drvOn->load() > 0.5f && juce::roundToInt (prm.drvType->load()) == 3;
        headBumpDbSm.setTargetValue (tapeOn ? 2.5f : 0.0f);
        if (headBumpNeedsUpdate || headBumpDbSm.isSmoothing())
        {
            const float db = headBumpDbSm.skip (numSamples);
            *headBump.state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (
                                  getSampleRateSafe(), 90.0f, 0.8f, juce::Decibels::decibelsToGain (db));
            headBumpNeedsUpdate = headBumpDbSm.isSmoothing();
        }
        headBump.process (ctx);
    }

    // 3) EQ gráfico 10 bandas
    updateEq();
    for (auto& band : eqBands)
        band.process (ctx);

    // 4) Reverb
    processReverb (buffer, numSamples);

    // 5) Bass boost 60 Hz +4 dB (pré-maximizer)
    updateBoost (numSamples);
    boostFilter.process (ctx);

    auto storePeaks = [&] (std::array<std::atomic<float>, 2>& dest)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            const int src = std::min (ch, numChannels - 1);
            dest[(size_t) ch].store (juce::Decibels::gainToDecibels (buffer.getMagnitude (src, 0, numSamples), -100.0f));
        }
    };

    // 6) Maximizer (com bypass)
    storePeaks (inPeakDb);
    maximizer.setParameters (prm.maxGain->load(), prm.maxCeiling->load(),
                             prm.maxRelease->load(), prm.maxBypass->load() > 0.5f);
    const float minGain = maximizer.process (buffer, numSamples);
    gainReductionDb.store (juce::Decibels::gainToDecibels (minGain, -60.0f));

    // 7) Saída
    outGainSm.setTargetValue (juce::Decibels::decibelsToGain (prm.output->load()));
    outGainSm.applyGain (buffer, numSamples);

    storePeaks (outPeakDb);
    currentNote.store (voice.isActive() ? voice.getCurrentNote() : -1.0f);

    scopeFifo.push (buffer.getReadPointer (0), numSamples);
    midi.clear();
}

//==============================================================================
juce::AudioProcessorEditor* DubBassAudioProcessor::createEditor()
{
    return new DubBassAudioProcessorEditor (*this);
}

void DubBassAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DubBassAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DubBassAudioProcessor();
}
