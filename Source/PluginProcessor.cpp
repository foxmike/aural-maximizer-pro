#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <memory>

namespace {
constexpr auto parameterVersion = 1;
constexpr double smoothingTimeSeconds = 0.02;

using APVTS = juce::AudioProcessorValueTreeState;
using ParameterID = juce::ParameterID;

ParameterID parameterID(const char *id) { return {id, parameterVersion}; }

juce::NormalisableRange<float> linearRange(float minimum, float maximum,
                                           float interval = 0.01f) {
  return {minimum, maximum, interval};
}

juce::NormalisableRange<float> skewedRange(float minimum, float maximum,
                                           float interval, float skew) {
  return {minimum, maximum, interval, skew};
}

float loadParameter(const std::atomic<float> *parameter) noexcept {
  jassert(parameter != nullptr);
  return parameter != nullptr ? parameter->load(std::memory_order_relaxed)
                              : 0.0f;
}

bool isEnabled(const std::atomic<float> *parameter) noexcept {
  return loadParameter(parameter) >= 0.5f;
}

nSpace::RoutingMode routingModeFromParameter(float value) noexcept {
  switch (juce::jlimit(0, 2, juce::roundToInt(value))) {
  case 1:
    return nSpace::RoutingMode::SerialExciterThenMaximizer;
  case 2:
    return nSpace::RoutingMode::ParallelMatrixBlend;
  default:
    return nSpace::RoutingMode::SerialMaximizerThenExciter;
  }
}

nSpace::AuralExciter::WaveShaperMode
exciterModeFromParameter(float value) noexcept {
  switch (juce::jlimit(0, 2, juce::roundToInt(value))) {
  case 1:
    return nSpace::AuralExciter::WaveShaperMode::asymmetricTube;
  case 2:
    return nSpace::AuralExciter::WaveShaperMode::tape;
  default:
    return nSpace::AuralExciter::WaveShaperMode::cubicSoftClipping;
  }
}

nSpace::AuralExciter::OversamplingFactor
oversamplingFactorFromParameter(float value) noexcept {
  return juce::roundToInt(value) == 1
             ? nSpace::AuralExciter::OversamplingFactor::fourTimes
             : nSpace::AuralExciter::OversamplingFactor::twoTimes;
}

nSpace::SonicMaximizer::PhaseMode
maximizerPhaseModeFromParameter(float value) noexcept {
  return juce::roundToInt(value) == 1
             ? nSpace::SonicMaximizer::PhaseMode::modern
             : nSpace::SonicMaximizer::PhaseMode::classic;
}
} // namespace

APVTS::ParameterLayout
AuralMaximizerProAudioProcessor::createParameterLayout() {
  APVTS::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterBool>(
      parameterID("MAX_ENABLE"), "Maximizer Enable", true));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("MAX_PROCESS"), "Maximizer Process",
      linearRange(0.0f, 1.0f, 0.001f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("MAX_CONTOUR"), "Maximizer Lo Contour",
      linearRange(0.0f, 1.0f, 0.001f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("MAX_LOW_FREQ"), "Maximizer Low Frequency",
      skewedRange(40.0f, 400.0f, 1.0f, 0.5f), 120.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("MAX_HIGH_FREQ"), "Maximizer High Frequency",
      skewedRange(1000.0f, 12000.0f, 1.0f, 0.5f), 5000.0f));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      parameterID("MAX_PHASE_MODE"), "Maximizer Phase Mode",
      juce::StringArray{"Classic", "Modern"}, 0));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      parameterID("EXC_ENABLE"), "Exciter Enable", true));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("EXC_TUNE"), "Exciter Tune",
      skewedRange(800.0f, 10000.0f, 1.0f, 0.5f), 3000.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("EXC_DRIVE"), "Exciter Drive", linearRange(0.0f, 24.0f, 0.1f),
      6.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("EXC_MIX"), "Exciter Mix", linearRange(0.0f, 1.0f, 0.001f),
      0.5f));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      parameterID("EXC_MODE"), "Exciter Mode",
      juce::StringArray{"Cubic Soft Clipping", "Asymmetric Tube", "Tape"}, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("EXC_HARMONICS"), "Exciter Harmonics",
      linearRange(0.0f, 1.0f, 0.001f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("EXC_AIR_GAIN"), "Exciter Air Gain",
      linearRange(0.0f, 12.0f, 0.1f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      parameterID("ROUTING_MODE"), "Routing Mode",
      juce::StringArray{"Maximizer -> Exciter", "Exciter -> Maximizer",
                        "Parallel Matrix"},
      0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("PARALLEL_BLEND"), "Parallel Blend",
      linearRange(0.0f, 1.0f, 0.001f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("MASTER_IN_GAIN"), "Master Input Gain",
      linearRange(-24.0f, 24.0f, 0.1f), 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("MASTER_OUT_GAIN"), "Master Output Gain",
      linearRange(-24.0f, 24.0f, 0.1f), 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      parameterID("MASTER_MIX"), "Master Mix", linearRange(0.0f, 1.0f, 0.001f),
      1.0f));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      parameterID("OVERSAMPLING"), "Oversampling",
      juce::StringArray{"2x", "4x"}, 0));

  return layout;
}

