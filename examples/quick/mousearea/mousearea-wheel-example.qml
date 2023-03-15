// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    height: 700
    width: 485
    color: "#333333"

    Column {
        anchors.centerIn: parent
        spacing: 2

        Repeater {
            model: ["#9ACD32", "#EEEEEE", "#FFD700", "#87CEEB"]

            Rectangle {
                required property color modelData

                property real scaleFactor: 1

                height: 40 * scaleFactor
                width: 60 * scaleFactor
                color: modelData
                anchors.horizontalCenter: parent.horizontalCenter

                MouseArea {
                    anchors.fill: parent
                    onWheel: (wheel) => {
                        if (wheel.modifiers & Qt.ControlModifier) {
                            parent.scaleFactor += 0.2 * wheel.angleDelta.y / 120
                            if (parent.scaleFactor < 0)
                                parent.scaleFactor = 0
                        }
                    }
                }
            }
        }
    }

    Text {
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        color: "#FFD700"
        text: qsTr("Rotate the mouse wheel pressing <Control> to resize the squares.")
    }
}
