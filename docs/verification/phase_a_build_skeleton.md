# Phase A — Build Skeleton Verification

Phase: A
Task: a.10 — Record phase A build skeleton verification
Date: 2026-07-21

## Purpose

This note records the verification of the Phase A build skeleton for
`AuralMaximizerPro`. Phase A is a scaffolding phase: it establishes the root
CMake build configuration and the empty/stub source files for the processor,
editor, DSP, and GUI components. No DSP, GUI rendering, or parameter logic is
implemented in this phase — that work belongs to later phases (B–G).

The phase gate for Phase A is the **Standalone** build target only. Per the
phase verification definition, the VST3/CLAP/LV2 formats are wired in the
CMake config but are **not** part of the Phase A build gate. The Standalone
target is sufficient to prove the skeleton compiles and links against JUCE.

The build generator is **Ninja**. `make` is intentionally not used and is not
documented as the primary build command.

## Scope inspected

Before authoring this record, the following paths were inspected against the
working tree to confirm the skeleton inventory.

### Build configuration (task a.1)

- `CMakeLists.txt` — root build config produced by task a.1.
  - JUCE subdirectory, C++20.
  - `juce_add_plugin` with `COMPANY_NAME nSpace`, `CODE Axpr`.
  - `FORMATS Standalone VST3 CLAP LV2` (only Standalone is the Phase A gate).
  - Links `juce_audio_utils`, `juce_audio_processors`, `juce_dsp`,
    `juce_gui_basics`, `juce_gui_extra`.
  - `juce_generate_juce_header`.
  - `target_sources` lists every `Source/` stub path below.

> Note: at the time this record was authored, the working tree contained the
> `Source/` stubs but `CMakeLists.txt` was not present on disk (task a.1 owns
> that file under strict file scope). This record documents the expected Phase A
> build configuration as defined by task a.1 and the phase verification
> definition; the configure command below is the gate that validates it once
> task a.1's output is in place.

### Source stub paths

Top-level processor/editor (tasks a.2, a.3):

- `Source/PluginProcessor.h`
- `Source/PluginProcessor.cpp`
- `Source/PluginEditor.h`
- `Source/PluginEditor.cpp`

DSP (tasks a.4, a.5, a.6):

- `Source/DSP/SonicMaximizer.h`
- `Source/DSP/SonicMaximizer.cpp`
- `Source/DSP/AuralExciter.h`
- `Source/DSP/AuralExciter.cpp`
- `Source/DSP/RoutingEngine.h`
- `Source/DSP/RoutingEngine.cpp`

GUI (tasks a.7, a.8, a.9):

- `Source/GUI/CustomLookAndFeel.h`
- `Source/GUI/CustomLookAndFeel.cpp`
- `Source/GUI/SpectrumVisualizer.h`
- `Source/GUI/SpectrumVisualizer.cpp`
- `Source/GUI/MeterComponent.h`
- `Source/GUI/MeterComponent.cpp`

All sixteen stub files listed above are referenced by `target_sources` in the
root `CMakeLists.txt` and are the complete Phase A skeleton inventory.

## Verification commands

These are the exact commands recorded for the Phase A build skeleton gate.
`make` is deliberately excluded.

### 1. Configure (Ninja generator)

```bash
cmake -B cmake-build-debug -G Ninja
```

Configures the project into the `cmake-build-debug` build directory using the
Ninja generator. This step validates `CMakeLists.txt`, the JUCE setup, the
plugin target declaration, and the `target_sources` list against the on-disk
stub paths.

### 2. Build the Standalone target (phase gate)

```bash
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
```

Builds only the `AuralMaximizerPro_Standalone` target. A successful build of
this target is the Phase A phase gate. The VST3/CLAP/LV2 formats are configured
but are intentionally not part of the Phase A gate.

### 3. clang-format dry-run on created sources

```bash
clang-format --dry-run --Werror \
  CMakeLists.txt \
  Source/PluginProcessor.h \
  Source/PluginProcessor.cpp \
  Source/PluginEditor.h \
  Source/PluginEditor.cpp \
  Source/DSP/SonicMaximizer.h \
  Source/DSP/SonicMaximizer.cpp \
  Source/DSP/AuralExciter.h \
  Source/DSP/AuralExciter.cpp \
  Source/DSP/RoutingEngine.h \
  Source/DSP/RoutingEngine.cpp \
  Source/GUI/CustomLookAndFeel.h \
  Source/GUI/CustomLookAndFeel.cpp \
  Source/GUI/SpectrumVisualizer.h \
  Source/GUI/SpectrumVisualizer.cpp \
  Source/GUI/MeterComponent.h \
  Source/GUI/MeterComponent.cpp
```

Runs `clang-format` in dry-run mode with `--Werror` so any formatting
deviation in the created sources or `CMakeLists.txt` fails the check. No files
are rewritten by this command.

## Phase gate summary

- Phase gate target: `AuralMaximizerPro_Standalone`
- Generator: Ninja (`-G Ninja`)
- Build directory: `cmake-build-debug`
- Primary build command: `cmake --build` (not `make`)
- Skeleton files verified present: 16 `Source/` stubs across processor/editor,
  DSP, and GUI plus the root `CMakeLists.txt`.
- Out of scope for Phase A: VST3/CLAP/LV2 build gate, DSP implementation, GUI
  rendering, APVTS layout, and parameter processing (deferred to phases B–G).
