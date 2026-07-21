# Phase B — Sonic Maximizer Verification

Phase: B
Task: b — Record phase B Sonic Maximizer verification
Date: 2026-07-21

## Purpose

This note records the deterministic verification evidence for the Phase B
Sonic Maximizer work in `AuralMaximizerPro`. Phase B implements the Sonic
Maximizer DSP block. The verification gate for this phase is the
**Standalone** build target plus static analysis and formatting checks on the
`Source/DSP` directory, with `clang-format --dry-run --Werror` applied
specifically to the Sonic Maximizer sources.

The build generator is **Ninja**. `make` is intentionally not used and is not
documented as the primary build command.

## Scope inspected

Before authoring this record, the following paths were inspected against the
working tree.

- `Source/DSP/SonicMaximizer.h` — present. Declares the `SonicMaximizer` class
  with `prepare(double, int)`, `process(juce::AudioBuffer<float>&)`, and
  `reset()` public methods, plus a private
  `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` declaration. No heap-allocating
  members are exposed in the header.
- `Source/DSP/SonicMaximizer.cpp` — present. Defines the constructor,
  destructor, and the three public methods. Phase B implementation lives here.

### Real-time constraints

The `process(juce::AudioBuffer<float>&)` method runs on the audio thread and
must be real-time (RT) safe. The following RT-safety constraints apply to the
Sonic Maximizer implementation and were considered when authoring this record:

- No `malloc`/`new`/`free` or STL container growth on the audio thread.
- No locks (mutexes, spinlocks) taken in `process()`.
- No `std::atomic` operations beyond the lock-free, wait-free reads/writes
  used for coefficient hand-off.
- Filter coefficient state is updated off-thread and swapped atomically into
  the processing path; the processing path must only read published
  coefficients.

### Coefficient update sites

Filter coefficients for the Sonic Maximizer (low-band and high-band shelving
filters) are recomputed when parameters change or when `prepare()` is invoked.
The relevant update sites are:

- `SonicMaximizer::prepare()` — recomputes coefficients from the supplied
  sample rate and stores them for the processing path. This runs on the
  prepare/message thread, not the audio thread.
- Parameter change handler (setters invoked from `PluginProcessor` APVTS
  listener / `parameterChanged`) — recomputes coefficients off-thread and
  publishes them for the audio thread to read.

Any live coefficient mutation must occur off the audio thread; the audio thread
must only read the currently published coefficient set. This is the primary
residual RT-safety risk for Phase B and is called out below.

## Verification commands

These are the exact commands recorded for the Phase B Sonic Maximizer gate.
`make` is deliberately excluded.

### 1. Configure (Ninja generator)

```bash
cmake -B cmake-build-debug -G Ninja
```

Configures the project into the `cmake-build-debug` build directory using the
Ninja generator. This step validates `CMakeLists.txt`, the JUCE setup, the
plugin target declaration, and the `target_sources` list (including the Sonic
Maximizer sources) against the on-disk files.

### 2. Build the Standalone target (phase gate)

```bash
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
```

Builds only the `AuralMaximizerPro_Standalone` target. A successful build of
this target is the Phase B phase gate. The VST3/CLAP/LV2 formats are
configured but are intentionally not part of the Phase B gate.

### 3. cppcheck static analysis on Source/DSP

```bash
cppcheck --enable=all Source/DSP
```

Runs `cppcheck` with `--enable=all` across the entire `Source/DSP` directory,
covering `SonicMaximizer.h`/`.cpp`, `AuralExciter.h`/`.cpp`, and
`RoutingEngine.h`/`.cpp`. `--enable=all` enables style, performance,
portability, and unused-function checks in addition to the default error
checks. A clean run (no errors/warnings) is the expected pass outcome.

### 4. clang-format dry-run on Sonic Maximizer sources

```bash
clang-format --dry-run --Werror \
  Source/DSP/SonicMaximizer.h \
  Source/DSP/SonicMaximizer.cpp
```

Runs `clang-format` in dry-run mode with `--Werror` on the Sonic Maximizer
header and implementation only. Any formatting deviation fails the check. No
files are rewritten by this command.

## Pass / fail outcomes

The verification commands above are the recorded Phase B gate. Execution was
attempted in this verification environment with the following result:

- `cmake`: **not available** in this environment (`command not found`, exit
  127). The Ninja configure command could not be executed here.
- `ninja`: **not available** in this environment.
- `cppcheck`: **not available** in this environment.
- `clang-format`: **not available** in this environment.

The CMake/Ninja build gate (`AuralMaximizerPro_Standalone`) and the
`cppcheck`/`clang-format` static gates could therefore not be executed from
this environment at author time. The commands are recorded verbatim above as
the deterministic, reproducible Phase B gate: they are runnable from a clean
clone on any machine with the standard C++ toolchain (CMake ≥ 3.x, Ninja,
cppcheck, clang-format) installed. The source files under test
(`Source/DSP/SonicMaximizer.h`, `Source/DSP/SonicMaximizer.cpp`) are present on
disk and were inspected; the declarations and definitions match.

This record does not substitute model self-evaluation for tool output. Where
the toolchain is available, the pass condition is:

- `cmake -B cmake-build-debug -G Ninja` exits 0.
- `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
  exits 0 and produces the `AuralMaximizerPro_Standalone` artifact.
- `cppcheck --enable=all Source/DSP` exits 0 with no errors or warnings.
- `clang-format --dry-run --Werror` on the two Sonic Maximizer files exits 0.

## Residual risks

- **RT safety of `process()`:** the audio-thread processing path must remain
  allocation-free and lock-free. Any future change that introduces STL growth,
  mutex acquisition, or blocking I/O inside `SonicMaximizer::process()` is a
  regression of the Phase B gate. This is not statically enforced by cppcheck
  alone; it must be re-verified on any edit to the processing path.
- **Coefficient update sites:** coefficients recomputed in `prepare()` or in
  parameter-change setters must be published off the audio thread and read
  atomically/lock-free by `process()`. A coefficient update performed directly
  on the audio thread, or a non-atomic shared write, would violate RT safety.
  This is the highest-priority residual risk for Phase B.
- **Toolchain availability:** the Phase B gate depends on CMake, Ninja,
  cppcheck, and clang-format being installed. In the environment used to
  author this record they were absent; the gate must be re-run in an
  environment with the toolchain present before Phase B can be closed as
  fully passing.

## Phase changed files

The files changed as part of Phase B (Sonic Maximizer) are:

- `Source/DSP/SonicMaximizer.h`
- `Source/DSP/SonicMaximizer.cpp`
- `docs/verification/phase_b_sonic_maximizer.md` (this record)

No source or build files outside this list were created or modified by this
task.

## Phase gate summary

- Phase gate target: `AuralMaximizerPro_Standalone`
- Generator: Ninja (`-G Ninja`)
- Build directory: `cmake-build-debug`
- Primary build command: `cmake --build` (not `make`)
- Static analysis gate: `cppcheck --enable=all Source/DSP`
- Formatting gate: `clang-format --dry-run --Werror` on
  `Source/DSP/SonicMaximizer.h` and `Source/DSP/SonicMaximizer.cpp`
- Out of scope for Phase B: VST3/CLAP/LV2 build gate, Aural Exciter and Routing
  Engine DSP behavior, GUI rendering, and parameter wiring beyond the Sonic
  Maximizer coefficient update sites (deferred to later phases).
