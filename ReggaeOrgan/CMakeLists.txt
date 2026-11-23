cmake_minimum_required(VERSION 3.22)

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(ReggaeOrgan VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# Use -DJUCE_DIR=/caminho/JUCE para uma cópia local; senão baixa do GitHub
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# Pro Tools (AAX) no Windows e no Mac (veja .github/cmake/aax.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/../.github/cmake/aax.cmake")

# formatos (o gerador de captura de tela usa só VST3)
set(RO_FORMATS VST3 AU Standalone ${DM_AAX} CACHE STRING "")

juce_add_plugin(ReggaeOrgan
    COMPANY_NAME "DubMastor"
    BUNDLE_ID com.dubmastor.reggaeorgan
    PLUGIN_MANUFACTURER_CODE Dblb
    PLUGIN_CODE Mro6
    FORMATS ${RO_FORMATS}
    PRODUCT_NAME "Mastor Reggae Organ MRO-68"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth)

# o órgão, o visual e as fontes são os mesmos do Mastor Reggae Keys MRK-80 (pasta ReggaeKeys/)
juce_add_binary_data(ReggaeOrganFonts SOURCES
    ../ReggaeKeys/Resources/ArchivoBlack-Regular.ttf
    ../ReggaeKeys/Resources/RootsKeysDisplay-BoldItalic.ttf)

target_sources(ReggaeOrgan PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(ReggaeOrgan PRIVATE Source ../ReggaeKeys/Source ../RiddimBank/Source)

target_compile_definitions(ReggaeOrgan PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(UNIX AND NOT APPLE)
    # só para o teste de compilação no Linux (sem cabeçalhos de ALSA/Xrandr/Xcursor)
    target_compile_definitions(ReggaeOrgan PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(ReggaeOrgan
    PRIVATE ReggaeOrganFonts juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

# Captura de tela da interface (docs/interface.png e a imagem do site), sem DAW:
#   cmake -B build-snap -DRO_SNAPSHOT=ON -DRO_FORMATS=VST3 && cmake --build build-snap --target ReggaeOrganSnapshot
option(RO_SNAPSHOT "Gerador de captura de tela" OFF)
if(RO_SNAPSHOT)
    juce_add_console_app(ReggaeOrganSnapshot PRODUCT_NAME "ReggaeOrganSnapshot")
    target_sources(ReggaeOrganSnapshot PRIVATE tests/snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(ReggaeOrganSnapshot PRIVATE Source ../ReggaeKeys/Source ../RiddimBank/Source)
    target_compile_definitions(ReggaeOrganSnapshot PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0 JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Reggae Organ MRO-68" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0
        JucePlugin_VersionString="${PROJECT_VERSION}")
    target_link_libraries(ReggaeOrganSnapshot PRIVATE ReggaeOrganFonts juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
