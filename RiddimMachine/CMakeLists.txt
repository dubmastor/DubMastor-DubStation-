cmake_minimum_required(VERSION 3.22)
# binário universal no Mac (Apple Silicon + Intel); precisa vir antes do project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Mastor Riddim Machine MRM-0420 (DubStation): MRDM-12 + MRCM-1240 + MRBM-69 + Mastor Big Knob num só instrumento
# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(MastorRiddimMachine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# formatos: VST3 (Cubase, FL Studio, Ableton, Reaper, Studio One, Bitwig...), AU (Logic, GarageBand),
# AAX (Pro Tools; o SDK do AAX já vem dentro do JUCE 8) e Standalone.
# No Linux use -DMRM_FORMATS=VST3 para testar (AU e AAX não existem no Linux).
if(APPLE)
    set(MRM_DEFAULT_FORMATS VST3 AU AAX Standalone)
elseif(WIN32)
    set(MRM_DEFAULT_FORMATS VST3 AAX Standalone)
else()
    set(MRM_DEFAULT_FORMATS VST3 Standalone)
endif()
set(MRM_FORMATS ${MRM_DEFAULT_FORMATS} CACHE STRING "Formatos do plugin")

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


juce_add_plugin(MastorRiddimMachine
    COMPANY_NAME                "DubStation"
    BUNDLE_ID                   com.dubmastor.mrm0420
    PRODUCT_NAME                "Mastor Riddim Machine MRM-0420"
    PLUGIN_MANUFACTURER_CODE    Dblb
    PLUGIN_CODE                 M420
    FORMATS                     ${MRM_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Generator
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    AAX_CATEGORY                AAX_ePlugInCategory_SWGenerators
    PLUGIN_AAX_IDENTIFIER       com.dubmastor.mrm0420.aax)

# O código de cada máquina é usado direto da pasta dela (nada copiado): o que mudar no MRCM-1240, no
# MRDM-12, no MRBM-69 ou no Big Knob entra aqui no próximo build. As versões aparecem na aba RIDDIM.
function(mrm_version dir out)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../${dir}/CMakeLists.txt" line REGEX "^project\\(")
    string(REGEX MATCH "VERSION ([0-9]+)\\.([0-9]+)" _ "${line}")
    set(${out} "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" PARENT_SCOPE)
endfunction()
mrm_version(DubDrums    MRM_DRUMS_VER)
mrm_version(DubChords   MRM_CHORDS_VER)
mrm_version(BassMachine MRM_BASS_VER)
mrm_version(BigKnob     MRM_KNOB_VER)
message(STATUS "MRM-0420 ${PROJECT_VERSION}: MRDM-12 ${MRM_DRUMS_VER}, MRCM-1240 ${MRM_CHORDS_VER}, MRBM-69 ${MRM_BASS_VER}, Big Knob ${MRM_KNOB_VER}")

set(MRM_SOURCES
    Source/MrmProcessor.cpp
    Source/MrmEditor.cpp
    Source/MrmMain.cpp
    Source/ChordsProcessorUnit.cpp
    Source/ChordsEditorUnit.cpp
    Source/DrumsProcessorUnit.cpp
    Source/DrumsEditorUnit.cpp
    Source/BassProcessorUnit.cpp
    Source/BassEditorUnit.cpp
    Source/KnobProcessorUnit.cpp
    Source/KnobEditorUnit.cpp
    ../ReggaeDrumMachine/Source/PatternGenerator.cpp)
set(MRM_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/../RootsKeys/Source)     # timbres do piano M1 / Hammond
set(MRM_DEFINES
    DD_VERSION_SHORT="${MRM_DRUMS_VER}"
    MRM_DRUMS_VER="${MRM_DRUMS_VER}"
    MRM_CHORDS_VER="${MRM_CHORDS_VER}"
    MRM_BASS_VER="${MRM_BASS_VER}"
    MRM_KNOB_VER="${MRM_KNOB_VER}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_sources(MastorRiddimMachine PRIVATE ${MRM_SOURCES})
target_include_directories(MastorRiddimMachine PRIVATE ${MRM_INCLUDES})
target_compile_definitions(MastorRiddimMachine PUBLIC ${MRM_DEFINES})

if(UNIX AND NOT APPLE)
    # teste de compilação no Linux sem ALSA/JACK/Xrandr/Xcursor
    target_compile_definitions(MastorRiddimMachine PUBLIC JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(MastorRiddimMachine
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Teste sem DAW: PNG de cada aba, o MIDI do riddim e o áudio do PLAY RIDDIM
# cmake -B build -DMRM_RENDER_TEST=ON && cmake --build build --target mrm_render
option(MRM_RENDER_TEST "Compila o programa de teste offline" OFF)
if(MRM_RENDER_TEST)
    juce_add_console_app(mrm_render PRODUCT_NAME "mrm_render")
    target_sources(mrm_render PRIVATE tests/render_test.cpp ${MRM_SOURCES})
    target_include_directories(mrm_render PRIVATE ${MRM_INCLUDES})
    target_compile_definitions(mrm_render PRIVATE ${MRM_DEFINES}
        JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_ALSA=0 JUCE_JACK=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JucePlugin_Name="Mastor Riddim Machine MRM-0420" JucePlugin_VersionString="${PROJECT_VERSION}" JucePlugin_IsSynth=1
        JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=1 JucePlugin_IsMidiEffect=0)
    target_link_libraries(mrm_render PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
