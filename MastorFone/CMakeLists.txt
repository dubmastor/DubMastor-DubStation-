cmake_minimum_required(VERSION 3.22)

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

project(MastorFone VERSION 1.2.0)

set(CMAKE_CXX_STANDARD 17)

# formatos (no Linux use -DRF_FORMATS=VST3 para testar)
set(RF_FORMATS VST3 AU Standalone CACHE STRING "Formatos do plugin")

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

juce_add_plugin(MastorFone
    COMPANY_NAME "DubMastor"
    BUNDLE_ID com.dubmastor.mastorfone
    PLUGIN_MANUFACTURER_CODE Dblb
    PLUGIN_CODE Drfn
    FORMATS ${RF_FORMATS}
    PRODUCT_NAME "Mastor Fone"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Distortion Delay)

juce_add_binary_data(MastorFoneFonts SOURCES
    Resources/ArchivoBlack-Regular.ttf
    Resources/ComicRelief-Bold.ttf
    Resources/texgyrechorus-mediumitalic.otf)

target_sources(MastorFone PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/StationPanel.cpp)

target_compile_definitions(MastorFone PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(UNIX AND NOT APPLE)
    # só para o teste de compilação no Linux (sem cabeçalhos de ALSA/Xrandr/Xcursor)
    target_compile_definitions(MastorFone PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(MastorFone
    PRIVATE MastorFoneFonts juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
