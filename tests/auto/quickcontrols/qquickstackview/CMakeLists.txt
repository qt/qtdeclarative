# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickstackview LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickstackview
    SOURCES
    tst_qquickstackview.cpp
    DEFINES
        QQC2_IMPORT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols"
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTemplates2
        Qt::QuickTemplates2Private
        Qt::QuickTestUtilsPrivate
        Qt::QuickControlsTestUtilsPrivate
        Qt::TestPrivate
    TESTDATA ${test_data}
)

# Make the QML files available to Creator's locator.
target_sources(tst_qquickstackview
    PRIVATE
        ${test_data}
)
