#include "SpectrumVisualizer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr float minimumDecibels = -100.0f;
constexpr float spectrumInset = 8.0f;
constexpr float releaseSmoothing = 0.82f;
constexpr float analysisPercentile = 0.95f;
constexpr float minimumUsefulReferenceDecibels = -80.0f;
constexpr std::size_t minimumActiveBins = 16;
constexpr float visualHeadroomDecibels = 9.0f;
constexpr float autoRangeAmount = 0.65f;
constexpr float minimumDisplayCeilingDecibels = -36.0f;
constexpr float maximumDisplayCeilingDecibels = 0.0f;
constexpr float defaultDisplayCeilingDecibels = -18.0f;
constexpr float displayedDynamicRangeDecibels = 72.0f;
constexpr float ceilingAttackSeconds = 0.12f;
constexpr float ceilingReleaseSeconds = 2.5f;
constexpr int usefulScaleHoldFrames = SpectrumVisualizer::timerHz * 6 / 5;

float smoothingAmount(float timeSeconds) {
  return 1.0f -
         std::exp(-1.0f / (static_cast<float>(SpectrumVisualizer::timerHz) *
                           timeSeconds));
}
} // namespace

SpectrumVisualizer::SpectrumVisualizer(
    AuralMaximizerProAudioProcessor &processor)
    : processorRef(processor), forwardFft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann, true) {
  // A path command requires only a few floats. Reserving for every FFT bin
  // keeps path rebuilding allocation-free after construction.
  spectrumPath.preallocateSpace(static_cast<int>(fftSize * 4));
  smoothedDecibels.fill(minimumDecibels);
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

  graphics.setColour(juce::Colour::fromRGB(116, 126, 139));
  graphics.setFont(juce::FontOptions(10.0f));
  graphics.drawText(
      "AUTO " + juce::String(juce::roundToInt(displayCeilingDecibels)) + " dB",
      plotBounds.withHeight(12.0f), juce::Justification::topRight, false);

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
  bool receivedUsefulFrame = false;
  auto highestTargetCeiling = minimumDisplayCeilingDecibels;

  while (processorRef.popSpectrumFrame(frameReadBuffer)) {
    receivedFrame = true;
    const auto targetCeiling = analyseFrame(frameReadBuffer);
    if (std::isfinite(targetCeiling)) {
      receivedUsefulFrame = true;
      highestTargetCeiling = juce::jmax(highestTargetCeiling, targetCeiling);
    }
  }

  if (receivedFrame)
    updateMagnitudeEnvelope();

  updateDisplayCeiling(receivedUsefulFrame, highestTargetCeiling);
}

float SpectrumVisualizer::analyseFrame(const SpectrumFrame &frame) {
  std::fill(fftData.begin(), fftData.end(), 0.0f);
  std::copy(frame.begin(), frame.end(), fftData.begin());

  window.multiplyWithWindowingTable(fftData.data(), fftSize);
  forwardFft.performFrequencyOnlyForwardTransform(fftData.data(), true);

  constexpr auto fftScale = 2.0f / static_cast<float>(fftSize);
  std::size_t activeBinCount = 0;
  for (std::size_t bin = 1; bin < smoothedDecibels.size(); ++bin) {
    const auto magnitude = fftData[bin] * fftScale;
    if (!std::isfinite(magnitude) || magnitude <= 0.0f)
      continue;

    const auto decibels =
        juce::Decibels::gainToDecibels(magnitude, minimumDecibels);
    if (std::isfinite(decibels) && decibels > minimumDecibels)
      percentileScratch[activeBinCount++] = decibels;
  }

  if (activeBinCount < minimumActiveBins)
    return std::numeric_limits<float>::quiet_NaN();

  const auto percentileIndex = static_cast<std::size_t>(
      std::floor(analysisPercentile * static_cast<float>(activeBinCount - 1)));
  auto begin = percentileScratch.begin();
  auto percentile = begin + static_cast<std::ptrdiff_t>(percentileIndex);
  std::nth_element(begin, percentile,
                   begin + static_cast<std::ptrdiff_t>(activeBinCount));
  const auto referenceDecibels = *percentile;
  if (!std::isfinite(referenceDecibels) ||
      referenceDecibels < minimumUsefulReferenceDecibels)
    return std::numeric_limits<float>::quiet_NaN();

  // Partial auto-ranging improves readability without making sustained level
  // changes converge to the same graph height.
  const auto targetCeiling =
      autoRangeAmount * (referenceDecibels + visualHeadroomDecibels);
  return juce::jlimit(minimumDisplayCeilingDecibels,
                      maximumDisplayCeilingDecibels, targetCeiling);
}

