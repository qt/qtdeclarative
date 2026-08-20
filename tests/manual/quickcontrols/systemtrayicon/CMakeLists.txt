# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from systemtrayicon.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(systemtrayicon LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## systemtrayicon Binary:
#####################################################################

qt_internal_add_manual_test(systemtrayicon
    GUI
    SOURCES
        systemtrayicon.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
)

# Resources:
set(systemtrayicon_resource_files
    "images/qt_logo_green_256.png"
    "systemtrayicon.qml"
)

qt_internal_add_resource(systemtrayicon "systemtrayicon"
    PREFIX
        "/files"
    FILES
        ${systemtrayicon_resource_files}
)


#### Keys ignored in scope 1:.:.:systemtrayicon.pro:<TRUE>:
# TEMPLATE = "app"