AuralMaximizerProAudioProcessor::AuralMaximizerProAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout()) {
  cacheParameterHandles();
  spectrumFifo.reset();
}

AuralMaximizerProAudioProcessor::~AuralMaximizerProAudioProcessor() = default;

void AuralMaximizerProAudioProcessor::cacheParameterHandles() {
  maxEnableParameter = apvts.getRawParameterValue("MAX_ENABLE");
  maxProcessParameter = apvts.getRawParameterValue("MAX_PROCESS");
  maxContourParameter = apvts.getRawParameterValue("MAX_CONTOUR");
  maxLowFrequencyParameter = apvts.getRawParameterValue("MAX_LOW_FREQ");
  maxHighFrequencyParameter = apvts.getRawParameterValue("MAX_HIGH_FREQ");
  maxPhaseModeParameter = apvts.getRawParameterValue("MAX_PHASE_MODE");

  excEnableParameter = apvts.getRawParameterValue("EXC_ENABLE");
  excTuneParameter = apvts.getRawParameterValue("EXC_TUNE");
  excDriveParameter = apvts.getRawParameterValue("EXC_DRIVE");
  excMixParameter = apvts.getRawParameterValue("EXC_MIX");
  excModeParameter = apvts.getRawParameterValue("EXC_MODE");
  excHarmonicsParameter = apvts.getRawParameterValue("EXC_HARMONICS");
  excAirGainParameter = apvts.getRawParameterValue("EXC_AIR_GAIN");

  routingModeParameter = apvts.getRawParameterValue("ROUTING_MODE");
  parallelBlendParameter = apvts.getRawParameterValue("PARALLEL_BLEND");
  masterInputGainParameter = apvts.getRawParameterValue("MASTER_IN_GAIN");
  masterOutputGainParameter = apvts.getRawParameterValue("MASTER_OUT_GAIN");
  masterMixParameter = apvts.getRawParameterValue("MASTER_MIX");
  oversamplingParameter = apvts.getRawParameterValue("OVERSAMPLING");

  jassert(
      maxEnableParameter != nullptr && maxProcessParameter != nullptr &&
      maxContourParameter != nullptr && maxLowFrequencyParameter != nullptr &&
      maxHighFrequencyParameter != nullptr &&
      maxPhaseModeParameter != nullptr && excEnableParameter != nullptr &&
      excTuneParameter != nullptr && excDriveParameter != nullptr &&
      excMixParameter != nullptr && excModeParameter != nullptr &&
      excHarmonicsParameter != nullptr && excAirGainParameter != nullptr &&
      routingModeParameter != nullptr && parallelBlendParameter != nullptr &&
      masterInputGainParameter != nullptr &&
      masterOutputGainParameter != nullptr && masterMixParameter != nullptr &&
      oversamplingParameter != nullptr);
}

