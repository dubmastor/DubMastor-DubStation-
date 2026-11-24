// Desenha a interface da Monstro Machine MM-2500 em PNG e testa o plugin inteiro, sem DAW:
//   cmake -B build-snap -DMM_SNAPSHOT=ON -DMM_FORMATS=VST3 && cmake --build build-snap --target MonstroSnapshot
//   ./build-snap/MonstroSnapshot_artefacts/Release/MonstroSnapshot docs/interface.png
// Grava docs/interface.png (MAIN) e as telas PROGRAM, TIMBRE e MIXER. Confere o PLAY, o MIDI que chega, a troca de kit
// (BPM e groove do estilo), a troca de timbre dentro da família, o estado salvo e o arquivo MIDI.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

static int fails = 0;
static void check (bool c, const char* msg) { std::printf ("%s %s\n", c ? "ok    " : "FALHOU", msg); if (! c) ++fails; }
static void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

static double renderRms (MonstroProcessor& p, double seconds, juce::MidiBuffer* first = nullptr, float* peakOut = nullptr)
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

    MonstroProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    check (proc.get ("satOn") < 0.5f && proc.get ("echoOn") < 0.5f, "padr\xc3\xa3o de f\xc3\xa1" "brica: saturador e Tape Echo em bypass");
    check (proc.kit() == 0 && (int) proc.get ("bpm") == mm::kits()[0].bpm, "kit inicial DubMastor no BPM do kit");
    check (proc.getStep (0, 4, 4) == 2 && proc.getStep (0, 4, 12) == 2, "groove reggae: aro com acento no 2 e no 4");

    proc.setParam ("preview", 1.0f);
    float pk = 0;
    const double rms = renderRms (proc, 4.0, nullptr, &pk);
    char msg[200]; std::snprintf (msg, sizeof msg, "PLAY toca o groove (RMS %.4f, pico %.2f)", rms, pk);
    check (rms > 0.005 && pk <= 1.05f, msg);
    check (proc.padHits[0].load() >= 4, "o kick tocou (luz do pad)");
    proc.setParam ("preview", 0.0f);
    renderRms (proc, 2.5);
    check (renderRms (proc, 0.5) < 1e-4, "parou: nenhum som preso");

    {
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (10, 38, (juce::uint8) 110), 0);
        check (renderRms (proc, 0.3, &m) > 0.001, "MIDI nota 38 toca a caixa (pad 3)");
        renderRms (proc, 1.0);
    }

    // troca de kit: BPM e groove do estilo
    {
        int trap = -1; for (int k = 0; k < mm::numKits(); ++k) if (juce::String (mm::kits()[(size_t) k].style) == "trap") { trap = k; break; }
        proc.selectKit (trap);
        check (proc.kit() == trap && (int) proc.get ("bpm") == mm::kits()[(size_t) trap].bpm && proc.getStep (0, 3, 8) == 2, "kit de trap: BPM e groove trap");
        proc.selectKit (0);
    }

    // timbres da mesma família do mesmo produtor
    {
        check (proc.varCount (2) == 338, "DubMastor: 338 snares para o \xe2\x97\x80 \xe2\x96\xb6");
        proc.setVariation (2, -1);
        check (proc.variation (2) == 337 && proc.timbreName (2) == "SNARE 338", "\xe2\x97\x80 no primeiro timbre vai para o \xc3\xbaltimo");
        proc.setVariation (2, 4);
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (10, 38, (juce::uint8) 110), 0);
        check (renderRms (proc, 0.3, &m) > 0.001, "SNARE 05 toca");
        renderRms (proc, 1.0);
    }

    // estado
    {
        proc.setStep (2, 7, 9, 2);
        proc.setParam ("seq", 2.0f);
        proc.setParam ("tune3", 5.0f);
        juce::MemoryBlock mb; proc.getStateInformation (mb);
        MonstroProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
        check (p2.getStep (2, 7, 9) == 2 && p2.seq() == 2 && p2.variation (2) == 4 && (int) p2.get ("tune3") == 5, "estado: SEQ, passo, timbre e tune voltam");
        proc.setParam ("seq", 0.0f); proc.setParam ("tune3", 0.0f); proc.setStep (2, 7, 9, 0);
        const auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("mm2500-teste.mid");
        check (proc.writeMidiFile (f, 0) && f.getSize() > 60, "arquivo MIDI do SEQ A");
        juce::FileInputStream in (f); juce::MidiFile mf; mf.readFrom (in);
        int notes = 0; double last = 0;
        if (mf.getNumTracks() > 1) for (auto* e : *mf.getTrack (1)) if (e->message.isNoteOn()) { ++notes; last = std::max (last, e->message.getTimeStamp()); }
        check (notes == proc.notesIn (0) && last < 4 * 480, "MIDI: as notas do SEQ A em 1 compasso");
        check (proc.writeMidiFile (f, -1), "arquivo MIDI de A a D");
    }

    proc.setVariation (2, 0);
    // capturas
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* me = dynamic_cast<MonstroEditor*> (ed.get());
    proc.setParam ("preview", 1.0f);
    renderRms (proc, 0.9);
    pump (250);
    const juce::File base = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    const char* screens[4] { "main", "kit", "timbre", "mixer" };
    for (int i = 0; i < 4; ++i)
    {
        proc.uiPad = i == 2 ? 2 : 0;
        me->act (juce::String ("go:") + screens[i]);
        renderRms (proc, 0.12);
        pump (80);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f = i == 0 ? base : base.getSiblingFile (base.getFileNameWithoutExtension() + "-" + screens[i] + base.getFileExtension());
        f.getParentDirectory().createDirectory(); f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat png;
        const bool w = png.writeImageToStream (img, os);
        std::printf ("%s %s (%dx%d)\n", w ? "PNG" : "FALHOU", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
        if (! w) ++fails;
    }
    me->act ("go:main");
    proc.setParam ("preview", 0.0f);
    std::printf (fails == 0 ? "\nTUDO OK\n" : "\n%d FALHA(S)\n", fails);
    return fails == 0 ? 0 : 1;
}
