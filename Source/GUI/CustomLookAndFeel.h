#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * CustomLookAndFeel defines the custom visual styling of the plugin.
 */
class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
  CustomLookAndFeel();
  ~CustomLookAndFeel() override;

  // Exact palette Colours as specified (hex must match exactly)
  inline static const juce::Colour Background{0xFF121417};
  inline static const juce::Colour Panel{0xFF1A1D22};
  inline static const juce::Colour Border{0xFF2C3038};
  inline static const juce::Colour Maximizer{0xFF00D2FF};
  inline static const juce::Colour Exciter{0xFFFF9500};
  inline static const juce::Colour Master{0xFF34C759};
  inline static const juce::Colour Text{0xFFF2F2F7};

  // Lowercase/camelCase aliases for robust integration
  inline static const juce::Colour background{Background};
  inline static const juce::Colour panel{Panel};
  inline static const juce::Colour border{Border};
  inline static const juce::Colour maximizer{Maximizer};
  inline static const juce::Colour exciter{Exciter};
  inline static const juce::Colour master{Master};
  inline static const juce::Colour text{Text};
  inline static const juce::Colour maximizerAccent{Maximizer};
  inline static const juce::Colour exciterAccent{Exciter};
  inline static const juce::Colour masterAccent{Master};
  inline static const juce::Colour backgroundColour{Background};
  inline static const juce::Colour panelColour{Panel};
  inline static const juce::Colour borderColour{Border};
  inline static const juce::Colour textColour{Text};

  // LookAndFeel_V4 overrides for rotary slider, ComboBox, and ToggleButton
  void drawRotarySlider(juce::Graphics &, int x, int y, int width, int height,
                        float sliderPosProportional, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &) override;

  void drawComboBox(juce::Graphics &, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox &) override;

  juce::Font getComboBoxFont(juce::ComboBox &) override;

  void positionComboBoxText(juce::ComboBox &, juce::Label &) override;

  void drawToggleButton(juce::Graphics &, juce::ToggleButton &,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLookAndFeel)
};
