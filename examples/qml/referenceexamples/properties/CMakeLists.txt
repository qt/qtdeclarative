# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(properties LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/referenceexamples/properties")

find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_executable(properties
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)

set_target_properties(properties PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(properties PUBLIC
    Qt::Core
    Qt::Qml
)

qt_add_qml_module(properties
    URI People
    QML_FILES example.qml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS properties
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
