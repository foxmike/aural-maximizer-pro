#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * CustomLookAndFeel stub class inheriting from juce::LookAndFeel_V4.
 */
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomLookAndFeel)
};
