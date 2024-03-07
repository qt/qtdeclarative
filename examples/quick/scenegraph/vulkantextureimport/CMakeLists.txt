# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(vulkantextureimport LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup()

qt_add_executable(vulkantextureimport WIN32 MACOSX_BUNDLE
    main.cpp
    vulkantextureimport.cpp vulkantextureimport.h
)

set_target_properties(vulkantextureimport PROPERTIES
    # Prevent name clash with build subdirectory on case-insensitive file systems
    OUTPUT_NAME vulkantextureimportapp
)

target_link_libraries(vulkantextureimport PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(vulkantextureimport
    URI VulkanTextureImport
    QML_FILES
        main.qml
    RESOURCES
        squircle.frag.spv
        squircle.vert.spv
    RESOURCE_PREFIX /scenegraph/vulkantextureimport
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS vulkantextureimport
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET vulkantextureimport
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
