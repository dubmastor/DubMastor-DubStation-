// DUB BATTERY REGGAE — os 16 pads, os parâmetros de cada pad e os presets de fábrica.
// Notas dos pads: 36..51 em sequência (pad 1 = C1 36, embaixo à esquerda, como na MPC). Assim todas as
// notas GM do Mastor Reggae Drum Machine MRDM-12 (bumbo 36, aro 37, caixa 38, HH fechado 42, surdo 43,
// HH aberto 46, tom 2 47, prato 49, tom 1 50) caem no pad certo, sem remapear nada.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace dbr
{
inline constexpr int numPads   = 16;
inline constexpr int firstNote = 36;

struct PadInfo { const char* name; int chokeGroup; };   // chokeGroup: 0 = nenhum, 1 = hi-hat, 2 = A, 3 = B

inline const PadInfo pads[numPads] = {
    { "BUMBO 1",    0 },   // 36
    { "ARO",        0 },   // 37
    { "CAIXA 1",    0 },   // 38
    { "CAIXA 2",    0 },   // 39
    { "CAIXA 3",    0 },   // 40
    { "BUMBO 2",    0 },   // 41
    { "HH FECHADO", 1 },   // 42
    { "SURDO",      0 },   // 43
    { "HH MEIO",    1 },   // 44
    { "BUMBO 3",    0 },   // 45
    { "HH ABERTO",  1 },   // 46
    { "TOM 2",      0 },   // 47
    { "SHAKER",     0 },   // 48
    { "PRATO",      0 },   // 49
    { "TOM 1",      0 },   // 50
    { "BLOCO",      0 },   // 51
};

inline juce::String noteName (int n)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[n % 12]) + juce::String (n / 12 - 2);   // C1 = 36 (padrão Cubase / GM do MRDM-12)
}

// ids dos parâmetros de cada pad: "p01_level" ... "p16_choke"
// link = JUNTAR COM outro pad (0 = nenhum, k = pad k); layer = TIMBRE 2 (0 = nenhum, k = timbre k-1 da biblioteca)
enum class P { level, pan, tune, start, end, attack, hold, decay, sustain, release, cutoff, velocity, reverse, mode, choke,
               link, layer, layerLevel, layerTune, count };
inline const char* paramSuffix (P p)
{
    static const char* s[] = { "level", "pan", "tune", "start", "end", "attack", "hold", "decay", "sustain",
                               "release", "cutoff", "velocity", "reverse", "mode", "choke",
                               "link", "layer", "layerLevel", "layerTune" };
    return s[(int) p];
}
inline juce::String pid (int pad, P p)
{
    return "p" + juce::String (pad + 1).paddedLeft ('0', 2) + "_" + paramSuffix (p);
}

inline const juce::StringArray chokeNames { "NENHUM", "HI-HAT", "GRUPO A", "GRUPO B" };
inline const juce::StringArray modeNames  { "ONE SHOT", "GATE" };

//==============================================================================
// Presets de fábrica: cada um mexe nos valores padrão de alguns pads (o resto fica no padrão)
struct FactoryPreset
{
    const char* name;
    std::function<void (std::function<void (int pad, P, float)>)> apply;
    std::vector<std::pair<int, int>> samples {};   // timbre de fábrica por pad (pad, índice na biblioteca); o resto fica o do pad
};

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> list = {
        { "ROOTS DM", [] (auto) {} },
        { "STEPPERS", [] (auto set)
            {
                set (0, P::tune, -2.0f);  set (0, P::level, 2.0f);
                set (5, P::tune, -1.0f);
                set (2, P::sustain, 0.0f); set (2, P::decay, 180.0f);
                set (6, P::sustain, 0.0f); set (6, P::decay, 90.0f);
            } },
        { "ONE DROP ARO", [] (auto set)
            {
                set (1, P::level, 2.0f);   set (1, P::tune, 1.0f);
                set (0, P::tune, -3.0f);   set (0, P::cutoff, 2500.0f);
                set (10, P::sustain, 0.35f); set (10, P::decay, 300.0f);
            } },
        { "DIGITAL 85", [] (auto set)
            {
                for (int p = 0; p < numPads; ++p) { set (p, P::sustain, 0.0f); set (p, P::decay, 140.0f); set (p, P::cutoff, 7000.0f); }
                set (9, P::level, 2.0f);   set (4, P::level, 1.0f);
                set (0, P::decay, 220.0f); set (13, P::decay, 600.0f);
            } },
        { "DUB LAYER", [] (auto set)
            {
                // caixa 1 com o aro junto (TIMBRE 2), bumbo 1 com o sub do bumbo 2 por baixo,
                // e o HH aberto puxando o prato junto (mais baixo)
                set (2, P::layer, 2.0f);  set (2, P::layerLevel, -7.0f); set (2, P::layerTune, 0.5f);
                set (0, P::layer, 6.0f);  set (0, P::layerLevel, -5.0f); set (0, P::layerTune, -2.0f);
                set (10, P::link, 14.0f); set (13, P::level, -9.0f);
            } },
        { "DUB PESADO", [] (auto set)
            {
                for (int p = 0; p < numPads; ++p) set (p, P::tune, -2.0f);
                set (0, P::tune, -4.0f);  set (0, P::cutoff, 1800.0f); set (0, P::level, 3.0f);
                set (2, P::cutoff, 4500.0f); set (2, P::release, 900.0f);
                set (13, P::cutoff, 6000.0f);
            } },
        // SLENG TENG (1985): o Casio MT-40 do KIT DIGITAL do MRDM-12 nos pads, curto e seco como a máquina
        { "SLENG TENG (CASIO MT-40)", [] (auto set)
            {
                for (int p = 0; p < numPads; ++p) { set (p, P::sustain, 0.0f); set (p, P::cutoff, 12000.0f); }
                set (13, P::level, -6.0f);
            },
            { { 0, 34 }, { 5, 34 }, { 9, 34 },              // bumbos
              { 1, 35 },                                       // aro
              { 2, 36 }, { 3, 36 }, { 4, 36 },                 // caixas
              { 6, 37 }, { 8, 37 }, { 10, 38 },                // hi-hats
              { 7, 39 }, { 11, 40 }, { 14, 41 },               // surdo e tons
              { 13, 42 } } },                                  // prato
    };
    return list;
}
} // namespace dbr
