# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    project(textrendering LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

find_package(Qt6 COMPONENTS ShaderTools)

qt_internal_add_manual_test(textrendering
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
)


set(qml_resource_files
    "main.qml"
)

qt_internal_add_resource(textrendering "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

qt_add_qml_module(textrendering
    VERSION 1.0
    URI TextRendering
    RESOURCE_PREFIX /
)
