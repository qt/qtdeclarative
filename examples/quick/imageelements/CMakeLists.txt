# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(imageelements LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(imageelementsexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(imageelementsexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

qt_add_qml_module(imageelementsexample
    URI imageelements
    QML_FILES
        "LauncherList.qml"
        "SimpleLauncherDelegate.qml"
        "animatedimage.qml"
        "animatedsprite.qml"
        "borderimage.qml"
        "framestepping.qml"
        "image.qml"
        "imageelements.qml"
        "multiframeborderimage.qml"
        "shadows.qml"
        "spritesequence.qml"
        "BorderImageSelector.qml"
        "ImageCell.qml"
        "MyBorderImage.qml"
        "ShadowRectangle.qml"
    RESOURCES
        "images/back.png"
        "images/back@2x.png"
        "images/back@3x.png"
        "images/back@4x.png"
        "pics/qt-logo.png"
        "pics/shadow.png"
        "pics/speaker.png"
        "pics/colors.png"
        "pics/BearSheet.png"
        "pics/Uniflow_steam_engine.gif"
        "pics/arrow.png"
        "pics/bw.png"
        "pics/multi.ico"
        "pics/colors-round.sci"
        "pics/colors-stretch.sci"
    )

install(TARGETS imageelementsexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET imageelementsexample
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
