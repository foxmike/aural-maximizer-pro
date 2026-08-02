# Aural Maximizer Pro — Operations Manual

## Plugin Overview
Aural Maximizer Pro (plugin code `Axpr`, manufacturer `nSpace`, version `0.1.0`) is a dual-engine audio processor designed to restore acoustic presence, enhance low-frequency dynamic punch, and synthesize high-frequency harmonic shimmer in real time. It combines a phase-aligning 3-band **Sonic Maximizer** and an oversampled nonlinear **Aural Exciter** with post-saturation air filter control.

## Intended Use
The plugin is designed to surround high-gain amp simulators, mix buses, instrument tracks, submixes, or mastering chains to restore speaker cabinet punch, define pick separation, and add high-end sheen.

## Signal Flow
Audio flows sequentially through input gain staging, clean dry buffering, module routing, master dry/wet matrix blending, output gain staging, and thread-safe visualization metering:

1. **Master Input Gain (`MASTER_IN_GAIN`)**: Incoming PCM audio blocks are scaled by a smoothed input gain factor (-24.0 dB to +24.0 dB, 20 ms linear smoothing window).
2. **Dry Buffer Allocation (`dryBuffer`)**: An exact copy of the input-gained signal is written to `dryBuffer` prior to any DSP module processing.
3. **Routing Engine (`ROUTING_MODE`)**: The gained signal enters `RoutingEngine::process()`, which directs audio into the Sonic Maximizer and/or Aural Exciter modules according to the selected topology:
   - *Maximizer -> Exciter (Serial)*: Signal passes through Sonic Maximizer phase alignment and band expansion first, followed by Aural Exciter harmonic generation.
   - *Exciter -> Maximizer (Serial)*: Signal generates high-frequency harmonics in the Exciter first, followed by the Maximizer's phase alignment and frequency splitting.
   - *Parallel Matrix*: Signal is split into independent Maximizer (Path A) and Exciter (Path B) branches. The outputs are blended using `PARALLEL_BLEND` ($K$):
     $$\text{Output} = (1 - K) \cdot \text{Path}_A + K \cdot \text{Path}_B$$
4. **Module Bypass (`MAX_ENABLE`, `EXC_ENABLE`)**: Individual module enable switches selectively bypass module execution. Disabling a module in serial modes acts as a transparent wire; in parallel mode, it leaves the respective branch scratch buffer un-processed (passing dry input).
5. **Master Mix (`MASTER_MIX`)**: The processed output from the routing engine is crossfaced with the unprocessed `dryBuffer`:
   $$\text{Mixed} = (1 - \text{MasterMix}) \cdot \text{DryBuffer} + \text{MasterMix} \cdot \text{Processed}$$
6. **Master Output Gain (`MASTER_OUT_GAIN`)**: The mixed signal is scaled by a smoothed output gain factor (-24.0 dB to +24.0 dB, 20 ms linear smoothing).
7. **Metering & Visualization**: Post-output-gain samples are analyzed for peak/RMS levels and pushed to a lock-free spectrum FIFO for real-time visualization.

### Signal-Flow Diagram

```
                 [ Audio Input (Mono/Stereo) ]
                                |
                    [ Master Input Gain (dB) ]
                                |
               +----------------+----------------+
               |                                 |
        [ Dry Buffer ]                 [ Routing Engine ]
               |                                 |
               |       +-------------------------+-------------------------+
               |       |                                                   |
               |   (Serial Mode 1)                 (Serial Mode 2)     (Parallel Mode 3)
               |   Maximizer -> Exciter            Exciter -> Maximizer  Max || Exc (Blend K)
               |       |                                   |                       |
               |       +-------------------------+---------+                       |
               |                                 |                                 |
               |                     [ Routing Output Block ]                      |
               |                                 |                                 |
               +------------------------> [ Master Mix ] <-------------------------+
                                                 |
                                     [ Master Output Gain (dB) ]
                                                 |
                                    +------------+------------+
                                    |                         |
                           [ L/R Peak/RMS Meters ]    [ FFT Spectrum FIFO ]
                                    |                         |
                                 [ Output ]              [ GUI Render ]
```

## Processing Architecture

### Sonic Maximizer
Audio is divided into three bands using 2nd-order IIR filters:
- **Low Band**: $< \text{MAX\_LOW\_FREQ}$ (2 cascaded low-pass filters)
- **Mid Band**: $\text{MAX\_LOW\_FREQ} \le f \le \text{MAX\_HIGH\_FREQ}$ (2 cascaded HPFs at low cut + 2 cascaded LPFs at high cut)
- **High Band**: $> \text{MAX\_HIGH\_FREQ}$ (2 cascaded high-pass filters)

