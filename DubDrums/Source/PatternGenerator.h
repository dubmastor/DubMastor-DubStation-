// Dub Drums — gerador de esqueleto de bateria de reggae
// Código C++ puro (sem dependência do JUCE) para poder ser testado isoladamente.
#pragma once

#include <vector>
#include <cstdint>

namespace dubdrums
{

// Mapa General MIDI (canal 10)
namespace gm
{
    constexpr int Kick       = 36;
    constexpr int SideStick  = 37; // aro / cross-stick
    constexpr int Snare      = 38;
    constexpr int ClosedHat  = 42;
    constexpr int PedalHat   = 44;
    constexpr int OpenHat    = 46;
    constexpr int TomLow     = 43;
    constexpr int TomMid     = 47;
    constexpr int TomHigh    = 50;
    constexpr int Crash      = 49;
}

enum class Style      { OneDrop = 0, Rockers, Steppa };
enum class HatPattern { Eighths = 0, Sixteenths, Offbeat, Triplets };
enum class Swing      { Straight = 0, Light, Medium, Heavy, Shuffle };
enum class Dynamics   { Soft = 0, Medium, Loud, Accented, Crescendo, Breathing };

// Baterista de referência: muda o balanço (swing e microtempo) e as viradas
//  - Style Scott (Roots Radics): reto e em cima do tempo, aro estalado, viradas de semicolcheia
//  - Santa Davis (Soul Syndicate): cheio de enfeites, rufos de fusa e flams
//  - Carlton Barrett (Wailers): atrás do tempo, balanço de tercina, rufos em tercina no 4
enum class Drummer    { StyleScott = 0, SantaDavis, CarltonBarrett };

struct Settings
{
    Style      style      = Style::OneDrop;
    int        bars       = 4;              // 1..4
    HatPattern hatPattern = HatPattern::Eighths;
    Swing      swing      = Swing::Straight;
    bool       swingAll   = true;           // aplica o swing também em bumbo/caixa/tons
    Dynamics   dynamics   = Dynamics::Medium;

    // aleatoriedade por peça, 0..1
    float randKick  = 0.0f;
    float randSnare = 0.0f;
    float randHat   = 0.0f;
    float randPerc  = 0.0f;   // tons / viradas

    Drummer  drummer = Drummer::CarltonBarrett;
    bool     fill  = true;    // virada no último compasso (+ prato no "1")
    bool     intro = false;   // compasso de virada antes do primeiro compasso (toca uma vez)
    uint32_t seed = 1;
};

struct NoteEvent
{
    double beat;      // posição em semínimas desde o início do loop
    double length;    // duração em semínimas
    int    note;
    int    velocity;  // 1..127
    int    bar = 0;   // compasso do loop (0..3); -1 = introdução
};

struct Pattern
{
    std::vector<NoteEvent> events;   // loop principal, ordenados por posição (0..lengthInBeats)
    double lengthInBeats = 16.0;     // bars * 4

    std::vector<NoteEvent> intro;    // virada de introdução (0..introLengthInBeats), toca uma vez antes do loop
    double introLengthInBeats = 0.0; // 0 = sem introdução, 4 = um compasso
};

Pattern generate (const Settings& s);

// nomes para a interface
const char* styleName (Style);
const char* swingName (Swing);
const char* drummerName (Drummer);

} // namespace dubdrums
