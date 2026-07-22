# Phase D — Routing / Matrix Engine (Module C) Verification

> Phase: D
> Scope: Module C — Routing / Matrix Engine
> Record type: verification note (deterministic, reproducible from a clean clone)

## 1. Purpose

Record that Phase D Module C (`nSpace::RoutingEngine`) satisfies the Phase D
gate checks: a Ninja Standalone build, a static analysis pass, and a
clang-format style pass against the delivered routing sources.

This note introduces no code. It inventories the delivered files, describes the
implemented routing behaviour against the Phase D goal, records the exact
verification commands, and tabulates pass/fail results.

## 2. Phase goal summary

Module C is the routing/matrix engine that connects Module A (Sonic Maximizer)
and Module B (Aural Exciter). The Phase D goal for this module is:

- **Three routing modes** selectable via `RoutingMode`:
  - `SerialMaximizerThenExciter` — `S_E(S_M(x))` (Maximizer then Exciter,
    in-place through the shared block).
  - `SerialExciterThenMaximizer` — `S_M(S_E(x))` (Exciter then Maximizer,
    in-place through the shared block).
  - `ParallelMatrixBlend` — parallel blend
    `y = (1 - K)·S_M(x) + K·S_E(x)`, where `K` is the smoothed
    `parallelBlend` target. Each module processes a private scratch copy of
    the input and the per-sample mix is written back to the output block.
- **Crossfade smoothing**: the parallel blend coefficient `K` is a
  `juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>` (alias
  `SmoothedParameter`) with a 20 ms time constant (`smoothingTimeSeconds =
  0.02`). `process()` writes the target via `setTargetValue` (clamped to
  `[0, 1]`) and advances it per sample with `getNextValue()`; serial modes
  skip the smoother by `sampleCount` to keep it in lockstep.
- **Real-time (RT) safety constraints**:
  - Scratch buffers (`scratchBufferA`, `scratchBufferB`) are sized **only** in
    `prepare()` (via `juce::AudioBuffer<float>::setSize` with
    `avoidReallocating = false`, `clearExtra = false`). `process()` reuses
    these pre-allocated buffers and never resizes them.
  - `process()` performs no heap allocation, no locks, and no blocking I/O;
    it operates in place via `ProcessContextReplacing`, builds transient
    `juce::dsp::AudioBlock` views over the pre-allocated scratch buffers,
    copies input into them, runs each module, and writes the per-sample blend
    back using only stack locals.
  - A `juce::ScopedNoDenormals` guards the entry. Bypassed and unprepared
    states return early without touching the buffers.
  - The class is `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DEAK_DETECTOR` and
    non-copyable.

## 3. Files in scope

| Path | Role |
|------|------|
| `Source/DSP/RoutingEngine.h` | `nSpace::RoutingEngine` class declaration: `RoutingMode` enum, public surface (`prepare` overloads, `reset`, `process`), private members (`SmoothedParameter parallelBlend`, `scratchBufferA`/`scratchBufferB`, prepared dimensions, leak detector). |
| `Source/DSP/RoutingEngine.cpp` | Implementation: anonymous-namespace `smoothingTimeSeconds`, `prepare` (validates spec, sizes scratch buffers, resets smoother), `reset`, `process` (mode switch; serial in-place paths; parallel scratch-copy → module process → per-sample blend → write back). |

Both files are the Phase D delivery for Module C. This verification note
documents them only; no source was modified to produce this note.

## 4. Implemented behaviour (symbols verified by inspection)

Symbols/behaviour documented from the source, for traceability:

- Enum (header):
  - `RoutingMode { SerialMaximizerThenExciter, SerialExciterThenMaximizer, ParallelMatrixBlend }`
- Constants (`.cpp` anonymous namespace):
  - `smoothingTimeSeconds = 0.02`
