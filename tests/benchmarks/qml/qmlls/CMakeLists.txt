# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_highlightConstruction LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

find_package(Qt COMPONENTS OPTIONAL Qt::QmlLSPrivate)

if (NOT TARGET Qt::QmlLSPrivate)
    return()
endif()

qt_internal_add_benchmark(tst_highlightConstruction
    SOURCES
        tst_highlightConstruction.cpp
    DEFINES
        EXAMPLESDIR="${QtDeclarative_SOURCE_DIR}/examples"
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Core
        Qt::Test
        Qt::QuickTestUtilsPrivate
        Qt::QmlLSPrivate
)
