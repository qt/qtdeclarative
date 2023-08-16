# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickwidget.pro.

#####################################################################
## tst_qquickwidget Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickwidget LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickwidget
    SOURCES
        tst_qquickwidget.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::QuickWidgets
        Qt::QuickWidgetsPrivate
        Qt::WidgetsPrivate
        Qt::QuickTestUtilsPrivate
        Qt::TestPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickwidget.pro:<TRUE>:
# OTHER_FILES = "animating.qml" "error1.qml" "rectangle.qml" "resizemodeitem.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickwidget CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickwidget CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
