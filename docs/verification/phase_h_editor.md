# Phase H — PluginEditor Wiring (createEditor + Editor Contract) Verification

Phase: H
Task: h.5 — Record phase H editor verification
Date: 2026-07-21

## Purpose

This note records the deterministic verification evidence for the Phase H
work in `AuralMaximizerPro`: the `PluginEditor` class that hosts the full
plugin UI — the fixed 920x540 frame, the split-screen Maximizer (cyan) /
Exciter (amber) / Routing & Master (green) regions, the embedded
`SpectrumVisualizer` and `MeterComponent`, the `CustomLookAndFeel` instance,
and the 19 APVTS attachments that bind every control to the processor's
parameter tree — plus the `PluginProcessor::createEditor()` change that
returns a `PluginEditor`.

The Phase H verification gate is the **Standalone** build target plus
`cppcheck` static analysis and `clang-format` formatting checks applied to
the three Phase H sources. The build generator is **Ninja**. `make` is
intentionally not used and is not documented as the primary build command.

## Phase goal summary

Phase H closes the editor attachment loop opened (and deferred) in phases
F and G. The Phase H goals are:

- **Editor contract (declared):** `PluginEditor` inherits
  `juce::AudioProcessorEditor`, declares the fixed frame constants
  `defaultWidth = 920` / `defaultHeight = 540` (46:27 aspect ratio), owns a
  `CustomLookAndFeel` instance, embeds a `SpectrumVisualizer` and a
  `MeterComponent`, and declares all 19 parameter controls
  (`ToggleButton`/`Slider`/`ComboBox`) plus their 19 APVTS attachments.
- **createEditor returns PluginEditor:** `PluginProcessor::createEditor()`
  returns `new PluginEditor (*this)` and `hasEditor()` returns `true`, so the
  host instantiates the real editor rather than a generic/empty one.
- **Split-screen layout:** the editor is split into three colour-coded
  regions — Maximizer (cyan, `#00D2FF`), Exciter (amber, `#FF9500`), and
  Routing & Master (green, `#34C759`) — matching the Phase F palette accents.
- **APVTS binding:** all 19 parameters are bound via
  `juce::AudioProcessorValueTreeState::{Slider,ComboBox,Button}Attachment`.
- **No product-code drift in this task:** task h.5 only records verification;
  it does not implement or modify product code.

## Files in scope (Phase H changed files)

The Phase H deliverables are the three sources the task names as changed:

- `Source/PluginEditor.h` — declares the `PluginEditor` class (inherits
  `juce::AudioProcessorEditor`), the `defaultWidth = 920` /
  `defaultHeight = 540` constexpr frame constants, the constructor/destructor,
  `paint`/`resized` overrides, a `CustomLookAndFeel customLookAndFeel`
  member, embedded `SpectrumVisualizer spectrumVisualizer` and
  `MeterComponent meterComponent` members, the 19 control members grouped by
  section (Maximizer: `maxEnableToggle`, `maxProcessSlider`,
  `maxContourSlider`, `maxLowFreqSlider`, `maxHighFreqSlider`,
  `maxPhaseModeBox`; Exciter: `excEnableToggle`, `excTuneSlider`,
  `excDriveSlider`, `excMixSlider`, `excModeBox`, `excHarmonicsSlider`,
  `excAirGainSlider`; Routing & Master: `routingModeBox`,
  `parallelBlendSlider`, `masterInGainSlider`, `masterOutGainSlider`,
  `masterMixSlider`, `oversamplingBox`), the 19
  `std::unique_ptr<...Attachment>` members, and a
  `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` member.
- `Source/PluginEditor.cpp` — implements the editor. (On-disk state at author
  time: still the skeleton stub — constructor sets `setSize(400, 300)` and
  performs no component attachment, no `addAndMakeVisible`, no APVTS binding,
  no split-screen layout; `paint` and `resized` are empty. See §Scope
  inspected and the editor contract checklist.)
