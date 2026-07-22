#pragma once

#include <cstddef>
#include <memory>

#include <juce_dsp/juce_dsp.h>

namespace nSpace {
/** Adds controllable high-frequency harmonics and post-saturation air. */
class AuralExciter {
public:
  enum class WaveShaperMode {
    cubicSoftClipping,
    asymmetricTube,
    tape
  };

  enum class OversamplingFactor : std::size_t { twoTimes = 2, fourTimes = 4 };

  static constexpr float minimumTuneHz = 800.0f;
  static constexpr float maximumTuneHz = 10000.0f;
  static constexpr float airShelfFrequencyHz = 10000.0f;
  static constexpr float defaultDeltaBias = 0.15f;

  AuralExciter();
  ~AuralExciter() = default;

  /**
   * Allocates scratch storage, constructs the selected FIR oversampler, and
   * caches its latency. Must be called before process().
   */
  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  /** Processes a prepared block in place without allocation or locking. */
  void process(const juce::dsp::ProcessContextReplacing<float> &context);

  /** Control-rate setters; call before the next process() block. */
  void setTuneHz(float tuneHz) noexcept;
  void setMix(float mix) noexcept;
  void setDriveGain(float driveGainG) noexcept;
  void setDeltaBias(float delta) noexcept;
  void setHarmonics(float gamma) noexcept;
  void setAirGainDb(float airGainDb) noexcept;
  void setWaveShaperMode(WaveShaperMode mode) noexcept;

  /** Takes effect on the next prepare() call; never rebuilds in process(). */
  void setOversamplingFactor(OversamplingFactor factor) noexcept;

  /** Returns the latency cached by prepare(), without querying in process(). */
  [[nodiscard]] float getLatencySamples() const noexcept;

private:
  using SmoothedParameter =
      juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;
  using AirShelfFilter =
      juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                     juce::dsp::IIR::Coefficients<float>>;
  static constexpr auto oversamplingFilterType =
      juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple;

  void updateAirShelf();

  // Second-order high-pass sidechain driven by EXC_TUNE.
  juce::dsp::StateVariableTPTFilter<float> highPassSidechain;

  // Constructed and initialised only by prepare(); FIR stages wrap the
  // non-linear portion of the signal path.
  std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

  // Post-saturation shelf driven by EXC_AIR_GAIN, with a 10 kHz corner.
  AirShelfFilter airShelf;

  // Storage is sized once in prepare() and reused by process().
  juce::AudioBuffer<float> sidechainBuffer;

  SmoothedParameter tuneHz{minimumTuneHz};
  SmoothedParameter wetMix{0.0f};
  SmoothedParameter driveGainG{1.0f};
  SmoothedParameter deltaBias{defaultDeltaBias};
  SmoothedParameter gamma{1.0f};
  SmoothedParameter airGainDb{0.0f};

  WaveShaperMode waveShaperMode = WaveShaperMode::cubicSoftClipping;
  OversamplingFactor requestedOversamplingFactor =
      OversamplingFactor::twoTimes;

  double preparedSampleRate = 0.0;
  float preparedLatencySamples = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuralExciter)
};
} // namespace nSpace
