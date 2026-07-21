#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class SonicMaximizer;
class AuralExciter;

/**
 * Routing modes for the processing chain.
 * Defines how Module A (Sonic Maximizer) and Module B (Aural Exciter) are connected.
 */
enum class RoutingMode
{
    SerialMaximizerThenExciter = 0,
    SerialExciterThenMaximizer,
    ParallelMatrixBlend
};

/**
 * RoutingEngine controls signal routing and matrix blending between
 * SonicMaximizer and AuralExciter DSP modules.
 *
 * Real-Time Safety Contract:
 * - Audio thread processing: process() has no locks, no dynamic heap allocations,
 *   and no blocking I/O operations.
 * - Mode switches and parameter updates do not trigger memory reallocation.
 * - Scratch buffers are pre-allocated during prepare() based on max block size and channel count.
 * - PARALLEL_BLEND parameter transitions are smoothed using juce::SmoothedValue<float>
 *   to eliminate audible clicks during mode or blend parameter changes.
 */
class RoutingEngine
{
public:
    RoutingEngine();
    ~RoutingEngine();

    /**
     * Prepares the routing engine and pre-allocates scratch buffers.
     * Sized from samplesPerBlock and numChannels. Must be called before audio processing.
     *
     * @param sampleRate The sample rate in Hz.
     * @param samplesPerBlock The maximum number of samples per block.
     * @param numChannels The number of audio channels (default: 2).
     */
    void prepare(double sampleRate, int samplesPerBlock, int numChannels = 2);

    /**
     * Legacy prepare overload for two-channel setups.
     *
     * @param sampleRate The sample rate in Hz.
     * @param samplesPerBlock The maximum number of samples per block.
     */
    void prepare(double sampleRate, int samplesPerBlock);

    /**
     * Primary process method executing signal routing across DSP modules.
     * Real-time safe: zero heap allocations, zero locks, zero blocking operations.
     *
     * @param buffer Audio buffer to process in-place.
     * @param maximizer Reference to the SonicMaximizer module.
     * @param exciter Reference to the AuralExciter module.
     * @param mode Target routing mode.
     * @param targetParallelBlend Target blend factor (K) for ParallelMatrixBlend mode [0.0 to 1.0].
     */
    void process(juce::AudioBuffer<float>& buffer,
                 SonicMaximizer& maximizer,
                 AuralExciter& exciter,
                 RoutingMode mode,
                 float targetParallelBlend);

    /**
     * Pointer variant of process method for signal routing across DSP modules.
     * Real-time safe: zero heap allocations, zero locks, zero blocking operations.
     *
     * @param buffer Audio buffer to process in-place.
     * @param maximizer Pointer to the SonicMaximizer module.
     * @param exciter Pointer to the AuralExciter module.
     * @param mode Target routing mode.
     * @param targetParallelBlend Target blend factor (K) for ParallelMatrixBlend mode [0.0 to 1.0].
     */
    void process(juce::AudioBuffer<float>& buffer,
                 SonicMaximizer* maximizer,
                 AuralExciter* exciter,
                 RoutingMode mode,
                 float targetParallelBlend);

    /**
     * Single-buffer process overload for backward compatibility.
     *
     * @param buffer Audio buffer to process.
     */
    void process(juce::AudioBuffer<float>& buffer);

    /**
     * Resets internal state and smoothed values.
     */
    void reset();

    /**
     * Sets the ramp duration in seconds for smoothed parameter transitions.
     *
     * @param durationSeconds Ramp duration in seconds.
     */
    void setRampDuration(double durationSeconds);

private:
    // Pre-allocated per-channel scratch buffers sized in prepare() to avoid heap allocations in process()
    juce::AudioBuffer<float> scratchBufferA;
    juce::AudioBuffer<float> scratchBufferB;

    // Smoothed value for PARALLEL_BLEND parameter (K) so mode/blend changes do not click
    juce::SmoothedValue<float> parallelBlend;

    double currentSampleRate { 44100.0 };
    int currentBlockSize { 512 };
    int currentNumChannels { 2 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingEngine)
};


