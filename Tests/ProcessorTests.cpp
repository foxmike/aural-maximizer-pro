#include "../Source/PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string_view>

namespace {
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 512;

bool setParameter(AuralMaximizerProAudioProcessor &processor,
                  const char *parameterID, float value) {
  auto *parameter = processor.apvts.getParameter(parameterID);
  if (parameter == nullptr)
    return false;

  parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
  return true;
}

void fillTestSignal(juce::AudioBuffer<float> &buffer, int sampleOffset = 0) {
  for (auto channel = 0; channel < buffer.getNumChannels(); ++channel) {
    auto *samples = buffer.getWritePointer(channel);
    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {
      const auto time = static_cast<double>(sampleOffset + sample) / sampleRate;
      samples[sample] =
          0.09f * std::sin(juce::MathConstants<double>::twoPi * 70.0 * time) +
          0.08f * std::sin(juce::MathConstants<double>::twoPi * 220.0 * time) +
          0.07f * std::sin(juce::MathConstants<double>::twoPi * 1200.0 * time) +
          0.05f * std::sin(juce::MathConstants<double>::twoPi *
                           (channel == 0 ? 4000.0 : 6000.0) * time) +
          0.03f * std::sin(juce::MathConstants<double>::twoPi * 9000.0 * time);
    }
  }
}

float differenceRms(const juce::AudioBuffer<float> &left,
                    const juce::AudioBuffer<float> &right) {
  double sum = 0.0;
  auto sampleCount = 0;

  for (auto channel = 0; channel < left.getNumChannels(); ++channel) {
    for (auto sample = 0; sample < left.getNumSamples(); ++sample) {
      const auto difference = static_cast<double>(
          left.getSample(channel, sample) - right.getSample(channel, sample));
      sum += difference * difference;
      ++sampleCount;
    }
  }

  return sampleCount > 0 ? static_cast<float>(std::sqrt(
                               sum / static_cast<double>(sampleCount)))
                         : 0.0f;
}

float residualAfterOptimalGain(const juce::AudioBuffer<float> &reference,
                               const juce::AudioBuffer<float> &candidate) {
  double crossProduct = 0.0;
  double candidateEnergy = 0.0;

  for (auto channel = 0; channel < reference.getNumChannels(); ++channel) {
    for (auto sample = 0; sample < reference.getNumSamples(); ++sample) {
      const auto referenceValue =
          static_cast<double>(reference.getSample(channel, sample));
      const auto candidateValue =
          static_cast<double>(candidate.getSample(channel, sample));
      crossProduct += referenceValue * candidateValue;
      candidateEnergy += candidateValue * candidateValue;
    }
  }

  const auto optimalGain =
      candidateEnergy > 0.0 ? crossProduct / candidateEnergy : 0.0;
  double residualEnergy = 0.0;
  auto sampleCount = 0;

  for (auto channel = 0; channel < reference.getNumChannels(); ++channel) {
    for (auto sample = 0; sample < reference.getNumSamples(); ++sample) {
      const auto residual =
          static_cast<double>(reference.getSample(channel, sample)) -
          optimalGain *
              static_cast<double>(candidate.getSample(channel, sample));
      residualEnergy += residual * residual;
      ++sampleCount;
    }
  }

  return sampleCount > 0
             ? static_cast<float>(
                   std::sqrt(residualEnergy / static_cast<double>(sampleCount)))
             : 0.0f;
}

bool isFiniteAndBounded(const juce::AudioBuffer<float> &buffer,
                        float maximumMagnitude) {
  for (auto channel = 0; channel < buffer.getNumChannels(); ++channel) {
    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {
      const auto value = buffer.getSample(channel, sample);
      if (!std::isfinite(value) || std::abs(value) > maximumMagnitude)
        return false;
    }
  }

  return true;
}

class TestRunner {
public:
  void expect(bool condition, std::string_view message) {
    if (condition)
      return;

    ++failureCount;
    std::cerr << "FAIL: " << message << '\n';
  }

