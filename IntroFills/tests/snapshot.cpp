// Desenha a interface do Mastor Intro Fills MIF em PNG e testa o plugin inteiro, sem DAW:
//   cmake -B build-snap -DMIF_SNAPSHOT=ON -DMIF_FORMATS=VST3 && cmake --build build-snap --target IntroFillsSnapshot
//   ./build-snap/IntroFillsSnapshot_artefacts/Release/IntroFillsSnapshot docs/interface.png
// Confere: o PREVIEW toca a virada (som e notas na saída MIDI), Tape Echo e saturador começam desligados,
// a edição da grade fica salva no estado do projeto, o .mid exportado e a automação de todos os parâmetros sem NaN.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String out = argc > 1 ? juce::String (argv[1]) : juce::String ("docs/interface.png");
    const float scale = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;
    bool ok = true;
    auto check = [&] (bool c, const char* msg) { std::printf ("%s %s\n", c ? "ok    " : "FALHOU", msg); ok = ok && c; };

    IntroFillsProcessor proc;
    check (proc.get ("echoOn") < 0.5f && proc.get ("satOn") < 0.5f, "Tape Echo e saturador comecam desligados");
    check (proc.get ("play") < 0.5f, "PREVIEW comeca parado");
    proc.selectDrummer (1);                       // Sly Dunbar, timbales picados
    proc.setParam ("style", 1.0f);                // rockers
    proc.prepareToPlay (48000.0, 512);
    proc.setParam ("play", 1.0f);
    juce::AudioBuffer<float> b (2, 512);
    juce::MidiBuffer m;
    int midiNotes = 0; float peak = 0; bool finite = true;
    const int total = (int) (48000.0 * 60.0 / 74.0 * 3.4);   // para no meio da virada
    for (int pos = 0; pos < total; pos += 512)
    {
        b.clear(); m.clear(); proc.processBlock (b, m);
        for (const auto ev : m) if (ev.getMessage().isNoteOn()) ++midiNotes;
        peak = std::max (peak, b.getMagnitude (0, 512));
        for (int i = 0; i < 512; ++i) finite = finite && std::isfinite (b.getSample (0, i));
    }
    std::printf ("notas %d pico %.3f\n", midiNotes, peak);
    check (midiNotes > 4 && peak > 0.05f && peak < 1.5f && finite, "PREVIEW toca a virada (som e notas na saida MIDI)");

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* fe = dynamic_cast<IntroFillsEditor*> (ed.get());
    fe->refreshForSnapshot();
    auto save = [&] (const juce::File& f)
    {
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat png;
        const bool w = png.writeImageToStream (img, os);
        std::printf ("%s %s\n", w ? "PNG" : "FALHOU", f.getFullPathName().toRawUTF8());
        ok = ok && w;
    };
    const juce::File base (juce::File::getCurrentWorkingDirectory().getChildFile (out));
    save (base);

    // edição da grade: liga uma nota, salva no estado e volta
    proc.setParam ("play", 0.0f);
    proc.selectDrummer (0); proc.setParam ("fill", 1.0f);
    const size_t before = proc.currentFill().size();
    proc.toggleCell (mif::Tom1, 9);
    fe->refreshForSnapshot();
    save (base.getSiblingFile (base.getFileNameWithoutExtension() + "-editada" + base.getFileExtension()));
    juce::MemoryBlock mb; proc.getStateInformation (mb);
    IntroFillsProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
    check (p2.isEdited() && p2.currentFill().size() == before + 1 && p2.drummer() == 0 && p2.fill() == 1, "virada editada fica salva no projeto");
    check (p2.get ("play") < 0.5f, "projeto reaberto nao comeca tocando sozinho");
    proc.resetFill();
    check (! proc.isEdited() && proc.currentFill().size() == before, "VOLTAR AO ORIGINAL");

    // .mid
    auto mf = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("mif_test.mid");
    check (proc.writeMidiFile (mf) && mf.getSize() > 60, "MIDI exportado");

    // automação estilo pluginval: todos os parâmetros aleatórios a cada 32 amostras, com notas MIDI chegando
    {
        juce::Random rnd (7);
        bool fin = true;
        for (double sr : { 44100.0, 96000.0 })
            for (int bs : { 64, 512 })
            {
                proc.prepareToPlay (sr, bs);
                juce::AudioBuffer<float> bb (2, bs);
                for (int k = 0; k < 40; ++k)
                {
                    for (auto* prm : proc.getParameters()) if (rnd.nextInt (3) == 0) prm->setValueNotifyingHost (rnd.nextFloat());
                    juce::MidiBuffer mm;
                    mm.addEvent (juce::MidiMessage::noteOn (10, 36 + rnd.nextInt (16), (juce::uint8) (1 + rnd.nextInt (127))), rnd.nextInt (bs));
                    bb.clear(); proc.processBlock (bb, mm);
                    for (int i = 0; i < bs; ++i) fin = fin && std::isfinite (bb.getSample (0, i)) && std::abs (bb.getSample (0, i)) < 4.0f;
                }
            }
        check (fin, "automacao de todos os parametros sem NaN nem estouro");
    }
    std::printf (ok ? "TUDO OK\n" : "HA FALHAS\n");
    return ok ? 0 : 1;
}
