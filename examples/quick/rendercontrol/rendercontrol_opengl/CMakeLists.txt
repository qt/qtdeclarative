# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rendercontrol_opengl LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/rendercontrol/rendercontrol_opengl")

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(rendercontrol_openglexample WIN32 MACOSX_BUNDLE
    cuberenderer.cpp cuberenderer.h
    main.cpp
    window_singlethreaded.cpp window_singlethreaded.h
)

target_link_libraries(rendercontrol_openglexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::Qml
    Qt6::Quick
)

# Resources:
qt_add_qml_module(rendercontrol_openglexample
    URI rendercontrol
    QML_FILES
        "demo.qml"
)

install(TARGETS rendercontrol_openglexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
