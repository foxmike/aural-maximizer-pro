# Phase I — Multi-Format Build Targets Verification

Phase: I
Task: i.2 — Record phase I build targets verification
Date: 2026-07-21

## Purpose

This note records the verification evidence for the Phase I multi-format build
gate for `AuralMaximizerPro`. Phase I is the phase that, per the phase
verification definition, is the **only gate for the full four-format matrix**
(`Standalone`, `VST3`, `CLAP`, `LV2`). Phases A–H each gated only the
`AuralMaximizerPro_Standalone` target (plus `cppcheck`/`clang-format` static
gates in later phases) and explicitly deferred the VST3/CLAP/LV2 formats to
phase I; phase I is where that deferral is resolved.

The Phase I build gate is driven by the helper script
`scripts/build_all.sh` (authored by task i.1), which configures with CMake
using the **Ninja** generator and then builds each plugin format target via
`cmake --build`. `make` is intentionally **not used** and is not the Phase I
verifier; only CMake/Ninja are.

## Phase goal summary

Phase I closes the multi-format build loop deferred across phases A–H. The
Phase I goals are:

- **Multi-format build gate:** build all four plugin format targets —
  `AuralMaximizerPro_Standalone`, `AuralMaximizerPro_VST3`,
  `AuralMaximizerPro_CLAP`, `AuralMaximizerPro_LV2` — from a single
  deterministic helper, `scripts/build_all.sh`.
- **Ninja generator, no `make`:** the helper configures with
  `cmake -B cmake-build-debug -G Ninja` and builds each target with
  `cmake --build cmake-build-debug --target <name>`. `make` is never invoked.
- **Per-format attribution:** failures (undefined references, target-discovery
  failures) are reported per format so a failing format is attributable rather
  than collapsed into a single opaque error.
- **CLAP handling:** if CLAP cannot be built (missing CLAP extension / deferred),
  the helper builds `Standalone`+`VST3`+`LV2`, prints a clear pending/skipped
  message for CLAP, and does not treat deferred CLAP as a hard crash.
- **No product-code drift in this task:** task i.2 only records verification;
  it does not implement or modify product code, `CMakeLists.txt`, or
  `scripts/build_all.sh`.

## Scope inspected

Before authoring this record, the following paths were inspected against the
working tree.

### Build helper (task i.1)

- `scripts/build_all.sh` — **not present on disk** at author time. Task i.1
  (run id `c8cb5dda-2606-46a5-b13d-9de07e41f58a`, backend `codex_cli`) was
  blocked by the execution environment before any repo command or file edit
  could run: every shell call and the `apply_patch` file write failed with
  `bwrap: loopback: Failed RTM_NEWADDR: Operation not permitted`. The task
  execution record states "No files were changed." Consequently the Phase I
  build helper does not exist in the working tree, and the `scripts/`
  directory is absent. This record documents that on-disk reality; it does
  not substitute model self-evaluation for tool output and does not fabricate
  a build that did not run.

### Build configuration

- `CMakeLists.txt` — **not present on disk** at author time. (Per the
  `docs/verification/phase_a_build_skeleton.md` note, task a.1 owns the root
  `CMakeLists.txt` under strict file scope, and at the time that record was
  authored `CMakeLists.txt` was likewise not present on disk.) The plugin
  target names `AuralMaximizerPro_Standalone`, `AuralMaximizerPro_VST3`,
  `AuralMaximizerPro_CLAP`, and `AuralMaximizerPro_LV2` are the canonical
  JUCE `juce_add_plugin` target names for a project named `AuralMaximizerPro`
  with `FORMATS Standalone VST3 CLAP LV2`, as recorded in the Phase A
  verification record and reconfirmed by the task i.1 target list. With
  `CMakeLists.txt` absent, the configure step cannot run and CMake cannot
  enumerate its target list, so target-discovery and undefined-reference
  behaviour could not be exercised from this environment.

### Toolchain availability

- `cmake`, `ninja` — **not available** in this verification environment
  (commands not found); only `/usr/bin/make` is present on `PATH` and is
  deliberately **not used** (see the build system note below).

### Prior phase verification docs

