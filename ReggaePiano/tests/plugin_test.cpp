// Teste do Mastor Reggae Piano MRP-80 com a aba CHORD MACHINE (MRCM-1240 embutido).
// cmake -B build -DJUCE_DIR=... -DROOTSKEYS_TESTS=ON && cmake --build build --target RootsKeysTest
// ./RootsKeysTest [pasta-das-imagens]   -> falha (exit 1) se algum item não passar
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../../ReggaeChordMachine/Source/PluginProcessor.h"

namespace
{
int failures = 0;
void check (bool ok, const juce::String& what)
{
    std::printf ("%s  %s\n", ok ? "ok   " : "FALHA", what.toRawUTF8());
    if (! ok) ++failures;
}

struct FakePlayHead : juce::AudioPlayHead
{
    double bpm = 90.0, ppq = 0.0; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm);
        p.setPpqPosition (ppq);
        p.setIsPlaying (playing);
        p.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
        return p;
    }
};

void setP (juce::AudioProcessorValueTreeState& s, const juce::String& id, float v)
{
    auto* p = s.getParameter (id);
    p->setValueNotifyingHost (p->convertTo0to1 (v));
}
float getP (juce::AudioProcessorValueTreeState& s, const juce::String& id) { return s.getRawParameterValue (id)->load(); }

