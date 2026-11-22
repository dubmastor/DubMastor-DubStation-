// Teste do MRM-0420 sem DAW.
//   mrm_render riddim.png 0            -> imagem da aba (0 RIDDIM, 1 HARMONIA, 2 BATERIA, 3 BAIXO, 4 BIG KNOB)
//   mrm_render riddim.mid [intro]      -> o MIDI do ARRASTE O RIDDIM (faixas, tamanho, harmonia e baixo depois da intro)
//   mrm_render play.wav [intro]        -> 12 s do PLAY RIDDIM com as três máquinas e o Big Knob
//   mrm_render daw.wav intro           -> DAW tocando do zero: harmonia e baixo entram depois da intro
//   mrm_render links                   -> harmonia ligada (MRCM-1240 <-> MRBM-69), BPM único, semente do JACKPOT
//   mrm_render edicao                  -> MIDI editável da harmonia, da bateria e do baixo vai para o riddim
//   mrm_render maquinas                -> ARRASTE de cada máquina e máquina desligada fora do riddim
//   mrm_render samples                 -> o kit da bateria é o dos samples DM (os WAVs de DubDrums/Samples)
//   mrm_render estado                  -> salva e recarrega o estado (aba, BPM, harmonia, edições, Big Knob)
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/MrmProcessor.h"
#include "../Source/MrmEditor.h"
#include "../../DubDrums/Source/DMSamples.h"

namespace
{
void set (juce::AudioProcessorValueTreeState& s, const juce::String& id, float v)
{
    if (auto* p = s.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}
float get (juce::AudioProcessorValueTreeState& s, const juce::String& id) { return s.getRawParameterValue (id)->load(); }
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

struct TrackInfo { juce::String name; int notes = 0; double first = -1.0; };
std::vector<TrackInfo> readMidi (const juce::File& f, double& endBeat)
{
    std::vector<TrackInfo> out;
    juce::FileInputStream in (f);
    juce::MidiFile mf;
    endBeat = 0.0;
    if (! mf.readFrom (in)) return out;
    for (int t = 0; t < mf.getNumTracks(); ++t)
    {
        TrackInfo ti;
        for (auto* ev : *mf.getTrack (t))
        {
            const auto& m = ev->message;
            if (m.isTrackNameEvent()) ti.name = m.getTextFromTextMetaEvent();
            if (m.isNoteOn()) { ++ti.notes; if (ti.first < 0) ti.first = m.getTimeStamp() / 960.0; }
            endBeat = std::max (endBeat, m.getTimeStamp() / 960.0);
        }
        out.push_back (ti);
    }
    return out;
}
int notesIn (const std::vector<TrackInfo>& v, const char* key)
{
    for (auto& t : v) if (t.name.contains (key)) return t.notes;
    return -1;
}
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String arg1 (argc > 1 ? argv[1] : "riddim.png");
    const juce::String arg2 (argc > 2 ? argv[2] : "");
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (arg1));

    MrmProcessor proc;
    set (proc.apvts, "seed", 42.0f);
    set (proc.getDrums().apvts, "intro", arg2.contains ("intro") ? 1.0f : 0.0f);
    pump();

    if (arg1 == "links")
    {
        auto& c = proc.getChords().apvts; auto& b = proc.getBass().apvts;
        set (c, "root0", 2.0f); set (c, "qual0", 0.0f); set (c, "dur1", 3.0f); set (c, "on2", 1.0f);
        pump();
        bool ok = (int) get (b, "root0") == 2 && (int) get (b, "qual0") == mbm::Maj && (int) get (b, "dur1") == 3 && get (b, "on2") > 0.5f;
        set (b, "root1", 7.0f); set (b, "qual1", (float) mbm::Min7);   // o baixo também manda na harmonia
        pump();
        ok = ok && (int) get (c, "root1") == 7 && (int) get (c, "qual1") == dub::Min && (int) get (b, "qual1") == mbm::Min7;
        set (proc.getDrums().apvts, "bpm", 92.0f);
        pump();
        ok = ok && (int) get (proc.apvts, "bpm") == 92 && (int) get (c, "bpm") == 92 && (int) get (b, "bpm") == 92;
        proc.newRiddim();
        pump();
        const int s = (int) get (proc.apvts, "seed");
        ok = ok && s != 42 && (int) get (proc.getDrums().apvts, "seed") == s && (int) get (b, "seed") == s;
        ok = ok && get (b, "drumsOn") < 0.5f && get (b, "keysOn") < 0.5f;
        std::printf ("links: %s (semente nova %d)\n", ok ? "ok" : "FALHOU", s);
        return ok ? 0 : 1;
    }

