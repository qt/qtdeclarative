# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(shapestest LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Generated from shapestest.pro.

#####################################################################
## shapestest Binary:
#####################################################################

qt_internal_add_manual_test(shapestest
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(shapestest_resource_files
    "shapestest.qml"
)

qt_internal_add_resource(shapestest "shapestest"
    PREFIX
        "/shapestest"
    FILES
        ${shapestest_resource_files}
)


#### Keys ignored in scope 1:.:.:shapestest.pro:<TRUE>:
# OTHER_FILES = "shapestest.qml"
# TEMPLATE = "app"
