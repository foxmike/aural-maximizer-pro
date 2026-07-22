# Phase E — Processor APVTS / DSP Ownership / SPSC FIFO / Latency Verification

Phase: E
Task: e.3 — Record phase E processor APVTS verification
Date: 2026-07-21

## Purpose

This note records the deterministic verification evidence for the Phase E
work in `AuralMaximizerPro`: the `PluginProcessor` re-author that owns the
`juce::AudioProcessorValueTreeState` (APVTS) parameter layout, the owned DSP
instances (`SonicMaximizer`, `AuralExciter`, `RoutingEngine`), the lock-free
SPSC audio-to-UI spectrum transfer path (`juce::AbstractFifo` +
`std::array<float, N>`), and the reported plugin latency
(`setLatencySamples`) derived from the oversampling factor.

The Phase E verification gate is the **Standalone** build target plus
`cppcheck` static analysis and `clang-format` formatting checks applied
specifically to the two processor sources.

The build generator is **Ninja**. `make` is intentionally not used and is not
documented as the primary build command.

## Phase goal summary

Phase E moves the `PluginProcessor` from the Phase A skeleton stub to the real
integration host for the plugin. The Phase E goals are:

- **APVTS ownership:** `PluginProcessor` owns a
  `juce::AudioProcessorValueTreeState` constructed with a
  `createParameterLayout()` that declares all **19** project parameter IDs:
  `MAX_ENABLE`, `MAX_PROCESS`, `MAX_CONTOUR`, `MAX_LOW_FREQ`,
  `MAX_HIGH_FREQ`, `MAX_PHASE_MODE`, `EXC_ENABLE`, `EXC_TUNE`, `EXC_DRIVE`,
  `EXC_MIX`, `EXC_MODE`, `EXC_HARMONICS`, `EXC_AIR_GAIN`, `ROUTING_MODE`,
  `PARALLEL_BLEND`, `MASTER_IN_GAIN`, `MASTER_OUT_GAIN`, `MASTER_MIX`,
  `OVERSAMPLING`. Ranges, defaults, skews, and choice options must match the
  project parameter specification table exactly so later editor attachments
  do not break.
- **DSP ownership:** `PluginProcessor` owns the `SonicMaximizer`,
  `AuralExciter`, and `RoutingEngine` instances and drives them from
  `prepareToPlay` / `processBlock`.
- **SPSC FIFO spectrum path:** a lock-free single-producer/single-consumer
  path from the audio thread to the UI via `juce::AbstractFifo` and a
  `std::array<float, N>` spectrum buffer; `processBlock` pushes spectrum
  frames into this FIFO.
- **Latency from oversampling:** `prepareToPlay` (and any oversampling
  change) computes and reports latency via `setLatencySamples()` based on the
  selected oversampling factor.
- **Real-time (RT) safety constraints:**
  - `processBlock` begins with `juce::ScopedNoDenormals` as the **first**
    statement.
  - No `malloc`/`new`/`free` or STL container growth on the audio thread.
  - No locks (mutexes, spinlocks) taken in `processBlock`.
  - All DSP/oversampling/buffers pre-allocated in `prepareToPlay`.
  - Parameter reads on the audio thread use atomics / `SmoothedValue` only
    (input gain -> `RoutingEngine` -> output gain / dry-wet `MASTER_MIX`).
- **`createEditor` intentionally not wired to the real editor:** for Phase E
  the editor remains the generic/existing path (`GenericAudioProcessorEditor`
  or stub). The real split-screen `PluginEditor` is wired in a later editor
  attachment phase.

## Files in scope (Phase E changed files)

The Phase E deliverables are the two processor sources:

- `Source/PluginProcessor.h` (re-authored by task e.1 — APVTS member,
  `createParameterLayout` declaration, owned DSP members, `AbstractFifo` +
  spectrum `std::array`, latency state, `prepareToPlay`/`processBlock`/
  `releaseResources` signatures; declarations only, no bodies).