Phase alignment is controlled by `MAX_PHASE_MODE`:
- **Classic Mode**: Applies 4 cascaded 1st-order all-pass filters (APFs) to the low band, 2 APFs to the mid band, and 0 APFs to the high band (all-pass frequency equals `MAX_LOW_FREQ`). This causes high frequencies to lead, mid frequencies to follow, and low frequencies to arrive last, aligning with standard speaker dispersion characteristics.
- **Modern Mode**: Applies 1 APF to the low band and 0 APFs to the rest, reducing total phase rotation for punchier bass.
Phase mode changes crossfade smoothly over 20 ms.

### Aural Exciter
Synthesizes musically related high-frequency harmonics not present in the original input:
1. **High-Pass Sidechain**: A 2nd-order State Variable TPT high-pass filter isolates audio above `EXC_TUNE` (800 Hz to 10 kHz, $Q = 0.7071$).
2. **Drive & Waveshaping**: The sidechain is boosted by `EXC_DRIVE` (0 to 24 dB) and passed through the waveshaper at the oversampled rate:
   - *Cubic Soft Clipping*: Piecewise polynomial function limiting output smoothly between -1.0 and +1.0. Generates odd harmonics.
   - *Asymmetric Tube*: Applies an asymmetric `tanh` curve; `HARMONICS` blends in the even-harmonic component for tube-style warmth.
   - *Tape*: $\frac{2}{\pi}\arctan(x)$. Smooth tape-style soft saturation.
3. **Oversampling & Anti-Aliasing**: Waveshaping occurs inside an FIR oversampling block (2x or 4x) using linear-phase FIR half-band filters.
4. **Air Shelf Filter**: Post-saturation sidechain passes through a 10 kHz high-shelf filter boosted by `EXC_AIR_GAIN` (0 to 12 dB, $Q = 0.7071$).
5. **Parallel Wet Mix**: Sidechain output is mixed into main audio scaled by `EXC_MIX`.

## Controls and Parameters

| Section | Interface Label | Parameter ID | Control Type | Valid Range / Choices | Default | Units | Technical Summary & DSP Stage |
|:---|:---|:---|:---|:---|:---|:---|:---|
| **Maximizer** | MAXIMIZER ON | `MAX_ENABLE` | Toggle Button | Off (0.0), On (1.0) | On (1.0) | — | Enables/bypasses Sonic Maximizer module |
| **Maximizer** | PROCESS | `MAX_PROCESS` | Rotary Slider | 0.000 to 1.000 (linear) | 0.500 | Ratio | High-band expansion gain boost ($1.0\times$ to $4.0\times$, +0 to +12.04 dB) |
| **Maximizer** | LO CONTOUR | `MAX_CONTOUR` | Rotary Slider | 0.000 to 1.000 (linear) | 0.500 | Ratio | Low-band envelope follower ($\tau = 50\text{ ms}$) dynamic gain (up to +6.02 dB) |
| **Maximizer** | LOW FREQ | `MAX_LOW_FREQ` | Rotary Slider | 40.0 to 400.0 (skew 0.5) | 120.0 | Hz | Low/Mid crossover frequency. The crossover design ensures that the Low Crossover frequency remains below or equal to the High Crossover frequency. |
| **Maximizer** | HIGH FREQ | `MAX_HIGH_FREQ` | Rotary Slider | 1000.0 to 12000.0 (skew 0.5) | 5000.0 | Hz | Mid/High crossover frequency. The crossover design ensures that the Low Crossover frequency remains below or equal to the High Crossover frequency. |
| **Maximizer** | Phase Mode | `MAX_PHASE_MODE` | Choice Box | Classic (0), Modern (1) | Classic (0) | Choice | All-pass filter phase delay topology |
| **Exciter** | EXCITER ON | `EXC_ENABLE` | Toggle Button | Off (0.0), On (1.0) | On (1.0) | — | Enables/bypasses Aural Exciter module |
| **Exciter** | TUNE | `EXC_TUNE` | Rotary Slider | 800.0 to 10000.0 (skew 0.5) | 3000.0 | Hz | 2nd-order TPT high-pass sidechain cutoff ($Q=0.7071$) |
| **Exciter** | DRIVE | `EXC_DRIVE` | Rotary Slider | 0.0 to 24.0 (step 0.1) | 6.0 | dB | Sidechain pre-saturation linear drive gain |
| **Exciter** | MIX | `EXC_MIX` | Rotary Slider | 0.000 to 1.000 | 0.500 | Ratio | Parallel blend of excited sidechain into audio |
| **Exciter** | Exciter Mode | `EXC_MODE` | Choice Box | Cubic Soft Clipping (0), Asymmetric Tube (1), Tape (2) | Cubic (0) | Choice | Nonlinear waveshaper transfer function |
| **Exciter** | HARMONICS | `EXC_HARMONICS` | Rotary Slider | 0.000 to 1.000 | 0.500 | Ratio | Blends the selected waveshaper output toward the even-harmonic component. In Asymmetric Tube mode, this controls tube-style even-harmonic richness. |
| **Exciter** | AIR GAIN | `EXC_AIR_GAIN` | Rotary Slider | 0.0 to 12.0 (step 0.1) | 0.0 | dB | Post-saturation 10 kHz high-shelf filter gain ($Q=0.7071$) |
| **Master** | Routing Mode | `ROUTING_MODE` | Choice Box | Maximizer -> Exciter (0), Exciter -> Maximizer (1), Parallel Matrix (2) | Max->Exc (0) | Choice | Signal routing order & matrix topology |
| **Master** | BLEND | `PARALLEL_BLEND` | Rotary Slider | 0.000 to 1.000 | 0.500 | Ratio | Crossfades between the Maximizer and Exciter branches when Routing Mode is configured to Parallel Matrix. |
| **Master** | INPUT | `MASTER_IN_GAIN` | Rotary Slider | -24.0 to +24.0 (step 0.1) | 0.0 | dB | Pre-DSP global input gain (20 ms smoothing) |
| **Master** | OUTPUT | `MASTER_OUT_GAIN` | Rotary Slider | -24.0 to +24.0 (step 0.1) | 0.0 | dB | Post-mix global output gain (20 ms smoothing) |
| **Master** | MIX | `MASTER_MIX` | Rotary Slider | 0.000 to 1.000 | 1.000 | Ratio | Global dry/wet mix ratio |
| **Master** | Oversampling | `OVERSAMPLING` | Choice Box | 2x (0), 4x (1) | 2x (0) | Choice | FIR oversampling factor for Exciter waveshaper. Changes apply upon host transport restart or audio driver prepare. |

