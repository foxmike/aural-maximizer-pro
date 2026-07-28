# Aural Maximizer Pro — Operations Guide

## Purpose
Aural Maximizer Pro is a dual-engine audio processor designed to restore acoustic presence, enhance low-frequency dynamic punch, and synthesize high-frequency harmonic shimmer in real time. It is designed to surround a high-gain amp simulator or mix bus to restore cabinet punch, define pick separation, and add high-end sheen. The plugin delivers dynamic phase-aligned clarity and analog-modeled harmonic excitement on instrument tracks, submixes, and full master chains.

## Signal Flow
1. **Master Input Gain (`MASTER_IN_GAIN`)**: Incoming mono or stereo PCM audio blocks are scaled by a smoothed input gain factor (-24.0 dB to +24.0 dB, 20 ms linear smoothing window).
2. **Dry Buffer Allocation (`dryBuffer`)**: A clean copy of the input-gained signal is written to a dry buffer before any modular DSP processing.
3. **Routing Engine (`ROUTING_MODE`)**: The signal is routed through the Sonic Maximizer and Aural Exciter modules according to one of three selectable topologies:
   - **`Maximizer -> Exciter` (Serial, Default)**: The signal passes through the Sonic Maximizer phase alignment and band expansion first, followed by the Aural Exciter harmonic generation.
   - **`Exciter -> Maximizer` (Serial)**: The signal generates high-frequency harmonics in the Exciter first, which are then phase-aligned and frequency-split by the Maximizer.
   - **`Parallel Matrix`**: The signal is split into independent Maximizer (Path A) and Exciter (Path B) branches. The outputs are blended using the `PARALLEL_BLEND` factor ($K$): $\text{Output} = (1 - K) \cdot \text{Path}_A + K \cdot \text{Path}_B$.
4. **Module Bypass (`MAX_ENABLE`, `EXC_ENABLE`)**: Individual module enable switches selectively bypass module execution. Bypassed modules pass audio transparently.
5. **Master Mix (`MASTER_MIX`)**: The processed output from the routing engine is crossfaded with the unprocessed `dryBuffer`: $\text{Mixed} = (1 - \text{MasterMix}) \cdot \text{DryBuffer} + \text{MasterMix} \cdot \text{Processed}$.
6. **Master Output Gain (`MASTER_OUT_GAIN`)**: The mixed signal is scaled by a smoothed output gain factor (-24.0 dB to +24.0 dB, 20 ms linear smoothing).
7. **Metering & Visualization**: Post-output-gain samples are analyzed for peak/RMS levels and pushed to a lock-free spectrum FIFO.

---

## Controls Reference

Grouped by interface panel sections:

### Sonic Maximizer Panel

#### Maximizer On
- **Parameter ID:** `MAX_ENABLE`
- **Type:** bool
- **Range:** Off (0.0) to On (1.0), default On (1.0)
- **Unit:** none
- **What it does:** Toggles active processing of the `SonicMaximizer` class. When disabled, the Maximizer DSP loop is bypassed and audio passes through it transparently.

#### Process
- **Parameter ID:** `MAX_PROCESS`
- **Type:** float
- **Range:** 0.000 to 1.000, default 0.500
- **Unit:** ratio
- **What it does:** Controls the high-frequency band gain boost ($1.0\times$ to $4.0\times$, +0 dB to +12.04 dB). Reference stage: Sonic Maximizer high-band output.

#### Lo Contour
- **Parameter ID:** `MAX_CONTOUR`
- **Type:** float
- **Range:** 0.000 to 1.000, default 0.500
- **Unit:** ratio
- **What it does:** Scales dynamic gain applied to the low frequency band ($<\text{MAX\_LOW\_FREQ}$) based on an internal 50ms envelope follower, providing up to +6.02 dB dynamic bass expansion. Reference stage: Sonic Maximizer low-band output.

