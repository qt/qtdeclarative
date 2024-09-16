# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(maskedmousearea LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/customitems/maskedmousearea")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(maskedmousearea
    WIN32
    MACOSX_BUNDLE
    main.cpp
    maskedmousearea.cpp maskedmousearea.h
)

target_link_libraries(maskedmousearea PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(maskedmousearea
    URI Example
    QML_FILES
        maskedmousearea.qml
    RESOURCES
        images/cloud_1.png
        images/cloud_2.png
        images/moon.png
    RESOURCE_PREFIX /customitems/maskedmousearea
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS maskedmousearea
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
