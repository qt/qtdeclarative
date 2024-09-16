# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(canvas LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory("../shared" "shared")

qt_add_executable(canvasexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(canvasexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(canvasexample
    URI canvas
    QML_FILES
        "LabeledSlider.qml"
        "bezierCurve/bezierCurve.qml"
        "canvas.qml"
        "clip/clip.qml"
        "quadraticCurveTo/quadraticCurveTo.qml"
        "roundedrect/roundedrect.qml"
        "smile/smile.qml"
        "squircle/squircle.qml"
        "tiger/tiger.js"
        "tiger/tiger.qml"
    RESOURCES
        "contents/qt-logo.png"
        "squircle/squircle.png"
)

install(TARGETS canvasexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET canvasexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
