# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from styles-cover-flow.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(styles-cover-flow LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## stylescoverflow Binary:
#####################################################################

qt_internal_add_manual_test(stylescoverflow
    GUI
    SOURCES
        styles-cover-flow.cpp
    DEFINES
        DOC_IMAGES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/imports/controls/doc/images/"
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
)

file(GLOB resource_glob_0 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.qml,")
foreach(file IN LISTS resource_glob_0)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

# Resources:
set(qmake_immediate_resource_files
    ${resource_glob_0}
)

qt_internal_add_resource(stylescoverflow "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:styles-cover-flow.pro:<TRUE>:
# TEMPLATE = "app"