- `Source/PluginProcessor.cpp` (re-authored by task e.2 —
  `createParameterLayout()` with all 19 parameter IDs, constructor/destructor,
  `prepareToPlay` with `setLatencySamples`, `processBlock` with
  `ScopedNoDenormals` first + input gain -> `RoutingEngine` -> output
  gain/dry-wet + SPSC FIFO push, `releaseResources`, `createEditor` kept on
  the generic path).

No other source or build files are in scope for this task. This verification
record (`docs/verification/phase_e_processor_apvts.md`) is the only file
written by task e.3; no source files are modified by this task.

## Scope inspected

Before authoring this record, the following paths were inspected against the
working tree:

- `Source/PluginProcessor.h` — present on disk. As of this authoring it is
  still the **Phase A skeleton stub**: it declares only the JUCE
  `AudioProcessor` contract overrides (`prepareToPlay`, `releaseResources`,
  `isBusesLayoutSupported`, the two `processBlock` overloads,
  `createEditor`/`hasEditor`, program/state methods) and a
  `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`. It does **not** declare an
  `AudioProcessorValueTreeState` member, `createParameterLayout`, owned
  `SonicMaximizer`/`AuralExciter`/`RoutingEngine` members, an
  `AbstractFifo`/`std::array` spectrum path, or latency state members.
- `Source/PluginProcessor.cpp` — present on disk. As of this authoring it is
  still the **Phase A skeleton stub**: the constructor builds default
  `BusesProperties`, `prepareToPlay` ignores its arguments, `processBlock`
  contains `juce::ignoreUnused (midiMessages);` followed by
  `juce::ScopedNoDenormals noDenormals;` and a channel-count clear loop,
  `createEditor()` returns `new juce::GenericAudioProcessorEditor (*this)`,
  and `getStateInformation`/`setStateInformation` are no-ops. There is no
  `createParameterLayout()`, no APVTS construction, no `setLatencySamples`,
  no `RoutingEngine`/DSP drive, and no SPSC FIFO push.
- `docs/verification/phase_a_build_skeleton.md`, `phase_b_sonic_maximizer.md`,
  `phase_c_aural_exciter.md`, `phase_d_routing_engine.md` — inspected for the
  existing phase verification markdown style. Phase E follows the same
  structure (purpose, phase goal summary, files in scope, verification
  commands, pass/fail table, residual risks, gate summary).

### Phase execution context (e.1 and e.2)

Tasks e.1 and e.2 were dispatched to the `codex_cli` backend. Both were
**blocked by the execution environment before any file inspection or edits**:
every shell command (including `pwd`) failed immediately with

```text
bwrap: loopback: Failed RTM_NEWADDR: Operation not permitted
```

so the backend could not inspect the sources, the DSP headers, the parameter
spec, or run any verification command. Both runs recorded
`changed_files: []` and made **no modifications** to
`Source/PluginProcessor.h` or `Source/PluginProcessor.cpp`. The acceptance
reconciliation for both recorded `reconciliation_status:
"partially_satisfied"` (only the "no file outside scope modified" criterion
matched; all APVTS/FIFO/latency/processBlock criteria were unmatched because
no edits were made).

Consequently the on-disk processor sources at the time of this authoring are
the unmodified Phase A stubs described above. This record therefore does not
substitute model self-evaluation for tool output: the verification gate
commands below were not runnable in this environment, and the source files
under test do not yet contain the Phase E declarations/definitions. The
commands are recorded verbatim as the deterministic, reproducible Phase E
gate to be run in an environment with the standard C++ toolchain installed
once the e.1/e.2 source edits have actually landed.

## Verification commands

These are the exact commands recorded for the Phase E processor gate. `make`
is deliberately excluded.

### 1. Configure (Ninja generator)

```bash
cmake -B cmake-build-debug -G Ninja
```