## Stereo and Mono Behavior
Left and Right channels are processed independently with separate per-channel envelope followers in the Sonic Maximizer and independent sidechain state paths in the Aural Exciter. Shared parameters are smoothed concurrently across channels. Channel layouts supported are strictly 1:1 (Mono-in/Mono-out) and 2:2 (Stereo-in/Stereo-out).

## Latency and Lookahead (PDC)
The plugin reports latency introduced by the linear-phase FIR half-band oversampling stages in the `AuralExciter`. Latency samples are calculated from `juce::dsp::Oversampling::getLatencyInSamples()`, rounded up, and reported via `setLatencySamples()`.
- Latency updates occur when switching the oversampling factor (2x vs 4x), with changes taking effect when the plugin is re-prepared by the host.

## Bypass Behavior
- **Module Bypass (`MAX_ENABLE` / `EXC_ENABLE`)**: Silences or bypasses specific internal DSP engines while keeping the rest of the routing matrix active. Disabling `MAX_ENABLE` in Serial mode passes the input signal directly to the Exciter.
- **Master Dry/Wet (`MASTER_MIX`)**: Performs a global blend between the completely unprocessed input signal (after Input Gain) and the output of the full routing topology. When set to `0.0`, all internal processing is bypassed.

## Presets and State Recall
There are no factory presets compiled in the code. Save and load behavior relies entirely on host DAW state restoration. The plugin serializes all parameters to XML in `getStateInformation()` and restores them in `setStateInformation()`.

## Automation Behavior
- All 19 parameters are registered with APVTS and fully automatable in DAWs.
- Crossover frequencies (`MAX_LOW_FREQ` and `MAX_HIGH_FREQ`) update filter coefficients dynamically every 16 samples using a 50 ms smoothing window.

## Mode and Routing Dependencies
- Enable `EXC_ENABLE` to hear Tune, Drive, Mix, Mode, Harmonics, and Air Gain changes.
- `EXC_HARMONICS` applies to the exciter waveshaper; use Asymmetric Tube mode for its intended tube-style even-harmonic emphasis.
- `PARALLEL_BLEND` controls the balance between the Maximizer and Exciter branches only when Routing Mode is set to Parallel Matrix. In either serial routing mode, the selected processing order determines the result and Parallel Blend is not part of the signal path.
- In Parallel Matrix routing, disabling a module leaves that branch as the input signal; Parallel Blend still crossfades the two branch outputs.
- Oversampling applies to the Exciter waveshaper. A changed oversampling selection becomes active when the host next re-prepares the plugin, such as after a transport restart or audio-engine reset.

