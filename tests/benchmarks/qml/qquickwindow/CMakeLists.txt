# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickwindow.pro.

#####################################################################
## tst_qquickwindow Binary:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickwindow_bench LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_qquickwindow_bench # special case
    SOURCES
        tst_qquickwindow.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGLPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qquickwindow.pro:<TRUE>:
# DEPLOYMENT = "testDataFiles"
# testDataFiles.files = "data"
# testDataFiles.path = "."

## Scopes:
#####################################################################
