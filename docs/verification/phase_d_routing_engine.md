# Phase D — Routing / Matrix Engine Verification

Phase: D
Task: d — Record phase D Routing / Matrix engine verification
Date: 2026-07-21

## Purpose

This note records the deterministic verification evidence for the Phase D
Module C (Routing / Matrix engine) work in `AuralMaximizerPro`. Phase D
Module C implements the routing and matrix-blending engine that connects
Module A (Sonic Maximizer) and Module B (Aural Exciter). The verification
gate for this phase is the **Standalone** build target plus static analysis
and formatting checks applied specifically to the Routing Engine sources.

The build generator is **Ninja**. `make` is intentionally not used and is not
documented as the primary build command.

## Phase goal summary

Phase D Module C delivers the routing and matrix engine that selects how the
Sonic Maximizer and Aural Exciter are connected. The design goals for this
module are:

- **Serial Max -> Exc:** the Sonic Maximizer feeds the Aural Exciter
  (`RoutingMode::SerialMaximizerThenExciter`). Signal flows
  `S_M(x) -> S_E(S_M(x))`.
- **Serial Exc -> Max:** the Aural Exciter feeds the Sonic Maximizer
  (`RoutingMode::SerialExciterThenMaximizer`). Signal flows
  `S_E(x) -> S_M(S_E(x))`.
- **Parallel matrix blend:** the two modules run in parallel and their
  outputs are crossfaded by a blend factor `K` (`RoutingMode::ParallelMatrixBlend`).
  The blend is the convex combination
  `y = (1 - K) * S_M(x) + K * S_E(x)`, with `K` in `[0.0, 1.0]`. `K = 0` is
  the pure Maximizer path, `K = 1` is the pure Exciter path, and intermediate
  values are the matrix blend of the two.
- **Real-time (RT) safety constraints:**
  - No `malloc`/`new`/`free` or STL container growth on the audio thread.
  - No locks (mutexes, spinlocks) taken in `process()`.
  - Scratch buffers (`scratchBufferA`, `scratchBufferB`) are pre-allocated in
    `prepare()` based on `samplesPerBlock` and `numChannels`; the audio thread
    only reuses the already-sized buffers.
  - The `PARALLEL_BLEND` parameter (`K`) and any mode/crossfade transition are
    de-zippered with `juce::SmoothedValue<float>` so that mode switches and
    blend changes do not click.

## Scope inspected

Before authoring this record, the following paths were inspected against the
working tree.

- `Source/DSP/RoutingEngine.h` — present. Declares the `RoutingEngine` class
  with `prepare(double sampleRate, int samplesPerBlock, int numChannels = 2)`,
  a legacy `prepare(double sampleRate, int samplesPerBlock)` overload, two
  `process(...)` variants (reference and pointer) plus a single-buffer
  backward-compatibility `process(...)` overload, `reset()`, and
  `setRampDuration(double)`. The `RoutingMode` enum lists
  `SerialMaximizerThenExciter`, `SerialExciterThenMaximizer`, and
  `ParallelMatrixBlend`. Private members include pre-allocated
  `juce::AudioBuffer<float> scratchBufferA` / `scratchBufferB`, a
  `juce::SmoothedValue<float> parallelBlend`, and cached
  `currentSampleRate` / `currentBlockSize` / `currentNumChannels`. A
  `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` declaration is present. No
  heap-allocating members are introduced on the audio thread.
- `Source/DSP/RoutingEngine.cpp` — present. Defines the constructor,
  destructor, the two `prepare()` overloads, the three `process()` overloads,
  `reset()`, and `setRampDuration()`. The Phase D Module C implementation
  lives here.

### Real-time constraints

The `process(...)` methods run on the audio thread and must be real-time (RT)
safe. The following RT-safety constraints apply to the Routing Engine
implementation and were considered when authoring this record:

- No `malloc`/`new`/`free` or STL container growth on the audio thread.
- No locks (mutexes, spinlocks) taken in `process()`.
- The scratch buffers (`scratchBufferA`, `scratchBufferB`) are sized in
  `prepare()` from `samplesPerBlock` and `numChannels`. The audio thread only
  reuses the already-sized buffers; it must not resize or reallocate them.
