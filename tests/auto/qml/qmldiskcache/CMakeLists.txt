# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmldiskcache.pro.

#####################################################################
## tst_qmldiskcache Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmldiskcache LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qmldiskcache
    SOURCES
        tst_qmldiskcache.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Qml
        Qt::QmlPrivate
)

# special case begin
# Disable quick compiler for this test
set_source_files_properties(
    importmodule.qml
    module.mjs
    test.qml
    PROPERTIES
    QT_SKIP_QUICKCOMPILER TRUE
)
# special case end

# Resources:
set(qmake_immediate_resource_files
    "importmodule.qml"
    "module.mjs"
    "test.qml"
)

qt_internal_add_resource(tst_qmldiskcache "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


## Scopes:
#####################################################################
