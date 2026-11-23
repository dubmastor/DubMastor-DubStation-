// Renderiza a interface do Mastor Dub Horns em PNGs (uma imagem por aba) e um WAV de teste, sem DAW:
//   cmake -B build-snap -DHORNS_SNAPSHOT=ON && cmake --build build-snap --target HornsSnapshot
//   ./build-snap/HornsSnapshot_artefacts/Release/HornsSnapshot docs/          (PNGs)
//   ./build-snap/HornsSnapshot_artefacts/Release/HornsSnapshot teste.wav      (4 compassos da frase no clock interno + .mid)
//   ./build-snap/HornsSnapshot_artefacts/Release/HornsSnapshot transcrever audio.mp3 [bpm] [mix]   (transcrição, como na aba Frases)
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/Transcriber.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String out = argc > 1 ? juce::String (argv[1]) : juce::String ("docs/");
    const float scale = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;

    if (out == "transcrever" && argc > 2)
    {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (juce::File (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]))));
        if (rd == nullptr) { std::printf ("nao abriu %s\n", argv[2]); return 1; }
        const int n = (int) rd->lengthInSamples;
        juce::AudioBuffer<float> buf ((int) rd->numChannels, n);
        rd->read (&buf, 0, n, 0, true, true);
        std::vector<float> mono ((size_t) n);
        for (int i = 0; i < n; ++i) { float v = 0; for (int c = 0; c < buf.getNumChannels(); ++c) v += buf.getSample (c, i); mono[(size_t) i] = v / (float) buf.getNumChannels(); }
        const auto r = horns::transcribe (mono, rd->sampleRate, argc > 3 ? std::atof (argv[3]) : 74.0, argc > 4);
        std::printf ("formato=%s sr=%.0f notas=%zu sobras=%d\n", rd->getFormatName().toRawUTF8(), rd->sampleRate, r.phrase.events.size(), r.dropped);
        for (const auto& e : r.phrase.events) std::printf ("  %2d dur %2d midi %d %s\n", e.start, e.dur, e.note, horns::articName (e.artic));
        return 0;
    }

    HornsProcessor proc;
    proc.prepareToPlay (48000.0, 512);

    if (out.endsWithIgnoreCase (".wav"))
    {
        if (auto* p = proc.apvts.getParameter ("phrasePlay")) p->setValueNotifyingHost (1.0f);
        const int total = (int) (4 * 4 * 60.0 / 74.0 * 48000.0) + 96000;
        juce::AudioBuffer<float> b (2, 512), all (2, total + 512);
        all.clear();
        juce::MidiBuffer m;
        for (int pos = 0; pos + 512 <= total; pos += 512)
        {
            m.clear();
            if (pos >= total - 96000) if (auto* p = proc.apvts.getParameter ("phrasePlay")) p->setValueNotifyingHost (0.0f);
            proc.processBlock (b, m);
            for (int c = 0; c < 2; ++c) all.copyFrom (c, pos, b, c, 0, 512);
        }
        juce::File wav = juce::File::getCurrentWorkingDirectory().getChildFile (out);
        wav.deleteFile();
        juce::WavAudioFormat fmt;
        std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (new juce::FileOutputStream (wav), 48000.0, 2, 24, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (all, 0, total);
        w.reset();
        const float peak = all.getMagnitude (0, total), rms = all.getRMSLevel (0, 0, total);
        std::printf ("WAV %s  pico=%.3f  rms=%.4f\n", wav.getFullPathName().toRawUTF8(), peak, rms);
        auto mid = wav.withFileExtension ("mid");
        const bool ok = proc.writeMidiFile (mid);
        juce::FileInputStream in (mid);
        juce::MidiFile mf; mf.readFrom (in);
        std::printf ("MIDI %s trilhas=%d\n", ok ? "ok" : "FALHOU", mf.getNumTracks());
        // estado: salva e restaura
        juce::MemoryBlock mb; proc.getStateInformation (mb);
        HornsProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
        std::printf ("estado %s (%d eventos na frase restaurada)\n", p2.getPhrase().events.size() == proc.getPhrase().events.size() ? "ok" : "FALHOU",
                     (int) p2.getPhrase().events.size());
        return (peak > 0.01f && peak <= 1.0f && ok) ? 0 : 1;
    }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* he = dynamic_cast<HornsEditor*> (ed.get());
    const char* names[6] = { "naipe", "timbre", "echo", "eq", "frases", "musicos" };
    juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    dir.createDirectory();
    for (int i = 0; i < 6; ++i)
    {
        he->panel.showPage (i);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
        auto f = dir.getChildFile (juce::String ("interface-") + names[i] + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat png;
        const bool ok = os.openedOk() && png.writeImageToStream (img, os);
        std::printf ("%s -> %s (%dx%d)\n", ok ? "OK" : "FALHOU", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    }
    return 0;
}
