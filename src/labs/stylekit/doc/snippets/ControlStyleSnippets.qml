// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style:
    //! [ControlStyle]
    Style {
        control {
            padding: 6
            text {
                color: "white"
            }
            background {
                radius: 4
                border.color: "gray"
            }
            indicator {
                implicitWidth: 20
                implicitHeight: 20
                border.width: 1
            }
            handle {
                implicitWidth: 20
                implicitHeight: 20
                radius: 10
            }
        }

        button {
            background {
                implicitWidth: 120
                shadow.opacity: 0.6
                shadow.verticalOffset: 2
                shadow.horizontalOffset: 2
                shadow.color: "gray"
                color: "lightsteelblue"
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                    GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
                }
            }
            hovered {
                background.color: "lightslategrey"
            }
            pressed {
                background.scale: 0.95
            }
        }

        radioButton {
        }

        checkBox {
        }

        slider {
        }

        // etc...
    }
    //! [ControlStyle]

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
                checkable: true
            }
        }
    }
}
