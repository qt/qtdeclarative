# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(customrendernode LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/customrendernode")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_add_executable(customrendernode
    main.cpp
    customrender.cpp customrender.h
)

set_target_properties(customrendernode PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(customrendernode PUBLIC
    Qt::Core
    Qt::GuiPrivate
    Qt::Qml
    Qt::QuickPrivate
)

qt_add_qml_module(customrendernode
    URI SceneGraphRendering
    VERSION 1.0
    QML_FILES
        main.qml
    RESOURCE_PREFIX /scenegraph/customrendernode
    NO_RESOURCE_TARGET_PATH
)

qt6_add_shaders(customrendernode "shaders"
    PREFIX
        "/scenegraph/customrendernode"
    FILES
        "shaders/customrender.vert"
        "shaders/customrender.frag"
)

install(TARGETS customrendernode
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
