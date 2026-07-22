#include "CustomLookAndFeel.h"
#include <cmath>

CustomLookAndFeel::CustomLookAndFeel() {
  // Apply standard palette colors to default LookAndFeel color IDs
  setColour(juce::ComboBox::backgroundColourId, Panel);
  setColour(juce::ComboBox::outlineColourId, Border);
  setColour(juce::ComboBox::textColourId, Text);

  setColour(juce::Label::textColourId, Text);

  setColour(juce::ToggleButton::textColourId, Text);
  setColour(juce::ToggleButton::tickColourId, Maximizer);
  setColour(juce::ToggleButton::tickDisabledColourId, Border);

  setColour(juce::Slider::rotarySliderFillColourId, Maximizer);
  setColour(juce::Slider::rotarySliderOutlineColourId, Border);
  setColour(juce::Slider::thumbColourId, Maximizer);
}

CustomLookAndFeel::~CustomLookAndFeel() {}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y,
                                         int width, int height,
                                         float sliderPosProportional,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider &slider) {
  auto centreX = (float)x + (float)width * 0.5f;
  auto centreY = (float)y + (float)height * 0.5f;

  auto outerRadius = juce::jmin((float)width, (float)height) * 0.5f - 4.0f;
  auto knobRadius = outerRadius - 6.0f;

  if (knobRadius <= 0.0f)
    return;

  // 1. Theme and Accent Color Determination
  juce::Colour accentColour = CustomLookAndFeel::Maximizer;
  if (slider.isColourSpecified(juce::Slider::rotarySliderFillColourId)) {
    accentColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
  } else {
    auto name = slider.getName().toLowerCase() + " " +
                slider.getComponentID().toLowerCase();
    if (name.contains("exciter") || name.contains("harmonics") ||
        name.contains("timbre")) {
      accentColour = CustomLookAndFeel::Exciter;
    } else if (name.contains("master") || name.contains("routing") ||
               name.contains("output") || name.contains("gain") ||
               name.contains("volume")) {
      accentColour = CustomLookAndFeel::Master;
    } else if (name.contains("maximizer") || name.contains("amount") ||
               name.contains("ratio")) {
      accentColour = CustomLookAndFeel::Maximizer;
    }
  }

  if (!slider.isEnabled()) {
    accentColour = juce::Colours::grey;
  }

  // 2. Chassis Cutout Background
  g.setColour(CustomLookAndFeel::Background);
  g.fillEllipse(centreX - outerRadius, centreY - outerRadius,
                outerRadius * 2.0f, outerRadius * 2.0f);

  g.setColour(CustomLookAndFeel::Border);
  g.drawEllipse(centreX - outerRadius, centreY - outerRadius,
                outerRadius * 2.0f, outerRadius * 2.0f, 1.5f);

  // 3. Rotary Track/Arc
  auto currentAngle =
      rotaryStartAngle +
      sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
  auto trackRadius = outerRadius - 2.5f;

  if (trackRadius > 0.0f) {
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centreX, centreY, trackRadius, trackRadius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(CustomLookAndFeel::Border);
    g.strokePath(backgroundArc,
                 juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, trackRadius, trackRadius, 0.0f,
                           rotaryStartAngle, currentAngle, true);
    g.setColour(accentColour);
    g.strokePath(valueArc,
                 juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));
  }

  // 4. Knob Body (Precision Brushed Aluminum)
  g.setColour(juce::Colours::black.withAlpha(0.35f));
  g.fillEllipse(centreX - knobRadius + 1.0f, centreY - knobRadius + 1.5f,
                knobRadius * 2.0f, knobRadius * 2.0f);

  // Anisotropic metal face gradient
  juce::ColourGradient metalGradient(
      juce::Colour(0xFFE5E5EA), centreX - knobRadius, centreY - knobRadius,
      juce::Colour(0xFF3A3D42), centreX + knobRadius, centreY + knobRadius,
      false);
  metalGradient.addColour(0.12, juce::Colour(0xFFF2F2F7));
  metalGradient.addColour(0.28, juce::Colour(0xFFAEAEB2));
  metalGradient.addColour(0.45, juce::Colour(0xFFD1D1D6));
  metalGradient.addColour(0.62, juce::Colour(0xFF636366));
  metalGradient.addColour(0.78, juce::Colour(0xFFC7C7CC));
  metalGradient.addColour(0.92, juce::Colour(0xFF48484A));

  g.setGradientFill(metalGradient);
  g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f,
                knobRadius * 2.0f);

  // Lathe-turned brushed rings (concentric sub-pixel detail)
  g.setColour(juce::Colours::black.withAlpha(0.06f));
  g.drawEllipse(centreX - knobRadius * 0.85f, centreY - knobRadius * 0.85f,
                knobRadius * 1.7f, knobRadius * 1.7f, 0.5f);
  g.drawEllipse(centreX - knobRadius * 0.70f, centreY - knobRadius * 0.70f,
                knobRadius * 1.4f, knobRadius * 1.4f, 0.5f);
  g.drawEllipse(centreX - knobRadius * 0.55f, centreY - knobRadius * 0.55f,
                knobRadius * 1.1f, knobRadius * 1.1f, 0.5f);
  g.drawEllipse(centreX - knobRadius * 0.40f, centreY - knobRadius * 0.40f,
                knobRadius * 0.8f, knobRadius * 0.8f, 0.5f);
  g.drawEllipse(centreX - knobRadius * 0.25f, centreY - knobRadius * 0.25f,
                knobRadius * 0.5f, knobRadius * 0.5f, 0.5f);

  // Outer polished rim / bevel edge
  juce::ColourGradient rimGradient(
      juce::Colour(0xFFFFFFFF).withAlpha(0.6f), centreX - knobRadius,
      centreY - knobRadius, juce::Colour(0xFF000000).withAlpha(0.35f),
      centreX + knobRadius, centreY + knobRadius, false);
  g.setGradientFill(rimGradient);
  g.drawEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f,
                knobRadius * 2.0f, 1.0f);

  // 5. Indicator Needle/Pointer Line
  auto pointerLength = knobRadius - 1.0f;
  auto pointerThickness = 2.0f;
  float pointerStart = knobRadius * 0.20f;

  auto startX = centreX + std::sin(currentAngle) * pointerStart;
  auto startY = centreY - std::cos(currentAngle) * pointerStart;
  auto endX = centreX + std::sin(currentAngle) * pointerLength;
  auto endY = centreY - std::cos(currentAngle) * pointerLength;

  // Shadow under the pointer
  g.setColour(juce::Colours::black.withAlpha(0.4f));
  g.drawLine(startX + 0.5f, startY + 0.75f, endX + 0.5f, endY + 0.75f,
             pointerThickness);

  // The accent-colored pointer itself
  g.setColour(accentColour);
  g.drawLine(startX, startY, endX, endY, pointerThickness);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics &g, int width, int height,
                                     bool isButtonDown, int buttonX,
                                     int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox &box) {
  auto cornerSize = 4.0f;

  // Background fill
  g.setColour(CustomLookAndFeel::Panel);
  g.fillRoundedRectangle(1.0f, 1.0f, (float)width - 2.0f, (float)height - 2.0f,
                         cornerSize);

  // Border outline
  g.setColour(CustomLookAndFeel::Border);
  g.drawRoundedRectangle(1.0f, 1.0f, (float)width - 2.0f, (float)height - 2.0f,
                         cornerSize, 1.0f);

  // Highlight if pressed/active
  if (isButtonDown) {
    juce::Colour accent = CustomLookAndFeel::Maximizer;
    if (box.isColourSpecified(juce::ComboBox::outlineColourId))
      accent = box.findColour(juce::ComboBox::outlineColourId);

    g.setColour(accent.withAlpha(0.15f));
    g.fillRoundedRectangle(1.0f, 1.0f, (float)width - 2.0f,
                           (float)height - 2.0f, cornerSize);
    g.setColour(accent);
    g.drawRoundedRectangle(1.0f, 1.0f, (float)width - 2.0f,
                           (float)height - 2.0f, cornerSize, 1.0f);
  }

  // Draw arrow container divider line
  g.setColour(CustomLookAndFeel::Border);
  g.drawLine((float)buttonX, 1.0f, (float)buttonX, (float)height - 1.0f, 1.0f);

  // Calculate arrow geometry
  juce::Path arrowPath;
  float arrowW = 8.0f;
  float arrowH = 5.0f;
  float arrowX = (float)buttonX + ((float)buttonW - arrowW) * 0.5f;
  float arrowY = (float)buttonY + ((float)buttonH - arrowH) * 0.5f;

  arrowPath.startNewSubPath(arrowX, arrowY);
  arrowPath.lineTo(arrowX + arrowW, arrowY);
  arrowPath.lineTo(arrowX + arrowW * 0.5f, arrowY + arrowH);
  arrowPath.closeSubPath();

  g.setColour(CustomLookAndFeel::Text);
  g.fillPath(arrowPath);
}

