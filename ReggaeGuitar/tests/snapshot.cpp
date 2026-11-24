// Desenha a interface do Mastor Reggae Guitar MRG-68 em PNG, sem DAW:
//   cmake -B build-snap -DRG_SNAPSHOT=ON -DRG_FORMATS=VST3 && cmake --build build-snap --target ReggaeGuitarSnapshot
//   ./build-snap/ReggaeGuitarSnapshot_artefacts/Release/ReggaeGuitarSnapshot docs/interface.png
// Carrega o Bingy Bunny (bang-bang) e toca com o PLAY interno para a grade, as luzes e o osciloscópio terem sinal.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String out = argc > 1 ? juce::String (argv[1]) : juce::String ("docs/interface.png");
    const float scale = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;

    ReggaeGuitarProcessor proc;
    proc.applyPlayer (7);   // Bingy Bunny
    proc.prepareToPlay (48000.0, 512);
    proc.setParam ("play", 1.0f);
    proc.setParam ("band", 1.0f);
    juce::AudioBuffer<float> b (2, 512);
    juce::MidiBuffer m;
    const int total = (int) (48000.0 * 60.0 / 76.0 * 14.6);   // para no 4o compasso (variação), logo depois de um skank
    for (int pos = 0; pos < total; pos += 512) { b.clear(); m.clear(); proc.processBlock (b, m); }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
    const juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    f.getParentDirectory().createDirectory();
    f.deleteFile();
    bool ok = false;
    {
        juce::FileOutputStream os (f);
        juce::PNGImageFormat png;
        ok = png.writeImageToStream (img, os);
    }
    std::printf ("%s %s (%dx%d)\n", ok ? "PNG" : "FALHOU", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());

    // estado: salva e restaura
    juce::MemoryBlock mb; proc.getStateInformation (mb);
    ReggaeGuitarProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
    const bool st = p2.getCurrentProgram() == 7 && (int) p2.get ("stroke") == mrg::StBangBang && p2.get ("band") > 0.5f;
    std::printf ("estado %s\n", st ? "ok" : "FALHOU");
    return ok && st ? 0 : 1;
}
