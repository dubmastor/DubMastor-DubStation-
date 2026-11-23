cmake_minimum_required(VERSION 3.22)
# binário universal no Mac (Apple Silicon + Intel); precisa vir antes do project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

# Mastor Dub Battery Reggae DBR: sampler de bateria com 16 pads + aba Dub Drums (MRDM-12)
project(DubBatteryReggae VERSION 1.3.0)

# versão curta para a plaqueta (ex.: "1.2") e a do MRDM-12 embutido (lida da pasta dele)
set(DBR_VERSION_SHORT "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../ReggaeDrumMachine/CMakeLists.txt" _dd_line REGEX "^project\\(")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../ReggaeDrumMachine/CMakeLists.txt")
string(REGEX MATCH "VERSION ([0-9]+)\\.([0-9]+)" _ "${_dd_line}")
set(DBR_DRUMS_VER "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# formatos (no Linux use -DDBR_FORMATS=VST3 para testar)
set(DBR_FORMATS VST3 AU Standalone CACHE STRING "Formatos do plugin")

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

juce_add_plugin(DubBatteryReggae
    COMPANY_NAME                "DubMastor"
    BUNDLE_ID                   com.dubmastor.dubbatteryreggae
    PRODUCT_NAME                "Mastor Dub Battery Reggae DBR"
    PLUGIN_MANUFACTURER_CODE    Dblb
    PLUGIN_CODE                 Dbr1
    FORMATS                     ${DBR_FORMATS} ${DM_AAX}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Drum Sampler
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    AAX_CATEGORY                AAX_ePlugInCategory_SWGenerators
    PLUGIN_AAX_IDENTIFIER       com.dubmastor.dbr.aax)

# O código do Dub Drums (MRDM-12) é usado direto da pasta dele (nada copiado)
set(DBR_SOURCES
    Source/BatteryProcessor.cpp
    Source/BatteryEditor.cpp
    Source/DrumsProcessorUnit.cpp
    Source/DrumsEditorUnit.cpp
    ../ReggaeDrumMachine/Source/PatternGenerator.cpp)
set(DBR_DEFINES
    DBR_VERSION_SHORT="${DBR_VERSION_SHORT}"
    DD_VERSION_SHORT="${DBR_DRUMS_VER}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_sources(DubBatteryReggae PRIVATE ${DBR_SOURCES})
target_include_directories(DubBatteryReggae PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_compile_definitions(DubBatteryReggae PUBLIC ${DBR_DEFINES})

if(UNIX AND NOT APPLE)
    # teste de compilação no Linux sem ALSA/JACK/Xrandr/Xcursor
    target_compile_definitions(DubBatteryReggae PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(DubBatteryReggae
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Teste sem DAW: PNG de cada aba, áudio dos pads e do groove, estado e presets
# cmake -B build -DDBR_RENDER_TEST=ON && cmake --build build --target dbr_render
option(DBR_RENDER_TEST "Compila o programa de teste offline" OFF)
if(DBR_RENDER_TEST)
    juce_add_console_app(dbr_render PRODUCT_NAME "dbr_render")
    target_sources(dbr_render PRIVATE tests/render_test.cpp ${DBR_SOURCES})
    target_include_directories(dbr_render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    target_compile_definitions(dbr_render PRIVATE ${DBR_DEFINES}
        JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_ALSA=0 JUCE_JACK=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Dub Battery Reggae DBR" JucePlugin_VersionString="${PROJECT_VERSION}" JucePlugin_IsSynth=1
        JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=1 JucePlugin_IsMidiEffect=0)
    target_link_libraries(dbr_render PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
