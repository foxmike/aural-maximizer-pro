#pragma once

#include <array>

#include <juce_dsp/juce_dsp.h>

namespace nSpace
{
    /** Phase-aligns three frequency bands and applies a dynamic low contour. */
    class SonicMaximizer
    {
    public:
        enum class PhaseMode
        {
            classic,
            modern
        };

        SonicMaximizer();
        ~SonicMaximizer() = default;

        void prepare (double sampleRate, int samplesPerBlock);
        void reset() noexcept;

        /**
         * Updates the MAX_LOW_FREQ and MAX_HIGH_FREQ crossover targets and their
         * cached coefficients. Safe to retarget from the audio thread; process()
         * applies the change gradually without allocating.
         */
        void setCrossoverFrequencies (float maxLowFrequencyHz, float maxHighFrequencyHz) noexcept;

        /** Stores the LoContour target consumed by process(). */
        void setLoContour (float newLoContour) noexcept;

        /** Stores the high-band process amount consumed by process(). */
        void setProcessAmount (float newProcessAmount) noexcept;

        /** Selects the phase topology using a click-safe runtime transition. */
        void setPhaseMode (PhaseMode newPhaseMode) noexcept;

        /**
         * Processes a prepared mono or stereo block in place.
         *
         * The implementation is a real-time audio path: no new/delete/resize, no
         * locks, and no juce::String work. Coefficient and parameter-derived
         * constant updates belong in prepare() or the explicit parameter-change
         * helpers above, never in the per-sample path. Use juce::ScopedNoDenormals
         * in this method or at its processBlock() call site.
         */
        void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    private:
        using IIRFilter = juce::dsp::IIR::Filter<float>;
        using SmoothedParameter = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

        static constexpr std::size_t maxChannels = 2;
        static constexpr std::size_t splitFilterStages = 2;
        static constexpr std::size_t lowApfStageCount = 4;
        static constexpr std::size_t midApfStageCount = 2;
        static constexpr std::size_t highApfStageCount = 0;
        static constexpr std::size_t modernLowApfStageCount = 1;
        static constexpr std::size_t modernMidApfStageCount = 0;

        using ChannelFilterBank = std::array<IIRFilter, maxChannels>;

        void updateCrossoverCoefficients (float lowFrequencyHz, float highFrequencyHz) noexcept;

        // Three-band split: low < MAX_LOW_FREQ, MAX_LOW_FREQ <= mid <=
        // MAX_HIGH_FREQ, and high > MAX_HIGH_FREQ.
        std::array<ChannelFilterBank, splitFilterStages> lowBandLowPassFilters;
        std::array<ChannelFilterBank, splitFilterStages> midBandHighPassFilters;
        std::array<ChannelFilterBank, splitFilterStages> midBandLowPassFilters;
        std::array<ChannelFilterBank, splitFilterStages> highBandHighPassFilters;

        // Classic: four cascaded stages on low, two on mid, none on high.
        std::array<ChannelFilterBank, lowApfStageCount> lowBandAllPassFilters;
        std::array<ChannelFilterBank, midApfStageCount> midBandAllPassFilters;
        std::array<ChannelFilterBank, highApfStageCount> highBandAllPassFilters;

        // Modern: one all-pass stage on low, none on mid or high.
        std::array<ChannelFilterBank, modernLowApfStageCount> modernLowBandAllPassFilters;
        std::array<ChannelFilterBank, modernMidApfStageCount> modernMidBandAllPassFilters;

        // Per-channel envelope follower state e and its prepared decay constant alpha.
        std::array<float, maxChannels> envelopeE {};
        float envelopeAlpha = 0.0f;

        // Cached APF coefficient a and parameter targets; never derived per sample.
        float apfCoefficientA = 0.0f;
        float loContourTarget = 0.0f;
        float processAmountTarget = 0.0f;
        float maxLowFrequencyTargetHz = 0.0f;
        float maxHighFrequencyTargetHz = 0.0f;
        float appliedLowFrequencyHz = 0.0f;
        float appliedHighFrequencyHz = 0.0f;

        SmoothedParameter lowFrequencyHz { 0.0f };
        SmoothedParameter highFrequencyHz { 0.0f };

        SmoothedParameter phaseModeBlend { 0.0f };
        PhaseMode phaseModeTarget = PhaseMode::classic;

        double preparedSampleRate = 0.0;
        int preparedSamplesPerBlock = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonicMaximizer)
    };
}
