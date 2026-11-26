// As máquinas que formam o MRM-0420:
//   MRCM-1240 (harmonia), MRDM-12 (bateria), MRBM-69 (baixo), MRO-68 (órgão, desde a 2.8) e o Mastor Big Knob (efeito no master).
// O código de cada uma é compilado direto da pasta dela; as classes com nomes repetidos ganham outro
// nome aqui (DrumsNames.h, BassNames.h, KnobNames.h).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

#include "../../ReggaeChordMachine/Source/PluginEditor.h"

#include "DrumsNames.h"
#include "../../ReggaeDrumMachine/Source/PluginEditor.h"
#include "DrumsNamesEnd.h"

#include "BassNames.h"
#include "../../ReggaeBassMachine/Source/PluginEditor.h"
#include "BassNamesEnd.h"

#include "KnobNames.h"
#include "../../BigKnob/Source/PluginEditor.h"
#include "KnobNamesEnd.h"

#include "OrganNames.h"
#include "../../ReggaeOrgan/Source/PluginEditor.h"
#include "OrganNamesEnd.h"
