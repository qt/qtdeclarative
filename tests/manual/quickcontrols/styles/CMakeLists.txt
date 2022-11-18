# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from styles.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(styles LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## styles Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_qqc_styles
    GUI
    SOURCES
        styles.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
)

# Resources:
set(qmake_immediate_resource_files
    "styles.qml"
)

qt_internal_add_resource(tst_manual_qqc_styles "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:styles.pro:<TRUE>:
# TEMPLATE = "app"
