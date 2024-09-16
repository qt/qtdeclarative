# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(chapter1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(chattutorial-chapter1 WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(chattutorial-chapter1 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(chattutorial-chapter1
    URI chattutorial
    QML_FILES
        "Main.qml"
)

qt6_add_resources(chattutorial-chapter1 "conf"
    PREFIX
        "/"
    FILES
        "qtquickcontrols2.conf"
)

install(TARGETS chattutorial-chapter1
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET chattutorial-chapter1
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
