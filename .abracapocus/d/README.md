# d

## Goal

Implement Module C, the Routing / Matrix engine. Header first, then implementation. Pre-allocate all scratch buffers in prepareToPlay (no allocation in process()). Support three routing modes selected by ROUTING_MODE: Serial Maximizer -> Exciter, Serial Exciter -> Maximizer, and Parallel Matrix Blend y = (1-K)*S_M(x) + K*S_E(x) where K is PARALLEL_BLEND smoothed with juce::SmoothedValue<float> to avoid clicks on mode/blend changes. The engine owns the two modules' invocation order and the crossfade summing; it must not itself allocate or lock.

## Notes

- Async: Wave 1: d.1 writes Source/DSP/RoutingEngine.h only (mode enum, pre-allocated scratch buffers, SmoothedValue crossfader, prepare/process signatures). Wave 2: d.2 writes Source/DSP/RoutingEngine.cpp only and depends on d.1. Final wave: d.3 writes docs/verification/phase_d_routing_engine.md and depends on d.2. No same-file overlap.
- Risk: Verification: cmake/Ninja Standalone build plus cppcheck and clang-format on the two files; not make. Scratch buffers for the parallel path must be sized to samplesPerBlock and channel count in prepareToPlay. Switching routing mode at runtime must not reallocate; the crossfader smoothing prevents summing-bus clicks. Depends conceptually on phases b and c existing (guaranteed by phase order).
