// Compila o Processor do Mastor Reggae Organ MRO-68 dentro do MRM-0420, sem copiar o código.
#include <juce_audio_utils/juce_audio_utils.h>
#include "OrganNames.h"
#define createPluginFilter mrmUnused_createOrgan
#include "../../ReggaeOrgan/Source/PluginProcessor.cpp"
