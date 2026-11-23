cmake_minimum_required(VERSION 3.22)

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(BassMachine VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BASSMACHINE_SNAPSHOT "Compila tambem o app que salva um PNG da interface" OFF)
option(BASSMACHINE_PLUGIN   "Compila o plugin (desligue para gerar so o snapshot)" ON)

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

if(BASSMACHINE_PLUGIN)
    juce_add_plugin(BassMachine
        COMPANY_NAME               "DubMastor"
        BUNDLE_ID                  com.dubmastor.bassmachine
        PRODUCT_NAME               "Mastor Reggae Bass Machine MRBM-69"
        PLUGIN_MANUFACTURER_CODE   Dblb
        PLUGIN_CODE                Mbm7
        FORMATS                    VST3 AU Standalone ${DM_AAX}
        IS_SYNTH                   TRUE
        NEEDS_MIDI_INPUT           TRUE
        NEEDS_MIDI_OUTPUT          TRUE
        IS_MIDI_EFFECT             FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD    FALSE
        VST3_CATEGORIES            Instrument Generator
        AU_MAIN_TYPE               kAudioUnitType_MusicDevice)

    target_sources(BassMachine PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

    # timbres compartilhados: piano M1 + Hammond do Roots Keys (usados pela ponte do Dub Chords)
    target_include_directories(BassMachine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../RootsKeys/Source)

    target_compile_definitions(BassMachine PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(BassMachine
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()

# App de teste: abre o editor fora da DAW e salva docs/interface.png
if(BASSMACHINE_SNAPSHOT)
    juce_add_console_app(BassMachineSnapshot PRODUCT_NAME "BassMachineSnapshot")
    target_sources(BassMachineSnapshot PRIVATE
        tests/snapshot.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_compile_definitions(BassMachineSnapshot PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Reggae Bass Machine MRBM-69" JucePlugin_VersionString="${PROJECT_VERSION}" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=1 JucePlugin_IsMidiEffect=0)
    target_include_directories(BassMachineSnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../RootsKeys/Source)
    target_link_libraries(BassMachineSnapshot PRIVATE juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
