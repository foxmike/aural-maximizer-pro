#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel() {
  setColour(juce::ComboBox::backgroundColourId, CustomLookAndFeel::Panel);
  setColour(juce::ComboBox::outlineColourId, CustomLookAndFeel::Border);
  setColour(juce::ComboBox::textColourId, CustomLookAndFeel::Text);
  setColour(juce::PopupMenu::backgroundColourId, CustomLookAndFeel::Background);
  setColour(juce::PopupMenu::highlightedBackgroundColourId,
            CustomLookAndFeel::Border);
  setColour(juce::PopupMenu::highlightedTextColourId, CustomLookAndFeel::Text);
  setColour(juce::PopupMenu::headerTextColourId, CustomLookAndFeel::Text);
  setColour(juce::PopupMenu::textColourId, CustomLookAndFeel::Text);
  setColour(juce::Slider::rotarySliderFillColourId,
            CustomLookAndFeel::MaximizerAccent);
  setColour(juce::Slider::rotarySliderOutlineColourId,
            CustomLookAndFeel::Border);
  setColour(juce::Slider::thumbColourId, CustomLookAndFeel::Text);
  setColour(juce::ToggleButton::textColourId, CustomLookAndFeel::Text);
  setColour(juce::ToggleButton::tickColourId,
            CustomLookAndFeel::MaximizerAccent);
  setColour(juce::ToggleButton::tickDisabledColourId,
            CustomLookAndFeel::Border);
}

