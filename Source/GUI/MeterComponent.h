#pragma once

#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MeterComponent renders real-time L/R peak and RMS LED-style meter bars.
 * Level updates are processed via a lock-free atomic path and refreshed
 * on the message thread via a capped juce::Timer.
 */
class MeterComponent : public juce::Component, private juce::Timer
{
public:
    static constexpr int updateRateHz = 60;

    MeterComponent();
    ~MeterComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setLevels (float leftPeak, float rightPeak, float leftRms, float rightRms);
    void pushLevels (float leftPeak, float rightPeak, float leftRms, float rightRms);

    float getLeftPeak() const noexcept;
    float getRightPeak() const noexcept;
    float getLeftRms() const noexcept;
    float getRightRms() const noexcept;

private:
    void timerCallback() override;

    std::atomic<float> leftPeak { 0.0f };
    std::atomic<float> rightPeak { 0.0f };
    std::atomic<float> leftRms { 0.0f };
    std::atomic<float> rightRms { 0.0f };

    float currentLeftPeak { 0.0f };
    float currentRightPeak { 0.0f };
    float currentLeftRms { 0.0f };
    float currentRightRms { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterComponent)
};
