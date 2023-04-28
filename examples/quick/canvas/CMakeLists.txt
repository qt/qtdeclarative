# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(canvas LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/canvas")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

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
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(canvasexample)