- `Source/PluginProcessor.cpp` — `hasEditor()` returns `true` and
  `createEditor()` returns `new PluginEditor (*this)`. (On-disk state at
  author time: both bodies are present and return the real editor.)

No other source or build files are in scope for this task. This verification
record (`docs/verification/phase_h_editor.md`) is the only file written by
task h.5; no source files are modified by this task.

## Scope inspected

Before authoring this record, the following paths were inspected against the
working tree:

- `Source/PluginEditor.h` — present on disk. The header is the **fully
  declared** Phase H contract: 920x540 constexpr frame constants, the
  `CustomLookAndFeel` instance, embedded `SpectrumVisualizer` +
  `MeterComponent`, all 19 control members grouped by the three sections,
  and all 19 `std::unique_ptr<...Attachment>` members. Includes
  `GUI/CustomLookAndFeel.h`, `GUI/MeterComponent.h`, `GUI/SpectrumVisualizer.h`,
  and `PluginProcessor.h`.
- `Source/PluginEditor.cpp` — present on disk but **still the skeleton
  stub**. The constructor body is `setSize (400, 300);` only; it does **not**
  set the 920x540 frame, does **not** call `addAndMakeVisible` for any
  component, does **not** construct any of the 19 attachments, does **not**
  install the `CustomLookAndFeel`, and does **not** lay out the split-screen
  regions. `paint` does `juce::ignoreUnused (g);` and draws nothing;
  `resized` is empty. The editor contract is therefore declared in the header
  but **not yet implemented** in the implementation file.
- `Source/PluginProcessor.cpp` — present on disk. `hasEditor()` returns
  `true` (line 147) and `createEditor()` (lines 150–153) returns
  `new PluginEditor (*this)`. This is the on-disk change that satisfies the
  "createEditor now returns PluginEditor" acceptance item.
- `docs/verification/phase_a_build_skeleton.md`,
  `phase_b_sonic_maximizer.md`, `phase_c_aural_exciter.md`,
  `phase_d_routing_engine.md`, `phase_e_processor_apvts.md`,
  `phase_f_lookandfeel.md`, `phase_g_gui_components.md` — inspected for the
  existing phase verification markdown style. Phase H follows the same
  structure (purpose, phase goal summary, files in scope, scope inspected,
  verification commands, pass/fail table, editor contract checklist,
  residual risks, gate summary).

The Phase H source state is **asymmetric on disk**: the editor **contract**
is fully declared in `PluginEditor.h` (frame constants, embedded components,
all 19 controls, all 19 attachments) and `PluginProcessor.cpp` correctly
returns `new PluginEditor (*this)` from `createEditor()`, but
`PluginEditor.cpp` is still the skeleton stub that does not realise the
contract. This record documents that on-disk reality and does not substitute
model self-evaluation for tool output: the verification gate commands below
were not runnable in this environment (the toolchain is absent — see the
pass/fail table), and the editor contract criteria that depend on the `.cpp`
implementation are recorded as **not yet satisfied** rather than passing.

## Verification commands

These are the exact commands recorded for the Phase H editor gate. `make`
is deliberately excluded.

### 1. Configure (Ninja generator)

```bash
cmake -B cmake-build-debug -G Ninja
```

Configures the project into `cmake-build-debug` using the Ninja generator.
This validates `CMakeLists.txt`, the JUCE setup, the plugin target
declaration, and the `target_sources` list (including `PluginEditor.h`/
`PluginEditor.cpp` and `PluginProcessor.cpp`) against the on-disk files.

### 2. Build the Standalone target (phase gate)

```bash
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
```

Builds only the `AuralMaximizerPro_Standalone` target. A successful build of
this target is the Phase H phase gate. The VST3/CLAP/LV2 formats are
configured but are intentionally not part of the Phase H gate; the full
multi-format build is deferred to phase i.

### 3. cppcheck static analysis on the Source tree

```bash
cppcheck --enable=all Source/
```

