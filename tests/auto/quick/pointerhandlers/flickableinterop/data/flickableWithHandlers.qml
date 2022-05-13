// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Rectangle {
    id: root
    width: 800
    height: 480
    objectName: "root"
    color: "#222222"

    Flickable {
        anchors.fill: parent
        anchors.margins: 10
        anchors.topMargin: 40
        contentHeight: 600
        contentWidth: 1000
//        pressDelay: TODO

        Row {
            spacing: 6
            KnobDragSlider {
                label: "Slider with\nDH on knob"
                objectName: "knobSlider"
                value: 49; width: 120; height: 400
            }
            GrooveDragSlider {
                label: "Slider with\nDH on root"
                objectName: "grooveSlider"
                value: 49; width: 120; height: 400
            }
            Column {
                spacing: 6
                TapHandlerButton {
                    objectName: "DragThreshold"
                    label: "DragThreshold"
                    gesturePolicy: TapHandler.DragThreshold
                }
                TapHandlerButton {
                    objectName: "WithinBounds"
                    label: "WithinBounds"
                    gesturePolicy: TapHandler.WithinBounds
                }
                TapHandlerButton {
                    objectName: "ReleaseWithinBounds"
                    label: "ReleaseWithinBounds"
                    gesturePolicy: TapHandler.ReleaseWithinBounds // the default
                }
            }
            Column {
                spacing: 6
                Rectangle {
                    width: 50
                    height: 50
                    color: "aqua"
                    border.color: drag1.active ? "darkgreen" : "transparent"
                    border.width: 3
                    objectName: "drag"
                    DragHandler {
                        id: drag1
                        objectName: "drag1"
                    }
                    Text {
                        anchors.centerIn: parent
                        enabled: false
                        text: "drag"
                    }
                }
                Rectangle {
                    width: 50
                    height: 50
                    color: "aqua"
                    objectName: "tap"
                    border.color: tap1.isPressed ? "red" : "transparent"
                    border.width: 3
                    TapHandler {
                        id: tap1
                        objectName: "tap1"
                        gesturePolicy: TapHandler.DragThreshold
                    }
                    Text {
                        anchors.centerIn: parent
                        enabled: false
                        text: "tap"
                    }
                }
                Rectangle {
                    width: 50
                    height: 50
                    color: "aqua"
                    border.color: tap2.isPressed ? "red" : drag2.active ? "darkgreen" : "transparent"
                    border.width: 3
                    objectName: "dragAndTap"
                    DragHandler {
                        id: drag2
                        objectName: "drag2"
                    }
                    TapHandler {
                        id: tap2
                        objectName: "tap2"
                        gesturePolicy: TapHandler.DragThreshold
                    }
                    Text {
                        anchors.centerIn: parent
                        enabled: false
                        text: "drag\nand\ntap"
                    }
                }
                Rectangle {
                    width: 50
                    height: 50
                    color: "aqua"
                    border.color: tap3.isPressed ? "red" : drag3.active ? "darkgreen" : "transparent"
                    border.width: 3
                    objectName: "tapAndDrag"
                    TapHandler {
                        id: tap3
                        objectName: "tap3"
                        gesturePolicy: TapHandler.DragThreshold
                    }
                    DragHandler {
                        id: drag3
                        objectName: "drag3"
                    }
                    Text {
                        anchors.centerIn: parent
                        enabled: false
                        text: "tap\nand\ndrag"
                    }
                }
            }
        }
    }
}

