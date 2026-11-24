// Desenha a interface do Mastor Pick Guitar MPG-11 em PNG, sem DAW:
//   cmake -B build-snap -DPG_SNAPSHOT=ON -DPG_FORMATS=VST3 && cmake --build build-snap --target PickGuitarSnapshot
//   ./build-snap/PickGuitarSnapshot_artefacts/Release/PickGuitarSnapshot docs/interface.png
// Antes de desenhar, toca a linha guia com o PLAY interno (Hux Brown, pica-pau ligado) para o VU, o osciloscópio,
// a grade e o braço terem sinal. Grava também as outras abas (interface-timbre, -eq, -efeitos, -guitarristas).
// Depois confere o estado: salva, restaura e compara grade, edições do mapa e parâmetros.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String out = argc > 1 ? juce::String (argv[1]) : juce::String ("docs/interface.png");
    const float scale = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;

    PickGuitarProcessor proc;
    proc.setParam ("src", 1.0f);      // linha guia
    proc.setParam ("play", 1.0f);     // PLAY sem a DAW
    proc.toggleEdit (2);              // um pica-pau posto à mão no mapa
    proc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> b (2, 512);
    juce::MidiBuffer m;
    const int total = (int) (48000.0 * 60.0 / 76.0 * 9.3);
    for (int pos = 0; pos < total; pos += 512) { b.clear(); m.clear(); proc.processBlock (b, m); }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* pe = dynamic_cast<PickGuitarEditor*> (ed.get());
    bool ok = pe != nullptr;
    const juce::File base = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    const char* suffix[5] = { "", "-timbre", "-eq", "-efeitos", "-guitarristas" };
    for (int page = 0; page < 5 && pe; ++page)
    {
        pe->setPage (page);
        for (int k = 0; k < 8; ++k) { b.clear(); m.clear(); proc.processBlock (b, m); pe->refreshForSnapshot(); }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
        juce::File f = base.getSiblingFile (base.getFileNameWithoutExtension() + suffix[page] + base.getFileExtension());
        f.getParentDirectory().createDirectory();
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat png;
        const bool w = png.writeImageToStream (img, os);
        ok = ok && w;
        std::printf ("%s %s (%dx%d)\n", w ? "PNG" : "FALHOU", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    }
    if (pe) pe->setPage (0);

    // estado: salva e restaura
    proc.applyMusician (4);           // Chinna Smith
    proc.setCell (3, 'r');
    proc.toggleEdit (20);
    proc.setParam ("echoOn", 1.0f);
    juce::MemoryBlock mb; proc.getStateInformation (mb);
    PickGuitarProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
    bool st = p2.getCurrentProgram() == 4 && p2.get ("echoOn") > 0.5f && p2.cell (3) == 'r' && p2.edit (20) == proc.edit (20) && p2.numEdits() == proc.numEdits();
    for (int i = 0; i < mpg::kSteps; ++i) st = st && p2.cell (i) == proc.cell (i);
    std::printf ("estado %s\n", st ? "ok" : "FALHOU");

    // padrão de fábrica de um plugin novo
    PickGuitarProcessor fresh;
    auto is = [&] (const char* id, float v) { return std::abs (fresh.get (id) - v) < 1.0e-4f; };
    const bool fab = is ("mute", 100) && is ("pos", 7) && is ("atk", 100) && is ("drive", 0) && is ("tone", 62) && is ("body", 96)
                     && (int) fresh.get ("pickup") == mpg::Ponte && is ("gate", 8) && is ("swing", 10) && is ("ppSwing", 10)
                     && is ("echoOn", 0) && is ("satOn", 0) && is ("dose", 5) && (int) fresh.get ("rep") == 0;
    std::printf ("padrão de fábrica %s\n", fab ? "ok" : "FALHOU");
    return ok && st && fab ? 0 : 1;
}
