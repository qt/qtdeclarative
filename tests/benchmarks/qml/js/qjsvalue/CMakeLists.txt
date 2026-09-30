# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qjsvalue.pro.

#####################################################################
## tst_bench_qjsvalue Binary:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_bench_qjsvalue LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_bench_qjsvalue
    SOURCES
        tst_qjsvalue.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qjsvalue.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
