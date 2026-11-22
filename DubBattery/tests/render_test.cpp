// Teste do DUB BATTERY REGGAE sem DAW.
//   dbr_render battery.png 0     -> imagem da aba BATTERY (pad 3 aceso, envelope editado)
//   dbr_render drums.png 1       -> imagem da aba DUB DRUMS
//   dbr_render pads.wav          -> os 16 pads em sequência (confere que todos soam)
//   dbr_render groove.wav        -> 8 s do PLAY GROOVE: o groove do MRDM-12 tocando nos pads
//   dbr_render choke             -> HH fechado corta o HH aberto
//   dbr_render estado            -> salva e recarrega o estado (parâmetros, samples, aba, Dub Drums)
//   dbr_render presets           -> carrega todos os presets de fábrica e grava/relê um do usuário
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/BatteryProcessor.h"
#include "../Source/BatteryEditor.h"

namespace
{
void set (juce::AudioProcessorValueTreeState& s, const juce::String& id, float v)
{
    if (auto* p = s.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}
void pump (int times = 6) { for (int i = 0; i < times; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (40); }
int fail (const char* why) { std::printf ("FALHOU: %s\n", why); return 1; }

struct Render
{
    BatteryProcessor& proc;
    double sr = 48000.0;
    int block = 480;
    juce::AudioBuffer<float> buf { 2, 480 };
    explicit Render (BatteryProcessor& p) : proc (p)
    {
        proc.setPlayConfigDetails (0, 2, sr, block);
        proc.prepareToPlay (sr, block);
    }
    // renderiza 'blocks' blocos, com 'midi' no primeiro; devolve o pico
    float run (int blocks, juce::MidiBuffer midi, juce::AudioBuffer<float>* all = nullptr, int* writePos = nullptr)
    {
        float peak = 0.0f;
        for (int b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer m;
            if (b == 0) m = midi;
            proc.processBlock (buf, m);
            peak = std::max (peak, buf.getMagnitude (0, block));
            if (all != nullptr && writePos != nullptr && *writePos + block <= all->getNumSamples())
            {
                for (int c = 0; c < 2; ++c) all->copyFrom (c, *writePos, buf, c, 0, block);
                *writePos += block;
            }
        }
        return peak;
    }
};

bool writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 16, {}, 0));
    return w != nullptr && w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String arg1 (argc > 1 ? argv[1] : "battery.png");
    const juce::String arg2 (argc > 2 ? argv[2] : "0");
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (arg1));

    BatteryProcessor proc;
    set (proc.getDrums().apvts, "seed", 42.0f);
    pump();

    if (out.hasFileExtension ("png"))
    {
        proc.currentTab = arg2.getIntValue();
        proc.selectedPad = 2;
        set (proc.apvts, dbr::pid (2, dbr::P::start), 0.04f);
        set (proc.apvts, dbr::pid (2, dbr::P::end), 0.86f);
        set (proc.apvts, dbr::pid (2, dbr::P::attack), 8.0f);
        set (proc.apvts, dbr::pid (2, dbr::P::hold), 30.0f);
        set (proc.apvts, dbr::pid (2, dbr::P::decay), 220.0f);
        set (proc.apvts, dbr::pid (2, dbr::P::sustain), 0.45f);
        set (proc.apvts, dbr::pid (2, dbr::P::release), 300.0f);
        set (proc.apvts, dbr::pid (2, dbr::P::cutoff), 6500.0f);
        set (proc.apvts, dbr::pid (2, dbr::P::tune), -1.5f);
        Render r (proc);
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (10, 38, (juce::uint8) 120), 0);
        m.addEvent (juce::MidiMessage::noteOn (10, 42, (juce::uint8) 90), 0);
        r.run (6, m);
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        ed->setSize (BatteryRoot::baseW, BatteryRoot::baseH);
        pump (4);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat png;
        if (! png.writeImageToStream (img, os)) return fail ("png");
        std::printf ("png: %s %dx%d\n", out.getFileName().toRawUTF8(), img.getWidth(), img.getHeight());
        return 0;
    }

    if (arg1 == "choke")
    {
        Render r (proc);
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (1, 46, (juce::uint8) 120), 0);
        const float open = r.run (10, m);
        juce::MidiBuffer c;
        c.addEvent (juce::MidiMessage::noteOn (1, 42, (juce::uint8) 1), 0);
        set (proc.apvts, dbr::pid (6, dbr::P::level), -36.0f);     // HH fechado mudo: só sobra o aberto (que deve parar)
        r.run (3, c);
        const float after = r.run (10, {});
        std::printf ("choke: aberto %.3f -> depois do fechado %.5f\n", open, after);
        return open > 0.05f && after < 0.001f ? 0 : fail ("choke");
    }

    if (arg1 == "estado")
    {
        proc.selectedPad = 5;
        proc.currentTab = 1;
        set (proc.apvts, dbr::pid (5, dbr::P::tune), -7.0f);
        set (proc.apvts, dbr::pid (10, dbr::P::choke), 3.0f);
        set (proc.getDrums().apvts, "style", 2.0f);
        proc.setFactorySample (3, 13);
        pump();
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        BatteryProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        pump();
        const bool ok = other.selectedPad == 5 && other.currentTab == 1
                     && std::abs (other.padValue (5, dbr::P::tune) + 7.0f) < 0.01f
                     && juce::roundToInt (other.padValue (10, dbr::P::choke)) == 3
                     && other.factoryIndexOf (3) == 13
                     && (int) other.getDrums().apvts.getRawParameterValue ("style")->load() == 2;
        std::printf ("estado: %s (%d bytes)\n", ok ? "ok" : "FALHOU", (int) mb.getSize());
        return ok ? 0 : 1;
    }

    // O que as DAWs fazem de diferente: taxas de 44,1 a 192 kHz, blocos de tamanho variável (FL Studio, Reaper),
    // blocos maiores que o anunciado (Logic), saída mono, salvar/abrir o projeto tocando, DAW tocando com MIDI saindo
    if (arg1 == "daws")
    {
        struct FakeDaw : juce::AudioPlayHead
        {
            double ppq = 0.0, bpm = 92.0; bool playing = true;
            juce::Optional<PositionInfo> getPosition() const override
            {
                PositionInfo p; p.setIsPlaying (playing); p.setBpm (bpm); p.setPpqPosition (ppq);
                p.setTimeSignature (TimeSignature { 4, 4 }); return p;
            }
        } daw;
        juce::Random rng (7);
        int failures = 0;
        for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 })
            for (int chans : { 2, 1 })
            {
                BatteryProcessor p;
                set (p.getDrums().apvts, "seed", 42.0f);
                p.setPlayHead (&daw);
                daw.ppq = 0.0; daw.playing = true;
                const int announced = 512;
                p.setPlayConfigDetails (0, chans, rate, announced);
                p.prepareToPlay (rate, announced);
                juce::AudioBuffer<float> buf (chans, 4096);
                int notesOut = 0; float peak = 0.0f; bool bad = false;
                const int total = (int) (rate * 6.0);
                for (int pos = 0, k = 0; pos < total; ++k)
                {
                    const int n = (k % 7 == 3) ? 4096 : (k % 5 == 1 ? 1 : 1 + rng.nextInt (1500));   // 1 amostra, irregulares e maiores
                    juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), chans, 0, n);
                    juce::MidiBuffer m;
                    if (k % 40 == 0) m.addEvent (juce::MidiMessage::noteOn (1, 36 + rng.nextInt (16), (juce::uint8) 100), 0);
                    p.processBlock (view, m);
                    for (const auto meta : m) if (meta.getMessage().isNoteOn()) ++notesOut;
                    for (int c = 0; c < chans; ++c)
                        for (int i = 0; i < n; ++i)
                        {
                            const float x = view.getSample (c, i);
                            if (! std::isfinite (x) || std::abs (x) > 1.0f) bad = true;
                            peak = std::max (peak, std::abs (x));
                        }
                    daw.ppq += n / rate * daw.bpm / 60.0;
                    pos += n;
                    if (k == 300)                                  // salvar e reabrir o projeto com a DAW tocando
                    {
                        juce::MemoryBlock mb; p.getStateInformation (mb);
                        p.setStateInformation (mb.getData(), (int) mb.getSize());
                    }
                }
                daw.playing = false;
                juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), chans, 0, 256);
                juce::MidiBuffer m; p.processBlock (view, m);
                p.releaseResources();
                const bool ok = ! bad && peak > 0.05f && notesOut > 20;
                std::printf ("%6.0f Hz  %s  pico %.3f  notas MIDI saindo %4d  %s\n", rate, chans == 2 ? "estereo" : "mono   ",
                             peak, notesOut, ok ? "ok" : "FALHOU");
                if (! ok) ++failures;
            }
        return failures == 0 ? 0 : fail ("daws");
    }

    if (arg1 == "presets")
    {
        const auto names = proc.getPresetNames();
        for (int i = 0; i < proc.getNumFactoryPresets(); ++i)
        {
            proc.loadPreset (i);
            std::printf ("preset %02d %-14s  pad1 tune %+.1f  pad3 decay %.0f\n", i + 1, proc.getPresetName().toRawUTF8(),
                         proc.padValue (0, dbr::P::tune), proc.padValue (2, dbr::P::decay));
        }
        set (proc.apvts, dbr::pid (0, dbr::P::tune), 5.5f);
        proc.setFactorySample (0, 9);
        if (! proc.savePreset ("teste ci")) return fail ("gravar preset");
        const int idx = proc.getPresetIndex();
        proc.loadPreset (0);
        proc.loadPreset (idx);
        const bool ok = std::abs (proc.padValue (0, dbr::P::tune) - 5.5f) < 0.01f && proc.factoryIndexOf (0) == 9;
        BatteryProcessor::userPresetFolder().getChildFile ("TESTE CI.dbrpreset").deleteFile();
        std::printf ("preset do usuario: %s\n", ok ? "ok" : "FALHOU");
        return ok ? 0 : 1;
    }

    if (out.hasFileExtension ("wav"))
    {
        Render r (proc);
        const bool groove = arg1.startsWith ("groove");
        const int total = (int) (r.sr * (groove ? 8.0 : 9.0));
        juce::AudioBuffer<float> all (2, total);
        all.clear();
        int pos = 0;
        if (groove)
        {
            set (proc.getDrums().apvts, "bpm", 75.0f);
            proc.setGroovePlaying (true);
            pump();
            const float peak = r.run (total / r.block, {}, &all, &pos);
            std::printf ("groove: pico %.3f\n", peak);
            if (peak < 0.05f) return fail ("groove sem som");
        }
        else
        {
            for (int i = 0; i < dbr::numPads; ++i)
            {
                juce::MidiBuffer m;
                m.addEvent (juce::MidiMessage::noteOn (1, dbr::firstNote + i, (juce::uint8) 110), 0);
                const float peak = r.run (54, m, &all, &pos);
                std::printf ("pad %02d %-10s pico %.3f\n", i + 1, dbr::pads[i].name, peak);
                if (peak < 0.02f) return fail ("pad mudo");
            }
        }
        if (! writeWav (out, all, r.sr)) return fail ("wav");
        return 0;
    }
    return fail ("argumento");
}
