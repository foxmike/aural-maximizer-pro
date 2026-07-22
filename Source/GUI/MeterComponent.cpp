#include "MeterComponent.h"
#include <algorithm>
#include <cmath>

namespace {
float levelToNormalized(float level) noexcept {
  if (level <= 0.0f)
    return 0.0f;

  float db =
      (level <= 2.0f) ? juce::Decibels::gainToDecibels(level, -60.0f) : level;
  float norm = (db + 60.0f) / 60.0f;
  return juce::jlimit(0.0f, 1.0f, norm);
}
} // namespace

MeterComponent::MeterComponent() { startTimerHz(updateRateHz); }

MeterComponent::~MeterComponent() { stopTimer(); }

void MeterComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff121417));

  auto bounds = getLocalBounds().toFloat().reduced(2.0f);
  g.setColour(juce::Colour(0xff2c3038));
  g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

  auto innerBounds = bounds.reduced(4.0f);
  if (innerBounds.getWidth() <= 0.0f || innerBounds.getHeight() <= 0.0f)
    return;

  // Header area for L / R labels if height allows
  float labelHeight = (innerBounds.getHeight() > 60.0f) ? 14.0f : 0.0f;
  auto meterArea = innerBounds;
  if (labelHeight > 0.0f) {
    auto labelArea = innerBounds.removeFromTop(labelHeight);
    g.setColour(juce::Colour(0xfff2f2f7));
    g.setFont(juce::FontOptions(10.0f).withBold());

    float halfWidth = labelArea.getWidth() / 2.0f;
    auto leftLabelArea = labelArea.removeFromLeft(halfWidth);
    g.drawText("L", leftLabelArea, juce::Justification::centred, false);
    g.drawText("R", labelArea, juce::Justification::centred, false);
  }

  // Divide meter area into Left and Right channels with a gap in the middle
  float channelGap = 4.0f;
  float totalWidth = meterArea.getWidth();
  float channelWidth = (totalWidth - channelGap) / 2.0f;

  if (channelWidth <= 0.0f)
    return;

  auto leftChannelArea = meterArea.removeFromLeft(channelWidth);
  meterArea.removeFromLeft(channelGap);
  auto rightChannelArea = meterArea;

  // Snapshot normalized values for rendering
  float leftPeakNorm = levelToNormalized(currentLeftPeak);
  float rightPeakNorm = levelToNormalized(currentRightPeak);
  float leftRmsNorm = levelToNormalized(currentLeftRms);
  float rightRmsNorm = levelToNormalized(currentRightRms);

  auto drawChannelMeter = [&g](juce::Rectangle<float> channelRect,
                               float peakNorm, float rmsNorm) {
    float barGap = 2.0f;
    float barWidth = (channelRect.getWidth() - barGap) / 2.0f;
    if (barWidth <= 0.0f)
      return;

    auto rmsRect = channelRect.removeFromLeft(barWidth);
    channelRect.removeFromLeft(barGap);
    auto peakRect = channelRect;

    // Draw track background for RMS and Peak
    g.setColour(juce::Colour(0xff1a1d22));
    g.fillRect(rmsRect);
    g.fillRect(peakRect);

    float barH = rmsRect.getHeight();

    // Helper to draw a vertical meter bar with gradient
    auto drawBarFill = [&g, barH](juce::Rectangle<float> rect, float normVal,
                                  bool isRms) {
      if (normVal <= 0.0f || rect.getHeight() <= 0.0f)
        return;

      float fillH = rect.getHeight() * normVal;
      auto fillRect = rect.withHeight(fillH).withY(rect.getBottom() - fillH);

      juce::ColourGradient gradient(
          isRms ? juce::Colour(0xff00d2ff) : juce::Colour(0xff34c759),
          rect.getX(), rect.getBottom(), juce::Colour(0xffff3b30), rect.getX(),
          rect.getY(), false);
      gradient.addColour(0.7, juce::Colour(0xffff9500));

      g.setGradientFill(gradient);
      g.fillRect(fillRect);

      // LED Segment dividers overlay
      g.setColour(juce::Colour(0xff121417));
      int segmentSpacing = 4;
      for (float y = rect.getBottom() - static_cast<float>(segmentSpacing);
           y > rect.getY(); y -= static_cast<float>(segmentSpacing)) {
        if (y > fillRect.getY())
          g.drawHorizontalLine(juce::roundToInt(y), rect.getX(),
                               rect.getRight());
      }
    };

    drawBarFill(rmsRect, rmsNorm, true);
    drawBarFill(peakRect, peakNorm, false);

    // Draw Peak indicator tick
    if (peakNorm > 0.0f) {
      float peakY = peakRect.getBottom() - (barH * peakNorm);
      g.setColour(juce::Colour(0xfff2f2f7));
      g.drawHorizontalLine(juce::roundToInt(peakY), peakRect.getX(),
                           peakRect.getRight());
    }

    // Draw subtle border around each bar
    g.setColour(juce::Colour(0xff2c3038));
    g.drawRect(rmsRect, 1.0f);
    g.drawRect(peakRect, 1.0f);
  };

  drawChannelMeter(leftChannelArea, leftPeakNorm, leftRmsNorm);
  drawChannelMeter(rightChannelArea, rightPeakNorm, rightRmsNorm);
}

void MeterComponent::resized() {}

void MeterComponent::setLevels(float leftPeakIn, float rightPeakIn,
                               float leftRmsIn, float rightRmsIn) {
  leftPeak.store(leftPeakIn, std::memory_order_relaxed);
  rightPeak.store(rightPeakIn, std::memory_order_relaxed);
  leftRms.store(leftRmsIn, std::memory_order_relaxed);
  rightRms.store(rightRmsIn, std::memory_order_relaxed);
}

void MeterComponent::pushLevels(float leftPeakIn, float rightPeakIn,
                                float leftRmsIn, float rightRmsIn) {
  setLevels(leftPeakIn, rightPeakIn, leftRmsIn, rightRmsIn);
}

float MeterComponent::getLeftPeak() const noexcept {
  return leftPeak.load(std::memory_order_relaxed);
}

float MeterComponent::getRightPeak() const noexcept {
  return rightPeak.load(std::memory_order_relaxed);
}

float MeterComponent::getLeftRms() const noexcept {
  return leftRms.load(std::memory_order_relaxed);
}

float MeterComponent::getRightRms() const noexcept {
  return rightRms.load(std::memory_order_relaxed);
}

void MeterComponent::timerCallback() {
  float targetLeftPeak = leftPeak.load(std::memory_order_relaxed);
  float targetRightPeak = rightPeak.load(std::memory_order_relaxed);
  float targetLeftRms = leftRms.load(std::memory_order_relaxed);
  float targetRightRms = rightRms.load(std::memory_order_relaxed);

  constexpr float decayFactor = 0.88f;

  currentLeftPeak = std::max(targetLeftPeak, currentLeftPeak * decayFactor);
  currentRightPeak = std::max(targetRightPeak, currentRightPeak * decayFactor);
  currentLeftRms = std::max(targetLeftRms, currentLeftRms * decayFactor);
  currentRightRms = std::max(targetRightRms, currentRightRms * decayFactor);

  repaint();
}
