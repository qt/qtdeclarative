// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style: Style {
        button.pressed.background.color: "magenta"
        button.hovered.text.bold: true
    }
    //! [custom control]
    // Main.qml

    component CustomButton : Rectangle {
        StyleReader {
            id: styleReader
            controlType: StyleReader.Button
            hovered: hoverHandler.hovered
            pressed: tapHandler.pressed
            palette: app.palette
        }

        implicitWidth: styleReader.background.implicitWidth
        implicitHeight: styleReader.background.implicitHeight
        color: styleReader.background.color
        radius: styleReader.background.radius

        HoverHandler { id: hoverHandler }
        TapHandler { id: tapHandler }

        Text {
            font: styleReader.font
            anchors.centerIn: parent
            text: "ok"
        }
    }
    //! [custom control]

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            CustomButton {
            }

        }
    }
}
