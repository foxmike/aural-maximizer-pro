#include "PluginEditor.h"

namespace {
constexpr int headerHeight = 40;
constexpr int upperSectionBottom = 374;

void configureSlider(juce::Slider &slider, const juce::String &name,
                     juce::Colour accent) {
  slider.setName(name);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
  slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
  slider.setColour(juce::Slider::thumbColourId, accent);
}

void configureComboBox(juce::ComboBox &comboBox, const juce::String &name,
                       const juce::StringArray &items, juce::Colour accent) {
  comboBox.setName(name);
  comboBox.addItemList(items, 1);
  comboBox.setColour(juce::ComboBox::outlineColourId, accent);
}

void configureButton(juce::ToggleButton &button, const juce::String &text,
                     juce::Colour accent) {
  button.setButtonText(text);
  button.setColour(juce::ToggleButton::tickColourId, accent);
}

void drawPanel(juce::Graphics &graphics, juce::Rectangle<int> bounds,
               juce::Colour accent) {
  const auto panel = bounds.toFloat();
  graphics.setColour(CustomLookAndFeel::Panel);
  graphics.fillRoundedRectangle(panel, 7.0f);
  graphics.setColour(accent.withAlpha(0.7f));
  graphics.drawRoundedRectangle(panel, 7.0f, 1.25f);
}

void drawCaption(juce::Graphics &graphics, juce::StringRef text,
                 juce::Rectangle<int> bounds, juce::Colour colour,
                 float fontHeight = 12.0f) {
  graphics.setColour(colour);
  graphics.setFont(juce::Font(juce::FontOptions{}.withHeight(fontHeight)));
  graphics.drawFittedText(text, bounds, juce::Justification::centred, 1);
}
} // namespace

