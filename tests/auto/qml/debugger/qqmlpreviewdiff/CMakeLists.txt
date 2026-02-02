# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qqmlpreviewdiff Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qqmlpreviewdiff LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlpreviewdiff
    SOURCES
        qqmlpreviewpatch.cpp
        qqmlpreviewpatch.h
        tst_qqmlpreviewdiff.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::QmlPrivate
        Qt::QmlPreviewPrivate
        Qt::QuickTestUtilsPrivate
        Qt::Test
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qqmlpreviewdiff CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlpreviewdiff CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
