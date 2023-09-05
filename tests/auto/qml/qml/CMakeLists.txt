# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qml
    SOURCES
        tst_qml.cpp
    DEFINES
        QT_QMLTEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    LIBRARIES
        Qt::QuickTestUtilsPrivate
        Qt::Gui
        Qt::GuiPrivate
)
