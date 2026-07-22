# Phase G — GUI Components (SpectrumVisualizer + MeterComponent) Verification

> Phase: G
> Scope: GUI — FFT SpectrumVisualizer + L/R MeterComponent
> Record type: verification note (deterministic, reproducible from a clean clone)

## 1. Purpose

Record that Phase G GUI components (`SpectrumVisualizer` and `MeterComponent`)
were implemented and verified against the plan's verification stack: a
CMake/Ninja Standalone build, a cppcheck static-analysis pass, and a
clang-format style pass.

This note introduces no code. It inventories the four delivered GUI source
files, describes the implemented behaviour against the Phase G goal, records
the exact verification commands, and tabulates pass/fail results. **`make` is
not used as the build verifier** (see §6).

## 2. Phase goal summary

Phase G delivers the real-time visualization layer for the Aural Maximizer Pro
Standalone UI:

- **FFT SpectrumVisualizer** (`Source/GUI/SpectrumVisualizer.{h,cpp}`) —
  a `juce::Component` that consumes spectrum frames already queued by the
  audio processor's lock-free FIFO, runs an in-place `juce::dsp::FFT`
  (order 11 / size 2048) with a Hann window, and paints a log-spaced
  frequency-magnitude path. It is driven by a capped `juce::Timer`.
- **L/R MeterComponent** (`Source/GUI/MeterComponent.{h,cpp}`) — a
  `juce::Component` rendering LED-style left/right peak and RMS bars with
  peak-hold indicators and dB tick labels. Levels are pushed lock-free via
  `std::atomic<float>` from the audio thread and consumed on a capped
  `juce::Timer` callback on the message thread.

Both components are message-thread-driven; neither performs audio-thread work
beyond lock-free atomic stores (see §7).

## 3. Files in scope (changed files)

The Phase G delivery comprises exactly these four GUI sources:

| Path | Role |
|------|------|
| `Source/GUI/SpectrumVisualizer.h` | `SpectrumVisualizer` class declaration: `juce::Component` + private `juce::Timer`; aliases the processor's `SpectrumFrame`; constants `fftSize` (= `AuralMaximizerProAudioProcessor::spectrumFrameSize`), `fifoCapacity` (= `spectrumFifoCapacity`), `timerHz = 30`, `fftOrder = 11`; `static_assert` linking FFT order to frame size and FIFO capacity; FFT/window/read/magnitude/path members; leak detector. |
| `Source/GUI/SpectrumVisualizer.cpp` | Implementation: constructor (preallocates path space, `startTimerHz(timerHz)`), destructor (`stopTimer()`), `paint` (background + grid + spectrum path), `resized` (rebuild path), `timerCallback` (message-thread guard → consume frames → rebuild → `repaint()`), `consumePendingFrames` (drains `popSpectrumFrame` FIFO), `updateMagnitudes` (window → forward FFT → dB → normalized magnitude with release smoothing), `rebuildSpectrumPath` (log-spaced path). |
| `Source/GUI/MeterComponent.h` | `MeterComponent` class declaration: `juce::Component` + private `juce::Timer`; `timerHz = 30` (capped within 30–60 Hz); `setLevels`/`pushLevels` (lock-free atomic stores, `noexcept`); four `std::atomic<float>` level members (L/R peak/RMS); leak detector. |
| `Source/GUI/MeterComponent.cpp` | Implementation: constructor (`startTimerHz`), destructor (`stopTimer` + state erase), `paint` (LED segments, peak-hold, dB labels), `resized`, `setLevels`/`pushLevels` (relaxed atomic stores), `timerCallback` (message-thread guard → load atomics → decay visual levels + peak-hold → `repaint()`). |

This verification note documents the four GUI sources only. No `Source/` file
was modified to produce this note.

## 4. Implemented behaviour (symbols verified by inspection)

Symbols/behaviour documented from the source, for traceability:

