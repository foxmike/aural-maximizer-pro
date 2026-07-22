# Phase F — LookAndFeel / Palette / Widget Styling Verification

Phase: F
Task: f.3 — Record phase F lookandfeel verification
Date: 2026-07-21

## Purpose

This note records the deterministic verification evidence for the Phase F work
in `AuralMaximizerPro`: the `CustomLookAndFeel` class that defines the visual
styling and the exact palette colour constants used across the plugin UI, plus
its `LookAndFeel_V4` overrides for the three Phase F widgets — rotary sliders,
combo boxes, and toggle buttons.

The Phase F verification gate is the **Standalone** build target plus
`cppcheck` static analysis and `clang-format` formatting checks applied
specifically to the two LookAndFeel sources.

The build generator is **Ninja**. `make` is intentionally not used and is not
documented as a primary build command.

## Phase goal summary

Phase F introduces the project-wide visual theme via a single
`juce::LookAndFeel_V4` subclass (`CustomLookAndFeel`) so that every editor
widget draws against a shared dark palette and a consistent brushed-aluminum
knob / styled combo box / styled toggle button aesthetic. The Phase F goals
are:

- **Palette ownership:** `CustomLookAndFeel` declares the project palette as
  `inline static const juce::Colour` constants with exact hex values that all
  editor controls bind to:
  - Background `#121417`
  - Panel `#1A1D22`
  - Border `#2C3038`
  - Maximizer accent `#00D2FF`
  - Exciter accent `#FF9500`
  - Master accent `#34C759`
  - Text `#F2F2F7`
- **Widget overrides:** `CustomLookAndFeel` implements the three LookAndFeel
  overrides required for the Phase F widgets:
  - `drawRotarySlider` — brushed-aluminum knob with track arc + accent pointer.
  - `drawComboBox` — rounded panel background, border, and dropdown arrow.
  - `drawToggleButton` — rounded tick box with accent tick when active.
- **Shared colour wiring:** the constructor calls `setColour(...)` against the
  palette constants for `ComboBox`, `PopupMenu`, `Slider`, and
  `ToggleButton` colour IDs so every consumer of the look-and-feel inherits the
  palette without per-widget hardcoding.
- **No product-code drift in this task:** task f.3 only records verification; it
  does not implement or modify product code.

## Files in scope (Phase F changed files)

The Phase F deliverables are the two LookAndFeel sources:

- `Source/GUI/CustomLookAndFeel.h` — palette colour constants (Background,
  Panel, Border, MaximizerAccent, ExciterAccent, MasterAccent, Text and the
  Maximizer/Exciter/Master aliases), the public constructor/destructor, and
  the `drawRotarySlider` / `drawComboBox` / `getComboBoxFont` /
  `drawPopupMenuBackground` / `drawPopupMenuItem` / `drawToggleButton`
  override declarations, plus a
  `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` member.
- `Source/GUI/CustomLookAndFeel.cpp` — constructor colour wiring, the
  brushed-aluminum `drawRotarySlider` implementation (track arc, knob
  gradient body, inner cap, accent pointer), the `drawComboBox`
  implementation (rounded panel fill, border, dropdown arrow), the
  `getComboBoxFont` implementation, the `drawPopupMenuBackground` /
  `drawPopupMenuItem` implementations, and the `drawToggleButton`
  implementation (rounded tick box, accent tick, label).

No other source or build files are in scope for this task. This verification
record (`docs/verification/phase_f_lookandfeel.md`) is the only file written
by task f.3; no source files are modified by this task.

## Scope inspected

Before authoring this record, the following paths were inspected against the
working tree:

- `Source/GUI/CustomLookAndFeel.h` — present on disk. Declares the
  `CustomLookAndFeel` class (inherits `juce::LookAndFeel_V4`), the palette
  constants, the Maximizer/Exciter/Master aliases, and the six override
  declarations listed above.
- `Source/GUI/CustomLookAndFeel.cpp` — present on disk. Contains the
  constructor colour wiring and the full bodies for `drawRotarySlider`,
  `drawComboBox`, `getComboBoxFont`, `drawPopupMenuBackground`,
  `drawPopupMenuItem`, and `drawToggleButton`.
- `docs/verification/phase_a_build_skeleton.md`,
  `phase_b_sonic_maximizer.md`, `phase_c_aural_exciter.md`,
  `phase_d_routing_engine.md`, `phase_e_processor_apvts.md` — inspected for
  the existing phase verification markdown style. Phase F follows the same
  structure (purpose, phase goal summary, files in scope, verification
  commands, pass/fail table, palette checklist, residual risks, gate
  summary).

The Phase F source edits (f.1 / f.2) are present on disk: the LookAndFeel
sources are fully authored with the palette constants and widget overrides.
This is in contrast to Phase E, whose processor re-author was blocked. For
Phase F the gate commands below are the deterministic, reproducible gate to
be run in an environment with the standard C++ toolchain installed; the
source-level palette/widget checklist is verifiable directly from the
on-disk files and is recorded below.

