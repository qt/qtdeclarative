# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rhitextureitem LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/rhitextureitem")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_standard_project_setup()

qt_add_executable(rhitextureitem WIN32 MACOSX_BUNDLE
    rhitextureitem.cpp rhitextureitem.h
    main.cpp
)

target_include_directories(rhitextureitem PUBLIC
    ../shared
)

target_link_libraries(rhitextureitem PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickPrivate
)

qt_add_qml_module(rhitextureitem
    URI SceneGraphRendering
    QML_FILES main.qml
    RESOURCE_PREFIX /scenegraph/rhitextureitem
    NO_RESOURCE_TARGET_PATH
)

qt_add_shaders(rhitextureitem "rhitextureitem_shaders"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        /scenegraph/rhitextureitem
    FILES
        shaders/logo.vert
        shaders/logo.frag
)

qt_add_shaders(rhitextureitem "rhitextureitem_effect_shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        /scenegraph/rhitextureitem
    FILES
        shaders/checker.frag
)

install(TARGETS rhitextureitem
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
