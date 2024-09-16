// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls

Window {
    id: window
    visible: true

    width: grid.implicitWidth
    height: grid.implicitHeight

    title: "Window (" + grid.width + "x" + grid.height + ")"

    GridLayout {
        id: grid
        columns: 3
        rows: 3
        Layout.fillWidth: true
        Layout.fillHeight: true

        uniformCellWidths: true

        Repeater {
            model: 3
            Rectangle {
                color: "#243a5e"
                implicitWidth: 300
                implicitHeight: 300
                opacity: implicitWidth/600/2 + implicitHeight/600/2
                Layout.fillWidth: true
                Layout.fillHeight: true

                Layout.maximumWidth: 1000
                Layout.maximumHeight: 1000

                Layout.minimumWidth: 20
                Layout.minimumHeight: 20

                Layout.alignment: Qt.AlignCenter
                Text {
                    id: sizeText
                    anchors.centerIn: parent
                    text: "min :" + parent.Layout.minimumWidth + "x" + parent.Layout.minimumHeight
                }
                Text {
                    id: sizeText2
                    anchors.top: sizeText.bottom
                    anchors.horizontalCenter: sizeText.horizontalCenter
                    text: "want:" + parent.implicitWidth + "x" + parent.implicitHeight
                }
                Text {
                    id: sizeText3
                    anchors.top: sizeText2.bottom
                    anchors.horizontalCenter: sizeText2.horizontalCenter
                    text: "size :" + parent.width + "x" + parent.height
                }
                Text {
                    anchors.top: sizeText3.bottom
                    anchors.horizontalCenter: sizeText3.horizontalCenter
                    text: "max :" + parent.Layout.maximumWidth + "x" + parent.Layout.maximumHeight
                }
                Text {
                    anchors.bottom: sizeText.top
                    anchors.horizontalCenter: sizeText.horizontalCenter
                    text: index
                    font.pointSize: 14
                }
                WheelHandler {
                    acceptedModifiers: Qt.NoModifier
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    onWheel: (event)=> {
                                if (event.angleDelta.y > 0)
                                    implicitWidth += 5
                                else if (implicitWidth > 50)
                                    implicitWidth -= 5
                    }
                }
                WheelHandler {
                    acceptedModifiers: Qt.ShiftModifier
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    onWheel: (event)=> {
                                if (event.angleDelta.y > 0)
                                    parent.Layout.minimumWidth = Math.min(parent.Layout.minimumWidth + 5, parent.Layout.maximumWidth)
                                else
                                    parent.Layout.minimumWidth = Math.max(parent.Layout.minimumWidth - 5, 0)
                    }
                }
                WheelHandler {
                    acceptedModifiers: Qt.ControlModifier
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    onWheel: (event)=> {
                                 if (event.angleDelta.y > 0)
                                     parent.Layout.maximumWidth = Math.min(parent.Layout.maximumWidth + 5, 2500)
                                 else
                                     parent.Layout.maximumWidth = Math.max(parent.Layout.maximumWidth - 5, parent.Layout.minimumWidth)
                    }
                }
            }
        }
    }

    onWidthChanged: {
        console.log("Preferred Size:", grid.implicitWidth, "x", grid.implicitHeight)
        console.log("  Minimum Size:", grid.Layout.minimumWidth, "x", grid.Layout.minimumHeight)
        console.log("  Maximum Size:", grid.Layout.maximumWidth, "x", grid.Layout.maximumHeight)
    }
}
