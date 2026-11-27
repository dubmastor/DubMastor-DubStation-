#pragma once
// Próximas harmonias sugeridas, conforme as cadências do reggae.
// Conta as passagens de acorde das músicas do banco de riddims (só as com harmonia levantada, harmony < 2),
// sempre em graus da tonalidade do ACORDE 1 (a tônica). Passagem com pouca música (< 3) usa a tabela de teoria.
// Cópia em JavaScript na versão para celular do MRK-80 (site/keys/index.html, função cadenceHints): mudou um, mude o outro.
#include <algorithm>
#include <string>
#include <vector>
#include "../../RiddimBank/Source/RiddimBank.h"

namespace cadence
{
struct Hint { int root, minor, rank; };   // tônica absoluta 0..11, 1 = menor, rank 1 = a mais comum

inline const char* const kDegree[12] = { "I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII" };

// grau (0..11) + qualidade → código 0..23
inline int code (int interval, int minor) { return ((interval % 12 + 12) % 12) * 2 + (minor ? 1 : 0); }

// tabela de teoria: depois do grau X (em maior ou menor), as saídas clássicas do reggae, em ordem
struct Rule { int fromMinorKey; int from; std::vector<int> to; };
inline const std::vector<Rule>& rules()
{
    static const std::vector<Rule> r = {
        // tonalidade maior
        { 0, code (0, 0),  { code (5, 0), code (7, 0), code (2, 1), code (9, 1) } },    // I    → IV V IIm VIm
        { 0, code (5, 0),  { code (7, 0), code (0, 0), code (2, 1) } },                 // IV   → V I IIm
        { 0, code (7, 0),  { code (5, 0), code (0, 0), code (9, 1) } },                 // V    → IV I VIm
        { 0, code (2, 1),  { code (7, 0), code (5, 0) } },                              // IIm  → V IV
        { 0, code (9, 1),  { code (5, 0), code (2, 1), code (7, 0) } },                 // VIm  → IV IIm V
        { 0, code (4, 1),  { code (5, 0), code (9, 1) } },                              // IIIm → IV VIm
        { 0, code (10, 0), { code (5, 0), code (0, 0) } },                              // bVII → IV I
        // tonalidade menor
        { 1, code (0, 1),  { code (10, 0), code (5, 1), code (7, 1), code (8, 0) } },   // Im   → bVII IVm Vm bVI
        { 1, code (5, 1),  { code (0, 1), code (10, 0), code (7, 1) } },                // IVm  → Im bVII Vm
        { 1, code (7, 1),  { code (5, 1), code (0, 1) } },                              // Vm   → IVm Im
        { 1, code (8, 0),  { code (10, 0), code (7, 1) } },                             // bVI  → bVII Vm
        { 1, code (10, 0), { code (0, 1), code (8, 0), code (5, 1) } },                 // bVII → Im bVI IVm
        { 1, code (3, 0),  { code (5, 1), code (10, 0) } },                             // bIII → IVm bVII
    };
    return r;
}

// tonic/tonicMinor = ACORDE 1; prev/prevMinor = o acorde ligado logo antes do cartão. Devolve até 3 sugestões.
inline std::vector<Hint> suggest (int tonic, int tonicMinor, int prev, int prevMinor, int maxHints = 3)
{
    const int from = code (prev - tonic, prevMinor);
    int count[24] = {}, total = 0;
    for (int i = 0; i < riddimbank::numEntries; ++i)
    {
        const auto& e = riddimbank::entries[i];
        if (e.harmony >= 2 || e.numChords < 2 || (e.chords[0].minor != 0) != (tonicMinor != 0)) continue;
        const int t = e.chords[0].root;
        for (int k = 1; k < e.numChords; ++k)
            if (code (e.chords[k - 1].root - t, e.chords[k - 1].minor) == from)
            {
                ++count[code (e.chords[k].root - t, e.chords[k].minor)];
                ++total;
            }
    }

    // ordem da teoria (desempate e reserva quando o banco tem pouca música dessa passagem)
    int theory[24];
    std::fill (std::begin (theory), std::end (theory), 99);
    for (const auto& r : rules())
        if (r.fromMinorKey == (tonicMinor ? 1 : 0) && r.from == from)
            for (size_t k = 0; k < r.to.size(); ++k) theory[r.to[k]] = (int) k;

    std::vector<int> order;
    for (int c = 0; c < 24; ++c)
        if (c != from && ((total >= 3 && count[c] > 0) || theory[c] < 99)) order.push_back (c);
    std::stable_sort (order.begin(), order.end(), [&] (int a, int b)
    {
        const int ca = total >= 3 ? count[a] : 0, cb = total >= 3 ? count[b] : 0;
        if (ca != cb) return ca > cb;
        return theory[a] < theory[b];
    });

    std::vector<Hint> out;
    for (int c : order)
    {
        const int root = (tonic + c / 2) % 12;
        if (std::any_of (out.begin(), out.end(), [&] (const Hint& h) { return h.root == root; })) continue;   // uma por tecla
        out.push_back ({ root, c % 2, (int) out.size() + 1 });
        if ((int) out.size() >= maxHints) break;
    }
    return out;
}

// nome do grau em relação à tônica: "IV", "IIm", "bVII"
inline std::string degreeName (int tonic, int root, int minor)
{
    return std::string (kDegree[((root - tonic) % 12 + 12) % 12]) + (minor ? "m" : "");
}
}
