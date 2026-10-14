# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquicktooltip LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquicktooltip
    SOURCES
        tst_qquicktooltip.cpp
    DEFINES
        QQC2_IMPORT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols"
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickControls2
        Qt::QuickControls2Private
        Qt::QuickTemplates2
        Qt::QuickTemplates2Private
        Qt::QuickTestUtilsPrivate
        Qt::QuickControlsTestUtilsPrivate
        Qt::TestPrivate
    TESTDATA ${test_data}
)
