# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(layoutmirroring LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(tst_manual_layoutmirroring
    GUI
    SOURCES
        layoutmirroring.cpp
    LIBRARIES
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

set(resource_files
    "layoutmirroring.qml"
)


qt_internal_add_resource(tst_manual_layoutmirroring "qmake_immediate"
    PREFIX
        "/qt/qml/layoutmirroring"
    FILES
        ${resource_files}
)
