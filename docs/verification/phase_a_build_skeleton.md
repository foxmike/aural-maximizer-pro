# Phase A — Build Skeleton Verification

> Phase: A
> Scope: build skeleton only
> Record type: verification note (deterministic, reproducible from a clean clone)

## 1. Purpose

Record that the Phase A build skeletons exist and that the
`AuralMaximizerPro_Standalone` build target is the **phase gate** for Phase A.

This note does not introduce any code. It inventories the skeleton sources and
records the exact verification commands used to confirm the skeleton builds.

## 2. Build system (CMakeLists.txt)

`CMakeLists.txt` at the repository root defines the project:

- `cmake_minimum_required(VERSION 3.22)`
- `project(AuralMaximizerPro VERSION 0.1.0 LANGUAGES CXX)`
- `CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`
- `add_subdirectory(JUCE)` (JUCE is a git submodule, see `.gitmodules`)
- `juce_add_plugin(AuralMaximizerPro COMPANY_NAME nSpace PLUGIN_CODE Axpr FORMATS Standalone VST3 LV2 PRODUCT_NAME "Aural Maximizer Pro")`
- `juce_generate_juce_header(AuralMaximizerPro)`
- `target_sources(AuralMaximizerPro PRIVATE ...)` lists the 16 skeleton files (see §3)
- `target_link_libraries`: `juce::juce_audio_utils`, `juce::juce_audio_processors`, `juce::juce_dsp`, `juce::juce_gui_basics`, `juce::juce_gui_extra`

JUCE derives one target per requested format. Because `Standalone` is listed in
`FORMATS`, the target `AuralMaximizerPro_Standalone` is generated. Building that
target produces a self-contained standalone application and requires no plugin
host, so it is designated the **Phase A phase gate**.

## 3. Skeleton inventory (verified present)

All 16 files below exist on disk and are registered in `target_sources(...)`.
Each is a stub/skeleton: headers declare the class surface; implementations are
empty, no-op, or pass-through. None contain DSP behaviour yet.

| Path | Role | Stub status |
|------|------|-------------|
| `Source/PluginProcessor.h` | `AuralMaximizerProAudioProcessor` (juce::AudioProcessor) declaration | skeleton |
| `Source/PluginProcessor.cpp` | AudioProcessor impl: bus layout, `processBlock` clears extra channels only, empty state save/load, `createPluginFilter()` | skeleton (pass-through) |
| `Source/PluginEditor.h` | `AuralMaximizerProAudioProcessorEditor` declaration | skeleton |
| `Source/PluginEditor.cpp` | Editor impl: 400x300 window, draws title text only | skeleton |
| `Source/DSP/SonicMaximizer.h` | `nSpace::SonicMaximizer` declaration (prepare/process/reset) | skeleton |
| `Source/DSP/SonicMaximizer.cpp` | `prepare`/`process`/`reset` empty, pass-through | skeleton (pass-through) |
| `Source/DSP/AuralExciter.h` | `nSpace::AuralExciter` declaration | skeleton |
| `Source/DSP/AuralExciter.cpp` | `prepare`/`process`/`reset` no-op | skeleton (no-op) |
| `Source/DSP/RoutingEngine.h` | `nSpace::RoutingEngine` declaration | skeleton |
| `Source/DSP/RoutingEngine.cpp` | `prepare`/`process`/`reset` no-op | skeleton (no-op) |
| `Source/GUI/CustomLookAndFeel.h` | `CustomLookAndFeel` (juce::LookAndFeel_V4) declaration | skeleton |
| `Source/GUI/CustomLookAndFeel.cpp` | ctor/dtor empty | skeleton |
| `Source/GUI/SpectrumVisualizer.h` | `SpectrumVisualizer` (juce::Component) declaration | skeleton |
| `Source/GUI/SpectrumVisualizer.cpp` | `paint`/`resized` no-op | skeleton |
| `Source/GUI/MeterComponent.h` | `MeterComponent` (juce::Component) declaration | skeleton |
| `Source/GUI/MeterComponent.cpp` | `paint`/`resized` no-op | skeleton |

Conclusion: the Phase A skeletons exist and are wired into the CMake build graph.

## 4. Verification commands (exact)

These are the exact commands used. The Ninja generator is required; **make is
not used** (do not invoke `make` and do not configure with the `Unix Makefiles`
generator). Run from the repository root:

```sh
cmake -B cmake-build-debug -G Ninja
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
clang-format --dry-run --Werror on created sources
```

### 4.1 Configure

```sh
cmake -B cmake-build-debug -G Ninja
```

- Configures the project into `cmake-build-debug/` using the Ninja generator.
- Must succeed with exit code 0 and emit the `AuralMaximizerPro_Standalone`
  target in the build graph.

### 4.2 Build the phase gate target

```sh
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
```

- Builds only the `AuralMaximizerPro_Standalone` target (the phase gate).
- Must succeed with exit code 0, producing the standalone executable under
  `cmake-build-debug/AuralMaximizerPro_artefacts/Standalone/`.
- A successful build proves the entire skeleton (all 16 files in §3) compiles
  and links against JUCE. This is the Phase A gate criterion.

### 4.3 Style gate on created sources

```sh
clang-format --dry-run --Werror on created sources
```

- Runs `clang-format --dry-run --Werror` against the created sources listed in §3.
- Concrete invocation (header + implementation pairs under `Source/`):

  ```sh
  clang-format --dry-run --Werror \
    Source/PluginProcessor.h   Source/PluginProcessor.cpp \
    Source/PluginEditor.h      Source/PluginEditor.cpp \
    Source/DSP/SonicMaximizer.h   Source/DSP/SonicMaximizer.cpp \
    Source/DSP/AuralExciter.h     Source/DSP/AuralExciter.cpp \
    Source/DSP/RoutingEngine.h    Source/DSP/RoutingEngine.cpp \
    Source/GUI/CustomLookAndFeel.h   Source/GUI/CustomLookAndFeel.cpp \
    Source/GUI/SpectrumVisualizer.h  Source/GUI/SpectrumVisualizer.cpp \
    Source/GUI/MeterComponent.h      Source/GUI/MeterComponent.cpp
  ```

- Must exit 0 (no formatting violations).

## 5. Phase gate definition

Phase A is satisfied when, from a clean clone:

1. `cmake -B cmake-build-debug -G Ninja` exits 0, and
2. `cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone`
   exits 0, and
3. `clang-format --dry-run --Werror` on the created sources exits 0.

The `AuralMaximizerPro_Standalone` target is the phase gate: it is the smallest
self-contained artefact that exercises the full skeleton (processor, editor, DSP
stubs, GUI stubs) through the JUCE build.

## 6. Prerequisites and reproducibility notes

- `cmake`, `ninja`, and `clang-format` must be on `PATH`.
- The `JUCE` git submodule must be initialised (`git submodule update --init --recursive`).
- The build directory `cmake-build-debug/` is gitignored (see `.gitignore`) and is
  a disposable artefact; it can be removed and regenerated.
- Generator switching note: if `cmake-build-debug/` was previously configured with
  a different generator (e.g. `Unix Makefiles`), it must be removed before running
  the `-G Ninja` configure step, because CMake refuses to switch generators in an
  existing build directory. The canonical Phase A procedure uses Ninja exclusively
  and never invokes `make`.

## 7. Status

- Skeleton inventory (§3): verified present by inspection of `CMakeLists.txt` and
  the `Source/` tree.
- Phase gate: `AuralMaximizerPro_Standalone` (designated in §5).
- Verification commands: recorded in §4 (Ninja, no make).
