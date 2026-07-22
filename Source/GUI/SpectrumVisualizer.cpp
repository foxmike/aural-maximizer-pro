#include "SpectrumVisualizer.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float minimumDecibels = -100.0f;
constexpr float spectrumInset = 8.0f;
constexpr float releaseSmoothing = 0.82f;
} // namespace

SpectrumVisualizer::SpectrumVisualizer(
    AuralMaximizerProAudioProcessor &processor)
    : processorRef(processor), forwardFft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann, true) {
  // A path command requires only a few floats. Reserving for every FFT bin
  // keeps path rebuilding allocation-free after construction.
  spectrumPath.preallocateSpace(static_cast<int>(fftSize * 4));
  startTimerHz(timerHz);
}

SpectrumVisualizer::~SpectrumVisualizer() { stopTimer(); }

void SpectrumVisualizer::paint(juce::Graphics &graphics) {
  const auto bounds = getLocalBounds().toFloat();
  graphics.fillAll(juce::Colour::fromRGB(14, 18, 24));

  const auto plotBounds = bounds.reduced(spectrumInset);
  graphics.setColour(juce::Colour::fromRGB(42, 49, 59));
  for (auto division = 1; division < 4; ++division) {
    const auto y = plotBounds.getY() +
                   plotBounds.getHeight() * static_cast<float>(division) / 4.0f;
    graphics.drawHorizontalLine(juce::roundToInt(y), plotBounds.getX(),
                                plotBounds.getRight());
  }

  graphics.setColour(juce::Colour::fromRGB(74, 210, 176));
  graphics.strokePath(spectrumPath,
                      juce::PathStrokeType(1.75f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
}

void SpectrumVisualizer::resized() { rebuildSpectrumPath(); }

void SpectrumVisualizer::timerCallback() {
  const auto *messageManager =
      juce::MessageManager::getInstanceWithoutCreating();
  jassert(messageManager != nullptr &&
          messageManager->isThisTheMessageThread());
  consumePendingFrames();
  rebuildSpectrumPath();
  repaint();
}

void SpectrumVisualizer::consumePendingFrames() {
  bool receivedFrame = false;
  while (processorRef.popSpectrumFrame(frameReadBuffer))
    receivedFrame = true;

  if (receivedFrame)
    updateMagnitudes(frameReadBuffer);
}

void SpectrumVisualizer::updateMagnitudes(const SpectrumFrame &frame) {
  std::fill(fftData.begin(), fftData.end(), 0.0f);
  std::copy(frame.begin(), frame.end(), fftData.begin());

  window.multiplyWithWindowingTable(fftData.data(), fftSize);
  forwardFft.performFrequencyOnlyForwardTransform(fftData.data(), true);

  constexpr auto fftScale = 2.0f / static_cast<float>(fftSize);
  for (std::size_t bin = 0; bin < magnitudes.size(); ++bin) {
    const auto decibels = juce::Decibels::gainToDecibels(
        fftData[bin] * fftScale, minimumDecibels);
    const auto level = juce::jmap(decibels, minimumDecibels, 0.0f, 0.0f, 1.0f);
    magnitudes[bin] = juce::jmax(level, magnitudes[bin] * releaseSmoothing);
  }
}

void SpectrumVisualizer::rebuildSpectrumPath() {
  spectrumPath.clear();

  const auto plotBounds = getLocalBounds().toFloat().reduced(spectrumInset);
  if (plotBounds.isEmpty())
    return;

  const auto pointCount =
      juce::jlimit(2, static_cast<int>(magnitudes.size()),
                   juce::jmax(2, juce::roundToInt(plotBounds.getWidth())));
  constexpr auto firstBin = 1.0f;
  const auto lastBin = static_cast<float>(magnitudes.size() - 1);
  const auto logBinRange = std::log(lastBin / firstBin);

  for (auto point = 0; point < pointCount; ++point) {
    const auto proportion =
        static_cast<float>(point) / static_cast<float>(pointCount - 1);
    const auto binPosition = firstBin * std::exp(proportion * logBinRange);
    const auto lowerBin = static_cast<std::size_t>(binPosition);
    const auto upperBin = std::min(lowerBin + 1, magnitudes.size() - 1);
    const auto fraction = binPosition - static_cast<float>(lowerBin);
    const auto magnitude =
        juce::jmap(fraction, magnitudes[lowerBin], magnitudes[upperBin]);
    const auto x = plotBounds.getX() + proportion * plotBounds.getWidth();
    const auto y = plotBounds.getBottom() - magnitude * plotBounds.getHeight();

    if (point == 0)
      spectrumPath.startNewSubPath(x, y);
    else
      spectrumPath.lineTo(x, y);
  }
}
