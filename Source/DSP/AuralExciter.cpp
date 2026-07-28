#include "AuralExciter.h"

#include <algorithm>
#include <cmath>

namespace nSpace {
namespace {
constexpr double smoothingTimeSeconds = 0.02;
constexpr float shelfQ = 0.70710678f;

float cubicSoftClip(float input) noexcept {
  const auto magnitude = std::abs(input);

  if (magnitude <= 2.0f / 3.0f)
    return input;

  if (magnitude >= 4.0f / 3.0f)
    return std::copysign(1.0f, input);

  const auto shoulder = 2.0f - 1.5f * magnitude;
  return std::copysign(1.0f - shoulder * shoulder / 3.0f, input);
}
} // namespace

AuralExciter::AuralExciter() {
  highPassSidechain.setType(juce::dsp::StateVariableTPTFilterType::highpass);
  highPassSidechain.setResonance(shelfQ);
}

void AuralExciter::prepare(const juce::dsp::ProcessSpec &spec) {
  jassert(spec.sampleRate > 0.0);
  jassert(spec.maximumBlockSize > 0);
  jassert(spec.numChannels > 0);

  if (spec.sampleRate <= 0.0 || spec.maximumBlockSize == 0 ||
      spec.numChannels == 0) {
    preparedSampleRate = 0.0;
    preparedLatencySamples = 0.0f;
    oversampler.reset();
    sidechainBuffer.setSize(0, 0);
    return;
  }

  preparedSampleRate = spec.sampleRate;

  const auto stageCount =
      requestedOversamplingFactor == OversamplingFactor::fourTimes
          ? std::size_t{2}
          : std::size_t{1};
  oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
      spec.numChannels, stageCount, oversamplingFilterType, true, true);
  oversampler->initProcessing(spec.maximumBlockSize);
  preparedLatencySamples = oversampler->getLatencyInSamples();

  sidechainBuffer.setSize(static_cast<int>(spec.numChannels),
                          static_cast<int>(spec.maximumBlockSize), false, false,
                          true);

  highPassSidechain.prepare(spec);
  airShelf.prepare(spec);

  tuneHz.reset(preparedSampleRate, smoothingTimeSeconds);
  wetMix.reset(preparedSampleRate, smoothingTimeSeconds);
  airGainDb.reset(preparedSampleRate, smoothingTimeSeconds);

  const auto oversampledRate =
      preparedSampleRate *
      static_cast<double>(oversampler->getOversamplingFactor());
  driveGainG.reset(oversampledRate, smoothingTimeSeconds);
  deltaBias.reset(oversampledRate, smoothingTimeSeconds);
  gamma.reset(oversampledRate, smoothingTimeSeconds);

  updateAirShelf();
  reset();
}

void AuralExciter::reset() {
  highPassSidechain.reset();
  airShelf.reset();

  if (oversampler != nullptr)
    oversampler->reset();

  sidechainBuffer.clear();

  tuneHz.setCurrentAndTargetValue(tuneHz.getTargetValue());
  wetMix.setCurrentAndTargetValue(wetMix.getTargetValue());
  driveGainG.setCurrentAndTargetValue(driveGainG.getTargetValue());
  deltaBias.setCurrentAndTargetValue(deltaBias.getTargetValue());
  gamma.setCurrentAndTargetValue(gamma.getTargetValue());
  airGainDb.setCurrentAndTargetValue(airGainDb.getTargetValue());
  updateAirShelf();
}