    if (arg1 == "mute")
    {
        // CONTROL ROOM: cada máquina mutada some do som (o MIDI ao vivo continua); as três mutadas = silêncio
        auto render = [&proc] (bool& midiOut)
        {
            const double sr = 48000.0; const int block = 480;
            proc.setPlayConfigDetails (0, 2, sr, block);
            proc.prepareToPlay (sr, block);
            FakeDaw fake;
            proc.setPlayHead (&fake);
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer midi;
            double sum = 0.0;
            midiOut = false;
            for (int i = 0; i < 600; ++i)
            {
                midi.clear();
                proc.processBlock (buf, midi);
                fake.ppq += (double) block / sr * fake.bpm / 60.0;
                if (i > 100) for (int k = 0; k < block; ++k) sum += buf.getSample (0, k) * buf.getSample (0, k);
                if (! midi.isEmpty()) midiOut = true;
            }
            proc.setPlayHead (nullptr);
            return std::sqrt (sum / (499.0 * block));
        };
        bool m1 = false, m2 = false;
        set (proc.getKnob().apvts, "echoOn", 0.0f);
        pump();
        const double all = render (m1);
        for (int m = 0; m < MrmProcessor::NumMachines; ++m) set (proc.apvts, MrmProcessor::muteId (m), 1.0f);
        pump();
        const double none = render (m2);
        const bool ok = all > 0.01 && none < 1.0e-4 && m2;
        std::printf ("mute: %s (rms tudo %.4f, tudo mutado %.6f, MIDI continua: %s)\n", ok ? "ok" : "FALHOU", all, none, m2 ? "sim" : "nao");
        return ok ? 0 : 1;
    }

    if (arg1 == "samples")
    {
        // kit 0 = "Anos 70 (samples DM)": os WAVs de DubDrums/Samples, embutidos em DMSamples.h
        const bool ok = (int) get (proc.getDrums().apvts, "kit") == 0
                     && dubdrums::dmsamples::kickSize > 1000 && dubdrums::dmsamples::snareSize > 1000 && dubdrums::dmsamples::hatSize > 1000;
        std::printf ("samples: kit %s, DM-KICKS1 %d bytes, DM-SNARES6 %d bytes, DM-HATs6 %d bytes\n",
                     ok ? "DM" : "OUTRO", (int) dubdrums::dmsamples::kickSize, (int) dubdrums::dmsamples::snareSize, (int) dubdrums::dmsamples::hatSize);
        return ok ? 0 : 1;
    }

    if (arg1 == "edicao")
    {
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("mrm_edicao.mid");
        double end = 0.0;
        proc.writeRiddimFile (tmp);
        const auto before = readMidi (tmp, end);
        // harmonia: tira o primeiro ataque do skank
        double lb = 0.0;
        auto ev = proc.getChords().getCurrentLoop (lb);
        const double first = ev.front().start;
        const auto n0 = ev.size();
        ev.erase (std::remove_if (ev.begin(), ev.end(), [&] (const dub::NoteEvent& e) { return e.channel == dub::ChSkank && std::abs (e.start - first) < 1e-6; }), ev.end());
        const int removed = (int) (n0 - ev.size());
        proc.getChords().setEdits (ev, lb);
        // bateria: põe um bumbo numa semicolcheia vazia
        int addedDrums = 0;
        for (double beat = 0.25; beat < 4.0 && addedDrums == 0; beat += 0.25)
            if (! proc.getDrums().noteAt (dubdrums::gm::Kick, beat + 0.001) && proc.getDrums().toggleNote (dubdrums::gm::Kick, beat + 0.001, true)) addedDrums = 1;
        // baixo: tira a primeira nota
        double blb = 0.0;
        auto line = proc.getBass().getCurrentLine (blb);
        line.erase (line.begin());
        proc.getBass().setEdits (line, blb);
        pump();
        proc.writeRiddimFile (tmp);
        const auto after = readMidi (tmp, end);
        const int reps = (int) std::round (proc.riddimBars() * 4.0 / lb);
        const int breps = (int) std::round (proc.riddimBars() * 4.0 / blb);
        const bool ok = proc.getChords().hasEdits() && proc.getDrums().hasEdits() && proc.getBass().hasEdits()
                     && notesIn (after, "Skank") == notesIn (before, "Skank") - removed * reps
                     && notesIn (after, "Bateria") > notesIn (before, "Bateria")
                     && notesIn (after, "Baixo") == notesIn (before, "Baixo") - breps;
        std::printf ("edicao: %s (skank %d -> %d, bateria %d -> %d, baixo %d -> %d)\n", ok ? "ok" : "FALHOU",
                     notesIn (before, "Skank"), notesIn (after, "Skank"), notesIn (before, "Bateria"), notesIn (after, "Bateria"),
                     notesIn (before, "Baixo"), notesIn (after, "Baixo"));
        // mexer na harmonia invalida a edição da harmonia (como no MRDM-12 e no MRBM-69)
        set (proc.getChords().apvts, "root0", 4.0f);
        pump();
        const bool reset = ! proc.getChords().hasEdits();
        std::printf ("  edicao some ao mexer na harmonia: %s\n", reset ? "ok" : "FALHOU");
        return ok && reset ? 0 : 1;
    }

