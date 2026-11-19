cmake_minimum_required (VERSION 3.22)

set (CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

project (TriVerseChorus VERSION 0.1.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON) # for IntelliSense / clangd

# Windows: link the C++ runtime statically so users don't need to install the
# Visual C++ Redistributable.
if (MSVC)
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option (TRICHORUS_BUILD_TESTS "Build DSP unit tests" ON)
option (TRICHORUS_COPY_PLUGIN "Copy plugin to ~/Library/Audio/Plug-Ins after build" ON)

# ------------------------------------------------------------------------------
# JUCE: use a local copy if given (-DJUCE_DIR=...), otherwise download it.
set (JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout")

if (JUCE_DIR)
    add_subdirectory ("${JUCE_DIR}" JUCE)
else()
    include (FetchContent)
    FetchContent_Declare (JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.15
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable (JUCE)
endif()

# ------------------------------------------------------------------------------
# Pure DSP library (no JUCE) — used by the plugin and the unit tests.
add_library (trichorus_dsp STATIC Source/dsp/TriChorusEngine.cpp)
target_include_directories (trichorus_dsp PUBLIC Source)
set_target_properties (trichorus_dsp PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
juce_add_plugin (TriVerseChorus
    PRODUCT_NAME            "TriVerse Chorus"
    COMPANY_NAME            "TriVerse Audio"
    BUNDLE_ID               "com.triverseaudio.chorus"
    PLUGIN_MANUFACTURER_CODE Tvrs
    PLUGIN_CODE             Tvc3
    FORMATS                 AU VST3 Standalone
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES         Fx Modulation
    AU_MAIN_TYPE            kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD ${TRICHORUS_COPY_PLUGIN})

target_sources (TriVerseChorus PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/gui/TriVerseLookAndFeel.cpp
    Source/gui/PanelWidgets.cpp)

target_compile_definitions (TriVerseChorus PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries (TriVerseChorus
    PRIVATE
        trichorus_dsp
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ------------------------------------------------------------------------------
if (TRICHORUS_BUILD_TESTS)
    enable_testing()
    add_executable (trichorus_tests tests/DspUnitTests.cpp)
    target_link_libraries (trichorus_tests PRIVATE trichorus_dsp)
    add_test (NAME dsp COMMAND trichorus_tests)
endif()

# ------------------------------------------------------------------------------
# GUI-to-PNG render tool (for reviewing the look without a DAW).
option (TRICHORUS_BUILD_TOOLS "Build developer tools (GUI snapshot)" OFF)

if (TRICHORUS_BUILD_TOOLS)
    juce_add_console_app (trichorus_snapshot PRODUCT_NAME "trichorus_snapshot")
    target_sources (trichorus_snapshot PRIVATE
        tools/Snapshot.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/gui/TriVerseLookAndFeel.cpp
        Source/gui/PanelWidgets.cpp)
    target_include_directories (trichorus_snapshot PRIVATE Source)
    target_compile_definitions (trichorus_snapshot PRIVATE
        JucePlugin_Name="TriVerse Chorus"
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries (trichorus_snapshot PRIVATE
        trichorus_dsp
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_recommended_config_flags)
endif()
