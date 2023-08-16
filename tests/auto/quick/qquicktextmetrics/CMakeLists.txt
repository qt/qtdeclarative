# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquicktextmetrics.pro.

#####################################################################
## tst_qquicktextmetrics Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquicktextmetrics LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qquicktextmetrics
    SOURCES
        tst_qquicktextmetrics.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickPrivate
)

## Scopes:
#####################################################################
