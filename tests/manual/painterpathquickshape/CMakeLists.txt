# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    project(painterPathQuickShape LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

find_package(Qt6 COMPONENTS ShaderTools)

qt_internal_add_manual_test(painterPathQuickShape
    GUI
    SOURCES
        main.cpp
        debugpaintitem.h debugpaintitem.cpp
        svgpathloader.h svgpathloader.cpp
        debugvisualizationcontroller.h debugvisualizationcontroller.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
)


set(qml_resource_files
    "1535737773.svg"
    "main.qml"
    "SvgShape.qml"
    "ControlPanel.qml"
    "ControlPoint.qml"
    "TextShape.qml"
    "SimpleShape.qml"
    "SmallPolygon.qml"
    "Squircle.qml"
    "ControlledShape.qml"
    "Mussel.qml"
    "Graziano.ttf"
    "CubicShape.qml"
    "DashedStroke.qml"
    "hand-print.svg"
    "background.png"
    "arcDirection.qml"
    "arcRotation.qml"
    "capStyles.qml"
    "cubicCurve.qml"
    "dashPattern.qml"
    "ellipticalArcs.qml"
    "fillRules.qml"
    "gradientSpreadModes.qml"
    "joinStyles.qml"
    "largeOrSmallArc.qml"
    "linearGradient.qml"
    "quadraticCurve.qml"
    "radialGradient.qml"
    "strokeOrFill.qml"
    "text.qml"
    "tiger.qml"
)

qt_internal_add_resource(painterPathQuickShape "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

qt_add_shaders(painterPathQuickShape "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/"
    FILES
        "zoombox.frag"
)

qt_add_qml_module(painterPathQuickShape
    VERSION 1.0
    URI InstancingTest
    RESOURCE_PREFIX /
)
