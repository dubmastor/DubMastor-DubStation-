// Compila o Editor do Mastor Big Knob dentro do MRM-0420, sem copiar o código.
#include <juce_audio_utils/juce_audio_utils.h>
#include "KnobNames.h"
#define createPluginFilter mrmUnused_createBigKnob
#include "../../BigKnob/Source/PluginEditor.cpp"
