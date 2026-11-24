// Desenha a interface do Mastor Orch Hit MOH-88 em PNG e testa o plugin inteiro, sem DAW:
//   cmake -B build-snap -DMOH_SNAPSHOT=ON -DMOH_FORMATS=VST3 && cmake --build build-snap --target OrchHitSnapshot
//   ./build-snap/OrchHitSnapshot_artefacts/Release/OrchHitSnapshot docs/interface.png
// Grava docs/interface.png (aba SOM) e docs/interface-midis.png (aba MIDIs E GRADE). Confere o padrão de fábrica,
// o PREVIEW da grade, o TAMANHO DA NOTA, o estado salvo (com a grade e os MIDIs do usuário) e o arquivo MIDI.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

static int fails = 0;
static void check (bool c, const char* msg) { std::printf ("%s %s\n", c ? "ok    " : "FALHOU", msg); if (! c) ++fails; }
static void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

static double renderRms (OrchHitProcessor& p, double seconds, juce::MidiBuffer* first = nullptr, float* peakOut = nullptr)
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
    char msg[200];

    OrchHitProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    check ((int) proc.get ("timbre") == 0 && (int) proc.get ("noteLen") == 0 && proc.get ("twoHands") > 0.5f, "fábrica: ORCH5, nota 1/64, duas mãos");
    check (proc.get ("echoOn") < 0.5f && proc.get ("satOn") < 0.5f, "fábrica: Tape Echo e Saturador desligados");
    check (proc.getGrid().size() == 8 && ! proc.gridIsCustom(), "fábrica: o groove PONTUAÇÃO NO 2 E 4 já desenhado na grade");

    // PREVIEW: a grade toca (com a bateria)
    proc.setParam ("preview", 1.0f);
    float pk = 0;
    const double rms = renderRms (proc, 6.0, nullptr, &pk);
    std::snprintf (msg, sizeof msg, "PREVIEW toca a grade (RMS %.4f, pico %.2f)", rms, pk);
    check (rms > 0.01 && pk <= 1.0f, msg);
    proc.setParam ("preview", 0.0f);
    renderRms (proc, 3.0);
    check (renderRms (proc, 0.5) < 1e-4, "parou: nenhuma nota presa");

    // teclado / MIDI da DAW: com 1/64 a nota se solta sozinha, mesmo sem note off
    {
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 38, (juce::uint8) 110), 0);
        check (renderRms (proc, 0.08, &m) > 0.01, "MIDI toca o hit");
        renderRms (proc, 2.5);
        { juce::MidiBuffer o; o.addEvent (juce::MidiMessage::noteOff (1, 38), 0); renderRms (proc, 0.01, &o); }
        check (renderRms (proc, 0.3) < 1e-4, "1/64: a nota se soltou sozinha");
        proc.setParam ("noteLen", 6.0f);   // LIVRE
        renderRms (proc, 0.3, &m);
        juce::MidiBuffer off; off.addEvent (juce::MidiMessage::noteOff (1, 38), 0);
        renderRms (proc, 2.5, &off);
        check (renderRms (proc, 0.3) < 1e-4, "LIVRE: solta no note off");
        proc.setParam ("noteLen", 0.0f);
    }
    // todos os timbres soam
    for (int t = 0; t < moh::numPresets(); ++t)
    {
        proc.setParam ("timbre", (float) t);
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 110), 0);
        const double r = renderRms (proc, 0.1, &m);
        juce::MidiBuffer off; off.addEvent (juce::MidiMessage::noteOff (1, 45), 0);
        renderRms (proc, 2.0, &off);
        std::snprintf (msg, sizeof msg, "timbre %s soa (RMS %.3f)", moh::presets()[(size_t) t].name, r);
        check (r > 0.01, msg);
    }
    proc.setParam ("timbre", 0.0f);
    // efeitos ligados não estouram
    proc.setParam ("echoOn", 1.0f); proc.setParam ("satOn", 1.0f); proc.setParam ("preview", 1.0f);
    renderRms (proc, 4.0, nullptr, &pk);
    check (pk <= 1.0f, "Tape Echo + Saturador ligados sem estourar");
    proc.setParam ("echoOn", 0.0f); proc.setParam ("satOn", 0.0f); proc.setParam ("preview", 0.0f);
    renderRms (proc, 3.0);

    // grade editada + MIDIs do usuário + estado
    {
        proc.loadGroove (2);
        check (proc.getGrid().size() == 10 && ! proc.gridIsCustom(), "groove RESPOSTA 3a → 4 desenhado");
        auto g = proc.getGrid(); g.push_back ({ 100, 4, 40 }); proc.setGrid (g, true);
        proc.userClips.push_back ({ "MEU MIDI 1", { { 16, 1, 0 }, { 48, 1, 0 } } });
        juce::MemoryBlock mb; proc.getStateInformation (mb);
        OrchHitProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
        check (p2.getGrid().size() == 11 && p2.gridIsCustom() && (int) p2.get ("groove") == 2, "estado: a grade editada volta");
        check (p2.userClips.size() == 1 && p2.userClips[0].notes.size() == 2, "estado: os MIDIs do usuário voltam");
        const auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("moh88-teste.mid");
        check (proc.writeMidiFile (f) && f.getSize() > 60, "arquivo MIDI da grade");
        proc.userClips.clear();
        proc.loadGroove (0);
    }

    // capturas
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* he = dynamic_cast<OrchHitEditor*> (ed.get());
    pump (200);
    const juce::File base = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    const char* suffix[2] = { "", "-midis" };
    for (int page = 1; page >= 0; --page)
    {
        he->setPage (page);
        pump (150);
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
