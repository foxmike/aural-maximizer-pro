#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * SpectrumVisualizer stub component for rendering audio spectrum.
 */
class SpectrumVisualizer : public juce::Component
{
public:
    SpectrumVisualizer();
    ~SpectrumVisualizer() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumVisualizer)
};