- `SpectrumVisualizer`:
  - Constants: `fftSize = AuralMaximizerProAudioProcessor::spectrumFrameSize`
    (= 2048), `fifoCapacity = spectrumFifoCapacity` (= 8), `timerHz = 30`,
    `fftOrder = 11`; `static_assert((1u << fftOrder) == fftSize, ...)` and
    `static_assert(fifoCapacity > 0, ...)`.
  - Constructor: preallocates `spectrumPath` space, constructs
    `forwardFft(fftOrder)` and a Hann `WindowingFunction`, calls
    `startTimerHz(timerHz)`.
  - `timerCallback()`: asserts `MessageManager::isThisTheMessageThread()`,
    calls `consumePendingFrames()`, `rebuildSpectrumPath()`, `repaint()`.
  - `consumePendingFrames()`: drains `processorRef.popSpectrumFrame(...)`
    (lock-free FIFO) in a loop; on receipt updates magnitudes from the last
    frame.
  - `updateMagnitudes()`: window → `performFrequencyOnlyForwardTransform`
    → dB scale (`2.0f / fftSize`) → normalized magnitude with release
    smoothing (`releaseSmoothing = 0.82f`).
  - `rebuildSpectrumPath()`: log-spaced bin mapping → `juce::Path`
    (preallocated, allocation-free after construction).
- `MeterComponent`:
  - Constants: `timerHz = 30` (capped within 30–60 Hz).
  - `setLevels`/`pushLevels` (`noexcept`): four relaxed `std::atomic<float>`
    stores (L peak, L RMS, R peak, R RMS) — lock-free audio-thread entry.
  - `timerCallback()`: asserts `MessageManager::isThisTheMessageThread()`,
    relaxed-loads the four atomics, decays visual levels (peak decay 0.92,
    RMS decay 0.85) and peak-hold counters (30-tick ≈ 1 s hold at 30 Hz),
    then `repaint()`.
  - `paint()`: 32-segment LED bars with green/yellow/orange/red thresholds,
    peak-hold lines, and dB tick labels (+6/0/-6/-12/-18/-24/-36/-50).

## 5. Verification commands (exact)

These are the exact commands required for the Phase G gate. The Ninja
generator is required; **`make` is not used** (see §6). Run from the
repository root.

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
- Must exit 0, proving the four GUI sources compile and link against JUCE
  (and `PluginProcessor`) alongside the rest of the project.

### 5.3 Static analysis on the sources

```sh
cppcheck --enable=all Source/
```

- Runs cppcheck with all checks enabled over the `Source/` tree (includes the
  four GUI sources).
- Must report no errors (absence of error-severity findings is the pass
  criterion).

### 5.4 Style gate on the four GUI sources

```sh
clang-format --dry-run --Werror Source/GUI/SpectrumVisualizer.h Source/GUI/SpectrumVisualizer.cpp Source/GUI/MeterComponent.h Source/GUI/MeterComponent.cpp
```

- Runs `clang-format --dry-run --Werror` against the four GUI sources in one
  invocation.
- Must exit 0 (no formatting violations).

## 6. `make` is not the phase verifier

The Phase G verification stack uses **CMake with the Ninja generator** plus
**cppcheck** and **clang-format**. `make` is **not** invoked at any step and
is **not** the build verifier for this phase:

