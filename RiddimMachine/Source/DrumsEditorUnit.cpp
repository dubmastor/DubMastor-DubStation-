// Compila o Editor do Dub Drums dentro do MRM-0420, sem copiar o código.
// As classes de interface com o mesmo nome do Dub Chords ganham outro nome aqui.
#include <juce_audio_utils/juce_audio_utils.h>
#include "DrumsNames.h"
#define createPluginFilter mrmUnused_createDubDrums
#include "../../ReggaeDrumMachine/Source/PluginEditor.cpp"
