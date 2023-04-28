# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(eventcalendar LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols/eventcalendar")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Sql)

qt_add_executable(eventcalendar
    event.h
    eventcalendar.cpp
    eventmodel.cpp eventmodel.h
    sqleventdatabase.cpp sqleventdatabase.h
)

set_target_properties(eventcalendar PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(eventcalendar PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Sql
)

qt_add_qml_module(eventcalendar
    URI App
    QML_FILES
        MonthGridDelegate.qml
        eventcalendar.qml
    RESOURCES
        qtquickcontrols2.conf
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS eventcalendar
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
