cmake_minimum_required(VERSION 3.22)

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# a versão mora em Source/Version.h (toda alteração sobe o número: 1.0, 1.1, 1.2 ...)
file(STRINGS Source/Version.h HORNS_VERSION_LINE REGEX "MASTOR_HORNS_VERSION \"[0-9.]+\"")
string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)?" HORNS_VERSION "${HORNS_VERSION_LINE}")

# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(MastorDubHorns VERSION ${HORNS_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(HORNS_SNAPSHOT "Compila tambem o app que salva PNGs da interface" OFF)

# Use -DJUCE_DIR=/caminho/JUCE para uma cópia local; senão baixa do GitHub
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# Pro Tools (AAX) no Windows e no Mac (veja .github/cmake/aax.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/../.github/cmake/aax.cmake")

set(HORNS_FORMATS VST3 AU Standalone CACHE STRING "Formatos do plugin")

if(NOT HORNS_SNAPSHOT_ONLY)
juce_add_plugin(MastorDubHorns
    COMPANY_NAME               "DubMastor"
    BUNDLE_ID                  com.dubmastor.mastordubhorns
    PRODUCT_NAME               "Mastor Dub Horns MDH-70"
    PLUGIN_MANUFACTURER_CODE   Dblb
    PLUGIN_CODE                Dbhn
    FORMATS                    ${HORNS_FORMATS} ${DM_AAX}
    IS_SYNTH                   TRUE
    NEEDS_MIDI_INPUT           TRUE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD    FALSE
    VST3_CATEGORIES            Instrument Synth
    AU_MAIN_TYPE               kAudioUnitType_MusicDevice)

target_sources(MastorDubHorns PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(MastorDubHorns PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MastorDubHorns
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
endif()

# App de teste: abre o editor fora da DAW e salva os PNGs das abas (docs/)
if(HORNS_SNAPSHOT)
    juce_add_console_app(HornsSnapshot PRODUCT_NAME "HornsSnapshot")
    target_sources(HornsSnapshot PRIVATE
        tests/snapshot.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_compile_definitions(HornsSnapshot PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Dub Horns MDH-70" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(HornsSnapshot PRIVATE juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
