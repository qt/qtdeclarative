# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(chapter5 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Sql)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(chattutorial-chapter5 WIN32 MACOSX_BUNDLE
    main.cpp
    sqlcontactmodel.cpp sqlcontactmodel.h
    sqlconversationmodel.cpp sqlconversationmodel.h
)

target_link_libraries(chattutorial-chapter5 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Sql
)

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(chattutorial-chapter5
    URI chattutorial
    QML_FILES
        "+Material/ChatToolBar.qml"
        "ChatToolBar.qml"
        "ContactPage.qml"
        "ConversationPage.qml"
        "Main.qml"
    RESOURCES
        "images/Albert_Einstein.png"
        "images/Albert_Einstein@2x.png"
        "images/Albert_Einstein@3x.png"
        "images/Albert_Einstein@4x.png"
        "images/Ernest_Hemingway.png"
        "images/Ernest_Hemingway@2x.png"
        "images/Ernest_Hemingway@3x.png"
        "images/Ernest_Hemingway@4x.png"
        "images/Hans_Gude.png"
        "images/Hans_Gude@2x.png"
        "images/Hans_Gude@3x.png"
        "images/Hans_Gude@4x.png"
)

qt6_add_resources(chattutorial-chapter5 "conf"
    PREFIX
        "/"
    FILES
        "qtquickcontrols2.conf"
)

install(TARGETS chattutorial-chapter5
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET chattutorial-chapter5
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
