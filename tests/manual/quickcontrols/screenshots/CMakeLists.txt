# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from screenshots.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(screenshots LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## screenshots Binary:
#####################################################################

qt_internal_add_manual_test(screenshots
    GUI
    SOURCES
        screenshots.cpp
    DEFINES
        SNIPPETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols/doc/snippets"
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Widgets
)

# Resources:
set(qmake_immediate_resource_files
    "qtquickcontrols2.conf"
    "screenshots.qml"
)

qt_internal_add_resource(screenshots "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:screenshots.pro:<TRUE>:
# TEMPLATE = "app"
