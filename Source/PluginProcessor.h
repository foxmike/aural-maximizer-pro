#pragma once

#include <array>
#include <atomic>
#include <cstddef>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/AuralExciter.h"
#include "DSP/RoutingEngine.h"
#include "DSP/SonicMaximizer.h"

class AuralMaximizerProAudioProcessor : public juce::AudioProcessor {
public:
  static constexpr std::size_t spectrumFrameSize = 2048;
  static constexpr int spectrumFifoCapacity = 8;
  using SpectrumFrame = std::array<float, spectrumFrameSize>;

  struct MeterLevels {
    float leftPeak = 0.0f;
    float leftRms = 0.0f;
    float rightPeak = 0.0f;
    float rightRms = 0.0f;
  };

  AuralMaximizerProAudioProcessor();
  ~AuralMaximizerProAudioProcessor() override;

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;
  void processBlock(juce::AudioBuffer<double> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  bool popSpectrumFrame(SpectrumFrame &destination) noexcept;
  [[nodiscard]] MeterLevels getMeterLevels() const noexcept;

  // Kept public for the generic editor and later APVTS attachments.
  juce::AudioProcessorValueTreeState apvts{*this, nullptr};

private:
  using SmoothedParameter =
      juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

  void cacheParameterHandles();
  void updateParameterTargets() noexcept;
  void updateLatencyForOversampling();
  void updateMeterLevels(const juce::AudioBuffer<float> &buffer,
                         int channelCount) noexcept;
  void pushSpectrumFrame(const juce::AudioBuffer<float> &buffer) noexcept;

  nSpace::SonicMaximizer sonicMaximizer;
  nSpace::AuralExciter auralExciter;
  nSpace::RoutingEngine routingEngine;

  juce::AudioBuffer<float> dryBuffer;

  SmoothedParameter masterInputGain{1.0f};
  SmoothedParameter masterOutputGain{1.0f};
  SmoothedParameter masterMix{1.0f};

  std::atomic<float> *maxEnableParameter = nullptr;
  std::atomic<float> *maxProcessParameter = nullptr;
  std::atomic<float> *maxContourParameter = nullptr;
  std::atomic<float> *maxLowFrequencyParameter = nullptr;
  std::atomic<float> *maxHighFrequencyParameter = nullptr;
  std::atomic<float> *maxPhaseModeParameter = nullptr;

  std::atomic<float> *excEnableParameter = nullptr;
  std::atomic<float> *excTuneParameter = nullptr;
  std::atomic<float> *excDriveParameter = nullptr;
  std::atomic<float> *excMixParameter = nullptr;
  std::atomic<float> *excModeParameter = nullptr;
  std::atomic<float> *excHarmonicsParameter = nullptr;
  std::atomic<float> *excAirGainParameter = nullptr;

  std::atomic<float> *routingModeParameter = nullptr;
  std::atomic<float> *parallelBlendParameter = nullptr;
  std::atomic<float> *masterInputGainParameter = nullptr;
  std::atomic<float> *masterOutputGainParameter = nullptr;
  std::atomic<float> *masterMixParameter = nullptr;
  std::atomic<float> *oversamplingParameter = nullptr;

  std::array<SpectrumFrame, spectrumFifoCapacity> spectrumBuffer{};
  juce::AbstractFifo spectrumFifo{spectrumFifoCapacity};

  std::atomic<float> leftPeakMeter{0.0f};
  std::atomic<float> leftRmsMeter{0.0f};
  std::atomic<float> rightPeakMeter{0.0f};
  std::atomic<float> rightRmsMeter{0.0f};

  nSpace::AuralExciter::OversamplingFactor preparedOversamplingFactor =
      nSpace::AuralExciter::OversamplingFactor::twoTimes;
  // Cached during preparation before reporting through setLatencySamples().
  float oversamplingLatencySamples = 0.0f;
  int reportedLatencySamples = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuralMaximizerProAudioProcessor)
};
