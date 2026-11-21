// Renderiza a interface do Dub Chords num PNG (sem DAW):
//   cmake -B build-snap -DDUBCHORDS_SNAPSHOT=ON && cmake --build build-snap --target DubChordsSnapshot
//   ./build-snap/DubChordsSnapshot_artefacts/Release/DubChordsSnapshot docs/interface.png [4] [escala]
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String out = argc > 1 ? juce::String (argv[1]) : juce::String ("interface.png");

    DubChordsProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    if (argc > 2 && juce::String (argv[2]) == "4") // segundo argumento: "4" = mostrar os 4 acordes ligados
        if (auto* p = proc.apvts.getParameter ("on3")) p->setValueNotifyingHost (1.0f);

    // modo áudio: "snapshot saida.wav" grava 8 compassos do PREVIEW e um .mid exportado ao lado
    if (out.endsWithIgnoreCase (".wav"))
    {
        if (auto* p = proc.apvts.getParameter ("preview")) p->setValueNotifyingHost (1.0f);
        juce::AudioBuffer<float> b (2, 512), all (2, 48000 * 26);
        all.clear();
        juce::MidiBuffer m;
        const double secs = 8 * 4 * 60.0 / 75.0;          // 8 compassos a 75 BPM
        const int total = juce::jmin (all.getNumSamples(), (int) (secs * 48000.0) + 48000);
        int notes[17] {};
        for (int pos = 0; pos + 512 <= total; pos += 512)
        {
            m.clear();
            proc.processBlock (b, m);
            for (const auto meta : m) if (meta.getMessage().isNoteOn()) ++notes[meta.getMessage().getChannel()];
            for (int c = 0; c < 2; ++c) all.copyFrom (c, pos, b, c, 0, 512);
        }
        juce::File wav = juce::File::getCurrentWorkingDirectory().getChildFile (out);
        wav.deleteFile();
        juce::WavAudioFormat fmt;
        std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (new juce::FileOutputStream (wav), 48000.0, 2, 24, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (all, 0, total);
        w.reset();
        std::printf ("WAV %s  pico=%.3f  rms=%.4f  note-ons: skank(ch1)=%d baixo(ch2)=%d bubble(ch3)=%d\n",
                     wav.getFullPathName().toRawUTF8(), all.getMagnitude (0, total), all.getRMSLevel (0, 0, total), notes[1], notes[2], notes[3]);

        auto mid = wav.withFileExtension ("mid");
        proc.writeMidiFile (mid);
        juce::FileInputStream in (mid);
        juce::MidiFile mf;
        mf.readFrom (in);
        for (int t = 0; t < mf.getNumTracks(); ++t)
        {
            const auto* tr = mf.getTrack (t);
            juce::String name; int chCount[17] {}; juce::StringArray markers;
            for (const auto* ev : *tr)
            {
                const auto& msg = ev->message;
                if (msg.isTrackNameEvent()) name = msg.getTextFromTextMetaEvent();
                if (msg.isTextMetaEvent() && msg.getMetaEventType() == 6) markers.add (msg.getTextFromTextMetaEvent());
                if (msg.isNoteOn()) ++chCount[msg.getChannel()];
            }
            juce::String chs;
            for (int c = 1; c <= 16; ++c) if (chCount[c]) chs << " ch" << c << "=" << chCount[c];
            std::printf ("MIDI trilha %d '%s'%s %s\n", t, name.toRawUTF8(), chs.toRawUTF8(),
                         markers.isEmpty() ? "" : ("marcadores: " + markers.joinIntoString (" ")).toRawUTF8());
        }
        return 0;
    }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (ed->getWidth(), ed->getHeight());

    // um bloco de áudio para ter o loop montado
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    proc.processBlock (buf, midi);

    const float scale = argc > 3 ? (float) juce::String (argv[3]).getDoubleValue() : 1.0f;   // terceiro argumento: escala
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
    juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    f.getParentDirectory().createDirectory();
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat png;
    const bool ok = os.openedOk() && png.writeImageToStream (img, os);
    std::printf ("%s -> %s (%dx%d)\n", ok ? "OK" : "FALHOU", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    return ok ? 0 : 1;
}
