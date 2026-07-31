// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

// Toggles ShapePath.visible across several frames, changes hidden paths, then
// shows them again in a different order than they were hidden. The grab is the
// final frame: stacking follows declaration order, updates made while hidden
// are visible, and the path left hidden is not drawn.
//
// Rows, top to bottom: CurveRenderer async, CurveRenderer sync,
// GeometryRenderer async, GeometryRenderer sync.
Rectangle {
    id: root
    width: 480
    height: 320
    color: "lightgray"

    property int step: 0
    property bool suspendGrabbing: true

    NumberAnimation on step {
        from: 0
        to: 4
        duration: 1000
        easing.type: Easing.Linear
    }

    onStepChanged: {
        if (step === 4)
            suspendGrabbing = false
    }

    ListModel {
        id: renderers
        ListElement { renderer: Shape.CurveRenderer; async: true }
        ListElement { renderer: Shape.CurveRenderer; async: false }
        ListElement { renderer: Shape.GeometryRenderer; async: true }
        ListElement { renderer: Shape.GeometryRenderer; async: false }
    }

    Column {
        Repeater {
            model: renderers
            Shape {
                preferredRendererType: renderer
                asynchronous: async
                width: root.width
                height: root.height / 4

                // Stroke-only frame, back-most. Hidden at steps 1 and 2, widened while hidden.
                ShapePath {
                    visible: root.step === 0 || root.step >= 3
                    fillColor: "transparent"
                    strokeColor: "black"
                    strokeWidth: root.step >= 2 ? 8 : 3
                    PathRectangle {
                        x: 8
                        y: 8
                        width: 76
                        height: 64
                    }
                }

                // Hidden at steps 1-3. Moved and recolored at step 2, shown again last
                // so a wrong reinsertion paints it in front of the paths below.
                ShapePath {
                    visible: root.step === 0 || root.step >= 4
                    fillColor: root.step >= 2 ? "#800000" : "red"
                    strokeColor: "black"
                    strokeWidth: 2
                    PathRectangle {
                        x: root.step >= 2 ? 40 : 14
                        y: 12
                        width: 34
                        height: 30
                    }
                }

                ShapePath {
                    fillColor: "green"
                    strokeColor: "black"
                    strokeWidth: 2
                    PathRectangle {
                        x: 18
                        y: 16
                        width: 34
                        height: 30
                    }
                }

                ShapePath {
                    fillColor: "blue"
                    strokeColor: "black"
                    strokeWidth: 2
                    PathRectangle {
                        x: 24
                        y: 38
                        width: 34
                        height: 30
                    }
                }

                // Gradient stops change while hidden. Shown at step 3, before red returns.
                ShapePath {
                    visible: root.step === 0 || root.step >= 3
                    strokeColor: "black"
                    strokeWidth: 2
                    fillGradient: LinearGradient {
                        x1: 36
                        y1: 22
                        x2: 70
                        y2: 52
                        GradientStop { position: 0; color: root.step >= 2 ? "white" : "yellow" }
                        GradientStop { position: 1; color: root.step >= 2 ? "orange" : "red" }
                    }
                    PathRectangle {
                        x: 36
                        y: 22
                        width: 34
                        height: 30
                    }
                }

                // Covers the stack at step 1 only.
                ShapePath {
                    visible: root.step === 1
                    fillColor: "magenta"
                    strokeColor: "black"
                    strokeWidth: 2
                    PathRectangle {
                        x: 12
                        y: 10
                        width: 60
                        height: 58
                    }
                }

                // Built while hidden, then revealed under the cyan path.
                ShapePath {
                    visible: root.step >= 4
                    fillColor: "orange"
                    strokeColor: "black"
                    strokeWidth: 2
                    PathRectangle {
                        x: 120
                        y: 22
                        width: 40
                        height: 40
                    }
                }

                ShapePath {
                    fillColor: "cyan"
                    strokeColor: "black"
                    strokeWidth: 2
                    PathRectangle {
                        x: 142
                        y: 12
                        width: 40
                        height: 40
                    }
                }
            }
        }
    }
}
