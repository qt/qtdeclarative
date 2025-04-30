# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from designer.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_designer LANGUAGES C CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_designer Test:
#####################################################################

qt_internal_add_test(tst_designer
    SOURCES
        tst_designer.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:designer.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
