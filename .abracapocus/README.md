# AURAL_MAXIMIZER_PRO

## Purpose

Build the AuralMaximizerPro cross-platform C++20 / JUCE audio plugin (Standalone, VST3, CLAP, LV2) as a sequence of small, real-time-safe, independently verifiable tasks. Each DSP and GUI module is authored header-first then implementation, wired into the APVTS processor and split-screen editor, and gated by an incremental Standalone build. Because Abracapocus ships no C++ adapter (only Python/TypeScript/Go/C) and the C adapter covers .c/.h but not .cpp and defaults verification to make, this plan does not rely on adapter auto-detection: every task uses exact per-file allowed_write_paths (never globs) and every phase specifies its verification explicitly as a CMake/Ninja build plus cppcheck and clang-format, not make. Real-time-thread safety (no heap allocation, no locks, ScopedNoDenormals, smoothed parameters, lock-free SPSC audio-to-UI transfer) is a hard constraint on every audio-path task.