- Mode switches and `PARALLEL_BLEND` updates are de-zippered via
  `juce::SmoothedValue<float>` so that the matrix crossfade
  `y = (1 - K) * S_M(x) + K * S_E(x)` is click-free. Direct, non-smoothed
  writes to the blend coefficient during `process()` would cause zipper noise
  and must not be introduced.

### Phase ordering dependency

Phase D Module C depends conceptually on the prior phases via the phase
order: Phase B Module A (Sonic Maximizer) and Phase C Module B (Aural Exciter)
must exist before the Routing Engine can route signal between them. This is a
phase-order / interface dependency, not a source-level edit: the Phase D task
does not modify Module A or Module B sources. The Routing Engine references
`SonicMaximizer` and `AuralExciter` by name in its header (`forward
declaration`) and includes `SonicMaximizer.h` / `AuralExciter.h` in its
implementation, satisfying the dependency on phases B/C at the build level.

## Files in scope

The Phase D Module C (Routing / Matrix engine) deliverables are:

- `Source/DSP/RoutingEngine.h`
- `Source/DSP/RoutingEngine.cpp`

No other source or build files are in scope for this task.

## Verification commands

These are the exact commands recorded for the Phase D Routing / Matrix engine
gate. `make` is deliberately excluded.

### 1. Configure (Ninja generator)

```bash
cmake -B cmake-build-debug -G Ninja
```

Configures the project into the `cmake-build-debug` build directory using the
Ninja generator. This step validates `CMakeLists.txt`, the JUCE setup, the
plugin target declaration, and the `target_sources` list (including the
Routing Engine sources) against the on-disk files.

### 2. Build the Standalone target (phase gate)

```bash
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
```

Builds only the `AuralMaximizerPro_Standalone` target. A successful build of
this target is the Phase D phase gate. The VST3/CLAP/LV2 formats are
configured but are intentionally not part of the Phase D gate.

### 3. cppcheck static analysis on Routing Engine sources

```bash
cppcheck --enable=all Source/DSP/RoutingEngine.h Source/DSP/RoutingEngine.cpp
```

Runs `cppcheck` with `--enable=all` on the Routing Engine header and
implementation only. `--enable=all` enables style, performance, portability,
and unused-function checks in addition to the default error checks. A clean
run (no errors/warnings) is the expected pass outcome.

### 4. clang-format dry-run on Routing Engine sources

```bash
clang-format --dry-run --Werror \
  Source/DSP/RoutingEngine.h \
  Source/DSP/RoutingEngine.cpp
```

Runs `clang-format` in dry-run mode with `--Werror` on the Routing Engine
header and implementation only. Any formatting deviation fails the check. No
files are rewritten by this command.

## Pass / fail table

| # | Check                                                                                          | Tool            | Pass condition                                                                              | Outcome                                                            |
|---|------------------------------------------------------------------------------------------------|-----------------|---------------------------------------------------------------------------------------------|--------------------------------------------------------------------|
| 1 | Configure with Ninja generator                                                                 | `cmake`         | `cmake -B cmake-build-debug -G Ninja` exits 0                                               | Not run in this environment (`cmake` not available, exit 127).     |
| 2 | Build `AuralMaximizerPro_Standalone` target                                                    | `cmake`/`ninja` | `cmake --build ... --target AuralMaximizerPro_Standalone` exits 0, produces artifact        | Not run in this environment (toolchain not available).             |
| 3 | Static analysis on `Source/DSP/RoutingEngine.h` and `Source/DSP/RoutingEngine.cpp`            | `cppcheck`      | `cppcheck --enable=all Source/DSP/RoutingEngine.h Source/DSP/RoutingEngine.cpp` exits 0, no errors/warnings | Not run in this environment (`cppcheck` not available). |
| 4 | Formatting on `Source/DSP/RoutingEngine.h` and `Source/DSP/RoutingEngine.cpp`                 | `clang-format`  | `clang-format --dry-run --Werror ...` exits 0                                               | Not run in this environment (`clang-format` not available).        |

