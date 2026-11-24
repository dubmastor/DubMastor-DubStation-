// Compila a interface do Mastor Reggae Chord Machine MRCM-1240 dentro do Mastor Reggae Keys MRK-80 (aba CHORD MACHINE).
#include <juce_audio_utils/juce_audio_utils.h>
#define createPluginFilter mrkUnused_createChordMachine
#define MCM_PANEL_TITLE "CHORD MACHINE  -  MASTOR REGGAE KEYS "
#define MCM_PANEL_SIGLA "MRK-80"
#define MCM_MEGA_BANK "MRK-80"   // presets no padrão do mega banco da Riddim Machine (favoritos em DubMastor/MRK-80.settings)
#include "../../ReggaeChordMachine/Source/PluginEditor.cpp"

// o painel inteiro do MRCM-1240 (o editor do Roots Keys não vê as classes dele)
std::unique_ptr<juce::Component> makeChordMachinePanel (DubChordsProcessor& m)
{
    return std::make_unique<DubChordsPanel> (m);
}
