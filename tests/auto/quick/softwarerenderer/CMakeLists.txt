# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_softwarerenderer Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_softwarerenderer LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_softwarerenderer
    SOURCES
        tst_softwarerenderer.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_softwarerenderer CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_softwarerenderer CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
