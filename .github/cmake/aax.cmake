# Formato AAX (Pro Tools) para os plugins DubMastor.
# O JUCE 8 já traz o SDK do AAX (modules/juce_audio_plugin_client/AAX/SDK), então o AAX é
# compilado sempre no Windows e no Mac. Para desligar: cmake -B build -DDM_SEM_AAX=ON
# Uso no CMakeLists, depois de carregar o JUCE:   FORMATS VST3 AU Standalone ${DM_AAX}
# Lembrete: o Pro Tools normal só abre o AAX depois de assinado pela PACE (veja PROTOOLS.md).
set(DM_AAX "")
if((APPLE OR WIN32) AND NOT DM_SEM_AAX)
    set(DM_AAX AAX)
endif()
