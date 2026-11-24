// Compila a interface do Mastor Reggae Chord Machine MRCM-1240 dentro do Mastor Reggae Piano MRP-80 (aba CHORD MACHINE).
#include <juce_audio_utils/juce_audio_utils.h>
#define createPluginFilter mrkUnused_createChordMachine
#define MCM_PANEL_TITLE "CHORD MACHINE  -  MASTOR REGGAE PIANO "
#define MCM_NO_BUBBLE 1   // só o skank de piano: o bubble é do organista (Mastor Reggae Organ MRO-68)
#define MCM_PANEL_SIGLA "MRP-80"
#include "../../ReggaeChordMachine/Source/PluginEditor.cpp"

// o painel inteiro do MRCM-1240 (o editor do Roots Keys não vê as classes dele)
std::unique_ptr<juce::Component> makeChordMachinePanel (DubChordsProcessor& m)
{
    return std::make_unique<DubChordsPanel> (m);
}
