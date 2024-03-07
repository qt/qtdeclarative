# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(valuesource LANGUAGES CXX)

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
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET valuesource
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
