#!/usr/bin/env bash

# Configure and build each plugin format separately so the failing format is clear.

build_target() {
    local target="$1"
    local log_file
    local build_status

    log_file="$(mktemp "/tmp/aural-maximizer-pro-${target}.XXXXXX")" || {
        printf 'ERROR: Could not create a diagnostic log for target %s.\n' "$target" >&2
        return 1
    }

    printf '\n==> Building target: %s\n' "$target"
    cmake --build cmake-build-debug --target "$target" 2>&1 | tee "$log_file"
    build_status=${PIPESTATUS[0]}

    if [ "$build_status" -ne 0 ]; then
        printf 'ERROR: Build failed for target %s (status %d).\n' \
            "$target" "$build_status" >&2

        if grep -Eqi 'undefined reference|unresolved external symbol' "$log_file"; then
            printf 'ERROR: Undefined references were reported while linking target %s; see the linker output above.\n' \
                "$target" >&2
        fi

        if grep -Eqi 'unknown target|not a known target|target .* not found|no such target|does not exist' "$log_file"; then
            printf 'ERROR: Target discovery failed for %s; verify the JUCE format target name and generated Ninja build graph.\n' \
                "$target" >&2
        fi
    else
        printf 'SUCCESS: Built target %s.\n' "$target"
    fi

    rm -f -- "$log_file"
    return "$build_status"
}

main() {
    local overall_status=0
    local configure_status

    printf '==> Configuring AuralMaximizerPro with Ninja\n'
    cmake -B cmake-build-debug -G Ninja
    configure_status=$?

    if [ "$configure_status" -ne 0 ]; then
        printf 'ERROR: CMake configuration failed (status %d); targets could not be discovered.\n' \
            "$configure_status" >&2
        return "$configure_status"
    fi

    build_target AuralMaximizerPro_Standalone || overall_status=1
    build_target AuralMaximizerPro_VST3 || overall_status=1
    build_target AuralMaximizerPro_LV2 || overall_status=1

    if [ "$overall_status" -ne 0 ]; then
        printf '\nERROR: One or more AuralMaximizerPro format builds failed.\n' >&2
    else
        printf '\nSUCCESS: All AuralMaximizerPro format targets built successfully.\n'
    fi

    return "$overall_status"
}

main "$@"
