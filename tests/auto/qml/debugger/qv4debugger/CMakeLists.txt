# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qv4debugger.pro.

#####################################################################
## tst_qv4debugger Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qv4debugger LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        data/*)
list(APPEND test_data ${test_data_glob})

qt_add_library(testCppTypes STATIC)
qt_autogen_tools_initial_setup(testCppTypes)
target_link_libraries(testCppTypes PRIVATE Qt::Qml Qt::QmlPrivate Qt::Quick)

qt_policy(SET QTP0001 NEW)

qt6_add_qml_module(testCppTypes
    URI TestTypes
    SOURCES
        commontypes.h
    OUTPUT_DIRECTORY TestTypes
)
qt_autogen_tools_initial_setup(testCppTypesplugin)

qt_internal_add_test(tst_qv4debugger
    SOURCES
        ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4datacollector.cpp ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4datacollector.h
        ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugger.cpp ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugger.h
        ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugjob.cpp ../../../../../src/plugins/qmltooling/qmldbg_debugger/qv4debugjob.h
        tst_qv4debugger.cpp
    INCLUDE_DIRECTORIES
        ../../../../../src/plugins/qmltooling/qmldbg_debugger
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
        testCppTypesplugin
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qv4debugger CONDITION ANDROID OR IOS
        DEFINES
        QT_QMLTEST_DATADIR=":/data"
        )

qt_internal_extend_target(tst_qv4debugger CONDITION NOT ANDROID AND NOT IOS
        DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
        )
