# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from librarymetrics_performance.pro.

#####################################################################
## tst_librarymetrics_performance Binary:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_librarymetrics_performance LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_librarymetrics_performance
    SOURCES
        tst_librarymetrics_performance.cpp
    LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
        Qt::Quick
        Qt::Test
)

# Resources:
set(qmake_immediate_resource_files
    "data"
)

qt_internal_add_resource(tst_librarymetrics_performance "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:librarymetrics_performance.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
