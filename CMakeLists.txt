cmake_minimum_required(VERSION 3.24)
project(QuadBeatFX VERSION 0.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(QB_PRODUCT_NAME "QuadBeat FX" CACHE STRING "Product display name")
set(QB_COMPANY_NAME "QuadBeat Audio" CACHE STRING "Company display name")
set(QB_BUNDLE_ID "com.quadbeat.fx" CACHE STRING "macOS bundle identifier")
option(QB_ENABLE_ASIO "Enable JUCE ASIO support on Windows" ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    message(FATAL_ERROR "JUCE 8.0.13 is missing. Run git submodule update --init --recursive")
endif()

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "" FORCE)
endif()

add_subdirectory(external/JUCE EXCLUDE_FROM_ALL)
include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)

set(QB_CORE_SOURCES
    src/audio/MixerEngine.cpp
    src/bpm/TempoEngine.cpp
    src/dsp/BeatClock.cpp
    src/dsp/EffectRack.cpp
    src/midi/MidiMapper.cpp
    src/state/StateStore.cpp)

set(QB_UI_SOURCES
    src/ui/BeatFxDisplay.cpp
    src/ui/BeatFxDisplay.h
    src/ui/HardwareRotarySelector.cpp
    src/ui/HardwareRotarySelector.h
    src/ui/MidiMappingEditor.cpp
    src/ui/MidiMappingEditor.h
    src/ui/MainComponent.cpp
    src/ui/MainComponent.h
    src/ui/MainComponentLayout.cpp
    src/ui/QuadBeatLookAndFeel.cpp
    src/ui/QuadBeatLookAndFeel.h
    src/ui/SettingsComponent.cpp
    src/ui/SettingsComponent.h)

add_library(QuadBeatCore STATIC ${QB_CORE_SOURCES})
set_target_properties(QuadBeatCore PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(QuadBeatCore PUBLIC src)
target_compile_definitions(QuadBeatCore PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0
    $<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${QB_ENABLE_ASIO}>>:JUCE_ASIO=1>)
target_link_libraries(QuadBeatCore PUBLIC
    juce::juce_audio_basics juce::juce_audio_devices juce::juce_audio_formats
    juce::juce_audio_processors juce::juce_core juce::juce_data_structures juce::juce_dsp)
qb_enable_warnings(QuadBeatCore)
qb_enable_sanitizers(QuadBeatCore)

add_library(QuadBeatUI STATIC ${QB_UI_SOURCES})
set_target_properties(QuadBeatUI PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(QuadBeatUI PUBLIC src)
target_link_libraries(QuadBeatUI PUBLIC QuadBeatCore juce::juce_audio_utils juce::juce_gui_extra)
qb_enable_warnings(QuadBeatUI)
qb_enable_sanitizers(QuadBeatUI)

juce_add_gui_app(QuadBeatFX
    PRODUCT_NAME "${QB_PRODUCT_NAME}"
    COMPANY_NAME "${QB_COMPANY_NAME}"
    BUNDLE_ID "${QB_BUNDLE_ID}"
    VERSION "${PROJECT_VERSION}"
    NEEDS_MIDI_INPUT TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE)
target_sources(QuadBeatFX PRIVATE
    src/app/Main.cpp src/app/QuadBeatApplication.cpp src/app/QuadBeatApplication.h)
target_link_libraries(QuadBeatFX PRIVATE QuadBeatUI)
target_compile_definitions(QuadBeatFX PRIVATE JUCE_APPLICATION_NAME_STRING="${QB_PRODUCT_NAME}"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}")
qb_enable_warnings(QuadBeatFX)
qb_enable_sanitizers(QuadBeatFX)

juce_add_plugin(QuadBeatFXPlugin
    PRODUCT_NAME "${QB_PRODUCT_NAME}"
    COMPANY_NAME "${QB_COMPANY_NAME}"
    BUNDLE_ID "${QB_BUNDLE_ID}.plugin"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_MANUFACTURER_CODE QbAu
    PLUGIN_CODE QbFx
    FORMATS VST3
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(QuadBeatFXPlugin PRIVATE
    src/plugin/PluginEditor.cpp
    src/plugin/PluginEditor.h
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginProcessor.h)
target_include_directories(QuadBeatFXPlugin PRIVATE src)
target_link_libraries(QuadBeatFXPlugin PRIVATE QuadBeatUI)
target_compile_definitions(QuadBeatFXPlugin PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
qb_enable_warnings(QuadBeatFXPlugin)
qb_enable_sanitizers(QuadBeatFXPlugin)

juce_add_console_app(QuadBeatFXTests PRODUCT_NAME "QuadBeat FX Tests")
target_sources(QuadBeatFXTests PRIVATE tests/TestMain.cpp)
target_link_libraries(QuadBeatFXTests PRIVATE QuadBeatCore)
qb_enable_warnings(QuadBeatFXTests)
qb_enable_sanitizers(QuadBeatFXTests)

juce_add_console_app(QuadBeatFXPluginTests PRODUCT_NAME "QuadBeat FX Plugin Tests")
target_sources(QuadBeatFXPluginTests PRIVATE tests/PluginHostSmoke.cpp)
target_link_libraries(QuadBeatFXPluginTests PRIVATE juce::juce_audio_utils)
target_compile_definitions(QuadBeatFXPluginTests PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0)
qb_enable_warnings(QuadBeatFXPluginTests)
qb_enable_sanitizers(QuadBeatFXPluginTests)

enable_testing()
add_test(NAME QuadBeatFXTests COMMAND QuadBeatFXTests)
add_test(NAME QuadBeatFXPluginTests COMMAND QuadBeatFXPluginTests
    "${CMAKE_CURRENT_BINARY_DIR}/QuadBeatFXPlugin_artefacts/$<CONFIG>/VST3/${QB_PRODUCT_NAME}.vst3")
install(TARGETS QuadBeatFX RUNTIME DESTINATION . BUNDLE DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/QuadBeatFXPlugin_artefacts/$<CONFIG>/VST3/${QB_PRODUCT_NAME}.vst3"
        DESTINATION VST3 OPTIONAL)
install(FILES README.md INSTALL-VST3-WINDOWS.txt LICENSE.md THIRD_PARTY_NOTICES.md DESTINATION .)
include(cmake/Packaging.cmake)
