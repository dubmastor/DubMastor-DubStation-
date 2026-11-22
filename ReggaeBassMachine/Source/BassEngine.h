// Mastor Reggae Bass Machine MRBM-69 — motor de linhas de baixo (C++ puro, sem JUCE, testável isoladamente)
//
// Três escolas de baixo jamaicano, cada uma com células rítmicas próprias:
//
//  FAMILY MAN  (Aston "Family Man" Barrett, Wailers)  — one drop: o tempo 1 fica vazio, a linha é
//              melódica (terça, quinta, sexta, oitava), colcheias com swing e tercinas "puxadas para
//              trás" no fim da frase. Cada compasso pode trazer uma resposta diferente.
//  FLABBA      (Errol "Flabba" Holt, Roots Radics)    — rub-a-dub do começo dos anos 80: som redondo e
//              grave, notas longas, a fundamental no 1, quinta e sétima abaixo da tônica. Um riff curto
//              que se repete hipnoticamente, quase sem variação.
//  DERRICK BARNETT (Sagittarius Band)                 — dancehall ao vivo dos anos 80/90: ataque curto,
//              síncope 3-3-2, oitavas "estaladas", corridas de semicolcheias nas viradas.
//  ROBBIE      (Robbie Shakespeare, Sly & Robbie)    — rockers da Channel One: sincopado, firme e quase
//              mecânico, com o balanço do funk e do soul; fundamental no 1, oitavas e cromatismos.
//  LEROY SIBBLES (Heptones, Studio One)              — rocksteady/roots melódico: notas esticadas em linhas
//              cantadas, frases que descem pela escala e a nota sempre um pouco atrás do tempo.
//  FULLY FULLWOOD (Soul Syndicate)                   — riffs de riddim dos anos 70: gancho melódico de dois
//              compassos com saltos de oitava e anacruses de semicolcheia, repetido como assinatura.
//  JAMMYS      (King Jammy, Sleng Teng, 1985)        — o primeiro riddim digital: riff curto de teclado
//              Casio em semicolcheias, staccato, menor, repetido sem parar; pede o timbre SLENG TENG.
//
// A harmonia vem de até 4 acordes (tônica, tipo e duração). As notas de passagem e de aproximação
// usam o acorde seguinte, então a linha "chama" a próxima tônica.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace mbm
{
//==============================================================================
// Estilos
enum Style { FamilyMan, Flabba, DerrickBarnett, Robbie, LeroySibbles, FullyFullwood, Jammys, NumStyles };

inline const char* styleName (int s)
{
    static const char* n[NumStyles] = { "FAMILY MAN", "FLABBA", "DERRICK BARNETT", "ROBBIE", "LEROY SIBBLES", "FULLY FULLWOOD", "JAMMYS" };
    return n[std::clamp (s, 0, NumStyles - 1)];
}

inline const char* styleTagline (int s)
{
    static const char* n[NumStyles] = { "ONE DROP MELODICO", "RUB-A-DUB HIPNOTICO", "DANCEHALL AO VIVO", "ROCKERS DA CHANNEL ONE",
                                         "STUDIO ONE CANTADO", "RIFF DE RIDDIM 70s", "SLENG TENG DIGITAL 85" };
    return n[std::clamp (s, 0, NumStyles - 1)];
}

//==============================================================================
// Acordes
enum Quality { Maj, Min, Dom7, Min7, NumQualities };

inline const char* qualityName (int q)
{
    static const char* n[NumQualities] = { "", "m", "7", "m7" };
    return n[std::clamp (q, 0, NumQualities - 1)];
}

inline const char* qualityLongName (int q)
{
    static const char* n[NumQualities] = { "MAIOR", "MENOR", "SETIMA", "MENOR 7" };
    return n[std::clamp (q, 0, NumQualities - 1)];
}

inline const char* rootName (int r)
{
    static const char* n[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return n[((r % 12) + 12) % 12];
}

inline constexpr int numDurations = 6;
inline const char* durationName (int d)
{
    static const char* n[numDurations] = { "1/2 comp.", "1 comp.", "2 comp.", "3 comp.", "4 comp.", "8 comp." };
    return n[std::clamp (d, 0, numDurations - 1)];
}
inline double durationBeats (int d)
{
    static const double b[numDurations] = { 2, 4, 8, 12, 16, 32 };
    return b[std::clamp (d, 0, numDurations - 1)];
}

struct ChordSlot { int root = 9; int quality = Min; double beats = 8; };

inline std::string chordLabel (const ChordSlot& c) { return std::string (rootName (c.root)) + qualityName (c.quality); }

//==============================================================================
// Escala da edição: pentatônica maior (acordes maior e 7) ou menor (menor e m7) a partir da tônica
inline bool isMinorQuality (int q) { return q == Min || q == Min7; }

inline bool inPentatonic (int note, const ChordSlot& c)
{
    const int iv = ((note - c.root) % 12 + 12) % 12;
    return isMinorQuality (c.quality) ? (iv == 0 || iv == 3 || iv == 5 || iv == 7 || iv == 10)
                                      : (iv == 0 || iv == 2 || iv == 4 || iv == 7 || iv == 9);
}

// Acorde que soa no tempo 'beat' (só os acordes ligados, beats > 0)
inline ChordSlot chordAtBeat (const std::vector<ChordSlot>& slots, double beat)
{
    double x = 0;
    ChordSlot last {};
    bool any = false;
    for (const auto& c : slots)
    {
        if (c.beats <= 0) continue;
        last = c; any = true;
        if (beat < x + c.beats - 1e-9) return c;
        x += c.beats;
    }
    return any ? last : ChordSlot {};
}

// Nota da pentatônica mais próxima dentro de [lo, hi] (empate: a de baixo, ou na direção 'dir' se dado)
inline int snapToPentatonic (int note, const ChordSlot& c, int lo, int hi, int dir = 0)
{
    note = std::clamp (note, lo, hi);
    if (inPentatonic (note, c)) return note;
    for (int d = 1; d < 12; ++d)
    {
        const int a = note - d, b = note + d;
        const bool okA = a >= lo && inPentatonic (a, c), okB = b <= hi && inPentatonic (b, c);
        if (okA && okB) return dir > 0 ? b : a;
        if (okA) return a;
        if (okB) return b;
    }
    return note;
}

//==============================================================================
// Graus usados nas células (resolvidos pelo tipo do acorde)
enum Tok
{
    R,      // fundamental
    T3,     // terça (maior/menor conforme o acorde)
    T5,     // quinta
    T7,     // sétima (b7 em menor e em 7; 7M em maior)
    OCT,    // oitava acima
    S2,     // segunda (nota de passagem)
    S4,     // quarta
    S6,     // sexta
    LOW5,   // quinta abaixo da fundamental
    LOW7,   // sétima abaixo da fundamental
    LOW3,   // terça abaixo (décima grave)
    APP     // aproximação da próxima tônica (cromática ou diatônica)
};

inline int tokSemis (int tok, int q)
{
    const bool minor = (q == Min || q == Min7);
    const int third = minor ? 3 : 4;
    const int seventh = (q == Maj) ? 11 : 10;
    switch (tok)
    {
        case R:    return 0;
        case T3:   return third;
        case T5:   return 7;
        case T7:   return seventh;
        case OCT:  return 12;
        case S2:   return 2;
        case S4:   return 5;
        case S6:   return (q == Min) ? 8 : 9;   // menor natural escuro; m7 e maior com sexta maior (dórico/mixolídio)
        case LOW5: return -5;
        case LOW7: return seventh - 12;
        case LOW3: return third - 12;
        default:   return 0;
    }
}

//==============================================================================
// Células de um compasso (tempos em semínimas, 0..4)
struct Cell { double start; double len; int tok; float vel; bool opt; };
using Bar = std::vector<Cell>;

struct StyleBook
{
    std::vector<Bar> main;   // o groove
    std::vector<Bar> fills;  // viradas (fim de frase e troca de acorde)
    float swing;             // swing próprio do estilo (somado ao knob)
    float varScale;          // quanto o estilo aceita variar
    float newCellChance;     // chance de cada acorde trocar de célula (0 = riff único)
    int phraseBars;          // tamanho da frase (virada no último compasso)
    double lag = 0.0;        // microtempo: atraso fixo de todas as notas (tempos)
};

inline constexpr double t3 = 1.0 / 3.0;   // tercina

inline const StyleBook& book (int style)
{
    static const StyleBook books[NumStyles] = {
        // ---------------------------------------------------------------- FAMILY MAN
        { {
            // drop: entra no "e" do 1, quinta no 3, oitava respondendo
            { { 0.5, 0.9, R, 1.0f, false }, { 1.5, 0.4, R, 0.70f, true }, { 2.0, 0.9, T5, 0.95f, false },
              { 3.0, 0.45, OCT, 0.85f, false }, { 3.5, 0.4, T7, 0.75f, true } },
            // subida melódica: R 3 5 ... 4 3
            { { 0.5, 0.45, R, 1.0f, false }, { 1.0, 0.45, T3, 0.80f, false }, { 1.5, 0.9, T5, 0.95f, false },
              { 3.0, 0.4, S4, 0.80f, true }, { 3.5, 0.45, T3, 0.85f, false } },
            // tercina puxada para trás no 4
            { { 1.0, 0.9, R, 1.0f, false }, { 2.0, 0.5, T5, 0.90f, false }, { 2.5, 0.4, T3, 0.70f, true },
              { 3.0, t3 * 0.9, T5, 0.80f, false }, { 3.0 + t3, t3 * 0.9, S6, 0.75f, false }, { 3.0 + 2 * t3, t3, OCT, 0.85f, false } },
            // chamada grave: sétima e quinta abaixo
            { { 0.5, 0.7, R, 1.0f, false }, { 1.25, 0.2, R, 0.60f, true }, { 1.5, 0.45, LOW7, 0.80f, false },
              { 2.0, 0.9, LOW5, 0.90f, false }, { 3.5, 0.4, LOW7, 0.75f, true } },
          },
          {
            { { 0.5, 0.9, R, 1.0f, false }, { 2.0, 0.45, T5, 0.90f, false }, { 2.5, 0.45, T3, 0.80f, false },
              { 3.0, t3 * 0.9, S2, 0.75f, false }, { 3.0 + t3, t3 * 0.9, R, 0.75f, false }, { 3.0 + 2 * t3, t3, APP, 0.90f, false } },
            { { 0.5, 0.45, OCT, 1.0f, false }, { 1.0, 0.45, T7, 0.85f, false }, { 1.5, 0.9, T5, 0.90f, false },
              { 2.5, 0.45, T3, 0.80f, true }, { 3.0, 0.45, S2, 0.75f, false }, { 3.5, 0.45, APP, 0.90f, false } },
            { { 1.0, 0.9, R, 1.0f, false }, { 2.0, t3 * 0.9, OCT, 0.90f, false }, { 2.0 + t3, t3 * 0.9, T7, 0.80f, false },
              { 2.0 + 2 * t3, t3, T5, 0.80f, false }, { 3.0, 0.45, S4, 0.75f, true }, { 3.5, 0.45, APP, 0.90f, false } },
          },
          0.35f, 1.0f, 0.6f, 2 },

        // ---------------------------------------------------------------- FLABBA
        { {
            // fundamental longa no 1, repete no 3, cai na quinta grave
            { { 0.0, 1.4, R, 1.0f, false }, { 1.5, 0.4, R, 0.70f, true }, { 2.0, 0.9, R, 0.95f, false },
              { 3.0, 0.9, LOW5, 0.90f, false } },
            // desce para a sétima e senta na quinta grave
            { { 0.0, 0.9, R, 1.0f, false }, { 1.0, 0.4, R, 0.70f, true }, { 1.5, 0.45, LOW7, 0.80f, false },
              { 2.0, 1.4, LOW5, 0.95f, false }, { 3.5, 0.4, LOW5, 0.70f, true } },
            // rub-a-dub com terça
            { { 0.0, 0.7, R, 1.0f, false }, { 0.75, 0.2, R, 0.60f, true }, { 1.0, 0.9, T3, 0.85f, false },
              { 2.0, 0.45, R, 0.90f, false }, { 2.5, 0.45, LOW7, 0.80f, false }, { 3.0, 0.9, LOW5, 0.90f, false } },
            // balanço curto
            { { 0.0, 0.45, R, 1.0f, false }, { 0.5, 0.45, R, 0.75f, true }, { 1.5, 0.45, LOW5, 0.85f, false },
              { 2.0, 0.9, R, 0.95f, false }, { 3.0, 0.45, T3, 0.80f, true }, { 3.5, 0.45, LOW5, 0.80f, false } },
          },
          {
            { { 0.0, 1.4, R, 1.0f, false }, { 2.0, 0.9, R, 0.95f, false }, { 3.0, 0.45, LOW5, 0.85f, false },
              { 3.5, 0.45, APP, 0.90f, false } },
            { { 0.0, 1.4, R, 1.0f, false }, { 2.0, 0.45, R, 0.90f, false }, { 2.5, 0.45, LOW5, 0.85f, false },
              { 3.0, 0.45, LOW7, 0.80f, false }, { 3.5, 0.45, APP, 0.90f, false } },
          },
          0.15f, 0.35f, 0.0f, 4 },

        // ---------------------------------------------------------------- SAGITTARIUS
        { {
            // 3-3-2 com oitava
            { { 0.0, 0.6, R, 1.0f, false }, { 0.75, 0.5, R, 0.85f, false }, { 1.5, 0.4, OCT, 0.90f, false },
              { 2.0, 0.6, R, 1.0f, false }, { 2.75, 0.5, R, 0.85f, false }, { 3.5, 0.4, T5, 0.85f, false } },
            // oitava estalada
            { { 0.0, 0.4, R, 1.0f, false }, { 0.5, 0.2, OCT, 0.85f, false }, { 0.75, 0.2, R, 0.80f, true },
              { 1.5, 0.4, T5, 0.90f, false }, { 2.0, 0.4, R, 1.0f, false }, { 2.5, 0.2, OCT, 0.85f, false },
              { 2.75, 0.2, T7, 0.70f, true }, { 3.0, 0.4, T5, 0.90f, false }, { 3.5, 0.4, T3, 0.80f, true } },
            // stab com respiro
            { { 0.0, 0.3, R, 1.0f, false }, { 0.75, 0.25, R, 0.85f, false }, { 1.5, 0.25, T3, 0.80f, true },
              { 1.75, 0.7, T5, 0.90f, false }, { 3.0, 0.25, LOW7, 0.80f, false }, { 3.25, 0.25, R, 0.75f, true },
              { 3.5, 0.4, OCT, 0.90f, false } },
            // colcheias tocadas para frente
            { { 0.0, 0.4, R, 1.0f, false }, { 0.5, 0.2, R, 0.65f, true }, { 1.0, 0.4, T5, 0.85f, false },
              { 1.5, 0.4, OCT, 0.95f, false }, { 2.25, 0.4, R, 0.85f, false }, { 3.0, 0.25, T5, 0.80f, false },
              { 3.25, 0.25, T7, 0.75f, true }, { 3.5, 0.4, OCT, 0.90f, false } },
          },
          {
            // corrida de semicolcheias descendo até a próxima tônica
            { { 0.0, 0.6, R, 1.0f, false }, { 0.75, 0.5, R, 0.85f, false }, { 1.5, 0.4, OCT, 0.90f, false },
              { 2.0, 0.22, T5, 0.90f, false }, { 2.25, 0.22, T7, 0.80f, false }, { 2.5, 0.22, OCT, 0.90f, false },
              { 2.75, 0.22, T7, 0.80f, false }, { 3.0, 0.22, T5, 0.85f, false }, { 3.25, 0.22, T3, 0.80f, false },
              { 3.5, 0.22, S2, 0.80f, false }, { 3.75, 0.22, APP, 0.95f, false } },
            // oitavas em martelo
            { { 0.0, 0.4, R, 1.0f, false }, { 0.5, 0.2, OCT, 0.85f, false }, { 1.0, 0.4, R, 0.90f, false },
              { 1.5, 0.2, OCT, 0.85f, false }, { 2.0, 0.22, R, 0.90f, false }, { 2.5, 0.22, LOW7, 0.80f, false },
              { 2.75, 0.22, LOW5, 0.80f, true }, { 3.0, 0.22, R, 0.90f, false }, { 3.25, 0.22, OCT, 0.85f, false },
              { 3.5, 0.22, LOW5, 0.80f, true }, { 3.75, 0.22, APP, 0.95f, false } },
          },
          0.0f, 0.8f, 0.5f, 2 },

        // ---------------------------------------------------------------- ROBBIE
        { {
            // rockers: fundamental firme no 1, quinta e oitava empurrando
            { { 0.0, 0.45, R, 1.0f, false }, { 0.75, 0.2, R, 0.70f, true }, { 1.0, 0.45, T5, 0.90f, false },
              { 1.5, 0.4, OCT, 0.95f, false }, { 2.0, 0.45, R, 1.0f, false }, { 2.5, 0.2, T7, 0.75f, true },
              { 2.75, 0.4, OCT, 0.90f, false }, { 3.5, 0.4, T5, 0.85f, false } },
            // síncope de taxi: deixa o 3 vazio e cai no "e"
            { { 0.0, 0.7, R, 1.0f, false }, { 0.75, 0.2, T3, 0.75f, false }, { 1.0, 0.45, T5, 0.90f, false },
              { 1.75, 0.6, R, 0.90f, false }, { 2.5, 0.45, LOW7, 0.85f, false }, { 3.0, 0.45, R, 0.95f, false },
              { 3.5, 0.4, T3, 0.80f, true } },
            // funk: semicolcheias com oitava e sétima
            { { 0.0, 0.3, R, 1.0f, false }, { 0.5, 0.2, R, 0.65f, true }, { 0.75, 0.2, OCT, 0.85f, false },
              { 1.0, 0.2, T7, 0.75f, false }, { 1.25, 0.2, OCT, 0.70f, true }, { 1.5, 0.45, T5, 0.90f, false },
              { 2.25, 0.4, R, 0.90f, false }, { 3.0, 0.22, T3, 0.80f, false }, { 3.25, 0.22, S4, 0.75f, false },
              { 3.5, 0.4, T5, 0.90f, false } },
          },
          {
            // subida cromática até a próxima tônica
            { { 0.0, 0.45, R, 1.0f, false }, { 0.75, 0.2, R, 0.70f, true }, { 1.0, 0.45, T5, 0.90f, false },
              { 1.5, 0.4, OCT, 0.95f, false }, { 2.0, 0.45, R, 1.0f, false }, { 3.0, 0.22, T5, 0.85f, false },
              { 3.25, 0.22, S6, 0.80f, false }, { 3.5, 0.22, T7, 0.85f, false }, { 3.75, 0.22, APP, 0.95f, false } },
            { { 0.0, 0.7, R, 1.0f, false }, { 0.75, 0.2, T3, 0.75f, false }, { 1.0, 0.45, T5, 0.90f, false },
              { 2.0, 0.22, OCT, 0.90f, false }, { 2.25, 0.22, T7, 0.80f, false }, { 2.5, 0.22, T5, 0.85f, false },
              { 2.75, 0.22, T3, 0.80f, false }, { 3.0, 0.45, R, 0.90f, false }, { 3.5, 0.45, APP, 0.95f, false } },
          },
          0.0f, 0.7f, 0.4f, 2, 0.0 },

        // ---------------------------------------------------------------- LEROY SIBBLES
        { {
            // linha cantada: R 3 5 6 5
            { { 0.0, 0.9, R, 1.0f, false }, { 1.0, 0.45, T3, 0.80f, false }, { 1.5, 0.9, T5, 0.90f, false },
              { 2.5, 0.45, S6, 0.80f, true }, { 3.0, 0.9, T5, 0.85f, false } },
            // nota esticada e oitava
            { { 0.0, 1.4, R, 1.0f, false }, { 1.5, 0.45, T5, 0.85f, false }, { 2.0, 0.9, OCT, 0.90f, false },
              { 3.0, 0.45, T7, 0.80f, true }, { 3.5, 0.45, T5, 0.80f, false } },
            // frase que desce pela escala
            { { 0.0, 0.45, OCT, 1.0f, false }, { 0.5, 0.45, T7, 0.85f, false }, { 1.0, 0.9, T5, 0.90f, false },
              { 2.0, 0.45, T3, 0.85f, false }, { 2.5, 0.45, S2, 0.75f, true }, { 3.0, 0.9, R, 0.90f, false } },
            // espaço: respira no meio do compasso
            { { 0.0, 0.7, R, 1.0f, false }, { 1.0, 0.45, R, 0.70f, true }, { 1.5, 0.45, T3, 0.80f, false },
              { 2.0, 1.4, T5, 0.90f, false }, { 3.5, 0.45, S6, 0.75f, true } },
          },
          {
            { { 0.0, 0.9, R, 1.0f, false }, { 1.0, 0.45, T5, 0.85f, false }, { 1.5, 0.45, T3, 0.80f, false },
              { 2.0, 0.45, S2, 0.75f, false }, { 2.5, 0.45, R, 0.85f, false }, { 3.0, 0.45, LOW7, 0.80f, false },
              { 3.5, 0.45, APP, 0.90f, false } },
            { { 0.0, 1.4, R, 1.0f, false }, { 2.0, 0.45, T3, 0.85f, false }, { 2.5, 0.45, T5, 0.85f, false },
              { 3.0, 0.45, S6, 0.80f, false }, { 3.5, 0.45, APP, 0.90f, false } },
          },
          0.2f, 0.5f, 0.3f, 2, 0.035 },

        // ---------------------------------------------------------------- FULLY FULLWOOD
        { {
            // gancho com terça na anacruse e subida para a oitava
            { { 0.0, 0.4, R, 1.0f, false }, { 0.5, 0.2, R, 0.65f, true }, { 0.75, 0.2, T3, 0.80f, false },
              { 1.0, 0.9, T5, 0.90f, false }, { 2.0, 0.45, OCT, 0.95f, false }, { 2.5, 0.45, T7, 0.85f, false },
              { 3.0, 0.45, T5, 0.85f, false }, { 3.5, 0.45, T3, 0.80f, false } },
            // entra no "e" do 1 e salta de oitava no 3
            { { 0.5, 0.45, R, 1.0f, false }, { 1.0, 0.2, R, 0.70f, false }, { 1.25, 0.2, T3, 0.75f, false },
              { 1.5, 0.45, T5, 0.90f, false }, { 2.0, 0.9, OCT, 0.95f, false }, { 3.0, 0.22, T7, 0.80f, false },
              { 3.25, 0.22, T5, 0.80f, false }, { 3.5, 0.45, T3, 0.85f, false } },
            // passeio grave: sétima abaixo e volta
            { { 0.0, 0.9, R, 1.0f, false }, { 1.0, 0.45, LOW7, 0.80f, false }, { 1.5, 0.45, R, 0.85f, false },
              { 2.0, 0.45, T3, 0.85f, false }, { 2.5, 0.45, T5, 0.85f, false }, { 3.0, 0.45, T5, 0.70f, true },
              { 3.5, 0.45, OCT, 0.90f, false } },
          },
          {
            { { 0.0, 0.4, R, 1.0f, false }, { 0.75, 0.2, T3, 0.80f, false }, { 1.0, 0.9, T5, 0.90f, false },
              { 2.0, 0.45, OCT, 0.95f, false }, { 3.0, 0.22, T5, 0.85f, false }, { 3.25, 0.22, T3, 0.80f, false },
              { 3.5, 0.22, S2, 0.80f, false }, { 3.75, 0.22, APP, 0.95f, false } },
          },
          0.25f, 0.6f, 0.2f, 2, 0.0 },

        // ---------------------------------------------------------------- JAMMYS (Sleng Teng)
        { {
            // riff de teclado em semicolcheias: R 3 4 5 em volta da tônica
            { { 0.0, 0.2, R, 1.0f, false }, { 0.5, 0.2, R, 0.80f, false }, { 0.75, 0.2, T3, 0.85f, false },
              { 1.25, 0.2, R, 0.80f, false }, { 1.5, 0.2, S4, 0.85f, false }, { 2.0, 0.2, T5, 0.95f, false },
              { 2.5, 0.2, R, 0.80f, false }, { 2.75, 0.2, T3, 0.85f, false }, { 3.25, 0.2, S4, 0.80f, true },
              { 3.5, 0.2, T3, 0.85f, false } },
            { { 0.0, 0.25, R, 1.0f, false }, { 0.75, 0.25, R, 0.85f, false }, { 1.5, 0.25, T3, 0.85f, false },
              { 2.0, 0.25, S4, 0.90f, false }, { 2.25, 0.2, T5, 0.75f, true }, { 2.5, 0.25, S4, 0.85f, false },
              { 3.0, 0.25, T3, 0.85f, false }, { 3.5, 0.25, R, 0.90f, false } },
            { { 0.0, 0.2, R, 1.0f, false }, { 0.25, 0.2, R, 0.65f, true }, { 0.5, 0.2, OCT, 0.90f, false },
              { 1.0, 0.2, R, 0.85f, false }, { 1.5, 0.2, T3, 0.85f, false }, { 1.75, 0.2, S4, 0.80f, false },
              { 2.0, 0.45, T5, 0.95f, false }, { 3.0, 0.2, S4, 0.80f, false }, { 3.5, 0.2, T3, 0.85f, false } },
          },
          {
            { { 0.0, 0.2, R, 1.0f, false }, { 0.5, 0.2, R, 0.80f, false }, { 0.75, 0.2, T3, 0.85f, false },
              { 1.25, 0.2, R, 0.80f, false }, { 1.5, 0.2, S4, 0.85f, false }, { 2.0, 0.2, T5, 0.95f, false },
              { 2.5, 0.2, OCT, 0.90f, false }, { 3.0, 0.2, T5, 0.85f, false }, { 3.25, 0.2, S4, 0.80f, false },
              { 3.5, 0.2, T3, 0.85f, false }, { 3.75, 0.2, APP, 0.95f, false } },
          },
          0.0f, 0.3f, 0.0f, 4, 0.0 },
    };
    return books[std::clamp (style, 0, NumStyles - 1)];
}

//==============================================================================
// Gerador pseudoaleatório determinístico: a mesma IDEIA gera sempre a mesma linha
struct Rng
{
    uint64_t s;
    explicit Rng (uint64_t seed) : s (seed * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull) {}
    uint64_t next()
    {
        uint64_t z = (s += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    float uniform() { return (float) ((next() >> 40) * (1.0 / 16777216.0)); }   // 0..1
    int pick (int n) { return n <= 1 ? 0 : (int) (next() % (uint64_t) n); }
};

//==============================================================================
struct NoteEvent
{
    double start;    // tempos (semínimas) dentro do loop
    double length;   // tempos
    int note;
    int velocity;    // 1..127
    int slot;        // acorde de origem (para o visor)
    bool fill;       // nota de virada
};

struct Settings
{
    int style = FamilyMan;
    int seed = 1;           // IDEIA
    float density = 0.6f;   // 0..1: quanto das notas opcionais entra
    float variation = 0.5f; // 0..1: viradas e troca de célula
    int base = 31;          // a fundamental fica entre base e base+11 (G1..F#2 por padrão)
    float velocity = 0.85f; // 0..1
    float gate = 0.7f;      // 0..1: sustentação das notas
    float swing = 0.0f;     // 0..1: somado ao swing do estilo
    bool strict = false;    // reggae "de livro" (MRM-0420): toda nota dentro da pentatônica do acorde que soa,
                            // inclusive as aproximações (sem cromatismo)
};

inline int pc (int n) { return ((n % 12) + 12) % 12; }

// c + a*b sempre em dois arredondamentos de float: impede que o compilador funda numa FMA
// (cada compilador decide diferente), para o plugin dar a mesma linha no Windows, no Mac e no navegador
inline float addMul (float c, float a, float b)
{
    volatile float p = a * b;
    return c + p;
}

inline int fitRange (int n)
{
    while (n < 28) n += 12;   // E1 (corda solta mais grave)
    while (n > 67) n -= 12;   // G4
    return n;
}

// Nota de aproximação: encosta na tônica que o próximo acorde vai tocar (mesmo registro)
inline int approachNote (int prevNote, int nextRoot, int base, Rng& rng)
{
    (void) prevNote;
    const int target = base + pc (nextRoot - base);
    const float r = rng.uniform();
    int n;
    if (r < 0.5f)       n = target - 1;   // cromática por baixo
    else if (r < 0.8f)  n = target + 1;   // cromática por cima
    else                n = target - 2;   // diatônica por baixo
    return fitRange (n);
}

// Aproximação "de livro": a nota da pentatônica do acorde que ainda soa mais perto da tônica do próximo
// (de preferência por baixo, subindo para ela), nunca a própria tônica nem cromatismo
inline int strictApproach (int nextRoot, const ChordSlot& current, int base)
{
    const int target = fitRange (base + pc (nextRoot - base));
    for (int d = 1; d <= 5; ++d)
    {
        if (inPentatonic (target - d, current) && target - d >= 28) return target - d;
        if (inPentatonic (target + d, current) && target + d <= 67) return target + d;
    }
    return fitRange (base + pc (current.root - base));
}

//==============================================================================
inline void monophonic (std::vector<NoteEvent>& ev, double loopBeats);

// Gera o loop completo a partir dos acordes ativos
inline std::vector<NoteEvent> buildLoop (const std::vector<ChordSlot>& slotsIn, const Settings& s, double& loopBeats)
{
    std::vector<NoteEvent> ev;
    loopBeats = 0;

    std::vector<std::pair<int, ChordSlot>> slots;   // (índice original, acorde)
    for (int i = 0; i < (int) slotsIn.size(); ++i)
        if (slotsIn[(size_t) i].beats > 0) slots.push_back ({ i, slotsIn[(size_t) i] });
    if (slots.empty()) return ev;

    const auto& bk = book (s.style);
    Rng rng ((uint64_t) (s.seed * 7919 + s.style * 104729 + 17));
    const float var = std::clamp (s.variation, 0.0f, 1.0f) * bk.varScale;
    const float swing = std::clamp (s.swing + bk.swing, 0.0f, 1.0f);
    const float sustain = addMul (0.35f, 0.85f, std::clamp (s.gate, 0.0f, 1.0f));

    const int riff = rng.pick ((int) bk.main.size());   // identidade da linha (a mesma em todos os acordes)
    double pos = 0;
    int prevNote = s.base + 7;
    int barCounter = 0;

    for (size_t k = 0; k < slots.size(); ++k)
    {
        const auto& [slotIdx, ch] = slots[k];
        const auto& next = slots[(k + 1) % slots.size()].second;
        const bool changes = pc (next.root) != pc (ch.root) || next.quality != ch.quality;
        const int rootNote = s.base + pc (ch.root - s.base);

        int cellIdx = riff;
        if (rng.uniform() < bk.newCellChance * var) cellIdx = rng.pick ((int) bk.main.size());

        const int bars = (int) std::ceil (ch.beats / 4.0 - 1e-9);
        for (int b = 0; b < bars; ++b, ++barCounter)
        {
            const double barStart = pos + b * 4.0;
            const double barLen = std::min (4.0, ch.beats - b * 4.0);   // 2 tempos no "1/2 compasso"
            const bool lastBar = (b == bars - 1);
            const bool phraseEnd = ((barCounter + 1) % bk.phraseBars) == 0;

            bool useFill = (lastBar && changes) || (phraseEnd && rng.uniform() < var);
            if (lastBar && changes && rng.uniform() > addMul (0.35f, 0.65f, var)) useFill = false;  // nem toda troca tem virada
            const Bar& bar = useFill ? bk.fills[(size_t) rng.pick ((int) bk.fills.size())] : bk.main[(size_t) cellIdx];

            const size_t firstOfBar = ev.size();
            for (const auto& c : bar)
            {
                if (c.start >= barLen - 1e-6) continue;
                if (c.opt && rng.uniform() > s.density) continue;

                int note;
                if (c.tok == APP)
                {
                    if (! changes && ! lastBar) note = fitRange (rootNote + tokSemis (T5, ch.quality));
                    else if (! changes)         note = fitRange (rootNote + tokSemis (LOW7, ch.quality));
                    else                        note = s.strict ? strictApproach (next.root, ch, s.base)
                                                                : approachNote (prevNote, next.root, s.base, rng);
                }
                else
                    note = fitRange (rootNote + tokSemis (c.tok, ch.quality));
                if (s.strict && ! inPentatonic (note, ch))
                    note = snapToPentatonic (note, ch, 28, 67);

                // swing: colcheias e semicolcheias do contratempo atrasam em direção à tercina
                double st = c.start;
                const double frac = st - std::floor (st);
                if (std::abs (frac - 0.5) < 1e-6)                                  st += swing * (2.0 / 3.0 - 0.5);
                else if (std::abs (frac - 0.25) < 1e-6 || std::abs (frac - 0.75) < 1e-6) st += swing * (1.0 / 12.0);

                st += bk.lag;   // microtempo do estilo (Leroy Sibbles atrás do tempo)
                const float human = addMul (0.92f, 0.16f, rng.uniform());
                const int vel = std::clamp ((int) std::lround (c.vel * s.velocity * human * 127.0f), 1, 127);
                ev.push_back ({ barStart + st, c.len * sustain, note, vel, slotIdx, useFill });
                prevNote = note;
            }

            // meio compasso antes de uma troca: garante a aproximação no "e" do 2
            if (lastBar && changes && barLen < 4.0)
            {
                const double appAt = barStart + barLen - 0.5;
                while (ev.size() > firstOfBar && ev.back().start >= appAt - 1e-6) ev.pop_back();
                const int n = s.strict ? strictApproach (next.root, ch, s.base)
                                       : approachNote (ev.empty() ? prevNote : ev.back().note, next.root, s.base, rng);
                const int vel = std::clamp ((int) std::lround (0.9f * s.velocity * 127.0f), 1, 127);
                ev.push_back ({ appAt, 0.45 * sustain, n, vel, slotIdx, true });
                prevNote = n;
            }
        }
        pos += ch.beats;
    }
    loopBeats = pos;

    monophonic (ev, loopBeats);
    return ev;
}

// baixo é monofônico: ordena e faz cada nota terminar antes da próxima começar (inclusive na volta do loop)
inline void monophonic (std::vector<NoteEvent>& ev, double loopBeats)
{
    std::sort (ev.begin(), ev.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.start < b.start; });
    for (size_t i = 0; i < ev.size(); ++i)
    {
        const double nextStart = (i + 1 < ev.size()) ? ev[i + 1].start : ev[0].start + loopBeats;
        const double maxLen = nextStart - ev[i].start - 0.02;
        ev[i].length = std::max (0.04, std::min (ev[i].length, maxLen));
    }
}

} // namespace mbm