// toca `seconds` de DAW e devolve o RMS; ruim = NaN/Inf
double render (RootsKeysProcessor& p, double seconds, bool& finite)
{
    const double sr = 48000.0; const int bs = 512;
    FakePlayHead ph;
    p.setPlayHead (&ph);
    p.prepareToPlay (sr, bs);
    juce::AudioBuffer<float> buf (2, bs);
    double sum = 0.0; long n = 0; finite = true;
    const int blocks = (int) (seconds * sr / bs);
    for (int b = 0; b < blocks; ++b)
    {
        // deixa a thread de mensagens montar o loop da machine (timer de 30 ms)
        if (b % 8 == 0) juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
        juce::MidiBuffer midi;
        buf.clear();
        p.processBlock (buf, midi);
        ph.ppq += bs / sr * ph.bpm / 60.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < bs; ++i)
            {
                const float v = buf.getSample (ch, i);
                if (! std::isfinite (v)) finite = false;
                sum += (double) v * v; ++n;
            }
    }
    p.setPlayHead (nullptr);
    return std::sqrt (sum / (double) juce::jmax (1L, n));
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir = argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
                                       : juce::File::getCurrentWorkingDirectory();

    {   // 1) machine desligada: a DAW tocando não gera acorde nenhum
        RootsKeysProcessor p;
        bool fin = true;
        const double rms = render (p, 4.0, fin);
        check (fin && rms < 1.0e-5, "CHORD MACHINE desligada fica em silencio (rms " + juce::String (rms, 7) + ")");
    }
    {   // 2) ligada, SOM INTERNO desligado: os acordes tocam no piano do instrumento
        RootsKeysProcessor p;
        check (getP (p.getMachine().apvts, "sound") < 0.5f, "instancia nova: SOM INTERNO da machine desligado");
        setP (p.apvts, "machineOn", 1.0f);
        bool fin = true;
        const double rms = render (p, 6.0, fin);
        check (fin && rms > 1.0e-3, "CHORD MACHINE toca no piano do MRP-80 (rms " + juce::String (rms, 5) + ")");
        check (getP (p.getMachine().apvts, "bubble") < 0.5f, "so piano: BUBBLE da machine desligado");
        // BANG BANG: o bang dobrado, no & e no a de cada tempo (caixa no 2 e no 4)
        setP (p.getMachine().apvts, "pattern", 1.0f);
        double beats = 0;
        const auto ev = dub::buildLoop (p.getMachine().getSlots(), p.getMachine().getSettings(), beats);
        int onAnd = 0, onA = 0, other = 0;
        for (const auto& e : ev)
        {
            if (e.channel != dub::ChSkank) continue;
            const int st = (int) std::lround (e.start * 4.0) % 4;
            if (st == 2) ++onAnd; else if (st == 3) ++onA; else ++other;
        }
        check (onAnd > 0 && onA == onAnd && other == 0, "BANG BANG: skank no & e no a (" + juce::String (onAnd) + " / " + juce::String (onA) + ")");
        const double rmsBang = render (p, 6.0, fin);
        check (fin && rmsBang > rms, "BANG BANG toca mais (rms " + juce::String (rmsBang, 5) + ")");
    }
    {   // 2b) PIANISTAS, FILLS, nota de 1/64 e os timbres novos tocam pela CHORD MACHINE
        for (int pn = 1; pn < 8; ++pn)
        {
            RootsKeysProcessor p;
            setP (p.apvts, "machineOn", 1.0f);
            setP (p.apvts, "pianoType", (float) (pn % 7));   // um timbre diferente por pianista (0..6)
            setP (p.getMachine().apvts, "pianist", (float) pn);
            setP (p.getMachine().apvts, "pianoFill", (float) (6 + pn));   // FILL separado do pianista (7..13: os de acordes)
            setP (p.getMachine().apvts, "pianoFillEvery", pn % 2 == 0 ? 1.0f : 0.0f);
            setP (p.getMachine().apvts, "nota64", pn % 2 == 0 ? 1.0f : 0.0f);
            bool fin = true;
            const double rms = render (p, 5.0, fin);
            check (fin && rms > 5.0e-4, "pianista " + juce::String (pn) + " + fill " + juce::String (6 + pn) + " + timbre " + juce::String (pn % 7)
                                        + " tocam (rms " + juce::String (rms, 5) + ")");
        }
    }
    {   // 2c) nota padrão do MRP-80: 1/64
        RootsKeysProcessor p;
        check (getP (p.getMachine().apvts, "nota64") > 0.5f, "nota padrao 1/64 na CHORD MACHINE do MRP-80");
    }
    {   // 3) SOM INTERNO ligado (projeto antigo): toca no piano do instrumento, sem o som proprio da machine
        RootsKeysProcessor p;
        setP (p.apvts, "machineOn", 1.0f);
        setP (p.getMachine().apvts, "sound", 1.0f);
        bool fin = true;
        const double rms = render (p, 6.0, fin);
        check (fin && rms > 1.0e-3, "SOM INTERNO antigo: o skank toca no piano (rms " + juce::String (rms, 5) + ")");
    }
    {   // 4) projeto salvo: a machine volta igual
        RootsKeysProcessor a, b;
        setP (a.getMachine().apvts, "root0", 4.0f);
        setP (a.getMachine().apvts, "qual0", 1.0f);
        setP (a.getMachine().apvts, "sound", 1.0f);
        setP (a.apvts, "machineOn", 1.0f);
        juce::MemoryBlock m;
        a.getStateInformation (m);
        b.setStateInformation (m.getData(), (int) m.getSize());
        auto& mb = b.getMachine().apvts;
        check (juce::roundToInt (getP (mb, "root0")) == 4 && juce::roundToInt (getP (mb, "qual0")) == 1
                 && getP (mb, "sound") > 0.5f && getP (b.apvts, "machineOn") > 0.5f,
               "projeto salvo reabre com a CHORD MACHINE igual");
    }
    {   // 5) projeto do Roots Keys 2.4 (aba DUB CHORDS): vira a CHORD MACHINE
        juce::ValueTree old ("PARAMS");
        auto add = [&old] (const char* id, double v)
        {
            juce::ValueTree c ("PARAM"); c.setProperty ("id", id, nullptr); c.setProperty ("value", v, nullptr);
            old.appendChild (c, nullptr);
        };
        add ("mode", 1); add ("dcOn", 1); add ("dcRoot0", 4); add ("dcQual0", 3 /* m7 */); add ("dcDur0", 1);
        add ("dcRoot1", 9); add ("dcQual1", 4 /* maj7 */); add ("dcAct1", 1); add ("dcAct2", 0); add ("dcAct3", 0);
        add ("dcPattern", 0 /* Auto: orgao = bubble */); add ("dcRegister", 55); add ("dcBpm", 82);
        old.setProperty ("program", 0, nullptr);
        juce::MemoryBlock m;
        juce::AudioProcessor::copyXmlToBinary (*old.createXml(), m);
        RootsKeysProcessor p;
        p.setStateInformation (m.getData(), (int) m.getSize());
        auto& mb = p.getMachine().apvts;
        check (juce::roundToInt (getP (mb, "root0")) == 4 && juce::roundToInt (getP (mb, "qual0")) == 1
                 && juce::roundToInt (getP (mb, "dur0")) == 1 && juce::roundToInt (getP (mb, "root1")) == 9
                 && juce::roundToInt (getP (mb, "qual1")) == 0 && getP (mb, "on1") > 0.5f && getP (mb, "on2") < 0.5f,
               "projeto antigo: acordes da DUB CHORDS (m7 -> menor, maj7 -> maior)");
        check (getP (mb, "bubble") < 0.5f && juce::roundToInt (getP (mb, "center")) == 55
                 && juce::roundToInt (getP (mb, "bpm")) == 82 && getP (mb, "sound") < 0.5f,
               "projeto antigo: sem bubble (o orgao e do MRO-68), registro e BPM mantidos");
        check (getP (p.apvts, "machineOn") > 0.5f, "projeto antigo: 'tocar com a DAW' vira CHORD MACHINE ligada");
        check (getP (mb, "nota64") < 0.5f, "projeto antigo continua em 1/32");
    }
    {   // 6) capturas da interface
        RootsKeysProcessor p;
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        auto* rk = dynamic_cast<RootsKeysEditor*> (ed.get());
        for (int f = 0; f < 6; ++f) juce::MessageManager::getInstance()->runDispatchLoopUntil (34);
        auto save = [&] (const juce::String& name)
        {
            for (int f = 0; f < 6; ++f) juce::MessageManager::getInstance()->runDispatchLoopUntil (34);
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.4f);
            juce::File f = outDir.getChildFile (name);
            f.deleteFile();
            juce::FileOutputStream os (f);
            juce::PNGImageFormat fmt;
            check (os.openedOk() && fmt.writeImageToStream (img, os), "imagem " + name);
        };
        save ("interface-piano.png");
        {   // o gabinete muda com o timbre: Rhodes, Suitcase, Wurlitzer, Yamaha (Grand/CP/C7) e Casio
            const int types[5] = { 10, 11, 12, 4, 9 };
            const char* names[5] = { "interface-piano-rhodes.png", "interface-piano-suitcase.png", "interface-piano-wurlitzer.png",
                                     "interface-piano-yamaha.png", "interface-piano-casio.png" };
            for (int i = 0; i < 5; ++i) { setP (p.apvts, "pianoType", (float) types[i]); save (names[i]); }
            setP (p.apvts, "pianoType", 7.0f);
        }
        rk->setPage (1);
        save ("interface-chord-machine.png");
        // aba MIDI da CHORD MACHINE: o visor editável (clica no botão "MIDI")
        std::function<juce::TextButton* (juce::Component*)> findMidi = [&] (juce::Component* c) -> juce::TextButton*
        {
            for (auto* ch : c->getChildren())
            {
                if (auto* b = dynamic_cast<juce::TextButton*> (ch); b != nullptr && b->getButtonText().startsWith ("MIDI ")) return b;
                if (auto* r = findMidi (ch)) return r;
            }
            return nullptr;
        };
        auto* midiTab = findMidi (ed.get());
        check (midiTab != nullptr, "aba MIDI na CHORD MACHINE");
        if (midiTab != nullptr)
        {
            setP (p.getMachine().apvts, "pianist", 4.0f);
            setP (p.getMachine().apvts, "pianoFill", 5.0f);
            midiTab->triggerClick();
            save ("interface-chord-machine-midi.png");
        }
        setP (p.getMachine().apvts, "pattern", 1.0f);
        setP (p.getMachine().apvts, "pianist", 0.0f);
        setP (p.getMachine().apvts, "pianoFill", 0.0f);
        rk->setPage (0);
        rk->setPage (1);
        save ("interface-chord-machine-bang-bang.png");
    }

    std::printf ("\n%s\n", failures == 0 ? "TUDO OK" : "HA FALHAS");
    return failures == 0 ? 0 : 1;
}