## Metering and Visual Feedback
- **Stereo Level Meters (`MeterComponent`)**: Displays Left/Right Peak and RMS level bars on a -50 dB to +6 dB scale. Visual peak decay rate is 0.92/frame, RMS decay is 0.85/frame, and the peak hold indicator holds the absolute peak for 1 second (30 frames at 30 Hz timer rate) before decaying at 0.95/frame.
- **FFT Spectrum Visualizer (`SpectrumVisualizer`)**: Real-time 2048-point forward FFT spectrum curve display mapped to a logarithmic frequency curve. Downmixed mono audio frames are pushed to a lock-free spectrum FIFO and rendered on the GUI message thread at 30 Hz (Hann windowed, decay release factor 0.82). The OUTPUT SPECTRUM uses a slowly adapting global display range so quieter and louder sources remain readable while relative spectral balance, resonant peaks, rolloff, and meaningful output-level changes remain visible. Its subtle `AUTO` label shows the current top-of-graph dB reference.

## Recommended Workflows

### 1. Full Stereo Mix Enhancement
1. Set `ROUTING_MODE` to `Maximizer -> Exciter`.
2. Enable both `MAX_ENABLE` and `EXC_ENABLE`.
3. Set `MAX_LOW_FREQ` to 100 Hz and `MAX_HIGH_FREQ` to 5000 Hz.
4. Slowly increase `MAX_PROCESS` to 0.40 and `MAX_CONTOUR` to 0.35.
5. Set `EXC_TUNE` to 4000 Hz, `EXC_MODE` to `Tape`, and bring `EXC_MIX` to 0.25.
6. Adjust `MASTER_OUT_GAIN` to level-match output against bypassed signal.

### 2. Processing Guitar or NAM Amp Chains
1. Set `ROUTING_MODE` to `Parallel Matrix` and `PARALLEL_BLEND` to 0.50.
2. Select `EXC_MODE` = `Asymmetric Tube` and increase `HARMONICS` to 0.65 for rich even overtones.
3. Set `MAX_PHASE_MODE` to `Classic` for vintage speaker cabinet phase feel.

### 3. Level-Matched A/B Testing
1. Audition processing at desired settings.
2. Observe RMS level on `MeterComponent`.
3. Lower `MASTER_OUT_GAIN` until wet RMS matches dry RMS.
4. Toggle `MASTER_MIX` between 0.0 and 1.0 (or toggle `MAX_ENABLE`/`EXC_ENABLE`) to confirm true clarity enhancement.

## Troubleshooting
- **Plugin Not Appearing in DAW**: Check DAW cache has not rescanned VST3 or LV2 directories. Force a rescan. Ensure the VST3 bundle (`Aural Maximizer Pro.vst3`) is in `/usr/lib/vst3` or `~/.vst3` on Linux, or `C:\Program Files\Common Files\VST3` on Windows.
- **Oversampling updates**: Restart audio playback or re-prepare the plugin in the host to apply oversampling changes.
- **HARMONICS control**: Confirm that Exciter Mode is set to Asymmetric Tube when using the HARMONICS control.
- **BLEND control**: Confirm that Routing Mode is set to Parallel Matrix when using the BLEND control.
- **Output Is Distorted / Clipping**: Pushing `PROCESS`, `LO CONTOUR`, or `DRIVE` increases total RMS. Reduce `MASTER_OUT_GAIN` or `MASTER_IN_GAIN` to maintain clean headroom.

## DSP & Implementation Notes (Developer & AI Reference)
- **Source Files & Responsibilities**:
  - `Source/PluginProcessor.h/.cpp`: Implements processor, APVTS layout, parameter caching, routing, level meters, lock-free FFT spectrum FIFO, state serialization, and latency reporting.
  - `Source/PluginEditor.h/.cpp`: Implements GUI, APVTS attachments, look-and-feel styling, and child visualizers (size: 920x540 pixels).
  - `Source/DSP/SonicMaximizer.h/.cpp`: 3-band crossover, APF stages, low-band envelope follower.
  - `Source/DSP/AuralExciter.h/.cpp`: HPF sidechain, oversampler management, waveshapers, air high-shelf filter.
  - `Source/DSP/RoutingEngine.h/.cpp`: Manages serial/parallel topologies and scratch buffers.
  - `Source/GUI/`: Look and feel, level meters, spectrum visualizer.
- **Real-Time Safety**: All code in `processBlock()` and `DSP::process()` has zero dynamic memory allocation/deallocation, zero mutex locks, zero blocking I/O, and CPU denormals disabled via `juce::ScopedNoDenormals`.
- **Build Artifact Locations**:
  - *Linux Debug*: `cmake-build-debug/AuralMaximizerPro_artefacts/Debug/{Standalone,VST3,LV2}`
  - *Windows Release*: `build/AuralMaximizerPro_artefacts/Release/{Standalone,VST3,LV2}`
