# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.19)
project(qt_target_qml_sources_snippet)

set(CMAKE_AUTOMOC TRUE)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ![0]
set_source_files_properties(nested/way/down/File.qml PROPERTIES
    QT_RESOURCE_ALIAS File.qml
)
set_source_files_properties(TemplateFile.qml PROPERTIES
    QT_RESOURCE_ALIAS templates/File.qml
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
    QT_QML_SKIP_QMLLINT TRUE
    QT_QML_SKIP_CACHEGEN TRUE
)
set_source_files_properties(FunnySingleton.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
qt_add_qml_module(qt_target_qml_sources_example
    URI Example
    VERSION 2.3
    RESOURCE_PREFIX /my.company.com/imports
    QML_FILES
        nested/way/down/File.qml
        TemplateFile.qml
        FunnySingleton.qml
)

set_source_files_properties(some_old_thing.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "1.1;2.0"
    QT_QML_SOURCE_TYPENAME OldThing
)
set_source_files_properties(../../../images/button-types.png PROPERTIES
    QT_RESOURCE_ALIAS button-types.png
)
qt_target_qml_sources(qt_target_qml_sources_example
    QML_FILES some_old_thing.qml
    RESOURCES
        ../../../images/button-types.png
        doc/README.txt
)
# ![0]
