# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(animation LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(animationexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

set_target_properties(animationexample PROPERTIES FOLDER "qtc_runnable")

target_link_libraries(animationexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

qt_add_qml_module(animationexample
    URI animation
    QML_FILES
        "LauncherList.qml"
        "SimpleLauncherDelegate.qml"
        "animation.qml"
        "basics/animators.qml"
        "basics/color-animation.qml"
        "basics/property-animation.qml"
        "behaviors/FocusRect.qml"
        "behaviors/SideRect.qml"
        "behaviors/behavior-example.qml"
        "behaviors/tvtennis.qml"
        "behaviors/wigglytext.qml"
        "easing/easing.qml"
        "pathanimation/pathanimation.qml"
        "pathinterpolator/pathinterpolator.qml"
        "states/states.qml"
        "states/transitions.qml"
    RESOURCES
        "images/back.png"
        "images/back@2x.png"
        "images/back@3x.png"
        "images/back@4x.png"
        "basics/images/face-smile.png"
        "basics/images/moon.png"
        "basics/images/shadow.png"
        "basics/images/star.png"
        "basics/images/sun.png"
        "states/qt-logo.png"
)

install(TARGETS animationexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET animationexample
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
