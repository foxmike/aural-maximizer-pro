#pragma once

#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MeterComponent renders real-time L/R peak and RMS LED-style meter bars.
 * It is updated on a capped juce::Timer on the message thread.
 * Meter levels are supplied via a lock-free atomic path.
 */
class MeterComponent : public juce::Component, private juce::Timer {
public:
  static constexpr int timerHz = 30; // Cadence capped within 30-60 Hz range

  MeterComponent();
  ~MeterComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  /**
   * Updates the current meter levels. Safe to call lock-free from the audio
   * thread.
   */
  void setLevels(float newLeftPeak, float newLeftRms, float newRightPeak,
                 float newRightRms) noexcept;

  /**
   * Pushes new levels to the meter. Safe to call lock-free from the audio
   * thread.
   */
  void pushLevels(float newLeftPeak, float newLeftRms, float newRightPeak,
                  float newRightRms) noexcept;

private:
  void timerCallback() override;

  std::atomic<float> leftPeak{0.0f};
  std::atomic<float> leftRms{0.0f};
  std::atomic<float> rightPeak{0.0f};
  std::atomic<float> rightRms{0.0f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterComponent)
};
