# c

## Goal

Implement Module B, the Aural Exciter (sidechain saturation and harmonics). Header first, then implementation, real-time safe with all buffers and the oversampler pre-allocated in prepareToPlay. Requirements: a 2nd-order high-pass sidechain via juce::dsp::StateVariableFilter with cutoff EXC_TUNE in [800 Hz, 10 kHz]; three selectable wave-shapers on the high-passed signal (Cubic Soft Clipping with the piecewise 2/3 and 4/3 thresholds; Asymmetric Tube y = tanh(G*x + delta) + gamma*(G*x)^2 with delta approx 0.15 for even harmonics and gamma from EXC_HARMONICS; Tape y = (2/pi)*arctan(G*x)); the non-linear stage wrapped in juce::dsp::Oversampling<float> with selectable 2x/4x polyphase FIR; and a post-saturation high-shelf Air EQ above 10 kHz driven by EXC_AIR_GAIN. Split header vs implementation.

## Notes

- Async: Wave 1: c.1 writes Source/DSP/AuralExciter.h only (SVF member, Oversampling member, wave-shaper mode, air-EQ high-shelf, prepare/process signatures). Wave 2: c.2 writes Source/DSP/AuralExciter.cpp only and depends on c.1. Final wave: c.3 writes docs/verification/phase_c_aural_exciter.md and depends on c.2. No same-file overlap.
- Risk: Verification: cmake/Ninja Standalone build plus cppcheck --enable=all on Source/DSP and clang-format on the two files; not make. Oversampling must be constructed and its latency queried in prepareToPlay, never in process(). Drive gain G, delta bias, and gamma must be smoothed off the per-sample raw parameter path to avoid zipper noise. The exciter must not introduce locks or allocations.
