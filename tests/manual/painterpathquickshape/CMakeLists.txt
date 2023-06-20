# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

qt_add_qml_module(painterPathQuickShape
    VERSION 1.0
    URI InstancingTest
    RESOURCE_PREFIX /
)
