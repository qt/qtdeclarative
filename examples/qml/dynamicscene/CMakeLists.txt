# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(dynamicscene LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/dynamicscene")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(dynamicsceneexample WIN32 MACOSX_BUNDLE main.cpp)

target_link_libraries(dynamicsceneexample PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(dynamicsceneexample
    URI dynamicscene
    QML_FILES
        "GenericSceneItem.qml"
        "PaletteItem.qml"
        "PerspectiveItem.qml"
        "Sun.qml"
        "dynamicscene.qml"
        "itemCreation.js"
    RESOURCES
        "images/NOTE"
        "images/face-smile.png"
        "images/moon.png"
        "images/rabbit_brown.png"
        "images/rabbit_bw.png"
        "images/star.png"
        "images/sun.png"
        "images/tree_s.png"
)

install(TARGETS dynamicsceneexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
