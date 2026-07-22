#pragma once

#include <memory>
#include "PluginProcessor.h"
#include "GUI/CustomLookAndFeel.h"
#include "GUI/SpectrumVisualizer.h"
#include "GUI/MeterComponent.h"

//==============================================================================
class AuralMaximizerProAudioProcessorEditor final
    : public juce::AudioProcessorEditor {
public:
  static constexpr int editorWidth = 920;
  static constexpr int editorHeight = 540;

  explicit AuralMaximizerProAudioProcessorEditor(AuralMaximizerProAudioProcessor&);
  ~AuralMaximizerProAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics&) override;
  void resized() override;

private:
  AuralMaximizerProAudioProcessor& processorRef;

  // Custom Look and Feel
  CustomLookAndFeel customLookAndFeel;

  // Visualizers
  SpectrumVisualizer spectrumVisualizer;
  MeterComponent meterComponent;

  // --- UI Components ---
  
  // Maximizer Section Controls
  juce::ToggleButton maxEnableButton;
  juce::Slider maxProcessSlider;
  juce::Slider maxContourSlider;
  juce::Slider maxLowFreqSlider;
  juce::Slider maxHighFreqSlider;
  juce::ComboBox maxPhaseModeComboBox;

  // Exciter Section Controls
  juce::ToggleButton excEnableButton;
  juce::Slider excTuneSlider;
  juce::Slider excDriveSlider;
  juce::Slider excMixSlider;
  juce::ComboBox excModeComboBox;
  juce::Slider excHarmonicsSlider;
  juce::Slider excAirGainSlider;

  // Routing, Mix, and Global Controls
  juce::ComboBox routingModeComboBox;
  juce::Slider parallelBlendSlider;
  juce::Slider masterInGainSlider;
  juce::Slider masterOutGainSlider;
  juce::Slider masterMixSlider;
  juce::ComboBox oversamplingComboBox;

  // --- APVTS Attachments ---
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
  using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

  // Maximizer Attachments
  std::unique_ptr<ButtonAttachment> maxEnableAttachment;
  std::unique_ptr<SliderAttachment> maxProcessAttachment;
  std::unique_ptr<SliderAttachment> maxContourAttachment;
  std::unique_ptr<SliderAttachment> maxLowFreqAttachment;
  std::unique_ptr<SliderAttachment> maxHighFreqAttachment;
  std::unique_ptr<ComboBoxAttachment> maxPhaseModeAttachment;

  // Exciter Attachments
  std::unique_ptr<ButtonAttachment> excEnableAttachment;
  std::unique_ptr<SliderAttachment> excTuneAttachment;
  std::unique_ptr<SliderAttachment> excDriveAttachment;
  std::unique_ptr<SliderAttachment> excMixAttachment;
  std::unique_ptr<ComboBoxAttachment> excModeAttachment;
  std::unique_ptr<SliderAttachment> excHarmonicsAttachment;
  std::unique_ptr<SliderAttachment> excAirGainAttachment;

  // Routing, Mix, and Global Attachments
  std::unique_ptr<ComboBoxAttachment> routingModeAttachment;
  std::unique_ptr<SliderAttachment> parallelBlendAttachment;
  std::unique_ptr<SliderAttachment> masterInGainAttachment;
  std::unique_ptr<SliderAttachment> masterOutGainAttachment;
  std::unique_ptr<SliderAttachment> masterMixAttachment;
  std::unique_ptr<ComboBoxAttachment> oversamplingAttachment;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuralMaximizerProAudioProcessorEditor)
};

using PluginEditor = AuralMaximizerProAudioProcessorEditor;
