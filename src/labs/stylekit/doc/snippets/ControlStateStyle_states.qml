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
    Style {
        //! [States]
        button {
            text.color: "white"
            background.color: "cornflowerblue"

            pressed.background.color: "cadetblue"
            hovered.background.color: "dodgerblue"
            highlighted.background.color: "lightblue"
            focused.background.color: "lightskyblue"
            checked.background.color: "darkseagreen"
            disabled.background.color: "gray"

            // hovered.checked takes precedence over both hovered and checked
            hovered.checked.background.color: "mediumseagreen"

            hovered.checked {
                // Nested states are grouped properties, so you can use the compact
                // per-property form above, or structure them hierarchically for
                // better readability. Both forms are functionally equivalent.
                pressed {
                    // hovered.checked.pressed takes precedence over hovered.checked
                    background {
                        color: "mediumaquamarine"
                        scale: 0.95
                    }
                    text {
                        bold: true
                    }
                }
            }
        }
        //! [States]
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
                checkable: true
            }
        }
    }
}
