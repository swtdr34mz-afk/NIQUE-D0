# A picture of the plugin's panel without a host (for docs/panel.png and for checking the look):
#   cmake -S tools/panel -B build-panel -DNQ_JUCE_DIR=/path/to/JUCE && cmake --build build-panel
#   build-panel/nq_panel_artefacts/Release/nq_panel docs/panel.png
cmake_minimum_required(VERSION 3.22)
project(NqPanel VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(NQ_JUCE_DIR "" CACHE PATH "a JUCE checkout (empty: from GitHub)")
if(NQ_JUCE_DIR)
    add_subdirectory(${NQ_JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.9 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
endif()
set(R ${CMAKE_CURRENT_SOURCE_DIR}/../..)
juce_add_console_app(nq_panel PRODUCT_NAME "nq_panel")
juce_add_binary_data(NqPanelData SOURCES
    ${R}/fonts/FiraSans-SemiBold.ttf
    ${R}/fonts/FiraSans-Bold.ttf
    ${R}/fonts/FiraSans-ExtraBold.ttf
    ${R}/fonts/AnonymousPro-Bold.ttf
    ${R}/res/swtdr34mz-tag.png)
target_sources(nq_panel PRIVATE
    panel.cpp
    ${R}/src/PluginProcessor.cpp
    ${R}/src/PluginEditor.cpp
    ${R}/src/ui/Look.cpp
    ${R}/src/ui/Controls.cpp
    ${R}/dsp/engine.cpp
    ${R}/dsp/fx.cpp
    ${R}/dsp/params.cpp
    ${R}/dsp/presets.cpp
    ${R}/dsp/sound.cpp
    ${R}/dsp/waves.cpp)
target_include_directories(nq_panel PRIVATE ${R}/dsp ${R}/src ${R}/src/ui)
target_compile_definitions(nq_panel PRIVATE
    JucePlugin_Name="NIQUE D0" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0 JucePlugin_EditorRequiresKeyboardFocus=0
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0
    JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XSHM=0 JUCE_USE_XRENDER=0 JUCE_USE_XCURSOR=0
    JUCE_MODAL_LOOPS_PERMITTED=0)
target_link_libraries(nq_panel PRIVATE NqPanelData juce::juce_audio_utils juce::juce_recommended_config_flags)