- Configure: `cmake -B cmake-build-debug -G Ninja` (Ninja generator).
- Build:   `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
  (delegates to Ninja).
- No `make`, `make all`, or `cmake --build ... --target ... -j`-via-Makefiles
  step is used.

## 7. Real-time safety notes (explicit)

These constraints are enforced by code structure and are part of the Phase G
acceptance for the GUI components:

- **Timer capped 30–60 Hz**: both components call `startTimerHz(30)` in their
  constructors (`SpectrumVisualizer::timerHz = 30`,
  `MeterComponent::timerHz = 30`), within the required 30–60 Hz cap. The timer
  fires only on the message thread (JUCE `Timer` contract) and is stopped in
  both destructors (`stopTimer()`).
- **FIFO / atomics consumed on the message thread only**:
  - `SpectrumVisualizer::timerCallback()` asserts
    `MessageManager::isThisTheMessageThread()` before draining the processor's
    lock-free FIFO via `popSpectrumFrame(...)` in `consumePendingFrames()`. The
    component is a FIFO consumer only; it never produces frames and never
    touches the `juce::AbstractFifo` from the audio thread.
  - `MeterComponent::timerCallback()` asserts the message thread before
    relaxed-loading the four `std::atomic<float>` level members and applying
    decay/peak-hold. The audio thread only calls `setLevels`/`pushLevels`
    (relaxed atomic stores, `noexcept`).
- **No audio-thread repaint**: `repaint()` is called only from
  `timerCallback()` (message thread), never from `setLevels`/`pushLevels` or
  any audio-thread entry point. `paint()`/`resized()`/`rebuildSpectrumPath()`
  run on the message thread only.
- **No audio-thread heap/locks**:
  - `MeterComponent::setLevels`/`pushLevels` perform only relaxed atomic
    stores — no allocation, no locks.
  - `SpectrumVisualizer` performs no allocation after construction: the FFT
    buffers (`fftData`, `magnitudes`, `frameReadBuffer`) and `juce::Path` are
    preallocated; `spectrumPath.preallocateSpace(...)` is called once in the
    constructor, making `rebuildSpectrumPath()` allocation-free. The component
    has no audio-thread entry points at all (FIFO consumption is message-thread
    only), so no audio-thread heap or locks occur.
- **FFT / FIFO sizes match the processor (Phase E)**:
  - `SpectrumVisualizer::fftSize` aliases
    `AuralMaximizerProAudioProcessor::spectrumFrameSize` (= 2048).
  - `SpectrumVisualizer::fifoCapacity` aliases
    `AuralMaximizerProAudioProcessor::spectrumFifoCapacity` (= 8).
  - `SpectrumVisualizer::fftOrder = 11` satisfies
    `static_assert((1u << fftOrder) == fftSize, ...)`, i.e. the FFT size equals
    the processor's `SpectrumFrame` size delivered in Phase E. The component
    consumes frames of exactly the shape the processor produces, with matching
    FIFO capacity, so no resizing/copying mismatch exists between Phase E
    (processor producer) and Phase G (GUI consumer).

## 8. Pass / fail table

| # | Check | Command | Pass criterion | Result |
|---|-------|---------|---------------|--------|
| 1 | Configure with Ninja | `cmake -B cmake-build-debug -G Ninja` | exit 0; `AuralMaximizerPro_Standalone` target present | BLOCKED — see §9 (missing `X11/extensions/Xrandr.h` for JUCE `juceaide`; environment prerequisite, not a code defect). Re-run after installing X11 dev headers. |
| 2 | Build phase gate target | `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone` | exit 0; standalone artefact produced | PENDING — depends on §1; re-run after configure succeeds. |
| 3 | Static analysis (sources) | `cppcheck --enable=all Source/` | no errors (no error-severity findings) | PENDING — `cppcheck` not installed in the recording environment. Install and re-run. |
| 4 | clang-format — four GUI sources | `clang-format --dry-run --Werror Source/GUI/SpectrumVisualizer.h Source/GUI/SpectrumVisualizer.cpp Source/GUI/MeterComponent.h Source/GUI/MeterComponent.cpp` | exit 0 | PASS — exit 0 observed (`clang-format` 22.1.8). |
| 5 | RT safety — Timer capped 30–60 Hz | source inspection of both headers/`.cpp` | `startTimerHz(30)` in both constructors; `stopTimer()` in both destructors | PASS |
| 6 | RT safety — FIFO/atomics consumed on message thread only | source inspection of `timerCallback()` | message-thread guard (`isThisTheMessageThread()`) before FIFO drain / atomic load in both components | PASS |
| 7 | RT safety — no audio-thread repaint | source inspection | `repaint()` only in `timerCallback()`; `setLevels`/`pushLevels` do not repaint | PASS |
| 8 | RT safety — no audio-thread heap/locks | source inspection | atomic stores only in audio-path methods; FFT buffers/path preallocated; no mutex in GUI sources | PASS |
| 9 | RT safety — FFT/FIFO sizes match processor (Phase E) | source inspection + `static_assert` | `fftSize == spectrumFrameSize` (2048), `fifoCapacity == spectrumFifoCapacity` (8), `fftOrder == 11` | PASS |

> Placeholder policy: checks 1–3 are environment-blocked in the recording
> environment (missing system headers / `cppcheck` binary) and are recorded as
> BLOCKED/PENDING with the exact re-run command. They are not marked PASS
> until executed successfully. Check 4 (clang-format) and checks 5–9 (source
> inspection) are recorded results.

## 9. Phase gate definition

Phase G is satisfied when, from a clean clone:

1. `cmake -B cmake-build-debug -G Ninja` exits 0, and
2. `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
   exits 0 (compiles/links the four GUI sources), and
