// Teste offline do Mastor Reggae Percussion MRP-77 (sem JUCE):
//   g++ -std=c++17 -O2 -I Source tests/render_test.cpp -o /tmp/mrp_test && /tmp/mrp_test
// Confere: os 23 sons tocam e acabam (sem voz presa), sem estouro, Tape Echo e saturador em bypass de fábrica,
// os grooves na contagem (caixa no 2 e no 4), as notas fora da faixa do MRD-12 (36–51) e o MIDI colorido por compasso.
#include "PercEngine.h"
#include <cstdio>
#include <set>

static int fails = 0;
#define CHECK(c, msg) do { if (c) std::printf ("ok     %s\n", msg); else { std::printf ("FALHOU %s\n", msg); ++fails; } } while (0)

int main()
{
    const int sr = 48000;
    char m[200];
    // ---- cada som: toca, não estoura e termina
    for (int s = 0; s < mrp::numSounds; ++s)
    {
        mrp::Synth syn; syn.prepare ((float) sr);
        syn.hit (s, 1.0f, true, 0.4f, 0.8f, 0.0f, 0.2f);
        std::vector<float> L ((size_t) sr * 3, 0.0f), R ((size_t) sr * 3, 0.0f);
        for (int pos = 0; pos < sr * 3; pos += 256) syn.render (L.data() + pos, R.data() + pos, 256);
        float pk = 0; double e = 0;
        for (int i = 0; i < sr; ++i) { pk = std::max (pk, std::abs (L[(size_t) i])); e += L[(size_t) i] * L[(size_t) i]; }
        std::snprintf (m, sizeof m, "%-10s %-14s toca (pico %.2f) e termina", mrp::sounds()[(size_t) s].name, mrp::sounds()[(size_t) s].sub, pk);
        CHECK (pk > 0.01f && pk < 1.5f && e > 1e-4 && syn.activeVoices() == 0, m);
    }
    // ---- efeitos de fábrica
    mrp::FxSettings fs;
    CHECK (! fs.satOn && ! fs.echoOn, "padrão de fábrica: saturador e Tape Echo em bypass");
    // ---- cadeia de efeitos não estoura com eco no máximo
    {
        mrp::Synth syn; syn.prepare ((float) sr); mrp::FxChain fx; fx.prepare ((float) sr);
        fs.satOn = true; fs.echoOn = true; fs.echoFb = 0.85f; fs.echoMix = 1.0f; fs.drive = 1.0f;
        float pk = 0;
        std::vector<float> L (256), R (256);
        for (int b = 0; b < sr * 4 / 256; ++b)
        {
            std::fill (L.begin(), L.end(), 0.0f); std::fill (R.begin(), R.end(), 0.0f);
            if (b % 40 == 0) syn.hit (b % mrp::numSounds, 1.0f, true, 0.5f, 1.0f, 0.0f, 0.2f);
            syn.render (L.data(), R.data(), 256); fx.process (L.data(), R.data(), 256, fs);
            for (int i = 0; i < 256; ++i) pk = std::max (pk, std::abs (L[(size_t) i]));
        }
        std::snprintf (m, sizeof m, "eco no máximo + saturador: sem estouro (pico %.2f)", pk);
        CHECK (std::isfinite (pk) && pk < 4.0f, m);
    }
    // ---- pads e notas
    std::set<int> notes;
    bool drumFree = true;
    for (auto& p : mrp::pads()) { notes.insert (p.note); drumFree = drumFree && ! mrp::isDrumNote (p.note); }
    CHECK ((int) notes.size() == mrp::numPads, "24 pads, cada um com nota própria");
    CHECK (drumFree, "nenhuma nota na faixa do MRD-12 (36 a 51)");
    CHECK (mrp::pads()[(size_t) mrp::padOf (mrp::soundIndex ("gui"), true)].note == 74 && mrp::pads()[(size_t) mrp::padOf (mrp::soundIndex ("gui"), false)].note == 73, "guiro: curto 73, longo 74");
    // ---- grooves na contagem Mastor
    CHECK (mrp::isSnareStep (4) && mrp::isSnareStep (12) && ! mrp::isSnareStep (8), "caixa no 2 e no 4 (passos 4 e 12), nunca no 3");
    bool ok16 = true;
    for (auto& g : mrp::grooves()) for (auto* p : g.p) if (p && *p) ok16 = ok16 && std::char_traits<char>::length (p) == 16;
    CHECK (ok16, "todos os grooves com 16 semicolcheias por compasso");
    const auto& roots = mrp::grooves()[0];
    CHECK (mrp::stepValue (roots.p[0], 2) == 2 && mrp::stepValue (roots.p[0], 6) == 2, "Kit roots: shaker com acento no & (com a palheta)");
    // ---- MIDI colorido por compasso
    {
        std::array<std::array<uint8_t, 128>, mrp::numTracks> pat {};
        for (int b = 0; b < 8; ++b) for (int i = 0; i < 16; ++i) pat[0][(size_t) (b * 16 + i)] = (uint8_t) mrp::stepValue (roots.p[0], i);
        std::array<int, mrp::numTracks> snd {}; for (int t = 0; t < mrp::numTracks; ++t) snd[(size_t) t] = mrp::soundIndex (mrp::trackDefaults()[(size_t) t].soundKey);
        std::array<bool, mrp::numTracks> aud {}; aud.fill (true);
        std::array<int, mrp::numPads> pn {}; for (int i = 0; i < mrp::numPads; ++i) pn[(size_t) i] = mrp::pads()[(size_t) i].note;
        auto ns = mrp::buildNotes (pat, snd, aud, pn, 4, -1, -1, true, 10, true, 0.58f);
        std::set<int> chs; for (auto& n : ns) chs.insert (n.ch);
        CHECK (ns.size() == 64 && chs.size() == 4 && *chs.begin() == 1, "MIDI colorido: 4 compassos = canais 1 a 4, 64 notas");
        auto one = mrp::buildNotes (pat, snd, aud, pn, 4, 2, -1, true, 10, true, 0.58f);
        CHECK (one.size() == 16 && one.front().ch == 3 && one.front().tick == 0, "um compasso só, na cor dele (C3 = canal 3, começa no 0)");
        auto plain = mrp::buildNotes (pat, snd, aud, pn, 2, -1, -1, false, 10, false, 0.58f);
        bool all10 = true, noOverlap = true;
        for (auto& n : plain) all10 = all10 && n.ch == 10;
        for (size_t i = 1; i < ns.size(); ++i) if (ns[i].note == ns[i - 1].note && ns[i - 1].tick + ns[i - 1].len > ns[i].tick) noOverlap = false;
        CHECK (all10, "MIDI sem cor: tudo no canal padrão (10)");
        CHECK (noOverlap, "com swing, nenhuma nota passa por cima da seguinte");
        bool shakerNote = true; for (auto& n : ns) shakerNote = shakerNote && n.note == 82;
        CHECK (shakerNote, "shaker egg sai na nota 82 (GM)");
    }
    std::printf (fails == 0 ? "\nTUDO OK\n" : "\n%d FALHA(S)\n", fails);
    return fails == 0 ? 0 : 1;
}
