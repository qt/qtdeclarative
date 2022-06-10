# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(fboitem LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/fboitem")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup()

qt_add_executable(fboitem WIN32 MACOSX_BUNDLE
    ../shared/logorenderer.cpp ../shared/logorenderer.h
    fboinsgrenderer.cpp fboinsgrenderer.h
    main.cpp
)

target_include_directories(fboitem PUBLIC
    ../shared
)

target_link_libraries(fboitem PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(fboitem
    URI SceneGraphRendering
    QML_FILES main.qml
    RESOURCES shaders/checker.frag.qsb
    RESOURCE_PREFIX /scenegraph/fboitem
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS fboitem
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
