# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qjsengine.pro.

#####################################################################
## tst_qjsengine Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qjsengine LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    script/*)
list(APPEND test_data ${test_data_glob})

# special case begin
# TODO: Prepare for removal, once Platform brings in Threads.
if(NOT TARGET Threads::Threads)
    qt_find_package(Threads PROVIDED_TARGETS Threads::Threads)
endif()
# special case end

qt_internal_add_test(tst_qjsengine
    SOURCES
        tst_qjsengine.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    LIBRARIES # special case
        Threads::Threads # special case
    TESTDATA ${test_data} "dummy_imports.qml"
)

# Resources:
set(qjsengine_resource_files
    "translations/idtranslatable-unicode.qm"
    "translations/idtranslatable_la.qm"
    "translations/translatable-unicode.qm"
    "translations/translatable_la.qm"
)

qt_internal_add_resource(tst_qjsengine "qjsengine"
    PREFIX
        "/"
    FILES
        ${qjsengine_resource_files}
)
set(qmake_immediate_resource_files
    "exporterror1.mjs"
    "importerror1.mjs"
    "modulewithlexicals.mjs"
    "testmodule.mjs"
    "testregister.mjs"
    "testregister2.mjs"
    "testregister3.mjs"
)

qt_internal_add_resource(tst_qjsengine "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qjsengine)
endif()

## Scopes:
#####################################################################
