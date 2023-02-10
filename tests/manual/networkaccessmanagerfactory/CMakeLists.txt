# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(networkaccessmanagerfactory LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/networkaccessmanagerfactory")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Qml Quick)

qt_add_executable(networkaccessmanagerfactory
    main.cpp
)

set_target_properties(networkaccessmanagerfactory PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(networkaccessmanagerfactory PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(networkaccessmanagerfactory
    URI Example
    QML_FILES view.qml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS networkaccessmanagerfactory
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
