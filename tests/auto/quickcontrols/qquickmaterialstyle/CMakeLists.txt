# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickmaterialstyle.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickmaterialstyle LANGUAGES C CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_qquickmaterialstyle Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickmaterialstyle
    GUI
    QMLTEST
    SOURCES
        tst_qquickmaterialstyle.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickControls2MaterialPrivate
        Qt::QuickControlsTestUtilsPrivate
    TESTDATA ${test_data}
)

# Resources:
set(qmake_immediate_resource_files
    "qtquickcontrols2.conf"
)

qt_internal_add_resource(tst_qquickmaterialstyle "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

# Make the QML files available to Creator's locator.
target_sources(tst_qquickmaterialstyle
    PRIVATE
        ${test_data}
)
