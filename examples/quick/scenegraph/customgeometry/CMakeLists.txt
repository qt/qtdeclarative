# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(customgeometry_declarative LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/customgeometry")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)

qt_standard_project_setup()

qt_add_executable(customgeometry_declarative WIN32 MACOSX_BUNDLE
    beziercurve.cpp beziercurve.h
    main.cpp
)

target_link_libraries(customgeometry_declarative PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

qt_add_qml_module(customgeometry_declarative
    URI CustomGeometry
    QML_FILES main.qml
    RESOURCE_PREFIX /scenegraph/customgeometry
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS customgeometry_declarative
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
