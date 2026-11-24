// Desenha a interface do Mastor Reggae Clavinet MRCL-71 em PNG e testa o plugin inteiro, sem DAW:
//   cmake -B build-snap -DRCL_SNAPSHOT=ON -DRCL_FORMATS=VST3 && cmake --build build-snap --target ReggaeClavinetSnapshot
//   ./build-snap/ReggaeClavinetSnapshot_artefacts/Release/ReggaeClavinetSnapshot docs/interface.png
// Grava docs/interface.png (CLAVINET MACHINE) e docs/interface-instrumento.png. Confere o PREVIEW, o MIDI da trilha,
// o estado salvo (com o MIDI editado) e o arquivo MIDI.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "RiddimBank.h"

static int fails = 0;
static void check (bool c, const char* msg) { std::printf ("%s %s\n", c ? "ok    " : "FALHOU", msg); if (! c) ++fails; }
static void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

static double renderRms (ReggaeClavinetProcessor& p, double seconds, juce::MidiBuffer* first = nullptr, float* peakOut = nullptr)
{
    juce::AudioBuffer<float> b (2, 512);
    double acc = 0; long n = 0; float pk = 0;
    for (int pos = 0; pos < (int) (48000 * seconds); pos += 512)
    {
        b.clear(); juce::MidiBuffer m; if (first != nullptr && pos == 0) m = *first;
        p.processBlock (b, m);
        for (int i = 0; i < 512; ++i) { const float v = b.getSample (0, i); acc += v * v; pk = std::max (pk, std::abs (v)); }
        n += 512;
    }
    if (peakOut) *peakOut = pk;
    return std::sqrt (acc / (double) std::max (1L, n));
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String out = argc > 1 ? juce::String (argv[1]) : juce::String ("docs/interface.png");

    ReggaeClavinetProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    // timbre inicial: M1
    check ((int) proc.get ("timbre") == 0 && proc.get ("m1") > 0.5f && proc.get ("echoOn") < 0.5f, "timbre inicial M1, Tape Echo desligado");
    // "Satta Massagana" do banco único
    for (int i = 0; i < riddimbank::numEntries; ++i)
        if (juce::String (riddimbank::entries[i].name) == "Satta Massagana") proc.applyBankEntry (i);
    pump (150);
    check (proc.getChords().size() == 4 && proc.getChords()[0].root == 9, "banco de riddims: Satta Massagana (Am Dm Am Em)");

    // PREVIEW: a linha toca
    proc.setParam ("preview", 1.0f);
    float pk = 0;
    const double rms = renderRms (proc, 6.0, nullptr, &pk);
    char msg[160]; std::snprintf (msg, sizeof msg, "PREVIEW toca a linha (RMS %.4f, pico %.2f)", rms, pk);
    check (rms > 0.003 && pk < 1.0f, msg);
    proc.setParam ("preview", 0.0f);
    renderRms (proc, 1.5);
    check (renderRms (proc, 0.5) < 1e-4, "parou: nenhuma nota presa");

    // instrumento: MIDI tocando direto
    {
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0);
        check (renderRms (proc, 0.3, &m) > 0.002, "MIDI toca o clavinet");
        juce::MidiBuffer off; off.addEvent (juce::MidiMessage::noteOff (1, 48), 0);
        renderRms (proc, 0.5, &off);
    }
    // MIDI da trilha: o acorde que chega vira a harmonia da linha
    {
        proc.setParam ("machineOn", 1.0f); proc.setParam ("src", 1.0f); proc.setParam ("preview", 1.0f);
        juce::MidiBuffer m;
        for (int n : { 55, 59, 62 }) m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 90), 0);   // G maior
        renderRms (proc, 0.2, &m);
        check (proc.midiChordRoot.load() == 7 && proc.midiChordMinor.load() == 0, "MIDI da trilha: ouviu G maior");
        check (renderRms (proc, 3.0) > 0.002, "MIDI da trilha: a linha toca em G");
        proc.setParam ("preview", 0.0f); proc.setParam ("machineOn", 0.0f); proc.setParam ("src", 0.0f);
        renderRms (proc, 1.0);
    }

    // MIDI editado + estado
    {
        auto l = proc.getCurrentLine();
        l.events.erase (l.events.begin());
        proc.setEdits (l);
        proc.setParam ("musician", 3.0f); proc.setParam ("fill", 1.0f); proc.setParam ("center", 53.0f);
        proc.setEdits (proc.getCurrentLine());   // edita de novo depois de mudar (o timer ainda não rodou)
        pump (120);
        juce::MemoryBlock mb; proc.getStateInformation (mb);
        ReggaeClavinetProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
        check ((int) p2.get ("musician") == 3 && (int) p2.get ("center") == 53 && p2.getBankEntry() == proc.getBankEntry(), "estado: m\xc3\xbasico, oitava e riddim voltam");
        check (p2.hasEdits() == proc.hasEdits(), "estado: MIDI editado volta");
        const auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("mrcl71-teste.mid");
        check (proc.writeMidiFile (f) && f.getSize() > 100, "arquivo MIDI");
        proc.clearEdits();
    }

    // capturas
    proc.setParam ("musician", 0.0f); proc.setParam ("fill", 0.0f); proc.setParam ("center", 29.0f);
    pump (120);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* ce = dynamic_cast<ReggaeClavinetEditor*> (ed.get());
    pump (200);
    const juce::File base = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    const char* suffix[2] = { "-instrumento", "" };
    for (int page = 1; page >= 0; --page)
    {
        ce->setPage (page);
        pump (120);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f = base.getSiblingFile (base.getFileNameWithoutExtension() + suffix[page] + base.getFileExtension());
        f.getParentDirectory().createDirectory(); f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat png;
        const bool w = png.writeImageToStream (img, os);
        std::printf ("%s %s (%dx%d)\n", w ? "PNG" : "FALHOU", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
        if (! w) ++fails;
    }
    std::printf (fails == 0 ? "\nTUDO OK\n" : "\n%d FALHA(S)\n", fails);
    return fails == 0 ? 0 : 1;
}
