// Compila o Processor do Mastor Reggae Chord Machine MRCM-1240 dentro do Mastor Reggae Keys MRK-80 (aba CHORD MACHINE),
// direto da pasta dele (nada copiado): o que mudar lá entra aqui no próximo build.
#include <juce_audio_utils/juce_audio_utils.h>
#define createPluginFilter mrkUnused_createChordMachine
#include "../../ReggaeChordMachine/Source/PluginProcessor.cpp"
