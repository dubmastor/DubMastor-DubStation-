cmake_minimum_required(VERSION 3.22)

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(MastorTune VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# formatos (no Linux use -DMT_FORMATS=VST3 para testar)
set(MT_FORMATS VST3 AU Standalone CACHE STRING "Formatos do plugin")

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

juce_add_plugin(MastorTune
    COMPANY_NAME "DubMastor"
    BUNDLE_ID com.dubmastor.mastortune
    PLUGIN_MANUFACTURER_CODE Dblb
    PLUGIN_CODE Mt42
    FORMATS ${MT_FORMATS} ${DM_AAX}
    PRODUCT_NAME "Mastor Tune MT-0420"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Pitch Shift
    AU_MAIN_TYPE kAudioUnitType_MusicEffect)

juce_add_binary_data(MastorTuneFonts SOURCES
    ../TapeEcho/Resources/KaushanScript-Regular.ttf)

set(MT_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(MastorTune PRIVATE ${MT_SOURCES})

set(MT_DEFINES
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_compile_definitions(MastorTune PUBLIC ${MT_DEFINES})

if(UNIX AND NOT APPLE)
    # só para o teste de compilação no Linux (sem cabeçalhos de ALSA/Xrandr/Xcursor)
    target_compile_definitions(MastorTune PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(MastorTune
    PRIVATE MastorTuneFonts juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

# Teste sem DAW: PNG das duas abas (e a imagem do card do site)
# cmake -B build -DMT_RENDER_TEST=ON && cmake --build build --target mt_render
option(MT_RENDER_TEST "Compila o programa que desenha a interface em PNG" OFF)
if(MT_RENDER_TEST)
    juce_add_console_app(mt_render PRODUCT_NAME "mt_render")
    target_sources(mt_render PRIVATE tests/render_ui.cpp ${MT_SOURCES})
    target_compile_definitions(mt_render PRIVATE ${MT_DEFINES}
        JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_ALSA=0 JUCE_JACK=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Tune MT-0420" JucePlugin_VersionString="${PROJECT_VERSION}" JucePlugin_IsSynth=0
        JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(mt_render PRIVATE MastorTuneFonts juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
