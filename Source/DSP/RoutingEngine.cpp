#include "RoutingEngine.h"
#include "SonicMaximizer.h"
#include "AuralExciter.h"

RoutingEngine::RoutingEngine()
{
}

RoutingEngine::~RoutingEngine()
{
}

void RoutingEngine::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock, numChannels);
}

void RoutingEngine::prepare(double sampleRate, int samplesPerBlock)
{
    prepare(sampleRate, samplesPerBlock, 2);
}

void RoutingEngine::process(juce::AudioBuffer<float>& buffer,
                            SonicMaximizer& maximizer,
                            AuralExciter& exciter,
                            RoutingMode mode,
                            float targetParallelBlend)
{
    juce::ignoreUnused(buffer, maximizer, exciter, mode, targetParallelBlend);
}

void RoutingEngine::process(juce::AudioBuffer<float>& buffer,
                            SonicMaximizer* maximizer,
                            AuralExciter* exciter,
                            RoutingMode mode,
                            float targetParallelBlend)
{
    juce::ignoreUnused(buffer, maximizer, exciter, mode, targetParallelBlend);
}

void RoutingEngine::process(juce::AudioBuffer<float>& buffer)
{
    juce::ignoreUnused(buffer);
}

void RoutingEngine::reset()
{
}

void RoutingEngine::setRampDuration(double durationSeconds)
{
    juce::ignoreUnused(durationSeconds);
}