  [[nodiscard]] int failures() const noexcept { return failureCount; }

private:
  int failureCount = 0;
};

void configureNeutral(AuralMaximizerProAudioProcessor &processor,
                      TestRunner &runner) {
  runner.expect(setParameter(processor, "MAX_ENABLE", 0.0f),
                "MAX_ENABLE parameter exists");
  runner.expect(setParameter(processor, "EXC_ENABLE", 0.0f),
                "EXC_ENABLE parameter exists");
  runner.expect(setParameter(processor, "MASTER_IN_GAIN", 0.0f),
                "MASTER_IN_GAIN parameter exists");
  runner.expect(setParameter(processor, "MASTER_OUT_GAIN", 0.0f),
                "MASTER_OUT_GAIN parameter exists");
  runner.expect(setParameter(processor, "MASTER_MIX", 1.0f),
                "MASTER_MIX parameter exists");
}

juce::AudioBuffer<float> renderMaximizer(TestRunner &runner, float phaseMode,
                                         float process, float contour,
                                         float lowFrequency = 120.0f,
                                         float highFrequency = 5000.0f,
                                         int blocksToRender = 12) {
  AuralMaximizerProAudioProcessor processor;
  configureNeutral(processor, runner);
  runner.expect(setParameter(processor, "MAX_ENABLE", 1.0f),
                "MAX_ENABLE can enable the maximizer");
  runner.expect(setParameter(processor, "MAX_PHASE_MODE", phaseMode),
                "MAX_PHASE_MODE parameter exists");
  runner.expect(setParameter(processor, "MAX_PROCESS", process),
                "MAX_PROCESS parameter exists");
  runner.expect(setParameter(processor, "MAX_CONTOUR", contour),
                "MAX_CONTOUR parameter exists");
  runner.expect(setParameter(processor, "MAX_LOW_FREQ", lowFrequency),
                "MAX_LOW_FREQ parameter exists");
  runner.expect(setParameter(processor, "MAX_HIGH_FREQ", highFrequency),
                "MAX_HIGH_FREQ parameter exists");
  processor.prepareToPlay(sampleRate, blockSize);

  juce::AudioBuffer<float> buffer(2, blockSize);
  juce::MidiBuffer midi;
  for (auto block = 0; block < blocksToRender; ++block) {
    fillTestSignal(buffer, block * blockSize);
    processor.processBlock(buffer, midi);
  }

  return buffer;
}

void configureMaximizer(AuralMaximizerProAudioProcessor &processor,
                        TestRunner &runner, float phaseMode, float process,
                        float contour, float lowFrequency,
                        float highFrequency) {
  configureNeutral(processor, runner);
  runner.expect(setParameter(processor, "MAX_ENABLE", 1.0f),
                "MAX_ENABLE can enable the maximizer");
  runner.expect(setParameter(processor, "MAX_PHASE_MODE", phaseMode),
                "MAX_PHASE_MODE parameter exists");
  runner.expect(setParameter(processor, "MAX_PROCESS", process),
                "MAX_PROCESS parameter exists");
  runner.expect(setParameter(processor, "MAX_CONTOUR", contour),
                "MAX_CONTOUR parameter exists");
  runner.expect(setParameter(processor, "MAX_LOW_FREQ", lowFrequency),
                "MAX_LOW_FREQ parameter exists");
  runner.expect(setParameter(processor, "MAX_HIGH_FREQ", highFrequency),
                "MAX_HIGH_FREQ parameter exists");
}

struct LiveChangeMeasurement {
  float rmsDifference = 0.0f;
  float maximumJump = 0.0f;
};

LiveChangeMeasurement
measurePostPrepareChange(TestRunner &runner, const char *parameterID,
                         float initialValue, float changedValue,
                         float phaseMode, float process, float contour,
                         float lowFrequency, float highFrequency) {
  AuralMaximizerProAudioProcessor referenceProcessor;
  AuralMaximizerProAudioProcessor changedProcessor;
  configureMaximizer(referenceProcessor, runner, phaseMode, process, contour,
                     lowFrequency, highFrequency);
  configureMaximizer(changedProcessor, runner, phaseMode, process, contour,
                     lowFrequency, highFrequency);
  runner.expect(setParameter(referenceProcessor, parameterID, initialValue),
                "live reference parameter exists");
  runner.expect(setParameter(changedProcessor, parameterID, initialValue),
                "live changed parameter exists");
  referenceProcessor.prepareToPlay(sampleRate, blockSize);
  changedProcessor.prepareToPlay(sampleRate, blockSize);

  juce::AudioBuffer<float> referenceBuffer(2, blockSize);
  juce::AudioBuffer<float> changedBuffer(2, blockSize);
  juce::MidiBuffer referenceMidi;
  juce::MidiBuffer changedMidi;

  for (auto block = 0; block < 8; ++block) {
    fillTestSignal(referenceBuffer, block * blockSize);
    changedBuffer.makeCopyOf(referenceBuffer);
    referenceProcessor.processBlock(referenceBuffer, referenceMidi);
    changedProcessor.processBlock(changedBuffer, changedMidi);
  }

  runner.expect(setParameter(changedProcessor, parameterID, changedValue),
                "parameter can change after prepareToPlay");

  std::array<float, 2> previousSamples{
      changedBuffer.getSample(0, blockSize - 1),
      changedBuffer.getSample(1, blockSize - 1)};
  auto maximumJump = 0.0f;
  for (auto block = 8; block < 20; ++block) {
    fillTestSignal(referenceBuffer, block * blockSize);
    changedBuffer.makeCopyOf(referenceBuffer);
    referenceProcessor.processBlock(referenceBuffer, referenceMidi);
    changedProcessor.processBlock(changedBuffer, changedMidi);

    for (auto channel = 0; channel < changedBuffer.getNumChannels();
         ++channel) {
      maximumJump = std::max(
          maximumJump,
          std::abs(changedBuffer.getSample(channel, 0) -
                   previousSamples[static_cast<std::size_t>(channel)]));
      for (auto sample = 1; sample < changedBuffer.getNumSamples(); ++sample)
        maximumJump =
            std::max(maximumJump,
                     std::abs(changedBuffer.getSample(channel, sample) -
                              changedBuffer.getSample(channel, sample - 1)));
      previousSamples[static_cast<std::size_t>(channel)] =
          changedBuffer.getSample(channel, blockSize - 1);
    }
  }

  return {differenceRms(referenceBuffer, changedBuffer), maximumJump};
}

void testNeutralPassthrough(TestRunner &runner) {
  AuralMaximizerProAudioProcessor processor;
  configureNeutral(processor, runner);
  processor.prepareToPlay(sampleRate, blockSize);

  juce::AudioBuffer<float> buffer(2, blockSize);
  juce::AudioBuffer<float> original(2, blockSize);
  juce::MidiBuffer midi;
  fillTestSignal(buffer);
  original.makeCopyOf(buffer);

  processor.processBlock(buffer, midi);
  runner.expect(differenceRms(buffer, original) < 1.0e-6f,
                "disabled effects preserve the input signal");
}

void testExtremeExciter(TestRunner &runner) {
  AuralMaximizerProAudioProcessor processor;
  configureNeutral(processor, runner);
  runner.expect(setParameter(processor, "EXC_ENABLE", 1.0f),
                "EXC_ENABLE can enable the exciter");
  runner.expect(setParameter(processor, "EXC_TUNE", 800.0f),
                "EXC_TUNE parameter exists");
  runner.expect(setParameter(processor, "EXC_DRIVE", 24.0f),
                "EXC_DRIVE parameter exists");
  runner.expect(setParameter(processor, "EXC_MIX", 1.0f),
                "EXC_MIX parameter exists");
  runner.expect(setParameter(processor, "EXC_MODE", 0.0f),
                "EXC_MODE parameter exists");
  runner.expect(setParameter(processor, "EXC_HARMONICS", 1.0f),
                "EXC_HARMONICS parameter exists");
  runner.expect(setParameter(processor, "EXC_AIR_GAIN", 12.0f),
                "EXC_AIR_GAIN parameter exists");
  processor.prepareToPlay(sampleRate, blockSize);

  juce::AudioBuffer<float> buffer(2, blockSize);
  juce::AudioBuffer<float> original(2, blockSize);
  juce::MidiBuffer midi;
  for (auto block = 0; block < 8; ++block) {
    fillTestSignal(buffer, block * blockSize);
    original.makeCopyOf(buffer);
    processor.processBlock(buffer, midi);
  }

  runner.expect(differenceRms(buffer, original) > 0.02f,
                "extreme exciter settings measurably alter the output");
}

void testExtremeMaximizer(TestRunner &runner) {
  auto buffer = renderMaximizer(runner, 0.0f, 1.0f, 1.0f, 120.0f, 5000.0f, 8);
  juce::AudioBuffer<float> original(2, blockSize);
  fillTestSignal(original, 7 * blockSize);

  runner.expect(differenceRms(buffer, original) > 0.02f,
                "extreme maximizer settings measurably alter the output");
}

void testMaximizerPhaseModes(TestRunner &runner) {
  const auto classic = renderMaximizer(runner, 0.0f, 0.65f, 0.65f);
  const auto modern = renderMaximizer(runner, 1.0f, 0.65f, 0.65f);
  const auto modeDifference = differenceRms(classic, modern);
  const auto gainMatchedResidual = residualAfterOptimalGain(classic, modern);

  runner.expect(modeDifference > 0.005f,
                "Classic and Modern differ with Process and Lo Contour fixed");
  runner.expect(gainMatchedResidual > 0.003f,
                "phase modes are not related only by a fixed gain offset");
  runner.expect(isFiniteAndBounded(classic, 4.0f) &&
                    isFiniteAndBounded(modern, 4.0f),
                "both phase modes remain finite, stable, and bounded");

  std::cout << "Phase-mode RMS difference: " << modeDifference
            << ", gain-matched residual: " << gainMatchedResidual << '\n';
}

void testIndependentMaximizerControls(TestRunner &runner) {
  const auto processOff = renderMaximizer(runner, 1.0f, 0.0f, 0.0f);
  const auto processOn = renderMaximizer(runner, 1.0f, 1.0f, 0.0f);
  const auto contourOff = renderMaximizer(runner, 1.0f, 0.0f, 0.0f);
  const auto contourOn = renderMaximizer(runner, 1.0f, 0.0f, 1.0f);
  const auto highFrequencyLow =
      renderMaximizer(runner, 1.0f, 1.0f, 0.0f, 120.0f, 1500.0f);
  const auto highFrequencyHigh =
      renderMaximizer(runner, 1.0f, 1.0f, 0.0f, 120.0f, 11000.0f);
  const auto lowFrequencyLow =
      renderMaximizer(runner, 1.0f, 0.0f, 1.0f, 50.0f, 5000.0f);
  const auto lowFrequencyHigh =
      renderMaximizer(runner, 1.0f, 0.0f, 1.0f, 350.0f, 5000.0f);

  const auto processDifference = differenceRms(processOff, processOn);
  const auto contourDifference = differenceRms(contourOff, contourOn);
  const auto highFrequencyDifference =
      differenceRms(highFrequencyLow, highFrequencyHigh);
  const auto lowFrequencyDifference =
      differenceRms(lowFrequencyLow, lowFrequencyHigh);

  runner.expect(processDifference > 0.02f,
                "Process changes output with Lo Contour fixed at zero");
  runner.expect(contourDifference > 0.001f,
                "Lo Contour changes output with Process fixed at zero");
  runner.expect(highFrequencyDifference > 0.01f,
                "High Frequency changes response while Process is active");
  runner.expect(lowFrequencyDifference > 0.001f,
                "Low Frequency changes response while Lo Contour is active");

  std::cout << "Control RMS differences - Process: " << processDifference
            << ", Lo Contour: " << contourDifference
            << ", High Frequency: " << highFrequencyDifference
            << ", Low Frequency: " << lowFrequencyDifference << '\n';
}

void testPhaseModeSwitchContinuity(TestRunner &runner) {
  AuralMaximizerProAudioProcessor processor;
  configureNeutral(processor, runner);
  runner.expect(setParameter(processor, "MAX_ENABLE", 1.0f),
                "MAX_ENABLE can enable the maximizer");
  runner.expect(setParameter(processor, "MAX_PROCESS", 0.65f),
                "MAX_PROCESS parameter exists");
  runner.expect(setParameter(processor, "MAX_CONTOUR", 0.65f),
                "MAX_CONTOUR parameter exists");
  runner.expect(setParameter(processor, "MAX_PHASE_MODE", 0.0f),
                "MAX_PHASE_MODE can select Classic");
  processor.prepareToPlay(sampleRate, blockSize);

  juce::AudioBuffer<float> buffer(2, blockSize);
  juce::MidiBuffer midi;
  for (auto block = 0; block < 12; ++block) {
    fillTestSignal(buffer, block * blockSize);
    processor.processBlock(buffer, midi);
  }

  std::array<float, 2> previousSamples{buffer.getSample(0, blockSize - 1),
                                       buffer.getSample(1, blockSize - 1)};
  runner.expect(setParameter(processor, "MAX_PHASE_MODE", 1.0f),
                "MAX_PHASE_MODE can select Modern while processing");
  fillTestSignal(buffer, 12 * blockSize);
  processor.processBlock(buffer, midi);

  auto maximumJump = 0.0f;
  for (auto channel = 0; channel < buffer.getNumChannels(); ++channel) {
    maximumJump = std::max(maximumJump, std::abs(buffer.getSample(channel, 0) -
                                                 previousSamples[channel]));
    for (auto sample = 1; sample < buffer.getNumSamples(); ++sample)
      maximumJump = std::max(maximumJump,
                             std::abs(buffer.getSample(channel, sample) -
                                      buffer.getSample(channel, sample - 1)));
  }

  runner.expect(isFiniteAndBounded(buffer, 4.0f),
                "runtime phase-mode switch remains finite and bounded");
  runner.expect(maximumJump < 1.0f,
                "runtime phase-mode switch avoids an extreme discontinuity");
  std::cout << "Maximum sample jump during mode switch: " << maximumJump
            << '\n';
}

void testPostPrepareMaximizerControls(TestRunner &runner) {
  const auto processDifference = measurePostPrepareChange(
      runner, "MAX_PROCESS", 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 120.0f, 5000.0f);
  const auto contourDifference = measurePostPrepareChange(
      runner, "MAX_CONTOUR", 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 120.0f, 5000.0f);
  const auto lowFrequencyDifference = measurePostPrepareChange(
      runner, "MAX_LOW_FREQ", 50.0f, 350.0f, 1.0f, 0.0f, 1.0f, 50.0f, 5000.0f);
  const auto highFrequencyDifference =
      measurePostPrepareChange(runner, "MAX_HIGH_FREQ", 1500.0f, 11000.0f, 1.0f,
                               1.0f, 0.0f, 120.0f, 1500.0f);
  const auto phaseModeDifference =
      measurePostPrepareChange(runner, "MAX_PHASE_MODE", 0.0f, 1.0f, 0.0f,
                               0.65f, 0.65f, 120.0f, 5000.0f);

  runner.expect(processDifference.rmsDifference > 0.02f,
                "post-prepare Process change affects continuous processing");
  runner.expect(contourDifference.rmsDifference > 0.001f,
                "post-prepare Lo Contour change affects continuous processing");
  runner.expect(
      lowFrequencyDifference.rmsDifference > 0.001f,
      "post-prepare Low Frequency change affects continuous processing");
  runner.expect(
      highFrequencyDifference.rmsDifference > 0.01f,
      "post-prepare High Frequency change affects continuous processing");
  runner.expect(phaseModeDifference.rmsDifference > 0.005f,
                "post-prepare phase-mode change affects continuous processing");
  runner.expect(lowFrequencyDifference.maximumJump < 1.0f &&
                    highFrequencyDifference.maximumJump < 1.0f,
                "live crossover smoothing avoids extreme discontinuities");

  std::cout << "Post-prepare RMS differences - Process: "
            << processDifference.rmsDifference
            << ", Lo Contour: " << contourDifference.rmsDifference
            << ", Low Frequency: " << lowFrequencyDifference.rmsDifference
            << ", High Frequency: " << highFrequencyDifference.rmsDifference
            << ", Phase Mode: " << phaseModeDifference.rmsDifference << '\n';
  std::cout << "Live crossover maximum sample jumps - Low: "
            << lowFrequencyDifference.maximumJump
            << ", High: " << highFrequencyDifference.maximumJump << '\n';
}

void testMeterPublication(TestRunner &runner) {
  AuralMaximizerProAudioProcessor processor;
  configureNeutral(processor, runner);
  processor.prepareToPlay(sampleRate, blockSize);

  juce::AudioBuffer<float> buffer(2, blockSize);
  juce::MidiBuffer midi;
  fillTestSignal(buffer);
  processor.processBlock(buffer, midi);

  const auto active = processor.getMeterLevels();
  runner.expect(active.leftPeak > 0.0f && active.leftRms > 0.0f &&
                    active.rightPeak > 0.0f && active.rightRms > 0.0f,
                "known signal publishes nonzero peak and RMS meter levels");

  for (auto block = 0; block < 4; ++block) {
    buffer.clear();
    processor.processBlock(buffer, midi);
  }

  const auto silent = processor.getMeterLevels();
  runner.expect(silent.leftPeak < active.leftPeak &&
                    silent.leftRms < active.leftRms &&
                    silent.rightPeak < active.rightPeak &&
                    silent.rightRms < active.rightRms,
                "meter levels return toward silence after silent blocks");
  runner.expect(silent.leftPeak < 1.0e-6f && silent.leftRms < 1.0e-6f &&
                    silent.rightPeak < 1.0e-6f && silent.rightRms < 1.0e-6f,
                "silent output publishes silent meter levels");
}
} // namespace

int main() {
  TestRunner runner;
  testNeutralPassthrough(runner);
  testExtremeExciter(runner);
  testExtremeMaximizer(runner);
  testMaximizerPhaseModes(runner);
  testIndependentMaximizerControls(runner);
  testPhaseModeSwitchContinuity(runner);
  testPostPrepareMaximizerControls(runner);
  testMeterPublication(runner);

  if (runner.failures() != 0)
    return 1;

  std::cout << "All processor tests passed.\n";
  return 0;
}
