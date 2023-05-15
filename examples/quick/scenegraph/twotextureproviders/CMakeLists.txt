# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(twotextureproviders LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/twotextureproviders")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_standard_project_setup()

qt_add_executable(twotextureproviders WIN32 MACOSX_BUNDLE
    main.cpp
    xorblender.cpp xorblender.h
)

target_link_libraries(twotextureproviders PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(twotextureproviders
    URI SceneGraphRendering
    QML_FILES
        main.qml
    RESOURCE_PREFIX /scenegraph/twotextureproviders
    NO_RESOURCE_TARGET_PATH
)

qt6_add_shaders(twotextureproviders "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/scenegraph/twotextureproviders"
    FILES
        "shaders/xorblender.vert"
        "shaders/xorblender.frag"
        "shaders/checker.frag"
)

install(TARGETS twotextureproviders
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
