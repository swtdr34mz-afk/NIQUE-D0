# NIQUE D0: the plugin (VST3, AU on the Mac) and the standalone app, with JUCE 8.
#   cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release
# JUCE comes from GitHub (or -DNQ_JUCE_DIR=/path/to/JUCE).
cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    endif()
endif()
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(NIQUED0 VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NQ_JUCE_DIR "" CACHE PATH "a JUCE checkout (empty: from GitHub)")
if(NQ_JUCE_DIR)
    add_subdirectory(${NQ_JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
endif()

set(NQ_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND NQ_FORMATS AU)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND NQ_FORMATS LV2)
endif()

juce_add_plugin(NiqueD0
    PRODUCT_NAME "NIQUE D0"
    COMPANY_NAME "swtdr34mz"
    BUNDLE_ID "com.swtdr34mz.niqued0"
    LV2URI "https://github.com/swtdr34mz-afk/NIQUE-D0"
    PLUGIN_MANUFACTURER_CODE Swtd
    PLUGIN_CODE Nqd0
    FORMATS ${NQ_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    MICROPHONE_PERMISSION_ENABLED FALSE)

juce_add_binary_data(NqData SOURCES
    fonts/FiraSans-SemiBold.ttf
    fonts/FiraSans-Bold.ttf
    fonts/FiraSans-ExtraBold.ttf
    fonts/AnonymousPro-Bold.ttf
    res/swtdr34mz-tag.png)

target_sources(NiqueD0 PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/ui/Look.cpp
    src/ui/Controls.cpp
    dsp/engine.cpp
    dsp/fx.cpp
    dsp/params.cpp
    dsp/presets.cpp
    dsp/sound.cpp
    dsp/waves.cpp)

target_include_directories(NiqueD0 PRIVATE dsp src src/ui)

target_compile_definitions(NiqueD0 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(NiqueD0
    PRIVATE
        NqData
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