- `docs/verification/phase_a_build_skeleton.md` through
  `docs/verification/phase_h_editor.md` — inspected for the existing phase
  verification markdown style and for the consistent record that the
  VST3/CLAP/LV2 formats are configured but deferred to phase I. Phase I
  follows the same structure (purpose, phase goal summary, scope inspected,
  verification commands, pass/fail table, residual risks, gate summary) and
  is the phase that the prior records forward-reference as the multi-format
  gate.

## Verification commands

These are the exact commands recorded for the Phase I multi-format build gate.
`make` is deliberately excluded; the build is driven entirely by
`scripts/build_all.sh` (CMake + Ninja).

### 1. Run the multi-format build helper (phase gate)

```bash
bash scripts/build_all.sh
```

`scripts/build_all.sh` is the Phase I verifier. It is the deterministic,
paste-safe helper authored by task i.1. Internally it performs the configure
step and the four per-format builds:

#### 1a. Configure (Ninja generator)

```bash
cmake -B cmake-build-debug -G Ninja
```

Configures the project into `cmake-build-debug` using the Ninja generator.
This validates `CMakeLists.txt`, the JUCE setup, the `juce_add_plugin`
declaration with `FORMATS Standalone VST3 CLAP LV2`, and the `target_sources`
list against the on-disk sources.

#### 1b. Build each format target (per-format, attributable)

```bash
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
cmake --build cmake-build-debug --target AuralMaximizerPro_VST3
cmake --build cmake-build-debug --target AuralMaximizerPro_CLAP
cmake --build cmake-build-debug --target AuralMaximizerPro_LV2
```

Each format is built as its own explicit `cmake --build` invocation so that a
failure (undefined reference, missing target) is attributable to a single
format. The helper reports undefined references and target-discovery failures
clearly to stdout/stderr and, per task i.1's contract, does **not** use
`set -e` or `exit 0`, so a deferred CLAP does not collapse the whole run.

## Pass / fail table

| # | Check                                                                                          | Tool              | Pass condition                                                                                                        | Outcome                                                                                                                       |
|---|------------------------------------------------------------------------------------------------|-------------------|-----------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------|
| 1 | Run `scripts/build_all.sh` (configure + multi-format build)                                    | `bash`/`cmake`/`ninja` | `bash scripts/build_all.sh` exits 0 after building all four targets                                                   | Not run in this environment (`scripts/build_all.sh` not present on disk; task i.1 was blocked by the sandbox, see §Scope inspected). |
| 2 | Configure with Ninja generator                                                                | `cmake`           | `cmake -B cmake-build-debug -G Ninja` exits 0                                                                         | Not run in this environment (`CMakeLists.txt` absent; `cmake` not available).                                                  |
| 3 | Build `AuralMaximizerPro_Standalone` target                                                    | `cmake`/`ninja`   | `cmake --build ... --target AuralMaximizerPro_Standalone` exits 0, produces artifact                                   | Not run in this environment (helper + `CMakeLists.txt` + `cmake`/`ninja` absent).                                              |
| 4 | Build `AuralMaximizerPro_VST3` target                                                          | `cmake`/`ninja`   | `cmake --build ... --target AuralMaximizerPro_VST3` exits 0, produces `.vst3` artifact                                 | Not run in this environment (helper + `CMakeLists.txt` + `cmake`/`ninja` absent).                                              |
| 5 | Build `AuralMaximizerPro_CLAP` target                                                          | `cmake`/`ninja`   | `cmake --build ... --target AuralMaximizerPro_CLAP` exits 0, produces `.clap` artifact                                 | **Pending** — CLAP is the deferred/pending format (see §CLAP status). Not built; no undefined-reference or target-discovery data could be collected. |
| 6 | Build `AuralMaximizerPro_LV2` target                                                           | `cmake`/`ninja`   | `cmake --build ... --target AuralMaximizerPro_LV2` exits 0, produces `.lv2` bundle                                    | Not run in this environment (helper + `CMakeLists.txt` + `cmake`/`ninja` absent).                                              |

The verification commands above are the recorded Phase I gate. Execution was
**not possible** in this verification environment because the Phase I build
helper `scripts/build_all.sh` is not present on disk: task i.1, which owns
that file, was blocked by the execution environment (bubblewrap sandbox:
`bwrap: loopback: Failed RTM_NEWADDR: Operation not permitted`) before it
could create the file, and left no files changed. In addition,
`CMakeLists.txt` is not present on disk, and the `cmake`/`ninja` toolchain is
not installed (only `/usr/bin/make` is present, and it is deliberately not
used). The commands are recorded verbatim above as the deterministic,
reproducible Phase I gate: they are runnable from a clean clone on any
machine with the standard C++ toolchain (CMake ≥ 3.x, Ninja) installed, once
task i.1's `scripts/build_all.sh` and task a.1's `CMakeLists.txt` are in
place.