AuralMaximizerProAudioProcessorEditor::AuralMaximizerProAudioProcessorEditor(
    AuralMaximizerProAudioProcessor &processor)
    : AudioProcessorEditor(&processor), processorRef(processor),
      spectrumVisualizer(processor), meterComponent(processor) {
  setLookAndFeel(&customLookAndFeel);
  setResizable(false, false);
  setSize(920, 540);

  configureButton(maxEnableButton, "MAXIMIZER ON",
                  CustomLookAndFeel::Maximizer);
  configureSlider(maxProcessSlider, "Process", CustomLookAndFeel::Maximizer);
  configureSlider(maxContourSlider, "Lo Contour", CustomLookAndFeel::Maximizer);
  configureSlider(maxLowFreqSlider, "Low Frequency",
                  CustomLookAndFeel::Maximizer);
  configureSlider(maxHighFreqSlider, "High Frequency",
                  CustomLookAndFeel::Maximizer);
  configureComboBox(maxPhaseModeComboBox, "Phase Mode", {"Classic", "Modern"},
                    CustomLookAndFeel::Maximizer);

  configureButton(excEnableButton, "EXCITER ON", CustomLookAndFeel::Exciter);
  configureSlider(excTuneSlider, "Tune", CustomLookAndFeel::Exciter);
  configureSlider(excDriveSlider, "Drive", CustomLookAndFeel::Exciter);
  configureSlider(excMixSlider, "Mix", CustomLookAndFeel::Exciter);
  configureComboBox(excModeComboBox, "Exciter Mode",
                    {"Cubic Soft Clipping", "Asymmetric Tube", "Tape"},
                    CustomLookAndFeel::Exciter);
  configureSlider(excHarmonicsSlider, "Harmonics", CustomLookAndFeel::Exciter);
  configureSlider(excAirGainSlider, "Air Gain", CustomLookAndFeel::Exciter);

  configureComboBox(
      routingModeComboBox, "Routing Mode",
      {"Maximizer -> Exciter", "Exciter -> Maximizer", "Parallel Matrix"},
      CustomLookAndFeel::Master);
  configureSlider(parallelBlendSlider, "Parallel Blend",
                  CustomLookAndFeel::Master);
  configureSlider(masterInGainSlider, "Input Gain", CustomLookAndFeel::Master);
  configureSlider(masterOutGainSlider, "Output Gain",
                  CustomLookAndFeel::Master);
  configureSlider(masterMixSlider, "Master Mix", CustomLookAndFeel::Master);
  configureComboBox(oversamplingComboBox, "Oversampling", {"2x", "4x"},
                    CustomLookAndFeel::Master);

  addAndMakeVisible(maxEnableButton);
  addAndMakeVisible(maxProcessSlider);
  addAndMakeVisible(maxContourSlider);
  addAndMakeVisible(maxLowFreqSlider);
  addAndMakeVisible(maxHighFreqSlider);
  addAndMakeVisible(maxPhaseModeComboBox);
  addAndMakeVisible(excEnableButton);
  addAndMakeVisible(excTuneSlider);
  addAndMakeVisible(excDriveSlider);
  addAndMakeVisible(excMixSlider);
  addAndMakeVisible(excModeComboBox);
  addAndMakeVisible(excHarmonicsSlider);
  addAndMakeVisible(excAirGainSlider);
  addAndMakeVisible(spectrumVisualizer);
  addAndMakeVisible(routingModeComboBox);
  addAndMakeVisible(parallelBlendSlider);
  addAndMakeVisible(masterInGainSlider);
  addAndMakeVisible(masterOutGainSlider);
  addAndMakeVisible(masterMixSlider);
  addAndMakeVisible(oversamplingComboBox);
  addAndMakeVisible(meterComponent);

  auto &state = processorRef.apvts;
  maxEnableAttachment =
      std::make_unique<ButtonAttachment>(state, "MAX_ENABLE", maxEnableButton);
  maxProcessAttachment = std::make_unique<SliderAttachment>(
      state, "MAX_PROCESS", maxProcessSlider);
  maxContourAttachment = std::make_unique<SliderAttachment>(
      state, "MAX_CONTOUR", maxContourSlider);
  maxLowFreqAttachment = std::make_unique<SliderAttachment>(
      state, "MAX_LOW_FREQ", maxLowFreqSlider);
  maxHighFreqAttachment = std::make_unique<SliderAttachment>(
      state, "MAX_HIGH_FREQ", maxHighFreqSlider);
  maxPhaseModeAttachment = std::make_unique<ComboBoxAttachment>(
      state, "MAX_PHASE_MODE", maxPhaseModeComboBox);

  excEnableAttachment =
      std::make_unique<ButtonAttachment>(state, "EXC_ENABLE", excEnableButton);
  excTuneAttachment =
      std::make_unique<SliderAttachment>(state, "EXC_TUNE", excTuneSlider);
  excDriveAttachment =
      std::make_unique<SliderAttachment>(state, "EXC_DRIVE", excDriveSlider);
  excMixAttachment =
      std::make_unique<SliderAttachment>(state, "EXC_MIX", excMixSlider);
  excModeAttachment =
      std::make_unique<ComboBoxAttachment>(state, "EXC_MODE", excModeComboBox);
  excHarmonicsAttachment = std::make_unique<SliderAttachment>(
      state, "EXC_HARMONICS", excHarmonicsSlider);
  excAirGainAttachment = std::make_unique<SliderAttachment>(
      state, "EXC_AIR_GAIN", excAirGainSlider);

  routingModeAttachment = std::make_unique<ComboBoxAttachment>(
      state, "ROUTING_MODE", routingModeComboBox);
  parallelBlendAttachment = std::make_unique<SliderAttachment>(
      state, "PARALLEL_BLEND", parallelBlendSlider);
  masterInGainAttachment = std::make_unique<SliderAttachment>(
      state, "MASTER_IN_GAIN", masterInGainSlider);
  masterOutGainAttachment = std::make_unique<SliderAttachment>(
      state, "MASTER_OUT_GAIN", masterOutGainSlider);
  masterMixAttachment =
      std::make_unique<SliderAttachment>(state, "MASTER_MIX", masterMixSlider);
  oversamplingAttachment = std::make_unique<ComboBoxAttachment>(
      state, "OVERSAMPLING", oversamplingComboBox);
}

AuralMaximizerProAudioProcessorEditor::
    ~AuralMaximizerProAudioProcessorEditor() {
  setLookAndFeel(nullptr);
}