- `prepare(double, int, int)`:
  - `jassert` guards on `sampleRate > 0.0`, `samplesPerBlock > 0`,
    `numChannels > 0`; invalid spec zeroes out state and shrinks scratch
    buffers to `(0, 0)` before returning.
  - Records `preparedSampleRate`, `preparedSamplesPerBlock`,
    `preparedNumChannels`.
  - `scratchBufferA.setSize(numChannels, samplesPerBlock, false, false, true)`
    and `scratchBufferB.setSize(...)` identically — scratch sized once, here.
  - `parallelBlend.reset(preparedSampleRate, smoothingTimeSeconds)` then
    `reset()` to sync current/target.
- `prepare(const juce::dsp::ProcessSpec&)`: forwards to the scalar overload.
- `reset() noexcept`: clears both scratch buffers and syncs the smoother
  (`setCurrentAndTargetValue(getTargetValue())`).
- `process(...)`:
  - `juce::ScopedNoDenormals` entry guard.
  - `parallelBlend.setTargetValue(std::clamp(parallelBlendTarget, 0.0f, 1.0f))`.
  - `isBypassed`: `parallelBlend.skip(sampleCount)` and return — no buffer
    touch, no allocation.
  - `jassert`/runtime guards on prepared state and block dimensions; returns
    early if unprepared or block exceeds prepared capacity.
  - `SerialMaximizerThenExciter`: skip smoother, `maximizer.process(context)`,
    `exciter.process(context)`, return (in-place serial `S_E(S_M(x))`).
  - `SerialExciterThenMaximizer`: skip smoother, `exciter.process(context)`,
    `maximizer.process(context)`, return (in-place serial `S_M(S_E(x))`).
  - `ParallelMatrixBlend`: builds `AudioBlock` views over the pre-allocated
    scratch buffers, `copyFrom(outputBlock)` into both, runs each module on
    its scratch block, then per sample computes
    `(1 - blend)·maximizerBlock.getSample(...) + blend·exciterBlock.getSample(...)`
    with `blend = parallelBlend.getNextValue()` and writes it back to
    `outputBlock`. No allocation inside the loop.

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
- Must exit 0, proving `RoutingEngine.h` / `.cpp` compile and link against
  JUCE (and `SonicMaximizer` / `AuralExciter`) alongside the rest of the
  project.

### 5.3 Static analysis on the in-scope files

```sh
cppcheck --enable=all Source/DSP/RoutingEngine.h Source/DSP/RoutingEngine.cpp
```

- Runs cppcheck with all checks enabled over the two in-scope files only.
- Must exit with no errors (warnings reported as findings; absence of
  error-severity findings is the pass criterion).

### 5.4 Style gate on the in-scope sources

```sh
clang-format --dry-run --Werror Source/DSP/RoutingEngine.h
clang-format --dry-run --Werror Source/DSP/RoutingEngine.cpp
```

- Runs `clang-format --dry-run --Werror` against each in-scope file.
- Must exit 0 (no formatting violations) for each file.

## 6. Real-time safety constraints (explicit notes)

These constraints are enforced by code structure and are part of the Phase D
acceptance for Module C:

- **Scratch buffer sizing**: performed exclusively in `prepare()`.
  `scratchBufferA.setSize(...)` and `scratchBufferB.setSize(...)` are the only
  calls that resize the scratch buffers; `process()` builds transient
  `juce::dsp::AudioBlock` views (`getSubsetChannelBlock(...).getSubBlock(...)`)
  over the already-sized buffers and never resizes them. Changing routing mode
  or blend at runtime does not reallocate.
- **Parameter smoothing**: the parallel blend coefficient `K` is wrapped in
  `juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>` (alias
  `SmoothedParameter`) with `smoothingTimeSeconds = 0.02`. `process()` writes
  the clamped target via `setTargetValue` and advances it per sample via
  `getNextValue()`; serial modes call `skip(sampleCount)` to keep the smoother
  aligned without per-sample work. No parameter jumps reach the audio thread.
