#include "SonicMaximizer.h"

#include <algorithm>
#include <cmath>

namespace nSpace {
namespace {
constexpr float defaultMaxLowFrequencyHz = 120.0f;
constexpr float defaultMaxHighFrequencyHz = 5000.0f;
constexpr float minimumFrequencyHz = 1.0f;
constexpr double envelopeTimeSeconds = 0.05;
constexpr double phaseModeTransitionSeconds = 0.02;
constexpr double crossoverTransitionSeconds = 0.05;
constexpr std::size_t coefficientUpdateInterval = 16;
} // namespace

SonicMaximizer::SonicMaximizer()
    : maxLowFrequencyTargetHz(defaultMaxLowFrequencyHz),
      maxHighFrequencyTargetHz(defaultMaxHighFrequencyHz) {}

void SonicMaximizer::prepare(double sampleRate, int samplesPerBlock) {
  jassert(sampleRate > 0.0);
  jassert(samplesPerBlock > 0);

  if (sampleRate <= 0.0 || samplesPerBlock <= 0) {
    preparedSampleRate = 0.0;
    preparedSamplesPerBlock = 0;
    reset();
    return;
  }

  preparedSampleRate = sampleRate;
  preparedSamplesPerBlock = samplesPerBlock;
  envelopeAlpha = static_cast<float>(
      std::exp(-1.0 / (envelopeTimeSeconds * preparedSampleRate)));

  const auto maximumFrequencyHz =
      static_cast<float>(preparedSampleRate * 0.499);
  maxLowFrequencyTargetHz = std::clamp(maxLowFrequencyTargetHz,
                                       minimumFrequencyHz, maximumFrequencyHz);
  maxHighFrequencyTargetHz = std::clamp(
      maxHighFrequencyTargetHz, maxLowFrequencyTargetHz, maximumFrequencyHz);

  lowFrequencyHz.reset(preparedSampleRate, crossoverTransitionSeconds);
  highFrequencyHz.reset(preparedSampleRate, crossoverTransitionSeconds);
  lowFrequencyHz.setCurrentAndTargetValue(maxLowFrequencyTargetHz);
  highFrequencyHz.setCurrentAndTargetValue(maxHighFrequencyTargetHz);
  updateCrossoverCoefficients(maxLowFrequencyTargetHz,
                              maxHighFrequencyTargetHz);

  const juce::dsp::ProcessSpec spec{
      preparedSampleRate, static_cast<juce::uint32>(preparedSamplesPerBlock),
      1};
  const auto prepareFilterBank = [&spec](auto &stages) {
    for (auto &stage : stages)
      for (auto &filter : stage)
        filter.prepare(spec);
  };

  prepareFilterBank(lowBandLowPassFilters);
  prepareFilterBank(midBandHighPassFilters);
  prepareFilterBank(midBandLowPassFilters);
  prepareFilterBank(highBandHighPassFilters);
  prepareFilterBank(lowBandAllPassFilters);
  prepareFilterBank(midBandAllPassFilters);
  prepareFilterBank(highBandAllPassFilters);
  prepareFilterBank(modernLowBandAllPassFilters);
  prepareFilterBank(modernMidBandAllPassFilters);
  phaseModeBlend.reset(preparedSampleRate, phaseModeTransitionSeconds);
  reset();
}

void SonicMaximizer::reset() noexcept {
  const auto resetFilterBank = [](auto &stages) {
    for (auto &stage : stages)
      for (auto &filter : stage)
        filter.reset();
  };

  resetFilterBank(lowBandLowPassFilters);
  resetFilterBank(midBandHighPassFilters);
  resetFilterBank(midBandLowPassFilters);
  resetFilterBank(highBandHighPassFilters);
  resetFilterBank(lowBandAllPassFilters);
  resetFilterBank(midBandAllPassFilters);
  resetFilterBank(highBandAllPassFilters);
  resetFilterBank(modernLowBandAllPassFilters);
  resetFilterBank(modernMidBandAllPassFilters);
  envelopeE.fill(0.0f);
  phaseModeBlend.setCurrentAndTargetValue(
      phaseModeTarget == PhaseMode::modern ? 1.0f : 0.0f);
}

void SonicMaximizer::setCrossoverFrequencies(
    float maxLowFrequencyHz, float maxHighFrequencyHz) noexcept {
  maxLowFrequencyTargetHz = std::max(minimumFrequencyHz, maxLowFrequencyHz);
  maxHighFrequencyTargetHz =
      std::max(maxLowFrequencyTargetHz, maxHighFrequencyHz);

  if (preparedSampleRate <= 0.0)
    return;

  const auto maximumFrequencyHz =
      static_cast<float>(preparedSampleRate * 0.499);
  const auto lowFrequencyHz = std::clamp(
      maxLowFrequencyTargetHz, minimumFrequencyHz, maximumFrequencyHz);
  const auto highFrequencyHz =
      std::clamp(maxHighFrequencyTargetHz, lowFrequencyHz, maximumFrequencyHz);

  maxLowFrequencyTargetHz = lowFrequencyHz;
  maxHighFrequencyTargetHz = highFrequencyHz;

  this->lowFrequencyHz.setTargetValue(lowFrequencyHz);
  this->highFrequencyHz.setTargetValue(highFrequencyHz);
}

void SonicMaximizer::updateCrossoverCoefficients(float lowFrequency,
                                                 float highFrequency) noexcept {
  using ArrayCoefficients = juce::dsp::IIR::ArrayCoefficients<float>;
  const auto lowPassCoefficients =
      ArrayCoefficients::makeLowPass(preparedSampleRate, lowFrequency);
  const auto lowHighPassCoefficients =
      ArrayCoefficients::makeHighPass(preparedSampleRate, lowFrequency);
  const auto highLowPassCoefficients =
      ArrayCoefficients::makeLowPass(preparedSampleRate, highFrequency);
  const auto highPassCoefficients =
      ArrayCoefficients::makeHighPass(preparedSampleRate, highFrequency);

  const auto assignCoefficients = [](auto &stages, const auto &coefficients) {
    for (auto &stage : stages)
      for (auto &filter : stage)
        *filter.coefficients = coefficients;
  };

  assignCoefficients(lowBandLowPassFilters, lowPassCoefficients);
  assignCoefficients(midBandHighPassFilters, lowHighPassCoefficients);
  assignCoefficients(midBandLowPassFilters, highLowPassCoefficients);
  assignCoefficients(highBandHighPassFilters, highPassCoefficients);

  const auto tangent = std::tan(juce::MathConstants<float>::pi * lowFrequency /
                                static_cast<float>(preparedSampleRate));
  apfCoefficientA = (tangent - 1.0f) / (tangent + 1.0f);
  const auto allPassCoefficients = ArrayCoefficients::makeFirstOrderAllPass(
      preparedSampleRate, lowFrequency);

  assignCoefficients(lowBandAllPassFilters, allPassCoefficients);
  assignCoefficients(midBandAllPassFilters, allPassCoefficients);
  assignCoefficients(highBandAllPassFilters, allPassCoefficients);
  assignCoefficients(modernLowBandAllPassFilters, allPassCoefficients);
  assignCoefficients(modernMidBandAllPassFilters, allPassCoefficients);

  appliedLowFrequencyHz = lowFrequency;
  appliedHighFrequencyHz = highFrequency;
}

void SonicMaximizer::setLoContour(float newLoContour) noexcept {
  loContourTarget = std::clamp(newLoContour, 0.0f, 1.0f);
}

void SonicMaximizer::setProcessAmount(float newProcessAmount) noexcept {
  processAmountTarget = std::clamp(newProcessAmount, 0.0f, 1.0f);
}

void SonicMaximizer::setPhaseMode(PhaseMode newPhaseMode) noexcept {
  phaseModeTarget = newPhaseMode;
  phaseModeBlend.setTargetValue(newPhaseMode == PhaseMode::modern ? 1.0f
                                                                  : 0.0f);
}

void SonicMaximizer::process(
    const juce::dsp::ProcessContextReplacing<float> &context) noexcept {
  juce::ScopedNoDenormals noDenormals;

  if (context.isBypassed || preparedSampleRate <= 0.0)
    return;

  auto outputBlock = context.getOutputBlock();
  const auto channelCount = std::min(outputBlock.getNumChannels(), maxChannels);
  const auto sampleCount = outputBlock.getNumSamples();
  const auto oneMinusEnvelopeAlpha = 1.0f - envelopeAlpha;
  const auto loContour = loContourTarget;
  const auto highBandGain = 1.0f + 3.0f * processAmountTarget;

  for (std::size_t chunkStart = 0; chunkStart < sampleCount;
       chunkStart += coefficientUpdateInterval) {
    const auto chunkSize =
        std::min(coefficientUpdateInterval, sampleCount - chunkStart);
    const auto smoothedLowFrequency =
        lowFrequencyHz.skip(static_cast<int>(chunkSize));
    const auto smoothedHighFrequency =
        highFrequencyHz.skip(static_cast<int>(chunkSize));

    if (smoothedLowFrequency != appliedLowFrequencyHz ||
        smoothedHighFrequency != appliedHighFrequencyHz)
      updateCrossoverCoefficients(smoothedLowFrequency, smoothedHighFrequency);

    for (std::size_t sampleIndex = chunkStart;
         sampleIndex < chunkStart + chunkSize; ++sampleIndex) {
      const auto modernBlend = phaseModeBlend.getNextValue();
      const auto classicBlend = 1.0f - modernBlend;

      for (std::size_t channel = 0; channel < channelCount; ++channel) {
        auto *samples = outputBlock.getChannelPointer(channel);
        auto envelope = envelopeE[channel];
        const auto input = samples[sampleIndex];
        auto low = input;
        auto mid = input;
        auto high = input;

        for (auto &stage : lowBandLowPassFilters)
          low = stage[channel].processSample(low);

        for (auto &stage : midBandHighPassFilters)
          mid = stage[channel].processSample(mid);
        for (auto &stage : midBandLowPassFilters)
          mid = stage[channel].processSample(mid);

        for (auto &stage : highBandHighPassFilters)
          high = stage[channel].processSample(high);

        auto classicLow = low;
        auto classicMid = mid;
        auto modernLow = low;
        auto modernMid = mid;

        for (auto &stage : lowBandAllPassFilters)
          classicLow = stage[channel].processSample(classicLow);
        for (auto &stage : midBandAllPassFilters)
          classicMid = stage[channel].processSample(classicMid);
        for (auto &stage : highBandAllPassFilters)
          high = stage[channel].processSample(high);

        for (auto &stage : modernLowBandAllPassFilters)
          modernLow = stage[channel].processSample(modernLow);
        for (auto &stage : modernMidBandAllPassFilters)
          modernMid = stage[channel].processSample(modernMid);

        envelope =
            oneMinusEnvelopeAlpha * std::abs(low) + envelopeAlpha * envelope;
        const auto lowGain = 1.0f + loContour * (envelope / (1.0f + envelope));
        const auto classicOutput =
            classicLow * lowGain + classicMid + high * highBandGain;
        const auto modernOutput =
            modernLow * lowGain + modernMid + high * highBandGain;
        samples[sampleIndex] =
            classicBlend * classicOutput + modernBlend * modernOutput;

        envelopeE[channel] = envelope;
      }
    }
  }
}
} // namespace nSpace
