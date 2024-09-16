# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(shadereffects LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_standard_project_setup(REQUIRES 6.8)

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
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET shadereffectsexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
