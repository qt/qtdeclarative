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
    //! [Plain Style]
    // PlainStyle.qml

    Style {
        control {
            // control does not map to an actual Qt Quick Control, but is a shared
            // fallback for all other controls. Use it to define styling that is
            // common to all of them. Unset properties fall back to Style.fallbackStyle.
            padding: 6
            text.color: "white"
            background {
                radius: 4
                border.color: "gray"
            }
            indicator {
                width: 20
                height: 20
                border.width: 1
                radius: 3
            }
            handle {
                width: 20
                height: 20
                radius: 10
            }
        }

        slider {
            // slider defines the styling for a Qt Quick Slider.
            // Unset properties fall back to control.
            handle.color: "white"
            indicator {
                fillWidth: true
                height: 6
                color: "steelblue"
                foreground.color: "skyblue"
            }
        }

        abstractButton {
            // abstractButton does not map to an actual Qt Quick Control, but
            // is a shared fallback for button-like controls, such as button,
            // radioButton, checkBox). Use it to define styling that is common
            // to all of them. Unset properties fall back to control.
            background.shadow {
                opacity: 0.6
                verticalOffset: 2
                horizontalOffset: 2
                color: "gray"
            }
        }

        button {
            // button defines the styling for a Qt Quick Button.
            // Unset properties fall back to abstractButton.
            background {
                width: 120
                color: "lightsteelblue"
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                    GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
                }
            }
        }

        // Controls left undefined — such as radioButton, checkBox, roundButton
        // or switchControl — fall back to their immediate base type, which in
        // this style will be either abstractButton or control directly.
    }
    //! [Plain Style]

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

            Slider {
            }

            CheckBox {
                text: "Button"
                checkable: true
            }
        }
    }
}
