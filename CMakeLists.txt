cmake_minimum_required(VERSION 3.22)
project(AuralMaximizerPro VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(AuralMaximizerPro
    COMPANY_NAME "nSpace"
    PLUGIN_CODE "Axpr"
    PLUGIN_MANUFACTURER_CODE "nSpc"
    FORMATS Standalone VST3 CLAP LV2
    PRODUCT_NAME "AuralMaximizerPro"
)

juce_generate_juce_header(AuralMaximizerPro)

target_sources(AuralMaximizerPro
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/BandSplitter.cpp
        Source/DSP/DynamicsProcessor.cpp
        Source/DSP/HarmonicExciter.cpp
        Source/DSP/LimiterEngine.cpp
        Source/DSP/MidSideProcessor.cpp
        Source/DSP/StereoWidener.cpp
        Source/GUI/BandControlComponent.cpp
        Source/GUI/LevelMeterComponent.cpp
        Source/GUI/MainPanel.cpp
        Source/GUI/SpectrumAnalyzer.cpp
        Source/GUI/TransportBar.cpp
        Source/State/PresetManager.cpp
        Source/State/StateSerializer.cpp
        Source/Util/FFTWrapper.cpp
        Source/Util/LUFSMeter.cpp
        Source/Util/ParameterLayout.cpp
)

target_compile_definitions(AuralMaximizerPro
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AuralMaximizerPro
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
