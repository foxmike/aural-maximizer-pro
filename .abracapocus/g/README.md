# g

## Goal

Implement the two independent GUI display components: the FFT SpectrumVisualizer and the L/R MeterComponent. These live in separate files, so their headers run in parallel and their implementations run in parallel. SpectrumVisualizer uses juce::dsp::FFT with a lock-free juce::AbstractFifo ring buffer consumed on a juce::Timer at 30 Hz on the message thread; it must never touch the audio thread directly and must only read frames the processor pushed into the SPSC FIFO. MeterComponent renders real-time L/R peak/RMS LED bars, also updated on a capped Timer. Neither component allocates in any audio-thread callback.

## Notes

- Async: Wave 1 (parallel, disjoint): g.1 writes Source/GUI/SpectrumVisualizer.h only; g.3 writes Source/GUI/MeterComponent.h only. Wave 2 (parallel, disjoint): g.2 writes Source/GUI/SpectrumVisualizer.cpp only and depends on g.1; g.4 writes Source/GUI/MeterComponent.cpp only and depends on g.3. Final wave: g.5 writes docs/verification/phase_g_gui_components.md and depends on g.2 and g.4. The two components never share a write path.
- Risk: Verification: cmake/Ninja Standalone build plus cppcheck and clang-format on the four files; not make. The Timer must be capped at 30-60 Hz and must pull from the FIFO, never call repaint() from the audio thread. The FFT size and the SPSC array size must match the buffer the processor declared in phase e. Keep g.1/g.3 and g.2/g.4 disjoint to preserve the parallel waves.
