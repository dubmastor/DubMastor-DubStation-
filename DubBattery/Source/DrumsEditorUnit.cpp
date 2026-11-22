// Compila a interface do Mastor Reggae Drum Machine MRDM-12 (Dub Drums) dentro do DUB BATTERY REGGAE,
// direto da pasta DubDrums (nada copiado). Ela aparece inteira na aba DUB DRUMS.
#include <juce_audio_utils/juce_audio_utils.h>
#include "DrumsNames.h"
#define createPluginFilter dbrUnused_createDubDrums
#include "../../DubDrumsMachine/Source/PluginEditor.cpp"
