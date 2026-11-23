// Desenha a interface do Mastor Half Dub MHD-050 em PNG, sem DAW:
//   cmake -B build-snap -DHD_SNAPSHOT=ON -DHD_FORMATS=VST3 && cmake --build build-snap --target HalfDubSnapshot
//   ./build-snap/HalfDubSnapshot_artefacts/Release/HalfDubSnapshot docs/interface.png
// Antes de desenhar, toca 4 compassos de um riddim sintético para o VU, o medidor e o visor terem sinal.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String out = argc > 1 ? juce::String (argv[1]) : juce::String ("docs/interface.png");
    const float scale = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;

    HalfDubProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> b (2, 512);
    juce::MidiBuffer m;
    juce::Random rnd (3);
    double ph = 0;
    const int spb = 24000;   // 120 BPM
    for (int pos = 0; pos < spb * 16; pos += 512)
    {
        for (int i = 0; i < 512; ++i)
        {
            const int s = pos + i, beat = s / spb, in = s % spb;
            const float t = (float) in / 48000.0f;
            float y = 0;
            if (beat % 4 == 2) y += 0.8f * std::exp (-t * 18.0f) * std::sin (6.2832f * 70.0f * t);
            if (beat % 2 == 1) y += 0.3f * std::exp (-t * 30.0f) * (rnd.nextFloat() * 2 - 1);
            ph += 6.2832 * (55.0 + 10.0 * ((s / 12000) % 3)) / 48000.0;
            y += 0.3f * (float) std::sin (ph);
            b.setSample (0, i, y); b.setSample (1, i, y);
        }
        proc.processBlock (b, m);
    }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* he = dynamic_cast<HalfDubEditor*> (ed.get());
    for (int k = 0; k < 20; ++k) juce::MessageManager::getInstance()->runDispatchLoopUntil (40);
    auto img = he->panel.createComponentSnapshot (he->panel.getLocalBounds(), true, scale);
    juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    f.getParentDirectory().createDirectory();
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat png;
    const bool ok = png.writeImageToStream (img, os);
    std::printf ("%s %s (%dx%d)\n", ok ? "PNG" : "FALHOU", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());

    // estado: salva e restaura
    juce::MemoryBlock mb; proc.getStateInformation (mb);
    HalfDubProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
    std::printf ("estado %s\n", p2.apvts.getRawParameterValue ("drop")->load() > 0.5f ? "ok" : "FALHOU");
    return ok ? 0 : 1;
}
