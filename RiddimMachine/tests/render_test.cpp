// Teste do MRM-0420 sem DAW.
//   mrm_render riddim.png 0            -> imagem da aba (0 PRINCIPAL, 1 BATERIA, 2 BAIXO, 3 RIDDIM, 4 ÓRGÃO, 5 ARRANJO, 6 MIXER, 7 SHOW, 8 CONTROL ROOM)
//   mrm_render arranjo                 -> ARRANJO: cada parte toca só as máquinas dela, a música para no fim
//   mrm_render show                    -> SHOW: salva, carrega e fica no projeto
//   mrm_render exportar                -> EXPORTAR WAV + MIDI do arranjo, fora do tempo real
//   mrm_render sirene                  -> sirene pelo Tape Echo MTE-12: eco depois de soltar e silêncio depois do rabo
//   mrm_render mixer                   -> MIXER: baixo -2 dB de fábrica, volume, pan e solo
//   mrm_render riddim.png 0 alavanca   -> a mesma imagem depois de puxar a ALAVANCA (riddim 100% aleatório)
//   mrm_render riddim.mid [intro]      -> o MIDI do ARRASTE O RIDDIM (faixas, tamanho, harmonia e baixo depois da intro)
//   mrm_render play.wav [intro]        -> 12 s do PLAY RIDDIM com as três máquinas e o Big Knob
//   mrm_render daw.wav intro           -> DAW tocando do zero: harmonia e baixo entram depois da intro
//   mrm_render links                   -> harmonia ligada (MRCM-1240 <-> MRBM-69), BPM único, semente do JACKPOT
//   mrm_render edicao                  -> MIDI editável da harmonia, da bateria e do baixo vai para o riddim
//   mrm_render maquinas                -> ARRASTE de cada máquina e máquina desligada fora do riddim
//   mrm_render samples                 -> o kit da bateria é o dos samples DM (os WAVs de ReggaeDrumMachine/Samples)
//   mrm_render banco                  -> BANCO ÚNICO DE RIDDIMS: cada música vai inteira para as três máquinas e fica salva no projeto
//   mrm_render estado                  -> salva e recarrega o estado (aba, BPM, harmonia, edições, Big Knob)
//   mrm_render orgao                   -> ÓRGÃO MRO-68 (2.8): harmonia e BPM ligados, solo e mute no mixer, salvo no projeto, projeto 2.7 sem órgão
#include "../../ReggaeChordMachine/Source/HarmonyPresets.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <map>
#include "../Source/MrmProcessor.h"
#include "../Source/MrmEditor.h"
#include "../../ReggaeDrumMachine/Source/DMSamples.h"
#include "../../RiddimBank/Source/RiddimBank.h"

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

    if (arg1 == "inicial")
    {
        // configuração inicial: Lá menor e Ré menor, meio compasso cada (2 acordes)
        auto& c = proc.getChords().apvts; auto& b = proc.getBass().apvts;
        bool ok = (int) get (c, "root0") == 9 && (int) get (c, "qual0") == dub::Min && (int) get (c, "root1") == 2 && (int) get (c, "qual1") == dub::Min
               && get (c, "on1") > 0.5f && get (c, "on2") < 0.5f && get (c, "on3") < 0.5f
               && (int) get (b, "root0") == 9 && (int) get (b, "root1") == 2 && get (b, "on2") < 0.5f
               && get (proc.getKnob().apvts, "echoOn") < 0.5f;   // TAPE ECHO começa desligado
        std::printf ("inicial: %s (Am %s, Dm %s, tape echo desligado)\n", ok ? "ok" : "FALHOU",
                     proc.getChords().apvts.getParameter ("dur0")->getCurrentValueAsText().toRawUTF8(),
                     proc.getChords().apvts.getParameter ("dur1")->getCurrentValueAsText().toRawUTF8());
        return ok ? 0 : 1;
    }

    if (arg1 == "presets")
    {
        // banco de presets: cada um aplica raiz/qualidade/duração e liga só os acordes do preset; o baixo segue
        auto& c = proc.getChords().apvts; auto& b = proc.getBass().apvts;
        bool ok = dubpresets::numPresets == 180 && dubpresets::numFolders == 8;
        for (int f = 0; f < dubpresets::numFolders; ++f)
        {
            int n = 0;
            for (int i = 0; i < dubpresets::numPresets; ++i) n += juce::String (dubpresets::presets[i].folder) == dubpresets::folders[f];
            if (n < 10) { ok = false; std::printf ("pasta %s tem %d presets\n", dubpresets::folders[f], n); }
        }
        for (int i = 0; i < dubpresets::numPresets && ok; ++i)
        {
            const auto& pr = dubpresets::presets[i];
            proc.applyRiddimPreset (i);
            pump();
            // o riddim inteiro: BPM, estilo, baterista e baixista do preset
            if ((int) std::lround (get (proc.apvts, "bpm")) != pr.bpm || (int) get (proc.getDrums().apvts, "style") != pr.kit
                || (int) get (proc.getDrums().apvts, "drummer") != pr.drummer || (int) get (b, "style") != pr.bassist) ok = false;
            for (int k = 0; k < 4; ++k)
            {
                const juce::String n (k);
                const bool on = k == 0 || get (c, "on" + n) > 0.5f;
                if (on != (k < pr.numChords)) ok = false;
                if (k < pr.numChords && ((int) get (c, "root" + n) != pr.chords[k].root || (int) get (b, "root" + n) != pr.chords[k].root
                                         || ((int) get (c, "qual" + n) == dub::Min) != pr.chords[k].minor)) ok = false;
            }
            if (! ok) std::printf ("preset %d (%s) FALHOU\n", i, pr.name);
        }
        {   // NOVA LINHA: semente nova só na máquina pedida, a outra e o JACKPOT ficam
            auto& d = proc.getDrums().apvts;
            const float ds = get (d, "seed"), bs = get (b, "seed"), js = get (proc.apvts, "seed");
            proc.newLine (MrmProcessor::Drums);
            pump();
            const bool drumsOk = get (d, "seed") != ds && get (b, "seed") == bs && get (proc.apvts, "seed") == js;
            const float ds2 = get (d, "seed");
            proc.newLine (MrmProcessor::Bass);
            pump();
            const bool bassOk = get (b, "seed") != bs && get (d, "seed") == ds2;
            std::printf ("nova linha: bateria %s, baixo %s\n", drumsOk ? "ok" : "FALHOU", bassOk ? "ok" : "FALHOU");
            ok = ok && drumsOk && bassOk;
        }
        std::printf ("presets: %s (%d presets em %d pastas)\n", ok ? "ok" : "FALHOU", dubpresets::numPresets, dubpresets::numFolders);
        return ok ? 0 : 1;
    }

    if (arg1 == "banco")
    {
        // cada música do banco com harmonia: acordes no MRCM e no MRBM, BPM único, kit, baterista e baixista dela
        auto& c = proc.getChords().apvts; auto& b = proc.getBass().apvts; auto& d = proc.getDrums().apvts;
        int tested = 0, bad = 0;
        for (int i = 0; i < riddimbank::numEntries; ++i)
        {
            const auto& e = riddimbank::entries[i];
            if (e.numChords <= 0) continue;
            ++tested;
            proc.applyBankRiddim (i);
            pump (2);
            bool ok = proc.currentBankRiddim() == i
                   && (int) std::lround (get (proc.apvts, "bpm")) == e.bpm && (int) std::lround (get (b, "bpm")) == e.bpm
                   && (int) get (d, "style") == e.kit && (int) get (d, "drummer") == e.drummer && (int) get (b, "style") == e.bassStyle
                   && (int) get (b, "timbre") == e.timbre
                   && get (b, "drumsOn") < 0.5f;
            for (int k = 0; k < 4; ++k)
            {
                const juce::String n (k);
                const bool on = k == 0 || get (c, "on" + n) > 0.5f;
                if (on != (k < e.numChords)) ok = false;
                if (k < e.numChords && ((int) get (c, "root" + n) != e.chords[k].root || (int) get (b, "root" + n) != e.chords[k].root
                                        || (int) get (c, "dur" + n) != e.chords[k].dur || (int) get (b, "dur" + n) != e.chords[k].dur
                                        || ((int) get (c, "qual" + n) == dub::Min) != (e.chords[k].minor != 0))) ok = false;
            }
            if (! ok && ++bad < 8) std::printf ("  banco %d (%s) FALHOU\n", i, e.name);
        }
        // harmonia pendente não carrega (nada muda)
        int pending = -1;
        for (int i = 0; i < riddimbank::numEntries && pending < 0; ++i) if (riddimbank::entries[i].numChords == 0) pending = i;
        const int before = proc.currentBankRiddim();
        if (pending >= 0) proc.applyBankRiddim (pending);
        const bool pendingOk = proc.currentBankRiddim() == before;
        // o número da música fica salvo no projeto
        int pick = -1;
        for (int i = riddimbank::numEntries - 1; i >= 0 && pick < 0; --i) if (riddimbank::entries[i].numChords > 0 && riddimbank::entries[i].mrcm < 0) pick = i;
        proc.applyBankRiddim (pick);
        pump (2);
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        MrmProcessor again;
        again.setStateInformation (mb.getData(), (int) mb.getSize());
        pump (2);
        const bool saved = again.currentBankRiddim() == pick;
        // projeto antigo: o preset do MRCM-1240 aparece como a música do banco
        proc.applyRiddimPreset (0);
        pump (2);
        const bool legacy = proc.currentBankRiddim() == riddimbank::fromMrcm[0];
        const bool ok = bad == 0 && tested > 300 && pendingOk && saved && legacy;
        std::printf ("banco: %s (%d musicas testadas, %d falharam, pendente ignorada: %s, salvo no projeto: %s, preset antigo -> banco: %s)\n",
                     ok ? "ok" : "FALHOU", tested, bad, pendingOk ? "sim" : "nao", saved ? "sim" : "nao", legacy ? "sim" : "nao");
        return ok ? 0 : 1;
    }

    if (arg1 == "aleatorio")
    {
        // ALAVANCA: sorteia harmonia, durações, baterista e estilo; tudo fica nos parâmetros (na tela) e o baixo segue
        auto& c = proc.getChords().apvts; auto& b = proc.getBass().apvts; auto& d = proc.getDrums().apvts;
        auto snap = [&] { juce::String s; for (int k = 0; k < 4; ++k) s << get (c, "root" + juce::String (k)) << get (c, "dur" + juce::String (k)) << (k == 0 ? 1.0f : get (c, "on" + juce::String (k)));
                          return s + juce::String (get (d, "style")) + juce::String (get (d, "drummer")) + juce::String (get (proc.apvts, "seed")); };
        juce::StringArray seen;
        bool linked = true;
        int harmonyChanges = 0, drumChanges = 0;
        const auto start = snap();
        float lastStyle = get (d, "style"), lastDrummer = get (d, "drummer");
        juce::String lastHarm = start.substring (0, 12);
        for (int i = 0; i < 12; ++i)
        {
            proc.randomRiddim();
            pump();
            const auto s = snap();
            seen.addIfNotAlreadyThere (s);
            if (s.substring (0, 12) != lastHarm) ++harmonyChanges;
            if (get (d, "style") != lastStyle || get (d, "drummer") != lastDrummer) ++drumChanges;
            lastHarm = s.substring (0, 12); lastStyle = get (d, "style"); lastDrummer = get (d, "drummer");
            for (int k = 0; k < 4; ++k)
            {
                const juce::String n (k);
                if ((int) get (b, "root" + n) != (int) get (c, "root" + n) || (int) get (b, "dur" + n) != (int) get (c, "dur" + n)) linked = false;
            }
            if ((int) get (b, "seed") != (int) get (proc.apvts, "seed")) linked = false;
        }
        const bool ok = seen.size() >= 10 && harmonyChanges >= 8 && drumChanges >= 6 && linked && get (proc.apvts, "play") < 0.5f;
        std::printf ("aleatorio: %s (%d riddims diferentes, harmonia mudou %d/12, bateria mudou %d/12, baixo ligado: %s)\n",
                     ok ? "ok" : "FALHOU", seen.size(), harmonyChanges, drumChanges, linked ? "sim" : "nao");
        return ok ? 0 : 1;
    }

    if (arg1 == "reggae")
    {
        // ALAVANCA sempre no reggae: piano só no contratempo, caixa só no 2 e no 4,
        // baixo só com notas do acorde da hora (pentatônica do baixista)
        int badSkank = 0, badSnare = 0, badBass = 0, snares = 0, bassNotes = 0, skanks = 0;
        for (int run = 0; run < 40; ++run)
        {
            proc.randomRiddim();
            pump (3);
            double lb = 0.0;
            const auto slots = proc.getChords().getSlots();
            for (const auto& e : proc.getChords().getCurrentLoop (lb))
                if (e.channel == dub::ChSkank)
                {
                    ++skanks;
                    const double f = std::fmod (e.start, 1.0);
                    if (std::abs (f - 0.5) > 0.06) ++badSkank;
                }
            const auto pat = proc.getDrums().getPatternCopy();
            for (const auto& e : pat.events)
                if (e.note == dubdrums::gm::Snare || e.note == dubdrums::gm::SideStick)
                {
                    ++snares;
                    const double inBar = std::fmod (e.beat, 4.0);
                    const bool onBackbeat = std::abs (inBar - 1.0) < 0.06 || std::abs (inBar - 3.0) < 0.06;
                    const bool lastBarFill = e.beat >= pat.lengthInBeats - 4.0 + 1e-9 && get (proc.getDrums().apvts, "fill") > 0.5f;
                    if (! onBackbeat && ! lastBarFill) { ++badSnare; if (badSnare < 6) std::printf ("  caixa fora: tempo %.2f nota %d vel %d estilo %d baterista %d\n", e.beat, e.note, e.velocity, (int) get (proc.getDrums().apvts, "style"), (int) get (proc.getDrums().apvts, "drummer")); }
                }
            double bl = 0.0;
            for (const auto& e : proc.getBass().getCurrentLine (bl))
            {
                ++bassNotes;
                // acorde que soa naquele tempo
                double t = 0.0; int root = 0; bool minor = false;
                const double at = std::fmod (e.start, lb > 0 ? lb : 1.0);
                for (const auto& sl : slots) { if (sl.beats <= 0) continue; if (at >= t - 1e-9) { root = sl.root; minor = sl.quality == dub::Min; } t += sl.beats; }
                static const int minP[] = { 0, 3, 5, 7, 10 }, majP[] = { 0, 2, 4, 7, 9 };
                const int iv = ((e.note - root) % 12 + 12) % 12;
                bool ok = false;
                for (int k = 0; k < 5; ++k) if (iv == (minor ? minP[k] : majP[k])) ok = true;
                if (! ok) { ++badBass; if (badBass < 25) std::printf ("  baixo fora: tempo %.2f nota %d acorde %d%s baixista %d fill %d slot %d loop %.1f/%.1f\n", e.start, e.note, root, minor ? "m" : "", (int) get (proc.getBass().apvts, "style"), (int) e.fill, e.slot, lb, bl); }
            }
        }
        const bool ok = badSkank == 0 && badSnare == 0 && badBass == 0 && skanks > 0 && snares > 0 && bassNotes > 0;
        std::printf ("reggae: %s (40 alavancadas: skank fora do contratempo %d/%d, caixa fora do 2 e 4 %d/%d, baixo fora do acorde %d/%d)\n",
                     ok ? "ok" : "FALHOU", badSkank, skanks, badSnare, snares, badBass, bassNotes);
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
        for (int m = 0; m < MrmProcessor::NumMixer; ++m) set (proc.apvts, MrmProcessor::muteId (m), 1.0f);   // 2.8: o órgão também
        pump();
        const double none = render (m2);
        const bool ok = all > 0.01 && none < 1.0e-4 && m2;
        std::printf ("mute: %s (rms tudo %.4f, tudo mutado %.6f, MIDI continua: %s)\n", ok ? "ok" : "FALHOU", all, none, m2 ? "sim" : "nao");
        return ok ? 0 : 1;
    }

    if (arg1 == "samples")
    {
        // kit 0 = "Anos 70 (samples DM)": os WAVs de ReggaeDrumMachine/Samples, embutidos em DMSamples.h
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

    // DAW de mentira tocando: devolve o MIDI por compasso (canal -> notas)
    auto runDaw = [&proc] (double seconds, std::function<void (double)> onBlock = {}) -> std::map<int, std::map<int, int>>
    {
        const double sr = 48000.0; const int block = 480;
        proc.setPlayConfigDetails (0, 2, sr, block);
        proc.prepareToPlay (sr, block);
        FakeDaw fake;
        proc.setPlayHead (&fake);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        std::map<int, std::map<int, int>> perBar;
        for (int i = 0; i < (int) (seconds * sr / block); ++i)
        {
            midi.clear();
            proc.processBlock (buf, midi);
            for (const auto m : midi)
                if (m.getMessage().isNoteOn())
                    perBar[(int) ((fake.ppq + m.samplePosition / sr * fake.bpm / 60.0) / 4.0)][m.getMessage().getChannel()]++;
            if (onBlock) onBlock (fake.ppq);
            fake.ppq += (double) block / sr * fake.bpm / 60.0;
        }
        proc.setPlayHead (nullptr);
        return perBar;
    };

    if (arg1 == "arranjo")
    {
        // Intro (sem baixo) 1 compasso, Verso (todos) 1, Dub (sem riddim) 1: depois a música para
        auto a = mrm::Arrangement::makeDefault();
        a.sections[0].bars = 1; a.sections[1].bars = 1; a.sections[4].bars = 1;
        a.order = { 0, 1, 4 };
        a.on = true;
        proc.setArrangement (a);
        set (proc.getKnob().apvts, "echoOn", 0.0f);
        pump();
        std::vector<int> slots;
        const auto bars = runDaw (4 * 4 * 60.0 / 75.0 + 0.5, [&] (double) { slots.push_back (proc.getArrangeSlot()); });
        auto n = [&] (int bar, int ch) { auto it = bars.find (bar); if (it == bars.end()) return 0; auto j = it->second.find (ch); return j == it->second.end() ? 0 : j->second; };
        const int drumCh = juce::roundToInt (get (proc.getDrums().apvts, "channel"));
        bool ok = n (0, 1) > 0 && n (0, 2) == 0 && n (0, drumCh) > 0     // intro: skank e bateria, sem baixo
               && n (1, 1) > 0 && n (1, 2) > 0 && n (1, drumCh) > 0     // verso: todos
               && n (2, 1) == 0 && n (2, 2) > 0 && n (2, drumCh) > 0     // dub: sem o riddim
               && n (3, 1) + n (3, 2) + n (3, drumCh) == 0;               // fim: nada
        const bool sawAll = std::find (slots.begin(), slots.end(), 0) != slots.end() && std::find (slots.begin(), slots.end(), 2) != slots.end()
                         && proc.isArrangeEnded();
        ok = ok && sawAll;
        // estado: o arranjo fica no projeto
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        MrmProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        const auto b2 = other.getArrangement();
        ok = ok && b2.on && b2.order.size() == 3 && b2.sections[4].bars == 1 && ! b2.sections[4].on[MrmProcessor::Chords];
        std::printf ("arranjo: %s (intro skank %d baixo %d | verso skank %d baixo %d | dub skank %d baixo %d | fim %d)\n", ok ? "ok" : "FALHOU",
                     n (0, 1), n (0, 2), n (1, 1), n (1, 2), n (2, 1), n (2, 2), n (3, 1) + n (3, 2) + n (3, drumCh));
        return ok ? 0 : 1;
    }

    if (arg1 == "show")
    {
        set (proc.apvts, "bpm", 92.0f);
        pump();
        proc.saveShowSlot (3, "Show teste");
        set (proc.apvts, "bpm", 100.0f);
        pump();
        bool ok = proc.getShowSlot (3).used && proc.getShowSlot (3).name == "Show teste" && ! proc.getShowSlot (0).used;
        ok = ok && proc.loadShowSlot (3);
        pump();
        ok = ok && (int) get (proc.apvts, "bpm") == 92 && proc.getShowSlot (3).used;   // carregar não apaga o SHOW
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        MrmProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        ok = ok && other.getShowSlot (3).used && other.getShowSlot (3).info.contains ("92 BPM");
        const auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("teste.mrmshow");
        ok = ok && proc.exportShow (f);
        MrmProcessor third;
        ok = ok && third.importShow (f) && third.getShowSlot (3).name == "Show teste";
        std::printf ("show: %s (%s)\n", ok ? "ok" : "FALHOU", other.getShowSlot (3).info.toRawUTF8());
        return ok ? 0 : 1;
    }

    if (arg1 == "exportar")
    {
        auto a = mrm::Arrangement::makeDefault();
        a.sections[1].bars = 2;
        a.order = { 1 };
        proc.setArrangement (a);
        pump();
        const auto wav = juce::File::getCurrentWorkingDirectory().getChildFile ("arranjo.wav");
        if (! proc.startSongExport (wav)) return fail ("exportar não começou");
        for (int i = 0; i < 1200 && proc.getExportProgress() >= 0.0f; ++i) pump (1);
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (wav));
        double end = 0.0;
        const auto tracks = readMidi (wav.withFileExtension ("mid"), end);
        const double sec = r != nullptr ? (double) r->lengthInSamples / r->sampleRate : 0.0;
        float lo = 0, hi = 0;
        if (r) { juce::Range<float> rg[2]; r->readMaxLevels (0, r->lengthInSamples, rg, 2); lo = rg[0].getStart(); hi = juce::jmax (rg[0].getEnd(), rg[1].getEnd()); }
        int notes = 0; for (auto& t : tracks) notes += t.notes;
        const bool ok = r != nullptr && r->bitsPerSample == 24 && sec > 2 * 4 * 60.0 / 75.0 && sec < 2 * 4 * 60.0 / 75.0 + 4.5 && hi > 0.05f
                     && notes > 10 && end <= 9.0;
        std::printf ("exportar: %s (%.2f s, pico %.2f, MIDI %d notas em %d faixas, fim %.2f tempos) %s\n", ok ? "ok" : "FALHOU", sec, hi, notes,
                     (int) tracks.size(), end, proc.getExportMessage().toRawUTF8());
        return ok ? 0 : 1;
    }

    if (arg1 == "sirene")
    {
        // DAW parada: só a sirene. 0,5 s de FIRE, depois o eco do MTE-12 continua e some; 12 s depois, silêncio total
        const double sr = 48000.0; const int block = 480;
        proc.setPlayConfigDetails (0, 2, sr, block);
        proc.prepareToPlay (sr, block);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        double dry = 0, tail = 0, late = 0;
        bool knobSilent = get (proc.getKnob().apvts, "sirFire") < 0.5f;
        for (int i = 0; i < (int) (15.0 * sr / block); ++i)
        {
            const double t = i * block / sr;
            set (proc.apvts, "sirenFire", t < 0.5 ? 1.0f : 0.0f);
            midi.clear();
            proc.processBlock (buf, midi);
            const double pk = buf.getMagnitude (0, 0, block);
            if (t < 0.5) dry = std::max (dry, pk);
            else if (t > 0.9 && t < 3.0) tail = std::max (tail, pk);
            else if (t > 12.5) late = std::max (late, pk);
        }
        const bool echoOn = get (proc.apvts, "sirEchoOn") > 0.5f && (int) get (proc.apvts, "sirEchoMode") == 6;
        const bool ok = dry > 0.05 && tail > 0.02 && late < 1.0e-5 && echoOn && knobSilent;
        std::printf ("sirene: %s (sirene %.3f, eco depois de soltar %.3f, depois do rabo %.6f, tape echo ligado no Mode 7: %s)\n",
                     ok ? "ok" : "FALHOU", dry, tail, late, echoOn ? "sim" : "nao");
        return ok ? 0 : 1;
    }

    if (arg1 == "mixer")
    {
        bool ok = std::abs (get (proc.apvts, MrmProcessor::volId (MrmProcessor::Bass)) + 2.0f) < 0.01f
               && std::abs (get (proc.apvts, MrmProcessor::volId (MrmProcessor::Drums))) < 0.01f;
        set (proc.getKnob().apvts, "echoOn", 0.0f);
        set (proc.apvts, MrmProcessor::soloId (MrmProcessor::Drums), 1.0f);
        pump();
        float lv[3] {};
        runDaw (3.0, [&] (double) { for (int m = 0; m < 3; ++m) lv[m] = std::max (lv[m], proc.getMachineLevel (m)); });
        ok = ok && lv[MrmProcessor::Drums] > 0.05f && lv[MrmProcessor::Chords] < 1.0e-4f && lv[MrmProcessor::Bass] < 1.0e-4f;
        std::printf ("mixer: %s (baixo %.1f dB de fabrica; solo bateria: bateria %.3f riddim %.5f baixo %.5f)\n", ok ? "ok" : "FALHOU",
                     get (proc.apvts, MrmProcessor::volId (MrmProcessor::Bass)), lv[0], lv[1], lv[2]);
        return ok ? 0 : 1;
    }

    // 2.8 · ÓRGÃO (MRO-68): toca junto com o riddim, segue a harmonia e o BPM, mixer (solo e mute),
    // fica no projeto e projetos antigos abrem com ele desligado (e a aba certa)
    if (arg1 == "orgao")
    {
        set (proc.getKnob().apvts, "echoOn", 0.0f);
        set (proc.getChords().apvts, "root0", 7.0f);    // G
        set (proc.getChords().apvts, "qual0", 0.0f);    // maior
        set (proc.apvts, "bpm", 84.0f);
        pump();
        const bool linked = (int) get (proc.getOrgan().apvts, "root1") == 7 && get (proc.getOrgan().apvts, "minor1") < 0.5f
                         && (int) get (proc.getOrgan().apvts, "bpm") == 84 && get (proc.getOrgan().apvts, "chordSrc") < 0.5f;
        const bool onDefault = get (proc.apvts, "organOn") > 0.5f;
        set (proc.apvts, MrmProcessor::soloId (MrmProcessor::Organ), 1.0f);
        pump();
        float lv[4] {};
        runDaw (6.0, [&] (double) { for (int m = 0; m < 4; ++m) lv[m] = std::max (lv[m], proc.getMachineLevel (m)); });
        const bool solo = lv[MrmProcessor::Organ] > 0.01f && lv[MrmProcessor::Drums] < 1.0e-4f && lv[MrmProcessor::Bass] < 1.0e-4f;
        set (proc.apvts, MrmProcessor::soloId (MrmProcessor::Organ), 0.0f);
        set (proc.apvts, MrmProcessor::muteId (MrmProcessor::Organ), 1.0f);
        pump();
        float lv2[4] {};
        runDaw (4.0, [&] (double t) { if (t > 2.0) for (int m = 0; m < 4; ++m) lv2[m] = std::max (lv2[m], proc.getMachineLevel (m)); });   // o medidor cai devagar
        const bool mute = lv2[MrmProcessor::Organ] < 1.0e-4f && lv2[MrmProcessor::Drums] > 0.05f;
        set (proc.apvts, MrmProcessor::muteId (MrmProcessor::Organ), 0.0f);

        // estado: o órgão vai inteiro no projeto
        set (proc.getOrgan().apvts, "musico", 3.0f);
        proc.setCurrentTab (MrmProcessor::TabOrgan);
        pump();
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        MrmProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        pump();
        const bool saved = (int) get (other.getOrgan().apvts, "musico") == 3 && other.getCurrentTab() == MrmProcessor::TabOrgan
                        && get (other.apvts, "organOn") > 0.5f && (int) get (other.getOrgan().apvts, "root1") == 7;

        // projeto da 2.7: sem o órgão no estado e com a aba ARRANJO no índice 4
        auto xml = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
        xml->removeChildElement (xml->getChildByName ("MRO68"), true);
        xml->removeAttribute ("tabs");
        xml->setAttribute ("tab", 4);
        juce::MemoryBlock old;
        juce::AudioProcessor::copyXmlToBinary (*xml, old);
        MrmProcessor legacy;
        legacy.setStateInformation (old.getData(), (int) old.getSize());
        pump();
        const bool oldOk = get (legacy.apvts, "organOn") < 0.5f && legacy.getCurrentTab() == MrmProcessor::TabArrange;

        const bool ok = linked && onDefault && solo && mute && saved && oldOk;
        std::printf ("orgao: %s (harmonia/BPM ligados: %s, ligado de fabrica: %s, solo: orgao %.3f bateria %.5f baixo %.5f, mute: orgao %.5f bateria %.3f, "
                     "salvo no projeto: %s, projeto 2.7 abre sem orgao e na aba ARRANJO: %s)\n", ok ? "ok" : "FALHOU",
                     linked ? "sim" : "NAO", onDefault ? "sim" : "NAO", lv[3], lv[0], lv[2], lv2[3], lv2[0], saved ? "sim" : "NAO", oldOk ? "sim" : "NAO");
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
    if (argc > 3 && juce::String (argv[3]) == "alavanca") { proc.randomRiddim(); pump(); }   // imagem depois de puxar a ALAVANCA
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
