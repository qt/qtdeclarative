// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Shapes
import Wearable
import WearableStyle

SwipeViewPage {
    id: clock

    required property real shift
    required property string cityName

    property int hours
    property int minutes
    property int seconds
    property bool night: false
    property bool internationalTime: true //Unset for local time

    function timeChanged() {
        const date = new Date
        hours = internationalTime ? date.getUTCHours() + Math.floor(clock.shift) : date.getHours()
        night = (hours < 7 || hours > 19)
        minutes = internationalTime ? date.getUTCMinutes() + ((clock.shift % 1) * 60) : date.getMinutes()
        seconds = date.getUTCSeconds()
    }

    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: clock.timeChanged()
    }

    Rectangle {

        anchors.centerIn: parent

        id: background
        color: clock.night ? "#000000" : "#FFFFFF"
        radius: width / 2
        width: 200
        height: 200

        Repeater {
            model: 12
            Rectangle {
                id: minuteMarker
                x: parent.width / 2 + 73 ; y: parent.height / 2 - 3
                width: 14
                height: 6
                color: clock.night ? "#FFFFFF" : "#000000"
                antialiasing: true
                required property int index
                transform: Rotation {
                    angle: minuteMarker.index / 12 * 360
                    origin.x: -73
                    origin.y: 3
                }
            }
        }

        Repeater {
            model: 60
            Rectangle {
                id: secondMarker
                x: parent.width / 2 + 90 ; y: parent.height / 2 - 1
                width: 8
                height: 2
                color: clock.night ? "#FFFFFF" : "#000000"
                antialiasing: true
                required property int index
                transform: Rotation {
                    angle: secondMarker.index / 60 * 360
                    origin.x: -90
                    origin.y: 1
                }
            }
        }

        Shape {
            id: hourHand
            x: parent.width / 2
            y: parent.width / 2
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                fillColor: clock.night ? "#FFFFFF" : "#000000"
                strokeWidth:0

                startX: 5
                startY: 14
                PathLine {
                    x: -5
                    y: 14
                }
                PathLine {
                    x: -4
                    y: -71
                }
                PathLine {
                    x: 4
                    y: -71
                }
            }

            transform: Rotation {
                id: hourRotation
                angle: clock.hours * 6 + clock.minutes * 6 / 12
                Behavior on angle {
                    SpringAnimation {
                        spring: 2
                        damping: 0.2
                        modulus: 360
                    }
                }
            }
        }

        Shape {
            id: minuteHand
            x: parent.width / 2
            y: parent.width / 2
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                fillColor: clock.night ? "#FFFFFF" : "#000000"
                strokeWidth:0

                startX: 5
                startY: 14
                PathLine {
                    x: -5
                    y: 14
                }
                PathLine {
                    x: -4
                    y: -89
                }
                PathLine {
                    x: 4
                    y: -89
                }
            }

            transform: Rotation {
                id: minuteRotation
                angle: clock.minutes * 6
                Behavior on angle {
                    SpringAnimation {
                        spring: 2
                        damping: 0.2
                        modulus: 360
                    }
                }
            }
        }

        Shape {
            id: secondHand
            x: parent.width / 2
            y: parent.width / 2
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                fillColor: UIStyle.highlightColor
                strokeWidth: 0

                startX: 1
                startY: 14
                PathLine {
                    x: -1
                    y: 14
                }
                PathLine {
                    x: -1
                    y: -89
                }
                PathLine {
                    x: 1
                    y: -89
                }
            }

            ShapePath {
                fillColor: UIStyle.highlightColor
                strokeWidth: 0

                startX: 0
                startY: 3
                PathArc {
                    x: 0
                    y: -3
                    radiusX: 3
                    radiusY: 3
                }
                PathArc {
                    x: 0
                    y: 3
                    radiusX: 3
                    radiusY: 3
                }
                PathMove {
                    x: 0
                    y: -75
                }
                PathArc {
                    x: 0
                    y: -65
                    radiusX: 5
                    radiusY: 5
                }
                PathArc {
                    x: 0
                    y: -75
                    radiusX: 5
                    radiusY: 5
                }
            }

            transform: Rotation {
                id: secondsRotation
                angle: clock.seconds * 6
                Behavior on angle {
                    SpringAnimation {
                        spring: 2
                        damping: 0.2
                        modulus: 360
                    }
                }
            }
        }
    }

    Text {
        id: cityLabel
        anchors.top: background.bottom
        anchors.margins: 10
        anchors.horizontalCenter: parent.horizontalCenter

        text: parent.cityName
        color: UIStyle.textColor
        font: UIStyle.h2
    }

}
