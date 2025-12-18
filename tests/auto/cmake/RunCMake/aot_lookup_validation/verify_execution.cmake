# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT $ENV{QV4_FAIL_ON_INVALID_AOT} EQUAL "1")
    message(FATAL_ERROR "env var propagation problem")
endif()

execute_process(
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/aot_lookup_validation"
    RESULT_VARIABLE res
)

if(${res} EQUAL 0)
    if($ENV{SHOULD_FAIL})
        message(FATAL_ERROR "Should have crashed but didn't")
    endif()
else()
    if(NOT $ENV{SHOULD_FAIL})
        message(FATAL_ERROR "Should have passed but didn't")
    endif()
endif()

