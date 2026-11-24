// Desenha a interface do Mastor Reggae Percussion MRP-77 em PNG e testa o plugin inteiro, sem DAW:
//   cmake -B build-snap -DMRP_SNAPSHOT=ON -DMRP_FORMATS=VST3 && cmake --build build-snap --target ReggaePercussionSnapshot
//   ./build-snap/ReggaePercussionSnapshot_artefacts/Release/ReggaePercussionSnapshot docs/interface.png
// Grava docs/interface.png (aba PADS) e docs/interface-machine.png (aba PERC MACHINE). Confere o PLAY, o MIDI que chega,
// que as notas do MRD-12 não tocam nada, o estado salvo (grade e notas) e o arquivo MIDI.
#include <juce_audio_utils/juce_audio_utils.h>
#include <set>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

static int fails = 0;
static void check (bool c, const char* msg) { std::printf ("%s %s\n", c ? "ok    " : "FALHOU", msg); if (! c) ++fails; }
static void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

static double renderRms (ReggaePercussionProcessor& p, double seconds, juce::MidiBuffer* first = nullptr, float* peakOut = nullptr)
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

    ReggaePercussionProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    check (proc.get ("satOn") < 0.5f && proc.get ("echoOn") < 0.5f, "padr\xc3\xa3o de f\xc3\xa1" "brica: saturador e Tape Echo em bypass");
    check (proc.grooveName() == "Kit roots (Seeco)" && proc.getStep (0, 2) == 2, "groove inicial: Kit roots (Seeco)");

    // PLAY do plugin
    proc.setParam ("preview", 1.0f);
    float pk = 0;
    const double rms = renderRms (proc, 4.0, nullptr, &pk);
    char msg[160]; std::snprintf (msg, sizeof msg, "PLAY toca o groove (RMS %.4f, pico %.2f)", rms, pk);
    check (rms > 0.002 && pk < 1.0f, msg);
    check (proc.trackHits[0].load() > 10, "o shaker tocou (luz do canal)");
    proc.setParam ("preview", 0.0f);
    renderRms (proc, 2.0);
    check (renderRms (proc, 0.5) < 1e-4, "parou: nenhum som preso");

    // MIDI que chega: nota 54 = pandeiro; nota 38 (caixa do MRD-12) n\xc3\xa3o toca nada
    {
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (10, 54, (juce::uint8) 110), 0);
        check (renderRms (proc, 0.3, &m) > 0.001, "MIDI nota 54 toca o pandeiro");
        renderRms (proc, 1.0);
        juce::MidiBuffer d; d.addEvent (juce::MidiMessage::noteOn (10, 38, (juce::uint8) 110), 0);
        check (renderRms (proc, 0.3, &d) < 1e-5, "nota 38 (caixa do MRD-12) n\xc3\xa3o toca nada aqui");
    }

    // estado: grade, sons e notas
    {
        proc.loadGroove (1);
        proc.setStep (4, 40, 2);
        proc.setParam ("snd2", 18.0f);
        proc.padNote[0] = 84;
        proc.setParam ("bars", 3.0f);
        juce::MemoryBlock mb; proc.getStateInformation (mb);
        ReggaePercussionProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
        check (p2.getStep (4, 40) == 2 && p2.soundOf (2) == 18 && p2.padNote[0].load() == 84 && p2.numBars() == 8 && p2.grooveIndex.load() == 1,
               "estado: grade, pe\xc3\xa7" "a trocada, nota ajustada e compassos voltam");
        const auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("mrp77-teste.mid");
        check (proc.writeMidiFile (f) && f.getSize() > 100, "arquivo MIDI (groove inteiro)");
        juce::FileInputStream in (f); juce::MidiFile mf; mf.readFrom (in);
        int chans = 0; std::set<int> seen;
        if (mf.getNumTracks() > 1) for (auto* e : *mf.getTrack (1)) if (e->message.isNoteOn()) seen.insert (e->message.getChannel());
        chans = (int) seen.size();
        check (chans == 8, "MIDI colorido: 8 compassos = 8 canais");
        check (proc.writeMidiFile (f, -1, 2) && f.getSize() > 30, "arquivo MIDI de um compasso s\xc3\xb3");
        proc.padNote[0] = 82; proc.setParam ("snd2", 6.0f); proc.setParam ("bars", 2.0f); proc.loadGroove (0);
    }

    // capturas
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* pe = dynamic_cast<ReggaePercussionEditor*> (ed.get());
    proc.setParam ("preview", 1.0f);
    renderRms (proc, 0.9);
    pump (250);
    const juce::File base = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    const char* suffix[2] = { "", "-machine" };
    for (int page = 0; page < 2; ++page)
    {
        pe->setPage (page);
        renderRms (proc, 0.12);
        pump (60);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f = base.getSiblingFile (base.getFileNameWithoutExtension() + suffix[page] + base.getFileExtension());
        f.getParentDirectory().createDirectory(); f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat png;
        const bool w = png.writeImageToStream (img, os);
        std::printf ("%s %s (%dx%d)\n", w ? "PNG" : "FALHOU", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
        if (! w) ++fails;
    }
    proc.setParam ("preview", 0.0f);
    std::printf (fails == 0 ? "\nTUDO OK\n" : "\n%d FALHA(S)\n", fails);
    return fails == 0 ? 0 : 1;
}
