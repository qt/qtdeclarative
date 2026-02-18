// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit
import QtQuick.Templates as T

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style: Style {

        //! [color and gradient]
        button {
            background.gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
            }
            background.color: "lightsteelblue"
            hovered.background.color: Qt.darker("lightsteelblue", 1.1)
            pressed.background.color: Qt.darker("lightsteelblue", 1.2)
        }
        //! [color and gradient]
    }

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Button {
                text: "Button"
            }
        }
    }
}
