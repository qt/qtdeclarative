# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(extended LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/referenceexamples/extended")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Widgets)

qt_add_executable(extended
    lineedit.cpp lineedit.h
    main.cpp
)

set_target_properties(extended PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(extended PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Widgets
)

qt_add_qml_module(extended
    URI People
    QML_FILES example.qml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS extended
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
