// Renderiza a interface do Mastor Reggae Chord Machine (MRCM-1240) num PNG (sem DAW):
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

    // modo "ruido": liga o PREVIEW, desliga/liga o SOM INTERNO no meio das notas, para o PREVIEW
    // e mede se ficou algum som preso (tem que dar silêncio)
    if (out == "ruido")
    {
        auto set = [&] (const char* id, float v) { if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (v); };
        juce::AudioBuffer<float> b (2, 512);
        juce::MidiBuffer m;
        auto run = [&] (double secs) { float pk = 0; for (int n = 0; n < (int) (secs * 48000 / 512); ++n) { m.clear(); proc.processBlock (b, m); pk = juce::jmax (pk, b.getMagnitude (0, 512)); } return pk; };
        int fails = 0;
        for (int trial = 0; trial < 12; ++trial)
        {
            set ("preview", 1.0f);
            run (1.0 + 0.137 * trial);       // para em pontos diferentes do compasso
            set ("sound", 0.0f);
            run (0.4 + 0.05 * trial);
            set ("sound", 1.0f);
            run (0.6);
            set ("preview", 0.0f);
            run (2.5);                        // cauda do reverb/Leslie
            const float tail = run (1.0);
            std::printf ("tentativa %2d: pico depois de parar = %.5f %s\n", trial, tail, tail < 0.001f ? "ok" : "SOM PRESO");
            if (tail >= 0.001f) ++fails;
        }
        std::printf (fails == 0 ? "SEM RUIDO PRESO\n" : "%d TENTATIVA(S) COM SOM PRESO\n", fails);
        return fails == 0 ? 0 : 1;
    }

    // modo "mexer": com o PREVIEW tocando, muda acordes, inversões, padrão, nota, bubble e volumes
    // a cada ~0,15 s (200 vezes). Confere: MIDI sem nota presa, som que para ao parar o preview
    // e nenhum estalo (salto entre amostras muito maior que o normal do próprio timbre).
    if (out == "mexer")
    {
        proc.setNonRealtime (true);   // monta o loop novo no próprio bloco (sem thread de mensagens no teste)
        auto set = [&] (const juce::String& id, float norm) { if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (norm); };
        juce::AudioBuffer<float> b (2, 512);
        juce::MidiBuffer m;
        bool on[17][128] {};
        double onSince[17][128] {};
        double now = 0, longest = 0;   // nota mais longa (s) enquanto toca: o maior tamanho de nota é 1/2 = 2 tempos
        float prevL = 0, maxJump = 0, peak = 0;
        auto run = [&] (double secs)
        {
            for (int n = 0; n < (int) (secs * 48000 / 512); ++n)
            {
                m.clear();
                proc.processBlock (b, m);
                for (const auto meta : m)
                {
                    const auto msg = meta.getMessage();
                    const double t = now + meta.samplePosition / 48000.0;
                    const int c = msg.getChannel(), nn = msg.getNoteNumber();
                    if (msg.isNoteOn()) { if (! on[c][nn]) onSince[c][nn] = t; on[c][nn] = true; }
                    else if (msg.isNoteOff() && on[c][nn]) { longest = std::max (longest, t - onSince[c][nn]); on[c][nn] = false; }
                }
                now += 512 / 48000.0;
                const float* L = b.getReadPointer (0);
                for (int i = 0; i < 512; ++i) { maxJump = juce::jmax (maxJump, std::abs (L[i] - prevL)); prevL = L[i]; peak = juce::jmax (peak, std::abs (L[i])); }
            }
        };
        // referência: o mesmo tipo de loop tocando sem mexer em nada
        set ("on2", 1.0f); set ("on3", 1.0f); set ("bubble", 1.0f); set ("preview", 1.0f);
        run (4.0);
        const float baseJump = maxJump;
        maxJump = 0;

        juce::Random rnd (1240);
        for (int k = 0; k < 200; ++k)
        {
            const int i = rnd.nextInt (4);
            const juce::String n (i);
            switch (rnd.nextInt (9))
            {
                case 0: set ("root" + n, rnd.nextFloat()); break;
                case 1: set ("qual" + n, rnd.nextBool() ? 1.0f : 0.0f); break;
                case 2: set ("dur" + n, rnd.nextFloat()); break;
                case 3: set ("inv" + n, rnd.nextFloat()); break;
                case 4: set ("pattern", rnd.nextFloat()); break;
                case 5: set ("noteSize", rnd.nextFloat()); break;
                case 6: set ("bubble", rnd.nextBool() ? 1.0f : 0.0f); break;
                case 7: if (i > 0) set ("on" + n, rnd.nextBool() ? 1.0f : 0.0f); break;
                default: set (rnd.nextBool() ? "pianoLvl" : "organLvl", rnd.nextFloat()); break;
            }
            run (0.15);
        }
        const float changeJump = maxJump;
        set ("preview", 0.0f);
        run (3.0);
        peak = 0;
        run (1.0);
        int stuck = 0;
        for (auto& ch : on) for (bool x : ch) stuck += x;
        std::printf ("salto max entre amostras: tocando parado=%.4f  mexendo=%.4f\n", baseJump, changeJump);
        std::printf ("notas MIDI presas depois de parar: %d   som depois de parar: %.5f\n", stuck, peak);
        const double maxNote = 2.0 * 60.0 / 75.0 + 0.05;   // 1/2 (2 tempos) a 75 BPM
        std::printf ("nota mais longa enquanto tocava: %.2f s (limite %.2f s)\n", longest, maxNote);
        const bool ok = stuck == 0 && peak < 0.001f && longest <= maxNote && changeJump < baseJump * 1.5f + 0.02f;
        std::printf (ok ? "MEXER DURANTE O PREVIEW: OK\n" : "MEXER DURANTE O PREVIEW: PROBLEMA\n");
        return ok ? 0 : 1;
    }

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
