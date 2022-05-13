// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "Buttons"

    Row {
        spacing: container.rowSpacing

        Button {
            id: buttonWithCustomContentItem
            text: "Custom contentItem"
            contentItem: Rectangle {
                implicitWidth: 120
                implicitHeight: il.implicitHeight
                color: buttonWithCustomContentItem.pressed ? "green" : "lightGreen"
                Text {
                    id: il
                    text: buttonWithCustomContentItem.text
                    anchors.centerIn: parent
                }
            }
        }

        Button {
            id: cb
            text: "Custom background"
            background: Rectangle {
                implicitWidth: 200
                implicitHeight: 30
                radius: 5
                color: cb.pressed ? "LightGray" : "gray"
            }
        }

        Button {
            id: cb2
            text: "All custom"
            background: Rectangle {
                implicitWidth: 200
                implicitHeight: 30
                radius: 5
                color: cb2.pressed ? "LightGray" : "gray"
            }
            contentItem: Rectangle {
                implicitWidth: il2.implicitWidth
                implicitHeight: il2.implicitHeight
                radius: 3
                color: "lightgray"
                Text {
                    id: il2
                    text: cb2.text
                    anchors.centerIn: parent
                }
            }
        }
    }
}
