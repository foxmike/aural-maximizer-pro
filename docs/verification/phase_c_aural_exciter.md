# Phase C — Aural Exciter (Module B) Verification

> Phase: C
> Scope: Module B — Aural Exciter DSP
> Record type: verification note (deterministic, reproducible from a clean clone)

## 1. Purpose

Record that Phase C Module B (`nSpace::AuralExciter`) was implemented and that
it satisfies the Phase C gate checks: a Ninja Standalone build, a static
analysis pass, and a clang-format style pass against the delivered DSP sources.

This note introduces no code. It inventories the delivered files, describes the
implemented behaviour against the Phase C goal, records the exact verification
commands, and tabulates pass/fail results.

## 2. Phase goal summary

Module B is a high-frequency harmonic exciter feeding a post-saturation "Air"
shelf. The Phase C goal for this module is:

- **Sidechain high-pass driven by `EXC_TUNE`** — a second-order
  `juce::dsp::StateVariableTPTFilter` high-pass, swept per sample from the
  smoothed `tuneHz` parameter, clamped to the documented range
  `minimumTuneHz = 800 Hz` … `maximumTuneHz = 10 kHz`.
- **Three wave-shapers** selectable via `WaveShaperMode`:
  - `cubicSoftClipping` — piecewise cubic soft clip (static helper
    `cubicSoftClip`), the default mode.
  - `asymmetricTube` — `tanh(drive·x + delta) + gamma·(drive·x)²`, combining
    an asymmetric tube-like bend with a quadratic harmonic term.
  - `tape` — `(2/π)·atan(drive·x)` arc-tangent tape-style saturation.
- **2x / 4x oversampling** of the non-linear section via
  `juce::dsp::Oversampling<float>` using FIR half-band equiripple filters
  (`filterHalfBandFIREquiripple`, `isOversamplingFIR = true`,
  `isMaxQuality = true`). The factor is selected by
  `OversamplingFactor::twoTimes` (1 stage) or `fourTimes` (2 stages).
- **Air EQ shelf** — a post-saturation high-shelf IIR (`AirShelfFilter`,
  `makeHighShelf`) at `airShelfFrequencyHz = 10 kHz` (clamped to
  `0.499·sampleRate`), driven by `EXC_AIR_GAIN` in dB.
- **Real-time (RT) safety constraints**:
  - The oversampler is constructed and its latency cached **only** in
    `prepare()`; `process()` never builds, rebuilds, or queries oversampler
    latency. `setOversamplingFactor()` only records a request that takes effect
    at the next `prepare()`.
  - Smoothed parameters (`tuneHz`, `driveGainG`, `deltaBias`, `gamma`,
    `airGainDb`) are `juce::SmoothedValue` (linear) with a 20 ms time constant;
    parameter setters write target values only, and `process()` advances them
    per sample (`getNextValue()`).
  - No allocations, locks, or `std::mutex` use on the audio thread: scratch
    storage (`sidechainBuffer`) is sized once in `prepare()` and reused;
    `process()` operates in place via `ProcessContextReplacing`, uses only
    stack locals, and guards denormals with `juce::ScopedNoDenormals`.

## 3. Files delivered

| Path | Role |
|------|------|
| `Source/DSP/AuralExciter.h` | `nSpace::AuralExciter` class declaration: enums (`WaveShaperMode`, `OversamplingFactor`), public surface (`prepare`/`reset`/`process` + control-rate setters + `getLatencySamples`), private members (sidechain HP, oversampler, Air shelf, scratch buffer, smoothed params). |
| `Source/DSP/AuralExciter.cpp` | Implementation: cubic soft-clip helper, `prepare` (allocates scratch, builds oversampler, caches latency, resets smoothers), `reset`, `process` (sidechain HP → oversample up → per-sample shaper → oversample down → Air shelf → copy out), and the control-rate setters clamping/forwarding parameters. |

Both files were previously Phase A skeletons; Phase C replaces the no-op stubs
with the DSP described in §2 and §4.

## 4. Implemented behaviour (symbols verified by inspection)

Symbols/behaviour documented from the source, for traceability:

- Constants (header):
  - `minimumTuneHz = 800.0f`, `maximumTuneHz = 10000.0f`
  - `airShelfFrequencyHz = 10000.0f`
  - `defaultDeltaBias = 0.15f`
- Enums (header):
  - `WaveShaperMode { cubicSoftClipping, asymmetricTube, tape }`
  - `OversamplingFactor : std::size_t { twoTimes = 2, fourTimes = 4 }`
- Signal path (`.cpp`):
  - `highPassSidechain` set to `StateVariableTPTFilterType::highpass`,
    resonance `shelfQ = 0.70710678f`; cutoff advanced per sample from
    `tuneHz.getNextValue()`.
  - `oversampler` constructed in `prepare()` only;
    `requestedOversamplingFactor` selects 1 stage (2x) or 2 stages (4x);
    `preparedLatencySamples = oversampler->getLatencyInSamples()`.
  - Shaper loop applies `drive = driveGainG.getNextValue()`,
    `delta = deltaBias.getNextValue()`, `gamma = gamma.getNextValue()` per
    oversampled sample; mode `switch` selects `cubicSoftClip`, the
    `tanh(...) + harmonicAmount·x²` tube path, or the `atan` tape path.
  - `airGainDb.skip(sampleCount)` then `updateAirShelf()` recomputes the
    high-shelf coefficients; `airShelf.process(...)` applies the shelf.
- Control-rate setters: `setTuneHz` clamps to `[minimumTuneHz, maximumTuneHz]`;
  `setDriveGain` floors at 0; `setDeltaBias`, `setHarmonics`, `setAirGainDb`
  forward target values; `setWaveShaperMode` and `setOversamplingFactor` store
  the selection (latter takes effect only at next `prepare()`).
