// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256
    Shape {
        width: 200
        height: 150
        anchors.centerIn: parent
        ShapePath {
            strokeWidth: 4
            strokeColor: "red"
            fillGradient: RadialGradient {
                centerX: 100
                centerY: 100
                centerRadius: 100
                SequentialAnimation on focalRadius {
                    loops: Animation.Infinite
                    NumberAnimation {
                        from: 1
                        to: 20
                        duration: 2000
                    }
                    NumberAnimation {
                        from: 20
                        to: 1
                        duration: 2000
                    }
                }
                SequentialAnimation on focalX {
                    loops: Animation.Infinite
                    NumberAnimation {
                        from: 50
                        to: 150
                        duration: 3000
                    }
                    NumberAnimation {
                        from: 150
                        to: 50
                        duration: 3000
                    }
                }
                SequentialAnimation on focalY {
                    loops: Animation.Infinite
                    NumberAnimation {
                        from: 50
                        to: 150
                        duration: 1000
                    }
                    NumberAnimation {
                        from: 150
                        to: 50
                        duration: 1000
                    }
                }
                GradientStop {
                    position: 0
                    color: "#ffffff"
                }
                GradientStop {
                    position: 0.11
                    color: "#f9ffa0"
                }
                GradientStop {
                    position: 0.13
                    color: "#f9ff99"
                }
                GradientStop {
                    position: 0.14
                    color: "#f3ff86"
                }
                GradientStop {
                    position: 0.49
                    color: "#93b353"
                }
                GradientStop {
                    position: 0.87
                    color: "#264619"
                }
                GradientStop {
                    position: 0.96
                    color: "#0c1306"
                }
                GradientStop {
                    position: 1
                    color: "#000000"
                }
            }
            fillColor: "blue" // ignored with the gradient set
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 4 ]
            startX: 20
            startY: 20
            PathLine {
                x: 180
                y: 130
            }
            PathLine {
                x: 20
                y: 130
            }
            PathLine {
                x: 20
                y: 20
            }
        }
    }
}
