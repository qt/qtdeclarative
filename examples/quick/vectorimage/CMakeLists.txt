# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(vectorimage LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickTools)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(vectorimageexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(vectorimageexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(vectorimageexample
    URI vectorimage
    QML_FILES
        "Main.qml"
    RESOURCES
        "heart.svg"
)

#![svgtoqml]
qt_target_qml_from_svg(vectorimageexample
    CURVE_RENDERER
    OPTIMIZE_PATHS
    FILES heart.svg
    OUTPUTS Heart.qml
    COPYRIGHT_STATEMENT
        "Copyright © 2024 The Qt Company Ltd.\\nSPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause"
)
#![svgtoqml]

install(TARGETS vectorimageexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET vectorimageexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