#### Low Freq
- **Parameter ID:** `MAX_LOW_FREQ`
- **Type:** float
- **Range:** 40.0 Hz to 400.0 Hz, default 120.0 Hz (logarithmic skew 0.5)
- **Unit:** Hz
- **What it does:** Sets the crossover boundary between Low and Mid frequency bands. Coefficients update dynamically every 16 samples.

#### High Freq
- **Parameter ID:** `MAX_HIGH_FREQ`
- **Type:** float
- **Range:** 1000.0 Hz to 12000.0 Hz, default 5000.0 Hz (logarithmic skew 0.5)
- **Unit:** Hz
- **What it does:** Sets the crossover boundary between Mid and High frequency bands. Coefficients update dynamically every 16 samples.

#### Phase Mode
- **Parameter ID:** `MAX_PHASE_MODE`
- **Type:** choice
- **Range:** Classic (0), Modern (1), default Classic (0)
- **Unit:** none
- **What it does:** Selects between phase delay topologies. Classic mode applies 4 cascaded 1st-order all-pass filters (APFs) to the low band, 2 APFs to the mid band, and 0 APFs to the high band. Modern mode applies 1 APF to the low band and 0 to the rest.

### Aural Exciter Panel

#### Exciter On
- **Parameter ID:** `EXC_ENABLE`
- **Type:** bool
- **Range:** Off (0.0) to On (1.0), default On (1.0)
- **Unit:** none
- **What it does:** Bypasses or enables the Aural Exciter DSP module inside `RoutingEngine`.

#### Tune
- **Parameter ID:** `EXC_TUNE`
- **Type:** float
- **Range:** 800.0 Hz to 10000.0 Hz, default 3000.0 Hz (logarithmic skew 0.5)
- **Unit:** Hz
- **What it does:** Sets the cutoff frequency of the 2nd-order State Variable TPT high-pass filter feeding the exciter sidechain ($Q = 0.7071$).

#### Drive
- **Parameter ID:** `EXC_DRIVE`
- **Type:** float
- **Range:** 0.0 dB to 24.0 dB, default 6.0 dB
- **Unit:** dB
- **What it does:** Applies linear pre-gain boost to high-passed sidechain samples before entering the oversampled waveshaper.

#### Mix
- **Parameter ID:** `EXC_MIX`
- **Type:** float
- **Range:** 0.000 to 1.000, default 0.500
- **Unit:** ratio
- **What it does:** Controls the parallel wet blend of synthesized harmonics mixed back into the main audio path.

#### Exciter Mode
- **Parameter ID:** `EXC_MODE`
- **Type:** choice
- **Range:** Cubic Soft Clipping (0), Asymmetric Tube (1), Tape (2), default Cubic Soft Clipping (0)
- **Unit:** none
- **What it does:** Selects the active nonlinear waveshaper transfer function applied to the sidechain signal at the oversampled rate.

#### Harmonics
- **Parameter ID:** `EXC_HARMONICS`
- **Type:** float
- **Range:** 0.000 to 1.000, default 0.500
- **Unit:** ratio
- **What it does:** Scales the 2nd-order quadratic term ($\gamma \cdot x^2$) to add even-harmonic richness when `Exciter Mode` is set to `Asymmetric Tube`.

#### Air Gain
- **Parameter ID:** `EXC_AIR_GAIN`
- **Type:** float
- **Range:** 0.0 dB to 12.0 dB, default 0.0 dB
- **Unit:** dB
- **What it does:** Boosts the post-saturation sidechain signal using a 10 kHz corner frequency high-shelf filter ($Q = 0.7071$).

### Master / Routing Matrix Panel

#### Routing Mode
- **Parameter ID:** `ROUTING_MODE`
- **Type:** choice
- **Range:** Maximizer -> Exciter (0), Exciter -> Maximizer (1), Parallel Matrix (2), default Maximizer -> Exciter (0)
- **Unit:** none
- **What it does:** Selects the execution order or parallel configuration of the Maximizer and Exciter modules.

