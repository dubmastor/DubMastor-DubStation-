cmake_minimum_required(VERSION 3.22)
# binário universal no Mac (Apple Silicon + Intel); precisa vir antes do project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

# Mastor Riddim Machine MRM-0420: Dub Chords + Dub Drums num só instrumento
# Windows: runtime do C++ embutido no plugin (não depende do Visual C++ Redistributable do PC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
project(MastorRiddimMachine VERSION 1.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# formatos (no Linux use -DMRM_FORMATS=VST3 para testar)
set(MRM_FORMATS VST3 AU Standalone CACHE STRING "Formatos do plugin")

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
    COMPANY_NAME                "DubMastor"
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
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice)

# O código do Dub Chords e do Dub Drums é usado direto das pastas deles (nada copiado):
# o que mudar lá entra aqui no próximo build.
set(MRM_SOURCES
    Source/MrmProcessor.cpp
    Source/MrmEditor.cpp
    Source/ChordsProcessorUnit.cpp
    Source/ChordsEditorUnit.cpp
    Source/DrumsProcessorUnit.cpp
    Source/DrumsEditorUnit.cpp
    ../DubDrumsMachine/Source/PatternGenerator.cpp)
set(MRM_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/../RootsKeys/Source)     # timbres do piano M1 / Hammond
set(MRM_DEFINES
    DD_VERSION_SHORT="1.2"
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
    target_link_libraries(mrm_render PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
