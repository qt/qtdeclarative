# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickrectangleshape LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickrectangleshape
    SOURCES
        tst_qquickrectangleshape.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
        Qt::QuickShapesDesignHelpersPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qquickrectangleshape CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)

qt_internal_extend_target(tst_qquickrectangleshape CONDITION ANDROID OR IOS OR OHOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickrectangleshape CONDITION NOT ANDROID AND NOT IOS AND NOT OHOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_internal_add_resource(tst_qquickrectangleshape "tst_qquickrectangleshape_shared_fillitem"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/.."
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/../data/col320x480.jpg"
)
