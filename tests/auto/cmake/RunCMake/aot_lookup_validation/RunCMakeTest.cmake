# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

set(RunCMake_TEST_NO_CLEAN 1)
set(RunCMake_TEST_OUTPUT_MERGE 1)

set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/aot_lookup_validation")

# MetaObject mismatch -> crash
set(ENV{QV4_FAIL_ON_INVALID_AOT} "1")

if(WIN32)
    math(EXPR idxNeg3 "${CMAKE_ARGC} - 3")
    math(EXPR idxNeg4 "${CMAKE_ARGC} - 4")

    set(qt_install_prefix "${CMAKE_ARGV${idxNeg4}}")
    set(qt_install_bins "${CMAKE_ARGV${idxNeg3}}")
    set(qt_bin_dir "${qt_install_prefix}/${qt_install_bins}")

    # Needed to find DLLs
    set(new_path "${qt_bin_dir};$ENV{PATH}")
    set(ENV{PATH} "${new_path}")
endif()


run_cmake_with_options(config "-DQt6_DIR=${Qt6_DIR}")
run_cmake_command(build ${CMAKE_COMMAND} --build "${RunCMake_TEST_BINARY_DIR}" --parallel)

run_cmake_command(ensure-aotcompiled
    ${CMAKE_COMMAND}
    -P "${CMAKE_CURRENT_LIST_DIR}/ensure_aot_compiled.cmake")

set(ENV{SHOULD_FAIL} "0")
run_cmake_command(run-no-mismatch
    ${CMAKE_COMMAND}
    -P "${CMAKE_CURRENT_LIST_DIR}/verify_execution.cmake")

run_cmake_command(backup-other
    ${CMAKE_COMMAND} -E copy
        "${RunCMake_SOURCE_DIR}/Other/other.h"
        "${RunCMake_SOURCE_DIR}/Other/other_backup.h")

# Add signal to break metaobject layout. This shifts relative method indices.
run_cmake_command(add-signal
    ${CMAKE_COMMAND} -E copy
        "${RunCMake_SOURCE_DIR}/Other/other_with_signal.h"
        "${RunCMake_SOURCE_DIR}/Other/other.h")

run_cmake_command(rebuild ${CMAKE_COMMAND} --build "${RunCMake_TEST_BINARY_DIR}" --parallel)

set(ENV{SHOULD_FAIL} "1")
run_cmake_command(run-mismatch
    ${CMAKE_COMMAND}
    -P "${CMAKE_CURRENT_LIST_DIR}/verify_execution.cmake")

run_cmake_command(restore-other
    ${CMAKE_COMMAND} -E rename
        "${RunCMake_SOURCE_DIR}/Other/other_backup.h"
        "${RunCMake_SOURCE_DIR}/Other/other.h")
