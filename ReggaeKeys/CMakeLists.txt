cmake_minimum_required(VERSION 3.22)

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(RootsKeys VERSION 3.4.0)

set(CMAKE_CXX_STANDARD 17)

option(ROOTSKEYS_TESTS "Compila tambem o app de teste (CHORD MACHINE no som do instrumento, projeto antigo e PNG da interface)" OFF)

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

juce_add_plugin(RootsKeys
    COMPANY_NAME "DubMastor"
    BUNDLE_ID com.dubmastor.rootskeys
    PLUGIN_MANUFACTURER_CODE Dblb
    PLUGIN_CODE Rtky
    FORMATS VST3 AU Standalone ${DM_AAX}
    PRODUCT_NAME "Mastor Reggae Keys MRK-80"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth)

# fontes embutidas: Archivo Black (placa), Michroma (título piano), RootsKeys Display (título órgão)
juce_add_binary_data(RootsKeysFonts SOURCES
    Resources/ArchivoBlack-Regular.ttf
    Resources/Michroma-Regular.ttf
    Resources/RootsKeysDisplay-BoldItalic.ttf)

# aba CHORD MACHINE: o Mastor Reggae Chord Machine MRCM-1240 compilado direto da pasta dele (nada copiado)
set(MRK_MACHINE_SOURCES Source/MachineProcessorUnit.cpp Source/MachineEditorUnit.cpp)
target_sources(RootsKeys PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    ${MRK_MACHINE_SOURCES})

target_include_directories(RootsKeys PRIVATE Source)
if(UNIX AND NOT APPLE)
    # teste de compilação no Linux sem ALSA/JACK/Xrandr/Xcursor (como no Mastor Reggae Bass)
    target_compile_definitions(RootsKeys PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_compile_definitions(RootsKeys PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(RootsKeys
    PRIVATE RootsKeysFonts juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

# App de teste: a CHORD MACHINE tocando no som do instrumento, um projeto antigo (aba DUB CHORDS) abrindo
# e as capturas da interface (docs/interface.png e docs/interface-chord-machine.png)
if(ROOTSKEYS_TESTS)
    juce_add_console_app(RootsKeysTest PRODUCT_NAME "RootsKeysTest")
    target_sources(RootsKeysTest PRIVATE
        tests/plugin_test.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        ${MRK_MACHINE_SOURCES})
    target_include_directories(RootsKeysTest PRIVATE Source)
    target_compile_definitions(RootsKeysTest PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0 JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Reggae Keys MRK-80" JucePlugin_VersionString="${PROJECT_VERSION}"
        JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(RootsKeysTest PRIVATE RootsKeysFonts juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

    # teste de automação no estilo do pluginval (parâmetros aleatórios a cada 32 amostras + MIDI aleatório)
    juce_add_console_app(RootsKeysStress PRODUCT_NAME "RootsKeysStress")
    target_sources(RootsKeysStress PRIVATE
        tests/automation_stress.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        ${MRK_MACHINE_SOURCES})
    target_include_directories(RootsKeysStress PRIVATE Source)
    target_compile_definitions(RootsKeysStress PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0 JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Reggae Keys MRK-80" JucePlugin_VersionString="${PROJECT_VERSION}"
        JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(RootsKeysStress PRIVATE RootsKeysFonts juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
