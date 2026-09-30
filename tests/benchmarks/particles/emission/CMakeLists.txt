# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from emission.pro.

#####################################################################
## tst_emission Binary:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_emission LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_emission
    SOURCES
        tst_emission.cpp
    LIBRARIES
        Qt::Gui
        Qt::QuickParticlesPrivate
        Qt::Test
)

#### Keys ignored in scope 1:.:.:emission.pro:<TRUE>:
# DEPLOYMENT = "testDataFiles"
# testDataFiles.files = "data"
# testDataFiles.path = "."

## Scopes:
#####################################################################
