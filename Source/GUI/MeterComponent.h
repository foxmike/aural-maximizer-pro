#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class AuralMaximizerProAudioProcessor;

/**
 * MeterComponent renders real-time L/R peak and RMS LED-style meter bars.
 * It is updated on a capped juce::Timer on the message thread.
 * Meter levels are supplied via a lock-free atomic path.
 */
class MeterComponent : public juce::Component, private juce::Timer {
public:
  static constexpr int timerHz = 30; // Cadence capped within 30-60 Hz range

  explicit MeterComponent(AuralMaximizerProAudioProcessor &processor);
  ~MeterComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  void timerCallback() override;

  AuralMaximizerProAudioProcessor &processorRef;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterComponent)
};