void AuralMaximizerProAudioProcessor::updateParameterTargets() noexcept {
  sonicMaximizer.setProcessAmount(loadParameter(maxProcessParameter));
  sonicMaximizer.setLoContour(loadParameter(maxContourParameter));
  sonicMaximizer.setCrossoverFrequencies(
      loadParameter(maxLowFrequencyParameter),
      loadParameter(maxHighFrequencyParameter));
  sonicMaximizer.setPhaseMode(
      maximizerPhaseModeFromParameter(loadParameter(maxPhaseModeParameter)));

  auralExciter.setMix(loadParameter(excMixParameter));
  auralExciter.setTuneHz(loadParameter(excTuneParameter));
  auralExciter.setDriveGain(
      juce::Decibels::decibelsToGain(loadParameter(excDriveParameter)));
  auralExciter.setHarmonics(loadParameter(excHarmonicsParameter));
  auralExciter.setAirGainDb(loadParameter(excAirGainParameter));
  auralExciter.setWaveShaperMode(
      exciterModeFromParameter(loadParameter(excModeParameter)));

  masterInputGain.setTargetValue(
      juce::Decibels::decibelsToGain(loadParameter(masterInputGainParameter)));
  masterOutputGain.setTargetValue(
      juce::Decibels::decibelsToGain(loadParameter(masterOutputGainParameter)));
  masterMix.setTargetValue(loadParameter(masterMixParameter));
}

void AuralMaximizerProAudioProcessor::prepareToPlay(double sampleRate,
                                                    int samplesPerBlock) {
  jassert(sampleRate > 0.0);
  jassert(samplesPerBlock > 0);

  const auto channelCount = juce::jmax(1, getTotalNumOutputChannels());
  dryBuffer.setSize(channelCount, samplesPerBlock, false, false, true);

  preparedOversamplingFactor =
      oversamplingFactorFromParameter(loadParameter(oversamplingParameter));
  auralExciter.setOversamplingFactor(preparedOversamplingFactor);

  sonicMaximizer.setPhaseMode(
      maximizerPhaseModeFromParameter(loadParameter(maxPhaseModeParameter)));
  sonicMaximizer.setCrossoverFrequencies(
      loadParameter(maxLowFrequencyParameter),
      loadParameter(maxHighFrequencyParameter));
  sonicMaximizer.prepare(sampleRate, samplesPerBlock);

  const juce::dsp::ProcessSpec spec{sampleRate,
                                    static_cast<juce::uint32>(samplesPerBlock),
                                    static_cast<juce::uint32>(channelCount)};
  auralExciter.prepare(spec);
  routingEngine.prepare(spec);

  masterInputGain.reset(sampleRate, smoothingTimeSeconds);
  masterOutputGain.reset(sampleRate, smoothingTimeSeconds);
  masterMix.reset(sampleRate, smoothingTimeSeconds);
  updateParameterTargets();
  auralExciter.reset();
  masterInputGain.setCurrentAndTargetValue(masterInputGain.getTargetValue());
  masterOutputGain.setCurrentAndTargetValue(masterOutputGain.getTargetValue());
  masterMix.setCurrentAndTargetValue(masterMix.getTargetValue());

  spectrumFifo.reset();
  updateLatencyForOversampling();
}

void AuralMaximizerProAudioProcessor::updateLatencyForOversampling() {
  oversamplingLatencySamples = auralExciter.getLatencySamples();
  reportedLatencySamples =
      juce::jmax(0, static_cast<int>(std::ceil(oversamplingLatencySamples)));
  setLatencySamples(reportedLatencySamples);
}

