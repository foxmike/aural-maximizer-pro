#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * AuralExciter stub class for adding harmonic excitement to audio.
 */
class AuralExciter
{
public:
    AuralExciter();
    ~AuralExciter();

    /**
     * Prepares the exciter for processing.
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
     * Resets the internal state of the exciter.
     */
    void reset();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuralExciter)
};
