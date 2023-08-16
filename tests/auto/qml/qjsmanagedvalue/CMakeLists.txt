# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qjsmanagedvalue.pro.

#####################################################################
## tst_qjsmanagedvalue Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qjsmanagedvalue LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qjsmanagedvalue
    SOURCES
        tst_qjsmanagedvalue.cpp tst_qjsmanagedvalue.h
    LIBRARIES
        Qt::Qml
        Qt::QmlPrivate
    TESTDATA "dummy_imports.qml"
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qjsmanagedvalue)
endif()

## Scopes:
#####################################################################
