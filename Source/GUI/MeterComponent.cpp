#include "MeterComponent.h"
#include "../PluginProcessor.h"
#include <algorithm>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <unordered_map>

namespace {
struct MeterState {
  float leftPeakVisual = 0.0f;
  float leftRmsVisual = 0.0f;
  float rightPeakVisual = 0.0f;
  float rightRmsVisual = 0.0f;

  float leftPeakHold = 0.0f;
  float rightPeakHold = 0.0f;
  int leftPeakHoldTime = 0;
  int rightPeakHoldTime = 0;
};

// Global visual state mapping for components on the message thread.
std::unordered_map<const MeterComponent *, MeterState> g_meterStates;

float gainToNormalizedLevel(float gain) noexcept {
  constexpr float minDb = -50.0f;
  constexpr float maxDb = 6.0f;
  float db = juce::Decibels::gainToDecibels(gain, -100.0f);
  float normalized = (db - minDb) / (maxDb - minDb);
  return juce::jlimit(0.0f, 1.0f, normalized);
}
} // namespace

MeterComponent::MeterComponent(AuralMaximizerProAudioProcessor &processor)
    : processorRef(processor) {
  startTimerHz(timerHz);
}

MeterComponent::~MeterComponent() {
  stopTimer();
  g_meterStates.erase(this);
}

void MeterComponent::paint(juce::Graphics &g) {
  const auto bounds = getLocalBounds().toFloat().reduced(4.0f);

  // Background panel fill
  g.fillAll(juce::Colour(0xFF121417));

  if (bounds.isEmpty())
    return;

  // Retrieve visual state
  MeterState state;
  auto stateIt = g_meterStates.find(this);
  if (stateIt != g_meterStates.end()) {
    state = stateIt->second;
  }

  // Convert visual levels to 0.0 - 1.0 normalized meter height
  float leftPeakNorm = gainToNormalizedLevel(state.leftPeakVisual);
  float leftRmsNorm = gainToNormalizedLevel(state.leftRmsVisual);
  float rightPeakNorm = gainToNormalizedLevel(state.rightPeakVisual);
  float rightRmsNorm = gainToNormalizedLevel(state.rightRmsVisual);

  float leftPeakHoldNorm = gainToNormalizedLevel(state.leftPeakHold);
  float rightPeakHoldNorm = gainToNormalizedLevel(state.rightPeakHold);

  // Layout dimensions
  const float totalWidth = bounds.getWidth();
  constexpr float channelLabelHeight = 10.0f;
  constexpr float barLabelHeight = 11.0f;
  const auto meterBounds = bounds.withTrimmedTop(channelLabelHeight)
                               .withTrimmedBottom(barLabelHeight);
  const float totalHeight = meterBounds.getHeight();

  // Center label width
  const float labelWidth = juce::jlimit(20.0f, 40.0f, totalWidth * 0.2f);
  const float channelWidth = (totalWidth - labelWidth) / 2.0f;

  const float leftChannelX = bounds.getX();
  const float labelX = leftChannelX + channelWidth;
  const float rightChannelX = labelX + labelWidth;

  // Inside each channel, we draw Peak (outer) and RMS (inner)
  const float gap = 2.0f;
  const float padding = 4.0f;
  const float barWidth = (channelWidth - gap - padding * 2.0f) / 2.0f;

  const float leftPeakX = leftChannelX + padding;
  const float leftRmsX = leftPeakX + barWidth + gap;

  const float rightPeakX = rightChannelX + padding;
  const float rightRmsX = rightPeakX + barWidth + gap;

  // Helper lambda to draw a vertical LED/bar meter
  auto drawLedMeter = [&](float x, float normLevel, float normHold) {
    const auto barBounds =
        juce::Rectangle<float>(x, meterBounds.getY(), barWidth, totalHeight);

    // Draw track background
    g.setColour(juce::Colour(0xFF1A1D22));
    g.fillRect(barBounds);

    // Divide the height into segments
    const int numSegments = 32;
    const float segmentGap = 1.0f;
    const float segmentHeight =
        (totalHeight - (numSegments - 1) * segmentGap) / numSegments;

    for (int i = 0; i < numSegments; ++i) {
      // Segment starts from bottom
      float segY = meterBounds.getBottom() -
                   (i + 1) * (segmentHeight + segmentGap) + segmentGap;
      float segProp = (float)i / (float)(numSegments - 1);

      // Map segment proportion to dB to determine color
      constexpr float minDb = -50.0f;
      constexpr float maxDb = 6.0f;
      float segDb = minDb + segProp * (maxDb - minDb);

      // Determine active color for this segment
      juce::Colour activeColor;
      if (segDb < -18.0f) {
        activeColor = juce::Colour(0xFF34C759); // Green (Master)
      } else if (segDb < -6.0f) {
        activeColor = juce::Colour(0xFFD4F359); // Green-Yellow
      } else if (segDb < 0.0f) {
        activeColor = juce::Colour(0xFFFF9500); // Orange (Exciter)
      } else {
        activeColor = juce::Colour(0xFFFF3B30); // Red
      }

      // Is this segment active?
      bool isActive = (normLevel >= segProp);

      if (isActive) {
        g.setColour(activeColor);
      } else {
        g.setColour(activeColor.withAlpha(0.12f));
      }

      g.fillRect(juce::Rectangle<float>(x, segY, barWidth, segmentHeight));
    }

    // Draw Peak Hold indicator as a thin line
    if (normHold > 0.0f) {
      float holdY = juce::jlimit(
          meterBounds.getY() + 1.0f, meterBounds.getBottom() - 1.0f,
          meterBounds.getBottom() - normHold * totalHeight);
      g.setColour(juce::Colours::white.withAlpha(0.85f));
      g.fillRect(juce::Rectangle<float>(x, holdY - 1.0f, barWidth, 2.0f));
    }
  };

  // Draw the 4 meters
  drawLedMeter(leftPeakX, leftPeakNorm, leftPeakHoldNorm);
  drawLedMeter(leftRmsX, leftRmsNorm, 0.0f);
  drawLedMeter(rightPeakX, rightPeakNorm, rightPeakHoldNorm);
  drawLedMeter(rightRmsX, rightRmsNorm, 0.0f);

  const auto textColour = juce::Colour(0xFFF2F2F7);
  g.setColour(textColour.withAlpha(0.7f));
  g.setFont(8.5f);
  g.drawFittedText("L",
                   juce::Rectangle<float>(leftChannelX, bounds.getY(),
                                          channelWidth, channelLabelHeight)
                       .toNearestInt(),
                   juce::Justification::centred, 1);
  g.drawFittedText("R",
                   juce::Rectangle<float>(rightChannelX, bounds.getY(),
                                          channelWidth, channelLabelHeight)
                       .toNearestInt(),
                   juce::Justification::centred, 1);

  g.setColour(textColour.withAlpha(0.6f));
  g.setFont(7.5f);
  const auto drawBarLabel = [&](juce::StringRef text, float x) {
    g.drawFittedText(text,
                     juce::Rectangle<float>(x, meterBounds.getBottom(),
                                            barWidth, barLabelHeight)
                         .toNearestInt(),
                     juce::Justification::centred, 1);
  };
  drawBarLabel("PK", leftPeakX);
  drawBarLabel("RMS", leftRmsX);
  drawBarLabel("PK", rightPeakX);
  drawBarLabel("RMS", rightRmsX);

  // Draw Center dB Labels
  const float dbs[] = {6.0f,   0.0f,   -6.0f,  -12.0f,
                       -18.0f, -24.0f, -36.0f, -50.0f};
  g.setFont(9.0f);

  for (float db : dbs) {
    float norm = gainToNormalizedLevel(juce::Decibels::decibelsToGain(db));
    float y = meterBounds.getBottom() - norm * totalHeight;

    g.setColour(juce::Colour(0xFFF2F2F7).withAlpha(0.5f));
    juce::String text =
        (db > 0.0f ? "+" : "") + juce::String(juce::roundToInt(db));
    g.drawFittedText(text,
                     juce::Rectangle<float>(labelX, y - 6.0f, labelWidth, 12.0f)
                         .toNearestInt(),
                     juce::Justification::centred, 1);

    // Draw tick lines
    g.setColour(juce::Colour(0xFF2C3038).withAlpha(0.4f));
    g.drawHorizontalLine(juce::roundToInt(y), leftChannelX, labelX);
    g.drawHorizontalLine(juce::roundToInt(y), labelX + labelWidth,
                         bounds.getRight());
  }
}

