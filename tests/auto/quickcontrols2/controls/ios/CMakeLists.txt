# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_ios
    GUI
    QMLTEST
    SOURCES
        tst_ios.cpp
    DEFINES
        TST_CONTROLS_DATA=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../data\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QuickControls2
    TESTDATA ${test_data}
)

