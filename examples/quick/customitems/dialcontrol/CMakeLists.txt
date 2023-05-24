# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(dialcontrol LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/customitems/dialcontrol")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(dialcontrolexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(dialcontrolexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(dialcontrolexample
    URI dialcontrol
    QML_FILES
        "Dial.qml"
        "QuitButton.qml"
        "dialcontrol.qml"
    RESOURCES
        "background.png"
        "needle.png"
        "needle_shadow.png"
        "overlay.png"
        "quit.png"
)

install(TARGETS dialcontrolexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