void AuralMaximizerProAudioProcessor::releaseResources() {
  sonicMaximizer.reset();
  auralExciter.reset();
  routingEngine.reset();
  dryBuffer.clear();
  spectrumFifo.reset();
}

bool AuralMaximizerProAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  const auto output = layouts.getMainOutputChannelSet();
  if (output != juce::AudioChannelSet::mono() &&
      output != juce::AudioChannelSet::stereo())
    return false;

  return output == layouts.getMainInputChannelSet();
}

void AuralMaximizerProAudioProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  juce::ignoreUnused(midiMessages);

  const auto inputChannels = getTotalNumInputChannels();
  const auto outputChannels = getTotalNumOutputChannels();
  const auto sampleCount = buffer.getNumSamples();

  for (auto channel = inputChannels; channel < outputChannels; ++channel)
    buffer.clear(channel, 0, sampleCount);

  const auto channelCount = juce::jmin(outputChannels, buffer.getNumChannels());
  jassert(sampleCount <= dryBuffer.getNumSamples());
  jassert(channelCount <= dryBuffer.getNumChannels());
  if (sampleCount > dryBuffer.getNumSamples() ||
      channelCount > dryBuffer.getNumChannels()) {
    pushSpectrumFrame(buffer);
    return;
  }

  updateParameterTargets();

  for (auto sample = 0; sample < sampleCount; ++sample) {
    const auto inputGain = masterInputGain.getNextValue();
    for (auto channel = 0; channel < channelCount; ++channel)
      buffer.getWritePointer(channel)[sample] *= inputGain;
  }

  for (auto channel = 0; channel < channelCount; ++channel)
    dryBuffer.copyFrom(channel, 0, buffer, channel, 0, sampleCount);

  juce::dsp::AudioBlock<float> block(buffer);
  auto processBlock =
      block.getSubsetChannelBlock(0, static_cast<std::size_t>(channelCount));
  juce::dsp::ProcessContextReplacing<float> context(processBlock);

  routingEngine.process(
      context, sonicMaximizer, auralExciter,
      routingModeFromParameter(loadParameter(routingModeParameter)),
      loadParameter(parallelBlendParameter), isEnabled(maxEnableParameter),
      isEnabled(excEnableParameter));

  for (auto sample = 0; sample < sampleCount; ++sample) {
    const auto outputGain = masterOutputGain.getNextValue();
    const auto wet = masterMix.getNextValue();
    const auto dry = 1.0f - wet;

    for (auto channel = 0; channel < channelCount; ++channel) {
      const auto processed = buffer.getSample(channel, sample);
      const auto mixed =
          dry * dryBuffer.getSample(channel, sample) + wet * processed;
      buffer.setSample(channel, sample, mixed * outputGain);
    }
  }

  updateMeterLevels(buffer, channelCount);
  pushSpectrumFrame(buffer);
}

void AuralMaximizerProAudioProcessor::processBlock(
    juce::AudioBuffer<double> &buffer, juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  juce::ignoreUnused(midiMessages);

  for (auto channel = getTotalNumInputChannels();
       channel < getTotalNumOutputChannels(); ++channel)
    buffer.clear(channel, 0, buffer.getNumSamples());
}

void AuralMaximizerProAudioProcessor::pushSpectrumFrame(
    const juce::AudioBuffer<float> &buffer) noexcept {
  int startIndex1 = 0;
  int blockSize1 = 0;
  int startIndex2 = 0;
  int blockSize2 = 0;
  spectrumFifo.prepareToWrite(1, startIndex1, blockSize1, startIndex2,
                              blockSize2);
  juce::ignoreUnused(startIndex2, blockSize2);

  if (blockSize1 == 0)
    return;

  auto &frame = spectrumBuffer[static_cast<std::size_t>(startIndex1)];
  frame.fill(0.0f);

  const auto samplesToCopy =
      juce::jmin(buffer.getNumSamples(), static_cast<int>(spectrumFrameSize));
  const auto channelCount = buffer.getNumChannels();
  if (channelCount > 0) {
    const auto channelScale = 1.0f / static_cast<float>(channelCount);
    for (auto sample = 0; sample < samplesToCopy; ++sample) {
      auto sum = 0.0f;
      for (auto channel = 0; channel < channelCount; ++channel)
        sum += buffer.getSample(channel, sample);
      frame[static_cast<std::size_t>(sample)] = sum * channelScale;
    }
  }

  spectrumFifo.finishedWrite(1);
}

