cmake_minimum_required(VERSION 3.22)
# binário universal no Mac (Apple Silicon + Intel); precisa vir antes do project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

project(DubDrums VERSION 2.3.0)

# versão curta para a plaqueta (ex.: "1.0")
set(DD_VERSION_SHORT "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

# formatos (no Linux use -DDD_FORMATS=VST3 para testar)
set(DD_FORMATS VST3 AU Standalone CACHE STRING "Formatos do plugin")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Baixa o JUCE automaticamente na primeira configuração.
# Se já tiver o JUCE no computador: cmake -B build -DJUCE_DIR=/caminho/para/JUCE
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(DubDrums
    COMPANY_NAME                "DubMastor"
    BUNDLE_ID                   com.dubmastor.dubdrums
    PRODUCT_NAME                "Mastor Reggae Drum Machine MRDM-12"
    PLUGIN_MANUFACTURER_CODE    Dblb
    PLUGIN_CODE                 Dbd1
    FORMATS                     ${DD_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Drum Generator
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice)

target_sources(DubDrums PRIVATE
    Source/PatternGenerator.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(DubDrums PUBLIC
    DD_VERSION_SHORT="${DD_VERSION_SHORT}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(DubDrums
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(UNIX AND NOT APPLE)
    # teste de compilação no Linux sem ALSA/Xrandr/Xcursor
    target_compile_definitions(DubDrums PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

# Gera uma imagem da interface (docs/interface.png) sem abrir a DAW:
# cmake -B build -DDD_RENDER_PREVIEW=ON && cmake --build build --target render_preview
option(DD_RENDER_PREVIEW "Programa que salva um PNG da interface" OFF)
if(DD_RENDER_PREVIEW)
    juce_add_console_app(render_preview PRODUCT_NAME "render_preview")
    target_sources(render_preview PRIVATE tests/render_preview.cpp
        Source/PatternGenerator.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(render_preview PRIVATE Source)
    target_compile_definitions(render_preview PRIVATE DD_VERSION_SHORT="${DD_VERSION_SHORT}" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
    target_link_libraries(render_preview PRIVATE juce::juce_audio_utils juce::juce_audio_processors
        juce::juce_recommended_config_flags)
endif()

# Teste do gerador (não precisa do JUCE)
add_executable(test_generator tests/test_generator.cpp Source/PatternGenerator.cpp)
target_include_directories(test_generator PRIVATE Source)
