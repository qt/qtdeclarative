# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rendercontrol_rhi LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Widgets)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(rendercontrol_rhi WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(rendercontrol_rhi PRIVATE
    Qt6::Core
    Qt6::GuiPrivate
    Qt6::Quick
    Qt6::Widgets
)

qt_add_qml_module(rendercontrol_rhi
    URI rendercontrolrhiexample
    QML_FILES
        "demo.qml"
    RESOURCE_PREFIX
        "/"
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS rendercontrol_rhi
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET rendercontrol_rhi
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
