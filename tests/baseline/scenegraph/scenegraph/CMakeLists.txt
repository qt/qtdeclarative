# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_baseline_scenegraph Test:
#####################################################################

# Collect test data
list(APPEND test_data "../data")

qt_internal_add_test(tst_baseline_scenegraph
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
    SOURCES
        ../../../../../qtbase/tests/baseline/shared/baselineprotocol.cpp
        ../../../../../qtbase/tests/baseline/shared/baselineprotocol.h
        ../../../../../qtbase/tests/baseline/shared/lookup3.cpp
        ../../../../../qtbase/tests/baseline/shared/qbaselinetest.cpp
        ../../../../../qtbase/tests/baseline/shared/qbaselinetest.h
        tst_baseline_scenegraph.cpp
    INCLUDE_DIRECTORIES
        ../../../../../qtbase/tests/baseline/shared
    LIBRARIES
        Qt::Gui
        Qt::Network
    TESTDATA ${test_data}
)