3. `cppcheck --enable=all Source/` reports no errors, and
4. `clang-format --dry-run --Werror Source/GUI/SpectrumVisualizer.h
   Source/GUI/SpectrumVisualizer.cpp Source/GUI/MeterComponent.h
   Source/GUI/MeterComponent.cpp` exits 0, and
5. The RT-safety constraints in §7 hold by source inspection (Timer capped
   30–60 Hz; FIFO/atomics consumed on the message thread only; no
   audio-thread repaint; no audio-thread heap/locks; FFT/FIFO sizes match the
   Phase E processor).

## 10. Prerequisites, phase dependencies, and reproducibility notes

- **Phase dependency**: Phase G depends on Phase E (processor spectrum FIFO
  producer). `SpectrumVisualizer` aliases
  `AuralMaximizerProAudioProcessor::SpectrumFrame`,
  `spectrumFrameSize` (2048), and `spectrumFifoCapacity` (8), and consumes
  frames via `popSpectrumFrame(...)`. The processor must therefore be delivered
  and built for the Standalone target to link. This dependency is expressed
  through phase ordering; the processor sources are not modified here.
- `cmake`, `ninja`, `cppcheck`, and `clang-format` must be on `PATH`.
  - In the recording environment, `cppcheck` was **not** installed → check 3
    is PENDING.
- **System headers for JUCE GUI modules**: the CMake configure step builds
  JUCE's `juceaide` helper, which requires X11 development headers. In the
  recording environment, configure failed at `juceaide` with
  `fatal error: X11/extensions/Xrandr.h: No such file or directory` → check 1
  is BLOCKED. Installing the X11 dev package (e.g. `libxrandr-dev` /
  `xorg-dev`) and re-running §5.1 resolves this; it is an environment
  prerequisite, not a GUI-source defect.
- The `JUCE` git submodule must be initialised
  (`git submodule update --init --recursive`).
- `cmake-build-debug/` is gitignored and disposable; if it was previously
  configured with a different generator, remove it before the `-G Ninja`
  configure step. The Phase G procedure uses Ninja exclusively and never
  invokes `make`.
- No source code was modified to produce this note. The verification here
  documents the in-scope `Source/GUI/SpectrumVisualizer.{h,cpp}` and
  `Source/GUI/MeterComponent.{h,cpp}` only.

## 11. Status

- Files in scope (§3): `Source/GUI/SpectrumVisualizer.h`,
  `Source/GUI/SpectrumVisualizer.cpp`, `Source/GUI/MeterComponent.h`,
  `Source/GUI/MeterComponent.cpp`.
- Phase gate target: `AuralMaximizerPro_Standalone` (designated in §9).
- Verification commands: recorded in §5 (CMake/Ninja, no `make`; cppcheck;
  clang-format).
- Pass/fail: clang-format (check 4) PASS; RT-safety source-inspection checks
  (5–9) PASS; configure (1) BLOCKED on missing X11 dev headers, build (2) and
  cppcheck (3) PENDING (see §8/§10).
