#pragma once

#include <array>
#include <cstddef>

#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../PluginProcessor.h"

/**
 * Displays FFT magnitudes produced from frames already queued by the processor.
 *
 * The processor is the sole audio-thread producer for its lock-free
 * juce::AbstractFifo. This component only calls popSpectrumFrame() from its
 * message-thread timer; none of its methods are audio-thread entry points.
 */
class SpectrumVisualizer : public juce::Component, private juce::Timer {
public:
  using SpectrumFrame = AuralMaximizerProAudioProcessor::SpectrumFrame;

  static constexpr std::size_t fftSize =
      AuralMaximizerProAudioProcessor::spectrumFrameSize;
  static constexpr int fifoCapacity =
      AuralMaximizerProAudioProcessor::spectrumFifoCapacity;
  static constexpr int timerHz = 30;
  static constexpr int fftOrder = 11;

  explicit SpectrumVisualizer(AuralMaximizerProAudioProcessor &processor);
  ~SpectrumVisualizer() override;

  void paint(juce::Graphics &graphics) override;
  void resized() override;

private:
  void timerCallback() override;
  void consumePendingFrames();
  void updateMagnitudes(const SpectrumFrame &frame);
  void rebuildSpectrumPath();

  static_assert((std::size_t{1} << fftOrder) == fftSize,
                "The FFT order must match the processor spectrum frame size");
  static_assert(fifoCapacity > 0,
                "The spectrum FIFO must contain at least one frame");

  AuralMaximizerProAudioProcessor &processorRef;
  juce::dsp::FFT forwardFft;
  juce::dsp::WindowingFunction<float> window;
  SpectrumFrame frameReadBuffer{};
  std::array<float, fftSize * 2> fftData{};
  std::array<float, fftSize / 2> magnitudes{};
  juce::Path spectrumPath;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumVisualizer)
};
