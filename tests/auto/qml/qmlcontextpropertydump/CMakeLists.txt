# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmlcontextpropertydump LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmlcontextpropertydump
    SOURCES
        tst_qmlcontextpropertydump.cpp
    LIBRARIES
        Qt::Gui
        Qt::QuickTestUtilsPrivate
        Qt::QmlCompilerPrivate
        Qt::QmlTypeRegistrarPrivate
        LintPlugin
    TESTDATA ${test_data}
)

if (TARGET qmlcontextpropertydump)
    add_dependencies(tst_qmlcontextpropertydump Qt::qmlcontextpropertydump)
endif()
