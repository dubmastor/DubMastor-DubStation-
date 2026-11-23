// Desenha a interface do Mastor Tune MT-0420 em PNG, sem DAW (confere o layout e gera a imagem do site).
//   cmake -B build -DMT_RENDER_TEST=ON -DMT_FORMATS=VST3 && cmake --build build --target mt_render
//   ./build/mt_render_artefacts/mt_render  <pasta de saída>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

static void pump (int times = 6) { for (int i = 0; i < times; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (40); }

static void save (juce::Component& c, const juce::File& f, float scale)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, scale);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat png;
    png.writeImageToStream (img, os);
    std::printf ("  %s (%d x %d)\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
}

static void setP (MastorTuneProcessor& p, const char* id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]) : juce::File::getCurrentWorkingDirectory());
    out.createDirectory();

    MastorTuneProcessor proc;
    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    // processa um pouco de áudio (voz sintética desafinada) para conferir que nada quebra
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    double ph = 0;
    float peak = 0;
    for (int b = 0; b < 200; ++b)
    {
        for (int i = 0; i < 512; ++i)
        {
            ph += 223.0 / 48000.0; if (ph > 1) ph -= 1;
            float s = 0; for (int k = 1; k < 20; ++k) s += (float) (std::sin (2 * M_PI * k * ph) / k);
            buf.setSample (0, i, 0.1f * s); buf.setSample (1, i, 0.1f * s);
        }
        proc.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) peak = juce::jmax (peak, buf.getMagnitude (c, 0, 512));
    }
    std::printf ("processamento ok, pico %.3f, latencia %d amostras\n", peak, proc.getLatencySamples());
    if (! std::isfinite (peak) || peak > 4.0f) { std::printf ("ERRO: saída inválida\n"); return 1; }

    // notas de exemplo no editor (com dúvidas) para a imagem
    std::vector<HopFrame> fr;
    const int mel[] = { 57, 60, 62, 60, 64, 62, 60, 57, 55, 57, 60, 59, 57 };
    const bool dbt[] = { false, false, true, false, false, false, true, false, false, false, false, true, false };
    double t = 0.0;
    for (int n = 0; n < 13; ++n)
    {
        const double len = (n % 3 == 2) ? 1.5 : 1.0;
        for (double q = t; q < t + len - 0.12; q += 0.02)
            fr.push_back ({ q, (float) mel[n] + (dbt[n] ? 0.48f : 0.1f * (float) std::sin (q * 9.0)), mel[n], dbt[n] });
        t += len;
    }
    proc.getNotes().addFrames (fr.data(), (int) fr.size());
    proc.getNotes().closeCurrent();
    auto notes = proc.getNotes().getNotes();
    if (notes.size() > 4) proc.getNotes().setEdit (notes[4].id, 65);

    auto* ed = dynamic_cast<MastorTuneEditor*> (proc.createEditor());
    ed->setSize (mtui::Panel::kW, mtui::Panel::kH);
    pump();
    auto& panel = ed->getPanel();
    save (panel, out.getChildFile ("mt-afinacao.png"), 1.0f);

    // processamento: tudo ligado para a foto
    for (auto* id : { "eqOn", "satOn", "bkOn", "echoOn", "maxOn" }) setP (proc, id, 1.0f);
    setP (proc, "eqHp", 120.0f); setP (proc, "eqLp", 6000.0f); setP (proc, "eq2g", 3.0f); setP (proc, "eq3f", 3500.0f); setP (proc, "eq3g", -4.0f);
    panel.showTab (1);
    pump();
    save (panel, out.getChildFile ("mt-processamento.png"), 1.0f);
    for (auto* id : { "eqOn", "satOn", "bkOn", "echoOn", "maxOn" }) setP (proc, id, 0.0f);
    pump();
    save (panel, out.getChildFile ("mt-processamento-desligado.png"), 1.0f);
    panel.showTab (0);
    pump();
    save (panel, out.getChildFile ("mt-card.png"), 0.5f);

    // estado salvo e aberto de novo (notas corrigidas vão junto)
    juce::MemoryBlock mb;
    proc.getStateInformation (mb);
    MastorTuneProcessor p2;
    p2.setStateInformation (mb.getData(), (int) mb.getSize());
    std::printf ("estado: %d notas salvas, %d abertas\n", (int) proc.getNotes().getNotes().size(), (int) p2.getNotes().getNotes().size());
    const bool ok = proc.getNotes().getNotes().size() == p2.getNotes().getNotes().size() && ! p2.getNotes().getNotes().empty();
    delete ed;
    std::printf (ok ? "OK\n" : "ERRO no estado\n");
    return ok ? 0 : 1;
}
