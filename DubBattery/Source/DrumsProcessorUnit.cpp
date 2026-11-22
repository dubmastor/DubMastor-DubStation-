// Compila o Processor do Mastor Reggae Drum Machine MRDM-12 (Dub Drums) dentro do DUB BATTERY REGGAE,
// direto da pasta DubDrums (nada copiado): o que mudar lá entra aqui no próximo build.
#include <juce_audio_utils/juce_audio_utils.h>
#include "DrumsNames.h"
#define createPluginFilter dbrUnused_createDubDrums
#include "../../DubDrumsMachine/Source/PluginProcessor.cpp"
