# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquicklayouts.pro.

#####################################################################
## tst_qquicklayouts Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquicklayouts LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquicklayouts
    GUI
    QMLTEST
    SOURCES
        tst_qquicklayouts.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquicklayouts.pro:<TRUE>:
# OTHER_FILES = "data/tst_rowlayout.qml" "data/tst_gridlayout.qml" "data/tst_stacklayout.qml"
# TEMPLATE = "app"
