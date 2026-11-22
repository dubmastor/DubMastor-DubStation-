// Compila o Editor do Mastor Reggae Bass Machine MRBM-69 dentro do MRM-0420, sem copiar o código.
#include <juce_audio_utils/juce_audio_utils.h>
#include "BassNames.h"
#define createPluginFilter mrmUnused_createBassMachine
#include "../../ReggaeBassMachine/Source/PluginEditor.cpp"