    if (arg1 == "maquinas")
    {
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("mrm_maquinas");
        dir.createDirectory();
        bool ok = true;
        for (int m = 0; m < MrmProcessor::NumMachines; ++m)
        {
            auto f = dir.getChildFile (proc.suggestedMachineName (m));
            double end = 0.0;
            const bool w = proc.writeMachineFile (m, f);
            const auto t = readMidi (f, end);
            int notes = 0; for (auto& x : t) notes += x.notes;
            std::printf ("  %-48s %s, %d notas\n", f.getFileName().toRawUTF8(), w ? "ok" : "FALHOU", notes);
            ok = ok && w && notes > 0;
        }
        set (proc.apvts, "bassOn", 0.0f);
        pump();
        auto f = dir.getChildFile ("sem-baixo.mid");
        proc.writeRiddimFile (f);
        double end = 0.0;
        const auto t = readMidi (f, end);
        ok = ok && notesIn (t, "Baixo") < 0 && notesIn (t, "Bateria") > 0;
        std::printf ("maquinas: %s (baixo desligado fica fora do riddim)\n", ok ? "ok" : "FALHOU");
        return ok ? 0 : 1;
    }

    if (arg1 == "estado")
    {
        set (proc.apvts, "bpm", 92.0f);
        set (proc.getChords().apvts, "root1", 5.0f);
        set (proc.getDrums().apvts, "style", 2.0f);
        set (proc.getBass().apvts, "style", 3.0f);
        set (proc.getKnob().apvts, "knob", 0.3f);
        set (proc.apvts, "fxOn", 0.0f);
        proc.setCurrentTab (3);
        pump();
        double lb = 0.0;
        auto ev = proc.getChords().getCurrentLoop (lb);
        ev.pop_back();
        proc.getChords().setEdits (ev, lb);
        pump();
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        MrmProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        pump();
        double lb2 = 0.0;
        const bool ok = other.getCurrentTab() == 3
                     && (int) get (other.apvts, "bpm") == 92
                     && (int) get (other.getChords().apvts, "bpm") == 92
                     && (int) get (other.getDrums().apvts, "bpm") == 92
                     && (int) get (other.getBass().apvts, "bpm") == 92
                     && (int) get (other.getChords().apvts, "root1") == 5
                     && (int) get (other.getBass().apvts, "root1") == 5
                     && (int) get (other.getDrums().apvts, "style") == 2
                     && (int) get (other.getBass().apvts, "style") == 3
                     && std::abs (get (other.getKnob().apvts, "knob") - 0.3f) < 0.01f
                     && get (other.apvts, "fxOn") < 0.5f
                     && other.getChords().hasEdits() && other.getChords().getCurrentLoop (lb2).size() == ev.size();
        std::printf ("estado: %s (%d bytes)\n", ok ? "ok" : "FALHOU", (int) mb.getSize());
        return ok ? 0 : 1;
    }

    if (out.hasFileExtension ("mid"))
    {
        if (! proc.writeRiddimFile (out)) return fail ("escrever MIDI");
        double end = 0.0;
        const auto tracks = readMidi (out, end);
        const double intro = proc.introBeats();
        std::printf ("midi: %s  (%s)\n  faixas: %d\n", out.getFileName().toRawUTF8(), proc.suggestedRiddimName().toRawUTF8(), (int) tracks.size());
        bool late = true;
        for (auto& t : tracks)
        {
            std::printf ("  %-32s notas %4d  primeira em %.2f tempos\n", t.name.toRawUTF8(), t.notes, t.first);
            if ((t.name.contains ("Skank") || t.name.contains ("Bubble") || t.name.contains ("Baixo")) && t.first >= 0 && t.first < intro - 1e-6) late = false;
        }
        const double expectEnd = intro + proc.riddimBars() * 4.0;
        std::printf ("  fim: %.2f tempos (esperado %.2f)\n", end, expectEnd);
        if (std::abs (end - expectEnd) > 1e-6) return fail ("tamanho do riddim");
        if (! late) return fail ("harmonia ou baixo antes do fim da intro");
        if (notesIn (tracks, "Bateria") <= 0 || notesIn (tracks, "Skank") <= 0 || notesIn (tracks, "Baixo") <= 0) return fail ("faixa faltando");
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
        int firstChord = -1, firstBass = -1, firstDrum = -1, earlyNotes = 0;
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
                if (m.getChannel() == 10) { if (firstDrum < 0) firstDrum = s; continue; }
                if (m.getChannel() == 2) { if (firstBass < 0) firstBass = s; }
                else if (firstChord < 0) firstChord = s;
                if ((double) s / sr < introSec - 0.001) ++earlyNotes;
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
            std::printf ("  intro %.3f s | bateria %.3f s | harmonia %.3f s | baixo %.3f s | notas na intro: %d\n",
                         introSec, firstDrum / sr, firstChord / sr, firstBass / sr, earlyNotes);
            if (earlyNotes > 0 || firstChord < 0 || firstBass < 0 || firstDrum < 0) return fail ("alinhamento com a intro");
        }
        if (peak > 1.0f || sum <= 0.0) return fail ("nivel do audio");
        return 0;
    }

    // imagem de uma aba
    proc.setCurrentTab (arg2.getIntValue());
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (MrmRoot::baseW, MrmRoot::baseH);
    pump (14);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("salvo: %s (%dx%d)\n", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    return 0;
}
