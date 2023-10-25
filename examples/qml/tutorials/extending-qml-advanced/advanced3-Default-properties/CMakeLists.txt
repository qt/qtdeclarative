# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(default LANGUAGES CXX)

if (ANDROID)
    message(FATAL_ERROR "This project cannot be built on Android.")
endif()

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/tutorials/extending-qml-advanced/advanced3-Default-properties")

find_package(Qt6 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup()

qt_policy(SET QTP0001 NEW)

qt_add_executable(default
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)

set_target_properties(default PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(default PUBLIC
    Qt6::Core
    Qt6::Qml
)

qt_add_qml_module(default
    URI People
    QML_FILES Main.qml
)

install(TARGETS default
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
