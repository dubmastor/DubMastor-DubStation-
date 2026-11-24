cmake_minimum_required(VERSION 3.22)

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# versão: aparece no título da interface ("... MRDBI-07 1.5") e no plugin
# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(DubBass VERSION 2.1.0)   # alvo interno com o nome antigo: o ID de estado e o BUNDLE_ID não mudam

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DUBBASS_TESTS "Compila tambem o app de teste (renderiza audio e salva um PNG da interface)" OFF)

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

juce_add_plugin(DubBass
    COMPANY_NAME               "DubMastor"
    BUNDLE_ID                  com.dubmastor.dubbass
    PRODUCT_NAME               "Mastor Reggae Bass MRB-69"
    PLUGIN_MANUFACTURER_CODE   Dblb
    PLUGIN_CODE                Dbbs
    FORMATS                    VST3 AU Standalone ${DM_AAX}
    IS_SYNTH                   TRUE
    NEEDS_MIDI_INPUT           TRUE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD    FALSE
    VST3_CATEGORIES            Instrument Synth
    AU_MAIN_TYPE               kAudioUnitType_MusicDevice)

# aba BASS MACHINE: o MRBM-69 inteiro, compilado direto da pasta dele
set(MRB_MACHINE_SOURCES Source/MachineProcessorUnit.cpp Source/MachineEditorUnit.cpp)
target_sources(DubBass PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    ${MRB_MACHINE_SOURCES})
target_include_directories(DubBass PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ReggaeKeys/Source)   # teclado de acompanhamento
if(UNIX AND NOT APPLE)
    # teste de compilação no Linux sem ALSA/JACK/Xrandr/Xcursor (como no Mastor Reggae Drums)
    target_compile_definitions(DubBass PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_compile_definitions(DubBass PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(DubBass
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# App de teste: toca uma linha de baixo, confere o maximizer e salva docs/interface.png
if(DUBBASS_TESTS)
    juce_add_console_app(DubBassTest PRODUCT_NAME "DubBassTest")
    target_sources(DubBassTest PRIVATE
        tests/render_test.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        ${MRB_MACHINE_SOURCES})
    target_include_directories(DubBassTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ReggaeKeys/Source)
    target_compile_definitions(DubBassTest PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0 JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Reggae Dub Bass Instrument MRDBI-07" JucePlugin_VersionString="${PROJECT_VERSION}"
        JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(DubBassTest PRIVATE juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
