# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rhitextureitem LANGUAGES CXX)

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
)

qt_add_qml_module(rhitextureitem
    URI
        SceneGraphRendering
    QML_FILES
        main.qml
        SettingsDrawer.qml
    RESOURCE_PREFIX
        /scenegraph/rhitextureitem
    RESOURCES
        icon_settings.png
        icon_settings@2x.png
        icon_settings@3x.png
        icon_settings@4x.png
    NO_RESOURCE_TARGET_PATH
)

qt_add_shaders(rhitextureitem "rhitextureitem_shaders"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        /scenegraph/rhitextureitem
    FILES
        shaders/color.vert
        shaders/color.frag
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
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET rhitextureitem
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