bool AuralMaximizerProAudioProcessor::popSpectrumFrame(
    SpectrumFrame &destination) noexcept {
  int startIndex1 = 0;
  int blockSize1 = 0;
  int startIndex2 = 0;
  int blockSize2 = 0;
  spectrumFifo.prepareToRead(1, startIndex1, blockSize1, startIndex2,
                             blockSize2);
  juce::ignoreUnused(startIndex2, blockSize2);

  if (blockSize1 == 0)
    return false;

  destination = spectrumBuffer[static_cast<std::size_t>(startIndex1)];
  spectrumFifo.finishedRead(1);
  return true;
}

AuralMaximizerProAudioProcessor::MeterLevels
AuralMaximizerProAudioProcessor::getMeterLevels() const noexcept {
  return {leftPeakMeter.load(std::memory_order_relaxed),
          leftRmsMeter.load(std::memory_order_relaxed),
          rightPeakMeter.load(std::memory_order_relaxed),
          rightRmsMeter.load(std::memory_order_relaxed)};
}

void AuralMaximizerProAudioProcessor::updateMeterLevels(
    const juce::AudioBuffer<float> &buffer, int channelCount) noexcept {
  const auto sampleCount = buffer.getNumSamples();
  const auto leftPeak =
      channelCount > 0 ? buffer.getMagnitude(0, 0, sampleCount) : 0.0f;
  const auto leftRms =
      channelCount > 0 ? buffer.getRMSLevel(0, 0, sampleCount) : 0.0f;
  const auto rightPeak =
      channelCount > 1 ? buffer.getMagnitude(1, 0, sampleCount) : leftPeak;
  const auto rightRms =
      channelCount > 1 ? buffer.getRMSLevel(1, 0, sampleCount) : leftRms;

  leftPeakMeter.store(leftPeak, std::memory_order_relaxed);
  leftRmsMeter.store(leftRms, std::memory_order_relaxed);
  rightPeakMeter.store(rightPeak, std::memory_order_relaxed);
  rightRmsMeter.store(rightRms, std::memory_order_relaxed);
}

bool AuralMaximizerProAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *AuralMaximizerProAudioProcessor::createEditor() {
  return new PluginEditor(*this);
}

const juce::String AuralMaximizerProAudioProcessor::getName() const {
  return "Aural Maximizer Pro";
}

bool AuralMaximizerProAudioProcessor::acceptsMidi() const { return false; }

bool AuralMaximizerProAudioProcessor::producesMidi() const { return false; }

bool AuralMaximizerProAudioProcessor::isMidiEffect() const { return false; }

double AuralMaximizerProAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int AuralMaximizerProAudioProcessor::getNumPrograms() { return 1; }

int AuralMaximizerProAudioProcessor::getCurrentProgram() { return 0; }

void AuralMaximizerProAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String AuralMaximizerProAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void AuralMaximizerProAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {
  juce::ignoreUnused(index, newName);
}

void AuralMaximizerProAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  if (auto xml = apvts.copyState().createXml())
    copyXmlToBinary(*xml, destData);
}

void AuralMaximizerProAudioProcessor::setStateInformation(const void *data,
                                                          int sizeInBytes) {
  if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
    const auto state = juce::ValueTree::fromXml(*xml);
    if (state.isValid() && state.hasType(apvts.state.getType()))
      apvts.replaceState(state);
  }
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AuralMaximizerProAudioProcessor();
}
