// Compila o Processor do Mastor Reggae Bass Machine MRBM-69 dentro do Mastor Reggae Bass MRB-69 (aba BASS MACHINE),
// direto da pasta dele (nada copiado): o que mudar lá entra aqui no próximo build.
#include <juce_audio_utils/juce_audio_utils.h>
#include "MachineNames.h"
#define createPluginFilter mrbUnused_createBassMachine
#include "../../ReggaeBassMachine/Source/PluginProcessor.cpp"