void MeterComponent::resized() {}

void MeterComponent::timerCallback() {
  const auto *messageManager =
      juce::MessageManager::getInstanceWithoutCreating();
  jassert(messageManager != nullptr &&
          messageManager->isThisTheMessageThread());

  const auto levels = processorRef.getMeterLevels();
  const auto rawLeftPeak = levels.leftPeak;
  const auto rawLeftRms = levels.leftRms;
  const auto rawRightPeak = levels.rightPeak;
  const auto rawRightRms = levels.rightRms;

  // Retrieve or create the state for this component
  auto &state = g_meterStates[this];

  // Decay the visual levels
  constexpr float peakDecayCoeff = 0.92f;
  constexpr float rmsDecayCoeff = 0.85f;

  state.leftPeakVisual =
      std::max(rawLeftPeak, state.leftPeakVisual * peakDecayCoeff);
  state.leftRmsVisual =
      std::max(rawLeftRms, state.leftRmsVisual * rmsDecayCoeff);
  state.rightPeakVisual =
      std::max(rawRightPeak, state.rightPeakVisual * peakDecayCoeff);
  state.rightRmsVisual =
      std::max(rawRightRms, state.rightRmsVisual * rmsDecayCoeff);

  // Left Peak Hold
  if (rawLeftPeak >= state.leftPeakHold) {
    state.leftPeakHold = rawLeftPeak;
    state.leftPeakHoldTime = 30; // 1 second hold at 30Hz
  } else {
    if (state.leftPeakHoldTime > 0) {
      --state.leftPeakHoldTime;
    } else {
      state.leftPeakHold = std::max(rawLeftPeak, state.leftPeakHold * 0.95f);
    }
  }

  // Right Peak Hold
  if (rawRightPeak >= state.rightPeakHold) {
    state.rightPeakHold = rawRightPeak;
    state.rightPeakHoldTime = 30;
  } else {
    if (state.rightPeakHoldTime > 0) {
      --state.rightPeakHoldTime;
    } else {
      state.rightPeakHold = std::max(rawRightPeak, state.rightPeakHold * 0.95f);
    }
  }

  repaint();
}