void AuralExciter::process(
    const juce::dsp::ProcessContextReplacing<float> &context) {
  juce::ScopedNoDenormals noDenormals;

  if (context.isBypassed || preparedSampleRate <= 0.0 || oversampler == nullptr)
    return;

  auto outputBlock = context.getOutputBlock();
  const auto channelCount = outputBlock.getNumChannels();
  const auto sampleCount = outputBlock.getNumSamples();

  jassert(channelCount <=
          static_cast<std::size_t>(sidechainBuffer.getNumChannels()));
  jassert(sampleCount <=
          static_cast<std::size_t>(sidechainBuffer.getNumSamples()));

  if (channelCount >
          static_cast<std::size_t>(sidechainBuffer.getNumChannels()) ||
      sampleCount > static_cast<std::size_t>(sidechainBuffer.getNumSamples()))
    return;

  juce::dsp::AudioBlock<float> storageBlock(sidechainBuffer);
  auto sidechainBlock = storageBlock.getSubsetChannelBlock(0, channelCount)
                            .getSubBlock(0, sampleCount);
  sidechainBlock.copyFrom(outputBlock);

  for (std::size_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
    highPassSidechain.setCutoffFrequency(tuneHz.getNextValue());

    for (std::size_t channel = 0; channel < channelCount; ++channel) {
      auto *samples = sidechainBlock.getChannelPointer(channel);
      samples[sampleIndex] = highPassSidechain.processSample(
          static_cast<int>(channel), samples[sampleIndex]);
    }
  }

  auto oversampledBlock = oversampler->processSamplesUp(sidechainBlock);
  const auto oversampledSampleCount = oversampledBlock.getNumSamples();
  const auto mode = waveShaperMode;

  for (std::size_t sampleIndex = 0; sampleIndex < oversampledSampleCount;
       ++sampleIndex) {
    const auto drive = driveGainG.getNextValue();
    const auto delta = deltaBias.getNextValue();
    const auto harmonicAmount = gamma.getNextValue();

    for (std::size_t channel = 0; channel < channelCount; ++channel) {
      auto *samples = oversampledBlock.getChannelPointer(channel);
      const auto drivenInput = drive * samples[sampleIndex];
      auto oddHarmonics = 0.0f;

      switch (mode) {
      case WaveShaperMode::cubicSoftClipping:
        oddHarmonics = cubicSoftClip(drivenInput);
        break;
      case WaveShaperMode::asymmetricTube:
        oddHarmonics = std::tanh(drivenInput + delta);
        break;
      case WaveShaperMode::tape:
        oddHarmonics =
            (2.0f / juce::MathConstants<float>::pi) * std::atan(drivenInput);
        break;
      }

      const auto evenHarmonics = drivenInput * drivenInput;
      samples[sampleIndex] = (1.0f - harmonicAmount) * oddHarmonics +
                             harmonicAmount * evenHarmonics;
    }
  }

  oversampler->processSamplesDown(sidechainBlock);

  airGainDb.skip(static_cast<int>(sampleCount));
  updateAirShelf();
  juce::dsp::ProcessContextReplacing<float> shelfContext(sidechainBlock);
  airShelf.process(shelfContext);

  for (std::size_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
    const auto mix = wetMix.getNextValue();
    for (std::size_t channel = 0; channel < channelCount; ++channel)
      outputBlock.addSample(
          static_cast<int>(channel), static_cast<int>(sampleIndex),
          mix * sidechainBlock.getSample(channel, sampleIndex));
  }
}

void AuralExciter::setTuneHz(float newTuneHz) noexcept {
  tuneHz.setTargetValue(std::clamp(newTuneHz, minimumTuneHz, maximumTuneHz));
}

void AuralExciter::setMix(float newMix) noexcept {
  wetMix.setTargetValue(std::clamp(newMix, 0.0f, 1.0f));
}

void AuralExciter::setDriveGain(float newDriveGainG) noexcept {
  driveGainG.setTargetValue(std::max(0.0f, newDriveGainG));
}

void AuralExciter::setDeltaBias(float delta) noexcept {
  deltaBias.setTargetValue(delta);
}

void AuralExciter::setHarmonics(float newGamma) noexcept {
  gamma.setTargetValue(std::clamp(newGamma, 0.0f, 1.0f));
}

void AuralExciter::setAirGainDb(float newAirGainDb) noexcept {
  airGainDb.setTargetValue(newAirGainDb);
}

void AuralExciter::setWaveShaperMode(WaveShaperMode mode) noexcept {
  waveShaperMode = mode;
}

void AuralExciter::setOversamplingFactor(OversamplingFactor factor) noexcept {
  requestedOversamplingFactor = factor;
}

float AuralExciter::getLatencySamples() const noexcept {
  return preparedLatencySamples;
}

void AuralExciter::updateAirShelf() {
  if (preparedSampleRate <= 0.0)
    return;

  const auto shelfFrequency = std::min(
      airShelfFrequencyHz, static_cast<float>(preparedSampleRate * 0.499));
  const auto gain = juce::Decibels::decibelsToGain(airGainDb.getCurrentValue());
  *airShelf.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
      preparedSampleRate, shelfFrequency, shelfQ, gain);
}
} // namespace nSpace
