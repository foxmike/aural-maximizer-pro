cmake_minimum_required(VERSION 3.22)

project(AuralMaximizerPro VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(JUCE)

juce_add_plugin(AuralMaximizerPro
    COMPANY_NAME nSpace
    PLUGIN_CODE Axpr
    FORMATS Standalone VST3 LV2
    LV2URI urn:nspace:aural-maximizer-pro
    PRODUCT_NAME "Aural Maximizer Pro")

juce_generate_juce_header(AuralMaximizerPro)

target_sources(AuralMaximizerPro
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/DSP/SonicMaximizer.h
        Source/DSP/SonicMaximizer.cpp
        Source/DSP/AuralExciter.h
        Source/DSP/AuralExciter.cpp
        Source/DSP/RoutingEngine.h
        Source/DSP/RoutingEngine.cpp
        Source/GUI/CustomLookAndFeel.h
        Source/GUI/CustomLookAndFeel.cpp
        Source/GUI/SpectrumVisualizer.h
        Source/GUI/SpectrumVisualizer.cpp
        Source/GUI/MeterComponent.h
        Source/GUI/MeterComponent.cpp)

target_compile_definitions(AuralMaximizerPro
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AuralMaximizerPro
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra)
