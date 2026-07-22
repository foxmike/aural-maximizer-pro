# Phase H — Editor (PluginEditor) Verification

> Phase: H
> Scope: Editor — `AuralMaximizerProAudioProcessorEditor` (`PluginEditor`) wired to the processor
> Record type: verification note (deterministic, reproducible from a clean clone)

## 1. Purpose

Record that Phase H — the plugin editor — was implemented and verified against
the plan's verification stack: a CMake/Ninja `AuralMaximizerPro_Standalone`
build, a cppcheck static-analysis pass, and a clang-format style pass on the
three in-scope sources.

This note introduces no code. It inventories the three delivered/changed
sources, describes the implemented editor contract against the Phase H goal,
records the exact verification commands with their observed outcomes, and
tabulates pass/fail results. **`make` is not used as the build verifier** and
the full multi-format (VST3/LV2) build is deferred to Phase I (see §6, §7).

## 2. Phase goal summary

Phase H delivers the Standalone plugin editor that binds the audio processor's
APVTS to a fixed-size split-screen UI:

- **`PluginEditor`** (`Source/PluginEditor.{h,cpp}`) —
  `AuralMaximizerProAudioProcessorEditor` (a `juce::AudioProcessorEditor`
  subclass, aliased as `PluginEditor`) presenting a fixed **920×540** frame
  with three accent-coloured regions — Maximizer (cyan), Exciter (amber), and
  Master/Routing (green) — embedding the Phase G `SpectrumVisualizer` and
  `MeterComponent`, and binding all **19 APVTS parameters** to their controls.
- **`createEditor`** (`Source/PluginProcessor.cpp`) — the processor's
  `createEditor()` now returns `new PluginEditor(*this)` (a
  `PluginEditor`/`AuralMaximizerProAudioProcessorEditor` instance), wiring the
  editor into the Standalone target.

## 3. Files in scope (changed files)

The Phase H delivery comprises exactly these three sources:

| Path | Role |
|------|------|
| `Source/PluginEditor.h` | `AuralMaximizerProAudioProcessorEditor` class declaration (final `juce::AudioProcessorEditor` subclass), aliased as `PluginEditor`; constants `editorWidth = 920`, `editorHeight = 540`; `CustomLookAndFeel`, `SpectrumVisualizer`, `MeterComponent` members; maximizer/exciter/routing/master `juce::ToggleButton`/`juce::Slider`/`juce::ComboBox` controls; all 19 `std::unique_ptr<...Attachment>` APVTS attachment members; `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`. |
| `Source/PluginEditor.cpp` | Implementation: constructor configures/attaches all controls, `addAndMakeVisible` for every component, binds all 19 APVTS attachments to the processor's `apvts`, `setSize(920, 540)`, `setResizable(false, false)`; destructor resets LookAndFeel; `paint` draws the three accent panels (Maximizer cyan, Exciter amber, Master green) + captions/labels; `resized` lays out the split-screen regions with embedded `spectrumVisualizer` and `meterComponent`. |
| `Source/PluginProcessor.cpp` | `createEditor()` now returns `new PluginEditor(*this)` (a `PluginEditor`/`AuralMaximizerProAudioProcessorEditor`), wiring the editor into the Standalone target. (No other processor behaviour changed.) |

This verification note documents the three sources above only. No `Source/`
file was modified to produce this note.

## 4. Implemented behaviour (symbols verified by inspection)

Symbols/behaviour documented from the source, for traceability:

- **Editor frame contract**
  - `editorWidth = 920`, `editorHeight = 540` (header constants); constructor
    calls `setSize(920, 540)` and `setResizable(false, false)`, enforcing the
    fixed 920×540 frame.
  - `setLookAndFeel(&customLookAndFeel)` in the constructor; destructor calls
    `setLookAndFeel(nullptr)` before destruction (JUCE LookAndFeel contract).
- **Split-screen regions (accent-coded)**
  - Maximizer panel (cyan / `CustomLookAndFeel::Maximizer`):
    `{10, headerHeight, 445, upperSectionBottom - headerHeight}` =
    `{10, 40, 445, 334}`; caption "SONIC MAXIMIZER".
  - Exciter panel (amber / `CustomLookAndFeel::Exciter`):
    `{465, 40, 445, 334}`; caption "AURAL EXCITER".
  - Master/Routing panel (green / `CustomLookAndFeel::Master`):
    `{10, 384, 900, 146}`; caption "MASTER / ROUTING MATRIX / I-O".