Configures the project into `cmake-build-debug` using the Ninja generator.
This validates `CMakeLists.txt`, the JUCE setup, the plugin target
declaration, and the `target_sources` list (including the processor sources)
against the on-disk files.

### 2. Build the Standalone target (phase gate)

```bash
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
```

Builds only the `AuralMaximizerPro_Standalone` target. A successful build of
this target is the Phase E phase gate. The VST3/CLAP/LV2 formats are
configured but are intentionally not part of the Phase E gate.

### 3. cppcheck static analysis on processor sources

```bash
cppcheck --enable=all Source/PluginProcessor.h Source/PluginProcessor.cpp
```

Runs `cppcheck` with `--enable=all` on the processor header and
implementation only. `--enable=all` enables style, performance,
portability, and unused-function checks in addition to the default error
checks. A clean run (no error-level issues blocking the build) is the
expected pass outcome.

### 4. clang-format dry-run on processor sources

```bash
clang-format --dry-run --Werror Source/PluginProcessor.h Source/PluginProcessor.cpp
```

Runs `clang-format` in dry-run mode with `--Werror` on the processor header
and implementation only. Any formatting deviation fails the check. No files
are rewritten by this command.

## Pass / fail table

| # | Check                                                                                | Tool            | Pass condition                                                                                                          | Outcome                                                                                  |
|---|--------------------------------------------------------------------------------------|-----------------|-------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------|
| 1 | Configure with Ninja generator                                                       | `cmake`         | `cmake -B cmake-build-debug -G Ninja` exits 0                                                                           | Not run in this environment (`cmake` not available, command not found).                 |
| 2 | Build `AuralMaximizerPro_Standalone` target                                          | `cmake`/`ninja` | `cmake --build ... --target AuralMaximizerPro_Standalone` exits 0, produces artifact                                    | Not run in this environment (toolchain not available).                                   |
| 3 | Static analysis on `Source/PluginProcessor.h` and `Source/PluginProcessor.cpp`       | `cppcheck`      | `cppcheck --enable=all Source/PluginProcessor.h Source/PluginProcessor.cpp` exits 0, no error-level issues           | Not run in this environment (`cppcheck` not available).                                  |
| 4 | Formatting on `Source/PluginProcessor.h` and `Source/PluginProcessor.cpp`            | `clang-format`  | `clang-format --dry-run --Werror ...` exits 0                                                                           | Not run in this environment (`clang-format` not available).                              |

The verification commands above are the recorded Phase E gate. Execution was
attempted in this verification environment, but `cmake`, `ninja`,
`cppcheck`, and `clang-format` were **not available** (commands not found).
The CMake/Ninja build gate (`AuralMaximizerPro_Standalone`) and the
`cppcheck`/`clang-format` static gates could therefore not be executed from
this environment at author time. The commands are recorded verbatim above as
the deterministic, reproducible Phase E gate: they are runnable from a clean
clone on any machine with the standard C++ toolchain (CMake ≥ 3.x, Ninja,
cppcheck, clang-format) installed.

This record does not substitute model self-evaluation for tool output. Where
the toolchain is available and the e.1/e.2 source edits have landed, the pass
condition is:

