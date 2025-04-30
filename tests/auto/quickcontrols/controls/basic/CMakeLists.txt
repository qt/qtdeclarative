# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from basic.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_basic LANGUAGES C CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_basic Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/*)
list(APPEND test_data ${test_data_glob})

set(controls_data ${CMAKE_CURRENT_SOURCE_DIR}/../data)

qt_internal_add_test(tst_basic
    GUI
    QMLTEST
    SOURCES
        tst_basic.cpp
    DEFINES
        TST_CONTROLS_DATA="${controls_data}"
    LIBRARIES
        Qt::Gui
        Qt::QuickControls2
    TESTDATA ${test_data}
)

# Make sure qml import scanner finds the qml files when building against a static Qt build.
# By default it only scans the current source directory.
set_target_properties(tst_basic PROPERTIES
    QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS "${controls_data}")

# Make the QML files available to Creator's locator.
target_sources(tst_basic
    PRIVATE
        ${test_data}
)

set_source_files_properties(${test_data}
    PROPERTIES
        HEADER_FILE_ONLY ON
)

set(test_target tst_basic)
include(../shared.cmake)

#### Keys ignored in scope 1:.:.:basic.pro:<TRUE>:
# OTHER_FILES = "$$PWD/../data/*.qml"
# TEMPLATE = "app"
