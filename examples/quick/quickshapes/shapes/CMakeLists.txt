# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(shapes LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(shapesexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

set_target_properties(shapesexample PROPERTIES FOLDER "qtc_runnable")

target_link_libraries(shapesexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

qt_add_qml_module(shapesexample
    URI shapes
    QML_FILES
        "LauncherList.qml"
        "SimpleLauncherDelegate.qml"
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
        "cornershapes.qml"
        "fillItem.qml"
        "cosmetic.qml"
    RESOURCES
        "images/back.png"
        "images/back@2x.png"
        "images/back@3x.png"
        "images/back@4x.png"
        "images/qt-logo.png"
)

install(TARGETS shapesexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET shapesexample
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
