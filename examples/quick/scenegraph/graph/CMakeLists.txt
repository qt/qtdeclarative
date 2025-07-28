# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(graph LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick ShaderTools)

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
    RESOURCE_PREFIX /scenegraph/graph
    NO_RESOURCE_TARGET_PATH
)

qt6_add_shaders(graph "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/scenegraph/graph"
    FILES
        "shaders/line.vert"
        "shaders/line.frag"
        "shaders/noisy.vert"
        "shaders/noisy.frag"
)

install(TARGETS graph
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET graph
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
