// O Big Knob tem um namespace "dub" (filtro e efeitos) com o mesmo nome do namespace do MRCM-1240 e uma
// DubLookAndFeel própria: dentro do MRM-0420 os dois ganham outro nome, sem mexer no código original.
// Sem #pragma once de propósito: incluir antes do código do Big Knob e depois KnobNamesEnd.h.
#define dub            bigknob_dub
#define DubLookAndFeel KnobLookAndFeel
