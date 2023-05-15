# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(shadereffects LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/shadereffects")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(shadereffectsexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(shadereffectsexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(shadereffectsexample
    URI shadereffects
    QML_FILES
        "shadereffects.qml"
    RESOURCES
        "content/face-smile.png"
        "content/qt-logo.png"
)

qt6_add_shaders(shadereffectsexample "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt/qml/shadereffects"
    FILES
        "content/shaders/genie.vert"
        "content/shaders/blur.frag"
        "content/shaders/colorize.frag"
        "content/shaders/outline.frag"
        "content/shaders/shadow.frag"
        "content/shaders/wobble.frag"
)

install(TARGETS shadereffectsexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
