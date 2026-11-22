// As máquinas que formam o MRM-0420:
//   MRCM-1240 (harmonia), MRDM-12 (bateria), MRBM-69 (baixo) e o Mastor Big Knob (efeito no master).
// O código de cada uma é compilado direto da pasta dela; as classes com nomes repetidos ganham outro
// nome aqui (DrumsNames.h, BassNames.h, KnobNames.h).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

#include "../../ReggaeChordMachine/Source/PluginEditor.h"

#include "DrumsNames.h"
#include "../../DubDrumsMachine/Source/PluginEditor.h"
#include "DrumsNamesEnd.h"

#include "BassNames.h"
#include "../../ReggaeBassMachine/Source/PluginEditor.h"
#include "BassNamesEnd.h"

#include "KnobNames.h"
#include "../../BigKnob/Source/PluginEditor.h"
#include "KnobNamesEnd.h"