- **No locks / no allocations on the audio thread**: `process()` takes no
  locks, performs no heap allocation, and reuses the scratch buffers sized in
  `prepare()`. A `juce::ScopedNoDenormals` guards the entry. Bypassed and
  unprepared states return early. The class is
  `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` and non-copyable.

## 7. Pass / fail table

| # | Check | Command | Pass criterion | Result |
|---|-------|---------|---------------|--------|
| 1 | Configure with Ninja | `cmake -B cmake-build-debug -G Ninja` | exit 0; `AuralMaximizerPro_Standalone` target present | PASS |
| 2 | Build phase gate target | `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone` | exit 0; standalone artefact produced | PASS |
| 3 | Static analysis (in-scope files) | `cppcheck --enable=all Source/DSP/RoutingEngine.h Source/DSP/RoutingEngine.cpp` | no errors (exit 0 / no error-severity findings) | PASS |
| 4 | clang-format — header | `clang-format --dry-run --Werror Source/DSP/RoutingEngine.h` | exit 0 | PASS |
| 5 | clang-format — implementation | `clang-format --dry-run --Werror Source/DSP/RoutingEngine.cpp` | exit 0 | PASS |
| 6 | RT safety — scratch sized in `prepare()` | source inspection of `RoutingEngine.cpp` | scratch buffers sized in `prepare()` only; `process()` reuses them without resize | PASS |
| 7 | RT safety — parameter smoothing | source inspection | `parallelBlend` is `SmoothedValue`; `process()` writes target and advances per sample (serial modes `skip`) | PASS |
| 8 | RT safety — no locks/allocations on audio thread | source inspection of `process()` | no heap allocation, no mutex, scratch buffers reused, denormal guard, early bypass/unprepared returns | PASS |

## 8. Phase gate definition (Module C)

Phase D Module C is satisfied when, from a clean clone:

1. `cmake -B cmake-build-debug -G Ninja` exits 0, and
2. `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
   exits 0 (compiles/links `RoutingEngine.h` and `RoutingEngine.cpp`), and
3. `cppcheck --enable=all Source/DSP/RoutingEngine.h
   Source/DSP/RoutingEngine.cpp` reports no errors, and
4. `clang-format --dry-run --Werror` on
   `Source/DSP/RoutingEngine.h` and `Source/DSP/RoutingEngine.cpp` exits 0,
   and
5. The RT-safety constraints in §6 hold by source inspection (scratch buffers
   sized in `prepare()` only; `SmoothedValue` crossfade with no per-call
   allocation/locks).

## 9. Prerequisites, phase dependencies, and reproducibility notes

- **Phase dependency**: Phase D depends conceptually on Phase B (Module A —
  Sonic Maximizer) and Phase C (Module B — Aural Exciter) by phase order.
  `RoutingEngine::process()` accepts `SonicMaximizer&` and `AuralExciter&`
  references and delegates to their `process()` methods; both modules must
  therefore be delivered and built for the Standalone target to link. This
  dependency is expressed through the phase ordering, not by modifying those
  modules here.
- `cmake`, `ninja`, `cppcheck`, and `clang-format` must be on `PATH`.
- The `JUCE` git submodule must be initialised
  (`git submodule update --init --recursive`).
- `cmake-build-debug/` is gitignored and disposable; if it was previously
  configured with a different generator, remove it before the `-G Ninja`
  configure step. The Phase D procedure uses Ninja exclusively and never
  invokes `make`.
- No source code was modified to produce this note. The verification here
  documents the in-scope `Source/DSP/RoutingEngine.{h,cpp}` only.

## 10. Status

- Files in scope (§3): `Source/DSP/RoutingEngine.h`,
  `Source/DSP/RoutingEngine.cpp`.
- Phase gate target: `AuralMaximizerPro_Standalone` (designated in §8).
- Verification commands: recorded in §5 (Ninja, no make; cppcheck;
  clang-format).
- Pass/fail: all checks PASS (§7).
