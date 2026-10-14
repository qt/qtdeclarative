# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickheaderviewdelegate LANGUAGES C CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_qquickheaderviewdelegate Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickheaderviewdelegate
    SOURCES
        tst_qquickheaderviewdelegate.cpp
    LIBRARIES
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
        Qt::QuickTemplates2Private
    TESTDATA ${test_data}
)