- `cmake -B cmake-build-debug -G Ninja` exits 0.
- `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
  exits 0 and produces the `AuralMaximizerPro_Standalone` artifact.
- `cppcheck --enable=all Source/PluginProcessor.h Source/PluginProcessor.cpp`
  exits 0 with no error-level issues blocking the build.
- `clang-format --dry-run --Werror` on the two processor files exits 0.

## Build system note

`make` was **not used** for any Phase E verification step. The build is driven
by CMake with the **Ninja** generator (`-G Ninja`) and invoked through
`cmake --build`. No `make`, `make all`, or `make AuralMaximizerPro_Standalone`
commands were issued or recorded.

## Phase E acceptance checklist

Each Phase E acceptance item is addressed below with its **actual on-disk
status** at author time. Because e.1/e.2 were blocked and made no edits, the
processor sources remain the Phase A stubs; the Phase E source-level criteria
are therefore recorded as **not yet satisfied** rather than passing. This
reflects the real command/file outcomes and does not invent passing results.

| # | Criterion                                                                                          | Status at author time        | Evidence                                                                                                                                                                                                                                                                                                                                  |
|---|----------------------------------------------------------------------------------------------------|------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | `ScopedNoDenormals` is the **first** statement in `processBlock`                                   | Not yet satisfied (on-disk)  | `Source/PluginProcessor.cpp` `processBlock` begins with `juce::ignoreUnused (midiMessages);`; `juce::ScopedNoDenormals noDenormals;` follows. Phase E requires `ScopedNoDenormals` first; the e.2 re-author would enforce this but has not landed.                                                                                       |
| 2 | All **19** APVTS parameter IDs present in `createParameterLayout`                                  | Not yet satisfied (on-disk)  | No `createParameterLayout` and no `AudioProcessorValueTreeState` exist in the on-disk stub. None of the 19 IDs (`MAX_ENABLE`, `MAX_PROCESS`, `MAX_CONTOUR`, `MAX_LOW_FREQ`, `MAX_HIGH_FREQ`, `MAX_PHASE_MODE`, `EXC_ENABLE`, `EXC_TUNE`, `EXC_DRIVE`, `EXC_MIX`, `EXC_MODE`, `EXC_HARMONICS`, `EXC_AIR_GAIN`, `ROUTING_MODE`, `PARALLEL_BLEND`, `MASTER_IN_GAIN`, `MASTER_OUT_GAIN`, `MASTER_MIX`, `OVERSAMPLING`) appear in the processor sources. |
| 3 | `setLatencySamples` tracks oversampling                                                            | Not yet satisfied (on-disk)  | No `setLatencySamples` call and no oversampling/latency state in the on-disk stub; `prepareToPlay` ignores its arguments.                                                                                                                                                                                                                |
| 4 | `createEditor` not wired to real `PluginEditor`                                                    | Satisfied (on-disk)          | The on-disk `createEditor()` returns `new juce::GenericAudioProcessorEditor (*this)` and does not reference a real `PluginEditor`. Phase E requires this to remain the generic path until the later editor attachment phase.                                                                                                              |
| 5 | SPSC `AbstractFifo` spectrum path present                                                          | Not yet satisfied (on-disk)  | No `juce::AbstractFifo` member and no spectrum `std::array<float, N>` in the on-disk stub; no FIFO push in `processBlock`.                                                                                                                                                                                                                |
| 6 | RT-safety constraints respected (no alloc/lock on audio thread; atomics/`SmoothedValue` reads)     | Partially satisfied (on-disk) | The stub `processBlock` performs no allocation and takes no locks (only a channel-clear loop), but it also performs no parameter reads and no DSP/FIFO work, so the Phase E RT-safety contract is trivially met by absence rather than by the intended atomic/`SmoothedValue` design. Must be re-verified once e.2 lands.                  |

Summary: of the six Phase E source-level acceptance items, **1 of 6**
(item 4, `createEditor` generic path) is satisfied by the current on-disk
stub; the remaining **5 of 6** are not yet satisfied because the e.1/e.2
source re-authors did not land. The Phase E phase gate
(`AuralMaximizerPro_Standalone` build + `cppcheck` + `clang-format`) is
**not passing** at author time because (a) the toolchain is unavailable in
this environment and (b) the source edits that the gate is meant to verify
were not applied.

## Residual risks

- **e.1/e.2 source edits not landed:** the Phase E processor re-author was
  blocked by the backend execution sandbox (`bwrap: loopback: Failed
  RTM_NEWADDR: Operation not permitted`). The header (`Source/PluginProcessor.h`)
  and implementation (`Source/PluginProcessor.cpp`) re-authors must be
  re-run in a working environment before any Phase E criterion other than
  "generic `createEditor`" can be considered satisfied. Until then the
  processor is still the Phase A skeleton and cannot host the APVTS, owned
  DSP, SPSC FIFO, or latency reporting.
- **Toolchain availability:** the Phase E gate depends on CMake, Ninja,
  cppcheck, and clang-format being installed. In the environment used to
  author this record they were absent (commands not found). The gate must be
  re-run in an environment with the toolchain present before Phase E can be
  closed as fully passing.
- **Parameter layout fidelity:** once e.2 lands, `createParameterLayout()`
  must match the project parameter specification table exactly (all 19 IDs,
  ranges, defaults, skews, choice options). Drift here would break later
  editor attachment (Phase G/H) which binds UI controls to these IDs.
- **RT safety of `processBlock`:** the audio-thread path (input gain ->
  `RoutingEngine` -> output gain / `MASTER_MIX` dry-wet + SPSC FIFO push)
  must remain allocation-free and lock-free. Any future change that
  introduces STL growth, mutex acquisition, blocking I/O, buffer resizing,
  or non-atomic/non-`SmoothedValue` parameter reads inside `processBlock`
  is a regression of the Phase E gate. Not statically enforceable by
  `cppcheck` alone; must be re-verified on any edit to the processing path.
- **SPSC FIFO capacity / overflow policy:** the `AbstractFifo` +
  `std::array<float, N>` spectrum path must define a fixed `N` sized for
  spectrum frames and a documented overflow/drop policy when the UI consumer
  falls behind. An undersized buffer or an unspecified overflow policy is a
  later-phase risk for the editor attachment phase.
- **Latency reporting on oversampling change:** `setLatencySamples()` must be
  called whenever the oversampling factor changes (not only in
  `prepareToPlay`); failing to re-report latency on a runtime oversampling
  change would desync the host's delay compensation.
- **Later editor attachment phase:** `createEditor()` is intentionally left
  on the generic path for Phase E. Wiring the real split-screen
  `PluginEditor` (and binding its controls to the 19 APVTS IDs and the SPSC
  spectrum FIFO) is a later phase and must not be introduced prematurely
  here.

## Phase changed files

The files changed as part of Phase E are:

- `docs/verification/phase_e_processor_apvts.md` (this record — written by task e.3)

The files **intended** to be changed by Phase E (tasks e.1 and e.2) are:

- `Source/PluginProcessor.h`
- `Source/PluginProcessor.cpp`

At author time those two source files were **not actually modified**: the
e.1 and e.2 backend runs were blocked by the execution sandbox and recorded
`changed_files: []`. No source or build files outside this list were created
or modified by this task. `Source/PluginProcessor.h` and
`Source/PluginProcessor.cpp` are referenced as the files under test; they
were not modified by task e.3.

## Phase gate summary

- Phase gate target: `AuralMaximizerPro_Standalone`
- Generator: Ninja (`-G Ninja`)
- Build directory: `cmake-build-debug`
- Primary build command: `cmake --build` (not `make`)
- Static analysis gate: `cppcheck --enable=all Source/PluginProcessor.h
  Source/PluginProcessor.cpp`
- Formatting gate: `clang-format --dry-run --Werror` on
  `Source/PluginProcessor.h` and `Source/PluginProcessor.cpp`
- Phase E acceptance items: 1 of 6 satisfied on-disk (`createEditor` generic
  path); 5 of 6 not yet satisfied because e.1/e.2 source edits did not land.
- RT-safety constraints: `ScopedNoDenormals` first in `processBlock`; no
  locks or allocations on the audio thread; parameter reads via
  atomics/`SmoothedValue` only; DSP/buffers pre-allocated in `prepareToPlay`;
  `setLatencySamples` tracks oversampling.
- Out of scope for Phase E: VST3/CLAP/LV2 build gate, Sonic Maximizer / Aural
  Exciter / RoutingEngine DSP behavior (verified in phases B/C/D), and real
  `PluginEditor` wiring (deferred to the later editor attachment phase).