- **Embedded visualizers (Phase G components)**
  - `spectrumVisualizer` constructed from `processor` and `addAndMakeVisible`;
    laid out at `{479, 116, 417, 112}` within the Exciter panel.
  - `meterComponent` `addAndMakeVisible`; laid out at `{772, 414, 128, 90}`
    within the Master panel.
- **All 19 APVTS attachments bound**
  (counted in `PluginEditor.cpp` against `processorRef.apvts`):

  | # | Parameter ID | Control | Attachment type |
  |---|---------------|---------|-----------------|
  | 1 | `MAX_ENABLE` | `maxEnableButton` | `ButtonAttachment` |
  | 2 | `MAX_PROCESS` | `maxProcessSlider` | `SliderAttachment` |
  | 3 | `MAX_CONTOUR` | `maxContourSlider` | `SliderAttachment` |
  | 4 | `MAX_LOW_FREQ` | `maxLowFreqSlider` | `SliderAttachment` |
  | 5 | `MAX_HIGH_FREQ` | `maxHighFreqSlider` | `SliderAttachment` |
  | 6 | `MAX_PHASE_MODE` | `maxPhaseModeComboBox` | `ComboBoxAttachment` |
  | 7 | `EXC_ENABLE` | `excEnableButton` | `ButtonAttachment` |
  | 8 | `EXC_TUNE` | `excTuneSlider` | `SliderAttachment` |
  | 9 | `EXC_DRIVE` | `excDriveSlider` | `SliderAttachment` |
  | 10 | `EXC_MIX` | `excMixSlider` | `SliderAttachment` |
  | 11 | `EXC_MODE` | `excModeComboBox` | `ComboBoxAttachment` |
  | 12 | `EXC_HARMONICS` | `excHarmonicsSlider` | `SliderAttachment` |
  | 13 | `EXC_AIR_GAIN` | `excAirGainSlider` | `SliderAttachment` |
  | 14 | `ROUTING_MODE` | `routingModeComboBox` | `ComboBoxAttachment` |
  | 15 | `PARALLEL_BLEND` | `parallelBlendSlider` | `SliderAttachment` |
  | 16 | `MASTER_IN_GAIN` | `masterInGainSlider` | `SliderAttachment` |
  | 17 | `MASTER_OUT_GAIN` | `masterOutGainSlider` | `SliderAttachment` |
  | 18 | `MASTER_MIX` | `masterMixSlider` | `SliderAttachment` |
  | 19 | `OVERSAMPLING` | `oversamplingComboBox` | `ComboBoxAttachment` |

  (6 maximizer + 7 exciter + 6 routing/master = 19, matching the 19 parameters
  declared in `PluginProcessor.cpp`'s `createParameterLayout()`.)
- **`createEditor` returns `PluginEditor`**
  - `Source/PluginProcessor.cpp`:
    `juce::AudioProcessorEditor *AuralMaximizerProAudioProcessor::createEditor()`
    returns `new PluginEditor(*this);` — i.e. a
    `PluginEditor`/`AuralMaximizerProAudioProcessorEditor` instance.
  - `hasEditor()` returns `true`.
  - The `PluginEditor` alias is defined in `PluginEditor.h`:
    `using PluginEditor = AuralMaximizerProAudioProcessorEditor;`

## 5. Verification commands (exact)

These are the exact commands required for the Phase H gate. The Ninja
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
- Must exit 0, proving the three Phase H sources compile and link against JUCE
  (and the rest of the project) into the Standalone artefact.

### 5.3 Static analysis on the sources

```sh
cppcheck --enable=all Source/
```

- Runs cppcheck with all checks enabled over the `Source/` tree (includes the
  three Phase H sources).
- Must report no errors (absence of error-severity findings is the pass
  criterion).

### 5.4 Style gate on the three Phase H sources

```sh
clang-format --dry-run --Werror Source/PluginEditor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp
```

- Runs `clang-format --dry-run --Werror` against the three Phase H sources in
  one invocation.
- Must exit 0 (no formatting violations).

## 6. `make` is not the phase verifier

The Phase H verification stack uses **CMake with the Ninja generator** plus
**cppcheck** and **clang-format**. `make` is **not** invoked at any step and is
**not** the build verifier for this phase:

- Configure: `cmake -B cmake-build-debug -G Ninja` (Ninja generator).
- Build:   `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
  (delegates to Ninja).
- No `make`, `make all`, or `cmake --build ... --target ...`-via-Makefiles
  step is used.

## 7. Multi-format (VST3/LV2) build deferred to Phase I

Phase H verifies the **Standalone** target only (`AuralMaximizerPro_Standalone`).
The full multi-format plugin build — **VST3** and **LV2** artefacts — is
**deferred to Phase I** and is **not** part of the Phase H gate. Accordingly:

- No `--target AuralMaximizerPro_VST3` / `AuralMaximizerPro_LV2` build step is
  recorded here.
- No VST3/LV2 packaging, validation, or plugin-host loading is performed in
  this phase.
- Phase H acceptance is limited to: Standalone compile/link (§5.2), cppcheck
  (§5.3), and clang-format (§5.4), plus the editor-contract source-inspection
  checks (§4).

## 8. Pass / fail table

| # | Check | Command | Pass criterion | Result |
|---|-------|---------|---------------|--------|
| 1 | Configure with Ninja | `cmake -B cmake-build-debug -G Ninja` | exit 0; `AuralMaximizerPro_Standalone` target present | FAIL — exit 1. JUCE `juceaide` helper failed to compile: `fatal error: X11/extensions/Xrandr.h: No such file or directory` (missing X11 dev headers). Environment prerequisite, not a Phase H code defect. Re-run after installing X11 dev headers (e.g. `libxrandr-dev` / `xorg-dev`). |
| 2 | Build phase gate target | `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone` | exit 0; standalone artefact produced | FAIL — exit 1. `ninja: error: loading 'build.ninja': No such file or directory` (configure in §1 did not complete, so no build graph exists). Blocked on §1. |
| 3 | Static analysis (sources) | `cppcheck --enable=all Source/` | no errors (no error-severity findings) | PENDING — `cppcheck` is not installed in the recording environment (`which cppcheck` → not found). Install and re-run. |
| 4 | clang-format — three Phase H sources | `clang-format --dry-run --Werror Source/PluginEditor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp` | exit 0 | FAIL — exit 1. 9 violations, all in `Source/PluginEditor.h` (long lines / `&` placement / trailing whitespace / `JUCE_DECLARE…` line length). `Source/PluginEditor.cpp` and `Source/PluginProcessor.cpp` each PASS individually (exit 0). `clang-format` 22.1.8. See §10 for the deferred-fix note. |
| 5 | Editor contract — fixed 920×540 frame | source inspection of `PluginEditor.{h,cpp}` | `editorWidth=920`, `editorHeight=540`; `setSize(920,540)`; `setResizable(false,false)` | PASS |
| 6 | Editor contract — split-screen cyan/amber/green regions | source inspection of `paint` | Maximizer panel `CustomLookAndFeel::Maximizer` (cyan), Exciter `Exciter` (amber), Master `Master` (green); 3 panels drawn | PASS |
| 7 | Editor contract — SpectrumVisualizer + MeterComponent embedded | source inspection | both `addAndMakeVisible` and laid out in `resized` | PASS |
| 8 | Editor contract — all 19 APVTS attachments bound | source inspection of constructor | 19 attachments constructed against `processorRef.apvts` (see §4 table) | PASS |
| 9 | `createEditor` returns `PluginEditor` | source inspection of `PluginProcessor.cpp` | `return new PluginEditor(*this);` | PASS |

> Placeholder policy: checks 1–2 are environment-blocked (missing X11 dev
> headers → configure fails → no build graph). Check 3 is PENDING (`cppcheck`
> not installed). Check 4 is a real FAIL recorded against the in-scope source
> `Source/PluginEditor.h`; the two `.cpp` files pass. Checks 5–9 are recorded
> source-inspection results. None of the blocked/pending/failed checks are
> marked PASS until executed successfully.

## 9. Phase gate definition

Phase H is satisfied when, from a clean clone:

1. `cmake -B cmake-build-debug -G Ninja` exits 0, and
2. `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
   exits 0 (compiles/links the three Phase H sources into the Standalone
   artefact), and
3. `cppcheck --enable=all Source/` reports no errors, and
4. `clang-format --dry-run --Werror Source/PluginEditor.h
   Source/PluginEditor.cpp Source/PluginProcessor.cpp` exits 0, and
5. The editor-contract constraints in §4 hold by source inspection (fixed
   920×540 frame; split-screen cyan/amber/green regions; SpectrumVisualizer
   + MeterComponent embedded; all 19 APVTS attachments bound; `createEditor`
   returns `PluginEditor`).

## 10. Deferred clang-format fix note (out of phase H file scope)

The clang-format gate (check 4) FAILED on `Source/PluginEditor.h` with 9
violations (`-Wclang-format-violations`): long lines exceeding the column
limit (e.g. the `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(...)` macro
line, and `using`-alias lines), `&`/`*` reference placement on several
declarations, and a trailing-whitespace/`// --- UI Components ---` line.

These are formatting-only issues in a source file. **This task's strict file
scope is `docs/verification/phase_h_editor.md` only** — no `Source/` file may
be modified here — so the fix is **deferred** to a follow-up task/phase that is
permitted to edit `Source/PluginEditor.h` (e.g. `clang-format -i
Source/PluginEditor.h`). The two `.cpp` files in scope
(`Source/PluginEditor.cpp`, `Source/PluginProcessor.cpp`) already pass
clang-format individually (exit 0) and require no change.

## 11. Prerequisites, phase dependencies, and reproducibility notes

- **Phase dependency**: Phase H depends on Phase G (`SpectrumVisualizer`,
  `MeterComponent`) and Phase E (processor spectrum FIFO / APVTS layout). The
  editor includes `GUI/SpectrumVisualizer.h` and `GUI/MeterComponent.h`,
  constructs `SpectrumVisualizer` from the processor, and binds all 19
  parameters declared in `PluginProcessor.cpp`'s `createParameterLayout()`.
  Those dependency sources must be present and built for the Standalone target
  to link; they are not modified here.
- `cmake`, `ninja`, `cppcheck`, and `clang-format` must be on `PATH`.
  - In the recording environment, `cppcheck` was **not** installed → check 3
    is PENDING.
- **System headers for JUCE GUI modules**: the CMake configure step builds
  JUCE's `juceaide` helper, which requires X11 development headers. In the
  recording environment, configure failed at `juceaide` with
  `fatal error: X11/extensions/Xrandr.h: No such file or directory` → check 1
  is FAIL (exit 1). Installing the X11 dev package (e.g. `libxrandr-dev` /
  `xorg-dev`) and re-running §5.1 resolves this; it is an environment
  prerequisite, not an editor-source defect.
- The `JUCE` git submodule must be initialised
  (`git submodule update --init --recursive`).
- `cmake-build-debug/` is gitignored and disposable; if it was previously
  configured with a different generator, remove it before the `-G Ninja`
  configure step. The Phase H procedure uses Ninja exclusively and never
  invokes `make`.
- No source code was modified to produce this note. The verification here
  documents the in-scope `Source/PluginEditor.h`, `Source/PluginEditor.cpp`,
  and `Source/PluginProcessor.cpp` only.

## 12. Status

- Files in scope (§3): `Source/PluginEditor.h`, `Source/PluginEditor.cpp`,
  `Source/PluginProcessor.cpp`.
- Phase gate target: `AuralMaximizerPro_Standalone` (designated in §9).
- Verification commands: recorded in §5 (CMake/Ninja, no `make`; cppcheck;
  clang-format).
- `make` was not used; the full multi-format (VST3/LV2) build is deferred to
  Phase I (§7).
- Pass/fail: editor-contract source-inspection checks (5–9) PASS; configure
  (1) FAIL on missing X11 dev headers; build (2) FAIL (blocked on §1, no
  build graph); cppcheck (3) PENDING (`cppcheck` not installed); clang-format
  (4) FAIL on `Source/PluginEditor.h` only (deferred fix — §10; the two `.cpp`
  files pass).