## Verification commands

These are the exact commands recorded for the Phase F LookAndFeel gate. `make`
is deliberately excluded.

### 1. Configure (Ninja generator)

```bash
cmake -B cmake-build-debug -G Ninja
```

Configures the project into `cmake-build-debug` using the Ninja generator.
This validates `CMakeLists.txt`, the JUCE setup, the plugin target
declaration, and the `target_sources` list (including the LookAndFeel sources
under `Source/GUI/`) against the on-disk files.

### 2. Build the Standalone target (phase gate)

```bash
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
```

Builds only the `AuralMaximizerPro_Standalone` target. A successful build of
this target is the Phase F phase gate. The VST3/CLAP/LV2 formats are
configured but are intentionally not part of the Phase F gate.

### 3. cppcheck static analysis on LookAndFeel sources

```bash
cppcheck --enable=all Source/GUI/CustomLookAndFeel.h Source/GUI/CustomLookAndFeel.cpp
```

Runs `cppcheck` with `--enable=all` on the LookAndFeel header and
implementation only. `--enable=all` enables style, performance,
portability, and unused-function checks in addition to the default error
checks. A clean run (no error-level issues blocking the build) is the
expected pass outcome.

### 4. clang-format dry-run on LookAndFeel sources

```bash
clang-format --dry-run --Werror Source/GUI/CustomLookAndFeel.h Source/GUI/CustomLookAndFeel.cpp
```

Runs `clang-format` in dry-run mode with `--Werror` on the LookAndFeel header
and implementation only. Any formatting deviation fails the check. No files
are rewritten by this command.

## Pass / fail table

| # | Check                                                                                  | Tool            | Pass condition                                                                                                              | Outcome                                                                      |
|---|----------------------------------------------------------------------------------------|-----------------|-----------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------|
| 1 | Configure with Ninja generator                                                         | `cmake`         | `cmake -B cmake-build-debug -G Ninja` exits 0                                                                               | Not run in this environment (`cmake` not available, command not found).       |
| 2 | Build `AuralMaximizerPro_Standalone` target                                            | `cmake`/`ninja` | `cmake --build ... --target AuralMaximizerPro_Standalone` exits 0, produces artifact                                        | Not run in this environment (toolchain not available).                        |
| 3 | Static analysis on `Source/GUI/CustomLookAndFeel.h` and `Source/GUI/CustomLookAndFeel.cpp` | `cppcheck`      | `cppcheck --enable=all Source/GUI/CustomLookAndFeel.h Source/GUI/CustomLookAndFeel.cpp` exits 0, no error-level issues      | Not run in this environment (`cppcheck` not available).                       |
| 4 | Formatting on `Source/GUI/CustomLookAndFeel.h` and `Source/GUI/CustomLookAndFeel.cpp`  | `clang-format`  | `clang-format --dry-run --Werror ...` exits 0                                                                              | Not run in this environment (`clang-format` not available).                   |

The verification commands above are the recorded Phase F gate. Execution was
attempted in this verification environment, but `cmake`, `ninja`,
`cppcheck`, and `clang-format` were **not available** (commands not found):

```text
$ which cmake ninja cppcheck clang-format
bash: cmake: command not found
bash: ninja: command not found
bash: cppcheck: command not found
bash: clang-format: command not found
```

The CMake/Ninja build gate (`AuralMaximizerPro_Standalone`) and the
`cppcheck`/`clang-format` static gates could therefore not be executed from
this environment at author time. The commands are recorded verbatim above as
the deterministic, reproducible Phase F gate: they are runnable from a clean
clone on any machine with the standard C++ toolchain (CMake ≥ 3.x, Ninja,
cppcheck, clang-format) installed.

This record does not substitute model self-evaluation for tool output. Where
the toolchain is available, the pass condition is:

- `cmake -B cmake-build-debug -G Ninja` exits 0.
- `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
  exits 0 and produces the `AuralMaximizerPro_Standalone` artifact.
- `cppcheck --enable=all Source/GUI/CustomLookAndFeel.h Source/GUI/CustomLookAndFeel.cpp`
  exits 0 with no error-level issues blocking the build.
- `clang-format --dry-run --Werror` on the two LookAndFeel files exits 0.

The source-level palette and widget checklist below is verifiable directly
from the on-disk files (which were inspected in full) and is recorded as
**confirmed** because the f.1/f.2 source edits are present on disk.

## Palette hex checklist

The palette constants are declared in `Source/GUI/CustomLookAndFeel.h` as
`inline static const juce::Colour` using JUCE's `0xAARRGGBB` literal form
(`0xff` alpha prefix + the RGB hex). Each constant was inspected against the
required exact hex value:

| # | Palette entry      | Required hex | On-disk literal        | Status   |
|---|--------------------|--------------|------------------------|----------|
| 1 | Background         | `#121417`    | `0xff121417`           | Confirmed |
| 2 | Panel              | `#1A1D22`    | `0xff1a1d22`           | Confirmed |
| 3 | Border             | `#2C3038`    | `0xff2c3038`           | Confirmed |
| 4 | Maximizer accent   | `#00D2FF`    | `0xff00d2ff`           | Confirmed |
| 5 | Exciter accent     | `#FF9500`    | `0xffff9500`           | Confirmed |
| 6 | Master accent      | `#34C759`    | `0xff34c759`           | Confirmed |
| 7 | Text               | `#F2F2F7`    | `0xfff2f2f7`           | Confirmed |

