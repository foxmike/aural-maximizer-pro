#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * SonicMaximizer stub class for audio maximization.
 */
class SonicMaximizer
{
public:
    SonicMaximizer();
    ~SonicMaximizer();

    /**
     * Prepares the maximizer for processing.
     * @param sampleRate The sample rate of the audio.
     * @param samplesPerBlock The number of samples per block.
     */
    void prepare(double sampleRate, int samplesPerBlock);

    /**
     * Processes the provided audio buffer.
     * @param buffer The audio buffer to process.
     */
    void process(juce::AudioBuffer<float>& buffer);

    /**
     * Resets the internal state of the maximizer.
     */
    void reset();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SonicMaximizer)
};
