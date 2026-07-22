#include "RoutingEngine.h"

#include "AuralExciter.h"
#include "SonicMaximizer.h"

#include <algorithm>

namespace nSpace {
namespace {
constexpr double smoothingTimeSeconds = 0.02;
}

RoutingEngine::RoutingEngine() = default;

void RoutingEngine::prepare(double sampleRate, int samplesPerBlock,
                            int numChannels) {
  jassert(sampleRate > 0.0);
  jassert(samplesPerBlock > 0);
  jassert(numChannels > 0);

  if (sampleRate <= 0.0 || samplesPerBlock <= 0 || numChannels <= 0) {
    preparedSampleRate = 0.0;
    preparedSamplesPerBlock = 0;
    preparedNumChannels = 0;
    scratchBufferA.setSize(0, 0);
    scratchBufferB.setSize(0, 0);
    return;
  }

  preparedSampleRate = sampleRate;
  preparedSamplesPerBlock = samplesPerBlock;
  preparedNumChannels = numChannels;

  scratchBufferA.setSize(preparedNumChannels, preparedSamplesPerBlock, false,
                         false, true);
  scratchBufferB.setSize(preparedNumChannels, preparedSamplesPerBlock, false,
                         false, true);

  parallelBlend.reset(preparedSampleRate, smoothingTimeSeconds);
  reset();
}

void RoutingEngine::prepare(const juce::dsp::ProcessSpec &spec) {
  prepare(spec.sampleRate, static_cast<int>(spec.maximumBlockSize),
          static_cast<int>(spec.numChannels));
}

void RoutingEngine::reset() noexcept {
  scratchBufferA.clear();
  scratchBufferB.clear();
  parallelBlend.setCurrentAndTargetValue(parallelBlend.getTargetValue());
}

void RoutingEngine::process(
    const juce::dsp::ProcessContextReplacing<float> &context,
    SonicMaximizer &maximizer, AuralExciter &exciter, RoutingMode mode,
    float parallelBlendTarget) noexcept {
  juce::ScopedNoDenormals noDenormals;

  auto outputBlock = context.getOutputBlock();
  const auto channelCount = outputBlock.getNumChannels();
  const auto sampleCount = outputBlock.getNumSamples();

  parallelBlend.setTargetValue(std::clamp(parallelBlendTarget, 0.0f, 1.0f));

  if (context.isBypassed) {
    parallelBlend.skip(static_cast<int>(sampleCount));
    return;
  }

  jassert(preparedSampleRate > 0.0);
  jassert(channelCount <= static_cast<std::size_t>(preparedNumChannels));
  jassert(sampleCount <= static_cast<std::size_t>(preparedSamplesPerBlock));

  if (preparedSampleRate <= 0.0 ||
      channelCount > static_cast<std::size_t>(preparedNumChannels) ||
      sampleCount > static_cast<std::size_t>(preparedSamplesPerBlock))
    return;

  switch (mode) {
  case RoutingMode::SerialMaximizerThenExciter:
    parallelBlend.skip(static_cast<int>(sampleCount));
    maximizer.process(context);
    exciter.process(context);
    return;

  case RoutingMode::SerialExciterThenMaximizer:
    parallelBlend.skip(static_cast<int>(sampleCount));
    exciter.process(context);
    maximizer.process(context);
    return;

  case RoutingMode::ParallelMatrixBlend:
    break;
  }

  juce::dsp::AudioBlock<float> scratchStorageA(scratchBufferA);
  juce::dsp::AudioBlock<float> scratchStorageB(scratchBufferB);
  auto maximizerBlock = scratchStorageA.getSubsetChannelBlock(0, channelCount)
                            .getSubBlock(0, sampleCount);
  auto exciterBlock = scratchStorageB.getSubsetChannelBlock(0, channelCount)
                          .getSubBlock(0, sampleCount);

  maximizerBlock.copyFrom(outputBlock);
  exciterBlock.copyFrom(outputBlock);

  juce::dsp::ProcessContextReplacing<float> maximizerContext(maximizerBlock);
  juce::dsp::ProcessContextReplacing<float> exciterContext(exciterBlock);
  maximizer.process(maximizerContext);
  exciter.process(exciterContext);

  for (std::size_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
    const auto blend = parallelBlend.getNextValue();
    const auto maximizerGain = 1.0f - blend;

    for (std::size_t channel = 0; channel < channelCount; ++channel) {
      outputBlock.setSample(
          static_cast<int>(channel), static_cast<int>(sampleIndex),
          maximizerGain * maximizerBlock.getSample(channel, sampleIndex) +
              blend * exciterBlock.getSample(channel, sampleIndex));
    }
  }
}
} // namespace nSpace