All seven palette hex values match exactly. The three accent aliases
(`Maximizer`, `Exciter`, `Master`) re-declare the same literals and also
match. The palette is therefore the single source of truth for the dark
theme; no per-widget hardcoded colour deviates from these constants in the
inspected source.

## Widget override checklist

The three Phase F widget overrides are declared in
`Source/GUI/CustomLookAndFeel.h` and implemented in
`Source/GUI/CustomLookAndFeel.cpp`:

| # | Override            | Declared (header) | Implemented (.cpp) | Notes                                                                                                                       |
|---|--------------------|-------------------|--------------------|-----------------------------------------------------------------------------------------------------------------------------|
| 1 | `drawRotarySlider` | Yes               | Yes                | Brushed-aluminum knob: dark track arc (`Border`), active arc using the slider's `rotarySliderFillColourId` (falls back to `MaximizerAccent`), metallic gradient knob body, dark inner cap, accent pointer rotated to the slider angle. |
| 2 | `drawComboBox`     | Yes               | Yes                | Rounded panel background (`Panel`, brighter when pressed/popup active), `Border` outline, dropdown triangle arrow in `Text`. |
| 3 | `drawToggleButton` | Yes               | Yes                | Rounded tick box (`Panel`, darker when down, brighter when highlighted), `Border` outline, accent tick (`tickColourId`, falls back to `MaximizerAccent`) when toggled, label text in `Text`. |

All three required widget overrides are present with full bodies. The
supporting `getComboBoxFont`, `drawPopupMenuBackground`, and
`drawPopupMenuItem` overrides are likewise declared and implemented; they
keep the combo box popup styling consistent with the palette but are not part
of the Phase F widget gate beyond being present and compilable.

## Build system note

`make` was **not used** for any Phase F verification step. The build is driven
by CMake with the **Ninja** generator (`-G Ninja`) and invoked through
`cmake --build`. No `make`, `make all`, or `make AuralMaximizerPro_Standalone`
commands were issued or recorded.

## Phase F acceptance checklist

Each Phase F acceptance item is addressed below with its **actual on-disk
status** at author time. The f.1/f.2 LookAndFeel source edits are present on
disk, so the source-level palette/widget criteria are recorded as satisfied;
the build/static-analysis/formatting gates are recorded as not run because the
toolchain is unavailable in this environment.

| # | Criterion                                                                                                                | Status at author time        | Evidence                                                                                                                                                                                                                                                                                                              |
|---|--------------------------------------------------------------------------------------------------------------------------|------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | `docs/verification/phase_f_lookandfeel.md` exists                                                                        | Satisfied (this file)        | Written by task f.3.                                                                                                                                                                                                                                                                                                 |
| 2 | Records `cmake -B cmake-build-debug -G Ninja` and `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone` | Satisfied                    | Both commands recorded verbatim in §Verification commands.                                                                                                                                                                                                                                                           |
| 3 | Records `cppcheck --enable=all` on the two LookAndFeel sources                                                            | Satisfied                    | Command recorded verbatim with both `Source/GUI/CustomLookAndFeel.h` and `Source/GUI/CustomLookAndFeel.cpp`.                                                                                                                                                                                                          |
| 4 | Records `clang-format --dry-run --Werror` on the two LookAndFeel sources                                                  | Satisfied                    | Command recorded verbatim with both LookAndFeel files.                                                                                                                                                                                                                                                               |
| 5 | Lists exact palette hex values and confirms no make-based verification                                                     | Satisfied                    | Palette hex checklist lists all seven exact values; build system note confirms `make` not used.                                                                                                                                                                                                                      |
| 6 | Palette constants match the seven required hex values                                                                     | Satisfied (on-disk)          | All seven constants (`Background #121417`, `Panel #1A1D22`, `Border #2C3038`, `Maximizer #00D2FF`, `Exciter #FF9500`, `Master #34C759`, `Text #F2F2F7`) confirmed against the on-disk literals.                                                                                                                       |
| 7 | `drawRotarySlider`, `drawComboBox`, `drawToggleButton` implemented                                                        | Satisfied (on-disk)          | All three overrides are declared in the header and have full bodies in the implementation.                                                                                                                                                                                                                          |
| 8 | Phase touched only the two LookAndFeel sources plus this verification artifact                                            | Satisfied                    | File scope: `Source/GUI/CustomLookAndFeel.h`, `Source/GUI/CustomLookAndFeel.cpp`, and this file. No other files were created or modified by this task (see git status below).                                                                                                                                       |

