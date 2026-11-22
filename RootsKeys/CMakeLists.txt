cmake_minimum_required(VERSION 3.22)

# precisam vir ANTES do project(): binário universal (Apple Silicon + Intel)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(RootsKeys VERSION 2.3.0)

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

juce_add_plugin(RootsKeys
    COMPANY_NAME "DubMastor"
    BUNDLE_ID com.dubmastor.rootskeys
    PLUGIN_MANUFACTURER_CODE Dblb
    PLUGIN_CODE Rtky
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Mastor Roots Keys MRK-80"
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

target_sources(RootsKeys PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DubChordsPage.cpp)

target_include_directories(RootsKeys PRIVATE Source)

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
