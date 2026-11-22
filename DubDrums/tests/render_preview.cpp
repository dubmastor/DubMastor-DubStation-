// Salva uma imagem da interface sem abrir a DAW:
// ./render_preview interface.png [estilo] [compassos] [semente] [intro|notas|editado] [baterista 0-3] [kit 0-2]
// ./render_preview troca.txt   -> toca 60 s trocando variação e baterista no meio do tempo e procura notas dobradas
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <cstdlib>
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
    set ("drummer", argc > 6 ? (float) juce::String (argv[6]).getIntValue() : 0.0f);
    set ("kit", argc > 7 ? (float) juce::String (argv[7]).getIntValue() : 0.0f);

    // "algo.txt": teste da troca de padrão tocando (o "embolado" da 1.3)
    if (out.hasFileExtension ("txt"))
    {
        struct Head : juce::AudioPlayHead
        {
            double ppq = 0.0, bpm = 100.0;
            juce::Optional<PositionInfo> getPosition() const override
            {
                PositionInfo i; i.setIsPlaying (true); i.setPpqPosition (ppq); i.setBpm (bpm);
                return i;
            }
        } head;
        set ("intro", 0.0f); set ("fill", 1.0f); set ("rndHat", 60.0f); set ("rndSnare", 60.0f); set ("rndKick", 60.0f);
        proc.setPlayHead (&head);
        const double sr = 44100.0; const int block = 256;
        proc.setPlayConfigDetails (0, 2, sr, block);
        proc.prepareToPlay (sr, block);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        struct On { double beat; int note, vel; };
        std::vector<On> ons;
        juce::Random rnd (7);
        int changes = 0, hanging = 0;
        std::array<int, 128> open {};
        const double beatsPerBlock = block / sr * head.bpm / 60.0;
        for (int b = 0; b < (int) (60.0 * sr / block); ++b)
        {
            proc.processBlock (buf, midi);
            for (const auto m : midi)
            {
                const auto msg = m.getMessage();
                const double t = head.ppq + m.samplePosition / (double) block * beatsPerBlock;
                if (msg.isNoteOn()) { ons.push_back ({ t, msg.getNoteNumber(), msg.getVelocity() }); ++open[(size_t) msg.getNoteNumber()]; }
                else if (msg.isNoteOff()) open[(size_t) msg.getNoteNumber()] = 0;
            }
            head.ppq += beatsPerBlock;
            if (b % 37 == 36)   // ~0,2 s: nova variação, outro baterista ou outro estilo, em qualquer ponto do tempo
            {
                const int what = rnd.nextInt (3);
                if (what == 0) proc.newVariation();
                else if (what == 1) set ("drummer", (float) rnd.nextInt (4));
                else set ("swing", changes % 2 == 0 ? 4.0f : 0.0f);   // reto <-> shuffle: o contratempo pula de 0,5 para 0,667
                ++changes;
                juce::MessageManager::getInstance()->runDispatchLoopUntil (60);   // o timer gera o padrão novo
            }
        }
        for (int n : open) hanging += n > 1 ? 1 : 0;
        // nota dobrada: a mesma peça, duas vezes forte, a menos de 0,1 tempo (flam e arrasto têm a 1a nota fraca)
        int doubles = 0;
        for (size_t i = 0; i < ons.size(); ++i)
            for (size_t j = i + 1; j < ons.size() && ons[j].beat - ons[i].beat < 0.10; ++j)
                if (ons[j].note == ons[i].note && ons[i].vel > 64 && ons[j].vel > 64)
                {
                    ++doubles;
                    std::printf ("  dobrada: nota %d em %.3f e %.3f\n", ons[i].note, ons[i].beat, ons[j].beat);
                }
        juce::String rep;
        rep << "versoes do padrao: " << proc.getPatternVersion() << "  trocas: " << changes << "  notas: " << (int) ons.size() << "  dobradas: " << doubles << "\n";
        out.replaceWithText (rep);
        std::printf ("%s", rep.toRawUTF8());
        proc.setPlayHead (nullptr);
        return doubles == 0 ? 0 : 1;
    }

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

    // "editado" no 5º argumento: tira o hi-hat do 1 e põe um tom, como um clique na grade PADRÃO
    if (argc > 5 && juce::String (argv[5]).contains ("editado"))
    {
        proc.toggleNote (dubdrums::gm::TomHigh, 3.76, true);
        proc.toggleNote (dubdrums::gm::Kick, 1.51, true);
        for (int i = 0; i < 4; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (40);
        std::printf ("edicoes: %s\n", proc.hasEdits() ? "sim" : "nao");
    }

    // MRDM_SPEAKER=0/1/2: escolhe o desenho do alto-falante (e mostra o cone tocando)
    if (const char* sp = std::getenv ("MRDM_SPEAKER"))
    {
        std::function<void (juce::Component&)> setSpeaker = [&] (juce::Component& c)
        {
            if (auto* v = dynamic_cast<VintageSpeaker*> (&c)) { v->design = juce::String (sp).getIntValue(); v->level = 0.55f; v->repaint(); }
            for (auto* child : c.getChildren()) setSpeaker (*child);
        };
        setSpeaker (*ed);
    }

    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("salvo: %s (%dx%d)\n", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    return 0;
}
