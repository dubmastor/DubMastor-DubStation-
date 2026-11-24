// Compila o Processor do Dub Chords dentro do MRM-0420, sem copiar o código.
#include <juce_audio_utils/juce_audio_utils.h>
#define createPluginFilter mrmUnused_createDubChords
#define MCM_NOTA64_DEFAULT 1   // no MRM-0420 a nota padrão do piano é 1/64
#include "../../ReggaeChordMachine/Source/PluginProcessor.cpp"
