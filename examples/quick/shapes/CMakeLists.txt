# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(shapes LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/shapes")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory("../shared" "shared")

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
)

install(TARGETS shapesexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(shapesexample)