This record does not substitute model self-evaluation for tool output. Where
the helper and toolchain are available, the pass condition is:

- `bash scripts/build_all.sh` runs the configure step
  (`cmake -B cmake-build-debug -G Ninja`) and the four per-format builds
  (`cmake --build cmake-build-debug --target AuralMaximizerPro_{Standalone,VST3,CLAP,LV2}`)
  and exits 0 with all artifacts produced (or with CLAP clearly marked
  pending/skipped per the CLAP deferral contract).
- `cmake -B cmake-build-debug -G Ninja` exits 0.
- `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
  exits 0 and produces the Standalone artifact.
- `cmake --build cmake-build-debug --target AuralMaximizerPro_VST3` exits 0
  and produces the `.vst3` artifact.
- `cmake --build cmake-build-debug --target AuralMaximizerPro_CLAP` exits 0
  and produces the `.clap` artifact — or, if CLAP is deferred, the helper
  builds `Standalone`+`VST3`+`LV2` and prints a clear pending message for
  CLAP without crashing.
- `cmake --build cmake-build-debug --target AuralMaximizerPro_LV2` exits 0
  and produces the `.lv2` bundle.

## Undefined references and target-discovery failures

Task i.2 records the following for the Phase I gate:

- **Undefined references:** none were observed, because no build ran. The
  per-format build output that would surface linker-level undefined
  references could not be produced in this environment (helper, build
  config, and toolchain all absent). No undefined references are claimed or
  ruled out by this record; they must be re-assessed once
  `scripts/build_all.sh`, `CMakeLists.txt`, and the `cmake`/`ninja`
  toolchain are present and the gate is run.
- **Target-discovery failures:** none were observed, because CMake's target
  enumeration (`cmake --build ... --target <name>` resolving the named
  target) could not run without `CMakeLists.txt` and `cmake`. The canonical
  JUCE target names `AuralMaximizerPro_Standalone`,
  `AuralMaximizerPro_VST3`, `AuralMaximizerPro_CLAP`, and
  `AuralMaximizerPro_LV2` are the names the helper is contracted to build;
  whether CMake exposes all four as resolvable targets is itself part of the
  Phase I gate and remains to be confirmed once the configure step can run.
  A target-discovery failure for any format would be reported by
  `scripts/build_all.sh` to stdout/stderr (per task i.1's contract) and is
  attributable per format because the helper builds each target explicitly.

## CLAP status

CLAP is the **deferred/pending** format of the four-format matrix. Per the
task i.1 contract, `scripts/build_all.sh` is required to handle a missing
CLAP extension / deferred CLAP by building `Standalone`+`VST3`+`LV2`,
printing a clear pending/skipped message for CLAP, and not treating deferred
CLAP as a hard crash solely through `set -e`.

At author time the **Standalone + VST3 + LV2** status could not be
established either, because the Phase I gate did not run: `scripts/build_all.sh`
is absent (task i.1 blocked), `CMakeLists.txt` is absent, and the
`cmake`/`ninja` toolchain is not installed. Accordingly:

- `AuralMaximizerPro_Standalone`: **not built** (gate did not run; no pass/fail
  data). Status: pending the Phase I gate being runnable.
- `AuralMaximizerPro_VST3`: **not built** (gate did not run; no pass/fail
  data). Status: pending the Phase I gate being runnable.
- `AuralMaximizerPro_LV2`: **not built** (gate did not run; no pass/fail
  data). Status: pending the Phase I gate being runnable.
- `AuralMaximizerPro_CLAP`: **pending** (explicitly marked). CLAP is the
  deferred format of the matrix; even when the gate runs, CLAP may remain
  pending if the CLAP extension is unavailable, in which case the helper
  builds `Standalone`+`VST3`+`LV2` and emits a clear pending message for
  CLAP.

Because the gate did not run, the Standalone+VST3+LV2 trio is recorded as
**not yet established** rather than passing. Once `scripts/build_all.sh`,
`CMakeLists.txt`, and the toolchain are in place, the trio must be built and
verified before CLAP's pending status is the only outstanding item.

## Build system note

`make` was **not used** for any Phase I verification step and is **not the
Phase I verifier**. The Phase I build is driven by `scripts/build_all.sh`,
which configures with CMake using the **Ninja** generator
(`cmake -B cmake-build-debug -G Ninja`) and builds each format via
`cmake --build cmake-build-debug --target <name>`. No `make`, `make all`, or
`make <target>` commands were issued or recorded. Only `/usr/bin/make` is
present in this verification environment, and it is deliberately **not**
used; the absence of `cmake`/`ninja` is recorded as the blocker, not
worked around by falling back to `make`.

## Phase I as the sole gate for the four-format matrix

Phases A–H each built and gated only the `AuralMaximizerPro_Standalone`
target and explicitly deferred the VST3/CLAP/LV2 formats to phase I (see
`docs/verification/phase_a_build_skeleton.md` §Phase gate summary and
`docs/verification/phase_h_editor.md` §Multi-format build deferral). Phase I
is the **only** phase that gates the full four-format matrix
(`Standalone` + `VST3` + `CLAP` + `LV2`) via `scripts/build_all.sh`. Until
the Phase I gate runs green (with CLAP either built or explicitly marked
pending while Standalone+VST3+LV2 build), the plugin is not considered
shippable across the full format matrix, and the VST3/CLAP/LV2 wrapping
code paths remain unverified at the build level.

## Phase I acceptance checklist

Each Phase I acceptance item from the task definition is addressed below
with its **actual on-disk status** at author time.

| # | Criterion                                                                                                                                                | Status at author time        | Evidence                                                                                                                                                                                                                                                                                                                |
|---|----------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | `docs/verification/phase_i_build_targets.md` exists                                                                                                     | Satisfied (this file)        | Written by task i.2.                                                                                                                                                                                                                                                                                                       |
| 2 | `docs/verification/phase_i_build_targets.md` references `scripts/build_all.sh` and `cmake -B cmake-build-debug -G Ninja`                                  | Satisfied                    | §Verification commands records `bash scripts/build_all.sh` as the phase gate and `cmake -B cmake-build-debug -G Ninja` as the configure step it performs.                                                                                                                                                                |
| 3 | `docs/verification/phase_i_build_targets.md` records build outcome for Standalone, VST3, CLAP, and LV2 targets                                           | Satisfied (recorded)         | §Pass / fail table records an outcome row for each of `AuralMaximizerPro_Standalone`, `AuralMaximizerPro_VST3`, `AuralMaximizerPro_CLAP`, `AuralMaximizerPro_LV2`; §CLAP status records the per-format status of all four.                                                                                                 |
| 4 | `docs/verification/phase_i_build_targets.md` states verification does not use `make`                                                                     | Satisfied                    | §Build system note states `make` was not used and is not the Phase I verifier; only CMake/Ninja are.                                                                                                                                                                                                                       |
| 5 | If CLAP is pending, `docs/verification/phase_i_build_targets.md` marks CLAP pending and records Standalone+VST3+LV2                                      | Satisfied                    | §CLAP status marks `AuralMaximizerPro_CLAP` as **pending** explicitly and records the Standalone+VST3+LV2 trio (as not yet established, since the gate did not run).                                                                                                                                                       |
| 6 | No file outside the task's `allowed_write_paths` is created or modified                                                                                  | Satisfied                    | File scope: only `docs/verification/phase_i_build_targets.md` (see §Phase changed files). No `Source/`, `CMakeLists.txt`, or `scripts/` files were created or modified by task i.2.                                                                                                                                       |

Summary: of the six Phase I acceptance items, all **6 of 6** are satisfied at
the document/scope level. The four format-target build outcomes and the
configure step are recorded verbatim as the deterministic Phase I gate but
were **not executed** in this environment because `scripts/build_all.sh` is
absent (task i.1 blocked), `CMakeLists.txt` is absent, and the
`cmake`/`ninja` toolchain is not installed. They remain to be run in a
toolchain-equipped environment with task i.1's helper and task a.1's
`CMakeLists.txt` in place before Phase I can be closed as fully passing at
the build level.

## Residual risks

- **`scripts/build_all.sh` absent:** task i.1, which owns the Phase I build
  helper, was blocked by the execution environment (bubblewrap sandbox
  loopback error) and left no files changed. Until `scripts/build_all.sh`
  is authored, the Phase I gate cannot run. This is the primary residual
  Phase I blocker and is upstream of every format-target outcome below.
- **`CMakeLists.txt` absent:** the root build config owned by task a.1 is
  not present on disk (consistent with the Phase A verification record's
  note). The configure step cannot run without it, and CMake cannot enumerate
  the four format targets, so target-discovery behaviour is unverified.
- **Toolchain availability:** the Phase I gate depends on CMake and Ninja
  being installed. In the environment used to author this record they were
  absent (commands not found; only `/usr/bin/make` was present, deliberately
  not used). The gate must be re-run in an environment with the toolchain
  present.
- **CLAP pending may persist even after the gate runs:** even once
  `scripts/build_all.sh`, `CMakeLists.txt`, and the toolchain are in place,
  CLAP may remain pending if the CLAP extension is unavailable. The helper
  is contracted to build Standalone+VST3+LV2 and emit a clear pending
  message for CLAP in that case; that path must be confirmed when the gate
  runs.
- **Per-format attribution depends on the helper's contract:** the
  undefined-reference and target-discovery reporting recorded in §Undefined
  references and target-discovery failures is the contract from task i.1,
  not an observed output. Until the helper exists and runs, its
  stdout/stderr reporting cannot be confirmed.
- **Multi-format wrapping code paths unverified:** with the full
  four-format build not yet run, the VST3/CLAP/LV2 plugin-format wrapping
  code paths (loader entry points, format-specific manifest/bundle
  generation) remain unverified at the build level. Standalone-only gates in
  phases A–H did not exercise these paths.

## Phase changed files

The files changed as part of Phase I (this task, i.2) are:

- `docs/verification/phase_i_build_targets.md` (this record — written by task i.2)

The file **intended** to be the Phase I build deliverable (owned by task i.1,
not by this task) is:

- `scripts/build_all.sh`

`scripts/build_all.sh` is referenced as the Phase I verifier and was
**inspected for (and found absent)**, not created or modified, by task i.2.
No `Source/`, `CMakeLists.txt`, or other `scripts/` files were created or
modified by this task.

Git working-tree status at author time: only `docs/verification/phase_i_build_targets.md`
is written by this task; the only other modified/untracked paths are
pre-existing runtime state / report / log files under `.abracapocus/`
(excluded from commit per AGENTS.md).

## Phase gate summary

- Phase gate verifier: `scripts/build_all.sh` (authored by task i.1)
- Configure command: `cmake -B cmake-build-debug -G Ninja`
- Build commands: `cmake --build cmake-build-debug --target AuralMaximizerPro_{Standalone,VST3,CLAP,LV2}`
- Generator: Ninja (`-G Ninja`)
- Build directory: `cmake-build-debug`
- Primary build command: `cmake --build` (not `make`)
- `make` is **not** the Phase I verifier; only CMake/Ninja are.
- Format targets gated: `AuralMaximizerPro_Standalone`, `AuralMaximizerPro_VST3`,
  `AuralMaximizerPro_CLAP`, `AuralMaximizerPro_LV2`.
- CLAP status: **pending** (deferred format of the matrix; helper builds
  Standalone+VST3+LV2 and emits a clear pending message for CLAP).
- Standalone+VST3+LV2 status: **not yet established** (the Phase I gate did
  not run; helper, `CMakeLists.txt`, and toolchain absent).
- Undefined references / target-discovery failures: none observed; not
  assessable until the gate runs (per-format attribution depends on
  `scripts/build_all.sh` stdout/stderr).
- Phase I is the **only** gate for the full four-format matrix; phases A–H
  gated Standalone only and deferred VST3/CLAP/LV2 to here.
- File scope: only `docs/verification/phase_i_build_targets.md` is written
  by this task; `scripts/build_all.sh` is inspected (absent), not modified.
- Phase I acceptance items: 6 of 6 satisfied at the document/scope level.
  The four format-target build outcomes plus the configure step remain to
  be run in a toolchain-equipped environment with task i.1's helper and
  task a.1's `CMakeLists.txt` in place.
- Out of scope for Phase I: DSP behaviour (phases B/C/D), processor
  APVTS/SPSC FIFO (phase E), LookAndFeel palette (phase F), GUI component
  internals (phase G), editor contract implementation (phase H).