void AuralMaximizerProAudioProcessorEditor::paint(juce::Graphics &graphics) {
  graphics.fillAll(CustomLookAndFeel::Background);

  drawPanel(graphics,
            {10, headerHeight, 445, upperSectionBottom - headerHeight},
            CustomLookAndFeel::Maximizer);
  drawPanel(graphics,
            {465, headerHeight, 445, upperSectionBottom - headerHeight},
            CustomLookAndFeel::Exciter);
  drawPanel(graphics, {10, 384, 900, 146}, CustomLookAndFeel::Master);

  graphics.setFont(
      juce::Font(juce::FontOptions{}.withHeight(20.0f).withStyle("Bold")));
  graphics.setColour(CustomLookAndFeel::Text);
  graphics.drawText("AURAL MAXIMIZER PRO", 16, 5, 300, 28,
                    juce::Justification::centredLeft);

  drawCaption(graphics, "SONIC MAXIMIZER", {20, 45, 210, 22},
              CustomLookAndFeel::Maximizer, 15.0f);
  drawCaption(graphics, "AURAL EXCITER", {475, 45, 190, 22},
              CustomLookAndFeel::Exciter, 15.0f);
  drawCaption(graphics, "FFT / OUTPUT SPECTRUM", {475, 98, 425, 16},
              CustomLookAndFeel::Exciter, 11.0f);
  drawCaption(graphics, "MASTER / ROUTING MATRIX / I-O", {20, 389, 330, 18},
              CustomLookAndFeel::Master, 14.0f);

  const juce::String maxLabels[] = {"PROCESS", "LO CONTOUR", "LOW FREQ",
                                    "HIGH FREQ"};
  for (int index = 0; index < 4; ++index)
    drawCaption(graphics, maxLabels[index], {28 + index * 104, 315, 88, 16},
                CustomLookAndFeel::Text.withAlpha(0.75f), 10.0f);

  const juce::String excLabels[] = {"TUNE", "DRIVE", "MIX", "HARMONICS",
                                    "AIR GAIN"};
  for (int index = 0; index < 5; ++index)
    drawCaption(graphics, excLabels[index], {474 + index * 84, 342, 76, 15},
                CustomLookAndFeel::Text.withAlpha(0.75f), 9.5f);

  const juce::String masterLabels[] = {"BLEND", "INPUT", "OUTPUT", "MIX"};
  for (int index = 0; index < 4; ++index)
    drawCaption(graphics, masterLabels[index], {352 + index * 96, 508, 80, 14},
                CustomLookAndFeel::Text.withAlpha(0.75f), 9.5f);

  drawCaption(graphics, "OUTPUT LEVELS", {772, 508, 128, 14},
              CustomLookAndFeel::Master, 9.0f);
}

void AuralMaximizerProAudioProcessorEditor::resized() {
  maxEnableButton.setBounds(24, 70, 150, 24);
  maxPhaseModeComboBox.setBounds(286, 70, 150, 24);
  maxProcessSlider.setBounds(28, 130, 88, 180);
  maxContourSlider.setBounds(132, 130, 88, 180);
  maxLowFreqSlider.setBounds(236, 130, 88, 180);
  maxHighFreqSlider.setBounds(340, 130, 88, 180);

  excEnableButton.setBounds(479, 70, 130, 24);
  excModeComboBox.setBounds(678, 70, 218, 24);
  spectrumVisualizer.setBounds(479, 116, 417, 112);
  excTuneSlider.setBounds(474, 240, 76, 98);
  excDriveSlider.setBounds(558, 240, 76, 98);
  excMixSlider.setBounds(642, 240, 76, 98);
  excHarmonicsSlider.setBounds(726, 240, 76, 98);
  excAirGainSlider.setBounds(810, 240, 76, 98);

  routingModeComboBox.setBounds(24, 420, 210, 26);
  oversamplingComboBox.setBounds(24, 468, 210, 26);
  parallelBlendSlider.setBounds(352, 414, 80, 90);
  masterInGainSlider.setBounds(448, 414, 80, 90);
  masterOutGainSlider.setBounds(544, 414, 80, 90);
  masterMixSlider.setBounds(640, 414, 80, 90);
  meterComponent.setBounds(772, 414, 128, 90);
}
