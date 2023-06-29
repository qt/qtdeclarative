# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_reformatter:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_reformatter LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/..
    domdata/reformatter/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_reformatter
    SOURCES
        tst_reformatter.cpp tst_reformatter.h
    DEFINES
        QT_DEPRECATED_WARNINGS
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/../domdata"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::Test
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_reformatter CONDITION ANDROID OR IOS
    DEFINES
        QT_REFORMATTERTEST_DATADIR=":/domdata"
)