#### Blend
- **Parameter ID:** `PARALLEL_BLEND`
- **Type:** float
- **Range:** 0.000 to 1.000, default 0.500
- **Unit:** ratio
- **What it does:** Crossfades between the Maximizer branch (0.0) and the Exciter branch (1.0) when routing mode is set to Parallel Matrix.

#### Input
- **Parameter ID:** `MASTER_IN_GAIN`
- **Type:** float
- **Range:** -24.0 dB to +24.0 dB, default 0.0 dB
- **Unit:** dB
- **What it does:** Scales incoming signal before internal processing and dry copy allocation.

#### Output
- **Parameter ID:** `MASTER_OUT_GAIN`
- **Type:** float
- **Range:** -24.0 dB to +24.0 dB, default 0.0 dB
- **Unit:** dB
- **What it does:** Scales the final post-mix signal.

#### Mix
- **Parameter ID:** `MASTER_MIX`
- **Type:** float
- **Range:** 0.000 to 1.000, default 1.000
- **Unit:** ratio
- **What it does:** Crossfades globally between the dry input (0.0) and the processed output (1.0).

#### Oversampling
- **Parameter ID:** `OVERSAMPLING`
- **Type:** choice
- **Range:** 2x (0), 4x (1), default 2x (0)
- **Unit:** none
- **What it does:** Selects the oversampling factor (2x or 4x) for the Exciter's nonlinear waveshaper to suppress aliasing.

---

## Metering and Visualization
- **Stereo Level Meters (`MeterComponent`)**: Displays Left/Right Peak and RMS level bars on a -50 dB to +6 dB scale. Visual peak decay rate is 0.92/frame, RMS decay is 0.85/frame, and the peak hold indicator holds the absolute peak for 1 second (30 frames at 30 Hz timer rate) before decaying at 0.95/frame. Driven by post-output-gain samples.
- **FFT Spectrum Visualizer (`SpectrumVisualizer`)**: Real-time 2048-point forward FFT spectrum curve display mapped to a logarithmic frequency curve. Downmixed mono audio frames are pushed to a lock-free spectrum FIFO and rendered on the GUI message thread at 30 Hz (decay release factor 0.82).

## Preset System
There are no factory presets in the code. Save and load behavior relies entirely on host DAW state restoration. The plugin serializes all parameters to XML in `getStateInformation()` and restores them in `setStateInformation()`.

## Latency
- The plugin reports latency caused by the linear-phase FIR half-band oversampling stages in the `AuralExciter`.
- Latency samples are calculated from `juce::dsp::Oversampling::getLatencyInSamples()`, rounded up to the nearest integer, and reported via `setLatencySamples()`.
- Latency changes when switching `OVERSAMPLING` (2x vs 4x). The update occurs in the state but the active DSP oversampler and host PDC reporting are refreshed only when the host DAW triggers a re-prepare (e.g. playback restart) or standalone audio driver reset.

## Supported Formats
- VST3, LV2, Standalone (specified in `CMakeLists.txt`).

## Known Limitations
- **Oversampling Hot-swapping**: Modifying the `OVERSAMPLING` parameter during active playback does not re-initialize the DSP oversampling factor or update reported host latency immediately; a DAW audio driver reset or transport restart is required to trigger `prepareToPlay()`.
- **Tube Mode Harmonics Lock**: The `EXC_HARMONICS` parameter is active only when `EXC_MODE` is set to `Asymmetric Tube (1)`. In Cubic Soft Clipping (0) or Tape (2) modes, the control does nothing.
- **Parallel Blend Serial Bypass**: The `PARALLEL_BLEND` parameter has no effect unless `ROUTING_MODE` is set to `Parallel Matrix (2)`.
- **Crossover Constraints**: The code checks and enforces that `MAX_LOW_FREQ` remains $\le$ `MAX_HIGH_FREQ`.
