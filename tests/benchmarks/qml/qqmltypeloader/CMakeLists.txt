# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qqmltypeloader_benchmark Binary:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qqmltypeloader_benchmark LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmltypeloader_benchmark
    GUI
    SOURCES
        tst_qqmltypeloader.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
        Qt::Test
    TESTDATA ${test_data}
)


qt_internal_extend_target(tst_qqmltypeloader_benchmark CONDITION ANDROID OR IOS OR OHOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmltypeloader_benchmark CONDITION NOT ANDROID AND NOT IOS AND NOT OHOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_add_resources(tst_qqmltypeloader_benchmark "typeloader"
    PREFIX
        "/"
    FILES
        "data/doesExist.qml"
)
