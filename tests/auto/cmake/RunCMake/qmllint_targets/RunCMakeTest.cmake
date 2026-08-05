# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

include(${_Qt6CTestMacros})
# Stub function to make `_qt_internal_get_cmake_test_configure_options` work
function(_qt_internal_get_build_vars_for_external_projects)
endfunction()

_qt_internal_get_cmake_test_configure_options(config_flags)
list(APPEND config_flags "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

set(RunCMake_TEST_NO_CLEAN 1)
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/qmllint_targets_build")

run_cmake_with_options(configuration ${config_flags})
foreach(target IN ITEMS with_qml_files without_qml_files)
    foreach(postfix IN ITEMS _qmllint _qmllint_json _qmllint_module)
        run_cmake_command(${target}${postfix}
            ${CMAKE_COMMAND}
            --build "${RunCMake_TEST_BINARY_DIR}"
            -t ${target}${postfix})
    endforeach()
endforeach()

function(assert_file_exists path)
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "Test failed: .json was not found at ${path}")
    endif()
endfunction()

set(build_folder "${RunCMake_BINARY_DIR}/qmllint_targets_build")
assert_file_exists("${build_folder}/with_qml_files_qmllint.json")
assert_file_exists("${build_folder}/without_qml_files_qmllint.json")
