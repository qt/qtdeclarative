# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rendercontrol_d3d11 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/rendercontrol/rendercontrol_d3d11")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(rendercontrol_d3d11example WIN32 MACOSX_BUNDLE
    engine.cpp engine.h
    main.cpp
    window.cpp window.h
)

target_link_libraries(rendercontrol_d3d11example PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    d3d11
    dxgi
    dxguid
    uuid
)

# Resources:
qt_add_qml_module(rendercontrol_d3d11example
    URI rendercontrol
    QML_FILES
        "demo.qml"
)

install(TARGETS rendercontrol_d3d11example
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
