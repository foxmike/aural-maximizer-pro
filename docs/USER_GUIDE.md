# Aural Maximizer Pro — Official User & Technical Guide

---

## Table of Contents
1. [Product Overview](#1-product-overview)
2. [Signal-Flow Overview](#2-signal-flow-overview)
3. [Complete Interface Reference](#3-complete-interface-reference)
4. [Maximizer Section](#4-maximizer-section)
5. [Aural Exciter Section](#5-aural-exciter-section)
6. [Routing and Blend](#6-routing-and-blend)
7. [Master Controls](#7-master-controls)
8. [Metering and Visualization](#8-metering-and-visualization)
9. [Standalone Application](#9-standalone-application)
10. [Plugin Operation](#10-plugin-operation)
11. [Recommended Workflows](#11-recommended-workflows)
12. [Parameter Reference Table](#12-parameter-reference-table)
13. [Technical Reference for Developers and LLM Agents](#13-technical-reference-for-developers-and-llm-agents)
14. [Troubleshooting](#14-troubleshooting)

---

## 1. Product Overview

**Aural Maximizer Pro** (plugin code `Axpr`, manufacturer `nSpace`, version `0.1.0`) is a dual-engine audio processor designed to restore acoustic presence, enhance low-frequency dynamic punch, and synthesize high-frequency harmonic shimmer in real time. It combines two DSP architectures—a phase-aligning 3-band **Sonic Maximizer** and an oversampled nonlinear **Aural Exciter** with post-saturation air filter control—into a unified plugin and standalone audio application.

### Primary Processing Sections
1. **Sonic Maximizer**: Divides the incoming signal into three distinct frequency bands (Low, Mid, High) separated by user-configurable crossover frequencies. It applies dynamic envelope-followed bass expansion (`LO CONTOUR`) and frequency-dependent phase delay/high-frequency expansion (`PROCESS`). Users can toggle between **Classic** (multi-stage all-pass phase alignment) and **Modern** (single-stage low-band phase alignment) topologies.
2. **Aural Exciter**: Extracts a high-frequency sidechain above a tunable cutoff frequency (`TUNE`), drives it through an oversampled nonlinear waveshaper (`DRIVE` and `HARMONICS` across Soft Clipping, Tube, and Tape modes), boosts high-frequency air (`AIR GAIN`), and blends the generated harmonics back in parallel with the main audio path (`MIX`).

### Supported Formats & OS Compatibility
Based on the project build configuration (`CMakeLists.txt`), Aural Maximizer Pro compiles natively for the following formats:
- **Standalone Application**: Independent executable with JUCE Audio/MIDI device configuration wrappers.
- **VST3 Plugin**: 64-bit VST3 bundle (`.vst3`) for standard DAW insertion.
- **LV2 Plugin**: Linux/open-platform LV2 bundle (`.lv2`) complete with TTL manifests (`manifest.ttl`, `AuralMaximizerPro.ttl`).

> [!NOTE]
> Format support is governed strictly by `CMakeLists.txt` (`FORMATS Standalone VST3 LV2`). Formats such as AAX, VST2, AU, and Unity are not enabled in the current build specification.

### Channel Layout & Stereo Topology
- **Supported Layouts**: Mono-in / Mono-out and Stereo-in / Stereo-out.
- **Bus Constraints**: Enforced by `isBusesLayoutSupported()`; input channel count must equal output channel count (1:1 or 2:2). Multi-mono and surround layouts are not supported.
- **Stereo Behavior**: Left and Right channels are processed independently with separate per-channel envelope followers in the Sonic Maximizer and independent sidechain state paths in the Aural Exciter. Shared parameters are smoothed concurrently across channels.

---

## 2. Signal-Flow Overview

Audio entering Aural Maximizer Pro flows sequentially through input gain staging, clean dry buffering, module routing, master dry/wet matrix blending, output gain staging, and thread-safe visualization metering.

### Complete Processing Order
1. **Master Input Gain (`MASTER_IN_GAIN`)**: Incoming PCM audio blocks are multiplied sample-by-sample by a smoothed gain factor (ranging from -24.0 dB to +24.0 dB, 20 ms linear smoothing window).
2. **Dry Buffer Allocation (`dryBuffer`)**: An exact copy of the input-gained signal is written to `dryBuffer` prior to any DSP module processing.
3. **Routing Engine (`ROUTING_MODE`)**: The gained signal enters `RoutingEngine::process()`, which directs audio into the Sonic Maximizer and/or Aural Exciter modules according to one of three selectable topologies:
   - **Serial: Maximizer -> Exciter**
   - **Serial: Exciter -> Maximizer**
   - **Parallel Matrix Blend**: Input is duplicated into scratch buffers `scratchBufferA` and `scratchBufferB`. The Maximizer processes Path A; the Exciter processes Path B. Output is blended via `PARALLEL_BLEND` ($K$):
     $$\text{Output} = (1 - K) \cdot \text{Path}_A + K \cdot \text{Path}_B$$
4. **Module Bypass Evaluation (`MAX_ENABLE`, `EXC_ENABLE`)**: Individual module enable switches selectively bypass module execution. In serial modes, a disabled module acts as a transparent wire. In parallel mode, a disabled module leaves its respective scratch buffer un-processed (dry input to that branch).
5. **Master Mix (`MASTER_MIX`)**: The processed output from the routing engine is crossfaded with the unprocessed `dryBuffer`:
   $$\text{Mixed} = (1 - \text{MasterMix}) \cdot \text{DryBuffer} + \text{MasterMix} \cdot \text{Processed}$$
6. **Master Output Gain (`MASTER_OUT_GAIN`)**: The mixed signal is multiplied by a smoothed output gain factor (-24.0 dB to +24.0 dB, 20 ms linear smoothing).
7. **Metering & Visualization**:
   - Post-output-gain samples are evaluated for Peak magnitude (`getMagnitude()`) and RMS energy (`getRMSLevel()`) on Left and Right channels.
   - Downmixed mono audio frames are pushed to a lock-free ring buffer (`spectrumFifo`) for GUI FFT spectrum rendering.
8. **Audio Output**: Final block delivered to host DAW or audio interface.

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

### Module Bypass vs. Master Dry/Wet
- **Module Bypass (`MAX_ENABLE` / `EXC_ENABLE`)**: Silences or bypasses specific internal DSP engines while keeping the rest of the routing matrix active. For instance, disabling `MAX_ENABLE` in Serial mode passes the input signal directly to the Exciter without Maximizer band-splitting or phase delay.
- **Master Dry/Wet (`MASTER_MIX`)**: Performs a global blend between the completely unprocessed input signal (after Input Gain) and the output of the full routing topology. When set to `0.0` (100% Dry), all internal processing is auditioned against pure pass-through audio.

---

## 3. Complete Interface Reference

The Aural Maximizer Pro interface measures 920 x 540 pixels and is structured into three visually distinct dark-themed panels: **Sonic Maximizer** (Cyan border), **Aural Exciter** (Orange border), and **Master / Routing Matrix / I-O** (Green border).

### Global & Header Displays
- **Window Title**: `"AURAL MAXIMIZER PRO"` (Top-left header, 20 pt bold typography).
- **Sonic Maximizer Header**: `"SONIC MAXIMIZER"` (Top panel label, Cyan `#34C759` accent).
- **Aural Exciter Header**: `"AURAL EXCITER"` (Top-right panel label, Orange `#FF9500` accent).
- **Visualizer Header**: `"FFT / OUTPUT SPECTRUM"` (Upper right display label).
- **Master Header**: `"MASTER / ROUTING MATRIX / I-O"` (Bottom panel label, Green `#34C759` accent).
- **Spectrum Visualizer**: Real-time 2048-point FFT curve display (479x112 px) rendered in cyan (`#4AD2B0`) over a dark grid (`#0E1218`).
- **Output Level Meters**: Dual stereo peak and RMS vertical LED bar meters (128x90 px) with peak hold indicators and center dB scale (+6 to -50 dB).

### Detailed Parameter Guide

#### 1. Maximizer Enable
- **Interface Label**: `MAXIMIZER ON`
- **Parameter ID**: `MAX_ENABLE`
- **Control Type**: Toggle Button
- **Valid Range**: Off (`false` / `0.0`), On (`true` / `1.0`)
- **Default**: On (`true` / `1.0`)
- **Technical Action**: Toggles active processing of the `SonicMaximizer` class inside `RoutingEngine`. When `false`, the Maximizer DSP loop is skipped entirely.
- **Audible Result**: Disabling removes bass dynamic contouring and treble phase delay expansion.
- **Saved in State**: Yes.

#### 2. Maximizer Process
- **Interface Label**: `PROCESS`
- **Parameter ID**: `MAX_PROCESS`
- **Control Type**: Rotary Slider (`RotaryHorizontalVerticalDrag`)
- **Valid Range**: `0.000` to `1.000` (step `0.001`, linear scale)
- **Default**: `0.500`
- **Units**: Normalized ratio
- **Technical Action**: Controls high-frequency band gain multiplier according to:
  $$\text{HighGain} = 1.0 + 3.0 \cdot \text{MAX\_PROCESS}$$
  At `0.0`, high band gain is $1.0\times$ (+0 dB). At `0.5`, gain is $2.5\times$ (+7.96 dB). At `1.0`, gain is $4.0\times$ (+12.04 dB).
- **Audible Result**: Low settings maintain original high-frequency balance. High settings dramatically boost bite, sparkle, and vocal/instrument transient attack above `MAX_HIGH_FREQ`.
- **Interactions**: Strongly interacts with `MAX_HIGH_FREQ` (defines where boost begins) and `MASTER_OUT_GAIN` (high settings can cause output clipping).
- **Saved in State**: Yes.

#### 3. Maximizer Lo Contour
- **Interface Label**: `LO CONTOUR`
- **Parameter ID**: `MAX_CONTOUR`
- **Control Type**: Rotary Slider
- **Valid Range**: `0.000` to `1.000` (step `0.001`, linear scale)
- **Default**: `0.500`
- **Units**: Normalized ratio
- **Technical Action**: Scales the dynamic gain applied to the low frequency band ($<\text{MAX\_LOW\_FREQ}$). An internal envelope follower ($\tau = 50\text{ ms}$) tracks low-band amplitude $e$. The applied gain is:
  $$\text{LowGain} = 1.0 + \text{LO\_CONTOUR} \cdot \left(\frac{e}{1.0 + e}\right)$$
  Max boost approaches $1.0 + \text{LO\_CONTOUR}$ ($+6.02\text{ dB}$ at `1.0`).
- **Audible Result**: Adds dynamic warmth, low-end punch, and cabinet thud to kick drums, bass guitars, and full mixes without muddying static mid-bass frequencies.
- **Saved in State**: Yes.

#### 4. Maximizer Low Frequency
- **Interface Label**: `LOW FREQ`
- **Parameter ID**: `MAX_LOW_FREQ`
- **Control Type**: Rotary Slider
- **Valid Range**: `40.0` Hz to `400.0` Hz (step `1.0` Hz, logarithmic skew `0.5`)
- **Default**: `120.0` Hz
- **Units**: Hz
- **Technical Action**: Sets the low-pass / mid-high-pass crossover frequency between Low and Mid bands. Coefficients update dynamically every 16 samples using a 50 ms smoothing parameter. Clamped to $\le \text{MAX\_HIGH\_FREQ}$.
- **Audible Result**: Adjusts the upper boundary of bass enhancement. Lower values focus contouring on deep sub-bass; higher values extend punch into upper bass/low-mids.
- **Saved in State**: Yes.

#### 5. Maximizer High Frequency
- **Interface Label**: `HIGH FREQ`
- **Parameter ID**: `MAX_HIGH_FREQ`
- **Control Type**: Rotary Slider
- **Valid Range**: `1000.0` Hz to `12000.0` Hz (step `1.0` Hz, logarithmic skew `0.5`)
- **Default**: `5000.0` Hz
- **Units**: Hz
- **Technical Action**: Sets the crossover frequency separating Mid and High bands. Clamped to $\ge \text{MAX\_LOW\_FREQ}$.
- **Audible Result**: Low settings (1–3 kHz) make Process affect upper midrange clarity; high settings (6–12 kHz) isolate Process to air, cymbal sheen, and breathiness.
- **Saved in State**: Yes.

#### 6. Maximizer Phase Mode
- **Interface Label**: `Phase Mode`
- **Parameter ID**: `MAX_PHASE_MODE`
- **Control Type**: Dropdown ComboBox
- **Choices**: `Classic` (index 0), `Modern` (index 1)
- **Default**: `Classic`
- **Technical Action**:
  - **Classic**: Low band passes through 4 cascaded 1st-order all-pass filters (APFs); Mid band passes through 2 APF stages; High band has 0 APFs. All-pass frequency equals `MAX_LOW_FREQ`.
  - **Modern**: Low band passes through 1 APF stage; Mid band has 0 APFs; High band has 0 APFs.
  - Phase mode changes crossfade smoothly over 20 ms (`phaseModeBlend`) to eliminate switching clicks.
- **Audible Result**: Classic mode provides vintage phase-aligned dispersion and warmth. Modern mode preserves tighter transient alignment and punch.
- **Saved in State**: Yes.

#### 7. Exciter Enable
- **Interface Label**: `EXCITER ON`
- **Parameter ID**: `EXC_ENABLE`
- **Control Type**: Toggle Button
- **Valid Range**: Off (`false`), On (`true`)
- **Default**: On (`true`)
- **Technical Action**: Toggles `AuralExciter` processing inside `RoutingEngine`.
- **Saved in State**: Yes.

#### 8. Exciter Tune
- **Interface Label**: `TUNE`
- **Parameter ID**: `EXC_TUNE`
- **Control Type**: Rotary Slider
- **Valid Range**: `800.0` Hz to `10000.0` Hz (step `1.0` Hz, logarithmic skew `0.5`)
- **Default**: `3000.0` Hz
- **Units**: Hz
- **Technical Action**: Sets cutoff frequency of the 2nd-order State Variable TPT high-pass filter feeding the exciter sidechain ($Q = 0.7071$).
- **Audible Result**: Determines which frequencies feed the distortion generator. Lower settings add grit and body to upper mids; higher settings isolate excitation to top-end air.
- **Saved in State**: Yes.

#### 9. Exciter Drive
- **Interface Label**: `DRIVE`
- **Parameter ID**: `EXC_DRIVE`
- **Control Type**: Rotary Slider
- **Valid Range**: `0.0` dB to `24.0` dB (step `0.1` dB, linear scale)
- **Default**: `6.0` dB
- **Units**: dB
- **Technical Action**: Applies linear pre-gain boost ($\text{gain} = 10^{\text{dB}/20}$) to high-passed sidechain samples before entering waveshaper.
- **Audible Result**: Higher drive pushes waveshaper deeper into saturation, generating richer harmonic overtones.
- **Saved in State**: Yes.

#### 10. Exciter Mix
- **Interface Label**: `MIX`
- **Parameter ID**: `EXC_MIX`
- **Control Type**: Rotary Slider
- **Valid Range**: `0.000` to `1.000` (step `0.001`, linear scale)
- **Default**: `0.500`
- **Units**: Normalized ratio
- **Technical Action**: Blends excited sidechain output into the main signal path:
  $$\text{Output} = \text{MainInput} + \text{EXC\_MIX} \cdot \text{SidechainOutput}$$
- **Audible Result**: Controls intensity of synthesized harmonics mixed into signal.
- **Saved in State**: Yes.

#### 11. Exciter Mode
- **Interface Label**: `Exciter Mode`
- **Parameter ID**: `EXC_MODE`
- **Control Type**: Dropdown ComboBox
- **Choices**: `Cubic Soft Clipping` (0), `Asymmetric Tube` (1), `Tape` (2)
- **Default**: `Cubic Soft Clipping`
- **Technical Action**: Selects nonlinear transfer function applied at oversampled rate:
  - **Cubic Soft Clipping**: Piecewise polynomial function limiting output smoothly between -1.0 and +1.0. Produces odd harmonics ($3^{\text{rd}}, 5^{\text{th}}$).
  - **Asymmetric Tube**: $\tanh(x + 0.15) + \gamma \cdot x^2$. Introduces DC bias shift and strong $2^{\text{nd}}$ even harmonics scaled by `HARMONICS`.
  - **Tape**: $\frac{2}{\pi} \arctan(x)$. Smooth soft saturation with gentle harmonic compression.
- **Saved in State**: Yes.

#### 12. Exciter Harmonics
- **Interface Label**: `HARMONICS`
- **Parameter ID**: `EXC_HARMONICS`
- **Control Type**: Rotary Slider
- **Valid Range**: `0.000` to `1.000` (step `0.001`, linear scale)
- **Default**: `0.500`
- **Units**: Normalized ratio
- **Technical Action**: Scales 2nd-order quadratic term ($\gamma \cdot x^2$) in `Asymmetric Tube` mode.
- **Audible Result**: Increases warmth and even-harmonic richness in Tube mode.

> [!IMPORTANT]
> **Known Implementation Subtlety**: `HARMONICS` (`EXC_HARMONICS`) is evaluated ONLY when `Exciter Mode` is set to `Asymmetric Tube`. In `Cubic Soft Clipping` and `Tape` modes, adjusting `HARMONICS` has no DSP effect.

#### 13. Exciter Air Gain
- **Interface Label**: `AIR GAIN`
- **Parameter ID**: `EXC_AIR_GAIN`
- **Control Type**: Rotary Slider
- **Valid Range**: `0.0` dB to `12.0` dB (step `0.1` dB, linear scale)
- **Default**: `0.0` dB
- **Units**: dB
- **Technical Action**: Boosts post-saturation sidechain signal using an IIR high-shelf filter ($10.0\text{ kHz}$ corner frequency, $Q = 0.7071$).
- **Audible Result**: Adds ultra-high sheen and silkiness above 10 kHz without distorting lower frequencies.
- **Saved in State**: Yes.

#### 14. Routing Mode
- **Interface Label**: `Routing Mode`
- **Parameter ID**: `ROUTING_MODE`
- **Control Type**: Dropdown ComboBox
- **Choices**: `Maximizer -> Exciter` (0), `Exciter -> Maximizer` (1), `Parallel Matrix` (2)
- **Default**: `Maximizer -> Exciter`
- **Technical Action**: Selects DSP module execution sequence or parallel matrix blending.
- **Saved in State**: Yes.

#### 15. Parallel Blend
- **Interface Label**: `BLEND`
- **Parameter ID**: `PARALLEL_BLEND`
- **Control Type**: Rotary Slider
- **Valid Range**: `0.000` to `1.000` (step `0.001`, linear scale)
- **Default**: `0.500`
- **Units**: Normalized ratio
- **Technical Action**: Crossfades between Maximizer branch ($0.0$) and Exciter branch ($1.0$) in `Parallel Matrix` mode.

> [!NOTE]
> **Known Implementation Subtlety**: `PARALLEL_BLEND` is active ONLY when `Routing Mode` is set to `Parallel Matrix`. In serial routing modes (`Maximizer -> Exciter` or `Exciter -> Maximizer`), the Blend control is bypassed in DSP.

#### 16. Master Input Gain
- **Interface Label**: `INPUT`
- **Parameter ID**: `MASTER_IN_GAIN`
- **Control Type**: Rotary Slider
- **Valid Range**: `-24.0` dB to `+24.0` dB (step `0.1` dB, linear scale)
- **Default**: `0.0` dB
- **Units**: dB
- **Technical Action**: Multiplies incoming PCM samples before dry buffer copy and module routing (20 ms linear smoothing).
- **Saved in State**: Yes.

#### 17. Master Output Gain
- **Interface Label**: `OUTPUT`
- **Parameter ID**: `MASTER_OUT_GAIN`
- **Control Type**: Rotary Slider
- **Valid Range**: `-24.0` dB to `+24.0` dB (step `0.1` dB, linear scale)
- **Default**: `0.0` dB
- **Units**: dB
- **Technical Action**: Multiplies final mixed signal after Master Mix (20 ms linear smoothing).
- **Saved in State**: Yes.

#### 18. Master Mix
- **Interface Label**: `MIX`
- **Parameter ID**: `MASTER_MIX`
- **Control Type**: Rotary Slider
- **Valid Range**: `0.000` to `1.000` (step `0.001`, linear scale)
- **Default**: `1.000` (100% Wet)
- **Units**: Normalized ratio
- **Technical Action**: Crossfades between pre-DSP dry buffer ($0.0$) and routing engine output ($1.0$).
- **Saved in State**: Yes.

#### 19. Oversampling
- **Interface Label**: `Oversampling`
- **Parameter ID**: `OVERSAMPLING`
- **Control Type**: Dropdown ComboBox
- **Choices**: `2x` (0), `4x` (1)
- **Default**: `2x`
- **Technical Action**: Configures JUCE FIR half-band equiripple oversampling stage count (1 stage for 2x, 2 stages for 4x) for Exciter nonlinear waveshaper.

> [!IMPORTANT]
> **Known Implementation Subtlety**: The oversampling factor is allocated and initialized strictly during `prepareToPlay()`. Changing `OVERSAMPLING` live during playback stores the choice in APVTS state, but DSP oversampling factor and host latency reporting update when audio processing is re-prepared by host DAW or standalone audio driver restart.

---

## 4. Maximizer Section

The **Sonic Maximizer** section restores natural frequency phase alignment and dynamic punch.

### Signal Band Splitting
Audio is divided into three bands using 2nd-order IIR filters:
- **Low Band**: $< \text{MAX\_LOW\_FREQ}$ (2 cascaded low-pass filters)
- **Mid Band**: $\text{MAX\_LOW\_FREQ} \le f \le \text{MAX\_HIGH\_FREQ}$ (2 cascaded high-pass filters at low cut + 2 cascaded low-pass filters at high cut)
- **High Band**: $> \text{MAX\_HIGH\_FREQ}$ (2 cascaded high-pass filters)

### Processing & Phase Topology
- **Process Gain**: Applied exclusively to the High band:
  $$\text{HighGain} = 1.0 + 3.0 \cdot \text{MAX\_PROCESS}$$
  Increases high-frequency amplitude up to $4.0\times$ (+12.04 dB).
- **Lo Contour Gain**: An envelope follower ($\tau = 50\text{ ms}$) measures low-band peak energy $e$. Low band multiplier:
  $$\text{LowGain} = 1.0 + \text{LO\_CONTOUR} \cdot \left(\frac{e}{1.0 + e}\right)$$
  Provides up to +6.02 dB dynamic bass expansion.
- **Phase Modes**:
  - **Classic**: Low band receives 4 all-pass filter stages; Mid band receives 2 APF stages; High band receives 0 APF stages. All-pass frequency equals `MAX_LOW_FREQ`. This creates group delay compensation where high frequencies lead, mid frequencies follow, and low frequencies arrive last.
  - **Modern**: Low band receives 1 APF stage; Mid/High bands receive 0 APF stages. Reduces total phase rotation for punchier bass.
  - Crossfading between phase modes is smoothed linearly over 20 ms.

### Level-Matched Evaluation Guidance
Because high `PROCESS` and `LO CONTOUR` values increase total RMS energy (up to +12 dB on highs and +6 dB on lows), high settings will sound louder. Always use `MASTER_OUT_GAIN` to match peak/RMS levels on meters when performing A/B bypass comparisons to evaluate genuine clarity gains rather than psychoacoustic loudness bias.

---

## 5. Aural Exciter Section

The **Aural Exciter** synthesizes musically related high-frequency harmonics not present in the original input signal.

### Sidechain Path & Saturation
1. **High-Pass Sidechain**: A 2nd-order State Variable TPT high-pass filter isolates audio above `EXC_TUNE` (800 Hz to 10,000 Hz).
2. **Drive & Waveshaping**: The sidechain is boosted by `EXC_DRIVE` (0 to 24 dB) and passed through the selected waveshaper:
   - **Cubic Soft Clipping**: Smooth polynomial saturation ($|x| \ge 4/3 \implies \pm 1.0$). Generates odd harmonics.
   - **Asymmetric Tube**: $\tanh(x + 0.15) + \gamma \cdot x^2$. Generates even harmonics governed by `HARMONICS` ($\gamma$).
   - **Tape**: $\frac{2}{\pi}\arctan(x)$. Soft tape saturation curve.
3. **Oversampling & Anti-Aliasing**: Waveshaping occurs inside an FIR oversampling block (2x or 4x). 4x oversampling provides deeper aliasing suppression at the cost of higher CPU consumption and linear-phase latency (cached via `setLatencySamples()`).
4. **Air Shelf Filter**: Post-saturation sidechain passes through a 10 kHz high-shelf filter boosted by `EXC_AIR_GAIN` (0 to 12 dB).
5. **Parallel Wet Mix**: Sidechain output is mixed into main audio scaled by `EXC_MIX`.

---

## 6. Routing and Blend

Aural Maximizer Pro provides three signal topologies via `ROUTING_MODE`:

1. **`Maximizer -> Exciter` (Serial, Default)**: Signal passes through Sonic Maximizer phase alignment and band expansion first, followed by Aural Exciter harmonic generation. Ideal for general mix bus processing.
2. **`Exciter -> Maximizer` (Serial)**: Signal generates high-frequency harmonics first, which are subsequently phase-aligned and frequency-split by the Maximizer. Produces smoother harmonic integration on dense guitar tracks.
3. **`Parallel Matrix`**: Signal splits into independent Maximizer and Exciter branches. `PARALLEL_BLEND` ($K$) crossfades output:
   - $K = 0.00$: 100% Maximizer, 0% Exciter.
   - $K = 0.50$: 50% Maximizer, 50% Exciter.
   - $K = 1.00$: 0% Maximizer, 100% Exciter.

---

## 7. Master Controls

- **Master Input Gain (`MASTER_IN_GAIN`)**: Range -24.0 dB to +24.0 dB. Adjusts incoming level before internal DSP and dry buffering. Useful for driving exciter sidechain or reducing hot signals.
- **Master Output Gain (`MASTER_OUT_GAIN`)**: Range -24.0 dB to +24.0 dB. Final gain adjustment applied after master dry/wet mixing. Use this for gain-staging and preventing output clipping.
- **Master Mix (`MASTER_MIX`)**: Range 0.0 (100% Dry) to 1.0 (100% Wet). Blends original unprocessed dry signal with routing engine output.

---

## 8. Metering and Visualization

### Stereo Level Meters (`MeterComponent`)
- **Channels**: Left Peak, Left RMS, Right Peak, Right RMS.
- **Scale**: -50 dB to +6 dB normalized LED display.
- **Color Zones**: Green ($< -18\text{ dB}$), Yellow-Green ($-18\text{ to } -6\text{ dB}$), Orange ($-6\text{ to } 0\text{ dB}$), Red ($> 0\text{ dB}$ / Clipping).
- **Decay & Hold**: Peak visual decay rate 0.92/frame; RMS decay 0.85/frame; Peak hold holds max peak for 1 second (30 frames at 30 Hz timer rate) before decaying at 0.95/frame.
- **Measurement Position**: Taken post-output-gain at final buffer delivery.

### FFT Spectrum Visualizer (`SpectrumVisualizer`)
- **FFT Size**: 2048 samples (`spectrumFrameSize`), 11th order Hann windowing.
- **Data Transport**: Lock-free single-producer single-consumer ring buffer (`spectrumFifo`, capacity 8 frames). Audio thread downmixes stereo to mono average and pushes frames without locking.
- **GUI Render**: Message thread pops frames at 30 Hz, computes forward FFT, maps magnitudes to a logarithmic frequency curve (1 bin to 1024 bins), and renders a curved line in cyan (`#4AD2B0`). Release decay factor is 0.82.

---

## 9. Standalone Application

The Standalone application (`Aural Maximizer Pro`) allows operating the plugin without a DAW.

### Configuration
- **Audio Device Setup**: Click options bar to select Audio API (ALSA/Jack/PipeWire on Linux; ASIO/WASAPI on Windows), sample rate, block size, and active input/output channels.
- **System Audio Routing**: System audio does not automatically route through the standalone app. On Linux, use PipeWire (`pw-jack` or qpwgraph) or JACK to route system audio into plugin inputs. On Windows, use virtual audio cables or ASIO routing.

---

## 10. Plugin Operation

### Host DAW Insertion
- **Plugin Format**: VST3 (`.vst3`) or LV2 (`.lv2`). Insert on track FX slots, submix busses, or master output channel.
- **Automation**: All 19 parameters are registered with APVTS and fully automatable in DAWs.
- **State Restoration**: Full state (all parameter values and phase/routing modes) is serialized to XML via `getStateInformation()` and restored seamlessly via `setStateInformation()`.

### Build Artifact Locations (Developer Reference)
- **Linux**:
  - Standalone: `cmake-build-debug/AuralMaximizerPro_artefacts/Debug/Standalone/Aural Maximizer Pro`
  - VST3: `cmake-build-debug/AuralMaximizerPro_artefacts/Debug/VST3/Aural Maximizer Pro.vst3`
  - LV2: `cmake-build-debug/AuralMaximizerPro_artefacts/Debug/LV2/Aural Maximizer Pro.lv2`
- **Windows**:
  - Standalone: `build/AuralMaximizerPro_artefacts/Release/Standalone/Aural Maximizer Pro.exe`
  - VST3: `build/AuralMaximizerPro_artefacts/Release/VST3/Aural Maximizer Pro.vst3`
  - LV2: `build/AuralMaximizerPro_artefacts/Release/LV2/Aural Maximizer Pro.lv2`

---

## 11. Recommended Workflows

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

---

## 12. Parameter Reference Table

| Section | Interface Label | Parameter ID | Control Type | Valid Range / Choices | Default | Units | Technical Summary |
|---|---|---|---|---|---|---|---|
| Maximizer | MAXIMIZER ON | `MAX_ENABLE` | Toggle Button | Off (`0.0`), On (`1.0`) | On (`1.0`) | — | Enables/bypasses Sonic Maximizer module |
| Maximizer | PROCESS | `MAX_PROCESS` | Rotary Slider | `0.000` to `1.000` (step `0.001`) | `0.500` | Ratio | High-band expansion gain ($1.0\times$ to $4.0\times$) |
| Maximizer | LO CONTOUR | `MAX_CONTOUR` | Rotary Slider | `0.000` to `1.000` (step `0.001`) | `0.500` | Ratio | Dynamic low-band envelope follower gain boost |
| Maximizer | LOW FREQ | `MAX_LOW_FREQ` | Rotary Slider | `40.0` to `400.0` (skew `0.5`) | `120.0` | Hz | Low/Mid crossover frequency |
| Maximizer | HIGH FREQ | `MAX_HIGH_FREQ` | Rotary Slider | `1000.0` to `12000.0` (skew `0.5`) | `5000.0` | Hz | Mid/High crossover frequency |
| Maximizer | Phase Mode | `MAX_PHASE_MODE` | Choice Box | `Classic` (0), `Modern` (1) | `Classic` (0) | Choice | All-pass filter phase delay topology |
| Exciter | EXCITER ON | `EXC_ENABLE` | Toggle Button | Off (`0.0`), On (`1.0`) | On (`1.0`) | — | Enables/bypasses Aural Exciter module |
| Exciter | TUNE | `EXC_TUNE` | Rotary Slider | `800.0` to `10000.0` (skew `0.5`) | `3000.0` | Hz | 2nd-order TPT high-pass sidechain cutoff |
| Exciter | DRIVE | `EXC_DRIVE` | Rotary Slider | `0.0` to `24.0` (step `0.1`) | `6.0` | dB | Sidechain pre-saturation drive gain |
| Exciter | MIX | `EXC_MIX` | Rotary Slider | `0.000` to `1.000` (step `0.001`) | `0.500` | Ratio | Parallel blend of excited sidechain into audio |
| Exciter | Exciter Mode | `EXC_MODE` | Choice Box | `Cubic Soft Clipping` (0), `Asymmetric Tube` (1), `Tape` (2) | `Cubic Soft Clipping` (0) | Choice | Nonlinear waveshaper transfer function |
| Exciter | HARMONICS | `EXC_HARMONICS` | Rotary Slider | `0.000` to `1.000` (step `0.001`) | `0.500` | Ratio | 2nd-harmonic amount (active in Tube mode only) |
| Exciter | AIR GAIN | `EXC_AIR_GAIN` | Rotary Slider | `0.0` to `12.0` (step `0.1`) | `0.0` | dB | Post-saturation 10 kHz high-shelf filter gain |
| Master | Routing Mode | `ROUTING_MODE` | Choice Box | `Maximizer -> Exciter` (0), `Exciter -> Maximizer` (1), `Parallel Matrix` (2) | `Maximizer -> Exciter` (0) | Choice | Signal routing order & matrix topology |
| Master | BLEND | `PARALLEL_BLEND` | Rotary Slider | `0.000` to `1.000` (step `0.001`) | `0.500` | Ratio | Crossfade between Maximizer/Exciter in Parallel mode |
| Master | INPUT | `MASTER_IN_GAIN` | Rotary Slider | `-24.0` to `+24.0` (step `0.1`) | `0.0` | dB | Pre-DSP global input gain |
| Master | OUTPUT | `MASTER_OUT_GAIN` | Rotary Slider | `-24.0` to `+24.0` (step `0.1`) | `0.0` | dB | Post-mix global output gain |
| Master | MIX | `MASTER_MIX` | Rotary Slider | `0.000` to `1.000` (step `0.001`) | `1.000` | Ratio | Global dry/wet mix ratio |
| Master | Oversampling | `OVERSAMPLING` | Choice Box | `2x` (0), `4x` (1) | `2x` (0) | Choice | FIR oversampling factor for Exciter waveshaper |

---

## 13. Technical Reference for Developers and LLM Agents

### Main Source Files & Responsibilities
- `Source/PluginProcessor.h/.cpp`: Implements `AuralMaximizerProAudioProcessor`, APVTS parameter layout creation, parameter caching, `processBlock` routing execution, meter computation, lock-free spectrum FIFO pushing, XML state serialization/restoration, and latency reporting.
- `Source/PluginEditor.h/.cpp`: Implements GUI layout (`AuralMaximizerProAudioProcessorEditor`), APVTS attachments, look-and-feel styling, and child visualizers.
- `Source/DSP/SonicMaximizer.h/.cpp`: 3-band crossover filter bank, all-pass phase delay stages, low-band envelope follower, and smoothed crossover update logic.
- `Source/DSP/AuralExciter.h/.cpp`: 2nd-order TPT high-pass sidechain, FIR oversampling stage management, cubic/tube/tape waveshapers, 10 kHz air high-shelf filter.
- `Source/DSP/RoutingEngine.h/.cpp`: Manages serial execution and parallel matrix scratch buffer allocations & linear crossfading.
- `Source/GUI/CustomLookAndFeel.h/.cpp`: Dark UI theme color definitions and custom rotary slider / combo box drawing.
- `Source/GUI/MeterComponent.h/.cpp`: 30 Hz timer-driven peak & RMS meter UI rendering with lock-free atomic level reads and peak hold decay.
- `Source/GUI/SpectrumVisualizer.h/.cpp`: 30 Hz timer-driven 2048-point FFT spectrum curve renderer reading from `spectrumFifo`.

### Real-Time Safety Expectations
All code inside `processBlock()` and `DSP::process()` adheres to strict audio-thread real-time safety:
- Zero dynamic memory allocation/deallocation (`new`/`delete`/`malloc`/`free` or `std::vector` resizing).
- Zero mutex locks, critical sections, or thread synchronization primitives (uses lock-free `std::atomic` and `juce::AbstractFifo`).
- Zero blocking I/O, file access, or string allocations.
- CPU denormals disabled via `juce::ScopedNoDenormals`.

### Parameter Ownership & APVTS
Parameters are owned by `apvts` (`juce::AudioProcessorValueTreeState`). Raw parameter atomic pointers are cached during constructor initialization (`cacheParameterHandles()`) and loaded via `loadParameter()` with relaxed memory ordering (`std::memory_order_relaxed`).

### State Serialization
Plugin state is exported as XML via `apvts.copyState().createXml()` in `getStateInformation()` and restored via `apvts.replaceState()` in `setStateInformation()`.

> [!WARNING]
> **Documentation Maintenance Mandate**: Developers and LLM agents modifying parameter IDs, parameter default values, crossover ranges, oversampling factors, DSP order, or GUI control layout MUST update `docs/USER_GUIDE.md` concurrently.

---

## 14. Troubleshooting

### 1. Plugin Not Appearing in DAW
- **Cause**: DAW cache has not rescanned VST3 or LV2 directories.
- **Solution**: Perform a forced re-scan in DAW preferences. Ensure the VST3 bundle (`Aural Maximizer Pro.vst3`) is located in `/usr/lib/vst3` or `~/.vst3` on Linux, or `C:\Program Files\Common Files\VST3` on Windows.

### 2. Oversampling Selection Does Not Reduce Aliasing Immediately
- **Cause**: `OVERSAMPLING` selection takes effect on `prepareToPlay()`.
- **Solution**: Restart audio playback or re-initialize the plugin instance in host.

### 3. `HARMONICS` Knob Seems to Do Nothing
- **Cause**: Exciter is in `Cubic Soft Clipping` or `Tape` mode.
- **Solution**: Switch `Exciter Mode` to `Asymmetric Tube`. `HARMONICS` is active only in Tube mode.

### 4. `BLEND` Control Has No Effect
- **Cause**: `Routing Mode` is set to a serial mode (`Maximizer -> Exciter` or `Exciter -> Maximizer`).
- **Solution**: Switch `Routing Mode` to `Parallel Matrix`. Blend is active only in Parallel mode.

### 5. Output Is Distortion / Clipping
- **Cause**: High `PROCESS`, `LO CONTOUR`, or `DRIVE` settings pushing output beyond 0.0 dBFS.
- **Solution**: Reduce `MASTER_OUT_GAIN` or lower `MASTER_IN_GAIN` to maintain clean headroom.

---
