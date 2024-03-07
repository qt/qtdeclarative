# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rhiunderqml LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_standard_project_setup()

qt_add_executable(rhiunderqml WIN32 MACOSX_BUNDLE
    main.cpp
    rhisquircle.cpp rhisquircle.h
)

set_target_properties(rhiunderqml PROPERTIES
    # Prevent name clash with build subdirectory on case-insensitive file systems
    OUTPUT_NAME rhiunderqmlapp
)

target_link_libraries(rhiunderqml PRIVATE
    Qt6::Core
    Qt6::GuiPrivate
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(rhiunderqml
    URI RhiUnderQML
    QML_FILES
        main.qml
    RESOURCE_PREFIX /scenegraph/rhiunderqml
    NO_RESOURCE_TARGET_PATH
)

qt_add_shaders(rhiunderqml "rhiunderqml_shaders"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        /scenegraph/rhiunderqml
    BASE
        ../shared
    FILES
        ../shared/squircle_rhi.vert
        ../shared/squircle_rhi.frag
)

install(TARGETS rhiunderqml
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET rhiunderqml
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