- `getLatencySamples()` returns the cached `preparedLatencySamples` without
  touching the oversampler (RT-safe query).

## 5. Verification commands (exact)

These are the exact commands used. The Ninja generator is required; **make is
not used**. Run from the repository root.

### 5.1 Configure

```sh
cmake -B cmake-build-debug -G Ninja
```

- Configures the project into `cmake-build-debug/` using the Ninja generator.
- Must exit 0 and emit `AuralMaximizerPro_Standalone` in the build graph.

### 5.2 Build the phase gate target

```sh
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
```

- Builds only the `AuralMaximizerPro_Standalone` target (the phase gate).
- Must exit 0, proving `AuralExciter.h` / `.cpp` compile and link against JUCE
  alongside the rest of the project.

### 5.3 Static analysis on the DSP tree

```sh
cppcheck --enable=all Source/DSP
```

- Runs cppcheck with all checks enabled over the `Source/DSP` directory
  (including `AuralExciter.h` and `AuralExciter.cpp`).
- Must exit with no errors (warnings reported as findings; absence of error
  exit is the pass criterion).

### 5.4 Style gate on the delivered sources

```sh
clang-format --dry-run --Werror Source/DSP/AuralExciter.h
clang-format --dry-run --Werror Source/DSP/AuralExciter.cpp
```

- Runs `clang-format --dry-run --Werror` against each delivered file.
- Must exit 0 (no formatting violations) for each file.

## 6. Real-time safety constraints (explicit notes)

These constraints are enforced by code structure and are part of the Phase C
acceptance for Module B:

- **Oversampler construction / latency query**: performed exclusively in
  `prepare()`. `std::make_unique<juce::dsp::Oversampling<float>>(...)`,
  `oversampler->initProcessing(...)`, and
  `oversampler->getLatencyInSamples()` all occur inside `prepare()`; the
  result is stored in `preparedLatencySamples`. `process()` only calls
  `processSamplesUp`, the per-sample shaper loop, and `processSamplesDown` —
  no construction, no re-init, no latency query. Changing the oversampling
  factor via `setOversamplingFactor()` does not rebuild anything until the
  next `prepare()`.
- **Parameter smoothing**: all user-facing controls are wrapped in
  `juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>` (alias
  `SmoothedParameter`) with `smoothingTimeSeconds = 0.02`. Setters write
  target values only; `process()` advances each smoother via `getNextValue()`
  (and `airGainDb.skip(...)` for the shelf). No parameter jumps reach the
  audio thread.
- **No locks / no allocations on the audio thread**: `process()` takes no
  locks, performs no heap allocation, and reuses the `sidechainBuffer` sized
  in `prepare()`. A `juce::ScopedNoDenormals` guards the entry. Bypass and
  unprepared states return early. The class is
  `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` and non-copyable.

## 7. Pass / fail table

| # | Check | Command | Pass criterion | Result |
|---|-------|---------|---------------|--------|
| 1 | Configure with Ninja | `cmake -B cmake-build-debug -G Ninja` | exit 0; `AuralMaximizerPro_Standalone` target present | PASS |
| 2 | Build phase gate target | `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone` | exit 0; standalone artefact produced | PASS |
| 3 | Static analysis (DSP tree) | `cppcheck --enable=all Source/DSP` | no errors (exit 0 / no error-severity findings) | PASS |
| 4 | clang-format — header | `clang-format --dry-run --Werror Source/DSP/AuralExciter.h` | exit 0 | PASS |
| 5 | clang-format — implementation | `clang-format --dry-run --Werror Source/DSP/AuralExciter.cpp` | exit 0 | PASS |
| 6 | RT safety — oversampler confined to `prepare()` | source inspection of `AuralExciter.cpp` | oversampler built + latency cached in `prepare()` only; `process()` uses cached results | PASS |
| 7 | RT safety — parameter smoothing | source inspection | all controls are `SmoothedValue`; setters write targets; `process()` advances per sample | PASS |
| 8 | RT safety — no locks/allocations on audio thread | source inspection of `process()` | no heap allocation, no mutex, scratch buffer reused | PASS |

## 8. Phase gate definition (Module B)

Phase C Module B is satisfied when, from a clean clone:

1. `cmake -B cmake-build-debug -G Ninja` exits 0, and
2. `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
   exits 0 (compiles/links `AuralExciter.h` and `AuralExciter.cpp`), and
3. `cppcheck --enable=all Source/DSP` reports no errors, and
4. `clang-format --dry-run --Werror` on
   `Source/DSP/AuralExciter.h` and `Source/DSP/AuralExciter.cpp` exits 0, and
5. The RT-safety constraints in §6 hold by source inspection (oversampler
   constructed/latency queried in `prepare()` only; smoothed G/delta/gamma;
   no locks/allocations on the audio thread).

## 9. Prerequisites and reproducibility notes

- `cmake`, `ninja`, `cppcheck`, and `clang-format` must be on `PATH`.
- The `JUCE` git submodule must be initialised
  (`git submodule update --init --recursive`).
- `cmake-build-debug/` is gitignored and disposable; if it was previously
  configured with a different generator, remove it before the `-G Ninja`
  configure step. The Phase C procedure uses Ninja exclusively and never
  invokes `make`.
- No source code was modified to produce this note. The verification here
  documents the delivered `Source/DSP/AuralExciter.{h,cpp}` only.

## 10. Status

- Files delivered (§3): `Source/DSP/AuralExciter.h`, `Source/DSP/AuralExciter.cpp`.
- Phase gate target: `AuralMaximizerPro_Standalone` (designated in §8).
- Verification commands: recorded in §5 (Ninja, no make; cppcheck; clang-format).
- Pass/fail: all checks PASS (§7).
