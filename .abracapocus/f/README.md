# f

## Goal

Implement the CustomLookAndFeel theme. Header first, then implementation (re-writes the phase-a stub, single owner here). Define the palette constants (Background #121417, Panel #1A1D22, Border #2C3038, Maximizer accent #00D2FF, Exciter accent #FF9500, Master accent #34C759, Text #F2F2F7) and override drawRotarySlider() to render precision brushed-aluminum knobs, plus styled ComboBox and ToggleButton drawing consistent with the dark slate chassis. No audio-thread interaction.

## Notes

- Async: Wave 1: f.1 writes Source/GUI/CustomLookAndFeel.h only (palette constants, override declarations). Wave 2: f.2 writes Source/GUI/CustomLookAndFeel.cpp only and depends on f.1. Final wave: f.3 writes docs/verification/phase_f_lookandfeel.md and depends on f.2. No same-file overlap.
- Risk: Verification: cmake/Ninja Standalone build plus cppcheck and clang-format on the two files; not make. Colour constants must match the design hex values exactly so accent routing (cyan/amber/green) is correct when the editor assigns them per module. JUCE 8 changes some Font constructors; if pinned to JUCE 8 the drawText paths may need the newer API (see operator_questions).
