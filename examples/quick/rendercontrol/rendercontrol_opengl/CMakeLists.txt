# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rendercontrol_opengl LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(rendercontrol_openglexample WIN32 MACOSX_BUNDLE
    cuberenderer.cpp cuberenderer.h
    main.cpp
    window_singlethreaded.cpp window_singlethreaded.h
)

target_link_libraries(rendercontrol_openglexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::Qml
    Qt6::Quick
)

# Resources:
qt_add_qml_module(rendercontrol_openglexample
    URI rendercontrol
    QML_FILES
        "demo.qml"
)

install(TARGETS rendercontrol_openglexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET rendercontrol_openglexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