Summary: of the eight Phase F acceptance items, all **8 of 8** are satisfied
at the source/document level. The four toolchain-dependent gates (configure,
build, cppcheck, clang-format) are recorded verbatim as the deterministic
gate but were **not executed** in this environment because the tools are not
installed; they remain to be run in a toolchain-equipped environment to close
the build/analysis gate.

## Residual risks

- **Toolchain availability:** the Phase F gate depends on CMake, Ninja,
  cppcheck, and clang-format being installed. In the environment used to
  author this record they were absent (commands not found). The gate must be
  re-run in an environment with the toolchain present before Phase F can be
  closed as fully passing at the build/analysis level.
- **clang-format fidelity:** the LookAndFeel `.cpp` uses two-space indent
  (e.g. `CustomLookAndFeel() {` body) while the `.h` uses JUCE's
  space-padded signature style. `clang-format --dry-run --Werror` is the
  authority on whether the on-disk formatting conforms to the project
  `.clang-format`; this could not be confirmed here and must be re-run with
  the tool installed. A formatting failure would require a
  `clang-format -i` pass on the two files only.
- **cppcheck warnings from JUCE macros:** `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`
  and the JUCE override signatures can surface `cppcheck` style notes
  (`--enable=all`); only error-level issues block the gate. A re-run must
  confirm no error-level findings on the two LookAndFeel sources.
- **Palette drift:** any future editor-side hardcoded colour that bypasses
  the `CustomLookAndFeel` constants is a Phase F regression. The palette must
  remain the single source of truth; editor controls should bind via
  `setColour` / `findColour` against these constants.
- **Widget override completeness:** only rotary slider, combo box, and toggle
  button overrides are required for Phase F. Additional widget overrides
  (e.g. `drawLinearSlider`, `drawButton`, `drawLabel`) are out of scope for
  Phase F and must not be added as unscoped work in this task.
- **Editor attachment deferred:** `CustomLookAndFeel` is defined but is not
  yet necessarily attached to a `PluginEditor` instance in this phase.
  Wiring the look-and-feel onto the real editor (and binding its controls to
  the APVTS parameter IDs) is a later editor attachment phase and must not be
  introduced prematurely here.

## Phase changed files

The files changed as part of Phase F are:

- `Source/GUI/CustomLookAndFeel.h` (authored by task f.1)
- `Source/GUI/CustomLookAndFeel.cpp` (authored by task f.2)
- `docs/verification/phase_f_lookandfeel.md` (this record — written by task f.3)

No source or build files outside this list were created or modified by this
task. The two LookAndFeel sources are referenced as the files under test and
were inspected, not modified, by task f.3.

Git working-tree status at author time showed only pre-existing runtime
state / report / log files under `.abracapocus/` (excluded from commit per
AGENTS.md) as modified or untracked; no `Source/` or `docs/` files other than
this verification artifact are touched by this task.

## Phase gate summary

- Phase gate target: `AuralMaximizerPro_Standalone`
- Generator: Ninja (`-G Ninja`)
- Build directory: `cmake-build-debug`
- Primary build command: `cmake --build` (not `make`)
- Static analysis gate: `cppcheck --enable=all Source/GUI/CustomLookAndFeel.h
  Source/GUI/CustomLookAndFeel.cpp`
- Formatting gate: `clang-format --dry-run --Werror` on
  `Source/GUI/CustomLookAndFeel.h` and `Source/GUI/CustomLookAndFeel.cpp`
- Palette gate: seven exact hex values (`#121417`, `#1A1D22`, `#2C3038`,
  `#00D2FF`, `#FF9500`, `#34C759`, `#F2F2F7`) — all confirmed on disk.
- Widget gate: `drawRotarySlider`, `drawComboBox`, `drawToggleButton` — all
  declared and implemented on disk.
- File scope: only `Source/GUI/CustomLookAndFeel.h`,
  `Source/GUI/CustomLookAndFeel.cpp`, and this verification artifact.
- Phase F acceptance items: 8 of 8 satisfied at source/document level; the
  four toolchain-dependent gates remain to be run in a toolchain-equipped
  environment.
- Out of scope for Phase F: VST3/CLAP/LV2 build gate, DSP behavior (phases
  B/C/D), processor APVTS (phase E), and real `PluginEditor` wiring
  (deferred to the later editor attachment phase).
