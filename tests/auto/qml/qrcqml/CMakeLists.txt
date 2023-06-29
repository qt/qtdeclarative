# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qrcqml.pro.

#####################################################################
## tst_qrcqml Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qrcqml LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qrcqml
    SOURCES
        tst_qrcqml.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
)

# Resources:
set_source_files_properties("data/SameDir2.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SameDir.qml"
)
set_source_files_properties("data/importtest.qml"
    PROPERTIES QT_RESOURCE_ALIAS "importtest.qml"
)
set_source_files_properties("data/main2.qml"
    PROPERTIES QT_RESOURCE_ALIAS "main.qml"
)
set(qrcqml_resource_files
    "data/SameDir.qml"
    "data/SameDir2.qml"
    "data/SubDir.qml"
    "data/importtest.qml"
    "data/main.qml"
    "data/main2.qml"
)

qt_internal_add_resource(tst_qrcqml "qrcqml"
    PREFIX
        "/"
    FILES
        ${qrcqml_resource_files}
)
set_source_files_properties("data/SameDir3.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SameDir.qml"
)
set_source_files_properties("data/main3.qml"
    PROPERTIES QT_RESOURCE_ALIAS "main.qml"
)
set(qrcqml1_resource_files
    "data/SameDir3.qml"
    "data/main3.qml"
)

qt_internal_add_resource(tst_qrcqml "qrcqml1"
    PREFIX
        "/search"
    FILES
        ${qrcqml1_resource_files}
)
set_source_files_properties("data/SubDir.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SubDir.qml"
)
set(qrcqml2_resource_files
    "data/SubDir.qml"
)

qt_internal_add_resource(tst_qrcqml "qrcqml2"
    PREFIX
        "/search/data"
    FILES
        ${qrcqml2_resource_files}
)
set_source_files_properties("data/imports/QrcImport/Imported.qml"
    PROPERTIES QT_RESOURCE_ALIAS "QrcImport/Imported.qml"
)
set_source_files_properties("data/imports/QrcImport/qmldir"
    PROPERTIES QT_RESOURCE_ALIAS "QrcImport/qmldir"
)
set(qrcqml3_resource_files
    "data/imports/QrcImport/Imported.qml"
    "data/imports/QrcImport/qmldir"
)

qt_internal_add_resource(tst_qrcqml "qrcqml3"
    PREFIX
        "/imports"
    FILES
        ${qrcqml3_resource_files}
)


## Scopes:
#####################################################################
