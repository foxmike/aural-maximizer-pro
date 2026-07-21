# Phase C — Aural Exciter Verification

Phase: C
Task: c — Record phase C Aural Exciter verification
Date: 2026-07-21

## Purpose

This note records the deterministic verification evidence for the Phase C
Module B (Aural Exciter) work in `AuralMaximizerPro`. Phase C Module B
implements the Aural Exciter DSP block. The verification gate for this phase is
the **Standalone** build target plus static analysis and formatting checks on
the `Source/DSP` directory, with `clang-format --dry-run --Werror` applied
specifically to the Aural Exciter sources.

The build generator is **Ninja**. `make` is intentionally not used and is not
documented as the primary build command.

## Phase goal summary

Phase C Module B delivers an Aural Exciter that adds controlled harmonic
excitement to the high-frequency content of the input. The design goals for
this module are:

- **Sidechain high-pass filter** with a tunable cutoff controlled by the
  `EXC_TUNE` parameter, swept across **800 Hz–10 kHz**. The HPF isolates the
  high-frequency region that is fed to the harmonic-generation path.
- **Three wave-shapers** applied to the sidechain high-passed signal to
  generate second/third-order harmonic content. The shapers are the harmonic
  generation core of the exciter.
- **2x / 4x oversampling** of the wave-shaper stage to suppress aliasing of the
  generated harmonics. The oversampler is constructed in `prepare()` only; no
  oversampler is (re)allocated on the audio thread.
- **Air EQ shelf** applied to the exciter return path to blend the excited
  signal back into the dry path with a high shelf character ("Air").
- **Real-time (RT) safety constraints:**
  - No `malloc`/`new`/`free` or STL container growth on the audio thread.
  - No locks (mutexes, spinlocks) taken in `process()`.
  - The oversampler is constructed and its latency queried in `prepare()` only;
    the audio thread only uses the already-constructed instance.
  - Smoothed parameters (`G`, `delta`, `gamma`) use JUCE `SmoothedValue`
    ramping so that parameter changes are de-zippered without per-block
    allocation or locking.

## Scope inspected

Before authoring this record, the following paths were inspected against the
working tree.

- `Source/DSP/AuralExciter.h` — present. Declares the `AuralExciter` class
  with `prepare(double sampleRate, int samplesPerBlock)`,
  `process(juce::AudioBuffer<float>& buffer)`, and `reset()` public methods,
  plus a private `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` declaration.
  No heap-allocating members are exposed in the header.
- `Source/DSP/AuralExciter.cpp` — present. Defines the constructor,
  destructor, and the three public methods. The Phase C Module B implementation
  lives here.

### Real-time constraints

The `process(juce::AudioBuffer<float>&)` method runs on the audio thread and
must be real-time (RT) safe. The following RT-safety constraints apply to the
Aural Exciter implementation and were considered when authoring this record:

- No `malloc`/`new`/`free` or STL container growth on the audio thread.
- No locks (mutexes, spinlocks) taken in `process()`.
- No `std::atomic` operations beyond the lock-free, wait-free reads/writes
  used for coefficient/parameter hand-off.

### Oversampling

Oversampling (2x/4x) is the primary aliasing mitigation for the wave-shaper
stage. The relevant constraint is:

- The `juce::dsp::Oversampling` (or equivalent) instance is constructed and its
  latency queried **in `prepare()` only**, on the prepare/message thread. The
  queried latency is stored so that the processing path can compensate delay
  without recomputing it per block. The audio thread must only use the
  already-constructed oversampler and must not (re)allocate it or query its
  latency during `process()`.

### Parameter smoothing

The exciter exposes smoothed parameters (`G` — exciter drive/gain, `delta` —
harmonic blend amount, `gamma` — Air EQ / shelf character). Smoothing rules:

- `G`, `delta`, and `gamma` are wrapped in JUCE `SmoothedValue` instances.
- `prepare()` resets the smoothed values and configures the ramp length on the
  prepare/message thread.
- Parameter change handlers update the target values off-thread; the audio
  thread only reads the currently published targets and advances the smoother
  per block. No allocation or locking occurs in the smoothing path.

Any live coefficient or parameter mutation must occur off the audio thread; the
audio thread must only read the currently published values. This is the
primary residual RT-safety risk for Phase C Module B and is called out below.

## Files delivered

The Phase C Module B (Aural Exciter) deliverables are:

- `Source/DSP/AuralExciter.h`
- `Source/DSP/AuralExciter.cpp`

No other source or build files are in scope for this task.

## Verification commands

These are the exact commands recorded for the Phase C Aural Exciter gate.
`make` is deliberately excluded.

### 1. Configure (Ninja generator)

```bash
cmake -B cmake-build-debug -G Ninja
```

Configures the project into the `cmake-build-debug` build directory using the
Ninja generator. This step validates `CMakeLists.txt`, the JUCE setup, the
plugin target declaration, and the `target_sources` list (including the Aural
Exciter sources) against the on-disk files.

### 2. Build the Standalone target (phase gate)

```bash
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
```

Builds only the `AuralMaximizerPro_Standalone` target. A successful build of
this target is the Phase C phase gate. The VST3/CLAP/LV2 formats are
configured but are intentionally not part of the Phase C gate.

### 3. cppcheck static analysis on Source/DSP

```bash
cppcheck --enable=all Source/DSP
```

