# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(attached LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/tutorials/extending-qml-advanced/advanced5-Attached-properties")

find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)
qt_standard_project_setup()

qt_policy(SET QTP0001 NEW)

qt_add_executable(attached
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)

set_target_properties(attached PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(attached PUBLIC
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(attached
    URI People
    QML_FILES Main.qml
    DEPENDENCIES
        QtQuick
)

install(TARGETS attached
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
