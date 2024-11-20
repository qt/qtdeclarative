// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias control: control
    property alias textArea: textArea
    property alias textField: textField

    MouseArea {
        anchors.fill: parent

        Column {
            Control {
                id: control
                implicitWidth: 100
                implicitHeight: 30
                hoverEnabled: true
                background: Rectangle {
                    color: control.hovered ? "salmon" : "grey"
                }
            }

            TextArea {
                id: textArea
                hoverEnabled: true
            }

            TextField {
                id: textField
                hoverEnabled: true
            }
        }
    }
}
