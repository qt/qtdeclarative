# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmltypeloader.pro.

#####################################################################
## tst_qqmltypeloader Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qqmltypeloader LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmltypeloader
    SOURCES
        declarativetesttype.h
        tst_qqmltypeloader.cpp
    LIBRARIES
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 2:.:.:tst_qqmltypeloader.pro:<TRUE>:
# QML_IMPORT_NAME = "declarative.import.for.typeloader.test"
# QML_IMPORT_VERSION = "3.2"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmltypeloader CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmltypeloader CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/lazyresource.rcc"
    PROPERTIES QT_RESOURCE_ALIAS "lazyresource.rcc")

qt_add_resources(tst_qqmltypeloader "typeloader"
    PREFIX
        "/"
    FILES
        "declarative/import/for/typeloader/test/qmldir"
        "declarative/import/for/typeloader/test/Singleton.qml"
        "data/compilercompatibility/signalHandlers.qml"
        "${CMAKE_CURRENT_BINARY_DIR}/lazyresource.rcc"
)

set_target_properties(tst_qqmltypeloader PROPERTIES
    QT_QML_MODULE_VERSION 3.2
    QT_QML_MODULE_URI declarative.import.for.typeloader.test
    QT_QML_MODULE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

_qt_internal_qml_type_registration(tst_qqmltypeloader)
add_subdirectory(SlowImport)

# A resource compiled to a standalone .rcc that the test registers at run time, to exercise
# the directory cache being invalidated when a resource appears after it was first probed.
qt_add_binary_resources(tst_qqmltypeloader_lazy_resource "lazyresource/lazy.qrc"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/lazyresource.rcc")
add_dependencies(tst_qqmltypeloader tst_qqmltypeloader_lazy_resource)
