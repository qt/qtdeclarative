# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qwidgetsinqml.pro.

#####################################################################
## tst_qwidgetsinqml Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qwidgetsinqml LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qwidgetsinqml
    SOURCES
        tst_qwidgetsinqml.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::Widgets
        Qt::QuickTestUtilsPrivate
    TESTDATA "dummy_imports.qml"
)

## Scopes:
#####################################################################
