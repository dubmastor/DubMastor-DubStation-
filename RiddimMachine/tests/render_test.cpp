// Teste do MRM-0420 sem DAW.
//   mrm_render harmonia.png 0          -> imagem da aba HARMONIA (Dub Chords)
//   mrm_render bateria.png 1           -> imagem da aba BATERIA (Dub Drums)
//   mrm_render riddim.mid [intro]      -> o MIDI do ARRASTE O RIDDIM (confere faixas e alinhamento)
//   mrm_render play.wav [intro]        -> 12 s do PLAY RIDDIM
//   mrm_render daw.wav intro           -> DAW tocando do zero: confere que os acordes entram depois da intro
//   mrm_render estado                  -> salva e recarrega o estado (aba, BPM, acordes, bateria)
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/MrmProcessor.h"
#include "../Source/MrmEditor.h"

namespace
{
void set (juce::AudioProcessorValueTreeState& s, const char* id, float v)
{
    if (auto* p = s.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}
void pump (int times = 6) { for (int i = 0; i < times; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (40); }

struct FakeDaw : juce::AudioPlayHead
{
    double ppq = 0.0, bpm = 75.0;
    bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setIsPlaying (playing);
        p.setBpm (bpm);
        p.setPpqPosition (ppq);
        p.setTimeSignature (TimeSignature { 4, 4 });
        return p;
    }
};

int fail (const char* why) { std::printf ("FALHOU: %s\n", why); return 1; }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String arg1 (argc > 1 ? argv[1] : "harmonia.png");
    const juce::String arg2 (argc > 2 ? argv[2] : "");
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (arg1));

    MrmProcessor proc;
    set (proc.getDrums().apvts, "seed", 42.0f);
    set (proc.getDrums().apvts, "intro", arg2.contains ("intro") ? 1.0f : 0.0f);
    pump();

    if (arg1 == "estado")
    {
        set (proc.apvts, "bpm", 92.0f);
        set (proc.getChords().apvts, "root1", 5.0f);
        set (proc.getDrums().apvts, "style", 2.0f);
        proc.setCurrentTab (1);
        pump();
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        MrmProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        pump();
        const bool ok = other.getCurrentTab() == 1
                     && (int) other.apvts.getRawParameterValue ("bpm")->load() == 92
                     && (int) other.getChords().apvts.getRawParameterValue ("bpm")->load() == 92
                     && (int) other.getDrums().apvts.getRawParameterValue ("bpm")->load() == 92
                     && (int) other.getChords().apvts.getRawParameterValue ("root1")->load() == 5
                     && (int) other.getDrums().apvts.getRawParameterValue ("style")->load() == 2;
        std::printf ("estado: %s (%d bytes)\n", ok ? "ok" : "FALHOU", (int) mb.getSize());
        return ok ? 0 : 1;
    }

    if (out.hasFileExtension ("mid"))
    {
        if (! proc.writeRiddimFile (out)) return fail ("escrever MIDI");
        juce::FileInputStream in (out);
        juce::MidiFile mf;
        if (! mf.readFrom (in)) return fail ("ler MIDI");
        const double intro = proc.introBeats();
        std::printf ("midi: %s  (%s)\n  faixas: %d\n", out.getFileName().toRawUTF8(), proc.suggestedRiddimName().toRawUTF8(), mf.getNumTracks());
        double firstChord = 1e9, lastTick = 0;
        for (int t = 0; t < mf.getNumTracks(); ++t)
        {
            auto* seq = mf.getTrack (t);
            juce::String name; int notes = 0; double first = -1;
            for (auto* ev : *seq)
            {
                const auto& m = ev->message;
                if (m.isTrackNameEvent()) name = m.getTextFromTextMetaEvent();
                if (m.isNoteOn()) { ++notes; if (first < 0) first = m.getTimeStamp(); }
                lastTick = std::max (lastTick, m.getTimeStamp());
            }
            std::printf ("  %-32s notas %4d  primeira em %.2f tempos\n", name.toRawUTF8(), notes, first / 960.0);
            if (name.contains ("Skank") && first >= 0) firstChord = first / 960.0;
        }
        const double expectEnd = intro + proc.riddimBars() * 4.0;
        std::printf ("  fim: %.2f tempos (esperado %.2f)\n", lastTick / 960.0, expectEnd);
        if (std::abs (lastTick / 960.0 - expectEnd) > 1e-6) return fail ("tamanho do riddim");
        if (firstChord < intro - 1e-6) return fail ("acordes antes do fim da intro");
        return 0;
    }

