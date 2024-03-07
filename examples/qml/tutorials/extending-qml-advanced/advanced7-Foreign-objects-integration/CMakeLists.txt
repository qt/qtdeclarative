# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(foreign LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick Gui)
qt_standard_project_setup()

qt_policy(SET QTP0001 NEW)

add_subdirectory(library/)

qt_add_executable(foreign
    birthdayparty.cpp
    birthdayparty.h
    foreigndisplay.h
    happybirthdaysong.cpp
    happybirthdaysong.h
    person.cpp
    person.h
    main.cpp
)

target_link_libraries(foreign PUBLIC
    Qt6::Core
    Qt6::Qml
    Qt6::Gui
    library
)

set_target_properties(foreign PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(foreign PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/library"
)

qt_add_qml_module(foreign
    URI People
    QML_FILES Main.qml
    SOURCES foreigndisplay.h
    DEPENDENCIES
        QtQuick
)

install(TARGETS foreign
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET foreign
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
