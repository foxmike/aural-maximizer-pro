# Outstanding Issues

Internal engineering document. Not user-facing.

## Confirmed Issues

### Oversampling Selection Is Applied Only During Host Preparation

- **Area:** DSP
- **Current behavior:** Changing the exposed `OVERSAMPLING` parameter updates saved parameter state, but the active oversampling factor and reported latency remain at the values selected during the most recent `prepareToPlay()` call. The new selection takes effect when the host re-prepares the plugin.
- **Expected behavior:** Either apply an oversampling selection while audio is active with a safe transition and refreshed host latency, or expose the selection as a configuration control whose preparation requirement is explicit to the host.
- **Impact:** Automating the selection during uninterrupted playback does not immediately change the Exciter oversampler or plugin delay compensation.
- **Evidence:** `Source/PluginProcessor.cpp:223-225` reads the parameter and selects the factor only in `prepareToPlay()`; `Source/PluginProcessor.cpp:237-250` prepares the Exciter and updates latency in that same path. `Source/DSP/AuralExciter.cpp:46-53` constructs the selected oversampler and caches its latency during `prepare()`. The audio-block path calls `updateParameterTargets()` but does not rebuild the oversampler (`Source/PluginProcessor.cpp:299-319`).
- **Next action:** Decide whether the parameter should be configuration-only or support live changes; then implement the corresponding host-automation and latency-update behavior.