Runs `cppcheck` with `--enable=all` across the entire `Source/` tree (which
includes the three Phase H sources). `--enable=all` enables style,
performance, portability, and unused-function checks in addition to the
default error checks. A clean run (no error-level issues blocking the
build) is the expected pass outcome.

### 4. clang-format dry-run on the three Phase H sources

```bash
clang-format --dry-run --Werror Source/PluginEditor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp
```

Runs `clang-format` in dry-run mode with `--Werror` on exactly the three
Phase H files. Any formatting deviation fails the check. No files are
rewritten by this command.

## Pass / fail table

| # | Check                                                                                                  | Tool            | Pass condition                                                                                                                                              | Outcome                                                                  |
|---|--------------------------------------------------------------------------------------------------------|-----------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------|
| 1 | Configure with Ninja generator                                                                         | `cmake`         | `cmake -B cmake-build-debug -G Ninja` exits 0                                                                                                               | Not run in this environment (`cmake` not available, command not found).  |
| 2 | Build `AuralMaximizerPro_Standalone` target                                                            | `cmake`/`ninja` | `cmake --build ... --target AuralMaximizerPro_Standalone` exits 0, produces artifact                                                                        | Not run in this environment (toolchain not available).                   |
| 3 | Static analysis on the `Source/` tree (includes the three Phase H sources)                              | `cppcheck`      | `cppcheck --enable=all Source/` exits 0, no error-level issues                                                                                             | Not run in this environment (`cppcheck` not available).                  |
| 4 | Formatting on the three Phase H sources                                                                | `clang-format`  | `clang-format --dry-run --Werror Source/PluginEditor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp` exits 0                                          | Not run in this environment (`clang-format` not available).              |

The verification commands above are the recorded Phase H gate. Execution was
attempted in this verification environment, but `cmake`, `ninja`,
`cppcheck`, and `clang-format` were **not available** (commands not found):

```text
$ which cmake ninja cppcheck clang-format
bash: cmake: command not found
bash: ninja: command not found
bash: cppcheck: command not found
bash: clang-format: command not found
```

Only `/usr/bin/make` was present on `PATH`; it is intentionally **not** used
as the Phase H verifier (see the build system note below).

The CMake/Ninja build gate (`AuralMaximizerPro_Standalone`) and the
`cppcheck`/`clang-format` static gates could therefore not be executed from
this environment at author time. The commands are recorded verbatim above as
the deterministic, reproducible Phase H gate: they are runnable from a clean
clone on any machine with the standard C++ toolchain (CMake ≥ 3.x, Ninja,
cppcheck, clang-format) installed.

This record does not substitute model self-evaluation for tool output. Where
the toolchain is available, the pass condition is:

