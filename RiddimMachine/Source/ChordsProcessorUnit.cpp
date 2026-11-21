// Compila o Processor do Dub Chords dentro do MRM-0420, sem copiar o código.
#include <juce_audio_utils/juce_audio_utils.h>
#define createPluginFilter mrmUnused_createDubChords
#include "../../DubChords/Source/PluginProcessor.cpp"
