#pragma once

#include <juce_dsp/juce_dsp.h>

namespace nSpace {
class SonicMaximizer;
class AuralExciter;

/**
 * Routing modes for signal path configurations.
 */
enum class RoutingMode {
  SerialMaximizerThenExciter, // S_E(S_M(x))
  SerialExciterThenMaximizer, // S_M(S_E(x))
  ParallelMatrixBlend         // (1-K)*S_M(x) + K*S_E(x)
};

/**
 * RoutingEngine manages signal routing and blending between DSP modules.
 */
class RoutingEngine {
public:
  RoutingEngine();
  ~RoutingEngine() = default;

  /**
   * Pre-allocates scratch buffers and configures the crossfade smoothing value.
   */
  void prepare(double sampleRate, int samplesPerBlock, int numChannels);

  /**
   * Overload to prepare using juce::dsp::ProcessSpec.
   */
  void prepare(const juce::dsp::ProcessSpec &spec);

  /**
   * Resets the routing engine state and smoothed values.
   */
  void reset() noexcept;

  /**
   * Processes a block of audio through the routing engine.
   *
   * Real-time safety contract:
   * - Run on the audio thread.
   * - No memory allocations/deallocations (no heap allocations).
   * - No locking or synchronization primitives.
   * - No blocking I/O operations.
   * - Routing mode and blend changes must not reallocate.
   */
  void process(const juce::dsp::ProcessContextReplacing<float> &context,
               SonicMaximizer &maximizer, AuralExciter &exciter,
               RoutingMode mode, float parallelBlendTarget,
               bool maximizerEnabled, bool exciterEnabled) noexcept;

private:
  using SmoothedParameter =
      juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

  // Smoothed value for the parallel blend parameter (K) to prevent clicks
  SmoothedParameter parallelBlend{0.0f};

  // Pre-allocated scratch buffers for parallel/intermediate paths
  juce::AudioBuffer<float> scratchBufferA;
  juce::AudioBuffer<float> scratchBufferB;

  double preparedSampleRate = 0.0;
  int preparedSamplesPerBlock = 0;
  int preparedNumChannels = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingEngine)
};
} // namespace nSpace