CustomLookAndFeel::~CustomLookAndFeel() {}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y,
                                         int width, int height,
                                         float sliderPosProportional,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider &slider) {
  auto bounds = juce::Rectangle<float>(
                    static_cast<float>(x), static_cast<float>(y),
                    static_cast<float>(width), static_cast<float>(height))
                    .reduced(2.0f);
  auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
  if (radius <= 0.0f)
    return;

  auto centerX = bounds.getCentreX();
  auto centerY = bounds.getCentreY();
  auto angle = rotaryStartAngle +
               sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

  // Track outer radius
  auto trackRadius = radius - 3.0f;
  if (trackRadius > 0.0f) {
    // Dark slate track background
    juce::Path backgroundTrack;
    backgroundTrack.addCentredArc(centerX, centerY, trackRadius, trackRadius,
                                  0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(CustomLookAndFeel::Border);
    g.strokePath(backgroundTrack,
                 juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    // Active value track arc
    if (sliderPosProportional > 0.0f) {
      juce::Path activeTrack;
      activeTrack.addCentredArc(centerX, centerY, trackRadius, trackRadius,
                                0.0f, rotaryStartAngle, angle, true);
      juce::Colour accentCol =
          slider.findColour(juce::Slider::rotarySliderFillColourId, true);
      if (accentCol == juce::Colour() ||
          accentCol == juce::Colours::transparentBlack)
        accentCol = CustomLookAndFeel::MaximizerAccent;
      g.setColour(accentCol);
      g.strokePath(activeTrack,
                   juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded));
    }
  }

  // Brushed-aluminum knob dial body
  auto knobRadius = juce::jmax(3.0f, radius - 9.0f);
  auto knobArea =
      juce::Rectangle<float>(centerX - knobRadius, centerY - knobRadius,
                             knobRadius * 2.0f, knobRadius * 2.0f);

  // Drop shadow
  g.setColour(CustomLookAndFeel::Background.withAlpha(0.6f));
  g.fillEllipse(knobArea.expanded(1.5f));

  // Outer metallic bevel ring gradient
  juce::ColourGradient ringGradient(juce::Colour(0xff8e959e), knobArea.getX(),
                                    knobArea.getY(), juce::Colour(0xff2c3038),
                                    knobArea.getRight(), knobArea.getBottom(),
                                    false);
  g.setGradientFill(ringGradient);
  g.drawEllipse(knobArea, 1.5f);

  // Precision brushed-aluminum multi-stop linear gradient
  juce::ColourGradient aluminumGradient(
      juce::Colour(0xffd8dce2), knobArea.getX(), knobArea.getY(),
      juce::Colour(0xff545a63), knobArea.getRight(), knobArea.getBottom(),
      false);
  aluminumGradient.addColour(0.2, juce::Colour(0xff9aa0a8));
  aluminumGradient.addColour(
      0.5, juce::Colour(0xffeef2f8)); // Brushed sheen highlight
  aluminumGradient.addColour(0.8, juce::Colour(0xff6c737e));
  g.setGradientFill(aluminumGradient);
  g.fillEllipse(knobArea);

  // Inner cap - dark slate disc
  auto innerCapArea = knobArea.reduced(knobRadius * 0.25f);
  juce::ColourGradient innerGradient(
      juce::Colour(0xff2a2e36), innerCapArea.getX(), innerCapArea.getY(),
      juce::Colour(0xff1a1d22), innerCapArea.getRight(),
      innerCapArea.getBottom(), false);
  g.setGradientFill(innerGradient);
  g.fillEllipse(innerCapArea);
  g.setColour(CustomLookAndFeel::Border);
  g.drawEllipse(innerCapArea, 1.0f);

  // Precision pointer indicator
  juce::Colour accentCol =
      slider.findColour(juce::Slider::rotarySliderFillColourId, true);
  if (accentCol == juce::Colour() ||
      accentCol == juce::Colours::transparentBlack)
    accentCol = CustomLookAndFeel::MaximizerAccent;

  juce::Path pointerPath;
  auto pointerThickness = 2.5f;
  auto pointerLength = knobRadius * 0.65f;
  pointerPath.addRoundedRectangle(-pointerThickness * 0.5f, -knobRadius + 2.0f,
                                  pointerThickness, pointerLength, 1.0f);
  pointerPath.applyTransform(
      juce::AffineTransform::rotation(angle).translated(centerX, centerY));
  g.setColour(accentCol);
  g.fillPath(pointerPath);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics &g, int width, int height,
                                     bool isButtonDown, int buttonX,
                                     int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox &box) {
  auto boxBounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width),
                                          static_cast<float>(height))
                       .reduced(0.5f);
  float cornerSize = 4.0f;
  juce::Colour bgCol = box.isEnabled() ? CustomLookAndFeel::Panel
                                       : CustomLookAndFeel::Background;
  if (isButtonDown || box.isPopupActive())
    bgCol = bgCol.brighter(0.1f);

  g.setColour(bgCol);
  g.fillRoundedRectangle(boxBounds, cornerSize);

  g.setColour(CustomLookAndFeel::Border);
  g.drawRoundedRectangle(boxBounds, cornerSize, 1.0f);

  // Dropdown arrow
  if (buttonW > 0 && buttonH > 0) {
    juce::Path arrow;
    auto arrowZone = juce::Rectangle<float>(
        static_cast<float>(buttonX), static_cast<float>(buttonY),
        static_cast<float>(buttonW), static_cast<float>(buttonH));
    auto arrowW = 8.0f;
    auto arrowH = 5.0f;
    auto arrowCentreX = arrowZone.getCentreX();
    auto arrowCentreY = arrowZone.getCentreY();
    arrow.addTriangle(
        arrowCentreX - arrowW * 0.5f, arrowCentreY - arrowH * 0.5f,
        arrowCentreX + arrowW * 0.5f, arrowCentreY - arrowH * 0.5f,
        arrowCentreX, arrowCentreY + arrowH * 0.5f);
    g.setColour(box.isEnabled() ? CustomLookAndFeel::Text
                                : CustomLookAndFeel::Border);
    g.fillPath(arrow);
  }
}

