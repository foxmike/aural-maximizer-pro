# i

## Goal

Full build and multi-format target verification. Provide a deterministic, paste-safe build helper script that configures with Ninja and builds every format, then record the verification artifact. The script runs: cmake -B cmake-build-debug -G Ninja, followed by cmake --build cmake-build-debug for the AuralMaximizerPro_Standalone, AuralMaximizerPro_VST3, AuralMaximizerPro_CLAP, and AuralMaximizerPro_LV2 targets, and reports any undefined references or target-discovery failures. This is the only phase that gates the full four-format matrix; earlier phases gate Standalone only.

## Notes

- Async: Wave 1: i.1 writes scripts/build_all.sh only (configure + build all four format targets, no set -e, no exit 0, paste-safe). Final wave: i.2 writes docs/verification/phase_i_build_targets.md and depends on i.1, recording the clean multi-format build result. No same-file overlap.
- Risk: Verification: run scripts/build_all.sh and confirm all requested format targets build; this phase intentionally uses the CMake/Ninja multi-target build, never make. CLAP and LV2 only build if the CLAP-extension and JUCE-version questions are resolved; if CLAP is deferred, the script and doc build Standalone+VST3+LV2 and mark CLAP pending rather than fail. The script must not use set -e or exit 0 so a paste into an interactive shell cannot close it.
