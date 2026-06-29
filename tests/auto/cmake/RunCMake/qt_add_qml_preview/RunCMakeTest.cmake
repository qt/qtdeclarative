# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

include(${_Qt6CTestMacros})
# Stub function to make `_qt_internal_get_cmake_test_configure_options` work
function(_qt_internal_get_build_vars_for_external_projects)
endfunction()

_qt_internal_get_cmake_test_configure_options(config_flags)
list(APPEND config_flags "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

# Assert the preview target's existence via the marker each case generates,
# without building it: building runs qmlpreview, which blocks and hangs CI.
function(check_preview_target case expected)
    set(marker "${RunCMake_BINARY_DIR}/${case}-build/preview_target_exists.txt")
    if(NOT EXISTS "${marker}")
        message(FATAL_ERROR "Preview marker file missing for ${case}: ${marker}")
    endif()
    file(READ "${marker}" actual)
    string(STRIP "${actual}" actual)
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR
            "Preview target check failed for ${case}: expected '${expected}', got '${actual}'")
    endif()
endfunction()

run_cmake_with_options(basic ${config_flags})
check_preview_target(basic previewapp_qmlpreview)

run_cmake_with_options(auto_add ${config_flags})
check_preview_target(auto_add app_qmlpreview)

run_cmake_with_options(auto_add_off ${config_flags} -DQT_QML_NO_QML_PREVIEW=ON)
check_preview_target(auto_add_off "")

run_cmake_with_options(auto_add_disabled ${config_flags})
check_preview_target(auto_add_disabled "")

run_cmake_with_options(non_executable ${config_flags})
run_cmake_with_options(skip_qml_module ${config_flags})
