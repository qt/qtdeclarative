# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rendercontrol_rhi LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/rendercontrol_rhi")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Widgets)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(rendercontrol_rhi WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(rendercontrol_rhi PRIVATE
    Qt6::Core
    Qt6::GuiPrivate
    Qt6::Quick
    Qt6::Widgets
)

qt_add_qml_module(rendercontrol_rhi
    URI rendercontrolrhiexample
    QML_FILES
        "demo.qml"
    RESOURCE_PREFIX
        "/"
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS rendercontrol_rhi
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
