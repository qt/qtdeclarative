# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(customrendernode LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/customrendernode")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_standard_project_setup()

qt_add_executable(customrendernode WIN32 MACOSX_BUNDLE
    main.cpp
    customrender.cpp customrender.h
)

target_link_libraries(customrendernode PRIVATE
    Qt6::Core
    Qt6::GuiPrivate
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(customrendernode
    URI SceneGraphRendering
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
