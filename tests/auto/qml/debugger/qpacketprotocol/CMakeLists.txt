# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qpacketprotocol.pro.

#####################################################################
## tst_qpacketprotocol Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qpacketprotocol LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qpacketprotocol
    SOURCES
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qpacketprotocol.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::QmlDebugPrivate
        Qt::QuickTestUtilsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpacketprotocol CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qpacketprotocol CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
