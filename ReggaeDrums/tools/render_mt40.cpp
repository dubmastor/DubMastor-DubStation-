// Gera os timbres de fábrica do kit SLENG TENG (Casio MT-40) do Battery a partir do KIT DIGITAL do MRDM-12:
// o mesmo sintetizador (ReggaeDrumMachine/Source/DrumSynth.h), uma peça por WAV mono 16 bits 44,1 kHz.
// Uso (na pasta DubBatteryReggae):
//   g++ -std=c++17 -O2 -I../ReggaeDrumMachine/Source tools/render_mt40.cpp ../ReggaeDrumMachine/Source/PatternGenerator.cpp -o /tmp/render_mt40
//   /tmp/render_mt40 Samples
#include "DrumSynth.h"
#include <cstdio>
#include <string>

static void writeWav (const std::string& path, const std::vector<float>& x, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) return;
    const uint32_t n = (uint32_t) x.size(), bytes = n * 2;
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16); u16 (1); u16 (1); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 2); u16 (2); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (bytes);
    for (float s : x) { const int16_t v = (int16_t) std::lround (std::clamp (s, -1.0f, 1.0f) * 32767.0f); std::fwrite (&v, 2, 1, f); }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : ".";
    const int sr = 44100;
    struct Piece { int note; const char* file; };
    const Piece pieces[] = {
        { dubdrums::gm::Kick,      "MT40-BUMBO.wav" },  { dubdrums::gm::SideStick, "MT40-ARO.wav" },
        { dubdrums::gm::Snare,     "MT40-CAIXA.wav" },  { dubdrums::gm::ClosedHat, "MT40-HH-FECHADO.wav" },
        { dubdrums::gm::OpenHat,   "MT40-HH-ABERTO.wav" }, { dubdrums::gm::TomLow, "MT40-TOM-GRAVE.wav" },
        { dubdrums::gm::TomMid,    "MT40-TOM.wav" },    { dubdrums::gm::TomHigh,   "MT40-TOM-AGUDO.wav" },
        { dubdrums::gm::Crash,     "MT40-PRATO.wav" } };
    for (const auto& p : pieces)
    {
        dubdrums::DrumSynth synth;
        synth.setKit (dubdrums::DrumSynth::Kit::Digital);
        synth.prepare (sr);
        synth.trigger (p.note, 120);
        std::vector<float> l ((size_t) sr * 2, 0.0f);
        synth.render (l.data(), nullptr, 0, (int) l.size(), 1.0f);
        // corta o silêncio do fim e normaliza em -1 dB
        float peak = 0.0f; size_t last = 0;
        for (size_t i = 0; i < l.size(); ++i) { peak = std::max (peak, std::abs (l[i])); if (std::abs (l[i]) > 1.0e-4f) last = i; }
        l.resize (std::max<size_t> (last + 64, 256));
        const float g = peak > 0.0f ? 0.89f / peak : 1.0f;
        for (auto& s : l) s *= g;
        for (size_t i = 0; i < 64 && i < l.size(); ++i) l[l.size() - 1 - i] *= (float) i / 64.0f;   // fim sem estalo
        writeWav (dir + "/" + p.file, l, sr);
        std::printf ("%s: %.3f s\n", p.file, l.size() / (double) sr);
    }
    return 0;
}