juce::Font CustomLookAndFeel::getComboBoxFont(juce::ComboBox &box) {
  return juce::Font(juce::FontOptions{}.withHeight(
      juce::jmin(16.0f, (float)box.getHeight() * 0.85f)));
}

void CustomLookAndFeel::positionComboBoxText(juce::ComboBox &box,
                                             juce::Label &label) {
  label.setBounds(1, 1, box.getWidth() - 30, box.getHeight() - 2);
  label.setFont(getComboBoxFont(box));
  label.setColour(juce::Label::textColourId, CustomLookAndFeel::Text);
}

void CustomLookAndFeel::drawToggleButton(juce::Graphics &g,
                                         juce::ToggleButton &button,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown) {
  auto fontSize = juce::jmin(15.0f, (float)button.getHeight() * 0.75f);
  auto tickWidth = fontSize * 1.1f;

  float tickX = 4.0f;
  float tickY = ((float)button.getHeight() - tickWidth) * 0.5f;

  juce::Rectangle<float> tickBounds(tickX, tickY, tickWidth, tickWidth);

  // Custom theme colors
  juce::Colour accentColour = CustomLookAndFeel::Maximizer;
  if (button.isColourSpecified(juce::ToggleButton::tickColourId)) {
    accentColour = button.findColour(juce::ToggleButton::tickColourId);
  } else {
    auto name = button.getButtonText().toLowerCase() + " " +
                button.getName().toLowerCase() + " " +
                button.getComponentID().toLowerCase();
    if (name.contains("exciter")) {
      accentColour = CustomLookAndFeel::Exciter;
    } else if (name.contains("master") || name.contains("routing") ||
               name.contains("bypass") || name.contains("mono") ||
               name.contains("mid") || name.contains("side")) {
      accentColour = CustomLookAndFeel::Master;
    } else if (name.contains("maximizer") || name.contains("max")) {
      accentColour = CustomLookAndFeel::Maximizer;
    }
  }

  // Draw tick box panel background
  g.setColour(CustomLookAndFeel::Panel);
  g.fillRoundedRectangle(tickBounds, 4.0f);

  // If checked, highlight/fill the box
  if (button.getToggleState()) {
    g.setColour(accentColour.withAlpha(0.15f));
    g.fillRoundedRectangle(tickBounds, 4.0f);
    g.setColour(accentColour);
    g.drawRoundedRectangle(tickBounds, 4.0f, 1.5f);

    // Draw tick mark inside
    juce::Path tickPath;
    auto innerW = tickWidth * 0.55f;
    auto innerX = tickX + (tickWidth - innerW) * 0.5f;
    auto innerY = tickY + (tickWidth - innerW) * 0.5f;

    tickPath.startNewSubPath(innerX, innerY + innerW * 0.45f);
    tickPath.lineTo(innerX + innerW * 0.35f, innerY + innerW * 0.85f);
    tickPath.lineTo(innerX + innerW * 0.95f, innerY + innerW * 0.05f);

    g.setColour(accentColour);
    g.strokePath(tickPath,
                 juce::PathStrokeType(2.0f, juce::PathStrokeType::mitered,
                                      juce::PathStrokeType::rounded));
  } else {
    g.setColour(CustomLookAndFeel::Border);
    g.drawRoundedRectangle(tickBounds, 4.0f, 1.0f);
  }

  // Draw button text
  g.setColour(CustomLookAndFeel::Text);
  g.setFont(juce::Font(juce::FontOptions{}.withHeight(fontSize)));

  if (!button.isEnabled())
    g.setOpacity(0.5f);

  g.drawFittedText(button.getButtonText(),
                   button.getLocalBounds()
                       .withTrimmedLeft(juce::roundToInt(tickWidth) + 10)
                       .withTrimmedRight(2),
                   juce::Justification::centredLeft, 10);
}