The verification commands above are the recorded Phase D Module C gate.
Execution was attempted in this verification environment, but `cmake`,
`ninja`, `cppcheck`, and `clang-format` were **not available** (commands not
found, exit 127 / not installed). The CMake/Ninja build gate
(`AuralMaximizerPro_Standalone`) and the `cppcheck`/`clang-format` static gates
could therefore not be executed from this environment at author time. The
commands are recorded verbatim above as the deterministic, reproducible Phase D
gate: they are runnable from a clean clone on any machine with the standard
C++ toolchain (CMake ≥ 3.x, Ninja, cppcheck, clang-format) installed. The
source files under test (`Source/DSP/RoutingEngine.h`,
`Source/DSP/RoutingEngine.cpp`) are present on disk and were inspected; the
declarations and definitions match.

This record does not substitute model self-evaluation for tool output. Where
the toolchain is available, the pass condition is:

- `cmake -B cmake-build-debug -G Ninja` exits 0.
- `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
  exits 0 and produces the `AuralMaximizerPro_Standalone` artifact.
- `cppcheck --enable=all Source/DSP/RoutingEngine.h Source/DSP/RoutingEngine.cpp`
  exits 0 with no errors or warnings.
- `clang-format --dry-run --Werror` on the two Routing Engine files exits 0.

## Build system note

`make` was **not used** for any Phase D verification step. The build is driven
by CMake with the **Ninja** generator (`-G Ninja`) and invoked through
`cmake --build`. No `make`, `make all`, or `make AuralMaximizerPro_Standalone`
commands were issued.

## Residual risks

- **RT safety of `process()`:** the audio-thread processing path (serial
  Max->Exc, serial Exc->Max, and the parallel matrix blend
  `y = (1 - K) * S_M(x) + K * S_E(x)`) must remain allocation-free and
  lock-free. Any future change that introduces STL growth, mutex acquisition,
  blocking I/O, or scratch-buffer resizing inside `RoutingEngine::process()`
  is a regression of the Phase D gate. This is not statically enforced by
  cppcheck alone; it must be re-verified on any edit to the processing path.
- **Scratch buffer lifecycle:** `scratchBufferA` and `scratchBufferB` must be
  sized in `prepare()` from `samplesPerBlock` and `numChannels`. Resizing or
  reallocating them during `process()` would violate RT safety.
- **Parameter smoothing:** `parallelBlend` (`K`) must be advanced via
  `SmoothedValue` on the audio thread reading targets published off-thread.
  Direct non-smoothed writes to the blend coefficient would cause zipper
  noise and must not be introduced.
- **Toolchain availability:** the Phase D gate depends on CMake, Ninja,
  cppcheck, and clang-format being installed. In the environment used to
  author this record they were absent; the gate must be re-run in an
  environment with the toolchain present before Phase D Module C can be closed
  as fully passing.

## Phase changed files

The files changed as part of Phase D Module C (Routing / Matrix engine) are:

- `docs/verification/phase_d_routing_engine.md` (this record)

No source or build files outside this list were created or modified by this
task. `Source/DSP/RoutingEngine.h` and `Source/DSP/RoutingEngine.cpp` are
referenced as the files under test but were not modified by this task.

## Phase gate summary

- Phase gate target: `AuralMaximizerPro_Standalone`
- Generator: Ninja (`-G Ninja`)
- Build directory: `cmake-build-debug`
- Primary build command: `cmake --build` (not `make`)
- Static analysis gate: `cppcheck --enable=all Source/DSP/RoutingEngine.h
  Source/DSP/RoutingEngine.cpp`
- Formatting gate: `clang-format --dry-run --Werror` on
  `Source/DSP/RoutingEngine.h` and `Source/DSP/RoutingEngine.cpp`
- RT-safety constraints: scratch buffers sized in `prepare()` only;
  `PARALLEL_BLEND` (`K`) smoothed via `SmoothedValue`; no locks or
  allocations on the audio thread.
- Routing modes covered: Serial Max->Exc, Serial Exc->Max, Parallel matrix
  blend `y = (1 - K) * S_M(x) + K * S_E(x)`.
- Phase dependency: depends conceptually on phases B/C (Module A Sonic
  Maximizer and Module B Aural Exciter) via phase order; this task does not
  modify those sources.
- Out of scope for Phase D Module C: VST3/CLAP/LV2 build gate, Sonic Maximizer
  and Aural Exciter DSP behavior, GUI rendering, and parameter wiring beyond
  the Routing Engine update sites (deferred to later phases).
