cmake_minimum_required(VERSION 3.22)
project(one_bite VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Download JUCE via FetchContent
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Plugin definition — metadata from one_bite.jucer
juce_add_plugin(one_bite
    PRODUCT_NAME              "one_bite"
    COMPANY_NAME              "GRAME"
    BUNDLE_ID                 "com.grame.one_bite"
    PLUGIN_MANUFACTURER_CODE  Manu
    PLUGIN_CODE               b009
    FORMATS                   VST3 AU Standalone
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         TRUE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
)

# Generate JuceHeader.h (required by the Faust-generated source)
juce_generate_juce_header(one_bite)

# Source files
target_sources(one_bite
    PRIVATE
        one_bite/FaustPluginProcessor.cpp
)

# Include path fix for Faust-generated #include directives:
#   Line 43:    #include "JuceLibraryCode/JuceHeader.h"      (needs parent of JuceLibraryCode/)
#   Lines 10K+: #include "../JuceLibraryCode/JuceHeader.h"   (needs grandparent of JuceLibraryCode/)
# JUCE CMake already adds <build>/one_bite_artefacts/JuceLibraryCode/ as include dir.
# Adding <build>/one_bite_artefacts/ resolves "JuceLibraryCode/JuceHeader.h".
# Adding <build>/ resolves "../JuceLibraryCode/JuceHeader.h" (from one_bite/ source dir context).
target_include_directories(one_bite
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/one_bite_artefacts"
)

# Compile definitions matching .jucer
target_compile_definitions(one_bite
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    PRIVATE
        MIDICTRL
        NVOICES=16
        FAUSTFLOAT=float
)

# Platform-specific compiler flags
if(MSVC)
    # 109K-line file exceeds default COFF section limit
    target_compile_options(one_bite PRIVATE /bigobj)
else()
    # Suppress warnings from Faust-generated code
    target_compile_options(one_bite PRIVATE -Wno-unused-parameter -Wno-sign-compare)
endif()

# Link all 14 JUCE modules used by the project
target_link_libraries(one_bite
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_opengl
        juce::juce_osc
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
