cmake_minimum_required(VERSION 3.22)

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(ReggaeClavinet VERSION 1.0.0)

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
set(RCL_FORMATS VST3 AU Standalone ${DM_AAX} CACHE STRING "")

juce_add_plugin(ReggaeClavinet
    COMPANY_NAME "DubMastor"
    BUNDLE_ID com.dubmastor.reggaeclavinet
    PLUGIN_MANUFACTURER_CODE Dblb
    PLUGIN_CODE Mcl7
    FORMATS ${RCL_FORMATS}
    PRODUCT_NAME "Mastor Reggae Clavinet MRCL-71"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth)

target_sources(ReggaeClavinet PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

# banco único de riddims (o mesmo das machines); 1 = só as músicas com a harmonia da gravação
target_include_directories(ReggaeClavinet PRIVATE Source ../RiddimBank/Source)

target_compile_definitions(ReggaeClavinet PUBLIC
    RIDDIMBANK_REQUIRE=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(UNIX AND NOT APPLE)
    # só para o teste de compilação no Linux (sem cabeçalhos de ALSA/Xrandr/Xcursor)
    target_compile_definitions(ReggaeClavinet PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(ReggaeClavinet
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

# Captura de tela da interface (docs/ e a imagem do site), sem DAW:
#   cmake -B build-snap -DRCL_SNAPSHOT=ON -DRCL_FORMATS=VST3 && cmake --build build-snap --target ReggaeClavinetSnapshot
option(RCL_SNAPSHOT "Gerador de captura de tela" OFF)
if(RCL_SNAPSHOT)
    juce_add_console_app(ReggaeClavinetSnapshot PRODUCT_NAME "ReggaeClavinetSnapshot")
    target_sources(ReggaeClavinetSnapshot PRIVATE tests/snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(ReggaeClavinetSnapshot PRIVATE Source ../RiddimBank/Source)
    target_compile_definitions(ReggaeClavinetSnapshot PRIVATE
        RIDDIMBANK_REQUIRE=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0 JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Reggae Clavinet MRCL-71" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0
        JucePlugin_VersionString="${PROJECT_VERSION}")
    target_link_libraries(ReggaeClavinetSnapshot PRIVATE juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
