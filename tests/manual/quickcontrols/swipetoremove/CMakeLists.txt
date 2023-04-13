# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(swipetoremove LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols/swipetoremove")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml)

qt_add_executable(swipetoremoveexample WIN32 MACOSX_BUNDLE
    swipetoremove.cpp
)
target_link_libraries(swipetoremoveexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
)

# Resources:
qt_add_qml_module(swipetoremoveexample
    URI swipetoremove
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        "swipetoremove.qml"
    RESOURCES
        "fonts/fontello.ttf"
)

install(TARGETS swipetoremoveexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
