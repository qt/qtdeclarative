# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickvectorimage LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickvectorimage
    SOURCES
        tst_qquickvectorimage.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA
        ${test_data}
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

file(GLOB_RECURSE test_data_glob_svg
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../baseline/scenegraph/data/shared/svg/*)
list(APPEND svg_data ${test_data_glob_svg})

qt_add_resources(tst_qquickvectorimage svgs
    PREFIX /svgs
    BASE ${CMAKE_CURRENT_SOURCE_DIR}/../../../baseline/scenegraph/data/shared/svg/
    FILES
        ${svg_data}
)

qt_internal_extend_target(tst_qquickvectorimage CONDITION ANDROID OR IOS
    DEFINES
    QT_QMLTEST_DATADIR=":/data"
)
