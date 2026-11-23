// Compila a interface do Mastor Reggae Bass Machine MRBM-69 dentro do Mastor Reggae Bass MRB-69 (aba BASS MACHINE).
#include <juce_audio_utils/juce_audio_utils.h>
#include "MachineNames.h"
#define createPluginFilter mrbUnused_createBassMachine
#define MBM_PANEL_TITLE "BASS MACHINE  -  MASTOR REGGAE BASS MRB-69 "
#include "../../ReggaeBassMachine/Source/PluginEditor.cpp"

// o painel inteiro do MRBM-69 (o editor do Dub Bass não vê as classes dele)
std::unique_ptr<juce::Component> makeBassMachinePanel (BassMachineProcessor& m)
{
    return std::make_unique<BassMachinePanel> (m);
}
