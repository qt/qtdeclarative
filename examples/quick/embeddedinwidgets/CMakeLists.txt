# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(embeddedinwidgets LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/embeddedinwidgets")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Widgets)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(embeddedinwidgetsexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(embeddedinwidgetsexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Widgets
)

# Resources:
qt_add_qml_module(embeddedinwidgetsexample
    URI embeddedinwidgets
    QML_FILES
        "main.qml"
    RESOURCES
        "reflect.frag.qsb"
    RESOURCE_PREFIX /embeddedinwidgets
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS embeddedinwidgetsexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
