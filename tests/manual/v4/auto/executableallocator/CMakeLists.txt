# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_executableallocator LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Generated from executableallocator.pro.

#####################################################################
## tst_executableallocator Binary:
#####################################################################

qt_internal_add_manual_test(tst_executableallocator
    SOURCES
        tst_executableallocator.cpp
    LIBRARIES
        Qt::Test
        v4Private
)

#### Keys ignored in scope 1:.:.:executableallocator.pro:<TRUE>:
# TEMPLATE = "app"
