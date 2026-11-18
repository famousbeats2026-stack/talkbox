cmake_minimum_required(VERSION 3.22)

# Famous Beats Talk Box — a playable MIDI talk box plugin.
# Play it from a MIDI keyboard: MIDI notes drive a built-in synth
# carrier through a formant (vowel) shaping engine, so it talks like
# a talk box — Roger Troutman / Zapp style, G-funk talkbox sound.
# Build on a Mac (see README.md) or let GitHub Actions build it for you.

# Target machine: macOS 10.15 (Catalina) and newer, Intel and Apple Silicon.
# These MUST be set before project() — the Xcode generator ignores them
# when set after project(). The GitHub workflow also passes them on the
# cmake command line, which is the belt-and-braces guarantee.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary")

project(FamousBeatsTalkBox VERSION 1.0.0)

# ---- JUCE (downloaded automatically at configure time, not bundled) ----
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
)
FetchContent_MakeAvailable(JUCE)

# ---- The plugin ----
# An audio effect with MIDI input: the mic (audio in) is the modulator
# and MIDI notes drive the built-in carrier synth. Talk Box mode makes
# its own sound from MIDI alone; Vocoder mode needs both the mic and
# the keys at the same time.
juce_add_plugin(FamousBeatsTalkBox
    COMPANY_NAME "Famous Beats Studio"
    PRODUCT_NAME "Famous Beats Talk Box"
    BUNDLE_ID com.famousbeatsstudio.talkbox
    PLUGIN_MANUFACTURER_CODE FbSt
    PLUGIN_CODE FBTB
    FORMATS VST3 AU
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_MusicEffect
)

# Generate JuceHeader.h (needed because the sources #include <JuceHeader.h>).
juce_generate_juce_header(FamousBeatsTalkBox)

target_sources(FamousBeatsTalkBox PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(FamousBeatsTalkBox
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(FamousBeatsTalkBox
    PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ---- Test harness (Linux only, off by default) ----
# Builds test/talkbox_test.cpp against the plugin target and feeds MIDI
# notes through the full chain: audibility, vowel spectra, morph,
# Auto-Talk, silence, presets, glide/mono.
# Enable with: cmake -B build -DFBS_BUILD_TESTS=ON
option(FBS_BUILD_TESTS "Build the talk box test harness" OFF)
if(FBS_BUILD_TESTS)
    add_executable(talkbox_test test/talkbox_test.cpp)
    target_link_libraries(talkbox_test PRIVATE
        FamousBeatsTalkBox
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp)
    target_include_directories(talkbox_test PRIVATE
        ${CMAKE_BINARY_DIR}/FamousBeatsTalkBox_artefacts/JuceLibraryCode)
endif()