Runs `cppcheck` with `--enable=all` across the entire `Source/DSP` directory,
covering `SonicMaximizer.h`/`.cpp`, `AuralExciter.h`/`.cpp`, and
`RoutingEngine.h`/`.cpp`. `--enable=all` enables style, performance,
portability, and unused-function checks in addition to the default error
checks. A clean run (no errors/warnings) is the expected pass outcome.

### 4. clang-format dry-run on Aural Exciter sources

```bash
clang-format --dry-run --Werror \
  Source/DSP/AuralExciter.h \
  Source/DSP/AuralExciter.cpp
```

Runs `clang-format` in dry-run mode with `--Werror` on the Aural Exciter
header and implementation only. Any formatting deviation fails the check. No
files are rewritten by this command.

## Pass / fail table

| # | Check                                                                                       | Tool         | Pass condition                                          | Outcome                                                            |
|---|---------------------------------------------------------------------------------------------|--------------|---------------------------------------------------------|--------------------------------------------------------------------|
| 1 | Configure with Ninja generator                                                             | `cmake`      | `cmake -B cmake-build-debug -G Ninja` exits 0           | Not run in this environment (`cmake` not available, exit 127).     |
| 2 | Build `AuralMaximizerPro_Standalone` target                                                 | `cmake`/`ninja` | `cmake --build ... --target AuralMaximizerPro_Standalone` exits 0, produces artifact | Not run in this environment (toolchain not available). |
| 3 | Static analysis on `Source/DSP`                                                             | `cppcheck`   | `cppcheck --enable=all Source/DSP` exits 0, no errors/warnings | Not run in this environment (`cppcheck` not available).     |
| 4 | Formatting on `Source/DSP/AuralExciter.h` and `Source/DSP/AuralExciter.cpp`                | `clang-format` | `clang-format --dry-run --Werror ...` exits 0           | Not run in this environment (`clang-format` not available).         |

The verification commands above are the recorded Phase C Module B gate.
Execution was attempted in this verification environment, but `cmake`,
`ninja`, `cppcheck`, and `clang-format` were **not available** (commands not
found, exit 127 / not installed). The CMake/Ninja build gate
(`AuralMaximizerPro_Standalone`) and the `cppcheck`/`clang-format` static gates
could therefore not be executed from this environment at author time. The
commands are recorded verbatim above as the deterministic, reproducible Phase C
gate: they are runnable from a clean clone on any machine with the standard
C++ toolchain (CMake ≥ 3.x, Ninja, cppcheck, clang-format) installed. The
source files under test (`Source/DSP/AuralExciter.h`,
`Source/DSP/AuralExciter.cpp`) are present on disk and were inspected; the
declarations and definitions match.

This record does not substitute model self-evaluation for tool output. Where
the toolchain is available, the pass condition is:

- `cmake -B cmake-build-debug -G Ninja` exits 0.
- `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
  exits 0 and produces the `AuralMaximizerPro_Standalone` artifact.
- `cppcheck --enable=all Source/DSP` exits 0 with no errors or warnings.
- `clang-format --dry-run --Werror` on the two Aural Exciter files exits 0.

## Residual risks

- **RT safety of `process()`:** the audio-thread processing path (sidechain HPF,
  wave-shapers, oversampling up/down, Air EQ shelf, wet/dry blend) must remain
  allocation-free and lock-free. Any future change that introduces STL growth,
  mutex acquisition, blocking I/O, or oversampler (re)construction inside
  `AuralExciter::process()` is a regression of the Phase C gate. This is not
  statically enforced by cppcheck alone; it must be re-verified on any edit to
  the processing path.
- **Oversampler lifecycle:** the oversampler must be constructed and its
  latency queried in `prepare()` only. (Re)allocating the oversampler or
  querying its latency during `process()` would violate RT safety. Latency
  compensation must use the value stored at prepare time.
- **Parameter smoothing:** `G`, `delta`, and `gamma` must be advanced via
  `SmoothedValue` on the audio thread reading targets published off-thread.
  Direct non-smoothed writes to the processing path would cause zipper noise
  and must not be introduced.
- **Toolchain availability:** the Phase C gate depends on CMake, Ninja,
  cppcheck, and clang-format being installed. In the environment used to
  author this record they were absent; the gate must be re-run in an
  environment with the toolchain present before Phase C Module B can be closed
  as fully passing.

## Phase changed files

The files changed as part of Phase C Module B (Aural Exciter) are:

- `Source/DSP/AuralExciter.h`
- `Source/DSP/AuralExciter.cpp`
- `docs/verification/phase_c_aural_exciter.md` (this record)

No source or build files outside this list were created or modified by this
task.

## Phase gate summary

- Phase gate target: `AuralMaximizerPro_Standalone`
- Generator: Ninja (`-G Ninja`)
- Build directory: `cmake-build-debug`
- Primary build command: `cmake --build` (not `make`)
- Static analysis gate: `cppcheck --enable=all Source/DSP`
- Formatting gate: `clang-format --dry-run --Werror` on
  `Source/DSP/AuralExciter.h` and `Source/DSP/AuralExciter.cpp`
- RT-safety constraints: oversampler constructed/latency queried in `prepare()`
  only; `G`/`delta`/`gamma` smoothed via `SmoothedValue`; no locks or
  allocations on the audio thread.
- Out of scope for Phase C Module B: VST3/CLAP/LV2 build gate, Sonic Maximizer
  and Routing Engine DSP behavior, GUI rendering, and parameter wiring beyond
  the Aural Exciter update sites (deferred to later phases).
