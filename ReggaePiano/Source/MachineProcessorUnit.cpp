// Compila o Processor do Mastor Reggae Chord Machine MRCM-1240 dentro do Mastor Reggae Piano MRP-80 (aba CHORD MACHINE),
// direto da pasta dele (nada copiado): o que mudar lá entra aqui no próximo build.
#include <juce_audio_utils/juce_audio_utils.h>
#define createPluginFilter mrkUnused_createChordMachine
#define MCM_NOTA64_DEFAULT 1   // no MRP-80 a nota padrão é 1/64
#include "../../ReggaeChordMachine/Source/PluginProcessor.cpp"