    if (out.hasFileExtension ("wav"))
    {
        const bool daw = arg1.startsWith ("daw");
        FakeDaw fake;
        if (daw) proc.setPlayHead (&fake);
        else     set (proc.apvts, "play", 1.0f);
        set (proc.apvts, "bpm", 75.0f);
        pump();

        const double sr = 48000.0; const int block = 480, total = (int) (sr * 12.0);
        proc.setPlayConfigDetails (0, 2, sr, block);
        proc.prepareToPlay (sr, block);
        juce::AudioBuffer<float> all (2, total), buf (2, block);
        juce::MidiBuffer midi;
        int firstChordSample = -1, firstDrumSample = -1, chordsDuringIntro = 0;
        const double introSec = proc.introBeats() * 60.0 / 75.0;
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = std::min (block, total - pos);
            buf.setSize (2, n, false, false, true);
            midi.clear();
            proc.processBlock (buf, midi);
            if (daw) fake.ppq += (double) n / sr * fake.bpm / 60.0;
            for (const auto meta : midi)
            {
                const auto m = meta.getMessage();
                if (! m.isNoteOn()) continue;
                const int s = pos + meta.samplePosition;
                if (m.getChannel() == 10) { if (firstDrumSample < 0) firstDrumSample = s; }
                else
                {
                    if (firstChordSample < 0) firstChordSample = s;
                    if ((double) s / sr < introSec - 0.001) ++chordsDuringIntro;
                }
            }
            for (int ch = 0; ch < 2; ++ch) all.copyFrom (ch, pos, buf, ch, 0, n);
        }
        float peak = 0.0f; double sum = 0.0;
        for (int i = 0; i < total; ++i) { const float v = all.getSample (0, i); peak = std::max (peak, std::abs (v)); sum += v * v; }
        out.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (out), sr, 2, 24, {}, 0));
        w->writeFromAudioSampleBuffer (all, 0, total);
        w.reset();
        std::printf ("audio: %s  pico %.3f  rms %.4f\n", out.getFileName().toRawUTF8(), peak, std::sqrt (sum / total));
        if (daw)
        {
            std::printf ("  intro %.3f s | 1a nota bateria %.3f s | 1a nota acordes %.3f s | acordes na intro: %d\n",
                         introSec, firstDrumSample / sr, firstChordSample / sr, chordsDuringIntro);
            if (chordsDuringIntro > 0 || firstChordSample < 0) return fail ("acordes na intro");
        }
        if (peak > 1.0f || sum <= 0.0) return fail ("nivel do audio");
        return 0;
    }

    // imagem de uma aba
    proc.setCurrentTab (arg2.getIntValue());
    if (arg2.contains ("play")) set (proc.apvts, "play", 1.0f);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (MrmRoot::baseW, juce::roundToInt ((double) ed->getHeight() * MrmRoot::baseW / ed->getWidth()));
    pump (12);
    if (arg2.contains ("notas"))
    {
        std::function<void (juce::Component&)> open = [&] (juce::Component& c)
        {
            if (auto* nm = dynamic_cast<NoteMapPanel*> (&c)) { nm->setVisible (true); nm->toFront (false); }
            for (auto* child : c.getChildren()) open (*child);
        };
        open (*ed);
        pump (2);
    }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("salvo: %s (%dx%d)\n", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    return 0;
}
