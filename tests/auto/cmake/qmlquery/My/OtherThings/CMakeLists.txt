# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_policy(SET QTP0001 NEW)

function(verify_result keyword expected actual)
    if(NOT "${actual}" STREQUAL "${expected}")
        message(SEND_ERROR
            "  Expected ${keyword}: ${expected}\n"
            "  Actual   ${keyword}: ${actual}"
        )
    endif()
endfunction()

qt_add_qml_module(MyOtherThings
    URI My.OtherThings
    SOURCES
        test.h test.cpp
)

qt_query_qml_module(MyOtherThings
    QML_FILES qml_files2
    RESOURCES resources2
)

# empty resources and files
verify_result(RESOURCES "${resources2}" "")
verify_result(QML_FILES "${qml_files2}" "")
