#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * CustomLookAndFeel class defining visual styling and palette constants for AuralMaximizerPro.
 * Inherits from juce::LookAndFeel_V4.
 */
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override;

    // Palette Colour constants (hex must match exactly)
    inline static const juce::Colour Background      { 0xff121417 };
    inline static const juce::Colour Panel           { 0xff1a1d22 };
    inline static const juce::Colour Border          { 0xff2c3038 };
    inline static const juce::Colour MaximizerAccent { 0xff00d2ff };
    inline static const juce::Colour ExciterAccent   { 0xffff9500 };
    inline static const juce::Colour MasterAccent    { 0xff34c759 };
    inline static const juce::Colour Text            { 0xfff2f2f7 };

    // Palette aliases
    inline static const juce::Colour Maximizer        { 0xff00d2ff };
    inline static const juce::Colour Exciter          { 0xffff9500 };
    inline static const juce::Colour Master           { 0xff34c759 };

    // LookAndFeel_V4 overrides for rotary sliders, combo boxes, and toggle buttons
    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g,
                       int width, int height,
                       bool isButtonDown,
                       int buttonX, int buttonY,
                       int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    juce::Font getComboBoxFont (juce::ComboBox& box) override;

    void drawPopupMenuBackground (juce::Graphics& g,
                                  int width, int height) override;

    void drawPopupMenuItem (juce::Graphics& g,
                            const juce::Rectangle<int>& area,
                            bool isHighlighted,
                            const juce::PopupMenu::Item& item) override;

    void drawToggleButton (juce::Graphics& g,
                           juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomLookAndFeel)
};

