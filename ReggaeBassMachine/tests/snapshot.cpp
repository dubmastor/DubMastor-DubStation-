// Renderiza a interface do Reggae Bass Machine num PNG (sem DAW), com o loop tocando para o visor e o osciloscópio:
//   cmake -B build-snap -DBASSMACHINE_SNAPSHOT=ON -DBASSMACHINE_PLUGIN=OFF && cmake --build build-snap --target BassMachineSnapshot
//   ./build-snap/.../BassMachineSnapshot docs/interface.png [estilo 0-6] [timbre 0-1] [escala] [wav] [bateria 0-3] [teclado 0-2]
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;

    // autoteste da edição de MIDI: editar, salvar o projeto, reabrir, conferir; mexer no gerador apaga
    if (argc > 1 && juce::String (argv[1]) == "--edit-test")
    {
        int fails = 0;
        BassMachineProcessor a;
        a.prepareToPlay (48000.0, 512);
        double lb = 0; auto line = a.getCurrentLine (lb);
        line.push_back ({ 1.25, 0.5, 45, 110, 0, false });     // nota nova
        line.erase (line.begin());                             // apaga a primeira
        a.setEdits (line, lb);
        if (! a.hasEdits()) { std::printf ("FALHOU: setEdits\n"); ++fails; }
        double lb2 = 0; const auto got = a.getCurrentLine (lb2);
        bool found = false; for (const auto& e : got) found |= (e.note == 45 && std::abs (e.start - 1.25) < 1e-9);
        if (! found || std::abs (lb2 - lb) > 1e-9) { std::printf ("FALHOU: linha editada\n"); ++fails; }
        for (size_t i = 0; i + 1 < got.size(); ++i)
            if (got[i].start + got[i].length > got[i + 1].start + 1e-9) { std::printf ("FALHOU: sobreposicao\n"); ++fails; }

        juce::MemoryBlock mb; a.getStateInformation (mb);
        BassMachineProcessor b; b.prepareToPlay (48000.0, 512);
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        double lb3 = 0; const auto back = b.getCurrentLine (lb3);
        if (! b.hasEdits() || back.size() != got.size()) { std::printf ("FALHOU: edicao nao voltou com o projeto\n"); ++fails; }

        auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("mrbm_edit_test.mid");
        if (! b.writeMidiFile (f)) { std::printf ("FALHOU: export\n"); ++fails; }
        else
        {
            juce::FileInputStream in (f); juce::MidiFile mf; mf.readFrom (in);
            int ons = 0; for (auto* ev : *mf.getTrack (1)) ons += ev->message.isNoteOn();
            const int reps = 2;   // REPETIR padrão = 2x
            if (ons != (int) got.size() * reps) { std::printf ("FALHOU: MIDI exportado sem a edicao (%d notas)\n", ons); ++fails; }
        }

        if (auto* p = b.apvts.getParameter ("seed")) p->setValueNotifyingHost (p->convertTo0to1 (7.0f));
        b.dropStaleEdits();
        if (b.hasEdits()) { std::printf ("FALHOU: edicao deveria sumir ao mexer no gerador\n"); ++fails; }
        if (auto* p = b.apvts.getParameter ("timbre")) p->setValueNotifyingHost (p->convertTo0to1 (2.0f));
        std::printf (fails == 0 ? "EDICAO OK\n" : "%d FALHAS NA EDICAO\n", fails);
        return fails == 0 ? 0 : 1;
    }
    const juce::String out = argc > 1 ? juce::String (argv[1]) : juce::String ("interface.png");

    BassMachineProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    auto set = [&] (const char* id, float v) { if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    if (argc > 2) set ("style",  (float) juce::String (argv[2]).getIntValue());
    if (argc > 3) set ("timbre", (float) juce::String (argv[3]).getIntValue());
    // 6º e 7º argumentos: estilo da bateria (0-3) e instrumento do teclado (0-2); -1 = desligado
    if (argc > 6 && juce::String (argv[6]).getIntValue() >= 0) { set ("drumsOn", 1.0f); set ("drumStyle", (float) juce::String (argv[6]).getIntValue()); }
    if (argc > 7 && juce::String (argv[7]).getIntValue() >= 0) { set ("keysOn", 1.0f);  set ("keysMode",  (float) juce::String (argv[7]).getIntValue()); }
    // 8º argumento: riddim clássico (0..159) carregado antes de tocar
    if (argc > 8 && juce::String (argv[8]).getIntValue() >= 0) proc.loadRiddim (juce::String (argv[8]).getIntValue());
    set ("preview", 1.0f);   // PLAY ligado: cursor e osciloscópio com sinal

    // toca ~3,4 tempos para ter nota soando e onda no osciloscópio
    juce::AudioBuffer<float> buf (2, 512);
    for (int i = 0; i < 200; ++i) { juce::MidiBuffer midi; proc.processBlock (buf, midi); }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (BassMachinePanel::kW, BassMachinePanel::kH);
    // o construtor do painel já atualiza o visor da linha e o osciloscópio com o estado acima

    // quinto argumento: também grava ~2 voltas do loop tocadas pelo preview (timbres do Dub Bass) em WAV
    if (argc > 5)
    {
        juce::AudioBuffer<float> all (2, 48000 * 40);
        all.clear();
        set ("preview", 0.0f); { juce::MidiBuffer m; proc.processBlock (buf, m); }
        set ("bpm", 76.0f); set ("preview", 1.0f);
        const int blocks = (int) (36.0 * 60.0 / 76.0 * 48000.0 / 512.0);
        for (int b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer midi; proc.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch) all.copyFrom (ch, b * 512, buf, ch, 0, 512);
        }
        juce::File wf = juce::File::getCurrentWorkingDirectory().getChildFile (argv[5]);
        wf.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (wf), 48000.0, 2, 16, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (all, 0, blocks * 512);
        std::printf ("WAV -> %s (pico %.2f)\n", wf.getFullPathName().toRawUTF8(), all.getMagnitude (0, blocks * 512));
    }

    const float scale = argc > 4 ? (float) juce::String (argv[4]).getDoubleValue() : 1.0f;
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
