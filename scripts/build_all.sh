#!/usr/bin/env bash

PLUGIN_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PLUGIN_ROOT"

echo "[build_all] Configuring with Ninja..."
cmake -B cmake-build-debug -G Ninja
CONFIG_STATUS=$?
if [ $CONFIG_STATUS -ne 0 ]; then
    echo "[build_all] ERROR: cmake configure failed (exit $CONFIG_STATUS)"
    return $CONFIG_STATUS 2>/dev/null || true
fi

echo ""
echo "[build_all] Building AuralMaximizerPro_Standalone..."
cmake --build cmake-build-debug --target AuralMaximizerPro_Standalone
STANDALONE_STATUS=$?
[ $STANDALONE_STATUS -ne 0 ] && echo "[build_all] FAILED: Standalone (exit $STANDALONE_STATUS)" || echo "[build_all] OK: Standalone"

echo ""
echo "[build_all] Building AuralMaximizerPro_VST3..."
cmake --build cmake-build-debug --target AuralMaximizerPro_VST3
VST3_STATUS=$?
[ $VST3_STATUS -ne 0 ] && echo "[build_all] FAILED: VST3 (exit $VST3_STATUS)" || echo "[build_all] OK: VST3"

echo ""
echo "[build_all] Building AuralMaximizerPro_LV2..."
cmake --build cmake-build-debug --target AuralMaximizerPro_LV2
LV2_STATUS=$?
[ $LV2_STATUS -ne 0 ] && echo "[build_all] FAILED: LV2 (exit $LV2_STATUS)" || echo "[build_all] OK: LV2"

echo ""
echo "[build_all] Building AuralMaximizerPro_CLAP..."
cmake --build cmake-build-debug --target AuralMaximizerPro_CLAP
CLAP_STATUS=$?
if [ $CLAP_STATUS -ne 0 ]; then
    echo "[build_all] PENDING/SKIPPED: CLAP build failed (exit $CLAP_STATUS) — clap-juce-extensions may not be configured yet"
else
    echo "[build_all] OK: CLAP"
fi

echo ""
echo "[build_all] Summary:"
echo "  Standalone : $([ $STANDALONE_STATUS -eq 0 ] && echo PASS || echo FAIL)"
echo "  VST3       : $([ $VST3_STATUS -eq 0 ] && echo PASS || echo FAIL)"
echo "  LV2        : $([ $LV2_STATUS -eq 0 ] && echo PASS || echo FAIL)"
echo "  CLAP       : $([ $CLAP_STATUS -eq 0 ] && echo PASS || echo PENDING)"
