# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from auto.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(treeview_sidebyside LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## treeview_sidebyside manual test
#####################################################################

qt_internal_add_manual_test(treeview_sidebyside
    GUI
    SOURCES
        testmodel.cpp testmodel.h
        uicallback.h
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTest
)

qt_internal_extend_target(treeview_sidebyside CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)

qt6_add_qml_module(treeview_sidebyside
    URI TestModel
    VERSION 1.0
)

# Resources:
set(qmake_immediate_resource_files
    "data"
)

qt_internal_add_resource(treeview_sidebyside "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

