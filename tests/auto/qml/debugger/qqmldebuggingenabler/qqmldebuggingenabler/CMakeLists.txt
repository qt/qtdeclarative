# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmldebuggingenabler.pro.

#####################################################################
## tst_qqmldebuggingenabler Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qqmldebuggingenabler LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qqmldebuggingenabler
    SOURCES
        ../../shared/debugutil.cpp ../../shared/debugutil_p.h
        ../../shared/qqmldebugprocess.cpp ../../shared/qqmldebugprocess_p.h
        tst_qqmldebuggingenabler.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlDebugPrivate
        Qt::QuickTestUtilsPrivate
)

#### Keys ignored in scope 1:.:.:qqmldebuggingenabler.pro:<TRUE>:
# OTHER_FILES = "data/test.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmldebuggingenabler CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmldebuggingenabler CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
