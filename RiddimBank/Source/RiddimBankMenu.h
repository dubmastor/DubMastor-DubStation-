// MENU DO BANCO ÚNICO DE RIDDIMS — o mesmo em todas as machines (MRCM-1240, MRBM-69, MRDM-12, MRM-0420).
// Três caminhos: ESTILOS (10 estilos de reggae), RIDDIMS A–Z (cada riddim com as músicas gravadas nele)
// e MÚSICAS A–Z. Cada item mostra as luzes H B D: acesa = a parte veio da gravação original.
// Precisa do JUCE (juce_gui_basics) incluído antes.
#pragma once
#include <map>
#include <memory>
#include "RiddimBank.h"

namespace riddimbank
{
inline constexpr int menuIdBase = 20000;   // id do item no PopupMenu = menuIdBase + índice da entrada
inline constexpr int toggleCompleteId = menuIdBase - 2;   // item "só completos" (o menu abre de novo)
inline bool& onlyComplete() { static bool b = false; return b; }   // vale para todas as machines abertas

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
    return led (e.orig & 1, "H") + " " + led (e.orig & 2, "B") + " " + led (e.orig & 4, "D");
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

inline void addItem (juce::PopupMenu& m, int i, int current, bool onlyComplete)
{
    const auto& e = entries[i];
    if (onlyComplete && e.orig != 7) return;
    m.addItem (menuIdBase + i, itemText (e), usable (i), i == current);
}

// ESTILOS ▸ RIDDIMS A–Z ▸ MÚSICAS A–Z (ticks no caminho da entrada atual)
inline void addMenus (juce::PopupMenu& menu, int current)
{
    const bool onlyComplete = riddimbank::onlyComplete();
    const auto star = juce::String::fromUTF8 ("\xc2\xb7");
    menu.addSectionHeader (juce::String::fromUTF8 ("BANCO DE RIDDIMS  ") + star + "  " + juce::String (numEntries)
                           + juce::String::fromUTF8 (" MÚSICAS  ") + star + "  " + juce::String (numFamilies) + " RIDDIMS");

    juce::PopupMenu byStyle;
    for (int s = 0; s < numStyles; ++s)
    {
        juce::PopupMenu sub;
        int n = 0;
        for (int i = 0; i < numEntries; ++i)
            if (entries[i].style == s && (! onlyComplete || entries[i].orig == 7)) { addItem (sub, i, current, onlyComplete); ++n; }
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
                shown += ! onlyComplete || entries[members[fam.first + k]].orig == 7;
            }
            if (shown == 0) continue;
            here |= famHere;
            ++n;
            const auto head = juce::String::fromUTF8 (fam.name) + " (" + juce::String (fam.year) + ")";
            if (fam.count == 1)
            {
                const int i = members[fam.first];
                letter.addItem (menuIdBase + i, head + "    " + title (entries[i]) + "    " + lights (entries[i]), usable (i), i == current);
            }
            else
            {
                juce::PopupMenu fm;
                for (int k = 0; k < fam.count; ++k)
                {
                    const int i = members[fam.first + k];
                    if (onlyComplete && entries[i].orig != 7) continue;
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
            if (onlyComplete && entries[i].orig != 7) continue;
            addItem (letter, i, current, onlyComplete);
            here |= i == current;
            ++n;
        }
        if (n > 0) bySong.addSubMenu (juce::String::charToString ((juce::juce_wchar) L) + "   (" + juce::String (n) + ")", letter, true, nullptr, here);
    }

    menu.addSubMenu ("ESTILOS", byStyle, true, nullptr, current >= 0);
    menu.addSubMenu (juce::String::fromUTF8 ("RIDDIMS A–Z"), byRiddim);
    menu.addSubMenu (juce::String::fromUTF8 ("MÚSICAS A–Z"), bySong);
    menu.addItem (toggleCompleteId, juce::String::fromUTF8 ("Só completos (harmonia, baixo e bateria da gravação)"), true, onlyComplete);
    menu.addItem (-1, juce::String::fromUTF8 ("Luzes: H harmonia · B baixo · D bateria  (\xe2\x97\x8f = da gravação, \xe2\x97\x8b = escolhida pelo músico mais próximo)"), false);
}

// ---------------------------------------------------------------- MEGA BANCO (padrão da Riddim Machine MRM-0420)
// ESTILOS ▸ riddim ▸ música · RIDDIMS A–Z · MÚSICAS A–Z · ★ FAVORITOS. Só entram as músicas com a harmonia levantada
// (conferida ou aproximada). Os favoritos ficam guardados no computador (DubMastor/<app>.settings), valem para todos os projetos.
inline constexpr int favToggleId = menuIdBase - 3;   // item "☆ favoritar a música atual"
inline bool hasHarmony (int i) { return usable (i) && entries[i].harmony < 2; }

inline juce::PropertiesFile& favFile (const juce::String& app)
{
    static std::map<juce::String, std::unique_ptr<juce::PropertiesFile>> files;
    auto& f = files[app];
    if (! f)
    {
        juce::PropertiesFile::Options o;
        o.applicationName = app; o.folderName = "DubMastor"; o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
        f = std::make_unique<juce::PropertiesFile> (o);
    }
    return *f;
}
inline juce::StringArray favorites (const juce::String& app)
{
    return juce::StringArray::fromTokens (favFile (app).getValue ("favoritos"), ",", "");
}
inline bool isFavorite (const juce::String& app, int i) { return i >= 0 && favorites (app).contains (entries[i].id); }
inline void toggleFavorite (const juce::String& app, int i)
{
    if (! usable (i)) return;
    auto f = favorites (app);
    if (f.contains (entries[i].id)) f.removeString (entries[i].id); else f.add (entries[i].id);
    f.removeEmptyStrings();
    favFile (app).setValue ("favoritos", f.joinIntoString (","));
    favFile (app).saveIfNeeded();
}

inline void addMegaMenus (juce::PopupMenu& menu, int current, const juce::String& app)
{
    const auto dot = juce::String::fromUTF8 ("  \xc2\xb7  ");
    int total = 0;
    for (int i = 0; i < numEntries; ++i) total += hasHarmony (i);
    menu.addSectionHeader (juce::String::fromUTF8 ("PRESETS ") + dot + juce::String::fromUTF8 ("MEGA BANCO") + dot + juce::String (total)
                           + juce::String::fromUTF8 (" MÚSICAS COM HARMONIA"));
    const auto fav = favorites (app);
    auto item = [&] (juce::PopupMenu& m, int i, const juce::String& prefix = {})
    {
        const auto& e = entries[i];
        m.addItem (menuIdBase + i, prefix + (fav.contains (e.id) ? juce::String::fromUTF8 ("\xe2\x98\x85 ") : juce::String())
                                    + title (e) + "    " + chordsText (e) + "  " + dot.trim() + "  " + juce::String (e.bpm) + " BPM    "
                                    + juce::String::fromUTF8 (e.harmony == 0 ? "\xe2\x97\x8f conferida" : "\xe2\x97\x90 aproximada"),
                   true, i == current);
    };

    // ESTILOS ▸ riddim ▸ música
    juce::PopupMenu byStyle;
    for (int s = 0; s < numStyles; ++s)
    {
        juce::PopupMenu sub;
        int n = 0;
        for (int f = 0; f < numFamilies; ++f)
        {
            if (families[f].style != s) continue;
            juce::Array<int> list;
            for (int k = 0; k < families[f].count; ++k) if (hasHarmony (members[families[f].first + k])) list.add (members[families[f].first + k]);
            if (list.isEmpty()) continue;
            if (list.size() == 1) item (sub, list[0]);
            else
            {
                juce::PopupMenu fm;
                bool here = false;
                for (int i : list) { item (fm, i); here |= i == current; }
                sub.addSubMenu (juce::String::fromUTF8 (families[f].name) + "   (" + juce::String (list.size()) + juce::String::fromUTF8 (" MÚSICAS)"), fm, true, nullptr, here);
            }
            n += list.size();
        }
        if (n > 0) byStyle.addSubMenu (juce::String (styles[s]) + "  " + styleYears[s] + "   (" + juce::String (n) + ")", sub, true, nullptr,
                                       current >= 0 && entries[current].style == s);
    }

    // RIDDIMS A–Z
    juce::PopupMenu byRiddim;
    for (char L = 'A'; L <= 'Z'; ++L)
    {
        juce::PopupMenu letter;
        int n = 0; bool here = false;
        for (int f = 0; f < numFamilies; ++f)
        {
            const auto& fam = families[f];
            if (fam.letter != L) continue;
            juce::Array<int> list;
            for (int k = 0; k < fam.count; ++k) if (hasHarmony (members[fam.first + k])) list.add (members[fam.first + k]);
            if (list.isEmpty()) continue;
            ++n;
            juce::PopupMenu fm;
            bool famHere = false;
            for (int i : list) { item (fm, i); famHere |= i == current; }
            here |= famHere;
            letter.addSubMenu (juce::String::fromUTF8 (fam.name) + " (" + juce::String (fam.year) + ")   " + juce::String (list.size()), fm, true, nullptr, famHere);
        }
        if (n > 0) byRiddim.addSubMenu (juce::String::charToString ((juce::juce_wchar) L) + "   (" + juce::String (n) + ")", letter, true, nullptr, here);
    }

    // MÚSICAS A–Z
    juce::PopupMenu bySong;
    for (char L = 'A'; L <= 'Z'; ++L)
    {
        juce::PopupMenu letter;
        int n = 0; bool here = false;
        for (int k = 0; k < numEntries; ++k)
        {
            const int i = songOrder[k];
            if (! hasHarmony (i)) continue;
            auto first = juce::String::fromUTF8 (entries[i].name).trimStart();
            if (first.startsWithIgnoreCase ("the ")) first = first.substring (4);
            const auto c = juce::CharacterFunctions::toUpperCase (first[0]);
            if (! ((L == 'A' && ! juce::CharacterFunctions::isLetter (c)) || c == (juce::juce_wchar) L)) continue;
            item (letter, i); here |= i == current; ++n;
        }
        if (n > 0) bySong.addSubMenu (juce::String::charToString ((juce::juce_wchar) L) + "   (" + juce::String (n) + ")", letter, true, nullptr, here);
    }

    // ★ FAVORITOS
    juce::PopupMenu favs;
    int nf = 0;
    for (int k = 0; k < numEntries; ++k) { const int i = songOrder[k]; if (hasHarmony (i) && fav.contains (entries[i].id)) { item (favs, i); ++nf; } }
    if (nf == 0) favs.addItem (-1, juce::String::fromUTF8 ("Nenhum favorito ainda: escolha uma música e use \xe2\x98\x86 FAVORITAR"), false);

    menu.addSubMenu ("ESTILOS", byStyle, true, nullptr, current >= 0);
    menu.addSubMenu (juce::String::fromUTF8 ("RIDDIMS A–Z"), byRiddim);
    menu.addSubMenu (juce::String::fromUTF8 ("MÚSICAS A–Z"), bySong);
    menu.addSubMenu (juce::String::fromUTF8 ("\xe2\x98\x85 FAVORITOS   (") + juce::String (nf) + ")", favs);
    menu.addSeparator();
    if (current >= 0 && usable (current))
        menu.addItem (favToggleId, (isFavorite (app, current) ? juce::String::fromUTF8 ("\xe2\x98\x85 Tirar dos favoritos: ") : juce::String::fromUTF8 ("\xe2\x98\x86 FAVORITAR: "))
                                       + juce::String::fromUTF8 (entries[current].name));
    menu.addItem (-1, juce::String::fromUTF8 ("\xe2\x97\x8f harmonia conferida  \xc2\xb7  \xe2\x97\x90 aproximada  \xc2\xb7  só entram riddims com a harmonia levantada"), false);
}

inline int entryFromMenuResult (int result)
{
    const int i = result - menuIdBase;
    return usable (i) ? i : -1;
}
} // namespace riddimbank
