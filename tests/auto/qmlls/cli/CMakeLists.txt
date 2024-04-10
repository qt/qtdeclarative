# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmlls_modules LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data)

qt_internal_add_test(tst_qmlls_cli
    SOURCES
        tst_qmlls_cli.cpp
        tst_qmlls_cli.h
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::Test
        Qt::QuickTestUtilsPrivate
        Qt::QmlLSPrivate
    TESTDATA ${test_data}
)

if (TARGET qmlls)
    # standalone test builds do not know the qmlls target
    # but if TARGET qmlls is known it should be built before this test
    add_dependencies(tst_qmlls_cli qmlls)
endif()
