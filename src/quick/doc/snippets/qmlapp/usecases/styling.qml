// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Controls.Styles

ApplicationWindow {

    //![0]
    Button {
        text: qsTr("Hello World")
        style: ButtonStyle {
            background: Rectangle {
                implicitWidth: 100
                implicitHeight: 25
                border.width: control.activeFocus ? 2 : 1
                border.color: "#FFF"
                radius: 4
                gradient: Gradient {
                    GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#fff" }
                    GradientStop { position: 1 ; color: control.pressed ? "#000" : "#fff" }
                }
            }
       }
    //![0]
    }
}
