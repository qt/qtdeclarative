# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(d3d11underqml LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/d3d11underqml")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(d3d11underqml
    d3d11squircle.cpp d3d11squircle.h
    main.cpp
)

set_target_properties(d3d11underqml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(d3d11underqml PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    d3d11
    d3dcompiler
)

qt_add_qml_module(d3d11underqml
    URI D3D11UnderQML
    QML_FILES
        main.qml
    RESOURCES
        squircle.frag
        squircle.vert
    RESOURCE_PREFIX /scenegraph/d3d11underqml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS d3d11underqml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
