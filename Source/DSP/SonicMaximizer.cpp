#include "SonicMaximizer.h"

#include <algorithm>
#include <cmath>

namespace nSpace {
namespace {
constexpr float defaultMaxLowFrequencyHz = 120.0f;
constexpr float defaultMaxHighFrequencyHz = 5000.0f;
constexpr float minimumFrequencyHz = 1.0f;
constexpr double envelopeTimeSeconds = 0.05;
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

  setCrossoverFrequencies(maxLowFrequencyTargetHz, maxHighFrequencyTargetHz);

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
  envelopeE.fill(0.0f);
}

void SonicMaximizer::setCrossoverFrequencies(float maxLowFrequencyHz,
                                             float maxHighFrequencyHz) {
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

  using Coefficients = juce::dsp::IIR::Coefficients<float>;
  const auto lowPassCoefficients =
      Coefficients::makeLowPass(preparedSampleRate, lowFrequencyHz);
  const auto lowHighPassCoefficients =
      Coefficients::makeHighPass(preparedSampleRate, lowFrequencyHz);
  const auto highLowPassCoefficients =
      Coefficients::makeLowPass(preparedSampleRate, highFrequencyHz);
  const auto highPassCoefficients =
      Coefficients::makeHighPass(preparedSampleRate, highFrequencyHz);

  const auto assignCoefficients = [](auto &stages, const auto &coefficients) {
    for (auto &stage : stages)
      for (auto &filter : stage)
        filter.coefficients = coefficients;
  };

  assignCoefficients(lowBandLowPassFilters, lowPassCoefficients);
  assignCoefficients(midBandHighPassFilters, lowHighPassCoefficients);
  assignCoefficients(midBandLowPassFilters, highLowPassCoefficients);
  assignCoefficients(highBandHighPassFilters, highPassCoefficients);

  const auto tangent =
      std::tan(juce::MathConstants<float>::pi * lowFrequencyHz /
               static_cast<float>(preparedSampleRate));
  apfCoefficientA = (tangent - 1.0f) / (tangent + 1.0f);
  const auto allPassCoefficients =
      Coefficients::makeFirstOrderAllPass(preparedSampleRate, lowFrequencyHz);

  assignCoefficients(lowBandAllPassFilters, allPassCoefficients);
  assignCoefficients(midBandAllPassFilters, allPassCoefficients);
  assignCoefficients(highBandAllPassFilters, allPassCoefficients);
}

void SonicMaximizer::setLoContour(float newLoContour) noexcept {
  loContourTarget = std::clamp(newLoContour, 0.0f, 1.0f);
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

  for (std::size_t channel = 0; channel < channelCount; ++channel) {
    auto *samples = outputBlock.getChannelPointer(channel);
    auto envelope = envelopeE[channel];

    for (std::size_t sampleIndex = 0; sampleIndex < sampleCount;
         ++sampleIndex) {
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

      for (auto &stage : lowBandAllPassFilters)
        low = stage[channel].processSample(low);
      for (auto &stage : midBandAllPassFilters)
        mid = stage[channel].processSample(mid);
      for (auto &stage : highBandAllPassFilters)
        high = stage[channel].processSample(high);

      envelope =
          oneMinusEnvelopeAlpha * std::abs(low) + envelopeAlpha * envelope;
      const auto lowGain = 1.0f + loContour * (envelope / (1.0f + envelope));
      samples[sampleIndex] = low * lowGain + mid + high;
    }

    envelopeE[channel] = envelope;
  }
}
} // namespace nSpace
