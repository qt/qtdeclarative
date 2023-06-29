# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from listview.pro.

#####################################################################
## listview Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(listview LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(listview
    GUI
    QMLTEST
    LIBRARIES
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:listview.pro:<TRUE>:
# DISTFILES = "$$PWD/tst_listview.qml" "$$PWD/data/*.qml"
