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
            text.color: "aliceblue"
            background.color: "cornflowerblue"
            pressed.background.color: "deepskyblue"
            hovered.background.color: "lightskyblue"
            focused.background.color: "lightsteelblue"
            checked.background.color: "royalblue"
            highlighted.background.color: "lightblue"
            disabled {
                background.color: "lightgray"
                background.border.color: "darkgray"
            }

            // Nested states, such as hovered.checked in this case, takes
            // precedence over both hovered and checked:
            hovered.checked.background.color: "steelblue"
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

            Button {
                text: "Disabled"
                enabled: false
            }

            CheckBox {
                text: "CheckBox"
            }
        }
    }
}
