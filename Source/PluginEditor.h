#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#include "GUI/CustomLookAndFeel.h"
#include "GUI/MeterComponent.h"
#include "GUI/SpectrumVisualizer.h"
#include "PluginProcessor.h"

//==============================================================================
/**
 * PluginEditor for AuralMaximizerPro.
 * Fixed editor dimensions: 920x540 (46:27 aspect ratio).
 * Manages custom look-and-feel, visualizer components, and APVTS attachments
 * for all 19 parameter controls.
 */
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    static constexpr int defaultWidth = 920;
    static constexpr int defaultHeight = 540;

    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    PluginProcessor& audioProcessor;

    // Custom LookAndFeel instance
    CustomLookAndFeel customLookAndFeel;

    // Embedded visualizer and meter components
    SpectrumVisualizer spectrumVisualizer;
    MeterComponent meterComponent;

    // Controls: Maximizer section (MAX_ENABLE, MAX_PROCESS, MAX_CONTOUR, MAX_LOW_FREQ, MAX_HIGH_FREQ, MAX_PHASE_MODE)
    juce::ToggleButton maxEnableToggle;
    juce::Slider maxProcessSlider;
    juce::Slider maxContourSlider;
    juce::Slider maxLowFreqSlider;
    juce::Slider maxHighFreqSlider;
    juce::ComboBox maxPhaseModeBox;

    // Controls: Exciter section (EXC_ENABLE, EXC_TUNE, EXC_DRIVE, EXC_MIX, EXC_MODE, EXC_HARMONICS, EXC_AIR_GAIN)
    juce::ToggleButton excEnableToggle;
    juce::Slider excTuneSlider;
    juce::Slider excDriveSlider;
    juce::Slider excMixSlider;
    juce::ComboBox excModeBox;
    juce::Slider excHarmonicsSlider;
    juce::Slider excAirGainSlider;

    // Controls: Routing & Master section (ROUTING_MODE, PARALLEL_BLEND, MASTER_IN_GAIN, MASTER_OUT_GAIN, MASTER_MIX, OVERSAMPLING)
    juce::ComboBox routingModeBox;
    juce::Slider parallelBlendSlider;
    juce::Slider masterInGainSlider;
    juce::Slider masterOutGainSlider;
    juce::Slider masterMixSlider;
    juce::ComboBox oversamplingBox;

    // APVTS Attachments for all 19 parameters
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttachment> maxEnableAttachment;
    std::unique_ptr<SliderAttachment> maxProcessAttachment;
    std::unique_ptr<SliderAttachment> maxContourAttachment;
    std::unique_ptr<SliderAttachment> maxLowFreqAttachment;
    std::unique_ptr<SliderAttachment> maxHighFreqAttachment;
    std::unique_ptr<ComboBoxAttachment> maxPhaseModeAttachment;

    std::unique_ptr<ButtonAttachment> excEnableAttachment;
    std::unique_ptr<SliderAttachment> excTuneAttachment;
    std::unique_ptr<SliderAttachment> excDriveAttachment;
    std::unique_ptr<SliderAttachment> excMixAttachment;
    std::unique_ptr<ComboBoxAttachment> excModeAttachment;
    std::unique_ptr<SliderAttachment> excHarmonicsAttachment;
    std::unique_ptr<SliderAttachment> excAirGainAttachment;

    std::unique_ptr<ComboBoxAttachment> routingModeAttachment;
    std::unique_ptr<SliderAttachment> parallelBlendAttachment;
    std::unique_ptr<SliderAttachment> masterInGainAttachment;
    std::unique_ptr<SliderAttachment> masterOutGainAttachment;
    std::unique_ptr<SliderAttachment> masterMixAttachment;
    std::unique_ptr<ComboBoxAttachment> oversamplingAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
