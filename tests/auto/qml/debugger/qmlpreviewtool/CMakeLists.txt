# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmlpreviewtool LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qmlpreviewtool
    SOURCES
        tst_qmlpreviewtool.cpp
    LIBRARIES
        Qt::Test
)

if(TARGET Qt::qmlpreviewtesthelper)
    add_dependencies(tst_qmlpreviewtool Qt::qmlpreviewtesthelper)
endif()
if(TARGET Qt::qmlpreviewtesthelpermodule)
    add_dependencies(tst_qmlpreviewtool Qt::qmlpreviewtesthelpermodule)
endif()
if(TARGET Qt::qmlpreview)
    add_dependencies(tst_qmlpreviewtool Qt::qmlpreview)
endif()
if(TARGET Qt::qml_tool)
    add_dependencies(tst_qmlpreviewtool Qt::qml_tool)
endif()
