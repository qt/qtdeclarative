# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(valuesource LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/tutorials/extending-qml-advanced/advanced6-Property-value-source")

find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)
qt_standard_project_setup()

qt_policy(SET QTP0001 NEW)

qt_add_executable(valuesource
    birthdayparty.cpp birthdayparty.h
    happybirthdaysong.cpp happybirthdaysong.h
    main.cpp
    person.cpp person.h
)

set_target_properties(valuesource PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(valuesource PUBLIC
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(valuesource
    URI People
    QML_FILES Main.qml
    DEPENDENCIES
        QtQuick
)

install(TARGETS valuesource
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
