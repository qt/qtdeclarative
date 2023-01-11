# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from viewinqwidget.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(viewinqwidget LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## viewinqwidget Binary:
#####################################################################

qt_internal_add_manual_test(viewinqwidget
    GUI
    SOURCES
        main.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/"
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickWidgets
        Qt::Widgets
)

# Resources:
set(viewinqwidget_resource_files
    "main.qml"
)

qt_internal_add_resource(viewinqwidget "viewinqwidget"
    PREFIX
        "/"
    FILES
        ${viewinqwidget_resource_files}
)


#### Keys ignored in scope 1:.:.:viewinqwidget.pro:<TRUE>:
# OTHER_FILES = "main.qml"
