# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(custommaterial_declarative LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_standard_project_setup()

qt_add_executable(custommaterial_declarative WIN32 MACOSX_BUNDLE
    customitem.cpp customitem.h
    main.cpp
)

target_link_libraries(custommaterial_declarative PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(custommaterial_declarative
    URI ExampleCustomMaterial
    QML_FILES
        main.qml
    RESOURCE_PREFIX /scenegraph/custommaterial
    NO_RESOURCE_TARGET_PATH
)

qt6_add_shaders(custommaterial_declarative "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/scenegraph/custommaterial"
    FILES
        "shaders/mandelbrot.vert"
        "shaders/mandelbrot.frag"
)

install(TARGETS custommaterial_declarative
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET custommaterial_declarative
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
