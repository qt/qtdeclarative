# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(openglunderqml LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup()

qt_add_executable(openglunderqml WIN32 MACOSX_BUNDLE
    main.cpp
    squircle.cpp squircle.h
)

set_target_properties(openglunderqml PROPERTIES

    # Prevent name clash with build subdirectory on case-insensitive file systems
    OUTPUT_NAME openglunderqmlapp
)

target_link_libraries(openglunderqml PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(openglunderqml
    URI OpenGLUnderQML
    QML_FILES main.qml
    RESOURCE_PREFIX /scenegraph/openglunderqml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS openglunderqml
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET openglunderqml
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
