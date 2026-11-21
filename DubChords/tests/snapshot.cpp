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
