# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_ecmascripttests Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_ecmascripttests LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    test262/*)
list(APPEND test_data ${test_data_glob})
list(FILTER test_data EXCLUDE REGEX ".git")

qt_internal_add_test(tst_ecmascripttests
    SOURCES
        test262runner.cpp test262runner.h
        tst_ecmascripttests.cpp
    LIBRARIES
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA
        ${test_data}

    # We're doing our own QTest timeout handling here.
    # Don't let ctest interfere.
    TIMEOUT 3600
)

if(ANDROID OR IOS)
    qt_add_resources(tst_ecmascripttests "testExpectations"
        PREFIX
            "/"
        FILES
            "TestExpectations"
    )

    qt_internal_extend_target(tst_ecmascripttests
        DEFINES
            QT_QMLTEST_DATADIR=":/test262"
    )
else()
    qt_internal_extend_target(tst_ecmascripttests
        DEFINES
            QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/test262"
    )
endif()
