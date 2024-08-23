// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes
import io.qt

Item {
    id: topLevel
    property alias capStyle: shapePath.capStyle
    property alias dashOffset: shapePath.dashOffset
    property alias dashPattern: shapePath.dashPattern
    property alias fillColor: shapePath.fillColor
    property alias fillRule: shapePath.fillRule
    property alias strokeColor: shapePath.strokeColor
    property alias strokeStyle: shapePath.strokeStyle
    property alias strokeWidth: shapePath.strokeWidth
    property alias shapeTransform: shape.transform

    property alias startX: shapePath.startX
    property alias startY: shapePath.startY

    property rect boundingRect: shape.boundingRect

    width: boundingRect.width
    height: boundingRect.height

    property vector2d startPoint: "0,0"
    property list<QtObject> delegate

    LinearGradient {
        id: linearGradient
        x1: shape.boundingRect.left
        y1: shape.boundingRect.top
        x2: shape.boundingRect.right
        y2: shape.boundingRect.bottom
        GradientStop { position: 0; color: fillColor }
        GradientStop { position: 1; color: Qt.rgba(1 - fillColor.r, 1 - fillColor.g, 1 - fillColor.b, 1) }
    }

    RadialGradient {
        id: radialGradient
        centerX: 0.5 * (shape.boundingRect.right + shape.boundingRect.left)
        centerY: 0.5 * (shape.boundingRect.top + shape.boundingRect.bottom)
        focalX: centerX
        focalY: centerY
        centerRadius: 0.5 * (shape.boundingRect.right - shape.boundingRect.left)
        focalRadius: 0.1
        GradientStop { position: 0.0; color: fillColor }
        GradientStop { position: 1.0; color: Qt.rgba(1 - fillColor.r, 1 - fillColor.g, 1 - fillColor.b, 1) }
    }

    ConicalGradient {
        id: conicalGradient
        centerX: 0.5 * (shape.boundingRect.right + shape.boundingRect.left)
        centerY: 0.5 * (shape.boundingRect.top + shape.boundingRect.bottom)
        GradientStop { position: 0.0; color: fillColor }
        GradientStop { position: 0.5; color: Qt.rgba(1 - fillColor.r, 1 - fillColor.g, 1 - fillColor.b, 1) }
        GradientStop { position: 1.0; color: fillColor }
    }

    property var gradients: [ null, linearGradient, radialGradient, conicalGradient ]

    Item {
        transform: [
            Scale {
                xScale: controlPanel.scale
                yScale: controlPanel.scale
            }
        ]
        Shape {
            id: shape
            x: 0
            y: 0
            asynchronous: controlPanel.async
            preferredRendererType: controlPanel.preferCurve ? Shape.CurveRenderer : Shape.UnknownRenderer
            onRendererTypeChanged: {
                controlPanel.rendererName = rendererType == Shape.SoftwareRenderer ? "Software" :
                                                                                     rendererType == Shape.GeometryRenderer ? "Geometry" :
                                                                                                                              rendererType == Shape.CurveRenderer ? "Curve" : "Unknown";
            }

            ShapePath {
                id: shapePath
                fillRule: controlPanel.fillRule
                fillGradient: gradients[controlPanel.gradientType]
                strokeColor: controlPanel.outlineColor
                fillColor: controlPanel.fillColor
                strokeWidth: controlPanel.outlineWidth
                strokeStyle: controlPanel.outlineStyle
                joinStyle: controlPanel.joinStyle
                capStyle: controlPanel.capStyle
                fillTransform: Qt.matrix4x4(controlPanel.fillScaleX,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)
            }

            Repeater {
                model: topLevel.delegate
                onModelChanged: {
                    shapePath.pathElements = []
                    for (var i = 0; i < model.length; ++i)
                        shapePath.pathElements.push(model[i])
                }
            }
        }
    }
    Connections {
        target: controlPanel
        function onPathChanged() {
            debugPaintPath.update()
        }
    }

    DebugPaintItem {
        id: debugPaintPath
        shape: shapePath
        visible: controlPanel.painterComparison > 0
        color: controlPanel.painterComparisonColor
        opacity: controlPanel.painterComparisonAlpha
        z: controlPanel.painterComparison > 1 ? -1 : 0
        pathScale: controlPanel.scale
        fillRule: topLevel.fillRule
        strokeStyle: topLevel.strokeStyle
        strokeColor: controlPanel.outlineEnabled ? Qt.rgba(1 - color.r, 1 - color.g, 1 - color.b, 1) : "transparent"
        capStyle: topLevel.capStyle
        joinStyle: controlPanel.joinStyle
        strokeWidth: topLevel.strokeWidth

        width: visible ? (shape.boundingRect.width + shape.boundingRect.x) * controlPanel.scale : 1
        height: visible ? (shape.boundingRect.height + shape.boundingRect.y) * controlPanel.scale : 1
    }

    DebugVisualizationController {
        showCurves: controlPanel.debugCurves
        showWireframe: controlPanel.debugWireframe
        onSettingsChanged: { shapePath.changed() }
    }
}