void SpectrumVisualizer::updateMagnitudeEnvelope() {
  constexpr auto fftScale = 2.0f / static_cast<float>(fftSize);
  for (std::size_t bin = 0; bin < smoothedDecibels.size(); ++bin) {
    const auto magnitude = fftData[bin] * fftScale;
    const auto decibels =
        std::isfinite(magnitude) && magnitude > 0.0f
            ? juce::Decibels::gainToDecibels(magnitude, minimumDecibels)
            : minimumDecibels;
    const auto releasedDecibels =
        minimumDecibels +
        (smoothedDecibels[bin] - minimumDecibels) * releaseSmoothing;
    smoothedDecibels[bin] = juce::jmax(decibels, releasedDecibels);
  }
}

void SpectrumVisualizer::updateDisplayCeiling(bool receivedUsefulFrame,
                                              float highestTargetCeiling) {
  auto targetCeiling = defaultDisplayCeilingDecibels;
  if (receivedUsefulFrame) {
    timersSinceUsefulFrame = 0;
    targetCeiling = highestTargetCeiling;
    if (!hasUsefulScale) {
      displayCeilingDecibels = targetCeiling;
      hasUsefulScale = true;
      return;
    }
  } else {
    timersSinceUsefulFrame =
        juce::jmin(timersSinceUsefulFrame + 1, usefulScaleHoldFrames + 1);
    if (hasUsefulScale && timersSinceUsefulFrame <= usefulScaleHoldFrames)
      return;
  }

  const auto timeSeconds =
      receivedUsefulFrame && targetCeiling > displayCeilingDecibels
          ? ceilingAttackSeconds
          : ceilingReleaseSeconds;
  displayCeilingDecibels +=
      smoothingAmount(timeSeconds) * (targetCeiling - displayCeilingDecibels);
  displayCeilingDecibels =
      juce::jlimit(minimumDisplayCeilingDecibels, maximumDisplayCeilingDecibels,
                   displayCeilingDecibels);
}

void SpectrumVisualizer::rebuildSpectrumPath() {
  spectrumPath.clear();

  const auto plotBounds = getLocalBounds().toFloat().reduced(spectrumInset);
  if (plotBounds.isEmpty())
    return;

  const auto pointCount =
      juce::jlimit(2, static_cast<int>(smoothedDecibels.size()),
                   juce::jmax(2, juce::roundToInt(plotBounds.getWidth())));
  constexpr auto firstBin = 1.0f;
  const auto lastBin = static_cast<float>(smoothedDecibels.size() - 1);
  const auto logBinRange = std::log(lastBin / firstBin);

  for (auto point = 0; point < pointCount; ++point) {
    const auto proportion =
        static_cast<float>(point) / static_cast<float>(pointCount - 1);
    const auto binPosition = firstBin * std::exp(proportion * logBinRange);
    const auto lowerBin = static_cast<std::size_t>(binPosition);
    const auto upperBin = std::min(lowerBin + 1, smoothedDecibels.size() - 1);
    const auto fraction = binPosition - static_cast<float>(lowerBin);
    const auto decibels = juce::jmap(fraction, smoothedDecibels[lowerBin],
                                     smoothedDecibels[upperBin]);
    const auto magnitude = juce::jlimit(
        0.0f, 1.0f,
        juce::jmap(decibels,
                   displayCeilingDecibels - displayedDynamicRangeDecibels,
                   displayCeilingDecibels, 0.0f, 1.0f));
    const auto x = plotBounds.getX() + proportion * plotBounds.getWidth();
    const auto y = plotBounds.getBottom() - magnitude * plotBounds.getHeight();

    if (point == 0)
      spectrumPath.startNewSubPath(x, y);
    else
      spectrumPath.lineTo(x, y);
  }
}
