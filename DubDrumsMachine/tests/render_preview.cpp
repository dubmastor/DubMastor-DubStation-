// Salva uma imagem da interface sem abrir a DAW:
// ./render_preview interface.png [estilo] [compassos] [semente] [intro|notas] [baterista 0-2] [kit 0-1]
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "interface.png"));

    DubDrumsProcessor proc;
    auto set = [&] (const char* id, float v)
    {
        if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    };
    set ("style", argc > 2 ? (float) juce::String (argv[2]).getIntValue() : 0.0f);
    proc.applyStyleDefaults (argc > 2 ? juce::String (argv[2]).getIntValue() : 0);   // como o clique no estilo
    set ("bars",  argc > 3 ? (float) juce::String (argv[3]).getIntValue() - 1 : 3.0f);
    set ("seed",  argc > 4 ? (float) juce::String (argv[4]).getIntValue() : 42.0f);
    set ("swing", 1.0f);
    set ("rndHat", 40.0f);
    set ("rndSnare", 35.0f);
    set ("intro", argc > 5 && juce::String (argv[5]).contains ("intro") ? 1.0f : 0.0f);
    set ("drummer", argc > 6 ? (float) juce::String (argv[6]).getIntValue() : 2.0f);
    set ("kit", argc > 7 ? (float) juce::String (argv[7]).getIntValue() : 0.0f);

    // "algo.mid": salva o MIDI que o EXPORTAR/arrastar gera
    if (out.hasFileExtension ("mid"))
    {
        for (int i = 0; i < 4; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (40);
        const bool ok = proc.writeMidiFile (out);
        std::printf ("midi: %s %s\n", out.getFullPathName().toRawUTF8(), ok ? "ok" : "FALHOU");
        return ok ? 0 : 1;
    }

    // "algo.wav": grava 12 s do PREVIEW (bateria interna) em vez da imagem
    if (out.hasFileExtension ("wav"))
    {
        set ("preview", 1.0f);
        set ("bpm", 75.0f);
        for (int i = 0; i < 4; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (40);
        const double sr = 48000.0; const int block = 512, total = (int) (sr * 12.0);
        proc.setPlayConfigDetails (0, 2, sr, block);
        proc.prepareToPlay (sr, block);
        juce::AudioBuffer<float> all (2, total), buf (2, block);
        juce::MidiBuffer midi;
        float peak = 0.0f; double sum = 0.0;
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = std::min (block, total - pos);
            buf.setSize (2, n, false, false, true);
            proc.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch) all.copyFrom (ch, pos, buf, ch, 0, n);
        }
        for (int i = 0; i < total; ++i) { const float v = all.getSample (0, i); peak = std::max (peak, std::abs (v)); sum += v * v; }
        out.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (out), sr, 2, 24, {}, 0));
        w->writeFromAudioSampleBuffer (all, 0, total);
        std::printf ("audio: %s pico %.3f  rms %.4f\n", out.getFullPathName().toRawUTF8(), peak, std::sqrt (sum / total));
        return 0;
    }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    // deixa o timer gerar o padrão e a tela atualizar
    for (int i = 0; i < 10; ++i)
        juce::MessageManager::getInstance()->runDispatchLoopUntil (40);

    // "notas" no 5º argumento abre a opção avançada do mapa de notas
    if (argc > 5 && juce::String (argv[5]).contains ("notas"))
    {
        std::function<void (juce::Component&)> openNoteMap = [&] (juce::Component& c)
        {
            if (auto* nm = dynamic_cast<NoteMapPanel*> (&c)) { nm->setVisible (true); nm->toFront (false); }
            for (auto* child : c.getChildren()) openNoteMap (*child);
        };
        openNoteMap (*ed);
        ed->repaint();
    }

    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("salvo: %s (%dx%d)\n", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    return 0;
}
