# b

## Goal

Implement Module A, the Sonic Maximizer (phase alignment and dynamic low contour). Header first, then implementation. Pre-allocate all state in prepareToPlay(sampleRate, samplesPerBlock) with zero heap allocation, locks, or juce::String work in process(): 3-band split (low < MAX_LOW_FREQ, mid, high > MAX_HIGH_FREQ) using juce::dsp::IIR::Filter cascades; cascaded 1st/2nd-order allpass filters giving frequency-dependent group delay (4x APF on the low band, 2x on mid, 0x on high) with coefficient a = (tan(pi*fc/fs) - 1)/(tan(pi*fc/fs) + 1); an envelope follower e[n] = (1-alpha)*|x_low[n]| + alpha*e[n-1] with alpha = exp(-1/(0.05*sampleRate)); and dynamic boost G_low[n] = 1 + LoContour*(e[n]/(1+e[n])). Split into header-declares-state and cpp-implements-math tasks.

## Notes

- Async: Wave 1: b.1 writes Source/DSP/SonicMaximizer.h only (band-filter members, APF cascade arrays, envelope state, prepare/process signatures). Wave 2: b.2 writes Source/DSP/SonicMaximizer.cpp only and depends on b.1. Final wave: b.3 writes docs/verification/phase_b_sonic_maximizer.md and depends on b.2. No same-file overlap.
- Risk: Verification: cmake/Ninja Standalone build plus 'cppcheck --enable=all Source/DSP' and clang-format on the two files; not make. Real-time safety is the primary risk: any new/delete/resize or juce::String in process() must be rejected. APF coefficient and envelope constants must be recomputed only in prepareToPlay or on parameter change, never per-sample from raw parameter reads. Header and cpp are serialized by dependency.
