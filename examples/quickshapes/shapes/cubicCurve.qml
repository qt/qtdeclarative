// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Item {
        width: 200
        height: 200
        anchors.centerIn: parent

        Shape {
            id: shape
            anchors.fill: parent

            ShapePath {
                strokeWidth: 4
                strokeColor: "black"
                fillGradient: ConicalGradient {
                    id: conGrad
                    centerX: 100
                    centerY: 75
                    NumberAnimation on angle {
                        from: 0
                        to: 360
                        duration: 10000
                        loops: Animation.Infinite
                    }
                    GradientStop {
                        position: 0
                        color: "#00000000"
                    }
                    GradientStop {
                        position: 0.10
                        color: "#ffe0cc73"
                    }
                    GradientStop {
                        position: 0.17
                        color: "#ffc6a006"
                    }
                    GradientStop {
                        position: 0.46
                        color: "#ff600659"
                    }
                    GradientStop {
                        position: 0.72
                        color: "#ff0680ac"
                    }
                    GradientStop {
                        position: 0.92
                        color: "#ffb9d9e6"
                    }
                    GradientStop {
                        position: 1.00
                        color: "#00000000"
                    }
                }

                startX: 50
                startY: 100
                PathCubic {
                    x: 150
                    y: 100
                    control1X: cp1.x
                    control1Y: cp1.y
                    control2X: cp2.x
                    control2Y: cp2.y
                }
            }
        }

        Rectangle {
            id: cp1
            color: "red"
            width: 10
            height: 10
            SequentialAnimation {
                loops: Animation.Infinite
                running: true
                NumberAnimation {
                    target: cp1
                    property: "x"
                    from: 0
                    to: shape.width - cp1.width
                    duration: 5000
                }
                NumberAnimation {
                    target: cp1
                    property: "x"
                    from: shape.width - cp1.width
                    to: 0
                    duration: 5000
                }
                NumberAnimation {
                    target: cp1
                    property: "y"
                    from: 0
                    to: shape.height - cp1.height
                    duration: 5000
                }
                NumberAnimation {
                    target: cp1
                    property: "y"
                    from: shape.height - cp1.height
                    to: 0
                    duration: 5000
                }
            }
        }

        Rectangle {
            id: cp2
            color: "blue"
            width: 10
            height: 10
            x: shape.width - width
            SequentialAnimation {
                loops: Animation.Infinite
                running: true
                NumberAnimation {
                    target: cp2
                    property: "y"
                    from: 0
                    to: shape.height - cp2.height
                    duration: 5000
                }
                NumberAnimation {
                    target: cp2
                    property: "y"
                    from: shape.height - cp2.height
                    to: 0
                    duration: 5000
                }
                NumberAnimation {
                    target: cp2
                    property: "x"
                    from: shape.width - cp2.width
                    to: 0
                    duration: 5000
                }
                NumberAnimation {
                    target: cp2
                    property: "x"
                    from: 0
                    to: shape.width - cp2.width
                    duration: 5000
                }
            }
        }
    }

    Text {
        anchors {
            right: parent.right
            top: parent.top
        }
        text: qsTr("Conical gradient angle: ") + Math.round(conGrad.angle)
    }
}