juce::Font CustomLookAndFeel::getComboBoxFont(juce::ComboBox &box) {
  juce::ignoreUnused(box);
  return juce::Font(14.0f);
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics &g, int width,
                                                int height) {
  auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width),
                                       static_cast<float>(height));
  g.setColour(CustomLookAndFeel::Panel);
  g.fillRect(bounds);
  g.setColour(CustomLookAndFeel::Border);
  g.drawRect(bounds, 1.0f);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics &g,
                                          const juce::Rectangle<int> &area,
                                          bool isHighlighted,
                                          const juce::PopupMenu::Item &item) {
  auto bounds = area.toFloat();

  if (item.isSeparator) {
    g.setColour(CustomLookAndFeel::Border);
    g.fillRect(bounds.reduced(10.0f, 0.0f)
                   .removeFromTop(1.0f)
                   .withY(bounds.getCentreY()));
    return;
  }

  if (item.isSectionHeader) {
    g.setColour(CustomLookAndFeel::Text.withAlpha(0.6f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(item.text, bounds.reduced(10.0f, 0.0f),
               juce::Justification::centredLeft, true);
    return;
  }

  if (isHighlighted && item.isEnabled) {
    g.setColour(CustomLookAndFeel::Border);
    g.fillRect(bounds);
  }

  juce::Colour textColour = item.isEnabled
                                ? CustomLookAndFeel::Text
                                : CustomLookAndFeel::Text.withAlpha(0.4f);

  if (item.isTicked) {
    juce::Path tick;
    auto tickBox = bounds.removeFromLeft(20.0f);
    tick.startNewSubPath(tickBox.getX() + 6.0f, tickBox.getCentreY());
    tick.lineTo(tickBox.getX() + 9.0f, tickBox.getCentreY() + 4.0f);
    tick.lineTo(tickBox.getX() + 14.0f, tickBox.getCentreY() - 4.0f);
    g.setColour(CustomLookAndFeel::MaximizerAccent);
    g.strokePath(tick, juce::PathStrokeType(2.0f));
  }

  g.setColour(textColour);
  g.setFont(juce::Font(14.0f));
  g.drawText(item.text, bounds.reduced(10.0f, 0.0f),
             juce::Justification::centredLeft, true);
}

void CustomLookAndFeel::drawToggleButton(juce::Graphics &g,
                                         juce::ToggleButton &button,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown) {
  auto fontSize =
      juce::jmin(15.0f, static_cast<float>(button.getHeight()) * 0.75f);
  auto tickSize = fontSize * 1.1f;
  auto bounds =
      juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(button.getWidth()),
                             static_cast<float>(button.getHeight()));
  auto tickBox = juce::Rectangle<float>(
      4.0f, bounds.getCentreY() - tickSize * 0.5f, tickSize, tickSize);

  juce::Colour boxBg = CustomLookAndFeel::Panel;
  if (shouldDrawButtonAsDown)
    boxBg = boxBg.darker(0.1f);
  else if (shouldDrawButtonAsHighlighted)
    boxBg = boxBg.brighter(0.1f);

  g.setColour(boxBg);
  g.fillRoundedRectangle(tickBox, 3.0f);

  g.setColour(CustomLookAndFeel::Border);
  g.drawRoundedRectangle(tickBox, 3.0f, 1.0f);

  if (button.getToggleState()) {
    juce::Colour tickCol =
        button.findColour(juce::ToggleButton::tickColourId, true);
    if (tickCol == juce::Colour() || tickCol == juce::Colours::transparentBlack)
      tickCol = CustomLookAndFeel::MaximizerAccent;

    juce::Path tick;
    tick.startNewSubPath(tickBox.getX() + tickSize * 0.25f,
                         tickBox.getCentreY());
    tick.lineTo(tickBox.getX() + tickSize * 0.45f,
                tickBox.getCentreY() + tickSize * 0.25f);
    tick.lineTo(tickBox.getX() + tickSize * 0.75f,
                tickBox.getCentreY() - tickSize * 0.25f);
    g.setColour(tickCol);
    g.strokePath(tick, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
  }

  if (button.getButtonText().isNotEmpty()) {
    auto textBounds = bounds.withTrimmedLeft(tickBox.getRight() + 6.0f);
    g.setColour(button.isEnabled() ? CustomLookAndFeel::Text
                                   : CustomLookAndFeel::Text.withAlpha(0.4f));
    g.setFont(juce::Font(fontSize));
    g.drawText(button.getButtonText(), textBounds,
               juce::Justification::centredLeft, true);
  }
}
