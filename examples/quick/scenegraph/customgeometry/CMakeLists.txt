# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(customgeometry_declarative LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)

qt_standard_project_setup()

qt_add_executable(customgeometry_declarative WIN32 MACOSX_BUNDLE
    beziercurve.cpp beziercurve.h
    main.cpp
)

target_link_libraries(customgeometry_declarative PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

qt_add_qml_module(customgeometry_declarative
    URI CustomGeometry
    QML_FILES main.qml
    RESOURCE_PREFIX /scenegraph/customgeometry
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS customgeometry_declarative
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET customgeometry_declarative
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
