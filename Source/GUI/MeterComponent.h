#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MeterComponent stub component for rendering audio level meters.
 */
class MeterComponent : public juce::Component
{
public:
    MeterComponent();
    ~MeterComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterComponent)
};
