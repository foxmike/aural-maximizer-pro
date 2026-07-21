# h

## Goal

Assemble the PluginEditor and attach every control to the APVTS. Header first, then implementation, then a single-line re-wire of the processor's createEditor. The editor header declares the fixed 920x540 window, the CustomLookAndFeel instance, all knobs/combos/toggles, the embedded SpectrumVisualizer and MeterComponent, and the APVTS attachment members. The implementation lays out the split-screen wireframe (left: Sonic Maximizer in cyan, right: Aural Exciter in amber with the FFT display, bottom: master routing/matrix/I-O in green), assigns accent colours, and binds every parameter with juce::AudioProcessorValueTreeState::SliderAttachment / ComboBoxAttachment / ButtonAttachment to the exact 19 parameter IDs. A separate task then updates PluginProcessor.cpp so createEditor() returns the real PluginEditor instead of the generic editor.

## Notes

- Async: Wave 1: h.1 writes Source/PluginEditor.h only (components, attachments, fixed size). Wave 2: h.2 writes Source/PluginEditor.cpp only and depends on h.1. Wave 3: h.3 writes Source/PluginProcessor.cpp only (swap createEditor to return the real editor) and depends on h.2; this is the only task in phase h that writes the processor file, so it is single owner here even though the file was authored in phases a and e. Final wave: h.4 writes docs/verification/phase_h_editor.md and depends on h.3. No two tasks in this phase share a write path.
- Risk: Verification: cmake/Ninja Standalone build plus cppcheck and clang-format on the changed files; not make. Attachment parameter IDs must exactly match the phase-e APVTS layout or attachments assert at construction. Layout coordinates must honor the fixed 920x540 (46:27) frame. h.3 touches PluginProcessor.cpp (also written in phases a and e) but is the sole writer within phase h; keep h.3 minimal (createEditor swap only).
