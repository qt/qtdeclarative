# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(StyleKitWidgetsExample VERSION 1.0 LANGUAGES CXX)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Widgets LabsStyleKit)

qt_standard_project_setup(REQUIRES 6.11)

qt_add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
    main.cpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE QT_NO_CAST_FROM_ASCII)

qt_add_resources(${PROJECT_NAME} "styles"
    PREFIX "/"
    FILES
        ClassicStyle.qml
        FlatStyle.qml
        NeonStyle.qml
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::LabsStyleKit
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
