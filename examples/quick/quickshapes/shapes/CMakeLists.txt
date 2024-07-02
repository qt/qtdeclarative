# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(shapes LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory("../../shared" "shared")

qt_add_executable(shapesexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(shapesexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

add_dependencies(shapesexample shapes_shared)

qt_add_qml_module(shapesexample
    URI shapes
    QML_FILES
        "clippedtigers.qml"
        "interactive.qml"
        "cubicCurve.qml"
        "ellipticalArcs.qml"
        "gradientSpreadModes.qml"
        "arcDirection.qml"
        "largeOrSmallArc.qml"
        "arcRotation.qml"
        "tigerLoader.qml"
        "text.qml"
        "strokeOrFill.qml"
        "dashPattern.qml"
        "linearGradient.qml"
        "radialGradient.qml"
        "fillRules.qml"
        "joinStyles.qml"
        "capStyles.qml"
        "quadraticCurve.qml"
        "main.qml"
        "sampling.qml"
        "shapegallery.qml"
        "tapableTriangle.qml"
        "tiger.qml"
        "zoomtiger.qml"
        "fillTransform.qml"
        "rectangle.qml"
        "fillItem.qml"
)

install(TARGETS shapesexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET shapesexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
