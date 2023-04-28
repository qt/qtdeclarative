# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(graph LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/graph")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)

qt_standard_project_setup()

qt_add_executable(graph WIN32 MACOSX_BUNDLE
    graph.cpp graph.h
    gridnode.cpp gridnode.h
    linenode.cpp linenode.h
    main.cpp
    noisynode.cpp noisynode.h
)

set_target_properties(graph PROPERTIES
    # Prevent name clash with build subdirectory on case-insensitive file systems
    OUTPUT_NAME graphapp
)

target_link_libraries(graph PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

qt_add_qml_module(graph
    URI Graph
    QML_FILES
        main.qml
    RESOURCES
        shaders/line.frag.qsb
        shaders/line.vert.qsb
        shaders/noisy.frag.qsb
        shaders/noisy.vert.qsb
    RESOURCE_PREFIX /scenegraph/graph
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS graph
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
