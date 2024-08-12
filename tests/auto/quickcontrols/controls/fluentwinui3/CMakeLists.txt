# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_fluentwinui3 LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_fluentwinui3
    GUI
    QMLTEST
    SOURCES
        tst_fluentwinui3.cpp
    DEFINES
        TST_CONTROLS_DATA="${CMAKE_CURRENT_SOURCE_DIR}/../data"
    LIBRARIES
        Qt::Gui
        Qt::QuickControls2
    TESTDATA ${test_data}
)

# Make the QML files available to Creator's locator.
target_sources(tst_fluentwinui3
    PRIVATE
        ${test_data}
)

set_source_files_properties(${test_data}
    PROPERTIES
        HEADER_FILE_ONLY ON
)

set(test_target tst_fluentwinui3)
include(../shared.cmake)
