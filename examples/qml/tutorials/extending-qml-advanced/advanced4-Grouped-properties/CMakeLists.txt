# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(grouped LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/tutorials/extending-qml-advanced/advanced4-Grouped-properties")

find_package(Qt6 REQUIRED COMPONENTS Core Qml Gui)
qt_standard_project_setup()

qt_policy(SET QTP0001 NEW)

qt_add_executable(grouped
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)

set_target_properties(grouped PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(grouped PUBLIC
    Qt6::Core
    Qt6::Qml
    Qt6::Gui
)

qt_add_qml_module(grouped
    URI People
    QML_FILES Main.qml
    DEPENDENCIES
        QtQuick
)

install(TARGETS grouped
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
