// MENU DO BANCO ÚNICO DE RIDDIMS — o mesmo em todas as machines (MRCM-1240, MRBM-69, MRDM-12, MRM-0420).
// Três caminhos: ESTILOS (10 estilos de reggae), RIDDIMS A–Z (cada riddim com as músicas gravadas nele)
// e MÚSICAS A–Z. Cada item mostra as luzes D B R (bateria, baixo e riddim/harmonia): acesa = a parte veio da gravação original.
// FILTRO DO INSTRUMENTO: o alvo do CMake define RIDDIMBANK_REQUIRE com as partes que a música precisa ter da gravação
// (bits de Entry::orig: 1 = R riddim/harmonia, 2 = B baixo, 4 = D bateria). Reggae Drums MRD-12 = 4, Reggae Bass MRB-69 = 2,
// Reggae Keys MRK-80 e Reggae Organ MRO-68 = 1; sem o define (MRM-0420 e as machines separadas) o menu mostra todas.
// Precisa do JUCE (juce_gui_basics) incluído antes.
#pragma once
#include "RiddimBank.h"

namespace riddimbank
{
inline constexpr int menuIdBase = 20000;   // id do item no PopupMenu = menuIdBase + índice da entrada
inline constexpr int toggleCompleteId = menuIdBase - 2;   // item "só completos" (o menu abre de novo)
inline bool& onlyComplete() { static bool b = false; return b; }   // vale para todas as machines abertas
#ifndef RIDDIMBANK_REQUIRE
 #define RIDDIMBANK_REQUIRE 0
#endif
inline constexpr int required = RIDDIMBANK_REQUIRE;

inline juce::String noteName (int r)
{
    static const char* const n[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return n[(r % 12 + 12) % 12];
}

inline juce::String chordsText (const Entry& e)
{
    if (e.numChords <= 0) return juce::String::fromUTF8 ("harmonia pendente");
    juce::StringArray ch;
    for (int k = 0; k < e.numChords; ++k) ch.add (noteName (e.chords[k].root) + (e.chords[k].minor ? "m" : ""));
    return ch.joinIntoString ("-");
}

inline juce::String lights (const Entry& e)
{
    auto led = [] (bool on, const char* l) { return juce::String::fromUTF8 (on ? "\xe2\x97\x8f" : "\xe2\x97\x8b") + l; };
    return led (e.orig & 4, "D") + " " + led (e.orig & 2, "B") + " " + led (e.orig & 1, "R");
}

inline juce::String title (const Entry& e)
{
    juce::String s = juce::String::fromUTF8 (e.name);
    if (*e.artist) s << juce::String::fromUTF8 (" \xe2\x80\x94 ") << juce::String::fromUTF8 (e.artist);
    if (e.year > 0) s << " (" << e.year << ")";
    return s;
}

inline juce::String itemText (const Entry& e)
{
    return title (e) + "    " + chordsText (e) + (e.harmony == 1 ? " ~" : "") + "  \xc2\xb7  "
         + juce::String (e.bpm) + " BPM    " + lights (e);
}

// CRÉDITOS no visor: a linha de baixo alterna a cada 4 s entre os músicos da máquina e os créditos da gravação
inline int creditPhase() { return (int) ((juce::Time::getMillisecondCounter() / 4000u) % 2u); }
inline bool showCredits (const Entry& e) { return *e.credits && creditPhase() == 1; }
inline juce::String creditsText (const Entry& e) { return juce::String::fromUTF8 (e.credits).toUpperCase(); }

inline bool usable (int i) { return i >= 0 && i < numEntries && entries[i].numChords > 0; }

// a música entra neste instrumento (filtro RIDDIMBANK_REQUIRE) e no "Só completos"
inline bool listed (int i, bool onlyComplete)
{
    const int o = entries[i].orig;
    return (o & required) == required && (! onlyComplete || o == 7);
}
// usável e dentro do filtro do instrumento (as setas do visor só passam por estas)
inline bool allowed (int i) { return usable (i) && listed (i, false); }

inline void addItem (juce::PopupMenu& m, int i, int current, bool onlyComplete)
{
    const auto& e = entries[i];
    if (! listed (i, onlyComplete)) return;
    m.addItem (menuIdBase + i, itemText (e), usable (i), i == current);
}

// ESTILOS ▸ RIDDIMS A–Z ▸ MÚSICAS A–Z (ticks no caminho da entrada atual)
inline void addMenus (juce::PopupMenu& menu, int current)
{
    const bool onlyComplete = riddimbank::onlyComplete();
    const auto star = juce::String::fromUTF8 ("\xc2\xb7");
    int listedSongs = 0;
    for (int i = 0; i < numEntries; ++i) listedSongs += listed (i, false);
    const char* filterName = required == 4 ? " COM A BATERIA DA GRAVA\xc3\x87\xc3\x83O" : required == 2 ? " COM O BAIXO DA GRAVA\xc3\x87\xc3\x83O"
                           : required == 1 ? " COM O RIDDIM DA GRAVA\xc3\x87\xc3\x83O" : "";
    menu.addSectionHeader (juce::String::fromUTF8 ("BANCO DE RIDDIMS  ") + star + "  " + juce::String (listedSongs)
                           + juce::String::fromUTF8 (" MÚSICAS") + juce::String::fromUTF8 (filterName)
                           + (required == 0 ? "  " + star + "  " + juce::String (numFamilies) + " RIDDIMS" : juce::String()));

    juce::PopupMenu byStyle;
    for (int s = 0; s < numStyles; ++s)
    {
        juce::PopupMenu sub;
        int n = 0;
        for (int i = 0; i < numEntries; ++i)
            if (entries[i].style == s && listed (i, onlyComplete)) { addItem (sub, i, current, onlyComplete); ++n; }
        byStyle.addSubMenu (juce::String (styles[s]) + "  " + styleYears[s] + "   (" + juce::String (n) + ")", sub, n > 0, nullptr,
                            current >= 0 && entries[current].style == s);
    }

    juce::PopupMenu byRiddim;
    for (char L = 'A'; L <= 'Z'; ++L)
    {
        juce::PopupMenu letter;
        bool here = false;
        int n = 0;
        for (int f = 0; f < numFamilies; ++f)
        {
            const auto& fam = families[f];
            if (fam.letter != L) continue;
            bool famHere = false;
            int shown = 0;
            for (int k = 0; k < fam.count; ++k)
            {
                famHere |= members[fam.first + k] == current;
                shown += listed (members[fam.first + k], onlyComplete);
            }
            if (shown == 0) continue;
            here |= famHere;
            ++n;
            const auto head = juce::String::fromUTF8 (fam.name) + " (" + juce::String (fam.year) + ")";
            if (fam.count == 1)
            {
                const int i = members[fam.first];   // (o filtro já foi conferido em "shown")
                letter.addItem (menuIdBase + i, head + "    " + title (entries[i]) + "    " + lights (entries[i]), usable (i), i == current);
            }
            else
            {
                juce::PopupMenu fm;
                for (int k = 0; k < fam.count; ++k)
                {
                    const int i = members[fam.first + k];
                    if (! listed (i, onlyComplete)) continue;
                    fm.addItem (menuIdBase + i, (k == 0 ? juce::String::fromUTF8 ("ORIGEM  ") : juce::String()) + itemText (entries[i]),
                                usable (i), i == current);
                }
                letter.addSubMenu (head + "   " + juce::String (shown) + juce::String::fromUTF8 (" MÚSICAS"), fm, true, nullptr, famHere);
            }
        }
        if (n > 0) byRiddim.addSubMenu (juce::String::charToString ((juce::juce_wchar) L) + "   (" + juce::String (n) + ")", letter, true, nullptr, here);
    }

    juce::PopupMenu bySong;
    for (char L = 'A'; L <= 'Z'; ++L)
    {
        juce::PopupMenu letter;
        bool here = false;
        int n = 0;
        for (int k = 0; k < numEntries; ++k)
        {
            const int i = songOrder[k];
            auto first = juce::String::fromUTF8 (entries[i].name).trimStart();
            if (first.startsWithIgnoreCase ("the ")) first = first.substring (4);
            const auto c = juce::CharacterFunctions::toUpperCase (first[0]);
            const bool match = (L == 'A' && ! juce::CharacterFunctions::isLetter (c)) || c == (juce::juce_wchar) L;
            if (! match) continue;
            if (! listed (i, onlyComplete)) continue;
            addItem (letter, i, current, onlyComplete);
            here |= i == current;
            ++n;
        }
        if (n > 0) bySong.addSubMenu (juce::String::charToString ((juce::juce_wchar) L) + "   (" + juce::String (n) + ")", letter, true, nullptr, here);
    }

    menu.addSubMenu ("ESTILOS", byStyle, true, nullptr, current >= 0);
    menu.addSubMenu (juce::String::fromUTF8 ("RIDDIMS A–Z"), byRiddim);
    menu.addSubMenu (juce::String::fromUTF8 ("MÚSICAS A–Z"), bySong);
    menu.addItem (toggleCompleteId, juce::String::fromUTF8 ("Só completos (bateria, baixo e riddim da gravação)"), true, onlyComplete);
    menu.addItem (-1, juce::String::fromUTF8 ("Luzes: D bateria · B baixo · R riddim (harmonia)  (\xe2\x97\x8f = da gravação, \xe2\x97\x8b = escolhida pelo músico mais próximo)"), false);
}

inline int entryFromMenuResult (int result)
{
    const int i = result - menuIdBase;
    return usable (i) ? i : -1;
}
} // namespace riddimbank
