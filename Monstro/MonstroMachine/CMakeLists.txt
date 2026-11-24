cmake_minimum_required(VERSION 3.22)

# Monstro Machine MM-2500 — drum machine com os timbres do Monstro.zip (Academia de Beats).
# Produto à parte: NÃO faz parte do Dub Station (fica fora do plugins.json, do site do Dub Station e do pacote).

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(MonstroMachine VERSION 1.1.0)

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
include("${CMAKE_CURRENT_LIST_DIR}/../../.github/cmake/aax.cmake")

set(MM_FORMATS VST3 AU Standalone ${DM_AAX} CACHE STRING "")

juce_add_binary_data(MonstroData SOURCES Resources/academia.png Resources/monstro.jpg)

juce_add_plugin(MonstroMachine
    COMPANY_NAME "DubMastor"
    BUNDLE_ID com.dubmastor.monstromachine
    PLUGIN_MANUFACTURER_CODE Dblb
    PLUGIN_CODE Mm25
    FORMATS ${MM_FORMATS}
    PRODUCT_NAME "Monstro Machine MM-2500"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Drum)

target_sources(MonstroMachine PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(MonstroMachine PRIVATE Source)

target_compile_definitions(MonstroMachine PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(UNIX AND NOT APPLE)
    # só para o teste de compilação no Linux (sem cabeçalhos de ALSA/Xrandr/Xcursor)
    target_compile_definitions(MonstroMachine PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(MonstroMachine
    PRIVATE juce::juce_audio_utils MonstroData
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

# Teste offline do motor (sem JUCE): cmake --build build --target mm_render && ./build/mm_render
add_executable(mm_render EXCLUDE_FROM_ALL tests/render_test.cpp)
target_include_directories(mm_render PRIVATE Source)

# Captura de tela da interface (docs/) e teste do plugin inteiro, sem DAW:
#   cmake -B build-snap -DMM_SNAPSHOT=ON -DMM_FORMATS=VST3 && cmake --build build-snap --target MonstroSnapshot
option(MM_SNAPSHOT "Gerador de captura de tela" OFF)
if(MM_SNAPSHOT)
    juce_add_console_app(MonstroSnapshot PRODUCT_NAME "MonstroSnapshot")
    target_sources(MonstroSnapshot PRIVATE tests/snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(MonstroSnapshot PRIVATE Source)
    target_compile_definitions(MonstroSnapshot PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0 JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Monstro Machine MM-2500" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0
        JucePlugin_VersionString="${PROJECT_VERSION}")
    target_link_libraries(MonstroSnapshot PRIVATE juce::juce_audio_utils MonstroData
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
