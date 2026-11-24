// Desenha a interface do Mastor Reggae Organ MRO-68 em PNG, sem DAW:
//   cmake -B build-snap -DRO_SNAPSHOT=ON -DRO_FORMATS=VST3 && cmake --build build-snap --target ReggaeOrganSnapshot
//   ./build-snap/ReggaeOrganSnapshot_artefacts/Release/ReggaeOrganSnapshot docs/interface.png
// Antes de desenhar, carrega um riddim do banco e toca com o PLAY interno para o VU, a grade e o teclado terem sinal.
// Grava também docs/interface-midi.png e docs/interface-orgao.png (as outras abas).
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "RiddimBank.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String out = argc > 1 ? juce::String (argv[1]) : juce::String ("docs/interface.png");
    const float scale = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;

    ReggaeOrganProcessor proc;
    // "Hot Milk" (Jackie Mittoo, 1968) do banco único, tocado pelo Winston Wright
    for (int i = 0; i < riddimbank::numEntries; ++i)
        if (juce::String (riddimbank::entries[i].name) == "Hot Milk") proc.applyBankEntry (i);
    proc.applyMusician (mro::MusWright);
    proc.setParam ("echoOn", 1.0f);
    proc.prepareToPlay (48000.0, 512);
    proc.setParam ("play", 1.0f);
    juce::AudioBuffer<float> b (2, 512);
    juce::MidiBuffer m;
    // para no meio de um compasso de fill, com notas acesas
    const int total = (int) (48000.0 * 60.0 / 76.0 * 15.4);   // para no meio de um compasso de fill
    for (int pos = 0; pos < total; pos += 512) { b.clear(); m.clear(); proc.processBlock (b, m); }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* re = dynamic_cast<ReggaeOrganEditor*> (ed.get());
    for (int k = 0; k < 12; ++k) { b.clear(); m.clear(); proc.processBlock (b, m); re->refreshForSnapshot(); }
    bool ok = true;
    const juce::File base = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    const char* suffix[3] = { "", "-midi", "-orgao" };
    for (int page = 0; page < 3; ++page)
    {
        re->setPage (page);
        re->refreshForSnapshot();
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
    {   // o gabinete muda com o INSTRUMENTO (aba GROOVE e aba ÓRGÃO de cada um)
        const char* nomes[5] = { "", "-rhodes", "-suitcase", "-wurlitzer", "-dxep" };
        for (int ins = 1; ins < 5; ++ins)
            for (int page : { 0, 2 })
            {
                proc.setParam ("instr", (float) ins);
                re->setPage (page);
                re->refreshForSnapshot();
                auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
                juce::File f = base.getSiblingFile (base.getFileNameWithoutExtension() + suffix[page] + nomes[ins] + base.getFileExtension());
                f.deleteFile();
                juce::FileOutputStream os (f);
                juce::PNGImageFormat png;
                const bool w = png.writeImageToStream (img, os);
                ok = ok && w;
                std::printf ("%s %s\n", w ? "PNG" : "FALHOU", f.getFullPathName().toRawUTF8());
            }
        proc.setParam ("instr", 0.0f);
        re->refreshForSnapshot();
    }
    {   // aba EQ (1.3) com o TIMBRE MIDNITE carregado
        proc.applyMidnite();
        for (int k = 0; k < 4; ++k) { b.clear(); m.clear(); proc.processBlock (b, m); }
        re->setPage (3);
        re->refreshForSnapshot();
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
        juce::File f = base.getSiblingFile (base.getFileNameWithoutExtension() + "-eq" + base.getFileExtension());
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat png;
        const bool w = png.writeImageToStream (img, os);
        ok = ok && w;
        std::printf ("%s %s\n", w ? "PNG" : "FALHOU", f.getFullPathName().toRawUTF8());
        juce::MemoryBlock mm; proc.getStateInformation (mm);
        ReggaeOrganProcessor p3; p3.setStateInformation (mm.getData(), (int) mm.getSize());
        const bool eqOk = p3.isMidnite() && std::abs (p3.get ("eqHpf") - mro::midniteTimbre().eq.hpf) < 0.5f
                          && std::abs (p3.get ("eqMidG") - mro::midniteTimbre().eq.midG) < 0.05f && p3.get ("reverb") == 0.0f;
        std::printf ("EQ e timbre Midnite no estado %s\n", eqOk ? "ok" : "FALHOU");
        ok = ok && eqOk;
        proc.applyMusician (mro::MusWright);   // volta ao estado do resto do teste
        proc.setParam ("echoOn", 1.0f);
        proc.applyEqPreset (0);
        ok = ok && ! proc.isMidnite() && proc.getEqPreset() == 0;
    }
    re->setPage (0);

    // estado: salva e restaura
    juce::MemoryBlock mb; proc.getStateInformation (mb);
    ReggaeOrganProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
    const bool st = p2.get ("echoOn") > 0.5f && (int) p2.get ("fillStyle") == mro::FillWright && p2.getCurrentProgram() == mro::MusWright
                    && p2.getGrid() == proc.getGrid();
    std::printf ("estado %s\n", st ? "ok" : "FALHOU");

    // MIDI editado: salva no estado e volta
    {
        auto loop = proc.getCurrentLoop();
        loop.events.erase (loop.events.begin());
        proc.setEdits (loop.events, loop.bars);
        juce::MemoryBlock mb2; proc.getStateInformation (mb2);
        ReggaeOrganProcessor p3; p3.setStateInformation (mb2.getData(), (int) mb2.getSize());
        const bool edOk = p3.hasEdits() && p3.getCurrentLoop().events.size() == loop.events.size();
        std::printf ("midi editado no estado %s\n", edOk ? "ok" : "FALHOU");
        ok = ok && edOk;
        proc.clearEdits();
    }

    // MIDI exportado
    auto mf = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("mro68_test.mid");
    const bool midiOk = proc.writeMidiFile (mf) && mf.getSize() > 200;
    std::printf ("midi %s (%d bytes)\n", midiOk ? "ok" : "FALHOU", (int) mf.getSize());
    return ok && st && midiOk ? 0 : 1;
}