- `cmake -B cmake-build-debug -G Ninja` exits 0.
- `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
  exits 0 and produces the `AuralMaximizerPro_Standalone` artifact.
- `cppcheck --enable=all Source/` exits 0 with no error-level issues
  blocking the build.
- `clang-format --dry-run --Werror Source/PluginEditor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp`
  exits 0.

The source-level editor contract checklist below is verifiable directly from
the on-disk files (which were inspected in full) and is recorded per-file.

## Editor contract checklist

The Phase H editor contract is the combination of the header declaration, the
implementation, and the processor's `createEditor()` wiring. Each contract
item is recorded with its **actual on-disk status** at author time.

| # | Contract item                                                                          | `PluginEditor.h` (on-disk)                                                              | `PluginEditor.cpp` (on-disk)                                                                                  | `PluginProcessor.cpp` (on-disk)                                  | Status            |
|---|----------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------|-------------------|
| 1 | Fixed 920x540 frame                                                                    | `defaultWidth = 920`, `defaultHeight = 540` constexpr declared.                          | Constructor sets `setSize (400, 300);` — the 920x540 frame is **not** applied.                                | N/A                                                              | Header: confirmed; .cpp: not yet satisfied.    |
| 2 | Split-screen Maximizer (cyan) / Exciter (amber) / Routing & Master (green) regions     | The 19 controls are grouped into Maximizer/Exciter/Routing & Master sections in comments. | `paint` draws nothing; `resized` is empty — no region rectangles are laid out or filled.                      | N/A                                                              | Header: grouped; .cpp: not yet satisfied.       |
| 3 | `SpectrumVisualizer` embedded                                                          | `SpectrumVisualizer spectrumVisualizer;` member declared.                               | No `addAndMakeVisible (spectrumVisualizer)` call; component not added/positioned.                              | N/A                                                              | Header: confirmed; .cpp: not yet satisfied.    |
| 4 | `MeterComponent` embedded                                                              | `MeterComponent meterComponent;` member declared.                                       | No `addAndMakeVisible (meterComponent)` call; component not added/positioned.                                | N/A                                                              | Header: confirmed; .cpp: not yet satisfied.    |
| 5 | `CustomLookAndFeel` installed                                                          | `CustomLookAndFeel customLookAndFeel;` member declared.                                 | No `setLookAndFeel (&customLookAndFeel)` call in the constructor.                                             | N/A                                                              | Header: confirmed; .cpp: not yet satisfied.    |
| 6 | All 19 APVTS attachments bound                                                          | All 19 `std::unique_ptr<...Attachment>` members declared (6 Maximizer, 7 Exciter, 6 Routing & Master). | No attachment is constructed in the constructor; all 19 remain null.                                         | N/A                                                              | Header: declared; .cpp: not yet satisfied.      |
| 7 | `createEditor()` returns `PluginEditor`                                                | N/A                                                                                     | N/A                                                                                                           | `createEditor()` returns `new PluginEditor (*this)`; `hasEditor()` returns `true`. | Confirmed.       |

Summary: the editor **contract is fully declared** in `PluginEditor.h`
(frame constants, embedded `SpectrumVisualizer` + `MeterComponent`,
`CustomLookAndFeel` member, all 19 controls, all 19 attachments) and the
processor **does** return `new PluginEditor (*this)` from `createEditor()`
(item 7, confirmed). However `PluginEditor.cpp` is still the skeleton stub:
it sets a 400x300 frame, attaches no components, installs no look-and-feel,
and constructs none of the 19 attachments. Contract items 1–6 are therefore
**declared but not yet implemented** on disk. This mirrors the asymmetric
state recorded for `SpectrumVisualizer` in the Phase G verification record.

## Build system note

`make` was **not used** for any Phase H verification step and is **not the
Phase H verifier**. The build is driven by CMake with the **Ninja** generator
(`-G Ninja`) and invoked through `cmake --build`. No `make`, `make all`, or
`make AuralMaximizerPro_Standalone` commands were issued or recorded. The
Standalone stack plus `cppcheck` and `clang-format` are the recorded Phase H
gate, not `make`.

## Multi-format build deferral

The full multi-format build (VST3, CLAP, LV2) is **deferred to phase i** and
is intentionally **not** part of the Phase H gate. Phase H builds and
verifies only the `AuralMaximizerPro_Standalone` target. The VST3/CLAP/LV2
targets remain configured in `CMakeLists.txt` but are not built, not
verified, and not gated in this phase. No `cmake --build` invocation for
those formats was issued or recorded.

## Phase H acceptance checklist

Each Phase H acceptance item from the task definition is addressed below
with its **actual on-disk status** at author time.

| # | Criterion                                                                                                                                                       | Status at author time        | Evidence                                                                                                                                                                                                                                                                                                                  |
|---|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | `docs/verification/phase_h_editor.md` exists                                                                                                                   | Satisfied (this file)        | Written by task h.5.                                                                                                                                                                                                                                                                                                       |
| 2 | Lists `Source/PluginEditor.h`, `Source/PluginEditor.cpp`, and `Source/PluginProcessor.cpp` as changed files                                                     | Satisfied                    | All three files listed in §Files in scope and re-listed in the clang-format command.                                                                                                                                                                                                                                       |
| 3 | Records `cmake -B cmake-build-debug -G Ninja` and `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`                                        | Satisfied                    | Both commands recorded verbatim in §Verification commands.                                                                                                                                                                                                                                                                |
| 4 | Records `cppcheck --enable=all Source/`                                                                                                                          | Satisfied                    | Command recorded verbatim in §Verification commands (the `Source/`-wide form required by the task).                                                                                                                                                                                                                        |
| 5 | Records `clang-format --dry-run --Werror` on the three Phase H sources                                                                                          | Satisfied                    | Command recorded verbatim with all three `Source/...` paths.                                                                                                                                                                                                                                                               |
| 6 | States `make` was not used and the full multi-format (VST3/CLAP/LV2) build is deferred to phase i                                                                | Satisfied                    | §Build system note states `make` was not used; §Multi-format build deferral states VST3/CLAP/LV2 is deferred to phase i.                                                                                                                                                                                                  |
| 7 | Pass/fail summary for the Standalone build, cppcheck, and clang-format                                                                                          | Satisfied (recorded)         | §Pass / fail table records the pass conditions and the not-run outcomes for all four gates (configure, Standalone build, cppcheck, clang-format).                                                                                                                                                                        |
| 8 | No file outside the task's `allowed_write_paths` is created or modified                                                                                          | Satisfied                    | File scope: only `docs/verification/phase_h_editor.md` (see git status below). No `Source/` files were modified by task h.5.                                                                                                                                                                                              |

Summary: of the eight Phase H acceptance items, all **8 of 8** are satisfied
at the document/scope level. The four toolchain-dependent gates (configure,
build, cppcheck, clang-format) are recorded verbatim as the deterministic
gate but were **not executed** in this environment because the tools are not
installed; they remain to be run in a toolchain-equipped environment to close
the build/analysis gate. The editor contract itself (§Editor contract
checklist) is fully declared in the header and `createEditor()` returns
`PluginEditor`, but `PluginEditor.cpp` is still the stub and does not yet
realise the 920x540 frame, split-screen regions, component embedding, or the
19 APVTS bindings.

## Residual risks

- **`PluginEditor.cpp` not yet authored:** the on-disk `PluginEditor.cpp` is
  the skeleton stub (`setSize(400, 300)`, empty `paint`/`resized`, no
  attachments constructed, no `addAndMakeVisible`, no `setLookAndFeel`). The
  Phase H editor contract — 920x540 frame, split-screen cyan/amber/green
  regions, embedded `SpectrumVisualizer` + `MeterComponent`, `CustomLookAndFeel`
  install, and all 19 APVTS attachments — must be implemented in the `.cpp`
  before contract items 1–6 can be considered satisfied. This is the primary
  residual Phase H gap.
- **Stub frame mismatch:** the stub constructor sets `setSize (400, 300)`,
  which contradicts the 920x540 `defaultWidth`/`defaultHeight` constants
  declared in the header. Until the `.cpp` is authored, the runtime editor
  window is 400x300, not 920x540.
- **Phase E spectrum producer not landed:** per
  `docs/verification/phase_e_processor_apvts.md`, the Phase E
  `PluginProcessor` re-author (which defines the `AbstractFifo` +
  `std::array<float, N>` spectrum path that `SpectrumVisualizer` consumes)
  did not land at author time. Even once `PluginEditor.cpp` calls
  `addAndMakeVisible (spectrumVisualizer)`, the visualiser will have no
  spectrum frames to render until the Phase E producer exists. This is the
  same cross-phase blocker recorded in the Phase G verification record.
- **`SpectrumVisualizer` still stub:** per the Phase G verification record,
  `SpectrumVisualizer.h`/`.cpp` are still the skeleton stub with no FFT/FIFO
  consumption or timer-driven repaint. Embedding it in the editor will
  compile and display an empty component, but no spectrum will render until
  both the Phase E producer and the Phase G visualiser implementation land.
- **Toolchain availability:** the Phase H gate depends on CMake, Ninja,
  cppcheck, and clang-format being installed. In the environment used to
  author this record they were absent (commands not found; only `/usr/bin/make`
  was present, and it is deliberately not used). The gate must be re-run in
  an environment with the toolchain present before Phase H can be closed as
  fully passing at the build/analysis level.
- **`cppcheck --enable=all Source/` is tree-wide:** unlike the per-file gates
  used in phases B–F, the Phase H task specifies `cppcheck --enable=all Source/`
  across the whole source tree. Pre-existing findings in other phases' sources
  (B/C/D/E/F/G) will surface in the Phase H run. Only error-level issues block
  the gate; style/unused notes from JUCE macros
  (`JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`, override signatures) are
  expected and do not block. A re-run must confirm no error-level findings
  attributable to the three Phase H sources.
- **clang-format fidelity:** `PluginEditor.h` uses the JUCE space-padded
  signature style while `PluginEditor.cpp` uses a compact style.
  `clang-format --dry-run --Werror` is the authority on whether the on-disk
  formatting conforms to the project `.clang-format`; this could not be
  confirmed here and must be re-run with the tool installed. A formatting
  failure would require a `clang-format -i` pass on the three Phase H files
  only.
- **Multi-format build deferred to phase i:** the VST3/CLAP/LV2 artifacts are
  not built or verified in Phase H. A standalone-only gate does not exercise
  the plugin-format wrapping code paths; the full multi-format build must be
  run in phase i before the plugin is considered shippable to hosts.

## Phase changed files

The files changed as part of Phase H (this task, h.5) are:

- `docs/verification/phase_h_editor.md` (this record — written by task h.5)

The files **intended** to be the Phase H deliverables (the three sources named
by the task) are:

- `Source/PluginEditor.h`
- `Source/PluginEditor.cpp`
- `Source/PluginProcessor.cpp`

These three sources are referenced as the files under test and were
**inspected, not modified**, by task h.5. No source or build files outside
this list were created or modified by this task.

Git working-tree status at author time showed only pre-existing runtime
state / report / log files under `.abracapocus/` (excluded from commit per
AGENTS.md) as modified or untracked; no `Source/` or `docs/` files other
than this verification artifact are touched by this task.

## Phase gate summary

- Phase gate target: `AuralMaximizerPro_Standalone`
- Generator: Ninja (`-G Ninja`)
- Build directory: `cmake-build-debug`
- Primary build command: `cmake --build` (not `make`)
- Static analysis gate: `cppcheck --enable=all Source/` (tree-wide, includes
  the three Phase H sources)
- Formatting gate: `clang-format --dry-run --Werror` on the three Phase H
  files `Source/PluginEditor.h`, `Source/PluginEditor.cpp`,
  `Source/PluginProcessor.cpp`
- Multi-format build: VST3/CLAP/LV2 **deferred to phase i**; not built,
  verified, or gated in Phase H.
- File scope: only `docs/verification/phase_h_editor.md` is written by this
  task; the three sources are inspected, not modified.
- Editor contract: fully declared in `PluginEditor.h` (920x540 frame,
  `CustomLookAndFeel`, embedded `SpectrumVisualizer` + `MeterComponent`,
  all 19 controls, all 19 APVTS attachments); `PluginProcessor.cpp`
  `createEditor()` returns `new PluginEditor (*this)` (confirmed on disk);
  `PluginEditor.cpp` is still the stub and does not yet realise the
  920x540 frame, split-screen regions, component embedding, or APVTS
  bindings.
- `make` is **not** the Phase H verifier; only the CMake/Ninja Standalone
  stack plus `cppcheck` and `clang-format` are.
- Phase H acceptance items: 8 of 8 satisfied at the document/scope level.
  The four toolchain-dependent gates remain to be run in a
  toolchain-equipped environment.
- Out of scope for Phase H: VST3/CLAP/LV2 build gate (deferred to phase i),
  DSP behavior (phases B/C/D), processor APVTS/SPSC FIFO (phase E),
  LookAndFeel palette (phase F), GUI component internals (phase G).
