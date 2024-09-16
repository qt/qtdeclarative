# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(embeddedinwidgets LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Widgets)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(embeddedinwidgetsexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(embeddedinwidgetsexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Widgets
)

# Resources:
qt_add_qml_module(embeddedinwidgetsexample
    URI embeddedinwidgets
    QML_FILES
        "Main.qml"
    RESOURCES
        "reflect.frag.qsb"
)

install(TARGETS embeddedinwidgetsexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET embeddedinwidgetsexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
