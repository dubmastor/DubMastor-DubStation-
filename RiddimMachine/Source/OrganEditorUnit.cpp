// Compila a interface do Mastor Reggae Organ MRO-68 dentro do MRM-0420, sem copiar o código.
// A versão que aparece no painel é a do MRO-68 (não a do MRM-0420).
#include <juce_audio_utils/juce_audio_utils.h>
#undef  JucePlugin_VersionString
#define JucePlugin_VersionString MRM_ORGAN_VER
#include "OrganNames.h"
#include "../../ReggaeOrgan/Source/PluginEditor.cpp"
