cmake_minimum_required(VERSION 3.22)

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(MastorKeyScout VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# formatos (no Linux use -DMKS_FORMATS=VST3 para testar)
set(MKS_FORMATS VST3 AU Standalone CACHE STRING "Formatos do plugin")

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

juce_add_plugin(MastorKeyScout
    COMPANY_NAME "DubMastor"
    BUNDLE_ID com.dubmastor.mastorkeyscout
    PLUGIN_MANUFACTURER_CODE Dblb
    PLUGIN_CODE Mks4
    FORMATS ${MKS_FORMATS} ${DM_AAX}
    PRODUCT_NAME "Mastor Key Scout MKS-0420"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Analyzer
)

juce_add_binary_data(MastorKeyScoutFonts SOURCES
    ../TapeEcho/Resources/KaushanScript-Regular.ttf)

set(MKS_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(MastorKeyScout PRIVATE ${MKS_SOURCES})

set(MKS_DEFINES
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_compile_definitions(MastorKeyScout PUBLIC ${MKS_DEFINES})

if(UNIX AND NOT APPLE)
    # só para o teste de compilação no Linux (sem cabeçalhos de ALSA/Xrandr/Xcursor)
    target_compile_definitions(MastorKeyScout PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(MastorKeyScout
    PRIVATE MastorKeyScoutFonts juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

# Teste sem DAW: o áudio passa igual, o link é publicado e a interface vira PNG (imagem do card do site)
# cmake -B build -DMKS_RENDER_TEST=ON && cmake --build build --target mks_render
option(MKS_RENDER_TEST "Compila o programa que desenha a interface em PNG" OFF)
if(MKS_RENDER_TEST)
    juce_add_console_app(mks_render PRODUCT_NAME "mks_render")
    target_sources(mks_render PRIVATE tests/render_ui.cpp ${MKS_SOURCES})
    target_compile_definitions(mks_render PRIVATE ${MKS_DEFINES}
        JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_ALSA=0 JUCE_JACK=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Key Scout MKS-0420" JucePlugin_VersionString="${PROJECT_VERSION}" JucePlugin_IsSynth=0
        JucePlugin_WantsMidiInput=0 JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(mks_render PRIVATE MastorKeyScoutFonts juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
